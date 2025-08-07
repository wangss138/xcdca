#ifndef __AUDIO_PROCESS_H__
#define __AUDIO_PROCESS_H__
#include "audio/audio_sr.h"
#include "bsp_es8311.h"
#include "bsp_wifi.h"
#include "bsp_ws2812.h"
#include "audio_encoder.h"
#include "audio_decoder.h"



typedef struct audio_process audio_process_t;
audio_process_t *audio_process_Init();

void audio_process_Start(audio_process_t *audio_process);
void *audio_process_read_buffer(audio_process_t *audio_process, size_t *len);
void audio_process_write_buffer(audio_process_t *audio_process, uint8_t *data, size_t len);

void audio_process_set_wakenet_callback(audio_process_t *audio_process, void (*callback)(void *));
void audio_process_set_vad_callback(audio_process_t *audio_process, void (*callback)(void *, vad_state_t));
void audio_process_reset_waknet(audio_process_t *audio_process);

#endif /* __AUDIO_PROCESS_H__ */