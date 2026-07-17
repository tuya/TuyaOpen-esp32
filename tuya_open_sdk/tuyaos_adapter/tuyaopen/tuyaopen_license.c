/**
 * @file tuyaopen_license.c
 * @brief tuyaopen_license module is used to 
 * @version 0.1
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include "tuyaopen_license.h"

/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/


/***********************************************************
***********************function define**********************
***********************************************************/

/**
 * @brief Write license data
 * @param[in] data Pointer to license data
 * @param[in] data_len Length of license data
 * @return OPRT_NOT_SUPPORTED - operation not supported
 */
int tuyaopen_license_write(const char *data, const uint32_t data_len)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief Read license data
 * @param[out] data Pointer to store license data address
 * @param[out] data_len Pointer to store license data length
 * @return OPRT_NOT_SUPPORTED - operation not supported
 */
int tuyaopen_license_read(char **data, uint32_t *data_len)
{
    return OPRT_NOT_SUPPORTED;
}
