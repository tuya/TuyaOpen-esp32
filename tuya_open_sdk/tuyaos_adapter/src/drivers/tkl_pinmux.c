/****************************************************************************
 * @file tkl_pinmux.c
 * @brief this module is used to tkl_pinmux
 * @version 0.0.1
 * @date 2023-06-07
 *
 * @copyright Copyright(C) 2021-2022 Tuya Inc. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdbool.h>
#include <string.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "soc/soc_caps.h"

#include "tuya_kconfig.h"
#include "tkl_pinmux.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/
typedef enum {
    TKL_PINMUX_FUNC_I2C0_SCL,
    TKL_PINMUX_FUNC_I2C0_SDA,
    TKL_PINMUX_FUNC_I2C1_SCL,
    TKL_PINMUX_FUNC_I2C1_SDA,
    TKL_PINMUX_FUNC_I2C2_SCL,
    TKL_PINMUX_FUNC_I2C2_SDA,
    TKL_PINMUX_FUNC_UART1_TX,
    TKL_PINMUX_FUNC_UART1_RX,
    TKL_PINMUX_FUNC_PWM0,
    TKL_PINMUX_FUNC_PWM1,
    TKL_PINMUX_FUNC_PWM2,
    TKL_PINMUX_FUNC_PWM3,
    TKL_PINMUX_FUNC_PWM4,
    TKL_PINMUX_FUNC_PWM5,
    TKL_PINMUX_FUNC_SPI0_MISO,
    TKL_PINMUX_FUNC_SPI0_MOSI,
    TKL_PINMUX_FUNC_SPI0_CLK,
    TKL_PINMUX_FUNC_SPI0_CS,
    TKL_PINMUX_FUNC_SPI1_MISO,
    TKL_PINMUX_FUNC_SPI1_MOSI,
    TKL_PINMUX_FUNC_SPI1_CLK,
    TKL_PINMUX_FUNC_SPI1_CS,
    TKL_PINMUX_FUNC_COUNT,
    TKL_PINMUX_FUNC_NONE = -1,
} TKL_PINMUX_FUNC_E;

/* Keep this in sync with the ports implemented by tkl_spi.c. */
#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S3) || \
    defined(CONFIG_IDF_TARGET_ESP32P4) || defined(CONFIG_IDF_TARGET_ESP32S31)
#define TKL_PINMUX_SPI_PORT_COUNT 2
#else
#define TKL_PINMUX_SPI_PORT_COUNT 1
#endif
/****************************************************************************
 * Private Data Declarations
 ****************************************************************************/
/*
 * The ESP GPIO matrix applies a route when the owning peripheral is
 * initialized.  Track only routes explicitly set through this API; implicit
 * driver defaults remain available to applications that do not use pinmux.
 */
static TUYA_PIN_NAME_E sg_func_pins[TKL_PINMUX_FUNC_COUNT];
static bool sg_func_pins_initialized;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void __pinmux_state_init(void)
{
    if (sg_func_pins_initialized) {
        return;
    }

    for (uint32_t i = 0; i < TKL_PINMUX_FUNC_COUNT; ++i) {
        sg_func_pins[i] = TUYA_IO_PIN_MAX;
    }
    sg_func_pins_initialized = true;
}

static bool __function_is_output(TKL_PINMUX_FUNC_E func)
{
    return func != TKL_PINMUX_FUNC_UART1_RX && func != TKL_PINMUX_FUNC_SPI0_MISO &&
           func != TKL_PINMUX_FUNC_SPI1_MISO;
}

static bool __pin_is_valid(TUYA_PIN_NAME_E pin)
{
    return pin < SOC_GPIO_PIN_COUNT && ((SOC_GPIO_VALID_GPIO_MASK & (1ULL << pin)) != 0);
}

static bool __pin_is_output_capable(TUYA_PIN_NAME_E pin)
{
    return pin < SOC_GPIO_PIN_COUNT && ((SOC_GPIO_VALID_OUTPUT_GPIO_MASK & (1ULL << pin)) != 0);
}

static OPERATE_RET __function_info(TUYA_PIN_FUNC_E pin_func, TKL_PINMUX_FUNC_E *func,
                                   bool *is_output)
{
    switch (pin_func) {
        case TUYA_IIC0_SCL:
            *func = TKL_PINMUX_FUNC_I2C0_SCL;
            break;
        case TUYA_IIC0_SDA:
            *func = TKL_PINMUX_FUNC_I2C0_SDA;
            break;
#if SOC_I2C_NUM > 1
        case TUYA_IIC1_SCL:
            *func = TKL_PINMUX_FUNC_I2C1_SCL;
            break;
        case TUYA_IIC1_SDA:
            *func = TKL_PINMUX_FUNC_I2C1_SDA;
            break;
#endif
#if SOC_I2C_NUM > 2
        case TUYA_IIC2_SCL:
            *func = TKL_PINMUX_FUNC_I2C2_SCL;
            break;
        case TUYA_IIC2_SDA:
            *func = TKL_PINMUX_FUNC_I2C2_SDA;
            break;
#endif
#if !ENABLE_ESP32S3_USB_JTAG_ONLY
        case TUYA_UART1_TX:
            *func = TKL_PINMUX_FUNC_UART1_TX;
            break;
        case TUYA_UART1_RX:
            *func = TKL_PINMUX_FUNC_UART1_RX;
            break;
#endif
        case TUYA_PWM0:
            *func = TKL_PINMUX_FUNC_PWM0;
            break;
        case TUYA_PWM1:
            *func = TKL_PINMUX_FUNC_PWM1;
            break;
        case TUYA_PWM2:
            *func = TKL_PINMUX_FUNC_PWM2;
            break;
        case TUYA_PWM3:
            *func = TKL_PINMUX_FUNC_PWM3;
            break;
        case TUYA_PWM4:
            *func = TKL_PINMUX_FUNC_PWM4;
            break;
        case TUYA_PWM5:
            *func = TKL_PINMUX_FUNC_PWM5;
            break;
        case TUYA_SPI0_MISO:
            *func = TKL_PINMUX_FUNC_SPI0_MISO;
            break;
        case TUYA_SPI0_MOSI:
            *func = TKL_PINMUX_FUNC_SPI0_MOSI;
            break;
        case TUYA_SPI0_CLK:
            *func = TKL_PINMUX_FUNC_SPI0_CLK;
            break;
        case TUYA_SPI0_CS:
            *func = TKL_PINMUX_FUNC_SPI0_CS;
            break;
#if TKL_PINMUX_SPI_PORT_COUNT > 1
        case TUYA_SPI1_MISO:
            *func = TKL_PINMUX_FUNC_SPI1_MISO;
            break;
        case TUYA_SPI1_MOSI:
            *func = TKL_PINMUX_FUNC_SPI1_MOSI;
            break;
        case TUYA_SPI1_CLK:
            *func = TKL_PINMUX_FUNC_SPI1_CLK;
            break;
        case TUYA_SPI1_CS:
            *func = TKL_PINMUX_FUNC_SPI1_CS;
            break;
#endif
        case TUYA_GPIO:
            *func = TKL_PINMUX_FUNC_NONE;
            *is_output = false;
            return OPRT_OK;
        default:
            return OPRT_NOT_SUPPORTED;
    }

    *is_output = __function_is_output(*func);
    return OPRT_OK;
}

static OPERATE_RET __validate_pinmux(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func,
                                     TKL_PINMUX_FUNC_E *func, bool *is_output)
{
    if (!__pin_is_valid(pin)) {
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET rt = __function_info(pin_func, func, is_output);
    if (rt != OPRT_OK) {
        return rt;
    }

    if (*is_output && !__pin_is_output_capable(pin)) {
        return OPRT_INVALID_PARM;
    }
    return OPRT_OK;
}

static OPERATE_RET __check_output_conflict(const TUYA_PIN_NAME_E *func_pins,
                                           TKL_PINMUX_FUNC_E func, TUYA_PIN_NAME_E pin,
                                           bool is_output)
{
    if (!is_output || func == TKL_PINMUX_FUNC_NONE) {
        return OPRT_OK;
    }

    for (uint32_t i = 0; i < TKL_PINMUX_FUNC_COUNT; ++i) {
        if ((TKL_PINMUX_FUNC_E)i != func && __function_is_output((TKL_PINMUX_FUNC_E)i) &&
            func_pins[i] == pin) {
            return OPRT_INVALID_PARM;
        }
    }
    return OPRT_OK;
}

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
extern void __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E scl_pin);
extern void __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E sda_pin);
extern void __tkl_pwm_set_pin(TUYA_GPIO_NUM_E pin, TUYA_PWM_NUM_E channel);
extern void __tkl_uart1_set_txd_pin(TUYA_PIN_NAME_E pin);
extern void __tkl_uart1_set_rxd_pin(TUYA_PIN_NAME_E pin);
extern void __tkl_spi_set_mosi_pin(TUYA_SPI_NUM_E port, const TUYA_PIN_NAME_E mosi_pin);
extern void __tkl_spi_set_miso_pin(TUYA_SPI_NUM_E port, const TUYA_PIN_NAME_E miso_pin);
extern void __tkl_spi_set_sclk_pin(TUYA_SPI_NUM_E port, const TUYA_PIN_NAME_E sclk_pin);
extern void __tkl_spi_set_cs_pin(TUYA_SPI_NUM_E port, const TUYA_PIN_NAME_E cs_pin);
static void __apply_pinmux(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func,
                           TKL_PINMUX_FUNC_E func)
{
    switch (pin_func) {
        case TUYA_IIC0_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_0, pin);
            break;
        case TUYA_IIC0_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_0, pin);
            break;
#if SOC_I2C_NUM > 1
        case TUYA_IIC1_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_1, pin);
            break;
        case TUYA_IIC1_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_1, pin);
            break;
#endif
#if SOC_I2C_NUM > 2
        case TUYA_IIC2_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_2, pin);
            break;
        case TUYA_IIC2_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_2, pin);
            break;
#endif
#if !ENABLE_ESP32S3_USB_JTAG_ONLY
        case TUYA_UART1_TX:
            __tkl_uart1_set_txd_pin(pin);
            break;
        case TUYA_UART1_RX:
            __tkl_uart1_set_rxd_pin(pin);
            break;
#endif
        case TUYA_PWM0:
        case TUYA_PWM1:
        case TUYA_PWM2:
        case TUYA_PWM3:
        case TUYA_PWM4:
        case TUYA_PWM5:
            __tkl_pwm_set_pin((TUYA_GPIO_NUM_E)pin,
                              (TUYA_PWM_NUM_E)(pin_func - TUYA_PWM0));
            break;
        case TUYA_SPI0_MISO:
            __tkl_spi_set_miso_pin(TUYA_SPI_NUM_0, pin);
            break;
        case TUYA_SPI0_MOSI:
            __tkl_spi_set_mosi_pin(TUYA_SPI_NUM_0, pin);
            break;
        case TUYA_SPI0_CLK:
            __tkl_spi_set_sclk_pin(TUYA_SPI_NUM_0, pin);
            break;
        case TUYA_SPI0_CS:
            __tkl_spi_set_cs_pin(TUYA_SPI_NUM_0, pin);
            break;
#if TKL_PINMUX_SPI_PORT_COUNT > 1
        case TUYA_SPI1_MISO:
            __tkl_spi_set_miso_pin(TUYA_SPI_NUM_1, pin);
            break;
        case TUYA_SPI1_MOSI:
            __tkl_spi_set_mosi_pin(TUYA_SPI_NUM_1, pin);
            break;
        case TUYA_SPI1_CLK:
            __tkl_spi_set_sclk_pin(TUYA_SPI_NUM_1, pin);
            break;
        case TUYA_SPI1_CS:
            __tkl_spi_set_cs_pin(TUYA_SPI_NUM_1, pin);
            break;
#endif
        case TUYA_GPIO:
        default:
            break;
    }

    if (func != TKL_PINMUX_FUNC_NONE) {
        sg_func_pins[func] = pin;
    }
}

/**
 * @brief Configure one GPIO-matrix route before the owning peripheral is initialized.
 */
OPERATE_RET tkl_io_pinmux_config(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func)
{
    TKL_PINMUX_FUNC_E func;
    bool is_output;
    OPERATE_RET rt = __validate_pinmux(pin, pin_func, &func, &is_output);
    if (rt != OPRT_OK) {
        return rt;
    }

    __pinmux_state_init();
    rt = __check_output_conflict(sg_func_pins, func, pin, is_output);
    if (rt != OPRT_OK) {
        return rt;
    }

    __apply_pinmux(pin, pin_func, func);
    return OPRT_OK;
}

OPERATE_RET tkl_multi_io_pinmux_config(TUYA_MUL_PIN_CFG_T *cfg, uint16_t num)
{
    if (num == 0) {
        return OPRT_OK;
    }
    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    /* Validate every entry before changing any peripheral route. */
    for (uint16_t i = 0; i < num; ++i) {
        TKL_PINMUX_FUNC_E func;
        bool is_output;
        OPERATE_RET rt = __validate_pinmux(cfg[i].pin, cfg[i].pin_func, &func, &is_output);
        if (rt != OPRT_OK) {
            return rt;
        }
    }

    __pinmux_state_init();
    TUYA_PIN_NAME_E staged_pins[TKL_PINMUX_FUNC_COUNT];
    memcpy(staged_pins, sg_func_pins, sizeof(staged_pins));

    /* Last assignment of a function wins; evaluate conflicts in final state. */
    for (uint16_t i = 0; i < num; ++i) {
        TKL_PINMUX_FUNC_E func;
        bool is_output;
        (void)__validate_pinmux(cfg[i].pin, cfg[i].pin_func, &func, &is_output);
        if (func != TKL_PINMUX_FUNC_NONE) {
            staged_pins[func] = cfg[i].pin;
        }
    }
    for (uint32_t i = 0; i < TKL_PINMUX_FUNC_COUNT; ++i) {
        if (staged_pins[i] == TUYA_IO_PIN_MAX) {
            continue;
        }
        OPERATE_RET rt = __check_output_conflict(staged_pins, (TKL_PINMUX_FUNC_E)i,
                                                  staged_pins[i], __function_is_output((TKL_PINMUX_FUNC_E)i));
        if (rt != OPRT_OK) {
            return rt;
        }
    }

    for (uint16_t i = 0; i < num; ++i) {
        TKL_PINMUX_FUNC_E func;
        bool is_output;
        (void)__validate_pinmux(cfg[i].pin, cfg[i].pin_func, &func, &is_output);
        __apply_pinmux(cfg[i].pin, cfg[i].pin_func, func);
    }

    return OPRT_OK;
}
int32_t tkl_io_pin_to_func(uint32_t pin, TUYA_PIN_TYPE_E pin_type)
{
    adc_unit_t unit;
    adc_channel_t channel;

    if (pin_type != TUYA_IO_TYPE_ADC || pin >= SOC_GPIO_PIN_COUNT ||
        (SOC_GPIO_VALID_GPIO_MASK & (1ULL << pin)) == 0) {
        return OPRT_NOT_SUPPORTED;
    }

    if (adc_oneshot_io_to_channel((int)pin, &unit, &channel) != ESP_OK) {
        return OPRT_NOT_SUPPORTED;
    }

    if (unit == ADC_UNIT_1) {
        return (int32_t)((TUYA_ADC_NUM_0 << 8) | (uint8_t)channel);
    }
    if (unit == ADC_UNIT_2) {
        return (int32_t)((TUYA_ADC_NUM_1 << 8) | (uint8_t)channel);
    }

    return OPRT_NOT_SUPPORTED;
}
