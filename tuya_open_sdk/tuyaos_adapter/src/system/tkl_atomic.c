/**
 * @file tkl_atomic.c
 * @brief Atomic operations implementation for ESP32 (Xtensa/RISC-V)
 *
 * Uses GCC built-in atomic operations which map to hardware instructions.
 */

#include "tkl_atomic.h"

uint32_t tkl_system_atomic_inc(uint32_t volatile *val)
{
    return __sync_fetch_and_add(val, 1);
}

uint32_t tkl_system_atomic_dec(uint32_t volatile *val)
{
    return __sync_fetch_and_sub(val, 1);
}

uint32_t tkl_system_atomic_add(uint32_t volatile *val, uint32_t count)
{
    return __sync_fetch_and_add(val, count);
}

uint32_t tkl_system_atomic_sub(uint32_t volatile *val, uint32_t count)
{
    return __sync_fetch_and_sub(val, count);
}

void *tkl_system_atomic_swap(void * volatile *pdst, void *psrc)
{
    void *old;
    do {
        old = *pdst;
    } while (!__sync_bool_compare_and_swap(pdst, old, psrc));
    return old;
}

BOOL_T tkl_system_atomic_cmp_and_set(uint32_t volatile *pdst, uint32_t val, uint32_t compare)
{
    return __sync_bool_compare_and_swap(pdst, compare, val) ? TRUE : FALSE;
}

BOOL_T tkl_system_atomic_cmp_and_swap(void * volatile *pdst, void *psrc, void *pcmp)
{
    return __sync_bool_compare_and_swap(pdst, pcmp, psrc) ? TRUE : FALSE;
}

uint32_t tkl_system_atomic_and(uint32_t volatile *pdst, uint32_t val)
{
    return __sync_fetch_and_and(pdst, val);
}

uint32_t tkl_system_atomic_or(uint32_t volatile *pdst, uint32_t val)
{
    return __sync_fetch_and_or(pdst, val);
}

uint32_t tkl_system_atomic_nand(uint32_t volatile *pdst, uint32_t val)
{
    return __sync_fetch_and_nand(pdst, val);
}

uint32_t tkl_system_atomic_xor(uint32_t volatile *pdst, uint32_t val)
{
    return __sync_fetch_and_xor(pdst, val);
}
