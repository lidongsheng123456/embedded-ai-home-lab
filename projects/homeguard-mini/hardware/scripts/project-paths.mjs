import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';

// 设计输入以脚本所在工程为准，避免从其他目录调用时误读同名文件。
export const hardwareRoot = resolve(dirname(fileURLToPath(import.meta.url)), '..');
export const hardwarePath = (...parts) => resolve(hardwareRoot, ...parts);
