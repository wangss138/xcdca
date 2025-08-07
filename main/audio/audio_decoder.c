#include "audio_decoder.h"

struct audio_decoder
{
    RingbufHandle_t input_ringbuffer;
    RingbufHandle_t out_ringbuffer;
    esp_audio_dec_handle_t decoder;
    uint8_t is_running;
};
void audio_decode_task(void *args);

audio_devoder_t *audio_decoder_Init(void)
{
    audio_devoder_t *audio_decoder = (audio_devoder_t *)malloc(sizeof(audio_devoder_t));
    audio_decoder->is_running = 0;
    // 注册opus解码器
    esp_opus_dec_register();
    // ​OPUS 解码器的专用配置结构体
    esp_opus_dec_cfg_t opus_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,
        .channel = ESP_AUDIO_MONO,
        .frame_duration = ESP_OPUS_DEC_FRAME_DURATION_60_MS,
        // 禁止使用OPUS 前向纠错（FEC）功能
        .self_delimited = false,
    };
    // ​通用音频解码器的配置结构体
    esp_audio_dec_cfg_t dec_cfg = {
        .type = ESP_AUDIO_TYPE_OPUS,
        .cfg = &opus_cfg,
        .cfg_sz = sizeof(esp_opus_dec_cfg_t),
    };
    // 启动解码器
    esp_audio_dec_open(&dec_cfg, &audio_decoder->decoder);
    return audio_decoder;
}

void audio_decoder_Start(audio_devoder_t *audio_decoder)
{
    audio_decoder->is_running = 1;
    // 选择外部空间创建任务
    // 确保任务运行在特定核心（如音频处理通常固定在某个核心）
    xTaskCreatePinnedToCoreWithCaps(audio_decode_task, "decode_task", 32 * 1024, audio_decoder, 5, NULL, 0, MALLOC_CAP_SPIRAM);
}
void audio_decoder_Stop(audio_devoder_t *audio_decoder)
{
    audio_decoder->is_running = 0;
}

void audio_decoder_set_input_ringbuffer(audio_devoder_t *audio_decoder, RingbufHandle_t ringbuffer)
{
    audio_decoder->input_ringbuffer = ringbuffer;
}
void audio_decoder_set_out_ringbuffer(audio_devoder_t *audio_decoder, RingbufHandle_t ringbuffer)
{
    audio_decoder->out_ringbuffer = ringbuffer;
}

void audio_decode_task(void *args)
{
    MY_LOGE("opus解码任务开始调度");
    audio_devoder_t *audio_decoder = (audio_devoder_t *)args;

    // int ret = 0;
    esp_audio_dec_in_raw_t raw = {0};
    // 输出
    int out_size = 16000 * 16 / 8 / 1000 * 60;
    uint8_t *out_buf = (uint8_t *)malloc(out_size);
    // 输出帧
    esp_audio_dec_out_frame_t out_frame = {
        .buffer = out_buf,
        .len = out_size,
    };
    while (audio_decoder->is_running)
    {
        size_t in_size = 0;
        // 从输入缓冲区读取数据
        void *data = xRingbufferReceive(audio_decoder->input_ringbuffer, &in_size, portMAX_DELAY);
        // MY_LOGE("555555555555555555555555555");

        // 输入帧
        raw.buffer = data;
        raw.len = in_size;
        while (raw.len > 0)
        {
            // 解码
            esp_audio_dec_process(audio_decoder->decoder, &raw, &out_frame);
            // MY_LOGE("ssssssssssssssssssssssssssssssssssss");

            // 将解码器输出结果写入输出缓冲区
            xRingbufferSend(audio_decoder->out_ringbuffer, (void *)out_frame.buffer, out_frame.decoded_size, portMAX_DELAY);
            // MY_LOGE("adsfgsgs");
            // 将已经解码的数据指针后移
            // consumed  :由解码器在调用esp_audio_dec_process()后填充。表示本次解码实际消耗的输入数据字节数。
            raw.len -= raw.consumed;
            raw.buffer += raw.consumed;
        }
        // 释放资源
        vRingbufferReturnItem(audio_decoder->input_ringbuffer, data);
        vTaskDelay(10);
    }
    // 释放资源
    free(out_buf);
    // 自杀
    vTaskDelete(NULL);
}