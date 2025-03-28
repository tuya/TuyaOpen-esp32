/**
 * @file tuya_ringbuff.h
 * @brief Common process - ring buff
 * @version 1.0.0
 * @date 2021-06-03
 *
 * @copyright Copyright 2018-2021 Tuya Inc. All Rights Reserved.
 *
 */
#ifndef __TUYA_RINGBUF_H__
#define __TUYA_RINGBUF_H__


#ifdef __cplusplus
    extern "C" {
#endif

#include "tuya_cloud_types.h"


typedef void* TUYA_RINGBUFF_T;

typedef enum {
    OVERFLOW_STOP_TYPE = 0, ///< unread buff area will not be overwritten when writing overflow
    OVERFLOW_COVERAGE_TYPE, ///< unread buff area will be overwritten when writing overflow
}RINGBUFF_TYPE_E;

#define TUYA_PSARM_SUPPORT
#if defined(TUYA_PSARM_SUPPORT) && defined(TUYA_PSARM_SUPPORT)
#define TY_RINGBUF_PSRAM_FLAG 0x80
#define OVERFLOW_PSRAM_STOP_TYPE (OVERFLOW_STOP_TYPE | TY_RINGBUF_PSRAM_FLAG)
#define OVERFLOW_PSRAM_COVERAGE_TYPE (OVERFLOW_COVERAGE_TYPE | TY_RINGBUF_PSRAM_FLAG)
#endif

/**
 * @brief ringbuff create
 *
 * @param[in]   len:      ringbuff length
 * @param[in]   type:     ringbuff type
 * @param[in]   ringbuff: ringbuff handle
 * @return  TRUE/ FALSE
 */
OPERATE_RET tuya_ring_buff_create(uint16_t len, RINGBUFF_TYPE_E type, TUYA_RINGBUFF_T *ringbuff);

/**
 * @brief ringbuff free
 *
 * @param[in]   ringbuff: ringbuff handle
 * @return  TRUE/ FALSE
 */
OPERATE_RET tuya_ring_buff_free(TUYA_RINGBUFF_T ringbuff);

/**
 * @brief ringbuff reset 
 * this API not free buff
 *
 * @param[in]   ringbuff: ringbuff handle
 * @return  none
 */
OPERATE_RET tuya_ring_buff_reset(TUYA_RINGBUFF_T ringbuff);

/**
 * @brief ringbuff free size get
 *
 * @param[in]   ringbuff: ringbuff handle
 * @return  size of ringbuff not used
 */
uint16_t tuya_ring_buff_free_size_get(TUYA_RINGBUFF_T ringbuff);

/**
 * @brief ringbuff used size get
 *
 * @param[in]   ringbuff: ringbuff handle
 * @return  size of ringbuff used
 */
uint16_t tuya_ring_buff_used_size_get(TUYA_RINGBUFF_T ringbuff);

/**
 * @brief ringbuff data read 
 *
 * @param[in]   ringbuff: ringbuff handle
 * @param[in]   data:     point to the data read cache 
 * @param[in]   len:      read len
 * @return  length of the data read
 */
uint16_t tuya_ring_buff_read(TUYA_RINGBUFF_T ringbuff, void *data, uint16_t len);

/**
 * @brief Discards a specified number of bytes from the ring buffer.
 *
 * This function removes a specified length of data from the ring buffer,
 * effectively advancing the read pointer by the given length. The discarded
 * data is no longer accessible after this operation.
 *
 * @param[in] ringbuff The ring buffer instance to operate on.
 * @param[in] len      The number of bytes to discard from the ring buffer.
 *
 * @return The actual number of bytes discarded. This may be less than the
 *         requested length if the ring buffer contains fewer bytes than `len`.
 */
uint32_t tuya_ring_buff_discard(TUYA_RINGBUFF_T ringbuff, uint32_t len);

/**
 * @brief ringbuff data peek 
 * this API read data but not output position
 * 
 * @param[in]   ringbuff: ringbuff handle
 * @param[in]   data:     point to the data read cache 
 * @param[in]   len:      read len
 * @return  length of the data read
 */
uint16_t tuya_ring_buff_peek(TUYA_RINGBUFF_T ringbuff, void *data, uint16_t len);

/**
 * @brief ringbuff data write 
 * 
 * @param[in]   ringbuff: ringbuff handle
 * @param[in]   data:     point to the data to be write 
 * @param[in]   len:      write len
 * @return  length of the data write
 */
uint16_t tuya_ring_buff_write(TUYA_RINGBUFF_T ringbuff, const void *data, uint16_t len);


#ifdef __cplusplus
}
#endif

#endif

