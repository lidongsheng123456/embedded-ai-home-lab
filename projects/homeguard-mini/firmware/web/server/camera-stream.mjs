/** 复用电脑采集的MJPEG画面，避免公网每帧都等待一次HTTP往返。 */
const BOUNDARY = 'homeguard-frame';
const MAX_VIEWERS = 8;
const STALE_MS = 5000;

export function createCameraStream(recorder) {
  let viewers = 0;
  return (request, response, url) => {
    if (!recorder || url.pathname !== '/api/camera/stream') return false;
    if (request.method !== 'GET') {
      response.writeHead(405, { Allow: 'GET' }); response.end(); return true;
    }
    if (viewers >= MAX_VIEWERS) {
      response.writeHead(503); response.end('观看人数过多，请稍后重试'); return true;
    }
    ++viewers;
    let lastWrite = Date.now();
    let unsubscribe;
    let watchdog;
    response.once('close', () => { --viewers; clearInterval(watchdog); unsubscribe?.(); });
    response.writeHead(200, { 'Content-Type': `multipart/x-mixed-replace; boundary=${BOUNDARY}`,
      'Cache-Control': 'no-store, no-transform', 'X-Accel-Buffering': 'no' });
    response.flushHeaders();
    const send = frame => {
      // 慢客户端丢弃中间帧，不把整段录像排队塞进内存。
      if (response.destroyed || response.writableNeedDrain) return;
      lastWrite = Date.now();
      const header = Buffer.from(`--${BOUNDARY}\r\nContent-Type: image/jpeg\r\nContent-Length: ${frame.bytes.length}\r\nX-Frame-At: ${frame.at}\r\n\r\n`);
      response.write(Buffer.concat([header, frame.bytes, Buffer.from('\r\n')]));
    };
    unsubscribe = recorder.subscribe(send);
    const current = recorder.live();
    if (current) send(current);
    // 相机断线后主动结束流，使浏览器重连并明确显示中断，不能永远停在旧帧。
    watchdog = setInterval(() => { if (Date.now() - lastWrite > STALE_MS) response.destroy(); }, 1000);
    return true;
  };
}
