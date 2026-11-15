/** iPhone不能依赖浏览器全屏API；页内放大也保持原始比例与安全区。 */
import { byId } from './dashboard-view.js';

export function createCameraLayout() {
  const panel = byId('camera-panel');
  const expand = byId('photo-expand');
  const close = byId('photo-close');
  let expanded = false;
  const setExpanded = value => {
    expanded = value;
    panel.classList.toggle('expanded', value);
    document.body.classList.toggle('camera-expanded', value);
    close.hidden = !value;
    expand.setAttribute('aria-expanded', String(value));
    (value ? close : expand).focus();
  };
  const open = () => setExpanded(true);
  const hide = () => setExpanded(false);
  const escape = event => { if (event.key === 'Escape' && expanded) hide(); };
  expand.addEventListener('click', open);
  close.addEventListener('click', hide);
  document.addEventListener('keydown', escape);
  return { destroy() {
    panel.classList.remove('expanded'); document.body.classList.remove('camera-expanded');
    expand.removeEventListener('click', open); close.removeEventListener('click', hide);
    document.removeEventListener('keydown', escape);
  } };
}
