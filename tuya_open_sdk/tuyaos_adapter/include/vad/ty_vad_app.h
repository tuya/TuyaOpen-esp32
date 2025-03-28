#ifndef __VAD_APP_H_
#define __VAD_APP_H_
// #include <os/os.h>

typedef enum {
	TY_VAD_FLAG_VAD_NONE = 0,
	TY_VAD_FLAG_VAD_START,
	TY_VAD_FLAG_VAD_END
} ty_vad_flag_e;

typedef struct {
    uint16_t start_threshold_ms;
    uint16_t end_threshold_ms;
    uint16_t silence_threshold_ms;
    uint16_t sample_rate;
    uint16_t channel;
    uint16_t vad_frame_duration;
} ty_vad_config_t;

int ty_vad_app_init(const ty_vad_config_t *config);
uint8_t ty_vad_app_is_init(void);
int ty_vad_app_start(void);
int ty_vad_app_stop(void);

int ty_vad_frame_put(unsigned char *data, unsigned int size);

uint8_t ty_get_vad_work_status(void);
uint8_t ty_get_vad_flag(void);
uint8_t ty_get_vad_silence_flag(void);
void ty_clear_vad_silence_flag(void);

void ty_set_mic_param(int sample_rate, int channel);
void ty_set_vad_threshold(int start_threshold_ms, int end_threshold_ms, int silence_threshold_ms);
int ty_vad_task_send_msg(uint16_t cmd, uint8_t* buf, uint16_t data_len);

#endif
