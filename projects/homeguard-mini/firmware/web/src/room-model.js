/** 场景模型只负责外观，不解析协议或决定告警规则。 */
import * as THREE from 'three';
import { createMaterials } from './room-materials.js';
import { createArchitecture } from './room-architecture.js';
import { createFurniture } from './room-furniture.js';
import { box } from './room-geometry.js';
import {HOME_LAYOUT} from './home-layout.js';
import {createHomeLabels} from './home-labels.js';

export function createRoom() {
  const room = new THREE.Group();
  const materials = createMaterials();
  const architecture = createArchitecture(room, materials);
  architecture.setCutaway(true);
  createFurniture(room, materials);
  createHomeLabels(room);
  const [x, y, z] = HOME_LAYOUT.device;
  box(room, [0.2, 0.28, 0.15], [x, y - 0.1, z], materials.cream, 0.025);
  box(room, [0.14, 0.11, 0.015], [x, y - 0.06, z + 0.08], materials.dark);
  const beacon = box(room, [0.045, 0.012, 0.016], [x, y - 0.15, z + 0.08],
    new THREE.MeshStandardMaterial({ color: '#738d69', emissive: '#738d69', emissiveIntensity: 0.5 }));
  // 雷达没有人员定位能力：只显示固定标记，不放一个假装位于真实坐标的人偶。
  const person = new THREE.Mesh(new THREE.RingGeometry(0.17, 0.2, 48),
    new THREE.MeshBasicMaterial({ color: '#78916c', transparent: true, opacity: 0.5, side: THREE.DoubleSide }));
  person.rotation.x = -Math.PI / 2; person.position.set(...HOME_LAYOUT.presenceMarker);
  person.visible = false; room.add(person);
  return { room, person, beacon, setCutaway: architecture.setCutaway, anchors: {
    device: new THREE.Vector3(x, y + 0.25, z),
    person: new THREE.Vector3(...HOME_LAYOUT.presenceMarker).add(new THREE.Vector3(0, 0.2, 0)),
  } };
}
