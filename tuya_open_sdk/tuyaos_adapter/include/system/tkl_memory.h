/**
 * @file tkl_memory.h
 * @brief Common process - adapter the semaphore api provide by OS
 * @version 0.1
 * @date 2020-11-09
 *
 * @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
 *
 */
#ifndef __TKL_MEMORY_H__
#define __TKL_MEMORY_H__

#include "tuya_cloud_types.h"

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Alloc memory of system
 *
 * @param[in] size: memory size
 *
 * @note This API is used to alloc memory of system.
 *
 * @return the memory address malloced
 */
void *tkl_system_malloc(size_t size);

/**
 * @brief Free memory of system
 *
 * @param[in] ptr: memory point
 *
 * @note This API is used to free memory of system.
 *
 * @return void
 */
void tkl_system_free(void *ptr);

/**
 * @brief Allocate and clear the memory
 *
 * @param[in]       nitems      the numbers of memory block
 * @param[in]       size        the size of the memory block
 *
 * @return the memory address calloced
 */
void *tkl_system_calloc(size_t nitems, size_t size);

/**
 * @brief Re-allocate the memory
 *
 * @param[in]       nitems      source memory address
 * @param[in]       size        the size after re-allocate
 *
 * @return void
 */
void *tkl_system_realloc(void *ptr, size_t size);

/**
 * @brief Get system free heap size
 *
 * @param none
 *
 * @return heap size
 */
int tkl_system_get_free_heap_size(void);

#if defined(CONFIG_SPIRAM)
/**
 * @brief Alloc memory of PSRAM
 *
 * @param[in] size: memory size
 *
 * @note This API is used to alloc memory of PSRAM.
 *
 * @return the memory address malloced
 */
void *tkl_system_psram_malloc(size_t size);

/**
 * @brief Free memory of PSRAM
 *
 * @param[in] ptr: memory point
 *
 * @note This API is used to free memory of PSRAM.
 *
 * @return void
 */
void tkl_system_psram_free(void *ptr);

/**
 * @brief Allocates memory for an array of elements in PSRAM and initializes all bytes to zero.
 *
 * This function allocates memory for an array of @p nitems elements, each of @p size bytes,
 * from the PSRAM (Pseudo Static RAM) region. The allocated memory is set to zero.
 *
 * @param[in] nitems Number of elements to allocate.
 * @param[in] size Size of each element in bytes.
 *
 * @return Pointer to the allocated memory on success, or NULL if the allocation fails.
 */
void *tkl_system_psram_calloc(size_t nitems, size_t size);

/**
 * @brief Reallocates memory in PSRAM (Pseudo Static RAM)
 *
 * This function changes the size of the memory block pointed to by ptr to size bytes.
 * The memory block is reallocated in PSRAM, which is external memory that provides
 * additional RAM capacity beyond the internal SRAM.
 *
 * @param ptr Pointer to the memory block to be reallocated. If NULL, the function
 *            behaves like tkl_system_psram_malloc(size)
 * @param size New size for the memory block in bytes. If 0 and ptr is not NULL,
 *             the memory block is freed and NULL is returned
 *
 * @return Pointer to the reallocated memory block, or NULL if the allocation fails
 *         or if size is 0. The returned pointer may be different from the original
 *         ptr if the memory block was moved to accommodate the new size
 *
 * @note The contents of the memory block are preserved up to the minimum of the
 *       old and new sizes. If the new size is larger, the additional memory is
 *       uninitialized
 * @note If reallocation fails, the original memory block remains unchanged
 * @note PSRAM typically has slower access speed compared to internal SRAM but
 *       provides larger capacity
 */
void *tkl_system_psram_realloc(void *ptr, size_t size);

/**
 * @brief Get PSRAM free heap size
 *
 * @param none
 *
 * @return PSRAM heap size
 */
int tkl_system_psram_get_free_heap_size(void);
#endif // CONFIG_SPIRAM

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
