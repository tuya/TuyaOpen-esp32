
#ifndef __BSP_PWM_H__
#define __BSP_PWM_H__
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "driver/ledc.h"

#include <stdint.h>
#include "driver/gpio.h"

typedef struct
{
    int max_fade_time_ms;//最大渐变时间
} Tch;
// 参数
typedef struct
{
    ledc_channel_config_t *ledc_channel;
    Tch *ch;
    int ch_len; // 通道数量;
} Tpwm;

void bsp_pwm_main(void);
void bsp_pwm_app(void);
void setCH_duty(int i, uint32_t duty);
void setCHfade(int i, uint32_t target_duty);

extern Tpwm pwmConfig;
extern SemaphoreHandle_t counting_sem;//信号量


#endif


