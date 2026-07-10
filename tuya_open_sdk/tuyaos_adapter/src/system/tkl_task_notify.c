/**
 * @file tkl_task_notify.c
 * @brief FreeRTOS task notification adapter for ESP32
 */

#include "tuya_cloud_types.h"
#include "tkl_task_notify.h"
#include "tkl_thread.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

OPERATE_RET tkl_task_notify(const TKL_THREAD_HANDLE thread, uint32_t ulValue, TUYA_TASK_NOTIFY_ACTION_E action)
{
    if (!thread) {
        return OPRT_INVALID_PARM;
    }

    BaseType_t res;
    if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        res = xTaskNotifyFromISR((TaskHandle_t)thread, ulValue, (eNotifyAction)action, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        res = xTaskNotify((TaskHandle_t)thread, ulValue, (eNotifyAction)action);
    }

    return (res == pdPASS) ? OPRT_OK : OPRT_COM_ERROR;
}

uint32_t tkl_task_notify_take(BOOL_T clearcount, uint32_t timeout)
{
    TickType_t ticks;
    if (timeout == TKL_NOTIFY_WAIT_FROEVER) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeout);
        if (ticks == 0) ticks = 1;
    }

    return ulTaskNotifyTake(clearcount, ticks);
}

OPERATE_RET tkl_task_notify_give(const TKL_THREAD_HANDLE thread)
{
    if (!thread) {
        return OPRT_INVALID_PARM;
    }

    if (xPortInIsrContext()) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR((TaskHandle_t)thread, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        xTaskNotifyGive((TaskHandle_t)thread);
    }

    return OPRT_OK;
}

OPERATE_RET tkl_task_notify_state_clear(const TKL_THREAD_HANDLE thread)
{
    if (!thread) {
        return OPRT_INVALID_PARM;
    }

    xTaskNotifyStateClear((TaskHandle_t)thread);
    return OPRT_OK;
}
