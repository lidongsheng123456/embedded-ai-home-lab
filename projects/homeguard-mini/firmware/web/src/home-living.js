/** 视频里的客厅与阳台：蓝沙发、深木桌、冰箱、低柜、防盗窗与晾衣。 */
import * as THREE from 'three';
import {box, cylinder} from './room-geometry.js';

function chair(parent, m, x, z, angle = 0) {
  const group = new THREE.Group(); group.position.set(x, 0, z); group.rotation.y = angle; parent.add(group);
  for (const x of [-0.17, 0.17]) for (const z of [-0.17, 0.17])
    box(group, [0.035, 0.47, 0.035], [x, 0.24, z], m.door);
  box(group, [0.42, 0.06, 0.42], [0, 0.49, 0], m.door);
  for (const x of [-0.18, 0.18]) box(group, [0.035, 0.52, 0.035], [x, 0.74, -0.18], m.door);
  box(group, [0.38, 0.055, 0.045], [0, 0.96, -0.18], m.door);
}

function sofa(room, m) {
  const group = new THREE.Group(); group.position.set(0.53, 0, -0.52); room.add(group);
  box(group, [0.86, 0.26, 2.02], [0, 0.27, 0], m.blue, 0.08);
  box(group, [0.18, 0.75, 2.02], [-0.34, 0.65, 0], m.blue, 0.07);
  for (const z of [-0.95, 0.95]) box(group, [0.86, 0.56, 0.13], [0, 0.52, z], m.blue, 0.05);
  box(group, [0.68, 0.16, 1.74], [0.08, 0.48, 0], m.quilt, 0.06);
  for (const z of [-0.53, 0.34]) {
    const cushion = box(group, [0.15, 0.47, 0.62], [-0.16, 0.8, z], m.linen, 0.05);
    cushion.rotation.z = -0.1;
  }
  box(group, [0.32, 0.27, 0.32], [0.03, 0.69, -0.62], m.sage, 0.08);
}

function desk(room, m) {
  box(room, [0.73, 0.065, 1.54], [1.41, 0.76, -0.5], m.door);
  for (const z of [-1.19, 0.19]) box(room, [0.67, 0.72, 0.065], [1.41, 0.37, z], m.door);
  box(room, [0.05, 0.6, 1.4], [1.09, 0.42, -0.5], m.door);
  // 笔记本、纸巾与杯子保留生活感；不复刻视频里的私人文字或屏幕内容。
  box(room, [0.39, 0.02, 0.28], [1.36, 0.81, -0.94], m.silver);
  const screen = box(room, [0.39, 0.26, 0.018], [1.36, 0.94, -1.06], m.dark);
  screen.rotation.x = -0.18;
  box(room, [0.18, 0.12, 0.12], [1.48, 0.85, -0.08], m.white);
  cylinder(room, 0.045, 0.04, 0.12, [1.52, 0.86, -0.38], m.glass);
}

function fridge(parent, m) {
  // 留出入户通道，不把冰箱放在新增厨房的侧墙或门洞里。
  const room = new THREE.Group(); room.position.z = -0.75; parent.add(room);
  box(room, [0.68, 1.84, 0.65], [2.98, 0.94, 1.65], m.silver, 0.035);
  for (const [height, y] of [[1.1, 1.27], [0.65, 0.38]]) {
    box(room, [0.025, height, 0.61], [2.625, y, 1.65], m.silver);
    box(room, [0.045, 0.32, 0.035], [2.6, y + 0.12, 1.89], m.dark);
  }
  box(room, [0.42, 0.12, 0.34], [2.98, 1.93, 1.65], m.clay, 0.06);
}

function balcony(room, m) {
  for (const x of [0.75, 2.55]) box(room, [0.035, 0.035, 1.24], [x, 2.18, -2.65], m.metal);
  box(room, [2.6, 0.035, 0.035], [1.7, 2.18, -2.64], m.metal);
  for (const [i, material] of [m.blue, m.sage, m.linen, m.clay].entries()) {
    const x = 0.65 + i * 0.63;
    box(room, [0.4, 0.48, 0.025], [x, 1.84, -2.64], material, 0.025);
    box(room, [0.59, 0.14, 0.025], [x, 2.02, -2.64], material, 0.02);
  }
  chair(room, m, 1.5, -2.85, 0.5);
  cylinder(room, 0.16, 0.13, 0.3, [2.24, 0.18, -2.36], new THREE.MeshStandardMaterial({color: '#aeced2', roughness: 0.4}));
  // 视频能看到洗衣设备，保留体块和圆形机门，不虚构运行状态。
  box(room, [0.58, 0.83, 0.57], [2.94, 0.44, -2.93], m.white);
  const door = cylinder(room, 0.19, 0.19, 0.03, [2.94, 0.44, -2.63], m.silver);
  door.rotation.x = Math.PI / 2;
  const glass = cylinder(room, 0.14, 0.14, 0.035, [2.94, 0.44, -2.61], m.dark);
  glass.rotation.x = Math.PI / 2;
}

export function createLiving(room, m) {
  sofa(room, m); desk(room, m); fridge(room, m); balcony(room, m);
  box(room, [0.42, 0.42, 1.8], [3.13, 0.25, -0.59], m.white);
  box(room, [0.3, 0.19, 0.33], [3.1, 0.56, -0.3], m.silver, 0.04);
  cylinder(room, 0.15, 0.14, 0.34, [2.64, 0.22, 0.32], m.glass);
  cylinder(room, 0.06, 0.06, 0.09, [2.64, 0.43, 0.32], m.blue);
  chair(room, m, 2.1, 1.0, -Math.PI / 2);
}
