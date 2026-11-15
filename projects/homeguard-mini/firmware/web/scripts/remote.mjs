/** 本机网关入口：配置、传感器历史与录像只保存在已忽略的.pio/remote中。 */
import { randomBytes } from 'node:crypto';
import { mkdir, readFile, writeFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { createGateway } from '../server/gateway.mjs';
import { validateDeviceUrl } from '../server/device-proxy.mjs';
import { createHistoryStore } from '../server/history-store.mjs';
import { createHistoryRecorder } from '../server/history-recorder.mjs';
import { createRecordingStore } from '../server/recording-store.mjs';
import { createCameraRecorder } from '../server/camera-recorder.mjs';

const configFile = new URL('../../.pio/remote/config.json', import.meta.url);
const assetRoot = new URL('../../.pio/web/', import.meta.url);
await mkdir(new URL('./', configFile), { recursive: true });
let config;
const argument = process.argv.indexOf('--device');
const requestedDevice = argument < 0 ? undefined : process.argv[argument + 1];
if (argument >= 0 && !requestedDevice) throw new Error('--device后需要填写屏幕IP对应的HTTP地址');
try { config = JSON.parse(await readFile(configFile, 'utf8')); }
catch (error) {
  if (error.code !== 'ENOENT') throw new Error('网关配置无法解析，请在本机检查config.json');
  if (!requestedDevice) throw new Error('首次启动请加 --device http://屏幕IP');
  config = { deviceUrl: validateDeviceUrl(requestedDevice).origin,
    username: 'homeguard', password: randomBytes(24).toString('base64url'), port: 4180 };
  await writeFile(configFile, JSON.stringify(config, null, 2) + '\n', { flag: 'wx', mode: 0o600 });
}
if (requestedDevice && config.deviceUrl !== validateDeviceUrl(requestedDevice).origin) {
  config.deviceUrl = validateDeviceUrl(requestedDevice).origin;
  await writeFile(configFile, JSON.stringify(config, null, 2) + '\n', { mode: 0o600 });
}
await readFile(new URL('index.html', assetRoot));
const device = validateDeviceUrl(config.deviceUrl);
if (!Number.isInteger(config.port) || config.port < 1024 || config.port > 65535)
  throw new Error('网关端口应在1024～65535之间');
const historyFile = new URL('../../.pio/remote/history.ndjson', import.meta.url);
const store = await createHistoryStore(fileURLToPath(historyFile));
const recorder = createHistoryRecorder({ device, store });
const recordingStore = await createRecordingStore(fileURLToPath(new URL('../../.pio/remote/recordings/', import.meta.url)));
const cameraRecorder = createCameraRecorder({ device, store: recordingStore,
  isReady: () => recorder.live()?.state?.camera === true });
const server = createGateway({ ...config, device, assetRoot, recorder, cameraRecorder });
server.on('error', () => { console.error('网关启动失败，请检查端口占用和本机配置'); process.exitCode = 1; });
server.listen(config.port, '127.0.0.1', () => {
  recorder.start();
  cameraRecorder.start();
  console.log(`本机网关：http://127.0.0.1:${config.port}`);
  console.log(`独立登录信息保存在：${fileURLToPath(configFile)}`);
  console.log('传感器每5秒记录；摄像头约每秒录一帧，保留最近24小时、最多4GiB，关闭网页后继续记录。');
});
for (const signal of ['SIGINT', 'SIGTERM']) process.once(signal, async () => {
  server.close();
  server.closeAllConnections();
  await Promise.all([recorder.stop(), cameraRecorder.stop()]);
});
