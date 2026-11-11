/** 开放两面的客厅剖面；窗洞、踢脚线和木地板提供真实空间尺度。 */
import * as THREE from 'three';
import { box } from './room-geometry.js';

export function createArchitecture(room, m) {
  box(room, [8.4, 0.2, 6.4], [0, -0.16, 0], m.cream);
  for (let z = -2.8; z < 3; z += 0.4) {
    for (let x = -4; x < 4; x += 2) {
      // 微小拼缝模拟铺装，不用发光线条勾勒家具。
      box(room, [1.988, 0.035, 0.389], [x + 1, -0.035, z], m.wood, 0.004);
    }
  }
  box(room, [8.25, 3.05, 0.14], [0, 1.49, -3.05], m.cream);
  // 左墙分成四段，真正留出窗洞，避免在实墙上贴一片蓝色玻璃。
  box(room, [0.14, 0.85, 6.1], [-4.05, 0.4, 0], m.cream);
  box(room, [0.14, 0.35, 6.1], [-4.05, 2.85, 0], m.cream);
  box(room, [0.14, 1.85, 1.1], [-4.05, 1.73, -2.5], m.cream);
  box(room, [0.14, 1.85, 1.5], [-4.05, 1.73, 2.3], m.cream);
  box(room, [8.1, 0.12, 0.07], [0, 0.06, -2.94], m.cream);
  box(room, [0.07, 0.12, 6], [-3.94, 0.06, 0], m.cream);
  const sky = new THREE.MeshBasicMaterial({ color: '#dbe2d3' });
  box(room, [0.025, 1.8, 3.55], [-4.12, 1.73, -0.17], sky);
  for (const z of [-1.94, -0.2, 1.55]) box(room, [0.14, 1.9, 0.065], [-3.98, 1.73, z], m.cream);
  for (const y of [0.79, 2.67]) box(room, [0.2, 0.065, 3.65], [-3.96, y, -0.2], m.wood);
  box(room, [0.38, 0.07, 3.85], [-3.9, 0.78, -0.2], m.wood);
  // 薄纱以曲面形成褶皱，保持柔和透光。
  for (const z of [-1.8, 1.4]) {
    const geometry = new THREE.PlaneGeometry(0.7, 2.35, 24, 1);
    const positions = geometry.attributes.position;
    for (let i = 0; i < positions.count; i++) positions.setZ(i, Math.sin(positions.getX(i) * 36) * 0.045);
    geometry.computeVertexNormals();
    const curtain = new THREE.Mesh(geometry, new THREE.MeshStandardMaterial({ color: '#fff9eb',
      transparent: true, opacity: 0.75, roughness: 1, side: THREE.DoubleSide }));
    curtain.rotation.y = Math.PI / 2;
    curtain.position.set(-3.76, 1.58, z);
    room.add(curtain);
  }
  // 抽象挂画是静态装饰，不伪装成设备屏幕或实时数据。
  box(room, [1.48, 1.12, 0.06], [-1.15, 2.05, -2.93], m.wood);
  box(room, [1.34, 0.98, 0.025], [-1.15, 2.05, -2.88], m.linen);
  const art = new THREE.Mesh(new THREE.CircleGeometry(0.32, 48), m.clay);
  art.position.set(-1.3, 2.13, -2.86); room.add(art);
  box(room, [0.48, 0.31, 0.012], [-0.92, 1.81, -2.85], m.sage);
}
