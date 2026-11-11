/** 图标和界面过渡独立管理，不参与任何设备判断。仅导入使用的图标，避免打包整个图标库。 */
import { animate, stagger } from 'motion';
import { createIcons, House, ShieldCheck, ScanLine, ChartNoAxesCombined, Camera, HardDrive,
  Maximize, FlaskConical, ArrowUpRight, Move, Pause, RotateCcw, BrainCircuit,
  VolumeX, RefreshCw, LockKeyhole, Thermometer, Droplets, Sun, AudioLines, Wind } from 'lucide';

export function initializeVisualUi() {
  createIcons({ icons: { House, ShieldCheck, ScanLine, ChartNoAxesCombined, Camera, HardDrive,
    Maximize, FlaskConical, ArrowUpRight, Move, Pause, RotateCcw, BrainCircuit,
    VolumeX, RefreshCw, LockKeyhole, Thermometer, Droplets, Sun, AudioLines, Wind } });
  // 尊重系统减少动态效果设置；初始内容始终可见，动画加载失败也不会出现空白页面。
  if (!matchMedia('(prefers-reduced-motion: reduce)').matches) {
    animate('.reveal', { opacity: [0, 1], y: [18, 0] },
      { duration: 0.7, delay: stagger(0.06), ease: [0.2, 0.7, 0.2, 1] });
  }
}
