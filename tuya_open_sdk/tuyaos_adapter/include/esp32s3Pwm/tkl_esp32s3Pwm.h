/**
 * @file tkl_pwm.h
 * @brief Common process - adapter the pwm api
 * @version 0.1
 * @date 2021-08-06
 *
 * @copyright Copyright 2021-2022 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TKL_ESP32S3PWM_H__
#define __TKL_ESP32S3PWM_H__




#ifdef __cplusplus
extern "C" {
#endif

#include "tuya_cloud_types.h"





OPERATE_RET tkl_esp32s3Pwm_init(int channel_count, const int *gpio_list, const int *max_fade_times_ms);
void tkl_esp32s3Pwm_test(void);
void tkl_esp32s3Pwm_setDuty(int channel, uint32_t duty);//没渐变
void tkl_esp32s3Pwm_setFade(int channel, uint32_t target_duty);//渐变

// extern Tpwm tkl_esp32s3PwmFig;
// extern SemaphoreHandle_t counting_sem;//信号量



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

