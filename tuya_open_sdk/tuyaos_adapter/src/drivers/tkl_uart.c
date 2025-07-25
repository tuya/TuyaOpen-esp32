/**
 * @file tkl_uart.c
 * @brief this file was auto-generated by tuyaos v&v tools, developer can add implements between BEGIN and END
 * 
 * @warning: changes between user 'BEGIN' and 'END' will be keeped when run tuyaos v&v tools
 *           changes in other place will be overwrited and lost
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 * 
 */

// --- BEGIN: user defines and implements ---
#include "tkl_uart.h"
#include "tuya_error_code.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/uart.h"
// #include "driver/gpio.h"
#include "soc/gpio_num.h"

#define DBG_TAG "TKL_UART"


#if ENABLE_ESP32S3_USB_JTAG_ONLY

#include "driver/usb_serial_jtag.h"
#define MAX_UART_NUM 1
#define BUF_SIZE (512)

TUYA_UART_IRQ_CB uart_rx_cb[MAX_UART_NUM];
TaskHandle_t tkl_uart_thread = NULL;
    
static uint8_t *read_data = NULL;
static int read_len = 0;
static int read_offset = 0;

static void tkl_uart_rx_process(void *args)
{
    while (1) {
        read_offset = 0;
        read_len = usb_serial_jtag_read_bytes(read_data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
        if (read_len) {
            if (uart_rx_cb[TUYA_UART_NUM_0]) {
                uart_rx_cb[TUYA_UART_NUM_0](TUYA_UART_NUM_0);
            }
        }
    }
}

/**
 * @brief uart init
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] cfg: uart config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_init(TUYA_UART_NUM_E port_id, TUYA_UART_BASE_CFG_T *cfg)
{
    BaseType_t ret;

    // Configure USB SERIAL JTAG
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
        .rx_buffer_size = BUF_SIZE,
        .tx_buffer_size = BUF_SIZE,
    };

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_jtag_config));
    ESP_LOGI("usb_serial_jtag echo", "USB_SERIAL_JTAG init done");

    uart_rx_cb[port_id] = NULL;

    read_data = (uint8_t *) malloc(BUF_SIZE);
    if (read_data == NULL) {
        ESP_LOGE("usb_serial_jtag echo", "no memory for read_data");
        return OPRT_MALLOC_FAILED;
    }

    ret = xTaskCreate(tkl_uart_rx_process, "tkl_uart_thread", 4096, NULL, 4, &tkl_uart_thread);
    if (ret != pdPASS) {
        ESP_LOGI(DBG_TAG, "%s: xTaskCreate failed", __func__);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

/**
 * @brief uart deinit
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_deinit(TUYA_UART_NUM_E port_id)
{
    // --- BEGIN: user implements ---
    if (NULL != tkl_uart_thread) {
        vTaskDelete(tkl_uart_thread);
        tkl_uart_thread = NULL;
    }

    if (read_data != NULL) {
        free(read_data);
        read_data = NULL;
    }

    return OPRT_OK;
    // --- END: user implements --
}

/**
 * @brief uart write data
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] data: write buff
 * @param[in] len:  buff len
 *
 * @return return > 0: number of data written; return <= 0: write errror
 */
int tkl_uart_write(TUYA_UART_NUM_E port_id, void *buff, uint16_t len)
{
    if (TUYA_UART_NUM_0 == port_id) {

        uint16_t write_len = len;
        uint8_t *data = (uint8_t *) buff;

        while (write_len > BUF_SIZE) {
            usb_serial_jtag_write_bytes((const char *) data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
            write_len -= BUF_SIZE;
            data += BUF_SIZE;
        }

        if (write_len > 0) {
            usb_serial_jtag_write_bytes((const char *) data, write_len, 20 / portTICK_PERIOD_MS);
            write_len = 0;
        }
        return len;
    }

    return -1;
}


/**
 * @brief enable uart rx interrupt and regist interrupt callback
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
void tkl_uart_rx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB rx_cb)
{
    // --- BEGIN: user implements ---
    uart_rx_cb[port_id] = rx_cb;

    // --- END: user implements ---
}

/**
 * @brief regist uart tx interrupt callback
 * If this function is called, it indicates that the data is sent asynchronously through interrupt,
 * and then write is invoked to initiate asynchronous transmission.
 *  
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
void tkl_uart_tx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB tx_cb)
{
    // --- BEGIN: user implements ---
    return ;
    // --- END: user implements ---
}


/**
 * @brief uart read data
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[out] data: read data
 * @param[in] len:  buff len
 * 
 * @return return >= 0: number of data read; return < 0: read errror
 */
int tkl_uart_read(TUYA_UART_NUM_E port_id, void *buff, uint16_t len)
{
    // --- BEGIN: user implements ---
    uint16_t data_size = read_len - read_offset;
    if (data_size > 0) {
        if (len > data_size) {
            len = data_size;
        }
        memcpy(buff, read_data + read_offset, len);
        read_offset += len;

        return len;
    } else {
        return 0;
    }
    // --- END: user implements --
}
#else

#define MAX_UART_NUM 2

#if (defined (UART_NUM0_TX_PIN)) && (defined (UART_NUM0_RX_PIN))
#define TKL_UART_NUM_0_TXD  (UART_NUM0_TX_PIN)
#define TKL_UART_NUM_0_RXD  (UART_NUM0_RX_PIN)
#else  // default esp32s3
#define TKL_UART_NUM_0_TXD  (GPIO_NUM_43)
#define TKL_UART_NUM_0_RXD  (GPIO_NUM_44)
#endif

#define TKL_UART_NUM_0_RTS  (UART_PIN_NO_CHANGE)
#define TKL_UART_NUM_0_CTS  (UART_PIN_NO_CHANGE)


#define TKL_UART_NUM_1_TXD  (GPIO_NUM_17)
#define TKL_UART_NUM_1_RXD  (GPIO_NUM_18)
#define TKL_UART_NUM_1_RTS  (UART_PIN_NO_CHANGE)
#define TKL_UART_NUM_1_CTS  (UART_PIN_NO_CHANGE)

TaskHandle_t tkl_uart_thread = NULL;
TUYA_UART_IRQ_CB uart_rx_cb[MAX_UART_NUM];
QueueHandle_t tkl_uart_rx_queue[MAX_UART_NUM] = {NULL};
static QueueSetHandle_t tkl_uart_queue_set = NULL;

// --- END: user defines and implements ---

static void tkl_uart_rx_process(void *args)
{
    uart_event_t event;
    QueueSetMemberHandle_t active_queue;
    // Initialize the UART event queue
    while (tkl_uart_queue_set) {
        active_queue = xQueueSelectFromSet(tkl_uart_queue_set, portMAX_DELAY);
        if (active_queue == NULL) continue;

        for (uart_port_t i = 0; i < MAX_UART_NUM; i++) {
            if (active_queue == (QueueSetMemberHandle_t)tkl_uart_rx_queue[i]) {
                // Drain the queue of all pending events
                while (xQueueReceive(tkl_uart_rx_queue[i], &event, 0) == pdPASS) {
                    switch (event.type) {
                    case UART_DATA:
                        if (uart_rx_cb[i] != NULL) {
                            uart_rx_cb[i](i);
                        }
                        break;
                    case UART_BREAK:
                        break;
                    case UART_BUFFER_FULL:
                        break;
                    case UART_FIFO_OVF:
                        break;
                    case UART_PARITY_ERR:
                        break;
                    case UART_FRAME_ERR:
                        break;
                    case UART_DATA_BREAK:
                        break;
                    case UART_PATTERN_DET:
                        break;
                    default:
                        break;
                    }
                    // Other events could be logged or handled here
                }
                // break; // Move to the next select
            }
        }
    }
    vTaskDelete(NULL);
}

/**
 * @brief uart init
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] cfg: uart config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_init(TUYA_UART_NUM_E port_id, TUYA_UART_BASE_CFG_T *cfg)
{
    // --- BEGIN: user implements ---
    esp_err_t ret;
 	uart_port_t uart_num;
	uart_config_t uart_cfg;
    int intr_alloc_flags = 0, uart_txd, uart_rxd, uart_rts, uart_cts;
    
   if (cfg == NULL)
        return OPRT_INVALID_PARM;
    
    uart_num = (uart_port_t)port_id;
    if (uart_num >= MAX_UART_NUM) {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_UART_DATA_LEN_5BIT == cfg->databits) {
        uart_cfg.data_bits = UART_DATA_5_BITS;
    } else if (TUYA_UART_DATA_LEN_6BIT == cfg->databits) {
        uart_cfg.data_bits = UART_DATA_6_BITS;
    } else if (TUYA_UART_DATA_LEN_7BIT ==  cfg->databits) {
        uart_cfg.data_bits = UART_DATA_7_BITS;
    } else if (TUYA_UART_DATA_LEN_8BIT == cfg->databits) {
        uart_cfg.data_bits = UART_DATA_8_BITS;
    } else {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_UART_STOP_LEN_1BIT == cfg->stopbits) {
        uart_cfg.stop_bits = UART_STOP_BITS_1;
    } else if (TUYA_UART_STOP_LEN_2BIT == cfg->stopbits) {
        uart_cfg.stop_bits = UART_STOP_BITS_2;
    } else if (TUYA_UART_STOP_LEN_1_5BIT1 == cfg->stopbits) {
        uart_cfg.stop_bits = UART_STOP_BITS_1_5;
    } else {
        return OPRT_INVALID_PARM;
    }

    if (TUYA_UART_PARITY_TYPE_NONE == cfg->parity) {
        uart_cfg.parity = UART_PARITY_DISABLE;
    } else if (TUYA_UART_PARITY_TYPE_EVEN == cfg->parity) {
        uart_cfg.parity = UART_PARITY_EVEN;
    } else if (TUYA_UART_PARITY_TYPE_ODD == cfg->parity) {
        uart_cfg.parity = UART_PARITY_ODD;
    } else {
        return OPRT_INVALID_PARM;
    }

    uart_cfg.baud_rate = cfg->baudrate;
    uart_cfg.rx_flow_ctrl_thresh = 122;
	uart_cfg.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
	uart_cfg.source_clk = UART_SCLK_DEFAULT;
    // uart_cfg.source_clk = UART_SCLK_XTAL;
#if !SOC_UART_SUPPORT_SLEEP_RETENTION
    uart_cfg.flags.allow_pd = 0;
    uart_cfg.flags.backup_before_sleep = 0;
#endif

    if (UART_NUM_1 == uart_num) {
        uart_txd = TKL_UART_NUM_1_TXD;
        uart_rxd = TKL_UART_NUM_1_RXD;
        uart_cts = TKL_UART_NUM_1_CTS;
        uart_rts = TKL_UART_NUM_1_RTS;
    } else {
        uart_txd = TKL_UART_NUM_0_TXD;
        uart_rxd = TKL_UART_NUM_0_RXD;
        uart_cts = TKL_UART_NUM_0_CTS;
        uart_rts = TKL_UART_NUM_0_RTS;
    }

    ret = uart_param_config(uart_num, &uart_cfg);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call uart_param_config failed(uart_num=%d,ret=%d)", __func__, uart_num, ret);
        return OPRT_COM_ERROR;
    }

    ret = uart_set_pin(uart_num, uart_txd, uart_rxd, uart_rts, uart_cts);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call uart_set_pin failed(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }
    
    ret = uart_driver_install(uart_num, 256, 0, 10, &tkl_uart_rx_queue[uart_num], intr_alloc_flags);
    if (ESP_OK != ret) {
        ESP_LOGE(DBG_TAG, "%s: call uart_driver_install failecd(ret=%d)", __func__, ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief uart deinit
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_deinit(TUYA_UART_NUM_E port_id)
{
    // --- BEGIN: user implements ---
    uart_port_t uart_num = (uart_port_t)port_id;
    if (uart_num > MAX_UART_NUM)
        return OPRT_INVALID_PARM;
    
    if (ESP_OK != uart_disable_rx_intr(uart_num)) {
        ESP_LOGE(DBG_TAG, "%s: call uart_disable_rx_intr failed", __func__);
        //return OPRT_COM_ERROR;
    }

    if (ESP_OK != uart_driver_delete(uart_num)) {
        ESP_LOGE(DBG_TAG, "%s: call uart_driver_delete failed", __func__);
        return OPRT_COM_ERROR;
    }

    if (NULL != tkl_uart_thread) {
        vTaskDelete(tkl_uart_thread);
        tkl_uart_thread = NULL;
    }
    tkl_uart_rx_queue[uart_num] = NULL;
    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief uart write data
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] data: write buff
 * @param[in] len:  buff len
 *
 * @return return > 0: number of data written; return <= 0: write errror
 */
int tkl_uart_write(TUYA_UART_NUM_E port_id, void *buff, uint16_t len)
{
    // --- BEGIN: user implements ---
    int ret;
    uart_port_t  uart_num = (uart_port_t)port_id; 
    if (NULL == buff || 0 == len || uart_num > MAX_UART_NUM) {
        return OPRT_INVALID_PARM;
    }

    // Write data back to the UART
    ret = uart_write_bytes(uart_num, (const char *)buff, len);
    if (ret < 0) {
        ESP_LOGI(DBG_TAG, "%s: uart_write_bytes failed(ret=%d)", __func__, ret);
    }
    return ret;
    // --- END: user implements ---
}

/**
 * @brief enable uart rx interrupt and regist interrupt callback
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
void tkl_uart_rx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB rx_cb)
{
    // --- BEGIN: user implements ---
    uart_port_t uart_num = (uart_port_t)port_id;

    if (NULL == rx_cb || uart_num >= MAX_UART_NUM) {
        ESP_LOGI(DBG_TAG, "%s: rx_cb is NULL or uart_num is invalid", __func__);
        return;
    }

    uart_rx_cb[uart_num] = rx_cb;

    if (tkl_uart_queue_set == NULL) {
        tkl_uart_queue_set = xQueueCreateSet(MAX_UART_NUM * 2); // Create a queue set that can hold all queues
        if (tkl_uart_queue_set == NULL) {
            ESP_LOGE(DBG_TAG, "Failed to create queue set");
            return;
        }
    }

    if (tkl_uart_rx_queue[uart_num] != NULL) {
        xQueueAddToSet(tkl_uart_rx_queue[uart_num], tkl_uart_queue_set);
    }

    if (tkl_uart_thread == NULL) {
        BaseType_t ret = xTaskCreate(tkl_uart_rx_process, "tkl_uart_thread", 4096, NULL, 4, &tkl_uart_thread);
        if (ret != pdPASS) {
            ESP_LOGE(DBG_TAG, "%s: xTaskCreate failed", __func__);
            vQueueDelete(tkl_uart_queue_set);
            tkl_uart_queue_set = NULL;
        }
    }

    uart_enable_rx_intr(uart_num);
}

/**
 * @brief regist uart tx interrupt callback
 * If this function is called, it indicates that the data is sent asynchronously through interrupt,
 * and then write is invoked to initiate asynchronous transmission.
 *  
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
void tkl_uart_tx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB tx_cb)
{
    // --- BEGIN: user implements ---
    return ;
    // --- END: user implements ---
}

/**
 * @brief uart read data
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[out] data: read data
 * @param[in] len:  buff len
 * 
 * @return return >= 0: number of data read; return < 0: read errror
 */
int tkl_uart_read(TUYA_UART_NUM_E port_id, void *buff, uint16_t len)
{
    // --- BEGIN: user implements ---
    int ret;
    uart_port_t uart_num = (uart_port_t)port_id;
    if (NULL == buff || 0 == len || uart_num > MAX_UART_NUM) {
        return OPRT_INVALID_PARM;
    }

    // Read data from the UART
    ret = uart_read_bytes(uart_num, buff, len, 20 / portTICK_PERIOD_MS);
    if (ret < 0) {
        ESP_LOGI(DBG_TAG, "%s: uart_read_bytes failed(ret=%d)", __func__, ret);
    }
    return ret;
    // --- END: user implements ---
}

/**
 * @brief set uart transmit interrupt status
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] enable: TRUE-enalbe tx int, FALSE-disable tx int
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_set_tx_int(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    // --- BEGIN: user implements ---
    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief set uart receive flowcontrol
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] enable: TRUE-enalbe rx flowcontrol, FALSE-disable rx flowcontrol
 * 
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_set_rx_flowctrl(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    // --- BEGIN: user implements ---
    return OPRT_OK;
    // --- END: user implements ---
}

/**
 * @brief wait for uart data
 * 
 * @param[in] port_id: uart port id, id index starts at 0
 *                     in linux platform, 
 *                         high 16 bits aslo means uart type, 
 *                                   it's value must be one of the TUYA_UART_TYPE_E type
 *                         the low 16bit - means uart port id
 *                         you can input like this TUYA_UART_PORT_ID(TUYA_UART_SYS, 2)
 * @param[in] timeout_ms: the max wait time, unit is millisecond
 *                        -1 : block indefinitely
 *                        0  : non-block
 *                        >0 : timeout in milliseconds
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_wait_for_data(TUYA_UART_NUM_E port_id, int timeout_ms)
{
    // --- BEGIN: user implements ---
    return OPRT_NOT_SUPPORTED;
    // --- END: user implements ---
}

/**
 * @brief uart control
 *
 * @param[in] uart refer to tuya_uart_t
 * @param[in] cmd control command
 * @param[in] arg command argument
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_ioctl(TUYA_UART_NUM_E port_id, uint32_t cmd, void *arg)
{
    // --- BEGIN: user implements ---
    return OPRT_NOT_SUPPORTED;
    // --- END: user implements ---
}


#endif
