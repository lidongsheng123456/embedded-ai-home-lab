'use strict';

const assert = require('node:assert/strict');
const test = require('node:test');
const { CSI_MAGIC, basicAuthMatches, parseCsiFrame, relativeMotion } = require('./monitor-server');

function frame({ nodeId = 1, subcarriers = 2, rssi = -48, sequence = 7 } = {}) {
  const buffer = Buffer.alloc(20 + subcarriers * 2);
  buffer.writeUInt32LE(CSI_MAGIC, 0);
  buffer[4] = nodeId;
  buffer[5] = 1;
  buffer.writeUInt16LE(subcarriers, 6);
  buffer.writeUInt32LE(2437, 8);
  buffer.writeUInt32LE(sequence, 12);
  buffer.writeInt8(rssi, 16);
  buffer.writeInt8(-95, 17);
  buffer.set([3, 4, 5, 12], 20);
  return buffer;
}

test('parses the upstream ADR-018 header and signed I/Q pairs', () => {
  const parsed = parseCsiFrame(frame());
  assert.equal(parsed.nodeId, 1);
  assert.equal(parsed.subcarriers, 2);
  assert.equal(parsed.frequencyMhz, 2437);
  assert.equal(parsed.sequence, 7);
  assert.equal(parsed.rssi, -48);
  assert.deepEqual(parsed.amplitudes, [5, 13]);
});

test('rejects non-CSI and truncated datagrams', () => {
  assert.equal(parseCsiFrame(Buffer.alloc(19)), null);
  const invalid = frame();
  invalid.writeUInt32LE(0, 0);
  assert.equal(parseCsiFrame(invalid), null);
});

test('motion is zero for identical frames and positive for changed CSI', () => {
  assert.equal(relativeMotion([5, 13], [5, 13]), 0);
  assert.ok(relativeMotion([5, 13], [10, 26]) > 0);
});

test('optional basic authentication rejects bad credentials', () => {
  const request = { headers: {} };
  assert.equal(basicAuthMatches(request, 'viewer', ''), true);
  assert.equal(basicAuthMatches(request, 'viewer', 'secret'), false);
  request.headers.authorization = `Basic ${Buffer.from('viewer:secret').toString('base64')}`;
  assert.equal(basicAuthMatches(request, 'viewer', 'secret'), true);
  assert.equal(basicAuthMatches(request, 'viewer', 'wrong'), false);
});
