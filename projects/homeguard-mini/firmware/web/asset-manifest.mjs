/** 固件、预览和公网网关共用固定资源白名单，不开放源码与本机配置。 */
export const webAssets = [
  { path: '/', file: 'index.html', contentType: 'text/html; charset=utf-8' },
  { path: '/app.js', file: 'app.js', contentType: 'text/javascript; charset=utf-8' },
  { path: '/styles.css', file: 'styles.css', contentType: 'text/css; charset=utf-8' },
  { path: '/licenses.txt', file: 'licenses.txt', contentType: 'text/plain; charset=utf-8' },
];
