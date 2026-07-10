#ifndef __TKL_TASK_NOTIFY_H__
#define __TKL_TASK_NOTIFY_H__

#include "tuya_cloud_types.h"
#include "tkl_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TKL_NOTIFY_WAIT_FROEVER 0xFFFFFFFF

typedef enum {
    TUYA_NOTIFY_NO_ACTION = 0,
    TUYA_NOTIFY_SETBITS,
    TUYA_NOTIFY_INCREMENT,
    TUYA_NOTIFY_SETVALUEWITHOVERWRITE,
    TUYA_NOTIFY_SETVALUEWITHOUTOVERWRITE
} TUYA_TASK_NOTIFY_ACTION_E;

OPERATE_RET tkl_task_notify(const TKL_THREAD_HANDLE thread, uint32_t ulValue, TUYA_TASK_NOTIFY_ACTION_E action);

uint32_t tkl_task_notify_take(BOOL_T clearcount, uint32_t timeout);

OPERATE_RET tkl_task_notify_give(const TKL_THREAD_HANDLE thread);

OPERATE_RET tkl_task_notify_state_clear(const TKL_THREAD_HANDLE thread);

#ifdef __cplusplus
}
#endif

#endif /* __TKL_TASK_NOTIFY_H__ */
