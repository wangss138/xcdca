#ifndef __AUDIO_SR_H__
#define __AUDIO_SR_H__
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_heap_caps.h"
#include "freertos/task.h"
#include "esp_task.h"
#include "freertos/ringbuf.h"
#include"bsp_es8311.h"
#include"Com_status.h"
typedef struct audio_sr audio_sr_t;

audio_sr_t *audio_sr_Init(void);
void audio_sr_Start(audio_sr_t *audio_sr);
void audio_sr_set_waknet_callback(audio_sr_t *audio_sr, void (*callback)(void *), void *args);
void audio_sr_set_vad_callback(audio_sr_t *audio_sr, void (*callback)(void *, vad_state_t), void *args);

void audio_sr_set_ringbuffer(audio_sr_t *audio_sr,RingbufHandle_t ringbuffer);
void audio_sr_reset_waknet(audio_sr_t *audio_sr);
#endif /* __AUDIO_SR_H__ */