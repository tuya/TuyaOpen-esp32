/**
 * @file tkl_i2c.c
 * @brief tkl_i2c module is used to 
 * @version 0.1
 * @date 2025-09-18
 */
#include "tkl_i2c.h"
#include "tkl_pinmux.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/i2c_types.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TAG "tkl_i2c"
#define I2C_DEVICE_MAX_NUM (5)
/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    TUYA_GPIO_NUM_E scl;
    TUYA_GPIO_NUM_E sda;
} SR_I2C_GPIO_T;

typedef struct {    
    i2c_device_config_t i2c_dev_cfg;
    i2c_master_dev_handle_t i2c_dev_handle;
} SG_I2C_DEVICE_MAP_T;
/***********************************************************
********************function declaration********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/
static SR_I2C_GPIO_T sg_i2c_pin[TUYA_I2C_NUM_MAX] = {
    {TUYA_GPIO_NUM_0, TUYA_GPIO_NUM_1},
    {TUYA_GPIO_NUM_2, TUYA_GPIO_NUM_3},
    {TUYA_GPIO_NUM_MAX, TUYA_GPIO_NUM_MAX},
    {TUYA_GPIO_NUM_MAX, TUYA_GPIO_NUM_MAX},
    {TUYA_GPIO_NUM_MAX, TUYA_GPIO_NUM_MAX},
    {TUYA_GPIO_NUM_MAX, TUYA_GPIO_NUM_MAX}
};
static SG_I2C_DEVICE_MAP_T sg_i2c_dev_map[TUYA_I2C_NUM_MAX][I2C_DEVICE_MAX_NUM] = {0};
static i2c_master_bus_handle_t i2c_bus[TUYA_GPIO_NUM_MAX] = {NULL};
/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief set i2c scl pin
 * NOTE: call this API in tkl_io_pinmux_config
 * * *
 * @param[in] port: i2c port
 * @param[in] i2c_pin: i2c scl pin number
 *
 *
 * @return void
 * */
void __tkl_i2c_set_scl_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E scl_pin)
{
    if (port >= TUYA_I2C_NUM_MAX) {
        return;
    }

    sg_i2c_pin[port].scl = scl_pin;
    return;
}

/**
 * @brief set i2c sda pin
 * NOTE: call this API in tkl_io_pinmux_config
 * * *
 * @param[in] port: i2c port
 * @param[in] i2c_pin: i2c scl pin number
 *
 *
 * @return void
 * */
void __tkl_i2c_set_sda_pin(TUYA_I2C_NUM_E port, const TUYA_PIN_NAME_E sda_pin)
{
    if (port >= TUYA_I2C_NUM_MAX) {
        return;
    }

    sg_i2c_pin[port].sda = sda_pin;
    return;
}
/**
 * @brief i2c init
 *
 * @param[in] i2c_pin: i2c pin number
 * @param[out] port: i2c pin number
 *
 * @return OPRT_OK on success, others on error
 */
OPERATE_RET tkl_i2c_init(TUYA_I2C_NUM_E port, const TUYA_IIC_BASE_CFG_T *cfg)
{
    esp_err_t esp_rt = ESP_OK;
    uint8_t i = 0;

    if (port >= TUYA_I2C_NUM_MAX || cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (i2c_bus[port] == NULL) {    // i2c bus not init
        for (i = 0; i < I2C_DEVICE_MAX_NUM; i++) {
            i2c_bus[port] = NULL;
            sg_i2c_dev_map[port][i].i2c_dev_handle = NULL;
            memset(&sg_i2c_dev_map[port][i].i2c_dev_cfg, 0, sizeof(i2c_device_config_t));
        }
    }

    for (i = 0; i < I2C_DEVICE_MAX_NUM; i++) {
        if (0 == sg_i2c_dev_map[port][i].i2c_dev_cfg.scl_speed_hz) {
            switch (cfg->speed) {
                case TUYA_IIC_BUS_SPEED_100K:
                    sg_i2c_dev_map[port][i].i2c_dev_cfg.scl_speed_hz = 100000;
                    break;
                case TUYA_IIC_BUS_SPEED_400K:
                    sg_i2c_dev_map[port][i].i2c_dev_cfg.scl_speed_hz = 400000;
                    break;
                case TUYA_IIC_BUS_SPEED_1M:
                    sg_i2c_dev_map[port][i].i2c_dev_cfg.scl_speed_hz = 1000000;
                    break;
                case TUYA_IIC_BUS_SPEED_3_4M:
                    sg_i2c_dev_map[port][i].i2c_dev_cfg.scl_speed_hz = 3400000;
                    break;
                default:
                    sg_i2c_dev_map[port][i].i2c_dev_cfg.scl_speed_hz = 100000;
                    break;
            }
            if (cfg->addr_width == TUYA_IIC_ADDRESS_7BIT) {
                sg_i2c_dev_map[port][i].i2c_dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
            } else {
                sg_i2c_dev_map[port][i].i2c_dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_10;
            }
            break;
        }
    }
    // initialize i2c bus
    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = port,
        .sda_io_num = sg_i2c_pin[port].sda,
        .scl_io_num = sg_i2c_pin[port].scl,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags =
            {
                .enable_internal_pullup = 1,
            },
    };
    esp_rt = i2c_new_master_bus(&i2c_bus_cfg, &i2c_bus[port]);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(esp_rt));
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}
/**
 * @brief i2c deinit
 *
 * @param[in] i2c_pin: i2c pin number
 *
 * @return OPRT_OK on success, others on error
 */
OPERATE_RET tkl_i2c_deinit(TUYA_I2C_NUM_E port)
{
    if (port >= TUYA_I2C_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }
    
    i2c_del_master_bus(i2c_bus[port]);
    for (uint8_t i = 0; i < I2C_DEVICE_MAX_NUM; i++) {
        if (sg_i2c_dev_map[port][i].i2c_dev_handle == NULL) {
            continue;
        }
        ESP_ERROR_CHECK(i2c_master_bus_rm_device(sg_i2c_dev_map[port][i].i2c_dev_handle));
        sg_i2c_dev_map[port][i].i2c_dev_handle = NULL;
    }
    return OPRT_OK;
}

/**
 * @brief i2c irq init
 * NOTE: call this API will not enable interrupt
 *
 * @param[in] port: i2c port, id index starts at 0
 * @param[in] cb:  i2c irq cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_irq_init(TUYA_I2C_NUM_E port, TUYA_I2C_IRQ_CB cb)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c irq enable
 *
 * @param[in] port: i2c port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_irq_enable(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c irq disable
 *
 * @param[in] port: i2c port id, id index starts at 0
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_irq_disable(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c master send
 *
 * @param[in] port: i2c port
 * @param[in] dev_addr: iic addrress of slave device.
 * @param[in] data: i2c data to send
 * @param[in] size: Number of data items to send
 * @param[in] xfer_pending: xfer_pending: TRUE : not send stop condition, FALSE : send stop condition.
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_master_send(TUYA_I2C_NUM_E port, uint16_t dev_addr, const void *data, uint32_t size, BOOL_T xfer_pending)
{
    uint8_t i = 0;
    i2c_master_dev_handle_t dev_handle;

    if (port >= I2C_DEVICE_MAX_NUM || i2c_bus[port] == NULL) {
        return OPRT_INVALID_PARM;
    }

    for (i = 0; i < I2C_DEVICE_MAX_NUM; i++) {
        if (sg_i2c_dev_map[port][i].i2c_dev_handle == NULL) {   // Add an new device
            sg_i2c_dev_map[port][i].i2c_dev_cfg.device_address = dev_addr;                              
            ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus[port], &(sg_i2c_dev_map[port][i].i2c_dev_cfg), &dev_handle));
            sg_i2c_dev_map[port][i].i2c_dev_handle = dev_handle;
            break;
        } else {
            if (sg_i2c_dev_map[port][i].i2c_dev_cfg.device_address == dev_addr) {
                break;
            }
        }
    }

    ESP_ERROR_CHECK(i2c_master_transmit(sg_i2c_dev_map[port][i].i2c_dev_handle, data, size, -1));
    return OPRT_OK;
}

/**
 * @brief i2c master recv
 *
 * @param[in] port: i2c port
 * @param[in] dev_addr: iic addrress of slave device.
 * @param[in] data: i2c buf to recv
 * @param[in] size: Number of data items to receive
 * @param[in] xfer_pending: TRUE : not send stop condition, FALSE : send stop condition.
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_master_receive(TUYA_I2C_NUM_E port, uint16_t dev_addr, void *data, uint32_t size,
                                   BOOL_T xfer_pending)
{
    uint8_t i = 0;
    i2c_master_dev_handle_t dev_handle;

    if (port >= I2C_DEVICE_MAX_NUM || i2c_bus[port] == NULL) {
        return OPRT_INVALID_PARM;
    }

    for (i = 0; i < I2C_DEVICE_MAX_NUM; i++) {
        if (sg_i2c_dev_map[port][i].i2c_dev_handle == NULL) {   // Add an new device
            sg_i2c_dev_map[port][i].i2c_dev_cfg.device_address = dev_addr;                              
            ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus[port], &(sg_i2c_dev_map[port][i].i2c_dev_cfg), &dev_handle));
            sg_i2c_dev_map[port][i].i2c_dev_handle = dev_handle;
            break;
        } else {
            if (sg_i2c_dev_map[port][i].i2c_dev_cfg.device_address == dev_addr) {
                break;
            }
        }
    }

    ESP_ERROR_CHECK(i2c_master_receive(sg_i2c_dev_map[port][i].i2c_dev_handle, data, size, -1));
    return OPRT_OK;
}
/**
 * @brief i2c slave
 *
 * @param[in] port: i2c port
 * @param[in] dev_addr: slave device addr
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_set_slave_addr(TUYA_I2C_NUM_E port, uint16_t dev_addr)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c slave send
 *
 * @param[in] port: i2c port
 * @param[in] data: i2c buf to send
 * @param[in] size: Number of data items to send
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2c_slave_send(TUYA_I2C_NUM_E port, const void *data, uint32_t size)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief IIC slave receive, Start receiving data as IIC Slave.
 *
 * @param[in] port: i2c port
 * @param[in] data: Pointer to buffer for data to receive from IIC Master
 * @param[in] size: Number of data items to receive
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

OPERATE_RET tkl_i2c_slave_receive(TUYA_I2C_NUM_E port, void *data, uint32_t size)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief IIC get status.
 *
 * @param[in] port: i2c port
 * @param[out]  TUYA_IIC_STATUS_T
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_get_status(TUYA_I2C_NUM_E port, TUYA_IIC_STATUS_T *status)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c's reset
 *
 * @param[in] port: i2c port number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET  tkl_i2c_reset(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c transferred data count.
 *
 * @param[in] port: i2c port id, id index starts at 0
 *
 * @return >=0,number of currently transferred data items. <0,err.
 * tkl_i2c_master_send:number of data bytes transmitted and acknowledged
 * tkl_i2c_master_receive:number of data bytes received
 * tkl_i2c_slave_send:number of data bytes transmitted
 * tkl_i2c_slave_receive:number of data bytes received and acknowledged
 */
int32_t tkl_i2c_get_data_count(TUYA_I2C_NUM_E port)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief i2c ioctl
 *
 * @param[in]       cmd     user def
 * @param[in]       args    args associated with the command
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_i2c_ioctl(TUYA_I2C_NUM_E port, uint32_t cmd,  void *args)
{
    return OPRT_NOT_SUPPORTED;
}
