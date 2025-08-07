
#include "bsp_es8311.h"

static i2s_chan_handle_t mic_handle;     // mic输入通道
static i2s_chan_handle_t speaker_handle; // 扬声器输出通道
static esp_codec_dev_handle_t codec_dev;

// 开启设备
static esp_codec_dev_sample_info_t fs = {
    .sample_rate = 16000,
    .channel = 1,
    .bits_per_sample = 16,
    .channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(0),
};
static void es8311_i2c_init()
{

    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
        .sda_io_num = ES8311_I2C_SDA_PIN,
        .scl_io_num = ES8311_I2C_SCL_PIN,
    };

    i2c_param_config(I2C_NUM_0, &i2c_cfg);
    i2c_driver_install(I2C_NUM_0, i2c_cfg.mode, 0, 0, 0);
}

static void es8311_i2s_init()
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_std_config_t std_cfg = {
        // 音频采样率
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        // 数据槽配置
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(16, I2S_SLOT_MODE_MONO),
        // 配置gpio引脚
        .gpio_cfg = {
            .mclk = ES8311_I2S_MCK_PIN,
            .bclk = ES8311_I2S_BCK_PIN,
            .ws = ES8311_I2S_DATA_WS_PIN,
            .dout = ES8311_I2S_DATA_OUT_PIN,
            .din = ES8311_I2S_DATA_IN_PIN,
        },
    };

    i2s_new_channel(&chan_cfg, &speaker_handle, &mic_handle);
    // 初始化
    i2s_channel_init_std_mode(speaker_handle, &std_cfg);
    i2s_channel_init_std_mode(mic_handle, &std_cfg);
    i2s_channel_enable(speaker_handle);
    i2s_channel_enable(mic_handle);
}

static void es8311_dev_Init()
{
    audio_codec_i2s_cfg_t i2s_cfg = {

        .rx_handle = mic_handle,
        .tx_handle = speaker_handle,
        .port = I2S_NUM_0,
    };
    const audio_codec_data_if_t *data_if = audio_codec_new_i2s_data(&i2s_cfg);
    audio_codec_i2c_cfg_t i2c_cfg = {
        .addr = ES8311_CODEC_DEFAULT_ADDR,
        .port = I2C_NUM_0,
    };

    const audio_codec_ctrl_if_t *out_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    TEST_ASSERT_NOT_NULL(out_ctrl_if);

    const audio_codec_gpio_if_t *gpio_if = audio_codec_new_gpio();
    TEST_ASSERT_NOT_NULL(gpio_if);
    // New output codec interface
    es8311_codec_cfg_t es8311_cfg = {
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .ctrl_if = out_ctrl_if,
        .gpio_if = gpio_if,
        .pa_pin = ES8311_PA_EN_PIN,
        .use_mclk = true,
    };
    const audio_codec_if_t *out_codec_if = es8311_codec_new(&es8311_cfg);
    TEST_ASSERT_NOT_NULL(out_codec_if);

    // New output codec device

    esp_codec_dev_cfg_t dev_cfg = {
        .codec_if = out_codec_if,
        .data_if = data_if,
        .dev_type = ESP_CODEC_DEV_TYPE_IN_OUT,
    };
    // 创建设备对象
    codec_dev = esp_codec_dev_new(&dev_cfg);
    // New input codec device
    // 7.设置输入增益以及输出音量
    esp_codec_dev_set_out_vol(codec_dev, 30.0);
    esp_codec_dev_set_in_gain(codec_dev, 10.0);
}
void bsp_es8311_Init()
{
    es8311_i2c_init();
    es8311_i2s_init();
    es8311_dev_Init();
}
void bsp_es8311_Write_to_Speaker(void *datas, size_t len)
{
    if (codec_dev != NULL && len > 0)
    {

        esp_codec_dev_write(codec_dev, datas, (int)len);
    }
}
void bsp_es8311_Read_from_Mic(void *datas, size_t len)
{
    if (codec_dev != NULL && len > 0)
    {
        esp_codec_dev_read(codec_dev, datas, (int)len);
    }
}
// 开启es8311设备
void bsp_es8311_Open()
{
    esp_err_t ret = esp_codec_dev_open(codec_dev, &fs);
    if (ret != ESP_OK)
    {
        MY_LOGE("Failed to open codec device: %s", esp_err_to_name(ret));
        // 添加一个全局标志或错误处理，确保在设备未打开时不尝试写入
    }
    else
    {
        MY_LOGI("Codec device opened successfully.");
    }
}
// 关闭es8311设备
void bsp_es8311_Close()
{
    esp_codec_dev_close(codec_dev);
}

void bsp_es8311_set_vol(int vol)
{
    esp_codec_dev_set_out_vol(codec_dev, vol);
}
void bsp_es8311_set_mute(bool mute)
{
    esp_codec_dev_set_out_mute(codec_dev, mute);
}