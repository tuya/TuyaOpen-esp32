/**
 * @file tal_api_ops.h
 * @brief 契约层（interface）：tal_api 平台 ops 卡类型。
 *
 * **分层定位**：本头是 core（上层）与 port（下层）之间的**中性契约**，物理上放在
 * port 下（每个芯片平台各一份，见 platform/<P>/port/interface）。依赖方向：
 *   core ──include──▶ 本头 ◀──include── port      # 谁都不依赖谁
 *   core ──extern tal_api_ops_platform / 经卡调用──▶ port   # 上层→下层，允许
 *   port ──▶ core                                  # 禁止（include 与调用都不许）
 * 本头**只含类型**（结构体 + 句柄/回调别名），不含任何 core 实现声明。
 *
 * 句柄/回调别名与 core 的 tal_semaphore/tal_mutex/tal_thread/tal_queue 定义逐字一致
 * （同为 void* / void(*)(void*) 别名）；C11 起重复 typedef 合法，core 的 TU 同时
 * include 两侧不会冲突。
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */
#ifndef __TAL_API_OPS_H__
#define __TAL_API_OPS_H__

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 句柄/回调别名（与 core 的 tal_* 头定义一致） */
typedef void *SEM_HANDLE;
typedef void *MUTEX_HANDLE;
typedef void *THREAD_HANDLE;
typedef void (*THREAD_FUNC_CB)(void *args);
typedef void *QUEUE_HANDLE;

/**
 * @brief tal_api 平台 ops：成员与 port 的 memory/system/semaphore/mutex/thread/queue/log
 *        实现一一对应，只收编实际被 tal 层分发的接口（OTA 为回调注入，见 tal_ota.h）
 */
typedef struct {
    //! log output（启动早期即被 tal_log 回调与 QR 输出使用，置卡首位）
    void (*log_output)(const char *format, ...);
    //! memory
    void *(*malloc)(size_t size);
    void (*free)(void *ptr);
    //! system
    void (*sleep)(uint32_t num_ms);
    void (*reset)(void);
    SYS_TIME_T (*get_millisecond)(void);
    int (*get_random)(uint32_t range);
    //! semaphore
    OPERATE_RET (*semaphore_create_init)(SEM_HANDLE *handle, uint32_t sem_cnt, uint32_t sem_max);
    OPERATE_RET (*semaphore_wait)(const SEM_HANDLE handle, uint32_t timeout);
    OPERATE_RET (*semaphore_post)(const SEM_HANDLE handle);
    OPERATE_RET (*semaphore_release)(const SEM_HANDLE handle);
    //! mutex
    OPERATE_RET (*mutex_create_init)(MUTEX_HANDLE *handle);
    OPERATE_RET (*mutex_lock)(const MUTEX_HANDLE handle);
    OPERATE_RET (*mutex_unlock)(const MUTEX_HANDLE handle);
    OPERATE_RET (*mutex_release)(const MUTEX_HANDLE handle);
    //! thread
    OPERATE_RET (*thread_create)(THREAD_HANDLE *thread, const char *name, uint32_t stack_size, uint32_t priority,
                                 THREAD_FUNC_CB func, void *const arg);
    OPERATE_RET (*thread_release)(const THREAD_HANDLE thread);
    OPERATE_RET (*thread_set_self_name)(const char *name);
    OPERATE_RET (*thread_is_self)(const THREAD_HANDLE thread, BOOL_T *is_self);
    OPERATE_RET (*thread_get_id)(THREAD_HANDLE *thread);
    //! queue
    OPERATE_RET (*queue_create_init)(QUEUE_HANDLE *queue, int msgsize, int msgcount);
    OPERATE_RET (*queue_post)(const QUEUE_HANDLE queue, void *data, uint32_t timeout);
    OPERATE_RET (*queue_fetch)(const QUEUE_HANDLE queue, void *msg, uint32_t timeout);
    void (*queue_free)(const QUEUE_HANDLE queue);
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
    //! ex ram（外部 RAM，如 PSRAM；宏隔离：未开启时卡不含这些成员，零开销）
    void *(*ex_ram_malloc)(size_t size);
    void (*ex_ram_free)(void *ptr);
#endif
} TAL_API_OPS_T;

#ifdef __cplusplus
}
#endif

#endif /* __TAL_API_OPS_H__ */
