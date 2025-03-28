#include "tkl_video_in.h"

/**
* @brief vi init
*
* @param[in] pconfig: vi config
* @param[in] count: count of pconfig
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_init(TKL_VI_CONFIG_T *pconfig, int count)
{
    return OPRT_OK;
}

/**
* @brief vi set mirror and flip
*
* @param[in] chn: vi chn
* @param[in] flag: mirror and flip value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_set_mirror_flip(TKL_VI_CHN_E chn, TKL_VI_MIRROR_FLIP_E flag)
{
    return OPRT_OK;
}

/**
* @brief vi get mirror and flip
*
* @param[in] chn: vi chn
* @param[out] flag: mirror and flip value
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_get_mirror_flip(TKL_VI_CHN_E chn, TKL_VI_MIRROR_FLIP_E *flag)
{
    return OPRT_OK;
}

/**
* @brief vi uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_uninit(TKL_VI_CAMERA_TYPE_E device_type)
{
    return OPRT_OK;
}

/**
* @brief  set sensor reg value
*
* @param[in] chn: vi chn
* @param[in] pparam: reg info
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_sensor_reg_set( TKL_VI_CHN_E chn, TKL_VI_SENSOR_REG_CONFIG_T *parg)
{
    return OPRT_OK;
}

/**
* @brief  get sensor reg value
*
* @param[in] chn: vi chn
* @param[in] pparam: reg info
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_sensor_reg_get( TKL_VI_CHN_E chn, TKL_VI_SENSOR_REG_CONFIG_T *parg)
{
    return OPRT_OK;
}

/**
* @brief detect init
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[in] pconfig: config
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_init(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_CONFIG_T *p_config)
{
    return OPRT_OK;
}

/**
* @brief detect start
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_start(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_OK;
}

/**
* @brief detect stop
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_stop(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_OK;
}

/**
* @brief get detection results
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[out] presult: detection results
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_get_result(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_RESULT_T *presult)
{
    return OPRT_OK;
}

/**
* @brief set detection param
*
* @param[in] chn: vi chn
* @param[in] type: detect type
* @param[in] pparam: detection param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_set(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type, TKL_VI_DETECT_PARAM_T *pparam)
{
    return OPRT_OK;
}

/**
* @brief detect uninit
*
* @param[in] chn: vi chn
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_detect_uninit(TKL_VI_CHN_E chn, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_OK;
}

/**
* @brief set video power control pin
*
* @param[in] chn: vi chn
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_vi_get_power_info(uint8_t device_type, uint8_t *io, uint8_t *active)
{
    return OPRT_OK;
}
