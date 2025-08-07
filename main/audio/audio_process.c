#include "audio_process.h"

struct audio_process
{
    audio_sr_t *audio_sr;
    audio_encoder_t *audio_encoder;
    audio_devoder_t *audio_decoder;

    RingbufHandle_t encoder_inbuf;
    RingbufHandle_t encoder_outbuf;
    RingbufHandle_t decoder_inbuf;
    RingbufHandle_t decoder_outbuf;
};
void audio_process_s_task(void *args);

audio_process_t *audio_process_Init()
{
    audio_process_t *audio_process = (audio_process_t *)malloc(sizeof(audio_process_t));
    assert(audio_process);
    audio_process->audio_sr = audio_sr_Init();

    audio_process->audio_encoder = audio_encode_Init();

    audio_process->audio_decoder = audio_decoder_Init();

    // 创建4个临时缓冲区
    audio_process->encoder_inbuf = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_BYTEBUF, MALLOC_CAP_SPIRAM);
    audio_process->encoder_outbuf = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    audio_process->decoder_inbuf = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);
    audio_process->decoder_outbuf = xRingbufferCreateWithCaps(16 * 1024, RINGBUF_TYPE_NOSPLIT, MALLOC_CAP_SPIRAM);

    // 设置缓冲区
    audio_sr_set_ringbuffer(audio_process->audio_sr, audio_process->encoder_inbuf);

    audio_encode_input_buffer(audio_process->audio_encoder, audio_process->encoder_inbuf);
    audio_encode_out_buffer(audio_process->audio_encoder, audio_process->encoder_outbuf);

    audio_decoder_set_input_ringbuffer(audio_process->audio_decoder, audio_process->decoder_inbuf);
    audio_decoder_set_out_ringbuffer(audio_process->audio_decoder, audio_process->decoder_outbuf);

    return audio_process;
}

void audio_process_Start(audio_process_t *audio_process)
{
    xTaskCreatePinnedToCoreWithCaps(audio_process_s_task, "process_task", 32 * 1024, audio_process, 5, NULL, 0, MALLOC_CAP_SPIRAM);
    audio_decoder_Start(audio_process->audio_decoder);
    audio_encode_Start(audio_process->audio_encoder);
    audio_sr_Start(audio_process->audio_sr);
}

void *audio_process_read_buffer(audio_process_t *audio_process, size_t *len)
{
    size_t out_size = 0;
    void *datas = xRingbufferReceive(audio_process->encoder_outbuf, &out_size, portMAX_DELAY);
    // MY_LOGE("编码器输出缓冲区读数据");
    // MY_LOGE("out_asize============%d",out_size);
    uint8_t *data = malloc(out_size);
    memcpy((void *)data, datas, out_size);
    *len = out_size;
    // 释放资源
    vRingbufferReturnItem(audio_process->encoder_outbuf, datas);

    return data;
}

void audio_process_write_buffer(audio_process_t *audio_process, uint8_t *data, size_t len)
{
    xRingbufferSend(audio_process->decoder_inbuf, (void *)data, len, portMAX_DELAY);
}

void audio_process_set_wakenet_callback(audio_process_t *audio_process, void (*callback)(void *))
{
    audio_sr_set_waknet_callback(audio_process->audio_sr, callback, audio_process->audio_sr);
}

void audio_process_set_vad_callback(audio_process_t *audio_process, void (*callback)(void *, vad_state_t))
{
    audio_sr_set_vad_callback(audio_process->audio_sr, callback, audio_process->audio_sr);
}

void audio_process_reset_waknet(audio_process_t *audio_process)
{
    audio_sr_reset_waknet(audio_process->audio_sr);
}

void audio_process_s_task(void *args)
{
    MY_LOGE("扬声器任务开始调度");
    // 强转
    audio_process_t *audio_process = (audio_process_t *)args;
    size_t out_size = 0;
    while (1)
    {

        // 接收解码器输出缓冲区
        void *data = xRingbufferReceive(audio_process->decoder_outbuf, &out_size, portMAX_DELAY);
        if (out_size > 0)
        {

            // 写入扬声器
            bsp_es8311_Write_to_Speaker(data, out_size);
            vRingbufferReturnItem(audio_process->decoder_outbuf, data);
            out_size = 0;
        }

        vTaskDelay(10);
    }
}