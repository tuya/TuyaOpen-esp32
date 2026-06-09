/**
 * @file tkl_dma2d.h
 * @brief 2D-DMA / pixel-processing hardware acceleration abstraction.
 * @version 0.1
 * @date 2026-06-08
 *
 * @copyright Copyright 2021-2026 Tuya Inc. All Rights Reserved.
 */

#ifndef __TKL_DMA2D_H__
#define __TKL_DMA2D_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief dma2d interrupt mode
 */
typedef enum {
    TUYA_DMA2D_TRANS_COMPLETE_ISR,
    TUYA_DMA2D_TRANS_ERROR_ISR,
} TUYA_DMA2D_IRQ_E;

typedef struct {
    uint16_t x_axis;
    uint16_t y_axis;
} TKL_DMA2D_POINT_T;

typedef struct {
    TUYA_FRAME_FMT_E   type;        // [in]     : pixel format of pbuf
    uint8_t           *pbuf;        // [in/out] : frame buffer base address
    uint16_t           width;       // [in]     : frame buffer width  (pixels)
    uint16_t           height;      // [in]     : frame buffer height (pixels)
    TKL_DMA2D_POINT_T  axis;        // [in]     : block offset inside the frame buffer
    uint16_t           width_cp;    // [in]     : block width to copy  (0 => whole frame)
    uint16_t           height_cp;   // [in]     : block height to copy (0 => whole frame)
} TKL_DMA2D_FRAME_INFO_T;

typedef void (*TUYA_DMA2D_IRQ_CB)(TUYA_DMA2D_IRQ_E type, void *args);

/**
 * @brief dma2d interrupt config
 */
typedef struct {
    TUYA_DMA2D_IRQ_CB cb;
    void             *arg;
} TUYA_DMA2D_BASE_CFG_T;

/**
 * @brief dma2d init
 *
 * @param[in] cfg: dma2d config (completion/error callback)
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dma2d_init(const TUYA_DMA2D_BASE_CFG_T *cfg);

/**
 * @brief dma2d deinit
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dma2d_deinit(void);

/**
 * @brief pixel-format converting 2D copy (e.g. RGB888 -> RGB565)
 *
 * @param[in]     src : source frame description
 * @param[in/out] dst : destination frame description
 *
 * @note Asynchronous: returns once the transfer is queued; completion is
 *       reported through the callback registered in tkl_dma2d_init().
 * @note The destination buffer must satisfy the underlying hardware's cache
 *       alignment requirements.
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dma2d_convert(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst);

/**
 * @brief same-format hardware 2D block copy (blit)
 *
 * @param[in]  src : source frame description
 * @param[out] dst : destination frame description
 *
 * @note Asynchronous, see tkl_dma2d_convert().
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_dma2d_memcpy(TKL_DMA2D_FRAME_INFO_T *src, TKL_DMA2D_FRAME_INFO_T *dst);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __TKL_DMA2D_H__
