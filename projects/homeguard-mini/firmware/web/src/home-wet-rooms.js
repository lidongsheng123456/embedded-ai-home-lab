/** 厨房与卫生间按视频中的橱柜、侧吸油烟机、热水器和蹲便组合。 */
import * as THREE from 'three';
import {box, cylinder} from './room-geometry.js';
import {homeRoom} from './home-layout.js';

function kitchen(parent, m) {
  const zone = homeRoom('kitchen');
  const room = new THREE.Group();
  // 厨具整体跟随厨房区域移动，内部坐标以厨房中心为原点，避免留在卧室一侧。
  room.name = 'kitchen-furniture';
  room.position.set(zone.center[0], 0, zone.center[1]); parent.add(room);
  box(room, [1.96, 0.76, 0.55], [0.02, 0.41, 0.59], m.wood);
  box(room, [2.05, 0.07, 0.61], [0.02, 0.83, 0.57], m.white);
  for (const x of [-0.75, -0.25, 0.25, 0.75]) {
    box(room, [0.46, 0.66, 0.025], [x, 0.43, 0.3], m.wood);
    box(room, [0.025, 0.19, 0.035], [x + 0.15, 0.55, 0.27], m.metal);
  }
  // 视频里的单灶和炒锅，不加并未接入的“智能开火”状态。
  box(room, [0.48, 0.04, 0.37], [0.04, 0.89, 0.52], m.dark);
  cylinder(room, 0.17, 0.1, 0.09, [0.04, 0.95, 0.52], m.dark);
  box(room, [0.24, 0.025, 0.035], [0.28, 0.98, 0.52], m.dark);
  const hood = box(room, [0.74, 0.54, 0.16], [0.04, 1.58, 0.66], m.dark);
  hood.rotation.x = -0.22;
  box(room, [0.26, 0.55, 0.22], [0.04, 2.13, 0.73], m.silver);
  box(room, [0.48, 0.025, 0.32], [-0.71, 0.88, 0.54], m.silver);
  box(room, [0.37, 0.015, 0.23], [-0.71, 0.89, 0.54], m.dark, 0.04);
  cylinder(room, 0.012, 0.012, 0.25, [-0.84, 1.01, 0.73], m.metal);
  box(room, [0.12, 0.025, 0.025], [-0.79, 1.12, 0.73], m.metal);
  cylinder(room, 0.04, 0.035, 0.2, [0.73, 0.98, 0.61], m.clay);
  box(room, [1.25, 0.8, 0.018], [0.03, 1.37, 0.93], m.glass);
}

function bathroom(room, m) {
  // 左墙的储水式电热水器，圆筒轴线沿房间宽度方向。
  const heater = cylinder(room, 0.25, 0.25, 0.66, [-3.25, 2.08, 2.22], m.white);
  heater.rotation.z = Math.PI / 2;
  cylinder(room, 0.014, 0.014, 0.46, [-3.2, 1.6, 2.22], m.metal);
  const basin = cylinder(room, 0.24, 0.16, 0.14, [-3.27, 0.79, 1.36], m.white);
  basin.scale.z = 0.75;
  const bowl = cylinder(room, 0.17, 0.1, 0.025, [-3.27, 0.865, 1.36], m.silver);
  bowl.scale.z = 0.7;
  cylinder(room, 0.012, 0.012, 0.2, [-3.43, 0.95, 1.37], m.metal);
  box(room, [0.13, 0.025, 0.025], [-3.38, 1.05, 1.37], m.metal);
  // 用白色脚踏框和内凹暗槽表达蹲便，不误画成视频中不存在的坐便器。
  box(room, [0.52, 0.065, 0.77], [-3.01, 0.06, 2.77], m.white, 0.07);
  box(room, [0.22, 0.018, 0.46], [-3.01, 0.102, 2.75], m.silver, 0.07);
  box(room, [0.13, 0.02, 0.25], [-3.01, 0.115, 2.8], m.dark, 0.06);
  for (const x of [-3.2, -2.82]) for (let i = 0; i < 6; i++)
    box(room, [0.08, 0.012, 0.012], [x, 0.106, 2.56 + i * 0.065], m.tileAccent, 0.002);
  box(room, [0.44, 0.42, 0.16], [-3.01, 0.99, 3.34], m.white, 0.035);
  cylinder(room, 0.025, 0.025, 0.6, [-3.01, 0.49, 3.34], m.white);
  box(room, [0.12, 0.7, 0.035], [-2.49, 1.5, 2.45], m.metal);
  const shower = cylinder(room, 0.075, 0.075, 0.025, [-2.55, 1.88, 2.45], m.silver);
  shower.rotation.z = Math.PI / 3;
  // 窄窗和浴巾保留视频可辨识细节，所有纹理都在本机生成。
  box(room, [0.025, 0.92, 0.65], [-2.46, 1.7, 2.08], m.glass);
  box(room, [0.02, 0.54, 0.34], [-3.5, 1.25, 1.87], m.linen);
}

export function createWetRooms(room, m) { kitchen(room, m); bathroom(room, m); }
