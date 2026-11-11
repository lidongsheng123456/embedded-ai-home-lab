/** 仅处理几何体；材质与摆放由各自的房间模块决定。 */
import * as THREE from 'three';
import { RoundedBoxGeometry } from 'three/addons/geometries/RoundedBoxGeometry.js';

export function box(parent, size, position, material, radius = 0.025) {
  const mesh = new THREE.Mesh(new RoundedBoxGeometry(...size, 3,
    Math.min(radius, Math.min(...size) / 3)), material);
  mesh.position.set(...position);
  mesh.castShadow = mesh.receiveShadow = true;
  parent.add(mesh);
  return mesh;
}

export function cylinder(parent, top, bottom, height, position, material) {
  const mesh = new THREE.Mesh(new THREE.CylinderGeometry(top, bottom, height, 40), material);
  mesh.position.set(...position);
  mesh.castShadow = mesh.receiveShadow = true;
  parent.add(mesh);
  return mesh;
}
