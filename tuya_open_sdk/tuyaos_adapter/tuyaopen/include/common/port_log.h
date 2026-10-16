/**
 * @file port_log.h
 * @brief port 侧日志（port 内部接口）。
 *
 * 分层约束：port 不向上调用 core，故不使用 core 的 PR_* 宏（展开即 tal_log_print_secure）。
 * 本层日志直接走 stdout（esp-idf 下经 VFS 直写 UART console），格式与 port 既有日志一致
 * （卡 .log_output 亦为 printf）。
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */
#ifndef __PORT_LOG_H__
#define __PORT_LOG_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PORT_LOG_ERR(fmt, ...)     printf("[port][E] " fmt "\n", ##__VA_ARGS__)
#define PORT_LOG_WARN(fmt, ...)    printf("[port][W] " fmt "\n", ##__VA_ARGS__)
#define PORT_LOG_NOTICE(fmt, ...)  printf("[port][N] " fmt "\n", ##__VA_ARGS__)
#define PORT_LOG_DEBUG(fmt, ...)   printf("[port][D] " fmt "\n", ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* __PORT_LOG_H__ */
