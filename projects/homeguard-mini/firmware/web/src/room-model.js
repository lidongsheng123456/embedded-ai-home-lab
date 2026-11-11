/** 只搭建几何模型，不请求API。所有家具尺寸为视觉示意，单位不对应真实户型。 */
import * as THREE from 'three';

// 统一生成有阴影的方块，家具使用基础几何体，省去模型下载和纹理资源。
function box(parent, size, position, color, radius = 0) {
  const geometry = radius
    ? new THREE.CylinderGeometry(size[0], size[0], size[1], 32)
    : new THREE.BoxGeometry(...size);
  const mesh = new THREE.Mesh(geometry, new THREE.MeshStandardMaterial({
    color, roughness: 0.82,
  }));
  mesh.position.set(...position);
  mesh.castShadow = true;
  mesh.receiveShadow = true;
  parent.add(mesh);
  return mesh;
}

function addArchitecture(room) {
  box(room, [7.6, 0.22, 5.8], [0, -0.17, 0], '#c5b598');
  box(room, [7.35, 0.07, 5.55], [0, -0.03, 0], '#e4dbc4');
  // 两面墙形成剖视房间，让浏览器上的人能直接看到内部。
  box(room, [7.6, 2.65, 0.16], [0, 1.24, -2.8], '#ece9d9');
  box(room, [0.16, 2.65, 5.65], [-3.72, 1.24, 0], '#dedfcd');
  box(room, [0.08, 0.13, 5.4], [-3.61, 0.09, 0], '#b6b89d');
  box(room, [7.2, 0.13, 0.08], [0, 0.09, -2.68], '#c9c6af');
  // 窗户由浅蓝玻璃和木色窗框组成，不需要外部图片。
  box(room, [2.8, 1.53, 0.08], [0.15, 1.65, -2.68], '#a9c4c0');
  box(room, [2.98, 0.11, 0.15], [0.15, 2.44, -2.62], '#c1b79c');
  box(room, [2.98, 0.11, 0.15], [0.15, 0.87, -2.62], '#c1b79c');
  for (const x of [-1.29, 0.15, 1.59]) {
    box(room, [0.08, 1.6, 0.13], [x, 1.65, -2.6], '#d8d3bc');
  }
  box(room, [2.9, 0.07, 0.13], [0.15, 1.65, -2.6], '#d8d3bc');
  for (const x of [-1.6, 1.91]) {
    box(room, [0.5, 1.85, 0.19], [x, 1.51, -2.5], '#c7c9ac');
  }
  // 地板接缝帮助人眼理解透视，少量线条不增加复杂纹理开销。
  for (let x = -3.2; x < 3.5; x += 0.65) {
    box(room, [0.012, 0.006, 5.45], [x, 0.012, 0], '#d3c8ac');
  }
}

function addLivingFurniture(room) {
  box(room, [3.7, 0.025, 2.9], [0.1, 0.04, 0.5], '#b9bd9b');
  box(room, [3.4, 0.03, 2.6], [0.1, 0.06, 0.5], '#c5c7aa');
  box(room, [2.9, 0.22, 1.12], [-1.25, 0.32, -1.55], '#6d8166');
  box(room, [2.9, 0.75, 0.26], [-1.25, 0.7, -2.05], '#899879');
  for (const x of [-2.1, -1.25, -0.4]) {
    box(room, [0.79, 0.2, 0.87], [x, 0.53, -1.52], '#a3b08b');
    const cushion = box(room, [0.68, 0.58, 0.17], [x, 0.84, -1.89], '#b0bb98');
    cushion.rotation.x = -0.15;
  }
  for (const x of [-2.73, 0.23]) {
    box(room, [0.22, 0.57, 1.25], [x, 0.57, -1.57], '#879575');
  }
  const pillow = box(room, [0.5, 0.4, 0.24], [-2.25, 0.85, -1.55], '#d6b381');
  pillow.rotation.z = -0.22;
  box(room, [1.55, 0.12, 0.9], [-0.7, 0.53, 0.3], '#bc9972');
  for (const x of [-1.28, -0.12]) {
    for (const z of [0.03, 0.58]) box(room, [0.07, 0.45, 0.07], [x, 0.27, z], '#9b7e5e');
  }
  box(room, [0.37, 0.05, 0.28], [-0.88, 0.62, 0.3], '#e6e2cf');
  box(room, [0.32, 0.035, 0.27], [-0.85, 0.66, 0.29], '#7c947c');
  box(room, [0.1, 0.16, 0.1], [-0.25, 0.67, 0.42], '#ede7d3', 1);
  // 右侧矮柜是HomeGuard示意设备的摆放位置。
  box(room, [0.72, 0.92, 2.35], [3.02, 0.48, -0.85], '#bfa57f');
  box(room, [0.8, 0.08, 2.45], [3.02, 0.99, -0.85], '#dfc9a6');
  for (const z of [-1.5, -0.77, -0.05]) {
    box(room, [0.025, 0.65, 0.64], [2.64, 0.49, z], '#a68d6b');
  }
}

function addDetails(room) {
  box(room, [0.9, 1.9, 0.35], [-3.08, 0.95, -2.45], '#b49c78');
  for (const y of [0.12, 0.72, 1.32, 1.92]) {
    box(room, [0.96, 0.07, 0.43], [-3.08, y, -2.42], '#d1bc99');
  }
  const colors = ['#8c9c7e', '#d1ac7b', '#d8d3ba', '#8aa6a1'];
  for (let i = 0; i < 7; i++) {
    box(room, [0.07, 0.28 + (i % 3) * 0.06, 0.2],
      [-3.36 + i * 0.09, 0.92, -2.36], colors[i % 4]);
  }
  box(room, [0.5, 0.18, 0.5], [-2.8, 0.11, 1.75], '#d0bc98', 1);
  box(room, [0.04, 1.25, 0.04], [-2.8, 0.8, 1.75], '#6f8054');
  for (let i = 0; i < 7; i++) {
    const leaf = new THREE.Mesh(new THREE.SphereGeometry(0.3, 10, 7),
      new THREE.MeshStandardMaterial({ color: i % 2 ? '#8a9c66' : '#6e8957', roughness: 1 }));
    const angle = i * 2.4;
    leaf.scale.set(0.8, 0.35, 1.6);
    leaf.position.set(-2.8 + Math.cos(angle) * 0.22, 0.75 + i * 0.13, 1.75 + Math.sin(angle) * 0.25);
    leaf.rotation.z = angle;
    room.add(leaf);
  }
  box(room, [0.7, 0.9, 0.06], [2.66, 1.9, -2.66], '#b49a72');
  box(room, [0.59, 0.78, 0.07], [2.66, 1.9, -2.61], '#ddd4b5');
  box(room, [0.24, 0.35, 0.08], [2.65, 1.85, -2.56], '#a1ae83');
}

function addDevice(room) {
  box(room, [0.34, 0.38, 0.3], [3, 1.23, -0.3], '#f0eee0');
  box(room, [0.025, 0.18, 0.19], [2.82, 1.25, -0.3], '#233a31');
  const beacon = new THREE.Mesh(new THREE.SphereGeometry(0.035, 12, 8),
    new THREE.MeshBasicMaterial({ color: '#99d8a6' }));
  beacon.position.set(2.79, 1.38, -0.3);
  room.add(beacon);
  const rings = [];
  for (let i = 0; i < 3; i++) {
    const ring = new THREE.Mesh(new THREE.RingGeometry(0.97, 1, 80),
      new THREE.MeshBasicMaterial({ color: '#86b79d', transparent: true,
        opacity: 0.25, side: THREE.DoubleSide, depthWrite: false }));
    ring.rotation.x = -Math.PI / 2;
    ring.position.set(1.5, 0.12 + i * 0.003, -0.2);
    room.add(ring);
    rings.push(ring);
  }
  return { beacon, rings };
}

function addPerson(room) {
  const person = new THREE.Group();
  person.position.set(0.8, 0, 1);
  const body = box(person, [0.34, 0.65, 0.28], [0, 0.85, 0], '#9eae94');
  body.rotation.z = -0.06;
  const head = new THREE.Mesh(new THREE.SphereGeometry(0.18, 18, 12),
    new THREE.MeshStandardMaterial({ color: '#dbc4a2', roughness: 1 }));
  head.position.y = 1.38;
  head.castShadow = true;
  person.add(head);
  for (const x of [-0.1, 0.1]) box(person, [0.1, 0.48, 0.14], [x, 0.31, 0], '#768b7e');
  person.visible = false;
  room.add(person);
  return person;
}

export function createRoom() {
  const room = new THREE.Group();
  addArchitecture(room);
  addLivingFurniture(room);
  addDetails(room);
  const effects = addDevice(room);
  return { room, ...effects, person: addPerson(room) };
}
