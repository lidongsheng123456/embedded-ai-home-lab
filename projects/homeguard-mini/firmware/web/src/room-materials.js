/** 本地生成木纹和织物，不依赖外网，也避免把大贴图塞进固件。 */
import * as THREE from 'three';

function texture(draw, repeat) {
  const canvas = document.createElement('canvas');
  canvas.width = canvas.height = 256;
  draw(canvas.getContext('2d'));
  const map = new THREE.CanvasTexture(canvas);
  map.colorSpace = THREE.SRGBColorSpace;
  map.wrapS = map.wrapT = THREE.RepeatWrapping;
  map.repeat.set(...repeat);
  map.anisotropy = 4;
  return map;
}

export function createMaterials() {
  const wood = texture(ctx => {
    ctx.fillStyle = '#bc9268'; ctx.fillRect(0, 0, 256, 256);
    for (let i = 0; i < 600; i++) {
      const y = (i * 43.71) % 256;
      ctx.strokeStyle = i % 3 ? '#9c704421' : '#edd3ac35';
      ctx.lineWidth = 0.4 + (i % 5) * 0.3;
      ctx.beginPath(); ctx.moveTo(0, y);
      ctx.bezierCurveTo(80, y + Math.sin(i) * 5, 180, y - 3, 256, y + 1);
      ctx.stroke();
    }
  }, [1, 1]);
  const fabric = texture(ctx => {
    ctx.fillStyle = '#ded5c4'; ctx.fillRect(0, 0, 256, 256);
    for (let i = 0; i < 256; i += 2) {
      ctx.fillStyle = i % 4 ? '#c2b6a444' : '#fff9e955';
      ctx.fillRect(i, 0, 1, 256); ctx.fillRect(0, i, 256, 1);
    }
  }, [5, 5]);
  return {
    wood: new THREE.MeshStandardMaterial({ map: wood, roughness: 0.62 }),
    linen: new THREE.MeshStandardMaterial({ map: fabric, roughness: 0.95 }),
    sage: new THREE.MeshStandardMaterial({ color: '#839281', map: fabric, roughness: 1 }),
    clay: new THREE.MeshStandardMaterial({ color: '#c38562', roughness: 0.85 }),
    cream: new THREE.MeshStandardMaterial({ color: '#f4eddf', roughness: 0.85 }),
    dark: new THREE.MeshStandardMaterial({ color: '#4c493e', roughness: 0.62 }),
  };
}
