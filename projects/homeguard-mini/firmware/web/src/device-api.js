/** 设备通信层：对HTTP错误和超时给出明确失败，绝不用演示数据代替断线状态。 */
export async function requestDevice(path, options = {}) {
  const controller = new AbortController();
  const timer = setTimeout(() => controller.abort(), 6000);
  try {
    const response = await fetch(path, { cache: 'no-store', ...options, signal: controller.signal });
    if (!response.ok) throw new Error(`设备响应异常：HTTP ${response.status}`);
    return await response.json();
  } finally {
    clearTimeout(timer);
  }
}

export const readStatus = () => requestDevice('/api/status');
export const sendCommand = (command) => requestDevice(`/api/${command}`, { method: 'POST' });
