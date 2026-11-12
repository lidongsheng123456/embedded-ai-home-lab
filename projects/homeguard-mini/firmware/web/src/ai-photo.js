/** 只读取已拍好的本机JPEG。轮询进度、刷新图片都不会再次驱动相机。 */
export function createAiPhoto() {
  const image = document.getElementById('ai-photo');
  let lastId = 0;
  let generation = 0;
  let url;
  let active;

  function clear() {
    ++generation;
    active?.abort();
    active = null;
    lastId = 0;
    image.hidden = true;
    image.removeAttribute('src');
    if (url) URL.revokeObjectURL(url);
    url = undefined;
  }

  return {
    clear,
    async update(id) {
      if (!id) { clear(); return; }
      if (id === lastId) return;
      clear();
      lastId = id;
      const version = generation;
      const controller = new AbortController();
      active = controller;
      const timer = setTimeout(() => controller.abort(), 6000);
      try {
        const response = await fetch(`/api/ai/photo.jpg?id=${id}`, {cache: 'no-store', signal: controller.signal});
        if (!response.ok || !response.headers.get('Content-Type')?.startsWith('image/jpeg')) throw new Error('照片读取失败');
        const blob = await response.blob();
        if (version !== generation) return;
        url = URL.createObjectURL(blob);
        image.src = url;
        await image.decode();
        if (version === generation) image.hidden = false;
      } catch {
        // 拍照结果仍由固件记录；照片下载失败不能误报“硬件重新拍摄失败”。
        if (version === generation) document.getElementById('ai-action').textContent += ' 照片暂未能显示，可刷新网页重新读取。';
      } finally {
        clearTimeout(timer);
        if (active === controller) active = null;
      }
    },
  };
}
