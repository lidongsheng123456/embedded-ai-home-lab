/** 历史画面仅接受时间参数；不暴露录像目录或允许客户端指定文件路径。 */
export async function handleRecording(request, response, url, recorder) {
  if (!recorder || !['/camera.jpg', '/api/recordings', '/api/recordings/frame'].includes(url.pathname)) return false;
  if (request.method !== 'GET') {
    response.writeHead(405, { Allow: 'GET' }); response.end(); return true;
  }
  if (url.pathname === '/api/recordings') {
    response.writeHead(200, { 'Content-Type': 'application/json; charset=utf-8' });
    response.end(JSON.stringify(recorder.history()));
    return true;
  }
  const historical = url.pathname === '/api/recordings/frame';
  const raw = url.searchParams.get('at');
  if (historical && (!raw || !/^\d{1,16}$/.test(raw) || !Number.isSafeInteger(Number(raw)))) {
    response.writeHead(400); response.end('时间参数无效'); return true;
  }
  const frame = historical ? await recorder.frame(Number(raw)) : recorder.live();
  if (!frame) {
    response.writeHead(historical ? 404 : 503, { 'Content-Type': 'application/json; charset=utf-8' });
    response.end(JSON.stringify({ error: historical ? '该时段未录到画面' : '摄像头暂无实时画面' }));
    return true;
  }
  response.writeHead(200, { 'Content-Type': 'image/jpeg', 'X-Frame-At': String(frame.at) });
  response.end(frame.bytes);
  return true;
}
