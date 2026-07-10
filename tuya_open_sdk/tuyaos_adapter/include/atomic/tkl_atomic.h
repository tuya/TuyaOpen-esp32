/**
 * @file tkl_atomic.h
 * @brief Atomic operations adapter for ESP32 platform
 * @version 0.1
 * @date 2024-01-01
 *
 * @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
 */
#ifndef __TKL_ATOMIC_H__
#define __TKL_ATOMIC_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t tkl_system_atomic_inc(uint32_t volatile *val);

uint32_t tkl_system_atomic_dec(uint32_t volatile *val);

uint32_t tkl_system_atomic_add(uint32_t volatile *val, uint32_t count);

uint32_t tkl_system_atomic_sub(uint32_t volatile *val, uint32_t count);

void *tkl_system_atomic_swap(void * volatile *pdst, void *psrc);

BOOL_T tkl_system_atomic_cmp_and_set(uint32_t volatile *pdst, uint32_t val, uint32_t compare);

BOOL_T tkl_system_atomic_cmp_and_swap(void * volatile *pdst, void *psrc, void *pcmp);

uint32_t tkl_system_atomic_and(uint32_t volatile *pdst, uint32_t val);

uint32_t tkl_system_atomic_or(uint32_t volatile *pdst, uint32_t val);

uint32_t tkl_system_atomic_nand(uint32_t volatile *pdst, uint32_t val);

uint32_t tkl_system_atomic_xor(uint32_t volatile *pdst, uint32_t val);

#ifdef __cplusplus
}
#endif

#endif /* __TKL_ATOMIC_H__ */
