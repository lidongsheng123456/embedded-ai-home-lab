/** 时间轴负责选时和回放，不发送硬件命令；实时采集不会被回放暂停。 */
import { requestDevice } from './device-api.js';
import { byId, formatTime } from './dashboard-view.js';
import { mergeRecords, recordAt, trendPath, metrics } from './history-model.js';

export function createHistoryController(onChange, { demo = false } = {}) {
  let records = [];
  let live = true;
  let selected = Date.now();
  let playing;
  let timer;
  let destroyed = false;
  let supported = false;
  let sourceNow = Date.now();
  let video = null;
  let seekId = 0;
  let playbackTick = 0;
  const slider = byId('history-slider');
  const play = byId('play-button');
  const liveButton = byId('live-button');
  const stop = () => { clearInterval(playing); playing = null; play.textContent = '▶ 回放'; };
  const bounds = () => {
    const end = sourceNow;
    return { end, start: Math.max(records[0]?.at ?? end, end - Number(byId('history-window').value) * 3600_000) };
  };

  function render() {
    const { start, end } = bounds();
    if (!live) selected = Math.min(end, Math.max(start, selected));
    slider.min = start; slider.max = end; slider.value = live ? end : selected;
    slider.disabled = !supported || records.length < 2;
    play.disabled = slider.disabled;
    liveButton.disabled = live;
    const speed = Number(byId('playback-speed').value) || 1;
    byId('view-mode').textContent = live ? '实时监测' : playing ? `录像回放 · ${speed}倍` : '历史回放';
    byId('view-time').textContent = new Date(live ? end : selected).toLocaleString('zh-CN', {
      month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit', hour12: false,
    });
    byId('history-start').textContent = formatTime(start);
    byId('history-end').textContent = formatTime(end);
    slider.setAttribute('aria-valuetext', formatTime(live ? end : selected));
    const recordings = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    recordings.setAttribute('d', (video?.ranges ?? []).filter(range => range.end >= start && range.start <= end)
      .map(range => {
        const left = Math.max(0, (range.start - start) / Math.max(1, end - start) * 1000);
        const right = Math.min(1000, (range.end - start) / Math.max(1, end - start) * 1000);
        return `M${left.toFixed(1)},2H${Math.max(left + 1, right).toFixed(1)}`;
      }).join(' '));
    byId('video-timeline').replaceChildren(recordings);
    const subset = records.filter(row => row.at >= start && row.at <= end);
    for (const key of metrics) {
      const { path, min, max } = trendPath(subset, key, start, end);
      const svg = byId(`${key}-chart`);
      const shape = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      shape.setAttribute('d', path);
      const cursor = document.createElementNS('http://www.w3.org/2000/svg', 'line');
      const x = 2 + ((live ? end : selected) - start) / Math.max(1, end - start) * 236;
      cursor.setAttribute('x1', x); cursor.setAttribute('x2', x);
      cursor.setAttribute('y1', 0); cursor.setAttribute('y2', 56);
      svg.replaceChildren(shape, cursor);
      // 不把历史最大/最小值误标为当前状态，人体趋势的纵轴仅表示存在。
      byId(`${key}-range`).textContent = min === null ? '暂无有效记录' : key === 'presence'
        ? '低：无人 · 高：有人' : `时段范围 ${min.toFixed(key === 'temp' ? 1 : 0)} — ${max.toFixed(key === 'temp' ? 1 : 0)}`;
    }
    const row = live ? null : recordAt(records, selected);
    onChange({ live, state: row?.state ?? null, at: selected, video, seekId });
  }

  async function refresh() {
    try {
      // 本页临时曲线不能充当远端游标，否则首次历史失败后会漏掉电脑早先记录。
      const since = supported ? records.at(-1)?.at || 0 : 0;
      const data = await requestDevice(`/api/history?since=${since}`);
      if (!Array.isArray(data.records) || !Number.isFinite(data.serverTime)) throw new Error('历史格式错误');
      if (destroyed) return;
      sourceNow = data.serverTime;
      records = mergeRecords(supported ? records : [], data.records, sourceNow);
      supported = true;
      video = data.video ?? null;
      byId('video-recording-status').textContent = !video ? '录像服务未启用'
        : !video.storageHealthy ? '录像磁盘写入失败'
        : !video.recording ? '录像已暂停'
        : video.error !== 'none' ? '录像中断 · 等待相机'
        : '录像中 · 近24小时';
      byId('history-note').textContent = video
        ? '绿条为已有录像；约1帧/秒，无声音；断线空档不补录。'
        : '录像回放需更新电脑网关；设备直连不保存录像。';
      byId('recording-status').textContent = !data.storageHealthy ? '磁盘写入失败 · 请检查电脑'
        : !data.recording ? '电脑记录已暂停' : `电脑持续记录 · 已有 ${records.length} 条`;
      render();
    } catch {
      if (destroyed) return;
      byId('recording-status').textContent = supported ? '记录服务断开 · 可查看已载入数据' : '24小时回放需从家中电脑网关打开';
      render();
    } finally {
      if (!destroyed) timer = setTimeout(refresh, 30_000);
    }
  }

  slider.addEventListener('input', () => { stop(); ++seekId; live = false; selected = Number(slider.value); render(); });
  liveButton.addEventListener('click', () => { stop(); live = true; render(); });
  byId('history-window').addEventListener('change', () => { stop(); ++seekId; render(); });
  byId('playback-speed').addEventListener('change', () => { playbackTick = Date.now(); render(); });
  play.addEventListener('click', () => {
    if (playing) { stop(); render(); return; }
    const { start, end } = bounds();
    if (live || selected >= end) {
      ++seekId;
      const firstVideo = video?.ranges?.find(range => range.end >= start && range.start <= end);
      selected = firstVideo ? Math.max(start, firstVideo.start) : start;
    }
    live = false;
    playbackTick = Date.now();
    playing = setInterval(() => {
      const now = Date.now();
      selected = Math.min(bounds().end, selected + (now - playbackTick) * (Number(byId('playback-speed').value) || 1));
      playbackTick = now;
      if (selected >= bounds().end) stop();
      render();
    }, 1000);
    play.textContent = 'Ⅱ 暂停';
    render();
  });
  if (demo) {
    const now = Date.now();
    sourceNow = now;
    byId('recording-status').textContent = '模拟演示 · 不记录真实设备';
  } else void refresh();
  return {
    update(state, at) {
      // 网关初次历史加载前也能呈现实时；本地直连只保留本页临时曲线，不冒充24小时记录。
      sourceNow = Math.max(sourceNow, at ?? Date.now());
      if (demo) supported = true;
      if (demo || !supported) records = mergeRecords(records, [{ at: at ?? Date.now(), state }], sourceNow);
      render();
    },
    destroy() { destroyed = true; stop(); clearTimeout(timer); },
  };
}
