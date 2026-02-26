/**
 * @file tkl_adc.c
 * @brief ADC adapter for ESP-IDF v5.x (Oneshot + Calibration)
 *
 * Implements TuyaOpen TKL ADC interface on top of ESP-IDF
 *   - esp_adc/adc_oneshot.h   (single / multi-channel oneshot reads)
 *   - esp_adc/adc_cali.h      (raw -> mV calibration)
 *   - esp_adc/adc_cali_scheme.h (line-fitting / curve-fitting)
 *
 * Mapping from TUYA_ADC_NUM_E -> ESP-IDF adc_unit_t:
 *   TUYA_ADC_NUM_0 -> ADC_UNIT_1
 *   TUYA_ADC_NUM_1 -> ADC_UNIT_2
 *
 * Channel list (TUYA_ADC_BASE_CFG_T::ch_list) is a bitmask where
 * bit N means ADC_CHANNEL_N should be configured.
 *
 * @copyright Copyright 2020-2025 Tuya Inc. All Rights Reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_err.h"
#include "esp_log.h"
#include "soc/soc_caps.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "hal/adc_types.h"

#include "tuya_error_code.h"
#include "tuya_cloud_types.h"

#include "tkl_adc.h"

/* ------------------------------------------------------------------ */
/*                        Internal definitions                         */
/* ------------------------------------------------------------------ */

static const char *TAG = "tkl_adc";

/** Maximum number of ADC units we support (ADC1 / ADC2) */
#define ADC_UNIT_MAX_SUPPORTED  2

/** Maximum number of ADC channels per unit */
#define ADC_CHAN_MAX             SOC_ADC_MAX_CHANNEL_NUM

/** Per-unit runtime context */
typedef struct {
    bool                      inited;           /**< whether this unit has been initialised       */
    adc_oneshot_unit_handle_t handle;            /**< ESP-IDF oneshot handle                       */
    adc_cali_handle_t         cali_handle;       /**< calibration handle (may be NULL)             */
    bool                      cali_valid;        /**< whether calibration was created successfully */
    TUYA_ADC_BASE_CFG_T       cfg;               /**< copy of the configuration from caller        */
    uint32_t                  ch_mask;            /**< channel bitmask that was configured          */
} adc_ctx_t;

static adc_ctx_t s_adc_ctx[ADC_UNIT_MAX_SUPPORTED];

/* ------------------------------------------------------------------ */
/*                         Helper functions                            */
/* ------------------------------------------------------------------ */

/**
 * @brief Map TUYA port number to ESP-IDF adc_unit_t.
 *        TUYA_ADC_NUM_0 -> ADC_UNIT_1, TUYA_ADC_NUM_1 -> ADC_UNIT_2
 */
static inline adc_unit_t __port_to_unit(TUYA_ADC_NUM_E port)
{
    return (port == TUYA_ADC_NUM_0) ? ADC_UNIT_1 : ADC_UNIT_2;
}

/**
 * @brief Map TUYA port number to internal context array index (0 or 1).
 */
static inline int __port_to_idx(TUYA_ADC_NUM_E port)
{
    return (port <= TUYA_ADC_NUM_0) ? 0 : 1;
}

/**
 * @brief Convert TUYA bit-width value (e.g. 9/10/11/12/13) to adc_bitwidth_t.
 */
static adc_bitwidth_t __width_to_esp(uint8_t width)
{
    switch (width) {
        case 9:  return ADC_BITWIDTH_9;
        case 10: return ADC_BITWIDTH_10;
        case 11: return ADC_BITWIDTH_11;
        case 12: return ADC_BITWIDTH_12;
        case 13: return ADC_BITWIDTH_13;
        default: return ADC_BITWIDTH_DEFAULT;
    }
}

/**
 * @brief Try to create an ADC calibration handle.
 *        Supports both curve-fitting (ESP32-S2/S3/C3/C6/H2) and
 *        line-fitting (ESP32) schemes automatically.
 *
 * @return true  if calibration was set up successfully
 * @return false if calibration is unavailable (raw -> mV not possible)
 */
static bool __calibration_init(adc_unit_t unit, adc_atten_t atten,
                               adc_bitwidth_t bitwidth,
                               adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    /* ---- Curve fitting (ESP32-S2 / S3 / C3 / C6 / H2) ---- */
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_curve_fitting_config_t cali_cfg = {
            .unit_id  = unit,
            .atten    = atten,
            .bitwidth = bitwidth,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    /* ---- Line fitting (ESP32) ---- */
#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        adc_cali_line_fitting_config_t cali_cfg = {
            .unit_id  = unit,
            .atten    = atten,
            .bitwidth = bitwidth,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_cfg, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;

    if (calibrated) {
        ESP_LOGI(TAG, "ADC unit %d calibration success", (int)unit);
    } else {
        ESP_LOGW(TAG, "ADC unit %d calibration not available, "
                 "raw_to_voltage will use formula", (int)unit);
    }
    return calibrated;
}

/**
 * @brief Delete a calibration handle, matching the scheme that was used.
 */
static void __calibration_deinit(adc_cali_handle_t handle)
{
    if (handle == NULL) {
        return;
    }

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (adc_cali_delete_scheme_curve_fitting(handle) == ESP_OK) {
        return;
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (adc_cali_delete_scheme_line_fitting(handle) == ESP_OK) {
        return;
    }
#endif
}

/* ------------------------------------------------------------------ */
/*                      TKL ADC public interface                       */
/* ------------------------------------------------------------------ */

/**
 * @brief Initialise an ADC unit and configure all requested channels.
 *
 *  - Creates an oneshot unit (ADC_UNIT_1 or ADC_UNIT_2).
 *  - Iterates through cfg->ch_list bitmask, calling
 *    adc_oneshot_config_channel() for every enabled bit.
 *  - Creates a calibration handle for raw -> mV conversion.
 */
OPERATE_RET tkl_adc_init(TUYA_ADC_NUM_E port_num, TUYA_ADC_BASE_CFG_T *cfg)
{
    if (cfg == NULL) {
        return OPRT_INVALID_PARM;
    }

    int idx = __port_to_idx(port_num);
    if (idx >= ADC_UNIT_MAX_SUPPORTED) {
        ESP_LOGE(TAG, "unsupported ADC port %d", port_num);
        return OPRT_INVALID_PARM;
    }

    adc_ctx_t *ctx = &s_adc_ctx[idx];

    /* Already initialised - deinit first */
    if (ctx->inited) {
        ESP_LOGW(TAG, "ADC unit %d already inited, deinit first", idx);
        tkl_adc_deinit(port_num);
    }

    /* ---------- 1. Create oneshot unit ---------- */
    adc_unit_t unit = __port_to_unit(port_num);

    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id  = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };

    esp_err_t err = adc_oneshot_new_unit(&init_cfg, &ctx->handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(err));
        return OPRT_COM_ERROR;
    }

    /* ---------- 2. Configure each channel ---------- */
    adc_bitwidth_t bw = __width_to_esp(cfg->width);
    adc_atten_t   att = ADC_ATTEN_DB_12;   /* 0 ~ 3.3 V full range */

    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = bw,
        .atten    = att,
    };

    uint32_t ch_mask = cfg->ch_list.data;
    ctx->ch_mask = ch_mask;

    for (int ch = 0; ch < ADC_CHAN_MAX; ch++) {
        if (ch_mask & (1U << ch)) {
            err = adc_oneshot_config_channel(ctx->handle, (adc_channel_t)ch, &chan_cfg);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "config channel %d failed: %s", ch, esp_err_to_name(err));
                /* clean up what we already set up */
                adc_oneshot_del_unit(ctx->handle);
                ctx->handle = NULL;
                return OPRT_COM_ERROR;
            }
            ESP_LOGI(TAG, "ADC unit %d channel %d configured (width=%d, atten=DB_12)",
                     (int)unit, ch, cfg->width);
        }
    }

    /* ---------- 3. Calibration ---------- */
    ctx->cali_valid = __calibration_init(unit, att, bw, &ctx->cali_handle);

    /* ---------- 4. Save context ---------- */
    memcpy(&ctx->cfg, cfg, sizeof(TUYA_ADC_BASE_CFG_T));
    ctx->inited = true;

    ESP_LOGI(TAG, "ADC unit %d init OK, ch_mask=0x%08lx, ch_nums=%d",
             (int)unit, (unsigned long)ch_mask, cfg->ch_nums);
    return OPRT_OK;
}

/**
 * @brief Deinitialise the ADC unit, releasing ESP-IDF resources.
 */
OPERATE_RET tkl_adc_deinit(TUYA_ADC_NUM_E port_num)
{
    int idx = __port_to_idx(port_num);
    if (idx >= ADC_UNIT_MAX_SUPPORTED) {
        return OPRT_INVALID_PARM;
    }

    adc_ctx_t *ctx = &s_adc_ctx[idx];
    if (!ctx->inited) {
        return OPRT_OK;     /* nothing to do */
    }

    /* Delete calibration handle first */
    if (ctx->cali_handle) {
        __calibration_deinit(ctx->cali_handle);
        ctx->cali_handle = NULL;
        ctx->cali_valid  = false;
    }

    /* Delete oneshot unit */
    if (ctx->handle) {
        adc_oneshot_del_unit(ctx->handle);
        ctx->handle = NULL;
    }

    ctx->inited  = false;
    ctx->ch_mask = 0;

    ESP_LOGI(TAG, "ADC unit %d deinited", idx);
    return OPRT_OK;
}

/**
 * @brief Get the configured ADC bit-width for a given port.
 */
uint8_t tkl_adc_width_get(TUYA_ADC_NUM_E port_num)
{
    int idx = __port_to_idx(port_num);
    if (idx >= ADC_UNIT_MAX_SUPPORTED || !s_adc_ctx[idx].inited) {
        return 0;
    }
    return s_adc_ctx[idx].cfg.width;
}

/**
 * @brief Get the ADC reference voltage in mV.
 *
 * With ADC_ATTEN_DB_12 the measurable range is approximately 0 - 3300 mV.
 */
uint32_t tkl_adc_ref_voltage_get(TUYA_ADC_NUM_E port_num)
{
    (void)port_num;
    return 3300;  /* mV, with ADC_ATTEN_DB_12 */
}

/**
 * @brief Get chip temperature via internal temperature sensor.
 *
 * @note  On ESP32 the internal temperature sensor is accessed via the
 *        dedicated temperature_sensor driver, not via ADC.
 *        Return OPRT_NOT_SUPPORTED here.
 */
int32_t tkl_adc_temperature_get(void)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief Read raw ADC data from all configured channels.
 *
 * Reads each channel that was set in ch_list during init, storing
 * results sequentially into buff[]. The caller must ensure buff has
 * room for at least cfg->ch_nums values (or len values).
 *
 * @param port_num  ADC port
 * @param buff      Output buffer for raw readings
 * @param len       Number of readings to store (<= ch_nums)
 */
OPERATE_RET tkl_adc_read_data(TUYA_ADC_NUM_E port_num, int32_t *buff, uint16_t len)
{
    if (buff == NULL || len == 0) {
        return OPRT_INVALID_PARM;
    }

    int idx = __port_to_idx(port_num);
    if (idx >= ADC_UNIT_MAX_SUPPORTED || !s_adc_ctx[idx].inited) {
        return OPRT_COM_ERROR;
    }

    adc_ctx_t *ctx = &s_adc_ctx[idx];
    uint16_t count = 0;

    for (int ch = 0; ch < ADC_CHAN_MAX && count < len; ch++) {
        if (ctx->ch_mask & (1U << ch)) {
            int raw = 0;
            esp_err_t err = adc_oneshot_read(ctx->handle, (adc_channel_t)ch, &raw);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "read channel %d failed: %s", ch, esp_err_to_name(err));
                buff[count] = 0;
            } else {
                buff[count] = (int32_t)raw;
            }
            count++;
        }
    }

    return OPRT_OK;
}

/**
 * @brief Read a single ADC channel.
 *
 * @param port_num  ADC port
 * @param ch_id     Channel number (0-9)
 * @param data      Output raw reading
 */
OPERATE_RET tkl_adc_read_single_channel(TUYA_ADC_NUM_E port_num, uint8_t ch_id, int32_t *data)
{
    if (data == NULL) {
        return OPRT_INVALID_PARM;
    }

    int idx = __port_to_idx(port_num);
    if (idx >= ADC_UNIT_MAX_SUPPORTED || !s_adc_ctx[idx].inited) {
        return OPRT_COM_ERROR;
    }

    adc_ctx_t *ctx = &s_adc_ctx[idx];

    /* Validate the channel was configured during init */
    if (!(ctx->ch_mask & (1U << ch_id))) {
        ESP_LOGE(TAG, "channel %d was not configured at init (mask=0x%08lx)",
                 ch_id, (unsigned long)ctx->ch_mask);
        return OPRT_INVALID_PARM;
    }

    int raw = 0;
    esp_err_t err = adc_oneshot_read(ctx->handle, (adc_channel_t)ch_id, &raw);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "read single channel %d failed: %s", ch_id, esp_err_to_name(err));
        return OPRT_COM_ERROR;
    }

    *data = (int32_t)raw;
    return OPRT_OK;
}

/**
 * @brief Read calibrated voltage (mV) from all configured channels.
 *
 * Uses adc_cali_raw_to_voltage() to convert raw readings into
 * millivolt values. Falls back to a simple formula if calibration
 * was not available:  voltage_mV = raw * 3300 / ((1 << width) - 1)
 *
 * @param port_num  ADC port
 * @param buff      Output buffer for voltage values (mV)
 * @param len       Number of values to store
 */
OPERATE_RET tkl_adc_read_voltage(TUYA_ADC_NUM_E port_num, int32_t *buff, uint16_t len)
{
    if (buff == NULL || len == 0) {
        return OPRT_INVALID_PARM;
    }

    int idx = __port_to_idx(port_num);
    if (idx >= ADC_UNIT_MAX_SUPPORTED || !s_adc_ctx[idx].inited) {
        return OPRT_COM_ERROR;
    }

    adc_ctx_t *ctx = &s_adc_ctx[idx];
    uint16_t count = 0;

    for (int ch = 0; ch < ADC_CHAN_MAX && count < len; ch++) {
        if (ctx->ch_mask & (1U << ch)) {
            int raw = 0;
            esp_err_t err = adc_oneshot_read(ctx->handle, (adc_channel_t)ch, &raw);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "read channel %d for voltage failed: %s",
                         ch, esp_err_to_name(err));
                buff[count] = 0;
                count++;
                continue;
            }

            int voltage_mv = 0;
            if (ctx->cali_valid && ctx->cali_handle) {
                /* Use calibration driver for accurate mV conversion */
                err = adc_cali_raw_to_voltage(ctx->cali_handle, raw, &voltage_mv);
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "cali raw_to_voltage ch %d failed, use formula", ch);
                    goto fallback;
                }
            } else {
fallback:
                /* Simple linear conversion: Vout = raw * Vmax / Dmax */
                {
                    uint8_t width = ctx->cfg.width ? ctx->cfg.width : 12;
                    int dmax = (1 << width) - 1;
                    voltage_mv = (int)((int64_t)raw * 3300 / dmax);
                }
            }

            buff[count] = (int32_t)voltage_mv;
            count++;
        }
    }

    return OPRT_OK;
}
