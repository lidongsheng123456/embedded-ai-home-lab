#pragma once

#include <stddef.h>
#include <stdint.h>

// 网页构建后成为Flash中的压缩字节，不占用同等大小的运行时RAM。
struct WebAsset {
  const char *path;         // 浏览器请求路径，例如/app.js。
  const char *contentType;  // HTTP告诉浏览器这是HTML、CSS还是JavaScript。
  const uint8_t *data;      // 指向Flash中的gzip压缩内容。
  size_t length;           // 压缩后的字节数，不能用字符串长度计算。
};

// 按固定路径查找网页资源；找不到时返回nullptr，不读取任意本地文件。
const WebAsset *findWebAsset(const char *path);
