/**
 * @file tuyaopen_license.h
 * @brief tuyaopen_license module is used to 
 * @version 0.1
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#ifndef __TUYAOPEN_LICENSE_H__
#define __TUYAOPEN_LICENSE_H__

#include "tuya_cloud_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/

/**
 * @brief Write license data
 * @param[in] data Pointer to license data
 * @param[in] data_len Length of license data
 * @return 0 on success, non-zero on failure
 */
int tuyaopen_license_write(const char *data, const uint32_t data_len);

/**
 * @brief Read license data
 * @param[out] data Pointer to store license data address
 * @param[out] data_len Pointer to store license data length
 * @return 0 on success, non-zero on failure
 */
int tuyaopen_license_read(char **data, uint32_t *data_len);

#ifdef __cplusplus
}
#endif

#endif /* __TUYAOPEN_LICENSE_H__ */
