#include "bsp_pwm.h"

#include <stdio.h>
#include <string.h>

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "freertos/semphr.h"
// #include "driver/ledc.h"
#include "esp_err.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "esp_log.h"



static const char *TAG = "BSP_pwm";

// static uint32_t ledcFadeTime_0 = 3000; // 淡入/淡出操作所需的时间（毫秒）
// #define CH_0 LEDC_CHANNEL_0     // 在esp32s3有0-7共8个通道

Tpwm pwmConfig = {0};
size_t num_channels = 0;
SemaphoreHandle_t counting_sem; // 信号量

// #define LEDC_TEST_DUTY (4095)      // 占空比
// #define LEDC_TEST_FADE_TIME (3000) // 淡入/淡出操作所需的时间

#define MAX_DUTY (4095) // 最大占空比
/*
LEDC_TIMER_13_BIT 13位分辨率 最大站空比为 8191 支持5khz
LEDC_TIMER_12_BIT 12位分辨率 最大站空比为 4095 支持10kHz
LEDC_TIMER_11_BIT 11位分辨率 最大站空比为 2047 支持20kHz
LEDC_TIMER_10_BIT 10位分辨率 最大站空比为 1023 支持40kHz
*/

/**
 * @brief LEDC 褪光结束事件回调函数
 *
 * 当 LEDC 褪光结束时，该回调函数会被触发。它会检查事件类型是否为 LEDC_FADE_END_EVT，
 * 如果是，则通过信号量释放一个计数信号量，以通知等待的任务。
 *
 * @param param 指向 ledc_cb_param_t 结构体的指针，包含事件相关信息
 * @param user_arg 用户传入的参数，此处用于传递信号量句柄
 * @return 返回值指示是否有任务被唤醒，pdTRUE 表示有任务被唤醒，pdFALSE 表示没有
 */
static IRAM_ATTR bool cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *user_arg)
{
    BaseType_t taskAwoken = pdFALSE; // 定义一个变量来标记是否唤醒了一个任务

    // 检查事件类型是否为淡入淡出结束事件
    if (param->event == LEDC_FADE_END_EVT)
    {
        // 将void类型的用户参数转换为SemaphoreHandle_t类型，这里假设用户参数是一个信号量的句柄
        SemaphoreHandle_t counting_sem1 = (SemaphoreHandle_t)user_arg;

        // 从中断服务例程（ISR）中释放信号量，可能会唤醒一个等待该信号量的任务
        // taskAwoken参数用于指示是否确实唤醒了某个任务
        xSemaphoreGiveFromISR(counting_sem1, &taskAwoken);
    }

    // 如果唤醒了任务，返回true；否则返回false
    // 在FreeRTOS中，从ISR返回true意味着需要调度器运行，以处理可能的任务切换
    return (taskAwoken == pdTRUE);
}

/*
    设置占空比 渐变模式
    参数
        通道,设置占空比
*/
void setCHfade(int i, uint32_t target_duty)
{
    if (target_duty > MAX_DUTY)
    {
        target_duty = MAX_DUTY;
    }
    ledc_set_fade_with_time(pwmConfig.ledc_channel[i].speed_mode,
                            pwmConfig.ledc_channel[i].channel, target_duty, pwmConfig.ch[i].max_fade_time_ms);
    ledc_fade_start(pwmConfig.ledc_channel[i].speed_mode,
                    pwmConfig.ledc_channel[i].channel, LEDC_FADE_NO_WAIT);
}

/*
设置占空比 立即模式
参数
    通道,设置占空比
*/
void setCH_duty(int i, uint32_t duty)
{
    if (duty > MAX_DUTY)
    {
        duty = MAX_DUTY;
    }
    ledc_set_duty(pwmConfig.ledc_channel[i].speed_mode, pwmConfig.ledc_channel[i].channel, duty);
    ledc_update_duty(pwmConfig.ledc_channel[i].speed_mode, pwmConfig.ledc_channel[i].channel);
}

void bsp_pwm_main(void)
{
    ESP_LOGI(TAG, "bsp_pwm_main");
    esp_err_t err = ESP_OK;
    int ch;

    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_12_BIT, // PWM占空比的分辨率
        .freq_hz = 1000 * 10,                 // PWM信号的频率，单位为赫兹
        .speed_mode = LEDC_LOW_SPEED_MODE,    // 定时器模式，此处为低速模式
        .timer_num = LEDC_TIMER_1,            // 定时器的索引，此处指定为低速定时器
        .clk_cfg = LEDC_AUTO_CLK,             // 时钟配置，此处为自动选择时钟源
    };
    ledc_timer_config(&ledc_timer);// 配置LEDC定时器
    // ledc_channel_config_t ledc_channel[] = {
    //     {
    //         .channel    = LEDC_CHANNEL_0,
    //         .duty       = 0,
    //         .gpio_num   = 0,
    //         .speed_mode = LEDC_LOW_SPEED_MODE,
    //         .hpoint     = 0,
    //         .timer_sel  = LEDC_TIMER_1,
    //         .flags.output_invert = 0
    //     },
    // };
    // pwmConfig.ledc_channel = ledc_channel;


    // pwmConfig.ch_len = sizeof(ledc_channel) / sizeof(ledc_channel[0]);//计算通道数量 

    for (ch = 0; ch < pwmConfig.ch_len; ch++) {
        ledc_channel_config(&pwmConfig.ledc_channel[ch]);
    }
    ledc_fade_func_install(0);
    ledc_cbs_t callbacks = {
        .fade_cb = cb_ledc_fade_end_event
    };

    SemaphoreHandle_t counting_sem = xSemaphoreCreateCounting(pwmConfig.ch_len, 0);

    for (ch = 0; ch < pwmConfig.ch_len; ch++) {
        ledc_cb_register(pwmConfig.ledc_channel[ch].speed_mode, pwmConfig.ledc_channel[ch].channel, &callbacks, (void *) counting_sem);
    }

    // while (1) {
    //     for (int i = 0; i < pwmConfig.ch_len; i++) {
    //         setCHfade(i,4095);
    //     }
    //     vTaskDelay(6000 / portTICK_PERIOD_MS);

    //     for (int i = 0; i < pwmConfig.ch_len; i++) {
    //         setCHfade(i,0);
    //     }
    //     vTaskDelay(6000 / portTICK_PERIOD_MS);
    // }



}

