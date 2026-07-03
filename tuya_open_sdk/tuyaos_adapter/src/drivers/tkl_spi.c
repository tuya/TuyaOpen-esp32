/**
 * @file tkl_spi.c
 * @brief ESP32 TKL SPI implementation based on ESP-IDF spi_master.
 *
 * If the user configures SPI pins via tkl_io_pinmux_config(), those values are
 * used.  Otherwise the driver falls back to chip-dependent defaults for each
 * TUYA_SPI_NUM_E port (MOSI / MISO / SCLK / CS).
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc.
 */

#include "tkl_spi.h"

#include "esp_err.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "hal/spi_types.h"

#include "esp_heap_caps.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TAG "tkl_spi"

/* SPIs available on the current chip (based on dev2.md IOMUX table) */
#if defined(CONFIG_IDF_TARGET_ESP32)
  #define TKL_SPI_PORT_COUNT  2        /* HSPI=SPI2_HOST, VSPI=SPI3_HOST */
  #define TKL_SPI_HOST0       SPI2_HOST
  #define TKL_SPI_HOST1       SPI3_HOST
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
  #define TKL_SPI_PORT_COUNT  2        /* FSPI=SPI2_HOST(IOMUX), SPI3=GPIO matrix */
  #define TKL_SPI_HOST0       SPI2_HOST
  #define TKL_SPI_HOST1       SPI3_HOST
#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
  #define TKL_SPI_PORT_COUNT  1        /* only SPI2_HOST available */
  #define TKL_SPI_HOST0       SPI2_HOST
#elif defined(CONFIG_IDF_TARGET_ESP32P4)
  #define TKL_SPI_PORT_COUNT  2        /* SPI2_HOST + SPI3_HOST */
  #define TKL_SPI_HOST0       SPI2_HOST
  #define TKL_SPI_HOST1       SPI3_HOST
#else
  #define TKL_SPI_PORT_COUNT  1
#endif

/* Conservative chunk size used by upper layers (e.g. display driver). */
#ifndef TKL_SPI_MAX_DMA_LEN
#define TKL_SPI_MAX_DMA_LEN (4092U)
#endif

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    bool                 inited;
    TUYA_SPI_BASE_CFG_T  cfg;
    spi_host_device_t    host;
    spi_device_handle_t  dev;

    /* Pin mapping configured via tkl_io_pinmux_config(). */
    int mosi_io;
    int miso_io;
    int sclk_io;
    int cs_io;

    /* Status / counters */
    TUYA_SPI_STATUS_T    status;
    int32_t              last_count;

    /* Optional IRQ style callback */
    TUYA_SPI_IRQ_CB      irq_cb;
    bool                 irq_enabled;
} TKL_ESP_SPI_CTX_T;

/***********************************************************
***********************variable define**********************
***********************************************************/

static TKL_ESP_SPI_CTX_T sg_spi[TUYA_SPI_NUM_MAX] = {
#if TKL_SPI_PORT_COUNT >= 1
    /* IOMUX default pins — overridable via tkl_io_pinmux_config() */
#if defined(CONFIG_IDF_TARGET_ESP32C3)
    [TUYA_SPI_NUM_0] = { .mosi_io = TUYA_IO_PIN_7,  .miso_io = TUYA_IO_PIN_2,  .sclk_io = TUYA_IO_PIN_6,  .cs_io = TUYA_IO_PIN_10 },
#elif defined(CONFIG_IDF_TARGET_ESP32C6)
    [TUYA_SPI_NUM_0] = { .mosi_io = TUYA_IO_PIN_7,  .miso_io = TUYA_IO_PIN_2,  .sclk_io = TUYA_IO_PIN_6,  .cs_io = TUYA_IO_PIN_16 },
#elif defined(CONFIG_IDF_TARGET_ESP32P4)
    [TUYA_SPI_NUM_0] = { .mosi_io = TUYA_IO_PIN_10, .miso_io = TUYA_IO_PIN_11, .sclk_io = TUYA_IO_PIN_9,  .cs_io = TUYA_IO_PIN_7  },
#else /* ESP32 / ESP32-S3 (both share FSPI IOMUX at GPIO 10-13) */
    [TUYA_SPI_NUM_0] = { .mosi_io = TUYA_IO_PIN_11, .miso_io = TUYA_IO_PIN_13, .sclk_io = TUYA_IO_PIN_12, .cs_io = TUYA_IO_PIN_10 },
#endif
#endif /* TKL_SPI_PORT_COUNT >= 1 */

#if TKL_SPI_PORT_COUNT >= 2
#if defined(CONFIG_IDF_TARGET_ESP32)
    [TUYA_SPI_NUM_1] = { .mosi_io = TUYA_IO_PIN_23, .miso_io = TUYA_IO_PIN_19, .sclk_io = TUYA_IO_PIN_18, .cs_io = TUYA_IO_PIN_5  },
#elif defined(CONFIG_IDF_TARGET_ESP32P4)
    [TUYA_SPI_NUM_1] = { .mosi_io = TUYA_IO_PIN_46, .miso_io = TUYA_IO_PIN_27, .sclk_io = TUYA_IO_PIN_53, .cs_io = TUYA_IO_PIN_47 },
#else /* ESP32-S3: SPI3 has no IOMUX — must be set by board */
    [TUYA_SPI_NUM_1] = { .mosi_io = TUYA_IO_PIN_23, .miso_io = TUYA_IO_PIN_19, .sclk_io = TUYA_IO_PIN_18, .cs_io = TUYA_IO_PIN_5  },
#endif
#endif /* TKL_SPI_PORT_COUNT >= 2 */
};

/***********************************************************
***********************function declaration********************
***********************************************************/
static spi_host_device_t __port_to_host(TUYA_SPI_NUM_E port);
static OPERATE_RET __ensure_bus_and_device(TUYA_SPI_NUM_E port);
static void *__dma_dup_if_needed(const void *buf, uint32_t len, bool for_tx);
static void __dma_free_if_dup(void *ptr, const void *orig);
static OPERATE_RET __do_xfer(TUYA_SPI_NUM_E port, const void *tx, uint32_t tx_len, void *rx, uint32_t rx_len);
static void __notify_irq(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_EVT_E evt);

/***********************************************************
***********************function define**********************
***********************************************************/
void __tkl_spi_set_mosi_pin(TUYA_SPI_NUM_E port, const TUYA_PIN_NAME_E mosi_pin)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return;
    }
    sg_spi[port].mosi_io = (int)mosi_pin;
}

void __tkl_spi_set_miso_pin(TUYA_SPI_NUM_E port, const TUYA_PIN_NAME_E miso_pin)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return;
    }
    sg_spi[port].miso_io = (int)miso_pin;
}

void __tkl_spi_set_sclk_pin(TUYA_SPI_NUM_E port, const TUYA_PIN_NAME_E sclk_pin)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return;
    }
    sg_spi[port].sclk_io = (int)sclk_pin;
}

void __tkl_spi_set_cs_pin(TUYA_SPI_NUM_E port, const TUYA_PIN_NAME_E cs_pin)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return;
    }
    sg_spi[port].cs_io = (int)cs_pin;
}

/* ── Internal helpers used by tkl_fs.c ──────────────── */

int __tkl_spi_port_to_host(TUYA_SPI_NUM_E port)
{
    return (int)__port_to_host(port);
}

void __tkl_spi_get_pins(TUYA_SPI_NUM_E port, int *mosi, int *miso, int *sclk, int *cs)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        *mosi = *miso = *sclk = *cs = -1;
        return;
    }
    TKL_ESP_SPI_CTX_T *ctx = &sg_spi[port];
    *mosi = ctx->mosi_io;
    *miso = ctx->miso_io;
    *sclk = ctx->sclk_io;
    *cs   = ctx->cs_io;
}

static spi_host_device_t __port_to_host(TUYA_SPI_NUM_E port)
{
#if TKL_SPI_PORT_COUNT == 0
    return (spi_host_device_t)(-1);
#else
    switch (port) {
        case TUYA_SPI_NUM_0: return TKL_SPI_HOST0;
#if TKL_SPI_PORT_COUNT >= 2
        case TUYA_SPI_NUM_1: return TKL_SPI_HOST1;
#endif
        default:             return (spi_host_device_t)(-1);
    }
#endif
}

static uint32_t __mode_to_idf(TUYA_SPI_MODE_E mode)
{
    switch (mode) {
        case TUYA_SPI_MODE0:
            return 0;
        case TUYA_SPI_MODE1:
            return 1;
        case TUYA_SPI_MODE2:
            return 2;
        case TUYA_SPI_MODE3:
            return 3;
        default:
            return 0;
    }
}

static OPERATE_RET __ensure_bus_and_device(TUYA_SPI_NUM_E port)
{
    esp_err_t esp_rt;
    TKL_ESP_SPI_CTX_T *ctx = &sg_spi[port];

    if (ctx->inited && ctx->dev != NULL) {
        return OPRT_OK;
    }

    if (ctx->host < 0) {
        return OPRT_NOT_SUPPORTED;
    }

    if (ctx->sclk_io <= 0 || ctx->mosi_io < 0) {
        /* Require at least SCLK + MOSI for master mode send operations. */
        ESP_LOGE(TAG, "SPI%d pins not configured (sclk=%d mosi=%d miso=%d cs=%d)",
                 port, ctx->sclk_io, ctx->mosi_io, ctx->miso_io, ctx->cs_io);
        return OPRT_INVALID_PARM;
    }

    /* Bus init (safe to call once per host). */
    spi_bus_config_t buscfg = {
        .mosi_io_num = ctx->mosi_io,
        .miso_io_num = (ctx->miso_io > 0) ? ctx->miso_io : -1,
        .sclk_io_num = ctx->sclk_io,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = (int)TKL_SPI_MAX_DMA_LEN,
    };

    /* If already initialised elsewhere, ESP_ERR_INVALID_STATE can happen. Treat as OK. */
    int dma_chan = (ctx->cfg.spi_dma_flags ? SPI_DMA_CH_AUTO : 0);
    esp_rt = spi_bus_initialize(ctx->host, &buscfg, dma_chan);
    if (esp_rt != ESP_OK && esp_rt != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "spi_bus_initialize(host=%d) failed: %s", (int)ctx->host, esp_err_to_name(esp_rt));
        return OPRT_COM_ERROR;
    }

    spi_device_interface_config_t devcfg = {
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .mode = __mode_to_idf(ctx->cfg.mode),
        .duty_cycle_pos = 128,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = (int)ctx->cfg.freq_hz,
        .spics_io_num = (ctx->cs_io > 0) ? ctx->cs_io : -1,
        .queue_size = 1,
        .flags = 0,
    };

    if (ctx->cfg.bitorder == TUYA_SPI_ORDER_LSB2MSB) {
        devcfg.flags |= SPI_DEVICE_BIT_LSBFIRST;
    }

    /* Only 8-bit transfers are used by current upper layers; keep data bits handling minimal. */
    esp_rt = spi_bus_add_device(ctx->host, &devcfg, &ctx->dev);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device(host=%d) failed: %s", (int)ctx->host, esp_err_to_name(esp_rt));
        return OPRT_COM_ERROR;
    }

    ctx->inited = true;
    return OPRT_OK;
}

static void *__dma_dup_if_needed(const void *buf, uint32_t len, bool for_tx)
{
    if (buf == NULL || len == 0) {
        return NULL;
    }

    /* If DMA is enabled, use DMA-capable memory and 32-bit alignment. */
    void *dup = heap_caps_malloc(len, MALLOC_CAP_DMA);
    if (dup == NULL) {
        return NULL;
    }

    if (for_tx) {
        memcpy(dup, buf, len);
    } else {
        memset(dup, 0, len);
    }

    return dup;
}

static void __dma_free_if_dup(void *ptr, const void *orig)
{
    if (ptr != NULL && ptr != orig) {
        heap_caps_free(ptr);
    }
}

static void __notify_irq(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_EVT_E evt)
{
    TKL_ESP_SPI_CTX_T *ctx = &sg_spi[port];
    if (ctx->irq_enabled && ctx->irq_cb) {
        ctx->irq_cb(port, evt);
    }
}

static OPERATE_RET __do_xfer(TUYA_SPI_NUM_E port, const void *tx, uint32_t tx_len, void *rx, uint32_t rx_len)
{
    OPERATE_RET rt;
    esp_err_t esp_rt;
    TKL_ESP_SPI_CTX_T *ctx = &sg_spi[port];

    TUYA_SPI_STATUS_T st = {0};
    st.busy = 1;
    ctx->status = st;

    rt = __ensure_bus_and_device(port);
    if (rt != OPRT_OK) {
        ctx->status.busy = 0;
        return rt;
    }

    void *tx_dma = (ctx->cfg.spi_dma_flags ? __dma_dup_if_needed(tx, tx_len, true) : (void *)tx);
    void *rx_dma = (ctx->cfg.spi_dma_flags ? __dma_dup_if_needed(rx, rx_len, false) : rx);
    if (ctx->cfg.spi_dma_flags) {
        if ((tx != NULL && tx_len > 0 && tx_dma == NULL) || (rx != NULL && rx_len > 0 && rx_dma == NULL)) {
            __dma_free_if_dup(tx_dma, tx);
            __dma_free_if_dup(rx_dma, rx);
            ctx->status.busy = 0;
            return OPRT_MALLOC_FAILED;
        }
    }

    spi_transaction_t t = {0};
    t.length = (size_t)tx_len * 8;
    t.rxlength = (size_t)rx_len * 8;
    t.tx_buffer = (tx_len > 0) ? tx_dma : NULL;
    t.rx_buffer = (rx_len > 0) ? rx_dma : NULL;

    esp_rt = spi_device_transmit(ctx->dev, &t);

    if (ctx->cfg.spi_dma_flags && rx != NULL && rx_len > 0 && rx_dma != rx) {
        memcpy(rx, rx_dma, rx_len);
    }

    __dma_free_if_dup(tx_dma, tx);
    __dma_free_if_dup(rx_dma, rx);

    ctx->status.busy = 0;

    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "spi_device_transmit failed: %s", esp_err_to_name(esp_rt));
        return OPRT_COM_ERROR;
    }

    ctx->last_count = (int32_t)(tx_len + rx_len);
    return OPRT_OK;
}

OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, const TUYA_SPI_BASE_CFG_T *cfg)
{
    if (port >= TUYA_SPI_NUM_MAX || cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (cfg->role != TUYA_SPI_ROLE_MASTER && cfg->role != TUYA_SPI_ROLE_MASTER_SIMPLEX) {
        return OPRT_NOT_SUPPORTED;
    }

    TKL_ESP_SPI_CTX_T *ctx = &sg_spi[port];
    memset(&ctx->status, 0, sizeof(ctx->status));
    ctx->last_count = 0;

    ctx->cfg = *cfg;
    ctx->host = __port_to_host(port);
    if (ctx->host < 0) {
        return OPRT_NOT_SUPPORTED;
    }

    /* Device/bus are lazily brought up on first transfer once pins are configured. */
    ctx->inited = false;
    ctx->dev = NULL;
    return OPRT_OK;
}

OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }

    TKL_ESP_SPI_CTX_T *ctx = &sg_spi[port];
    if (ctx->dev != NULL) {
        spi_bus_remove_device(ctx->dev);
        ctx->dev = NULL;
    }

    /* Avoid spi_bus_free() here because multiple clients may share the same host bus. */
    ctx->inited = false;
    memset(&ctx->status, 0, sizeof(ctx->status));
    ctx->last_count = 0;
    return OPRT_OK;
}

OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, void *data, uint32_t size)
{
    if (port >= TUYA_SPI_NUM_MAX || (size > 0 && data == NULL)) {
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET rt = __do_xfer(port, data, size, NULL, 0);
    if (rt == OPRT_OK) {
        __notify_irq(port, TUYA_SPI_EVENT_TX_COMPLETE);
    }
    return rt;
}

OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, void *data, uint32_t size)
{
    if (port >= TUYA_SPI_NUM_MAX || (size > 0 && data == NULL)) {
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET rt = __do_xfer(port, NULL, 0, data, size);
    if (rt == OPRT_OK) {
        __notify_irq(port, TUYA_SPI_EVENT_RX_COMPLETE);
    }
    return rt;
}

OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, void *send_buf, void *receive_buf, uint32_t length)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }
    if (length > 0 && send_buf == NULL && receive_buf == NULL) {
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET rt = __do_xfer(port, send_buf, length, receive_buf, length);
    if (rt == OPRT_OK) {
        __notify_irq(port, TUYA_SPI_EVENT_TRANSFER_COMPLETE);
    }
    return rt;
}

OPERATE_RET tkl_spi_transfer_with_length(TUYA_SPI_NUM_E port,
                                         void *send_buf,
                                         uint32_t send_len,
                                         void *receive_buf,
                                         uint32_t receive_len)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }
    if ((send_len > 0 && send_buf == NULL) || (receive_len > 0 && receive_buf == NULL)) {
        return OPRT_INVALID_PARM;
    }

    OPERATE_RET rt = __do_xfer(port, send_buf, send_len, receive_buf, receive_len);
    if (rt == OPRT_OK) {
        __notify_irq(port, TUYA_SPI_EVENT_TRANSFER_COMPLETE);
    }
    return rt;
}

OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port)
{
    (void)port;
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_spi_get_status(TUYA_SPI_NUM_E port, TUYA_SPI_STATUS_T *status)
{
    if (port >= TUYA_SPI_NUM_MAX || status == NULL) {
        return OPRT_INVALID_PARM;
    }
    *status = sg_spi[port].status;
    return OPRT_OK;
}

OPERATE_RET tkl_spi_irq_init(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_CB cb)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }
    sg_spi[port].irq_cb = cb;
    return OPRT_OK;
}

OPERATE_RET tkl_spi_irq_enable(TUYA_SPI_NUM_E port)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }
    sg_spi[port].irq_enabled = true;
    return OPRT_OK;
}

OPERATE_RET tkl_spi_irq_disable(TUYA_SPI_NUM_E port)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return OPRT_INVALID_PARM;
    }
    sg_spi[port].irq_enabled = false;
    return OPRT_OK;
}

int32_t tkl_spi_get_data_count(TUYA_SPI_NUM_E port)
{
    if (port >= TUYA_SPI_NUM_MAX) {
        return -1;
    }
    return sg_spi[port].last_count;
}

OPERATE_RET tkl_spi_ioctl(TUYA_SPI_NUM_E port, uint32_t cmd, void *args)
{
    (void)port;
    (void)cmd;
    (void)args;
    return OPRT_NOT_SUPPORTED;
}

uint32_t tkl_spi_get_max_dma_data_length(void)
{
    return TKL_SPI_MAX_DMA_LEN;
}

