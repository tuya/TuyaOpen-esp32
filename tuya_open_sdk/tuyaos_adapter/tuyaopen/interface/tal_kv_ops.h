/**
 * @file tal_kv_ops.h
 * @brief 契约层（interface）：tal_kv 平台 ops 卡类型 + 初始化配置类型。
 *
 * 分层定位与依赖方向见 tal_api_ops.h 的文件头。本头只含类型：
 *  - `tal_kv_cfg_t`（原在 core 的 tal_kv.h，随契约一并迁到本层；core 的 tal_kv.h
 *    反向 include 本头以保持其 API 面不变）
 *  - `TAL_KV_OPS_T`（卡类型）
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */
#ifndef __TAL_KV_OPS_H__
#define __TAL_KV_OPS_H__

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAL_LV_KEY_LEN 16

/**
 * @brief KV 初始化配置：seed/key 各 16 字符，port 实现决定用途
 *        （Linux port 经 SHA256 派生 AES-128 的 IV 与密钥；NVS 类平台可忽略）
 */
typedef struct {
    char seed[TAL_LV_KEY_LEN + 1];
    char key[TAL_LV_KEY_LEN + 1];
} tal_kv_cfg_t;

/**
 * @brief tal_kv 平台 ops：成员与 port 的 kv 实现一一对应
 */
typedef struct {
    /* 挂载介质 / 打开芯片 KV；见 tal_kv.h 的 tal_kv_init */
    OPERATE_RET (*init)(tal_kv_cfg_t *kv_cfg);
    /* 写入键值对；见 tal_kv.h 的 tal_kv_set */
    OPERATE_RET (*set)(const char *key, const uint8_t *value, size_t length);
    /* 读取键值对：成功时 *value 为堆内存（调用者 tal_free 释放）；见 tal_kv.h 的 tal_kv_get */
    OPERATE_RET (*get)(const char *key, uint8_t **value, size_t *length);
    /* 删除键；见 tal_kv.h 的 tal_kv_del */
    OPERATE_RET (*del)(const char *key);
} TAL_KV_OPS_T;

#ifdef __cplusplus
}
#endif

#endif /* __TAL_KV_OPS_H__ */
