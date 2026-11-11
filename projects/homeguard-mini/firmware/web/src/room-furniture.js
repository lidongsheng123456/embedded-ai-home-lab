/** 家具按使用比例摆放，软垫、木腿和织物材质分别处理。 */
import * as THREE from 'three';
import { box, cylinder } from './room-geometry.js';

function sofa(room, m) {
  for (const x of [-2.65, 0.35]) for (const z of [-2.25, -1.2])
    cylinder(room, 0.045, 0.035, 0.19, [x, 0.1, z], m.wood);
  box(room, [3.45, 0.32, 1.4], [-1.15, 0.37, -1.8], m.sage, 0.12);
  box(room, [3.45, 0.75, 0.24], [-1.15, 0.8, -2.39], m.sage, 0.1);
  for (const x of [-2.78, 0.48]) box(room, [0.25, 0.62, 1.42], [x, 0.65, -1.8], m.sage, 0.1);
  for (const x of [-2.16, -1.15, -0.14]) {
    box(room, [0.97, 0.23, 1.12], [x, 0.62, -1.7], m.linen, 0.1);
    const back = box(room, [0.96, 0.63, 0.24], [x, 0.98, -2.19], m.linen, 0.1);
    back.rotation.x = -0.12;
  }
  for (const [x, material, angle] of [[-2.35, m.clay, -0.16], [0.04, m.sage, 0.2]]) {
    const pillow = box(room, [0.5, 0.5, 0.19], [x, 0.94, -1.92], material, 0.09);
    pillow.rotation.set(-0.18, 0, angle);
  }
}

function plant(room, m, x, z) {
  cylinder(room, 0.23, 0.18, 0.4, [x, 0.2, z], m.clay);
  cylinder(room, 0.015, 0.02, 0.95, [x, 0.78, z], m.wood);
  for (let i = 0; i < 10; i++) {
    const angle = i * 2.4;
    const leaf = new THREE.Mesh(new THREE.SphereGeometry(1, 16, 10), m.sage);
    leaf.scale.set(0.12, 0.025, 0.36);
    leaf.position.set(x + Math.cos(angle) * 0.2, 0.65 + i * 0.075, z + Math.sin(angle) * 0.2);
    leaf.rotation.set(0.4, angle, 0.25);
    leaf.castShadow = true; room.add(leaf);
  }
}

export function createFurniture(room, m) {
  const rug = box(room, [4.7, 0.025, 3.4], [-0.45, 0.015, 0.3], m.linen, 0.01);
  rug.material = m.linen.clone(); rug.material.color.set('#e4d4b9');
  for (let i = 0; i < 60; i++) for (const z of [-1.43, 2.03])
    box(room, [0.018, 0.012, 0.11], [-2.72 + i * 0.077, 0.02, z], m.linen, 0.003);
  sofa(room, m);
  const table = cylinder(room, 0.86, 0.86, 0.09, [-0.9, 0.53, 0.45], m.wood);
  table.scale.z = 0.76;
  for (const [x, z] of [[-1.4, 0.1], [-0.4, 0.1], [-0.9, 0.86]])
    cylinder(room, 0.055, 0.035, 0.48, [x, 0.26, z], m.wood);
  box(room, [0.42, 0.045, 0.3], [-1.15, 0.607, 0.48], m.sage);
  box(room, [0.37, 0.035, 0.26], [-1.1, 0.647, 0.45], m.cream);
  cylinder(room, 0.1, 0.075, 0.12, [-0.55, 0.64, 0.47], m.cream);
  const handle = new THREE.Mesh(new THREE.TorusGeometry(0.055, 0.012, 8, 20), m.cream);
  handle.position.set(-0.44, 0.66, 0.47); room.add(handle);
  box(room, [1.85, 0.8, 0.64], [2.55, 0.52, -2.5], m.wood);
  for (const x of [2.08, 2.98]) {
    box(room, [0.87, 0.67, 0.03], [x, 0.54, -2.16], m.wood);
    box(room, [0.16, 0.018, 0.035], [x, 0.74, -2.13], m.dark);
  }
  for (const x of [1.8, 3.3]) cylinder(room, 0.035, 0.025, 0.15, [x, 0.08, -2.5], m.wood);
  plant(room, m, -3.3, 1.9);
  plant(room, m, 3.25, -1.25);
  cylinder(room, 0.28, 0.28, 0.05, [-3.2, 0.04, -1.75], m.dark);
  cylinder(room, 0.018, 0.018, 2.1, [-3.2, 1.08, -1.75], m.dark);
  cylinder(room, 0.24, 0.38, 0.4, [-3.2, 2.06, -1.75], m.linen);
}
