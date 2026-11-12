/** 卧室保留木床、浅花纹床品、米色窗帘、空调、行李箱和床边小架。 */
import * as THREE from 'three';
import {box, cylinder} from './room-geometry.js';

function curtain(room, m) {
  for (const x of [-2.55, -1.1]) {
    const geometry = new THREE.PlaneGeometry(1.45, 2.15, 40, 1);
    const position = geometry.attributes.position;
    for (let i = 0; i < position.count; i++) position.setZ(i, Math.sin(position.getX(i) * 30) * 0.05);
    geometry.computeVertexNormals();
    const fabric = m.quilt.clone(); fabric.side = THREE.DoubleSide;
    const panel = new THREE.Mesh(geometry, fabric);
    panel.position.set(x, 1.36, -3.29); panel.receiveShadow = true; room.add(panel);
  }
  box(room, [3, 0.06, 0.08], [-1.8, 2.49, -3.25], m.brass);
}

function suitcase(room, m, x, z, color) {
  const material = new THREE.MeshStandardMaterial({color, roughness: 0.6});
  box(room, [0.37, 0.61, 0.25], [x, 0.36, z], material, 0.05);
  for (const dx of [-0.14, 0.14]) cylinder(room, 0.035, 0.035, 0.055, [x + dx, 0.045, z + 0.07], m.dark);
  box(room, [0.16, 0.035, 0.07], [x, 0.68, z], m.dark);
}

export function createBedroom(room, m) {
  curtain(room, m);
  for (const x of [-2.4, -0.5]) for (const z of [-2.6, -1.18])
    box(room, [0.07, 0.32, 0.07], [x, 0.17, z], m.wood);
  box(room, [2.12, 0.22, 1.72], [-1.45, 0.4, -1.9], m.wood);
  box(room, [0.12, 0.83, 1.75], [-0.35, 0.67, -1.9], m.wood);
  box(room, [2.02, 0.19, 1.64], [-1.44, 0.59, -1.9], m.white, 0.06);
  box(room, [1.56, 0.1, 1.68], [-1.7, 0.73, -1.9], m.quilt, 0.045);
  for (const z of [-2.3, -1.48]) box(room, [0.4, 0.12, 0.62], [-0.64, 0.75, z], m.quilt, 0.08);
  // 轻微起伏的毯边让床品有厚度，而不是一块纯白积木。
  for (let i = 0; i < 7; i++) box(room, [0.08, 0.26 + Math.sin(i) * 0.025, 0.23],
    [-2.47, 0.61, -2.62 + i * 0.23], m.quilt, 0.018);
  suitcase(room, m, -3.11, -2.73, '#91968f');
  suitcase(room, m, -2.66, -2.78, '#c4b89e');
  // 环形灯是视频里的家具细节，不表示已经加入固件控制。
  cylinder(room, 0.016, 0.016, 1.18, [-3.02, 0.62, -1.8], m.dark);
  const ring = new THREE.Mesh(new THREE.TorusGeometry(0.15, 0.018, 8, 32), m.white);
  ring.position.set(-3.02, 1.36, -1.8); room.add(ring);
  box(room, [0.24, 0.28, 0.04], [-3.02, 0.16, -1.8], m.dark);
  box(room, [0.26, 0.36, 0.92], [-0.13, 2.13, -2.08], m.white, 0.045);
  box(room, [0.27, 0.018, 0.72], [-0.12, 1.99, -2.08], m.dark);
  const pink = new THREE.MeshStandardMaterial({color: '#caa8a3', roughness: 0.7});
  for (const y of [0.18, 0.45, 0.72]) box(room, [0.42, 0.05, 0.32], [-0.53, y, -0.55], pink);
  for (const x of [-0.72, -0.34]) box(room, [0.025, 0.75, 0.025], [x, 0.4, -0.55], pink);
}
