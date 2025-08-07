#include "audio_sr.h"

struct audio_sr
{
    void (*wakenet_callback)(void *);
    void *wakenet_args;
    void (*vad_callback)(void *, vad_state_t);
    void *vad_args;
    esp_afe_sr_iface_t *afe_handle;
    esp_afe_sr_data_t *afe_data;
    uint8_t is_waked;
    vad_state_t last_vad_state;
    RingbufHandle_t ringbuffer;
};
// 给afe输入数据任务
void audio_feed_task(void *args)
{
    MY_LOGE("feed任务开始调度");
    audio_sr_t *audio_sr = (audio_sr_t *)args;

    esp_afe_sr_iface_t *afe_handle = audio_sr->afe_handle;
    esp_afe_sr_data_t *afe_data = audio_sr->afe_data;
    // 获取单通道数据数据大小
    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
    // 获取通道数
    int feed_nch = afe_handle->get_feed_channel_num(afe_data);
    // 计算单次数据大小
    size_t size = feed_chunksize * feed_nch * sizeof(int16_t);

    // int sample_rate = afe_handle->get_samp_rate;
    // // 计算每个块的时间（毫秒）
    // float chunk_time_ms = (float)feed_chunksize * 1000.0 / sample_rate;
    // // 转换为FreeRTOS tick数（四舍五入）
    // const TickType_t xDelay = (chunk_time_ms + 0.5) / portTICK_PERIOD_MS;
    // // 如果计算为0，则至少为1（但通常不会为0）
    // TickType_t delay_ticks = xDelay > 0 ? xDelay : 1;

    int16_t *feed_buff = (int16_t *)malloc(size);
    while (1)
    {
        bsp_es8311_Read_from_Mic((void *)feed_buff, size);
        // afe输入数据
        afe_handle->feed(afe_data, feed_buff);
        vTaskDelay(10);
    }
}
// 从afe取出数据任务
void audio_fetch_task(void *args)
{
    MY_LOGE("fetch任务开始调度");
    audio_sr_t *audio_sr = (audio_sr_t *)args;
    esp_afe_sr_iface_t *afe_handle = audio_sr->afe_handle;
    esp_afe_sr_data_t *afe_data = audio_sr->afe_data;

    // 获取单通道数据数据大小
    int feed_chunksize = afe_handle->get_feed_chunksize(afe_data);
    // 获取通道数
    int feed_nch = afe_handle->get_feed_channel_num(afe_data);
    // 计算单次数据大小
    size_t size = feed_chunksize * feed_nch * sizeof(int16_t);
    while (1)
    {
        // 获取语音识别结果
        afe_fetch_result_t *result = afe_handle->fetch(afe_data);
        // 获取音频结果
        int16_t *processed_audio = result->raw_data;
        // 语音识别状态
        vad_state_t vad_state = result->vad_state;
        // 唤醒状态
        wakenet_state_t wakeup_state = result->wakeup_state;

        if (wakeup_state == WAKENET_DETECTED)
        {
            audio_sr->is_waked = 1; // 用于区分哪些语音需要保留
            if (audio_sr->wakenet_callback)
            {
                audio_sr->wakenet_callback(audio_sr->wakenet_args);
            }
        }
        if (audio_sr->is_waked)
        {
            // 语音识别状态发生变化时，调用vad回调函数
            if (vad_state != audio_sr->last_vad_state)
            {
                audio_sr->last_vad_state = vad_state;
                if (audio_sr->vad_callback)
                {
                    audio_sr->vad_callback(audio_sr->vad_args, vad_state);
                }
            }
            // 处于唤醒状态时有人说话，保留语音
            if (vad_state == VAD_SPEECH && state == LISTEN)
            {
                // MY_LOGE("语音已保留");
                xRingbufferSend(audio_sr->ringbuffer, processed_audio, size, portMAX_DELAY);
            }
        }

        vTaskDelay(10);
    }
}

audio_sr_t *audio_sr_Init(void)
{
    audio_sr_t *audio_sr = malloc(sizeof(audio_sr_t));
    audio_sr->is_waked = 0;
    audio_sr->last_vad_state = VAD_SILENCE;
    // 初始化afe
    // afe配置
    srmodel_list_t *models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    afe_config->aec_init = false;
    afe_config->se_init = false;
    afe_config->ns_init = false;
    afe_config->agc_init = false;

    afe_config->vad_init = true;
    afe_config->vad_mode = VAD_MODE_1;
    // afe_config->vad_min_speech_ms = 200;

    afe_config->wakenet_init = true;
    afe_config->wakenet_mode = DET_MODE_95;
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    // 创建afe实例
    // 获取句柄
    audio_sr->afe_handle = esp_afe_handle_from_config(afe_config);
    // 创建实例
    audio_sr->afe_data = audio_sr->afe_handle->create_from_config(afe_config);

    return audio_sr;
}

void audio_sr_Start(audio_sr_t *audio_sr)
{
    // xTaskCreate(audio_feed_task, "feed_task", 32 * 1024, audio_sr, 5, NULL);
    // xTaskCreate(audio_fetch_task, "fetch_task", 32 * 1024, audio_sr, 5, NULL);
    // xTaskCreateWithCaps(audio_feed_task, "feed_task", 32 * 1024, audio_sr, 5, NULL, MALLOC_CAP_SPIRAM);
    // xTaskCreateWithCaps(audio_fetch_task, "fetch_task", 32 * 1024, audio_sr, 5, NULL, MALLOC_CAP_SPIRAM);
    xTaskCreatePinnedToCoreWithCaps(audio_feed_task, "feed_task", 32 * 1024, audio_sr, 5, NULL, 1, MALLOC_CAP_SPIRAM);
    xTaskCreatePinnedToCoreWithCaps(audio_fetch_task, "fetch_task", 32 * 1024, audio_sr, 5, NULL, 1, MALLOC_CAP_SPIRAM);
}
void audio_sr_set_waknet_callback(audio_sr_t *audio_sr, void (*callback)(void *), void *args)
{
    audio_sr->wakenet_callback = callback;
    audio_sr->wakenet_args = args;
}
void audio_sr_set_vad_callback(audio_sr_t *audio_sr, void (*callback)(void *, vad_state_t), void *args)
{
    audio_sr->vad_callback = callback;
    audio_sr->vad_args = args;
}
// 设置输出环形缓冲区
void audio_sr_set_ringbuffer(audio_sr_t *audio_sr, RingbufHandle_t ringbuffer)
{
    audio_sr->ringbuffer = ringbuffer;
}
void audio_sr_reset_waknet(audio_sr_t *audio_sr)
{
    audio_sr->is_waked = 0;
}