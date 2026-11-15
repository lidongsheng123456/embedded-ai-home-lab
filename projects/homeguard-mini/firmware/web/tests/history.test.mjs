import test from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, readFile, rm, appendFile } from 'node:fs/promises';
import path from 'node:path';
import { tmpdir } from 'node:os';
import { once } from 'node:events';
import { createHistoryStore, RETENTION_MS } from '../server/history-store.mjs';
import { createHistoryRecorder } from '../server/history-recorder.mjs';
import { createGateway } from '../server/gateway.mjs';
import { createDemoState } from '../src/demo-state.js';
import { mergeRecords, recordAt, trendPath, metricValue } from '../src/history-model.js';

async function withStore(task) {
  const directory = await mkdtemp(path.join(tmpdir(), 'homeguard-history-'));
  let time = 1_800_000_000_000;
  const file = path.join(directory, 'history.ndjson');
  try { await task({ file, now: () => time, advance: ms => { time += ms; } }); }
  finally { await rm(directory, { recursive: true }); }
}

test('历史落盘只保存白名单字段，重启恢复，跳过半行，24小时后淘汰', async () => {
  await withStore(async ({ file, now, advance }) => {
    let store = await createHistoryStore(file, now);
    await store.append({ at: now(), state: { ...createDemoState(), password: 'must-not-persist' } });
    advance(5000);
    await store.append({ at: now(), state: null });
    assert.doesNotMatch(await readFile(file, 'utf8'), /password|must-not-persist/);
    await appendFile(file, '{"at":');
    store = await createHistoryStore(file, now);
    assert.equal(store.rows().length, 2);
    assert.equal(store.rows()[1].state, null);
    advance(RETENTION_MS);
    store = await createHistoryStore(file, now);
    assert.deepEqual(store.rows(), []);
    assert.equal(await readFile(file, 'utf8'), '');
  });
});

test('采样成功、掉线与坏响应均可停止，掉线不沿用旧值，不触发拍照或控制', async () => {
  for (const mode of ['ok', 'offline', 'invalid', 'oversize']) {
    await withStore(async ({ file, now }) => {
      const store = await createHistoryStore(file, now);
      const calls = [];
      const recorder = createHistoryRecorder({ device: new URL('http://192.168.1.96'), store, now,
        upstreamFetch: async (url, options) => {
          calls.push({ path: url.pathname, method: options.method ?? 'GET' });
          if (mode === 'offline') throw new Error('offline');
          return new Response(mode === 'invalid' ? '{}' : mode === 'oversize' ? 'x'.repeat(17000)
            : JSON.stringify(createDemoState()));
        } });
      recorder.start();
      await recorder.stop();
      assert.deepEqual(calls, [{ path: '/api/status', method: 'GET' }]);
      assert.equal(recorder.history().records.length, 1);
      assert.equal(recorder.live().state !== null, mode === 'ok');
      assert.equal(recorder.history().recording, false);
    });
  }
});

test('历史接口需要登录，拒绝写入和坏时间参数，实时缓存不重复请求设备', async () => {
  const rows = [{ at: Date.now(), state: createDemoState() }];
  let calls = 0;
  const recorder = { live: () => rows[0], history: since => ({ records: rows.filter(row => row.at > since) }) };
  const server = createGateway({ username: 'test', password: 'test-only', recorder,
    upstreamFetch: async () => { ++calls; throw new Error('不能请求设备'); } });
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  const origin = `http://127.0.0.1:${server.address().port}`;
  const headers = { authorization: `Basic ${Buffer.from('test:test-only').toString('base64')}` };
  try {
    assert.equal((await fetch(origin + '/api/history')).status, 401);
    const response = await fetch(origin + '/api/history', { headers });
    assert.deepEqual((await response.json()).records, rows);
    assert.equal((await fetch(origin + '/api/history?since=NaN', { headers })).status, 400);
    assert.equal((await fetch(origin + '/api/history', { method: 'POST', headers })).status, 405);
    assert.equal((await fetch(origin + '/api/status', { headers })).status, 200);
    rows[0] = { at: Date.now(), state: null };
    assert.equal((await fetch(origin + '/api/status', { headers })).status, 503);
    assert.equal(calls, 0);
  } finally { server.closeAllConnections(); await new Promise(resolve => server.close(resolve)); }
});

test('回放不使用未来值或跨掉线沿用旧值，预热和失效读数不画成零', () => {
  const rows = [{ at: 1000, state: createDemoState() }, { at: 6000, state: null },
    { at: 100000, state: createDemoState() }];
  assert.equal(recordAt(rows, 500), null);
  assert.equal(recordAt(rows, 1000), rows[0]);
  assert.equal(recordAt(rows, 6000).state, null);
  assert.equal(recordAt(rows, 50000), null);
  assert.equal(metricValue({ gasIndex: 50, gasWarmingUp: true }, 'gasIndex'), null);
  assert.equal(metricValue({ presence: false, radarValid: false }, 'presence'), null);
  const chart = trendPath(rows, 'temp', 0, 110000);
  assert.equal((chart.path.match(/M/g) || []).length, 2);
  assert.equal(chart.min, rows[0].state.temp);
});

test('趋势压缩保留尖峰和连续线，合并去重并限制24小时窗口', () => {
  const now = 1_800_000_000_000;
  const rows = Array.from({ length: 17_000 }, (_, i) => ({
    at: now - (17000 - i) * 5000, state: { temp: i === 8001 ? 99 : 20 },
  }));
  const chart = trendPath(rows, 'temp', now - RETENTION_MS, now);
  assert.equal(chart.max, 99);
  assert.ok(chart.path.includes('L'));
  assert.equal((chart.path.match(/M/g) || []).length, 1);
  assert.ok(chart.path.length < 8000);
  const merged = mergeRecords(rows, [{ ...rows[0] }, { at: now - RETENTION_MS - 1, state: null }], now);
  assert.equal(merged.length, rows.length);
});
