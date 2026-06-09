/**
 * @file tkl_dma2d.c
 * @brief 2D-DMA hardware acceleration for ESP32-P4, backed by the PPA
 *        (Pixel-Processing Accelerator).
 *
 * The PPA's scale-rotate-mirror (SRM) engine does a 2D block transfer with
 * optional pixel-format conversion, which covers both tkl_dma2d_memcpy()
 * (same format) and tkl_dma2d_convert() (different format). Transfers are
 * issued in non-blocking mode and completion is reported through the callback
 * registered in tkl_dma2d_init() (invoked from the PPA ISR).
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include "tkl_dma2d.h"

#include "soc/soc_caps.h"

#if defined(SOC_PPA_SUPPORTED) && SOC_PPA_SUPPORTED

#include "driver/ppa.h"
#include "esp_log.h"

#define TAG "tkl_dma2d"

static ppa_client_handle_t s_ppa_client = NULL;
static TUYA_DMA2D_IRQ_CB   s_irq_cb     = NULL;
static void               *s_irq_arg    = NULL;

/* Runs in PPA ISR context. tkl_semaphore_post (the usual consumer of this
 * callback) is ISR-safe, so the upstream callback may be invoked directly. */
static bool __ppa_trans_done(ppa_client_handle_t client, ppa_event_data_t *edata, void *user_data)
{
    (void)client;
    (void)edata;
    (void)user_data;

    if (s_irq_cb) {
        s_irq_cb(TUYA_DMA2D_TRANS_COMPLETE_ISR, s_irq_arg);
    }
    return false;
}

static int __fmt_to_srm_cm(TUYA_FRAME_FMT_E fmt, ppa_srm_color_mode_t *cm, uint8_t *bpp)
{
    switch (fmt) {
    case TUYA_FRAME_FMT_RGB565:
        *cm  = PPA_SRM_COLOR_MODE_RGB565;
        *bpp = 2;
        return 0;
    case TUYA_FRAME_FMT_RGB888:
        *cm  = PPA_SRM_COLOR_MODE_RGB888;
        *bpp = 3;
        return 0;
    default:
        /* YUV422 / YUV420 / others are not supported by the PPA SRM input on
         * this target. */
        return -1;
    }
}

OPERATE_RET tkl_dma2d_init(const TUYA_DMA2D_BASE_CFG_T *cfg)
{
    if (s_ppa_client) {
        /* already initialised: just refresh the callback */
        if (cfg) {
            s_irq_cb  = cfg->cb;
            s_irq_arg = cfg->arg;
        }
        return OPRT_OK;
    }

    ppa_client_config_t client_cfg = {
        .oper_type             = PPA_OPERATION_SRM,
        .max_pending_trans_num = 2,
    };
    if (ppa_register_client(&client_cfg, &s_ppa_client) != ESP_OK) {
        ESP_LOGE(TAG, "ppa_register_client failed");
        s_ppa_client = NULL;
        return OPRT_COM_ERROR;
    }

    ppa_event_callbacks_t cbs = {
        .on_trans_done = __ppa_trans_done,
    };
    if (ppa_client_register_event_callbacks(s_ppa_client, &cbs) != ESP_OK) {
        ESP_LOGE(TAG, "ppa register event cb failed");
        ppa_unregister_client(s_ppa_client);
        s_ppa_client = NULL;
        return OPRT_COM_ERROR;
    }

    if (cfg) {
        s_irq_cb  = cfg->cb;
        s_irq_arg = cfg->arg;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_dma2d_deinit(void)
{
    if (s_ppa_client) {
        ppa_unregister_client(s_ppa_client);
        s_ppa_client = NULL;
    }
    s_irq_cb  = NULL;
    s_irq_arg = NULL;
    return OPRT_OK;
}

/* Shared SRM submit for both memcpy (same fmt) and convert (different fmt);
 * the PPA SRM engine converts only when the input/output color modes differ. */
static OPERATE_RET __dma2d_srm(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst)
{
    if (NULL == src || NULL == dst || NULL == src->pbuf || NULL == dst->pbuf) {
        return OPRT_INVALID_PARM;
    }
    if (NULL == s_ppa_client) {
        ESP_LOGE(TAG, "dma2d not initialised");
        return OPRT_COM_ERROR;
    }

    ppa_srm_color_mode_t in_cm, out_cm;
    uint8_t in_bpp, out_bpp;
    if (__fmt_to_srm_cm(src->type, &in_cm, &in_bpp) != 0 ||
        __fmt_to_srm_cm(dst->type, &out_cm, &out_bpp) != 0) {
        ESP_LOGE(TAG, "unsupported pixel format (src=%d dst=%d)", src->type, dst->type);
        return OPRT_NOT_SUPPORTED;
    }

    if (src->axis.x_axis >= src->width || src->axis.y_axis >= src->height ||
        dst->axis.x_axis >= dst->width || dst->axis.y_axis >= dst->height) {
        return OPRT_INVALID_PARM;
    }

    /* block size: whole source frame when width_cp/height_cp is 0 */
    uint32_t block_w = (src->width_cp  == 0) ? src->width  : src->width_cp;
    uint32_t block_h = (src->height_cp == 0) ? src->height : src->height_cp;

    /* clamp to what remains inside both the source and destination frames */
    if (src->axis.x_axis + block_w > src->width)  block_w = src->width  - src->axis.x_axis;
    if (src->axis.y_axis + block_h > src->height) block_h = src->height - src->axis.y_axis;
    if (dst->axis.x_axis + block_w > dst->width)  block_w = dst->width  - dst->axis.x_axis;
    if (dst->axis.y_axis + block_h > dst->height) block_h = dst->height - dst->axis.y_axis;
    if (0 == block_w || 0 == block_h) {
        return OPRT_INVALID_PARM;
    }

    ppa_srm_oper_config_t op = {
        .in = {
            .buffer         = src->pbuf,
            .pic_w          = src->width,
            .pic_h          = src->height,
            .block_w        = block_w,
            .block_h        = block_h,
            .block_offset_x = src->axis.x_axis,
            .block_offset_y = src->axis.y_axis,
            .srm_cm         = in_cm,
        },
        .out = {
            .buffer         = dst->pbuf,
            .buffer_size    = (uint32_t)dst->width * dst->height * out_bpp,
            .pic_w          = dst->width,
            .pic_h          = dst->height,
            .block_offset_x = dst->axis.x_axis,
            .block_offset_y = dst->axis.y_axis,
            .srm_cm         = out_cm,
        },
        .rotation_angle = PPA_SRM_ROTATION_ANGLE_0,
        .scale_x        = 1.0f,
        .scale_y        = 1.0f,
        .mirror_x       = false,
        .mirror_y       = false,
        .rgb_swap       = false,
        .byte_swap      = false,
        .mode           = PPA_TRANS_MODE_NON_BLOCKING,
        .user_data      = NULL,
    };

    esp_err_t err = ppa_do_scale_rotate_mirror(s_ppa_client, &op);
    if (ESP_OK != err) {
        ESP_LOGE(TAG, "ppa srm failed: 0x%x", err);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_dma2d_convert(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst)
{
    return __dma2d_srm(src, dst);
}

OPERATE_RET tkl_dma2d_memcpy(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst)
{
    return __dma2d_srm(src, dst);
}

#else /* !SOC_PPA_SUPPORTED */

OPERATE_RET tkl_dma2d_init(const TUYA_DMA2D_BASE_CFG_T *cfg)
{
    (void)cfg;
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_dma2d_deinit(void)
{
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_dma2d_convert(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst)
{
    (void)src;
    (void)dst;
    return OPRT_NOT_SUPPORTED;
}

OPERATE_RET tkl_dma2d_memcpy(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst)
{
    (void)src;
    (void)dst;
    return OPRT_NOT_SUPPORTED;
}

#endif /* SOC_PPA_SUPPORTED */
