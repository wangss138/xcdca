#ifndef __AUDIO_DECODER_H__
#define __AUDIO_DECODER_H__

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "unity.h"
#include "esp_system.h"
#include "esp_audio_dec_default.h"
#include "esp_audio_dec.h"
#include "Com_Debug.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

typedef struct audio_decoder audio_devoder_t;

audio_devoder_t *audio_decoder_Init(void);

void audio_decoder_Start(audio_devoder_t *audio_decoder);
void audio_decoder_Stop(audio_devoder_t *audio_decoder);

void audio_decoder_set_input_ringbuffer(audio_devoder_t *audio_decoder, RingbufHandle_t ringbuffer);
void audio_decoder_set_out_ringbuffer(audio_devoder_t *audio_decoder, RingbufHandle_t ringbuffer);

#endif /* __AUDIO_DECODER_H__ */