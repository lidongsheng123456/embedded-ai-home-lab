/** 外墙、门洞和推拉门独立建模。剖面只降低遮挡墙，房间位置不会随着视图改变。 */
import * as THREE from 'three';
import { box, cylinder } from './room-geometry.js';
import { HOME_LAYOUT, homeRoom } from './home-layout.js';

function tiledFloor(room, m, center, size, checker = false) {
  const step = checker ? 0.24 : 0.6;
  for (let x = 0; x < size[0] - 0.01; x += step) {
    for (let z = 0; z < size[1] - 0.01; z += step) {
      const w = Math.min(step, size[0] - x), d = Math.min(step, size[1] - z);
      const material = checker && (Math.round(x / step) + Math.round(z / step)) % 2 ? m.tileAccent : m.tile;
      box(room, [w - 0.008, 0.025, d - 0.008],
        [center[0] - size[0] / 2 + x + w / 2, 0.006, center[1] - size[1] / 2 + z + d / 2], material, 0.002);
    }
  }
}

function doorFrame(room, m, x, z, width, alongZ = false) {
  const frame = new THREE.Group(); frame.position.set(x, 0, z);
  if (alongZ) frame.rotation.y = Math.PI / 2;
  for (const side of [-1, 1]) box(frame, [0.065, 2.2, 0.16], [side * width / 2, 1.1, 0], m.door);
  box(frame, [width + 0.06, 0.08, 0.16], [0, 2.2, 0], m.door);
  room.add(frame);
  return frame;
}

function slidingGlass(room, m, x, z, width, height = 2.24, bars = false) {
  const frame = new THREE.Group(); frame.position.set(x, 0, z); room.add(frame);
  for (const y of [0.08, height]) box(frame, [width, 0.065, 0.08], [0, y, 0], m.aluminum);
  for (let i = 0; i < 4; i++) {
    const position = -width / 2 + i * width / 3;
    box(frame, [0.04, height, 0.06], [position, height / 2, 0], m.aluminum);
    if (i < 3) box(frame, [width / 3 - 0.04, height - 0.1, 0.015],
      [position + width / 6, height / 2, -0.015], m.glass, 0);
  }
  if (bars) {
    for (let i = 1; i < 12; i++) box(frame, [0.018, height - 0.14, 0.025],
      [-width / 2 + i * width / 12, height / 2, -0.06], m.metal, 0);
    for (const y of [0.65, 1.25]) box(frame, [width, 0.024, 0.035], [0, y, -0.055], m.metal);
  }
}

/** 厨房门朝向与入户门一致；双轨、重叠门扇和把手用于表达真实的推拉结构。 */
function createKitchenDoor(room, m, door) {
  const frame = doorFrame(room, {...m, door: m.aluminum}, ...door.center, door.width);
  frame.name = 'kitchen-sliding-door';
  for (const z of [-0.035, 0.035])
    box(frame, [door.width, 0.025, 0.025], [0, 0.04, z], m.aluminum);
  const panelWidth = door.width * 0.52;
  // 两扇门向右侧交叠，左边留下可见入口，避免画成一整面封死的玻璃窗。
  for (const [x, z] of [[door.width * 0.24, -0.035], [0, 0.035]]) {
    const panel = new THREE.Group(); panel.position.set(x, 0.08, z); frame.add(panel);
    box(panel, [panelWidth, 2.02, 0.018], [0, 1.01, 0], m.glass, 0);
    for (const side of [-1, 1])
      box(panel, [0.035, 2.06, 0.045], [side * panelWidth / 2, 1.01, 0], m.aluminum);
    for (const y of [0, 2.04]) box(panel, [panelWidth, 0.035, 0.045], [0, y, 0], m.aluminum);
    box(panel, [0.025, 0.22, 0.05], [-panelWidth / 2 + 0.065, 1.03, 0.04], m.metal);
  }
}

/** 入户门只作为朝向参照，与厨房门共用横向门框，尺寸仍为示意。 */
function createEntryDoor(room, m, door) {
  const frame = doorFrame(room, m, ...door.center, door.width);
  frame.name = 'home-entry-door';
  box(frame, [door.width - 0.08, 2.1, 0.045], [0, 1.06, 0.035], m.door);
  box(frame, [0.13, 0.03, 0.07], [door.width / 2 - 0.18, 1.05, -0.01], m.brass);
}

export function createArchitecture(room, m) {
  const cutWalls = [];
  box(room, [7.3, 0.22, 7.2], [-0.1, -0.15, 0.05], m.base, 0.08);
  for (const zone of HOME_LAYOUT.rooms) {
    for (const section of zone.floorSections ?? [zone])
      tiledFloor(room, m, section.center, section.size, ['kitchen', 'bathroom'].includes(zone.id));
  }
  // 左侧原来误放厨房的位置恢复成过道，不再放厨具或厨房门。
  tiledFloor(room, m, [-1.2, 2.1], [2.4, 2.8]);
  const wall = (width, depth, x, z, cut = true) => {
    const mesh = box(room, [width, HOME_LAYOUT.wallHeight, depth], [x, HOME_LAYOUT.wallHeight / 2, z], m.wall, 0.006);
    if (cut) cutWalls.push(mesh);
  };
  // 背墙卧室留窗洞，阳台使用玻璃窗；前侧和右侧默认做低剖面，便于看清家具。
  wall(0.14, 6.9, -3.64, 0.05, false);
  wall(0.14, 6.9, 3.44, 0.05);
  const entry = HOME_LAYOUT.doors.entry;
  const entryLeft = entry.center[0] - entry.width / 2;
  const entryRight = entry.center[0] + entry.width / 2;
  wall(entryLeft + 3.6, 0.14, (entryLeft - 3.6) / 2, entry.center[1]);
  wall(3.4 - entryRight, 0.14, (entryRight + 3.4) / 2, entry.center[1]);
  createEntryDoor(room, m, entry);
  wall(0.5, 0.14, -3.35, -3.44, false);
  wall(0.5, 0.14, -0.25, -3.44, false);
  box(room, [2.65, 0.6, 0.14], [-1.8, 0.3, -3.44], m.wall);
  slidingGlass(room, m, -1.8, -3.45, 2.65, 2.22);
  slidingGlass(room, m, 1.7, -3.44, 3.4, 2.45, true);
  slidingGlass(room, m, 1.7, -1.9, 3.35);
  // 客厅与卧室之间的墙保留通道，而不是用纹理画一扇假门。
  wall(0.14, 2.95, 0, -1.925);
  // 左侧只保留卧室通道；厨房空间和门洞全部移入右侧客厅范围。
  wall(0.14, 2.55, 0, 2.225);
  const bedroomDoor = doorFrame(room, m, 0, 0.22, 1.04, true);
  const pivot = new THREE.Group(); pivot.position.set(-0.52, 0, 0); bedroomDoor.add(pivot);
  const leaf = box(pivot, [1.0, 2.1, 0.045], [0.5, 1.05, 0], m.door); pivot.rotation.y = -0.95;
  box(leaf, [0.04, 0.03, 0.11], [0.37, -0.08, 0.05], m.brass);
  wall(1.25, 0.12, -2.95, 0.72);
  wall(0.6, 0.12, -0.3, 0.72);
  wall(0.1, 0.09, -2.4, 0.745);
  wall(0.1, 1.99, -2.4, 2.505);
  const kitchen = homeRoom('kitchen'), door = HOME_LAYOUT.doors.kitchen;
  const [kx, kz] = kitchen.center, [kw, kd] = kitchen.size;
  const sideWidth = (kw - door.width) / 2;
  wall(0.1, kd, kx + kw / 2, kz);
  for (const side of [-1, 1])
    wall(sideWidth, 0.1, kx + side * (kw / 2 - sideWidth / 2), door.center[1]);
  createKitchenDoor(room, m, door);
  // 卫生间门采用视频里的浅金色铝框；花纹在本地生成，不放家庭照片。
  doorFrame(room, {...m, door: m.aluminum}, -2.4, 1.15, 0.72, true);
  box(room, [0.018, 1.85, 0.62], [-2.4, 1.08, 1.15], m.frosted);
  cylinder(room, 0.018, 0.018, 0.12, [-2.36, 1.03, 1.34], m.brass);
  return {setCutaway(enabled) {
    for (const mesh of cutWalls) {
      const height = enabled ? HOME_LAYOUT.cutawayHeight : HOME_LAYOUT.wallHeight;
      mesh.scale.y = height / HOME_LAYOUT.wallHeight;
      mesh.position.y = height / 2;
    }
  }};
}
