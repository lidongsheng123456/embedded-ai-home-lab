/** 家具glTF模型在本地加载；失败不影响环境数据和操作。 */
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';

function release(root) {
  root.traverse(object => {
    object.geometry?.dispose();
    const materials = Array.isArray(object.material) ? object.material : [object.material];
    for (const material of materials.filter(Boolean)) {
      for (const value of Object.values(material)) if (value?.isTexture) value.dispose();
      material.dispose();
    }
  });
}

export function loadRoomAssets(room) {
  let disposed = false;
  new GLTFLoader().load('/assets/lounge-chair.glb', gltf => {
    if (disposed) { release(gltf.scene); return; }
    const model = gltf.scene;
    const bounds = new THREE.Box3().setFromObject(model);
    const size = bounds.getSize(new THREE.Vector3());
    const center = bounds.getCenter(new THREE.Vector3());
    const scale = 1.25 / size.y;
    model.position.set(-center.x * scale, -bounds.min.y * scale, -center.z * scale);
    model.scale.setScalar(scale);
    model.traverse(object => {
      if (!object.isMesh) return;
      object.castShadow = object.receiveShadow = true;
      if (object.material.name.startsWith('fabric')) {
        // 保留原模型织物贴图与光泽，只把芒果橙调整为更柔和的陶土色。
        object.material.color.set('#b98a68');
      }
    });
    const chair = new THREE.Group(); chair.add(model);
    chair.position.set(2, 0, 1.05); chair.rotation.y = -0.55;
    room.add(chair);
    const note = document.getElementById('model-note');
    if (note) note.textContent = '本地家具模型 · 可拖动探索';
  }, undefined, () => {
    if (disposed) return;
    const note = document.getElementById('model-note');
    if (note) note.textContent = '家具资源加载失败，可刷新重试；设备功能仍可使用。';
  });
  return { dispose() { disposed = true; } };
}
