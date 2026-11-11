/** 场景模型只负责外观，不解析协议或决定告警规则。 */
import * as THREE from 'three';
import { createMaterials } from './room-materials.js';
import { createArchitecture } from './room-architecture.js';
import { createFurniture } from './room-furniture.js';
import { box } from './room-geometry.js';
import { loadRoomAssets } from './room-assets.js';

export function createRoom() {
  const room = new THREE.Group();
  const materials = createMaterials();
  createArchitecture(room, materials);
  createFurniture(room, materials);
  box(room, [0.36, 0.4, 0.26], [2.45, 1.14, -2.48], materials.cream, 0.04);
  box(room, [0.24, 0.17, 0.015], [2.45, 1.17, -2.34], materials.dark);
  const beacon = box(room, [0.07, 0.016, 0.016], [2.45, 1.035, -2.335],
    new THREE.MeshStandardMaterial({ color: '#738d69', emissive: '#738d69', emissiveIntensity: 0.5 }));
  // 雷达没有人员定位能力：只显示固定标记，不放一个假装位于真实坐标的人偶。
  const person = new THREE.Mesh(new THREE.RingGeometry(0.17, 0.2, 48),
    new THREE.MeshBasicMaterial({ color: '#78916c', transparent: true, opacity: 0.5, side: THREE.DoubleSide }));
  person.rotation.x = -Math.PI / 2; person.position.set(0.8, 0.05, 1.25);
  person.visible = false; room.add(person);
  const assets = loadRoomAssets(room);
  return { room, person, beacon, dispose: assets.dispose, anchors: {
    device: new THREE.Vector3(2.45, 1.45, -2.48),
    person: new THREE.Vector3(0.8, 0.25, 1.25),
  } };
}
