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

#include "tkl_pinmux.h"
// #include "esp_err.h"
// #include "esp_log.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/
typedef struct{
    TUYA_PIN_NAME_E pin;
    TUYA_PIN_FUNC_E func;
    
}TUYA_PIN_FUNC_MAP_T;
/****************************************************************************
 * Private Data Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/
extern void __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E scl_pin);
extern void __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E sda_pin);
extern void __tkl_pwm_set_pin(TUYA_GPIO_NUM_E pin, TUYA_PWM_NUM_E channel);
/**
 * @brief tuya io pinmux func
 *
 * @param[in] pin: pin number
 * @param[in] pin_func: pin function
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_io_pinmux_config(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func)
{
    switch (pin_func) {
        case TUYA_IIC0_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_0, pin);
            break;
        case TUYA_IIC0_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_0, pin);
            break;
        case TUYA_IIC1_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_1, pin);
            break;
        case TUYA_IIC1_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_1, pin);
            break;
        case TUYA_IIC2_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_2, pin);
            break;
        case TUYA_IIC2_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_2, pin);
            break;
#if 0
        case TUYA_IIC3_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_3, pin);
            break;
        case TUYA_IIC3_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_3, pin);
            break;
        case TUYA_IIC4_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_4, pin);
            break;
        case TUYA_IIC4_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_4, pin);
            break;
        case TUYA_IIC5_SCL:
            __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_5, pin);
            break;
        case TUYA_IIC5_SDA:
            __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_5, pin);
            break;
#endif
        case TUYA_PWM0 :
            __tkl_pwm_set_pin(pin, TUYA_PWM_NUM_0);
            break;
        case TUYA_PWM1 :
            __tkl_pwm_set_pin(pin, TUYA_PWM_NUM_1);
            break;
        case TUYA_PWM2 :
            __tkl_pwm_set_pin(pin, TUYA_PWM_NUM_2);
            break;
        case TUYA_PWM3 :
            __tkl_pwm_set_pin(pin, TUYA_PWM_NUM_3);
            break;
        case TUYA_PWM4 :
            __tkl_pwm_set_pin(pin, TUYA_PWM_NUM_4);
            break;
        case TUYA_PWM5 :
            __tkl_pwm_set_pin(pin, TUYA_PWM_NUM_5);
            break;

        case TUYA_SPI0_MISO:
        case TUYA_SPI0_MOSI:
        case TUYA_SPI0_CLK: 
        case TUYA_SPI0_CS:
            break;

        default:
            break;

    }
    return OPRT_OK;
}
int32_t tkl_io_pin_to_func(uint32_t pin, TUYA_PIN_TYPE_E pin_type)
{
	int32_t port_channel = OPRT_NOT_SUPPORTED;

    // switch (pin_type) {
    //     case TUYA_IO_TYPE_PWM:                  // all pwm channels belong to one port
    //         if (TUYA_IO_PIN_18 == pin) {
    //             port_channel = 0;
    //         } else if (TUYA_IO_PIN_24 == pin) {
    //             port_channel = 1;
    //         } else if (TUYA_IO_PIN_32 == pin) {
    //             port_channel = 2;
    //         } else if (TUYA_IO_PIN_34 == pin) {
    //             port_channel = 3;
    //         } else if (TUYA_IO_PIN_36 == pin) {
    //             port_channel = 4;
    //         } else if (TUYA_IO_PIN_19 == pin) {
    //             port_channel = 5;
    //         } else if (TUYA_IO_PIN_8 == pin) {
    //             port_channel = 6;
    //         } else if (TUYA_IO_PIN_9 == pin) {
    //             port_channel = 7;
    //         } else if (TUYA_IO_PIN_25 == pin) {
    //             port_channel = 8;
    //         } else if (TUYA_IO_PIN_33 == pin) {
    //             port_channel = 9;
    //         } else if (TUYA_IO_PIN_35 == pin) {
    //             port_channel = 10;
    //         } else if (TUYA_IO_PIN_37 == pin) {
    //             port_channel = 11;
    //         }
    //         break;
    //     case TUYA_IO_TYPE_ADC:
    //         if (TUYA_IO_PIN_25 == pin) {
    //             port_channel = ADC_1;
    //         } else if (TUYA_IO_PIN_24 == pin) {
    //             port_channel = ADC_2;
    //         } else if (TUYA_IO_PIN_23 == pin) {
    //             port_channel = ADC_3;
    //         } else if (TUYA_IO_PIN_28 == pin) {
    //             port_channel = ADC_4;
    //         } else if (TUYA_IO_PIN_22 == pin) {
    //             port_channel = ADC_5;
    //         } else if (TUYA_IO_PIN_21 == pin) {
    //             port_channel = ADC_6;
    //         } else if (TUYA_IO_PIN_8 == pin) {
    //             port_channel = ADC_10;
    //         } else if (TUYA_IO_PIN_13 == pin) {
    //             port_channel = ADC_15;
    //         } else if (TUYA_IO_PIN_12 == pin) {
    //             port_channel = ADC_14;
    //         } else if (TUYA_IO_PIN_1 == pin) {
    //             port_channel = ADC_13;
    //         } else if (TUYA_IO_PIN_0 == pin) {
    //             port_channel = ADC_12;
    //         }
    //         break;
    //     case TUYA_IO_TYPE_DAC:
    //         break;
    //     case TUYA_IO_TYPE_UART:
    //         break;
    //     case TUYA_IO_TYPE_SPI:
    //         break;
    //     case TUYA_IO_TYPE_I2C:
    //         break;
    //     case TUYA_IO_TYPE_I2S:
    //         break;
    //     case TUYA_IO_TYPE_GPIO:
    //         break;
    //     default:
    //         break;
    // }

    return port_channel;
}
