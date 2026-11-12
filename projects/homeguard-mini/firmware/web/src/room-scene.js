/** 场景调度：模型、效果和渲染各自独立，本模块只协调时间、镜头和实时状态。 */
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { RoomEnvironment } from 'three/addons/environments/RoomEnvironment.js';
import { animate } from 'motion';
import { createRoom } from './room-model.js';
import { createSceneRenderer } from './scene-renderer.js';
import {HOME_LAYOUT, homeRoom} from './home-layout.js';

/** 把三维视图挂到容器；返回更新状态、切换视角及释放资源的接口，硬件逻辑不进入这里。 */
export function createRoomScene(container) {
  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(36, 1, 0.1, 90);
  camera.position.set(...HOME_LAYOUT.views.overview[0]);
  const output = createSceneRenderer(container, scene, camera);
  // 室内反射环境让木材和织物有自然层次，无需在线下载HDR图片。
  const environmentSource = new RoomEnvironment();
  const environment = new THREE.PMREMGenerator(output.renderer);
  const environmentMap = environment.fromScene(environmentSource, 0.04);
  scene.environment = environmentMap.texture;
  scene.environmentIntensity = 0.35;
  environmentSource.dispose();
  environment.dispose();
  const controls = new OrbitControls(camera, output.renderer.domElement);
  controls.target.set(...HOME_LAYOUT.views.overview[1]);
  controls.enablePan = false;
  controls.enableDamping = true;
  controls.minDistance = 3.5;
  controls.maxDistance = 28;
  controls.minPolarAngle = 0.05;
  controls.maxPolarAngle = Math.PI / 2.1;
  controls.minAzimuthAngle = -Math.PI;
  controls.maxAzimuthAngle = Math.PI;
  controls.update();
  const ambient = new THREE.HemisphereLight('#fff8e8', '#b7aa91', 2.1);
  scene.add(ambient);
  const sunlight = new THREE.DirectionalLight('#fff0d1', 3.2);
  sunlight.position.set(-3, 8, 4);
  sunlight.castShadow = true;
  const shadowSize = innerWidth < 620 ? 1024 : 2048;
  sunlight.shadow.mapSize.set(shadowSize, shadowSize);
  Object.assign(sunlight.shadow.camera, { left: -8, right: 8, top: 8, bottom: -8 });
  sunlight.shadow.normalBias = 0.025;
  sunlight.shadow.bias = -0.0002;
  sunlight.shadow.radius = 3;
  scene.add(sunlight);
  const warmLight = new THREE.PointLight('#ffce99', 5, 7, 2);
  warmLight.position.set(1.8, 2.3, 0.15);
  scene.add(warmLight);
  const model = createRoom();
  scene.add(model.room);
  const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)');
  let paused = reducedMotion.matches;
  let latest = null;
  let active = true;
  let frame;
  let destroyed = false;
  let previousAt = 0;
  let viewAnimation;
  const labels = [...document.querySelectorAll('[data-anchor]')];
  const projected = new THREE.Vector3();

  function updateLabels() {
    for (const label of labels) {
      const person = label.dataset.anchor === 'person';
      label.hidden = person && !(latest?.radarValid === true && latest.presence);
      if (label.hidden) continue;
      projected.copy(model.anchors[label.dataset.anchor]).project(camera);
      const x = (projected.x * 0.5 + 0.5) * container.clientWidth;
      const y = (-projected.y * 0.5 + 0.5) * container.clientHeight;
      label.style.transform = `translate(${Math.max(12, Math.min(container.clientWidth - 120, x + 14))}px,${Math.max(90, Math.min(container.clientHeight - 100, y - 14))}px)`;
    }
  }

  function moveCamera(position, target) {
    viewAnimation?.stop();
    const startPosition = camera.position.clone();
    const startTarget = controls.target.clone();
    const endPosition = new THREE.Vector3(...position);
    const endTarget = new THREE.Vector3(...target);
    viewAnimation = animate(0, 1, { duration: reducedMotion.matches ? 0 : 1.15,
      ease: [0.2, 0.7, 0.2, 1], onUpdate(progress) {
        camera.position.lerpVectors(startPosition, endPosition, progress);
        controls.target.lerpVectors(startTarget, endTarget, progress);
        controls.update();
      } });
  }
  function setView(name) { moveCamera(...(HOME_LAYOUT.views[name] ?? HOME_LAYOUT.views.overview)); }
  controls.addEventListener('start', () => viewAnimation?.stop());
  const animateFrame = now => {
    if (destroyed) return;
    frame = requestAnimationFrame(animateFrame);
    const elapsed = now - previousAt;
    if (elapsed < 33) return;
    previousAt = now;
    if (!active || document.hidden) return;
    // 自动呼吸只作用于提示标记，不把装饰动画描述成雷达实际扫描。
    model.person.material.opacity = paused ? 0.45 : 0.38 + Math.sin(now / 1200) * 0.12;
    controls.update();
    camera.updateMatrixWorld();
    updateLabels();
    output.render();
  };
  const resize = new ResizeObserver(() => {
    const { width, height } = container.getBoundingClientRect();
    if (!width || !height) return;
    camera.aspect = width / height;
    camera.zoom = width > 620 ? 1.22 : 0.68;
    camera.updateProjectionMatrix();
    output.resize(width, height);
  });
  resize.observe(container);
  const visibility = new IntersectionObserver(([entry]) => { active = entry.isIntersecting; });
  visibility.observe(container);
  const handleMotion = event => { paused = event.matches; };
  reducedMotion.addEventListener('change', handleMotion);
  frame = requestAnimationFrame(animateFrame);
  return {
    get paused() { return paused; },
    setPaused(value) { paused = value; if (value) viewAnimation?.stop(); },
    get bloom() { return output.bloom; },
    setBloom(value) { output.setBloom(value); },
    setView,
    setCutaway: model.setCutaway,
    focusRoom(id) {
      const zone = homeRoom(id);
      if (!zone) return;
      const [x, z] = zone.center;
      moveCamera([x + 5.2, 6.2, z + 6.8], [x, 0.25, z]);
    },
    reset() { setView('overview'); },
    update(state) {
      latest = state;
      // 失联或旧固件未报告有效性时，不继续展示上次的“有人”标记。
      model.person.visible = state?.radarValid === true && Boolean(state.presence);
      const color = state == null ? '#a9a396' : ['#78916c', '#c39552', '#c06a57'][state.level];
      model.beacon.material.emissive.set(color);
      // 只示意照度变化，不推断天气或昼夜，离线时恢复中性照明。
      const light = state?.light == null ? 0.6 : Math.min(1, Math.max(0, state.light) / 600);
      sunlight.intensity = 1.8 + light * 1.4;
      warmLight.intensity = 5;
    },
    destroy() {
      destroyed = true;
      cancelAnimationFrame(frame);
      viewAnimation?.stop();
      resize.disconnect();
      visibility.disconnect();
      reducedMotion.removeEventListener('change', handleMotion);
      controls.dispose();
      scene.traverse(object => {
        object.geometry?.dispose();
        if (Array.isArray(object.material)) object.material.forEach(material => material.dispose());
        else object.material?.dispose();
        for (const material of [object.material].flat().filter(Boolean)) {
          for (const value of Object.values(material)) if (value?.isTexture) value.dispose();
        }
      });
      output.destroy();
      environmentMap.dispose();
    },
  };
}
