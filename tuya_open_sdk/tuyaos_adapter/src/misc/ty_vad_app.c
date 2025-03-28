#include "lib/vad.h"
#include "tuya_cloud_types.h"
#include "ty_vad_app.h"


int ty_vad_app_init(const ty_vad_config_t *config)
{
    return 0;
}

uint8_t ty_vad_app_is_init(void)
{
    return 0;
}

int ty_vad_app_start(void)
{
    return 0;
}

int ty_vad_app_stop(void)
{
    return 0;
}

int ty_vad_frame_put(unsigned char *data, unsigned int size)
{
    return 0;
}

uint8_t ty_get_vad_work_status(void)
{
    return 0;
}

uint8_t ty_get_vad_flag(void)
{
    return 0;
}

uint8_t ty_get_vad_silence_flag(void)
{
    return 0;
}

void ty_clear_vad_silence_flag(void)
{
}

void ty_set_mic_param(int sample_rate, int channel)
{
}

void ty_set_vad_threshold(int start_threshold_ms, int end_threshold_ms, int silence_threshold_ms)
{
}

int ty_vad_task_send_msg(uint16_t cmd, uint8_t* buf, uint16_t data_len)
{
    return 0;
}
