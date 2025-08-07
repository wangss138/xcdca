#include "audio_encoder.h"

struct audio_encoder
{
    RingbufHandle_t input_ringbuffer;
    RingbufHandle_t out_ringbuffer;
    // 声明一个编码器句柄
    esp_audio_enc_handle_t encoder;
};

void audio_encode_task(void *args);
static void audio_read_input_ringbuffer(RingbufHandle_t input_ringbuffer, size_t max_size, uint8_t *data)
{
    size_t len = 0;
    void *read_data = NULL;
    while (max_size)
    {
        // 从输入缓冲区读取数据
        read_data = xRingbufferReceiveUpTo(input_ringbuffer, &len, portMAX_DELAY, max_size);
        // 拼接直到设定长度接收完毕
        memcpy((void *)data, read_data, len);
        data += len;
        max_size -= len;

        vRingbufferReturnItem(input_ringbuffer, read_data);
        len = 0;
    }
}

audio_encoder_t *audio_encode_Init(void)
{
    audio_encoder_t *audio_encoder = (audio_encoder_t *)malloc(sizeof(audio_encoder_t));
    // 注册OPUS编码器
    esp_opus_enc_register();
    // 声明opus编码器的配置结构体
    esp_opus_enc_config_t opus_cfg = {
        .sample_rate = ESP_AUDIO_SAMPLE_RATE_16K,
        .channel = ESP_AUDIO_MONO,

        .bits_per_sample = ESP_AUDIO_BIT16,
        // 控制编码后的数据速率
        .bitrate = 32000,
        .frame_duration = ESP_OPUS_ENC_FRAME_DURATION_60_MS,
        .application_mode = ESP_OPUS_ENC_APPLICATION_VOIP,
        .complexity = 6,
        .enable_fec = true,
        .enable_dtx = false,
        .enable_vbr = true,

    };
    // 通用音频编码器配置结构体
    esp_audio_enc_config_t enc_cfg = {
        // 音频类型为opus
        .type = ESP_AUDIO_TYPE_OPUS,
        .cfg = &opus_cfg,
        .cfg_sz = sizeof(opus_cfg),
    };

    // 打开初始化编码器
    esp_audio_enc_open(&enc_cfg, &audio_encoder->encoder);
    return audio_encoder;
}

void audio_encode_Start(audio_encoder_t *audio_encoder)
{
    // xTaskCreateWithCaps(audio_encode_task,"encode_task",32*1024,audio_encoder,5,NULL,MALLOC_CAP_SPIRAM);
    // 选择外部空间创建任务
    // 确保任务运行在特定核心（如音频处理通常固定在某个核心）
    xTaskCreatePinnedToCoreWithCaps(audio_encode_task, "encode_task", 32 * 1024, audio_encoder, 5, NULL, 0, MALLOC_CAP_SPIRAM);
}
void audio_encode_input_buffer(audio_encoder_t *audio_encoder, RingbufHandle_t ringbuffer)
{
    audio_encoder->input_ringbuffer = ringbuffer;
}
void audio_encode_out_buffer(audio_encoder_t *audio_encoder, RingbufHandle_t ringbuffer)
{
    audio_encoder->out_ringbuffer = ringbuffer;
}

// 编码器任务
/*FreeRTOS 的任务创建函数（如 xTaskCreate()）
要求任务函数的签名必须是 void (*)(void *)*/
void audio_encode_task(void *args)
{
    MY_LOGE("编码器任务开始调度");
    audio_encoder_t *audio_encoder = (audio_encoder_t *)args;
    // 获取编码器所需要的输入(PCM)和输出(编码后)缓冲区大小
    int pcm_size = 0, raw_size = 0;
    esp_audio_enc_get_frame_size(audio_encoder->encoder, &pcm_size, &raw_size);
    uint8_t *pcm_data = (uint8_t *)malloc(pcm_size);
    uint8_t *raw_data = (uint8_t *)malloc(raw_size);
    // 输入输出帧结构体
    esp_audio_enc_in_frame_t in_frame = {
        .buffer = pcm_data,
        .len = pcm_size,
    };

    esp_audio_enc_out_frame_t out_frame = {
        .buffer = raw_data,
        .len = raw_size,
    };
    while (1)
    {
        // 获取输入数据(从输入缓冲区获取)
        audio_read_input_ringbuffer(audio_encoder->input_ringbuffer, pcm_size, pcm_data);
        // MY_LOGE("========%lu",in_frame.len);
        // 编码过程
        esp_audio_enc_process(audio_encoder->encoder, &in_frame, &out_frame);
        // MY_LOGE("========%lu",out_frame.encoded_bytes);
        // 将编码输出数据写入输出缓冲区
        xRingbufferSend(audio_encoder->out_ringbuffer, out_frame.buffer, out_frame.encoded_bytes, portMAX_DELAY);
        vTaskDelay(10);
    }
}