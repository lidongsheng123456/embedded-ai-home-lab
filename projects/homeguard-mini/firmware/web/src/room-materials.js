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
  const floral = texture(ctx => {
    ctx.fillStyle = '#f0eadc'; ctx.fillRect(0, 0, 256, 256);
    for (let i = 0; i < 24; i++) {
      const x = (i * 73) % 256, y = (i * 97) % 256;
      ctx.strokeStyle = '#b3b29c60'; ctx.lineWidth = 1;
      ctx.beginPath(); ctx.ellipse(x, y, 7, 2.4, i, 0, Math.PI * 2); ctx.stroke();
      ctx.fillStyle = '#cba59840'; ctx.beginPath(); ctx.arc(x + 5, y - 4, 2, 0, Math.PI * 2); ctx.fill();
    }
  }, [3, 3]);
  return {
    wood: new THREE.MeshStandardMaterial({ map: wood, roughness: 0.62 }),
    linen: new THREE.MeshStandardMaterial({ map: fabric, roughness: 0.95 }),
    sage: new THREE.MeshStandardMaterial({ color: '#839281', map: fabric, roughness: 1 }),
    clay: new THREE.MeshStandardMaterial({ color: '#c38562', roughness: 0.85 }),
    cream: new THREE.MeshStandardMaterial({ color: '#f4eddf', roughness: 0.85 }),
    dark: new THREE.MeshStandardMaterial({ color: '#4c493e', roughness: 0.62 }),
    wall: new THREE.MeshStandardMaterial({color: '#f3efe5', roughness: 0.94}),
    base: new THREE.MeshStandardMaterial({color: '#b5a68d', roughness: 0.85}),
    tile: new THREE.MeshStandardMaterial({color: '#e9e3d2', roughness: 0.28, metalness: 0.03}),
    tileAccent: new THREE.MeshStandardMaterial({color: '#c6b79c', roughness: 0.6}),
    blue: new THREE.MeshStandardMaterial({color: '#394b60', roughness: 0.96}),
    quilt: new THREE.MeshStandardMaterial({map: floral, roughness: 1}),
    door: new THREE.MeshStandardMaterial({color: '#614332', roughness: 0.55}),
    aluminum: new THREE.MeshStandardMaterial({color: '#b5aa89', roughness: 0.45, metalness: 0.45}),
    metal: new THREE.MeshStandardMaterial({color: '#797e7e', roughness: 0.4, metalness: 0.6}),
    silver: new THREE.MeshStandardMaterial({color: '#9a9e9c', roughness: 0.4, metalness: 0.25}),
    white: new THREE.MeshStandardMaterial({color: '#faf7ef', roughness: 0.35}),
    glass: new THREE.MeshStandardMaterial({color: '#b5d2d0', transparent: true, opacity: 0.12, roughness: 0.08, depthWrite: false, side: THREE.DoubleSide}),
    frosted: new THREE.MeshStandardMaterial({map: floral, color: '#dfded1', transparent: true, opacity: 0.7, roughness: 0.8, side: THREE.DoubleSide}),
    brass: new THREE.MeshStandardMaterial({color: '#b69c66', roughness: 0.4, metalness: 0.65}),
  };
}
