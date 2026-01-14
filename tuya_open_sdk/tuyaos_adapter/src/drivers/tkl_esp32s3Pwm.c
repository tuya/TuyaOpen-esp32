// #include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
// #include "tkl_esp32s3Pwm.h"

#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"  // 添加此行，引入pdMS_TO_TICKS宏
#include "tkl_esp32s3Pwm.h"     // 你的PWM头文件



// 最大支持的PWM通道数
#define MAX_PWM_CHANNELS 8
#define MAX_duty_4095 4095 //最大占空比

// 通道配置结构
typedef struct {
    int gpio_num;         // GPIO引脚号
    int max_fade_time_ms; // 最大渐变时间(ms)
    bool initialized;     // 通道是否已初始化
} Tch;

// PWM控制器结构
typedef struct {
    Tch channels[MAX_PWM_CHANNELS]; // 通道配置数组
    int channel_count;              // 实际使用的通道数
} Tpwm;

// 全局PWM控制器实例
static Tpwm s_pwm_controller = {0};
static const char *TAG = "tkl_esp32s3Pwm";

/**
 * @brief 初始化PWM控制器
 * @param channel_count 要使用的PWM通道数量(1-8)
 * @param gpio_list GPIO引脚号数组，长度至少为 channel_count
 * @param max_fade_times_ms 每个通道的最大渐变时间数组，长度至少为 channel_count
 * @return ESP_OK表示成功，其他值表示错误
 */
OPERATE_RET tkl_esp32s3Pwm_init(int channel_count, const int *gpio_list, const int *max_fade_times_ms)
{
    if (channel_count <= 0 || channel_count > MAX_PWM_CHANNELS) {
        ESP_LOGE(TAG, "无效的通道数量: %d", channel_count);
        return OPRT_NOT_SUPPORTED;
    }

    if (!gpio_list || !max_fade_times_ms) {
        ESP_LOGE(TAG, "gpio_list或max_fade_times_ms不能为空");
        return OPRT_NOT_SUPPORTED;
    }

    // 配置LEDC定时器
    ledc_timer_config_t ledc_timer = {
        // .duty_resolution = LEDC_TIMER_13_BIT, // 占空比分辨率13位(0-8191)
        // .freq_hz = 5000,                      // 频率5kHz
        .duty_resolution = LEDC_TIMER_12_BIT, // 占空比分辨率12位(0-4095)
        .freq_hz = 10 * 1000,                 // 频率10kHz
        .speed_mode = LEDC_LOW_SPEED_MODE,    // 低速模式
        .timer_num = LEDC_TIMER_1,            // 定时器1
        .clk_cfg = LEDC_AUTO_CLK,             // 自动选择时钟源
    };

    // 应用定时器配置
    esp_err_t err = ledc_timer_config(&ledc_timer);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "配置LEDC定时器失败: %s", esp_err_to_name(err));
        return OPRT_NOT_SUPPORTED;
    }

    // 初始化所有通道
    s_pwm_controller.channel_count = channel_count;

    for (int i = 0; i < channel_count; i++) {
        // 保存通道配置
        s_pwm_controller.channels[i].gpio_num = gpio_list[i];
        s_pwm_controller.channels[i].max_fade_time_ms = max_fade_times_ms[i];
        s_pwm_controller.channels[i].initialized = false;

        // 配置LEDC通道
        ledc_channel_config_t ledc_channel = {
            .channel = (ledc_channel_t)i,
            .duty = 0,
            .gpio_num = gpio_list[i],
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .hpoint = 0,
            .timer_sel = LEDC_TIMER_1,
            .flags.output_invert = 0,
        };

        // 应用通道配置
        err = ledc_channel_config(&ledc_channel);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "配置LEDC通道 %d 失败: %s", i, esp_err_to_name(err));
            return OPRT_NOT_SUPPORTED;
        }

        // 启用渐变功能
        err = ledc_fade_func_install(0);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "安装渐变功能失败: %s", esp_err_to_name(err));
            return OPRT_NOT_SUPPORTED;
        }

        s_pwm_controller.channels[i].initialized = true;
        ESP_LOGI(TAG, "通道 %d (GPIO %d) 初始化成功", i, gpio_list[i]);
    }

    return OPRT_OK;
}

/**
 * @brief 测试函数：演示PWM功能
 */
void tkl_esp32s3Pwm_test(void)
{



    // 配置参数
    int channel_count = 1;
    int gpio_list[4] = {0};//配置通道io
    int fade_times[4] = {3000}; // 渐变时间(ms)

    // 初始化PWM
    esp_err_t err = tkl_esp32s3Pwm_init(channel_count, gpio_list, fade_times);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "PWM初始化失败: %s", esp_err_to_name(err));
        return;
    }

    if (s_pwm_controller.channel_count == 0) {
        ESP_LOGE(TAG, "PWM控制器未初始化");
        return;
    }

    ESP_LOGI(TAG, "开始PWM测试...");

    // 测试所有通道
    for (int i = 0; i < 100; i++) {
        
        int chi = 0;
        ESP_LOGI(TAG, "测试通道 %d (GPIO %d)", chi, s_pwm_controller.channels[chi].gpio_num);
        ESP_LOGI(TAG,"无渐变 4095");
        tkl_esp32s3Pwm_setDuty(chi, 4095); // 无渐变
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG,"无渐变 0");
        tkl_esp32s3Pwm_setDuty(chi, 0); // 无渐变
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG,"渐变 4095");
        tkl_esp32s3Pwm_setFade(chi, 4095); // 100%
        vTaskDelay(pdMS_TO_TICKS(s_pwm_controller.channels[chi].max_fade_time_ms));
        vTaskDelay(pdMS_TO_TICKS(5000));
        ESP_LOGI(TAG,"渐变 0");
        // 渐变到0%
        tkl_esp32s3Pwm_setFade(chi, 0);
        vTaskDelay(pdMS_TO_TICKS(s_pwm_controller.channels[chi].max_fade_time_ms));
        vTaskDelay(pdMS_TO_TICKS(3000));
    }

    ESP_LOGI(TAG, "PWM测试完成");
}

/**
 * @brief 直接设置PWM通道的占空比(无渐变)
 * @param channel 通道索引(0-7)
 * @param duty 占空比值(0-8191 for 13-bit resolution)
 */
void tkl_esp32s3Pwm_setDuty(int channel, uint32_t duty)
{
    if (channel < 0 || channel >= s_pwm_controller.channel_count) {
        ESP_LOGE(TAG, "无效的通道索引: %d", channel);
        return;
    }

    if (!s_pwm_controller.channels[channel].initialized) {
        ESP_LOGE(TAG, "通道 %d 未初始化", channel);
        return;
    }

    // 限制占空比值在有效范围内
    if (duty > MAX_duty_4095)
        duty = 4095;

    // 设置占空比(无渐变)
    esp_err_t err = ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, duty);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "设置通道 %d 占空比失败: %s", channel, esp_err_to_name(err));
        return;
    }

    // 更新占空比
    err = ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "更新通道 %d 占空比失败: %s", channel, esp_err_to_name(err));
        return;
    }

    // ESP_LOGD(TAG, "通道 %d 占空比设置为 ", channel);
}

/**
 * @brief 设置PWM通道的占空比(带渐变效果)
 * @param channel 通道索引(0-7)
 * @param target_duty 目标占空比值(0-8191 for 13-bit resolution)
 */
void tkl_esp32s3Pwm_setFade(int channel, uint32_t target_duty)
{
    if (channel < 0 || channel >= s_pwm_controller.channel_count) {
        ESP_LOGE(TAG, "无效的通道索引: %d", channel);
        return;
    }

    if (!s_pwm_controller.channels[channel].initialized) {
        ESP_LOGE(TAG, "通道 %d 未初始化", channel);
        return;
    }

    // 限制占空比值在有效范围内
    if (target_duty > MAX_duty_4095)
        target_duty = 4095;

    // // 获取当前占空比
    // uint32_t current_duty = ledc_get_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel);

    // // 计算变化量，确定渐变时间
    // uint32_t duty_change = (current_duty > target_duty) ? (current_duty - target_duty) : (target_duty - current_duty);

    // // 根据变化量按比例计算渐变时间，最大不超过配置的最大值
    // uint32_t fade_time_ms = (duty_change * s_pwm_controller.channels[channel].max_fade_time_ms) / 8191;
    // if (fade_time_ms < 10)
    //     fade_time_ms = 10; // 最小渐变时间

    // // 设置渐变
    // esp_err_t err = ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, target_duty, fade_time_ms);

    uint32_t fade_time_ms = s_pwm_controller.channels[channel].max_fade_time_ms;//渐变时间
    esp_err_t err = ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, target_duty, fade_time_ms);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "设置通道 %d 渐变失败: %s", channel, esp_err_to_name(err));
        return;
    }

    // 启动渐变
    err = ledc_fade_start(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, LEDC_FADE_NO_WAIT);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "启动通道 %d 渐变失败: %s", channel, esp_err_to_name(err));
        return;
    }

    // ESP_LOGD(TAG, "通道 %d 开始渐变到 %u (时间:  ms)", channel, target_duty);
}