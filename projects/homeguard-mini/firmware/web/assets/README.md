# 本地客厅家具

本目录保留旧版示例家具及其许可供历史查阅。当前家庭视频模型已改用程序生成家具，
构建脚本不再打包或请求这张椅子；不能把本文件当作当前模型的资源清单。

`lounge-chair.glb` 来自 Khronos glTF Sample Assets 的 **Sheen Chair**：

- 原作者：Eric Chadwick / Wayfair, LLC，© 2020。
- 来源：https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/SheenChair
- 原文件：`glTF-Binary/SheenChair.glb`。
- 授权：CC0 1.0，允许修改、商用与再分发。完整文本见本目录 `CC0-1.0.txt`。
- 本项目修改：贴图最长边缩至512像素并转为WebP（质量88），保留几何、材质、法线、环境遮蔽与织物光泽；通过`EXT_texture_webp`声明格式。

木地板、沙发、窗帘等由本项目代码生成，椅子使用真实网格模型。没有复制参考作品的网页或房屋模型。
所有运行资源由ESP32本地提供，不会从第三方服务器加载。房间不是实际住宅的扫描结果。
