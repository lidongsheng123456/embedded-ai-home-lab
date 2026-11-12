/** 房间名称是静态导览标注，不表示每个房间都安装了传感器。 */
import * as THREE from 'three';
import {HOME_LAYOUT, homeRoom} from './home-layout.js';

export function createHomeLabels(room) {
  const positions = {living: [2.18, 0.75], bedroom: [-1.7, -0.2], kitchen: homeRoom('kitchen').center,
    bathroom: [-3.02, 1.86], balcony: [0.6, -2.65]};
  for (const zone of HOME_LAYOUT.rooms) {
    const canvas = document.createElement('canvas'); canvas.width = 192; canvas.height = 64;
    const context = canvas.getContext('2d');
    context.fillStyle = '#fffaf0e6'; context.beginPath(); context.roundRect(4, 4, 184, 56, 20); context.fill();
    context.fillStyle = '#59654e'; context.font = '40px "Microsoft YaHei", sans-serif';
    context.textAlign = 'center'; context.textBaseline = 'middle'; context.fillText(zone.name, 96, 33);
    const map = new THREE.CanvasTexture(canvas); map.colorSpace = THREE.SRGBColorSpace;
    const label = new THREE.Sprite(new THREE.SpriteMaterial({map, transparent: true, depthTest: false}));
    const [x, z] = positions[zone.id]; label.position.set(x, 0.28, z); label.scale.set(1.05, 0.35, 1);
    label.renderOrder = 5; room.add(label);
  }
}
