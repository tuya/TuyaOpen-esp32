// Copyright 2015-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef _VAD_H_
#define _VAD_H_

#include "wb_vad_c.h"

#define VAD_DEBUG_PRTF
#ifdef VAD_DEBUG_PRTF
#define VAD_PRTF                                 os_printf
#else
#define VAD_PRTF
#endif //VAD_DEBUG_PRTF

#define VAD_RET_SUCCESS                          (0)
#define VAD_RET_FAILURE                          (-1)

#define VAD_OCCUR_NULL                           (0)
#define VAD_OCCUR_DONE                           (1)

//#define HEAD_SPEECH_COUNT                        (8)
//#define TAIL_NOISE_COUNT                         (16)

//extern int head_speech_count;
//extern int tail_noise_count;

//#define WB_FRAME_LEN                             (FRAME_LEN)

#define WB_FRAME_TYPE_NULL                      (2)
#define WB_FRAME_TYPE_SPEECH                    (1)
#define WB_FRAME_TYPE_NOISE                     (0)

void vad_pre_start(void);
int vad_entry(short samples[], int len);

int wb_vad_enter(int start_threshold,int end_threshold,int frame_len, int pre_frame);
int wb_vad_entry(char *buffer, int len);
void wb_vad_deinit(void);
int wb_vad_get_frame_len(void);
void wb_estimate_init(void);

#endif // _VAD_H_
// eof

