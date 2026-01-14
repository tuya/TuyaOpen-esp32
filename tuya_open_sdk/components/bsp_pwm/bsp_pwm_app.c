#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
static const char *TAG = "bsp_pwm_app";


#include "bsp_pwm.h"



// 配置 LEDC 定时器
ledc_channel_config_t ledc_channel[] = {
    {
        .channel = LEDC_CHANNEL_0,         // 指定通道号，这里是低速通道0
        .duty = 0,                         // 初始占空比设置为0，即LED初始状态为关闭
        .gpio_num = GPIO_NUM_1,            // 指定与该通道相连的GPIO引脚号
        .speed_mode = LEDC_LOW_SPEED_MODE, // 指定速度模式，这里是低速模式
        .hpoint = 0,                       // 高点设置，通常用于特定波形生成，这里设置为0
        .timer_sel = LEDC_TIMER_1,         // 指定定时器索引，这里是低速定时器
        .flags.output_invert = 0,          // 输出反转标志，0表示不反转，1表示反转
    },
    {
        .channel = LEDC_CHANNEL_1,         // 指定通道号，这里是低速通道0
        .duty = 0,                         // 初始占空比设置为0，即LED初始状态为关闭
        .gpio_num = GPIO_NUM_2,            // 指定与该通道相连的GPIO引脚号
        .speed_mode = LEDC_LOW_SPEED_MODE, // 指定速度模式，这里是低速模式
        .hpoint = 0,                       // 高点设置，通常用于特定波形生成，这里设置为0
        .timer_sel = LEDC_TIMER_1,         // 指定定时器索引，这里是低速定时器
        .flags.output_invert = 0,          // 输出反转标志，0表示不反转，1表示反转
    },
    {
        .channel = LEDC_CHANNEL_2,         // 指定通道号，这里是低速通道0
        .duty = 0,                         // 初始占空比设置为0，即LED初始状态为关闭
        .gpio_num = 42,                    // 指定与该通道相连的GPIO引脚号
        .speed_mode = LEDC_LOW_SPEED_MODE, // 指定速度模式，这里是低速模式
        .hpoint = 0,                       // 高点设置，通常用于特定波形生成，这里设置为0
        .timer_sel = LEDC_TIMER_1,         // 指定定时器索引，这里是低速定时器
        .flags.output_invert = 0,          // 输出反转标志，0表示不反转，1表示反转
    },
    {
        .channel = LEDC_CHANNEL_3,         // 指定通道号，这里是低速通道0
        .duty = 0,                         // 初始占空比设置为0，即LED初始状态为关闭
        .gpio_num = 41,                    // 指定与该通道相连的GPIO引脚号
        .speed_mode = LEDC_LOW_SPEED_MODE, // 指定速度模式，这里是低速模式
        .hpoint = 0,                       // 高点设置，通常用于特定波形生成，这里设置为0
        .timer_sel = LEDC_TIMER_1,         // 指定定时器索引，这里是低速定时器
        .flags.output_invert = 0,          // 输出反转标志，0表示不反转，1表示反转
    },
    {
        .channel = LEDC_CHANNEL_4,         // 指定通道号，这里是低速通道0
        .duty = 0,                         // 初始占空比设置为0，即LED初始状态为关闭
        .gpio_num = 40,                    // 指定与该通道相连的GPIO引脚号
        .speed_mode = LEDC_LOW_SPEED_MODE, // 指定速度模式，这里是低速模式
        .hpoint = 0,                       // 高点设置，通常用于特定波形生成，这里设置为0
        .timer_sel = LEDC_TIMER_1,         // 指定定时器索引，这里是低速定时器
        .flags.output_invert = 0,          // 输出反转标志，0表示不反转，1表示反转
    },
    {
        .channel = LEDC_CHANNEL_5,         // 指定通道号，这里是低速通道0
        .duty = 0,                         // 初始占空比设置为0，即LED初始状态为关闭
        .gpio_num = 39,                    // 指定与该通道相连的GPIO引脚号
        .speed_mode = LEDC_LOW_SPEED_MODE, // 指定速度模式，这里是低速模式
        .hpoint = 0,                       // 高点设置，通常用于特定波形生成，这里设置为0
        .timer_sel = LEDC_TIMER_1,         // 指定定时器索引，这里是低速定时器
        .flags.output_invert = 0,          // 输出反转标志，0表示不反转，1表示反转
    },
    {
        .channel = LEDC_CHANNEL_6,         // 指定通道号，这里是低速通道0
        .duty = 0,                         // 初始占空比设置为0，即LED初始状态为关闭
        .gpio_num = 38,                    // 指定与该通道相连的GPIO引脚号
        .speed_mode = LEDC_LOW_SPEED_MODE, // 指定速度模式，这里是低速模式
        .hpoint = 0,                       // 高点设置，通常用于特定波形生成，这里设置为0
        .timer_sel = LEDC_TIMER_1,         // 指定定时器索引，这里是低速定时器
        .flags.output_invert = 0,          // 输出反转标志，0表示不反转，1表示反转
    },
    {
        .channel = LEDC_CHANNEL_7,         // 指定通道号，这里是低速通道0
        .duty = 0,                         // 初始占空比设置为0，即LED初始状态为关闭
        .gpio_num = 47,                    // 指定与该通道相连的GPIO引脚号
        .speed_mode = LEDC_LOW_SPEED_MODE, // 指定速度模式，这里是低速模式
        .hpoint = 0,                       // 高点设置，通常用于特定波形生成，这里设置为0
        .timer_sel = LEDC_TIMER_1,         // 指定定时器索引，这里是低速定时器
        .flags.output_invert = 0,          // 输出反转标志，0表示不反转，1表示反转
    },
};


void bsp_pwm_app(void)
{

    pwmConfig.ledc_channel = ledc_channel;
    pwmConfig.ch_len = sizeof(ledc_channel) / sizeof(ledc_channel[0]);//计算通道数量
	// 动态申请 pwmConfig.ch 数组的内存空间，大小为结构体数组的长度乘以每个结构体的大小 ch_len
    if (pwmConfig.ch)
    {
        free(pwmConfig.ch);
        pwmConfig.ch = NULL;
    }
    pwmConfig.ch = (Tch *)malloc(pwmConfig.ch_len * sizeof(Tch));
    for (size_t i = 0; i < pwmConfig.ch_len; i++)
    {
        pwmConfig.ch[i].max_fade_time_ms = 3000;//设置渐变时间
    }

    bsp_pwm_main();             // 调用 PWM 初始化函数


    while (1) {
        //渐变模式
        for (int i = 0; i < pwmConfig.ch_len; i++) {
            setCHfade(i,4095);
        }
        vTaskDelay(6000 / portTICK_PERIOD_MS);

        for (int i = 0; i < pwmConfig.ch_len; i++) {
            setCHfade(i,0);
        }
        vTaskDelay(6000 / portTICK_PERIOD_MS);
        //立即模式
        for (size_t i = 0; i < pwmConfig.ch_len; i++)
        {
            setCH_duty(i, 4095);
        }
        vTaskDelay(6000 / portTICK_PERIOD_MS);
        for (size_t i = 0; i < pwmConfig.ch_len; i++)
        {
            setCH_duty(i, 0);
        }
        vTaskDelay(6000 / portTICK_PERIOD_MS);
    }


}



