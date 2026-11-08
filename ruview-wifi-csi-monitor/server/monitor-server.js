'use strict';

const dgram = require('node:dgram');
const crypto = require('node:crypto');
const fs = require('node:fs');
const http = require('node:http');
const path = require('node:path');

const CSI_MAGIC = 0xc5110001;
const HEADER_SIZE = 20;

function signedByte(value) {
  return value > 127 ? value - 256 : value;
}

function parseCsiFrame(buffer) {
  if (!Buffer.isBuffer(buffer) || buffer.length < HEADER_SIZE) return null;
  if (buffer.readUInt32LE(0) !== CSI_MAGIC) return null;

  const nodeId = buffer.readUInt8(4);
  const antennas = buffer.readUInt8(5);
  const subcarriers = buffer.readUInt16LE(6);
  const pairCount = antennas * subcarriers;
  const expectedLength = HEADER_SIZE + pairCount * 2;
  if (antennas === 0 || subcarriers === 0 || buffer.length < expectedLength) return null;

  const amplitudes = new Array(pairCount);
  for (let index = 0; index < pairCount; index += 1) {
    const i = signedByte(buffer[HEADER_SIZE + index * 2]);
    const q = signedByte(buffer[HEADER_SIZE + index * 2 + 1]);
    amplitudes[index] = Math.hypot(i, q);
  }

  const rawRssi = signedByte(buffer[16]);
  return {
    nodeId,
    antennas,
    subcarriers,
    frequencyMhz: buffer.readUInt32LE(8),
    sequence: buffer.readUInt32LE(12),
    rssi: rawRssi > 0 ? -rawRssi : rawRssi,
    noiseFloor: signedByte(buffer[17]),
    ppduType: buffer[18],
    flags: buffer[19],
    amplitudes,
  };
}

function mean(values) {
  if (values.length === 0) return 0;
  return values.reduce((total, value) => total + value, 0) / values.length;
}

function relativeMotion(previous, current) {
  if (!previous || previous.length !== current.length || current.length === 0) return 0;
  let delta = 0;
  for (let index = 0; index < current.length; index += 1) {
    delta += Math.abs(current[index] - previous[index]);
  }
  return (delta / current.length) / Math.max(mean(previous), 1) * 100;
}

function safeEqual(left, right) {
  const leftBuffer = Buffer.from(left);
  const rightBuffer = Buffer.from(right);
  return leftBuffer.length === rightBuffer.length && crypto.timingSafeEqual(leftBuffer, rightBuffer);
}

function basicAuthMatches(request, username, password) {
  if (!password) return true;
  const header = request.headers.authorization || '';
  if (!header.startsWith('Basic ')) return false;
  try {
    const decoded = Buffer.from(header.slice(6), 'base64').toString('utf8');
    const separator = decoded.indexOf(':');
    if (separator < 0) return false;
    return safeEqual(decoded.slice(0, separator), username) && safeEqual(decoded.slice(separator + 1), password);
  } catch {
    return false;
  }
}

function createMonitor({
  udpPort = 5005,
  httpPort = 8080,
  bindAddress = '0.0.0.0',
  authUser = process.env.RUVIEW_AUTH_USER || 'viewer',
  authPassword = process.env.RUVIEW_AUTH_PASSWORD || '',
} = {}) {
  const nodes = new Map();
  const clients = new Set();
  const startedAt = Date.now();
  let validPackets = 0;
  let rejectedPackets = 0;

  function snapshot() {
    const now = Date.now();
    return {
      source: 'esp32-real-csi',
      synthetic: false,
      authEnabled: Boolean(authPassword),
      udpPort,
      uptimeSeconds: Math.floor((now - startedAt) / 1000),
      validPackets,
      rejectedPackets,
      nodes: [...nodes.values()].map((node) => {
        node.packetTimes = node.packetTimes.filter((time) => now - time <= 5000);
        return {
          nodeId: node.nodeId,
          online: now - node.lastSeen <= 3000,
          lastSeenMs: now - node.lastSeen,
          sourceAddress: node.sourceAddress,
          packets: node.packets,
          packetsPerSecond: Number((node.packetTimes.length / 5).toFixed(1)),
          sequence: node.sequence,
          rssi: node.rssi,
          noiseFloor: node.noiseFloor,
          frequencyMhz: node.frequencyMhz,
          antennas: node.antennas,
          subcarriers: node.subcarriers,
          motion: Number(node.motion.toFixed(2)),
          meanAmplitude: Number(mean(node.amplitudes).toFixed(2)),
          amplitudes: node.amplitudes.map((value) => Number(value.toFixed(2))),
        };
      }),
    };
  }

  function publish() {
    const payload = `data: ${JSON.stringify(snapshot())}\n\n`;
    for (const client of clients) client.write(payload);
  }

  const udp = dgram.createSocket('udp4');
  udp.on('error', (error) => {
    console.error(`UDP listener error on ${bindAddress}:${udpPort}: ${error.message}`);
  });
  udp.on('message', (message, remote) => {
    const frame = parseCsiFrame(message);
    if (!frame) {
      rejectedPackets += 1;
      return;
    }

    const now = Date.now();
    const prior = nodes.get(frame.nodeId);
    const instantMotion = relativeMotion(prior?.amplitudes, frame.amplitudes);
    nodes.set(frame.nodeId, {
      ...frame,
      sourceAddress: `${remote.address}:${remote.port}`,
      lastSeen: now,
      packets: (prior?.packets ?? 0) + 1,
      packetTimes: [...(prior?.packetTimes ?? []), now].filter((time) => now - time <= 5000),
      motion: prior ? prior.motion * 0.75 + instantMotion * 0.25 : 0,
    });
    validPackets += 1;
  });

  const webRoot = path.join(__dirname, '..', 'web');
  const httpServer = http.createServer((request, response) => {
    response.setHeader('X-Content-Type-Options', 'nosniff');
    response.setHeader('Referrer-Policy', 'no-referrer');
    response.setHeader('X-Frame-Options', 'DENY');
    response.setHeader('Content-Security-Policy', "default-src 'self'; connect-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; img-src 'self' data:; frame-ancestors 'none'");
    if (request.url === '/healthz') {
      response.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store' });
      response.end(JSON.stringify({ ok: true }));
      return;
    }
    if (!basicAuthMatches(request, authUser, authPassword)) {
      response.setHeader('WWW-Authenticate', 'Basic realm="RuView Monitor", charset="UTF-8"');
      response.writeHead(401, { 'Content-Type': 'text/plain; charset=utf-8', 'Cache-Control': 'no-store' });
      response.end('需要监控访问账号和密码。');
      return;
    }
    if (request.url === '/api/status') {
      response.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8', 'Cache-Control': 'no-store' });
      response.end(JSON.stringify(snapshot()));
      return;
    }
    if (request.url === '/events') {
      response.writeHead(200, {
        'Content-Type': 'text/event-stream; charset=utf-8',
        'Cache-Control': 'no-cache',
        Connection: 'keep-alive',
      });
      response.write(`data: ${JSON.stringify(snapshot())}\n\n`);
      clients.add(response);
      request.on('close', () => clients.delete(response));
      return;
    }
    if (request.url === '/' || request.url === '/index.html') {
      response.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8', 'Cache-Control': 'no-store' });
      fs.createReadStream(path.join(webRoot, 'index.html')).pipe(response);
      return;
    }
    response.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8' });
    response.end('Not found');
  });

  const timer = setInterval(publish, 250);
  timer.unref();

  function start() {
    udp.bind(udpPort, bindAddress, () => {
      console.log(`RuView real CSI UDP listener: ${bindAddress}:${udpPort}`);
    });
    httpServer.listen(httpPort, '127.0.0.1', () => {
      console.log(`RuView monitor: http://127.0.0.1:${httpPort}`);
    });
  }

  function close() {
    clearInterval(timer);
    for (const client of clients) client.end();
    clients.clear();
    try { udp.close(); } catch {}
    try { httpServer.close(); } catch {}
  }

  return { start, close, snapshot, udp, httpServer };
}

if (require.main === module) {
  const udpPort = Number(process.env.RUVIEW_UDP_PORT || 5005);
  const httpPort = Number(process.env.RUVIEW_HTTP_PORT || 8080);
  const monitor = createMonitor({ udpPort, httpPort });
  monitor.start();
  const shutdown = () => {
    monitor.close();
    process.exit(0);
  };
  process.on('SIGINT', shutdown);
  process.on('SIGTERM', shutdown);
}

module.exports = { CSI_MAGIC, basicAuthMatches, createMonitor, parseCsiFrame, relativeMotion };
