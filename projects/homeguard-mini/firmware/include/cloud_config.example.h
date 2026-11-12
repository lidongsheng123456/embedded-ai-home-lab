#pragma once

// 复制为cloud_config.h后只在本机填写。该文件被Git忽略，禁止把密钥发到聊天或写入网页。
// 开关和密钥同时有效时才启用云解读；没有配置时原有本地采集与告警照常工作。
#define HOMEGUARD_DEEPSEEK_ENABLED true
#define HOMEGUARD_DEEPSEEK_API_KEY ""

// 2026-11-12核对官方文档的模型名称。账户不支持时按官方控制台可用名称调整。
#define HOMEGUARD_DEEPSEEK_MODEL "deepseek-flash"
