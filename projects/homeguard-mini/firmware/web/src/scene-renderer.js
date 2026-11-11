/** 柔和色调与轻微高光；手机默认关闭后处理，以减少显存和耗电。 */
import * as THREE from 'three';
import { EffectComposer, RenderPass, EffectPass, BloomEffect,
  ToneMappingEffect, ToneMappingMode } from 'postprocessing';

/** 封装画质差异；调用方只需render/resize，不必知道后处理是否成功创建。 */
export function createSceneRenderer(container, scene, camera) {
  const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true, powerPreference: 'high-performance' });
  renderer.setPixelRatio(Math.min(devicePixelRatio, innerWidth < 620 ? 1.25 : 1.5));
  renderer.outputColorSpace = THREE.SRGBColorSpace;
  renderer.shadowMap.enabled = true;
  renderer.shadowMap.type = THREE.PCFSoftShadowMap;
  renderer.toneMapping = THREE.ACESFilmicToneMapping;
  renderer.toneMappingExposure = 0.95;
  container.appendChild(renderer.domElement);
  let enabled = innerWidth >= 620;
  let composer;
  try {
    composer = new EffectComposer(renderer, { frameBufferType: THREE.HalfFloatType });
    composer.addPass(new RenderPass(scene, camera));
    const bloom = new BloomEffect({ intensity: 0.12, luminanceThreshold: 1.1,
      luminanceSmoothing: 0.5, mipmapBlur: true });
    const toneMapping = new ToneMappingEffect({ mode: ToneMappingMode.ACES_FILMIC });
    composer.addPass(new EffectPass(camera, bloom, toneMapping));
  } catch {
    // 辉光依赖额外显存；创建失败时仍提供普通渲染，不影响查看数据。
    composer?.dispose();
    composer = null;
    enabled = false;
  }
  return {
    renderer,
    get bloom() { return enabled && Boolean(composer); },
    setBloom(value) { enabled = value && Boolean(composer); },
    resize(width, height) {
      renderer.setSize(width, height);
      composer?.setSize(width, height);
    },
    render() {
      if (enabled && composer) composer.render();
      else renderer.render(scene, camera);
    },
    destroy() { composer?.dispose(); renderer.dispose(); renderer.domElement.remove(); },
  };
}
