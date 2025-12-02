/**
 * @file tkl_i2c.c
 * @brief tkl_i2c module is used to 
 * @version 0.1
 * @date 2025-09-18
 * 
 * @version 0.2
 * @date 2025-12-02
 * ve2opn: Modifyed with Timeout
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
#define I2C_TIMEOUT_MS 1000  // Default timeout in milliseconds

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    TUYA_GPIO_NUM_E scl;
    TUYA_GPIO_NUM_E sda;
} SR_I2C_GPIO_T;
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

static i2c_master_bus_handle_t i2c_bus[TUYA_I2C_NUM_MAX] = {NULL};
static i2c_device_config_t i2c_dev_cfg[TUYA_I2C_NUM_MAX] = {0};
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

    if (port >= TUYA_I2C_NUM_MAX || cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (i2c_bus[port] == NULL) {    // i2c bus not init
        memset(&i2c_dev_cfg[port], 0, sizeof(i2c_device_config_t));
    }

    switch (cfg->speed) {
        case TUYA_IIC_BUS_SPEED_100K:
            i2c_dev_cfg[port].scl_speed_hz = 100000;
            break;
        case TUYA_IIC_BUS_SPEED_400K:
            i2c_dev_cfg[port].scl_speed_hz = 400000;
            break;
        case TUYA_IIC_BUS_SPEED_1M:
            i2c_dev_cfg[port].scl_speed_hz = 1000000;
            break;
        case TUYA_IIC_BUS_SPEED_3_4M:
            i2c_dev_cfg[port].scl_speed_hz = 3400000;
            break;
        default:
            i2c_dev_cfg[port].scl_speed_hz = 100000;
            break;
    }
    if (cfg->addr_width == TUYA_IIC_ADDRESS_7BIT) {
        i2c_dev_cfg[port].dev_addr_length = I2C_ADDR_BIT_LEN_7;
    } else {
        ESP_LOGE(TAG, "Not support 10bit address mode");
        return OPRT_NOT_SUPPORTED;
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
    i2c_bus[port] = NULL;

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
    i2c_master_dev_handle_t dev_handle;
    esp_err_t esp_rt;

    if (port >= TUYA_I2C_NUM_MAX || i2c_bus[port] == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (0 == size) {
        esp_rt = i2c_master_probe(i2c_bus[port], dev_addr, I2C_TIMEOUT_MS);
        if (esp_rt == ESP_OK) {
            return OPRT_OK;
        } else {
            ESP_LOGE(TAG, "I2C probe failed: %s", esp_err_to_name(esp_rt));
            return OPRT_COM_ERROR;
        }
    }

    i2c_dev_cfg[port].device_address = dev_addr;
    esp_rt = i2c_master_bus_add_device(i2c_bus[port], &i2c_dev_cfg[port], &dev_handle);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(esp_rt));
        return OPRT_COM_ERROR;
    }

    esp_rt = i2c_master_transmit(dev_handle, data, size, I2C_TIMEOUT_MS);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "I2C transmit failed: %s", esp_err_to_name(esp_rt));
        i2c_master_bus_rm_device(dev_handle);
        return OPRT_COM_ERROR;
    }

    esp_rt = i2c_master_bus_rm_device(dev_handle);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove I2C device: %s", esp_err_to_name(esp_rt));
        return OPRT_COM_ERROR;
    }
    
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
    i2c_master_dev_handle_t dev_handle;
    esp_err_t esp_rt;

    if (port >= TUYA_I2C_NUM_MAX || i2c_bus[port] == NULL) {
        return OPRT_INVALID_PARM;
    }

    i2c_dev_cfg[port].device_address = dev_addr;
    esp_rt = i2c_master_bus_add_device(i2c_bus[port], &i2c_dev_cfg[port], &dev_handle);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(esp_rt));
        return OPRT_COM_ERROR;
    }

    esp_rt = i2c_master_receive(dev_handle, data, size, I2C_TIMEOUT_MS);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "I2C receive failed: %s", esp_err_to_name(esp_rt));
        i2c_master_bus_rm_device(dev_handle);
        return OPRT_COM_ERROR;
    }

    esp_rt = i2c_master_bus_rm_device(dev_handle);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove I2C device: %s", esp_err_to_name(esp_rt));
        return OPRT_COM_ERROR;
    }
    
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
