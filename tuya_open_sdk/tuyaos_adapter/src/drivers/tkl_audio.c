#include "tkl_audio.h"

/**
* @brief ai init
*
* @param[in] pconfig: audio config
* @param[in] count: count of pconfig
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_init(TKL_AUDIO_CONFIG_T *pconfig, int count)
{
    return OPRT_OK;
}

/**
* @brief ai start
*
* @param[in] card: card number
* @param[in] chn: channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_start(int card, TKL_AI_CHN_E chn)
{
    return OPRT_OK;
}

/**
* @brief ai set mic volume
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] vol: mic volume,[0, 100]
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_set_vol(int card, TKL_AI_CHN_E chn, int vol)
{
    return OPRT_OK;
}

/**
* @brief ai get frame
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[out] pframe: audio frame, pframe->pbuf allocated by upper layer application
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_get_frame(int card, TKL_AI_CHN_E chn, TKL_AUDIO_FRAME_INFO_T *pframe)
{
    return OPRT_OK;
}

/**
* @brief ai set vqe param
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] type: vqe type
* @param[in] pparam: vqe param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_set_vqe(int card, TKL_AI_CHN_E chn, TKL_AUDIO_VQE_TYPE_E type, TKL_AUDIO_VQE_PARAM_T *pparam)
{
    return OPRT_OK;
}

/**
* @brief ai get vqe param
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] type: vqe type
* @param[out] pparam: vqe param
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_get_vqe(int card, TKL_AI_CHN_E chn, TKL_AUDIO_VQE_TYPE_E type, TKL_AUDIO_VQE_PARAM_T *pparam)
{
    return OPRT_OK;
}

/**
* @brief ai stop
*
* @param[in] card: card number
* @param[in] chn: channel number
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_stop(int card, TKL_AI_CHN_E chn)
{
    return OPRT_OK;
}

/**
* @brief ai uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_uninit(void)
{
    return OPRT_OK;
}

/**
* @brief ao init
*
* @param[in] pconfig: audio config
* @param[in] count: config count
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_init(TKL_AUDIO_CONFIG_T *pconfig, int count, void **handle)
{
    return OPRT_OK;
}

/**
* @brief ao start
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[out] handle: handle of start
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_start(int card, TKL_AO_CHN_E chn, void *handle)
{
    return OPRT_OK;
}

/**
* @brief ao set volume
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] vol: mic volume,[0, 100]
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_set_vol(int card, TKL_AO_CHN_E chn, void *handle, int vol)
{
    return OPRT_OK;
}

/**
* @brief ao get volume
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] vol: mic volume,[0, 100]
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_get_vol(int card, TKL_AO_CHN_E chn, void *handle, int *vol)
{
    return OPRT_OK;
}

/**
* @brief ao output frame
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] handle: handle of start
* @param[in] pframe: output frame
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_put_frame(int card, TKL_AO_CHN_E chn, void *handle, TKL_AUDIO_FRAME_INFO_T *pframe)
{
    return OPRT_OK;
}

/**
* @brief ao stop
*
* @param[in] card: card number
* @param[in] chn: channel number
* @param[in] handle: handle of start
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_stop(int card, TKL_AO_CHN_E chn, void *handle)
{
    return OPRT_OK;
}

/**
* @brief ao uninit
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ao_uninit(void *handle)
{
    return OPRT_OK;
}

/**
* @brief audio input detect start
*
* @param[in] card: card number
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_detect_start(int card, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_OK;
}

/**
* @brief audio input detect stop
*
* @param[in] card: card number
* @param[in] type: detect type
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_detect_stop(int card, TKL_MEDIA_DETECT_TYPE_E type)
{
    return OPRT_OK;
}

/**
* @brief audio detect get result
*
* @param[in] card: card number
* @param[in] type: detect type
* @param[out] presult: audio detect result
*
* @return OPRT_OK on success. Others on error, please refer to tkl_error_code.h
*/
OPERATE_RET tkl_ai_detect_get_result(int card, TKL_MEDIA_DETECT_TYPE_E type, TKL_AUDIO_DETECT_RESULT_T *presult)
{
    return OPRT_OK;
}

OPERATE_RET tkl_ao_clear_buffer(int card, TKL_AO_CHN_E chn)
{
    return OPRT_OK;
}
