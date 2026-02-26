/**
 * @file tkl_pin.c
 * @brief GPIO driver adapter for ESP-IDF platform.
 *
 * This file implements the TKL GPIO interface using the ESP-IDF gpio_config() API,
 * following the official ESP-IDF GPIO example:
 * https://github.com/espressif/esp-idf/blob/v5.5.2/examples/peripherals/gpio/generic_gpio/main/gpio_example_main.c
 *
 * Key design decisions:
 * 1. Use gpio_config() for one-shot GPIO configuration instead of separate calls.
 * 2. ISR service is installed once globally and shared across all GPIO pins.
 * 3. deinit only removes per-pin ISR handler, does not uninstall the global ISR service.
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */
#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"
#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_gpio.h"

#define DBG_TAG "TKL_PIN"

#define ESP_INTR_FLAG_DEFAULT 0

#define PIN_DEV_CHECK_ERROR_RETURN(__PIN, __ERROR)                          \
    if (__PIN >= sizeof(pinmap)/sizeof(pinmap[0])) {                        \
        return __ERROR;                                                     \
    }

typedef void (*tuya_pin_irq_cb)(void *args);

typedef struct {
    int gpio;
    tuya_pin_irq_cb cb;
    void *args;
} pin_dev_map_t;

/* Flag to track if ISR service has been installed */
static bool s_isr_service_installed = false;

static pin_dev_map_t pinmap[] = {
    {GPIO_NUM_0,  NULL, NULL}, {GPIO_NUM_1,  NULL, NULL}, {GPIO_NUM_2,  NULL, NULL}, {GPIO_NUM_3,  NULL, NULL},
    {GPIO_NUM_4,  NULL, NULL}, {GPIO_NUM_5,  NULL, NULL}, {GPIO_NUM_6,  NULL, NULL}, {GPIO_NUM_7,  NULL, NULL},
    {GPIO_NUM_8,  NULL, NULL}, {GPIO_NUM_9,  NULL, NULL}, {GPIO_NUM_10, NULL, NULL}, {GPIO_NUM_11, NULL, NULL},
    {GPIO_NUM_12, NULL, NULL}, {GPIO_NUM_13, NULL, NULL}, {GPIO_NUM_14, NULL, NULL}, {GPIO_NUM_15, NULL, NULL},
    {GPIO_NUM_16, NULL, NULL}, {GPIO_NUM_17, NULL, NULL}, {GPIO_NUM_18, NULL, NULL}, {GPIO_NUM_19, NULL, NULL},
    {GPIO_NUM_20, NULL, NULL},

    #if defined(CONFIG_IDF_TARGET_ESP32C3)
    {GPIO_NUM_21, NULL, NULL},
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
    {GPIO_NUM_21, NULL, NULL}, {GPIO_NUM_22, NULL, NULL}, {GPIO_NUM_23, NULL, NULL}, {GPIO_NUM_24, NULL, NULL},
    {GPIO_NUM_25, NULL, NULL}, {GPIO_NUM_26, NULL, NULL}, {GPIO_NUM_27, NULL, NULL}, {GPIO_NUM_28, NULL, NULL},
    {GPIO_NUM_29, NULL, NULL}, {GPIO_NUM_30, NULL, NULL},
    #elif defined(CONFIG_IDF_TARGET_ESP32)
    {GPIO_NUM_21, NULL, NULL}, {GPIO_NUM_22, NULL, NULL}, {GPIO_NUM_23, NULL, NULL}, {GPIO_NUM_NC, NULL, NULL},
    {GPIO_NUM_25, NULL, NULL}, {GPIO_NUM_26, NULL, NULL}, {GPIO_NUM_27, NULL, NULL}, {GPIO_NUM_28, NULL, NULL},
    {GPIO_NUM_29, NULL, NULL}, {GPIO_NUM_30, NULL, NULL}, {GPIO_NUM_31, NULL, NULL}, {GPIO_NUM_32, NULL, NULL},
    {GPIO_NUM_33, NULL, NULL}, {GPIO_NUM_34, NULL, NULL}, {GPIO_NUM_35, NULL, NULL}, {GPIO_NUM_36, NULL, NULL},
    {GPIO_NUM_37, NULL, NULL}, {GPIO_NUM_38, NULL, NULL}, {GPIO_NUM_39, NULL, NULL},
    #elif defined(CONFIG_IDF_TARGET_ESP32S2)
    {GPIO_NUM_21, NULL, NULL}, {GPIO_NUM_NC, NULL, NULL}, {GPIO_NUM_NC, NULL, NULL}, {GPIO_NUM_NC, NULL, NULL},
    {GPIO_NUM_NC, NULL, NULL}, {GPIO_NUM_26, NULL, NULL}, {GPIO_NUM_27, NULL, NULL}, {GPIO_NUM_28, NULL, NULL},
    {GPIO_NUM_29, NULL, NULL}, {GPIO_NUM_30, NULL, NULL}, {GPIO_NUM_31, NULL, NULL}, {GPIO_NUM_32, NULL, NULL},
    {GPIO_NUM_33, NULL, NULL}, {GPIO_NUM_34, NULL, NULL}, {GPIO_NUM_35, NULL, NULL}, {GPIO_NUM_36, NULL, NULL},
    {GPIO_NUM_37, NULL, NULL}, {GPIO_NUM_38, NULL, NULL}, {GPIO_NUM_39, NULL, NULL}, {GPIO_NUM_40, NULL, NULL},
    {GPIO_NUM_41, NULL, NULL}, {GPIO_NUM_42, NULL, NULL}, {GPIO_NUM_43, NULL, NULL},
    {GPIO_NUM_44, NULL, NULL}, {GPIO_NUM_45, NULL, NULL}, {GPIO_NUM_46, NULL, NULL},
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
    {GPIO_NUM_21, NULL, NULL}, {GPIO_NUM_NC, NULL, NULL}, {GPIO_NUM_NC, NULL, NULL}, {GPIO_NUM_NC, NULL, NULL},
    {GPIO_NUM_NC, NULL, NULL}, {GPIO_NUM_26, NULL, NULL}, {GPIO_NUM_27, NULL, NULL}, {GPIO_NUM_28, NULL, NULL},
    {GPIO_NUM_29, NULL, NULL}, {GPIO_NUM_30, NULL, NULL}, {GPIO_NUM_31, NULL, NULL}, {GPIO_NUM_32, NULL, NULL},
    {GPIO_NUM_33, NULL, NULL}, {GPIO_NUM_34, NULL, NULL}, {GPIO_NUM_35, NULL, NULL}, {GPIO_NUM_36, NULL, NULL},
    {GPIO_NUM_37, NULL, NULL}, {GPIO_NUM_38, NULL, NULL}, {GPIO_NUM_39, NULL, NULL}, {GPIO_NUM_40, NULL, NULL},
    {GPIO_NUM_41, NULL, NULL}, {GPIO_NUM_42, NULL, NULL}, {GPIO_NUM_43, NULL, NULL}, {GPIO_NUM_44, NULL, NULL},
    {GPIO_NUM_45, NULL, NULL}, {GPIO_NUM_46, NULL, NULL}, {GPIO_NUM_47, NULL, NULL}, {GPIO_NUM_48, NULL, NULL},
    #endif
};

/**
 * @brief gpio init
 * Use gpio_config() for one-shot configuration, following the official ESP-IDF example.
 *
 * @param[in] pin_id: gpio pin id
 * @param[in] cfg:    gpio base config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_init(TUYA_GPIO_NUM_E pin_id, const TUYA_GPIO_BASE_CFG_T *cfg)
{
    esp_err_t ret;
    int gpio_num;

    if (NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;

    /* Zero-initialize the config structure, following ESP-IDF official example */
    gpio_config_t io_conf = {};

    /* Disable interrupt during basic GPIO init */
    io_conf.intr_type = GPIO_INTR_DISABLE;

    /* Set pin bit mask */
    io_conf.pin_bit_mask = (1ULL << gpio_num);

    /* Set direction */
    switch (cfg->direct) {
    case TUYA_GPIO_INPUT:
        io_conf.mode = GPIO_MODE_INPUT;
        break;
    case TUYA_GPIO_OUTPUT:
        io_conf.mode = GPIO_MODE_OUTPUT;
        break;
    default:
        return OPRT_NOT_SUPPORTED;
    }

    /* Set pull mode */
    switch (cfg->mode) {
    case TUYA_GPIO_PUSH_PULL:
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        break;
    case TUYA_GPIO_PULLUP:
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        break;
    case TUYA_GPIO_PULLDOWN:
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
        break;
    case TUYA_GPIO_OPENDRAIN:
        io_conf.mode = GPIO_MODE_OUTPUT_OD;
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        break;
    case TUYA_GPIO_OPENDRAIN_PULLUP:
        io_conf.mode = GPIO_MODE_OUTPUT_OD;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        break;
    case TUYA_GPIO_FLOATING:
        io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        break;
    default:
        return OPRT_NOT_SUPPORTED;
    }

    /* Configure GPIO with the given settings */
    ret = gpio_config(&io_conf);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: gpio_config failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    /* Set initial output level for output pins */
    if (cfg->direct == TUYA_GPIO_OUTPUT) {
        gpio_set_level(gpio_num, cfg->level);
    }

    return OPRT_OK;
}

/**
 * @brief gpio write
 *
 * @param[in] pin_id: gpio pin id
 * @param[in] level:  output level
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_write(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E level)
{
    int gpio_num;

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;
    gpio_set_level(gpio_num, level);

    return OPRT_OK;
}

/**
 * @brief gpio read
 *
 * @param[in]  pin_id: gpio pin id
 * @param[out] level:  input level
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_read(TUYA_GPIO_NUM_E pin_id, TUYA_GPIO_LEVEL_E *level)
{
    int gpio_num;

    if (NULL == level) {
        return OPRT_INVALID_PARM;
    }

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;

    *level = gpio_get_level(gpio_num);
    return OPRT_OK;
}

/**
 * @brief Install ISR service if not yet installed (idempotent).
 *
 * @return ESP_OK on success, or if already installed.
 */
static esp_err_t __ensure_isr_service_installed(void)
{
    if (s_isr_service_installed) {
        return ESP_OK;
    }

    esp_err_t ret = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if (ESP_OK == ret) {
        s_isr_service_installed = true;
    } else if (ESP_ERR_INVALID_STATE == ret) {
        /* ISR service was already installed by someone else */
        s_isr_service_installed = true;
        ret = ESP_OK;
    }

    return ret;
}

/**
 * @brief gpio irq init
 * NOTE: call this API will not enable interrupt (interrupt is disabled after init).
 *
 * The GPIO pin is configured as input with the specified interrupt trigger type
 * using gpio_config(). The ISR service is installed once globally.
 *
 * @param[in] pin_id: gpio pin id
 * @param[in] cfg:    gpio irq config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_init(TUYA_GPIO_NUM_E pin_id, const TUYA_GPIO_IRQ_T *cfg)
{
    int gpio_num;
    gpio_int_type_t trigger;
    esp_err_t ret;

    if (NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);

    pinmap[pin_id].cb = cfg->cb;
    pinmap[pin_id].args = cfg->arg;
    gpio_num = pinmap[pin_id].gpio;

    /* Map TUYA IRQ mode to ESP-IDF interrupt type */
    switch (cfg->mode) {
    case TUYA_GPIO_IRQ_RISE:
        trigger = GPIO_INTR_POSEDGE;
        break;
    case TUYA_GPIO_IRQ_FALL:
        trigger = GPIO_INTR_NEGEDGE;
        break;
    case TUYA_GPIO_IRQ_RISE_FALL:
        trigger = GPIO_INTR_ANYEDGE;
        break;
    case TUYA_GPIO_IRQ_LOW:
        trigger = GPIO_INTR_LOW_LEVEL;
        break;
    case TUYA_GPIO_IRQ_HIGH:
        trigger = GPIO_INTR_HIGH_LEVEL;
        break;
    default:
        return OPRT_NOT_SUPPORTED;
    }

    /*
     * IMPORTANT: Follow the safe initialization order to avoid crashes:
     * 1. Configure GPIO as input with interrupt DISABLED (prevent premature trigger)
     * 2. Install ISR service (global, once)
     * 3. Add per-pin ISR handler
     * 4. Set the actual interrupt trigger type
     * 5. Disable interrupt again (gpio_isr_handler_add re-enables it internally)
     *
     * If gpio_config() is called with intr_type != GPIO_INTR_DISABLE, the interrupt
     * fires immediately when pin state matches, BEFORE the ISR service is installed,
     * causing a crash (no handler registered).
     */

    /* Step 1: Configure GPIO as input WITHOUT interrupt */
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << gpio_num);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;

    ret = gpio_config(&io_conf);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: gpio_config failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    /* Step 2: Install ISR service (only once globally) */
    ret = __ensure_isr_service_installed();
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: gpio_install_isr_service failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    /* Step 3: Hook ISR handler for this specific GPIO pin */
    ret = gpio_isr_handler_add(gpio_num, pinmap[pin_id].cb, pinmap[pin_id].args);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: gpio_isr_handler_add failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    /* Step 4: Now set the actual interrupt trigger type */
    ret = gpio_set_intr_type(gpio_num, trigger);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: gpio_set_intr_type failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    /* Step 5: Disable interrupt, user must call tkl_gpio_irq_enable() to activate.
     * Note: gpio_isr_handler_add() internally calls gpio_intr_enable_on_core(),
     * so we must disable again here. */
    gpio_intr_disable(gpio_num);

    return OPRT_OK;
}

/**
 * @brief gpio irq enable
 *
 * @param[in] pin_id: gpio pin id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_enable(TUYA_GPIO_NUM_E pin_id)
{
    int gpio_num;
    esp_err_t ret;

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;
    ret = gpio_intr_enable(gpio_num);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: gpio_intr_enable failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief gpio irq disable
 *
 * @param[in] pin_id: gpio pin id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_irq_disable(TUYA_GPIO_NUM_E pin_id)
{
    int gpio_num;

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;

    gpio_intr_disable(gpio_num);

    return OPRT_OK;
}

/**
 * @brief gpio deinit
 * Only removes the per-pin ISR handler and resets the pin.
 * Does NOT uninstall the global ISR service to avoid affecting other GPIO interrupts.
 *
 * @param[in] pin_id: gpio pin id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_gpio_deinit(TUYA_GPIO_NUM_E pin_id)
{
    int gpio_num;

    PIN_DEV_CHECK_ERROR_RETURN(pin_id, OPRT_INVALID_PARM);
    gpio_num = pinmap[pin_id].gpio;

    /* Disable interrupt first */
    gpio_intr_disable(gpio_num);

    /* Remove per-pin ISR handler (safe even if not registered) */
    if (s_isr_service_installed) {
        gpio_isr_handler_remove(gpio_num);
    }

    /* Clear callback info */
    pinmap[pin_id].cb = NULL;
    pinmap[pin_id].args = NULL;

    /* Reset pin to default state */
    gpio_reset_pin(gpio_num);

    return OPRT_OK;
}
