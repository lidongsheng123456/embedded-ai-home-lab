#include "ui/web_assets.h"

#include <Arduino.h>
#include <string.h>

// PlatformIO预构建脚本生成此头文件；请修改web/源码，不手改生成内容。
#include "web_bundle.h"

const WebAsset *findWebAsset(const char *path) {
  for (const WebAsset &asset : WEB_ASSETS) {
    if (strcmp(path, asset.path) == 0) return &asset;
  }
  return nullptr;
}
