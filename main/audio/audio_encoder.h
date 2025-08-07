#ifndef __AUDIO_ENCODER_H__
#define __AUDIO_ENCODER_H__


#include <stdint.h>
#include "esp_audio_types.h"
#include "esp_audio_simple_dec.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"
#include "Com_Debug.h"
#include "esp_audio_enc.h"
#include "esp_opus_enc.h"


typedef struct audio_encoder audio_encoder_t;

audio_encoder_t *audio_encode_Init(void);
void audio_encode_Start(audio_encoder_t *audio_encoder);
void audio_encode_input_buffer(audio_encoder_t *audio_encoder, RingbufHandle_t ringbuffer);
void audio_encode_out_buffer(audio_encoder_t *audio_encoder, RingbufHandle_t ringbuffer);


#endif /* __AUDIO_ENCODER_H__ */