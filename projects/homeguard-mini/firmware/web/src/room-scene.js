/** Three.js在访问网页的手机/电脑上运行，ESP32只负责发送文件和传感器数据。 */
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { createRoom } from './room-model.js';

export function createRoomScene(container) {
  const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 1.5));
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 1.35;
  container.appendChild(renderer.domElement);

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(35, 1, 0.1, 80);
  camera.position.set(10.5, 9.2, 12.2);
  const controls = new OrbitControls(camera, renderer.domElement);
  controls.target.set(0, 0.3, 0);
  controls.enablePan = false;
  controls.minDistance = 9;
  controls.maxDistance = 23;
  controls.minPolarAngle = 0.3;
  controls.maxPolarAngle = Math.PI / 2.15;
  controls.minAzimuthAngle = -0.25;
  controls.maxAzimuthAngle = Math.PI / 2;
  controls.enableDamping = true;
  controls.update();
  controls.saveState();

  const ambient = new THREE.HemisphereLight('#fff9e8', '#a7b99c', 2.7);
  scene.add(ambient);
  const sunlight = new THREE.DirectionalLight('#fff2d1', 3.2);
  sunlight.position.set(2, 9, 4);
  sunlight.castShadow = true;
  sunlight.shadow.mapSize.set(1024, 1024);
  sunlight.shadow.camera.left = -8;
  sunlight.shadow.camera.right = 8;
  sunlight.shadow.camera.top = 8;
  sunlight.shadow.camera.bottom = -8;
  sunlight.shadow.normalBias = 0.04;
  scene.add(sunlight);
  const { room, rings, beacon, person } = createRoom();
  scene.add(room);

  const reducedMotion = window.matchMedia('(prefers-reduced-motion: reduce)');
  let paused = reducedMotion.matches;
  let frame = 0;
  let previousAt = 0;
  let active = true;
  let latest = null;
  let destroyed = false;
  const initialAt = performance.now();
  const animate = (now) => {
    if (destroyed) return;
    frame = requestAnimationFrame(animate);
    // 大屏限制为约30帧；切到后台或滚出可视区时停止绘制，减少手机发热。
    if (!active || document.hidden || now - previousAt < 33) return;
    previousAt = now;
    controls.update();
    if (!paused) animateRings((now - initialAt) / 1000);
    renderer.render(scene, camera);
  };
  function animateRings(seconds) {
    rings.forEach((ring, index) => {
      const progress = (seconds * 0.23 + index / rings.length) % 1;
      ring.scale.setScalar(0.3 + progress * 3.2);
      ring.material.opacity = (1 - progress) * (latest ? 0.3 : 0.08);
    });
    if (latest?.presence) person.position.y = Math.sin(seconds * 1.5) * 0.015;
  }
  const resize = new ResizeObserver(() => {
    const { width, height } = container.getBoundingClientRect();
    if (!width || !height) return;
    camera.aspect = width / height;
    // 宽屏里放大房间，手机竖屏留出边缘，避免家具被裁掉。
    camera.zoom = width > 600 ? 1.45 : 1.05;
    camera.updateProjectionMatrix();
    renderer.setSize(width, height);
  });
  resize.observe(container);
  const visibility = new IntersectionObserver(([entry]) => { active = entry.isIntersecting; });
  visibility.observe(container);
  const handleMotion = (event) => { paused = event.matches; };
  reducedMotion.addEventListener('change', handleMotion);
  animateRings(0);
  frame = requestAnimationFrame(animate);

  return {
    get paused() { return paused; },
    setPaused(value) { paused = value; },
    reset() { controls.reset(); },
    update(state) {
      latest = state;
      person.visible = Boolean(state?.presence);
      const color = state == null ? '#9da89a' : state.level === 2 ? '#d97857'
        : state.level === 1 ? '#cfb367' : '#86b79d';
      rings.forEach((ring) => ring.material.color.set(color));
      beacon.material.color.set(color);
      // 光照只改变示意房间明暗，不据此推断真实天气或昼夜。
      const light = state?.light == null ? 1 : Math.min(1, Math.max(0, state.light) / 600);
      sunlight.intensity = 1.2 + light * 2;
      ambient.intensity = 1.7 + light;
    },
    destroy() {
      destroyed = true;
      cancelAnimationFrame(frame);
      resize.disconnect();
      visibility.disconnect();
      reducedMotion.removeEventListener('change', handleMotion);
      controls.dispose();
      scene.traverse((object) => {
        object.geometry?.dispose();
        object.material?.dispose();
      });
      renderer.dispose();
      renderer.domElement.remove();
    },
  };
}
