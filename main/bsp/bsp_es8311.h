#ifndef __BSP_ES8311_H__
#define __BSP_ES8311_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Codec configuration by ESP32S3_KORVO2_V3
 */
#define ES8311_I2C_SDA_PIN GPIO_NUM_0
#define ES8311_I2C_SCL_PIN GPIO_NUM_1

#define ES8311_I2S_BCK_PIN GPIO_NUM_2
#define ES8311_I2S_MCK_PIN GPIO_NUM_3
#define ES8311_I2S_DATA_IN_PIN GPIO_NUM_4
#define ES8311_I2S_DATA_OUT_PIN GPIO_NUM_6
#define ES8311_I2S_DATA_WS_PIN GPIO_NUM_5

#define ES8311_PA_EN_PIN GPIO_NUM_7

#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "soc/soc_caps.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "unity.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include <protocomm.h>     // 确锟斤拷锟斤拷锟斤拷 protocomm.h 锟斤拷识锟斤拷 PROTOCOMM_TRANSPORT_BLE_EVENT 锟斤拷 PROTOCOMM_SECURITY_SESSION_EVENT
#include <protocomm_ble.h> // BLE 锟斤拷锟斤拷锟斤拷要
#include "Com_Debug.h"
#ifdef __cplusplus
}
#endif
// #define ES8311_CODEC_DEFAULT_ADDR (0x30)
void bsp_es8311_Init();
void bsp_es8311_Write_to_Speaker(void *datas, size_t len);
void bsp_es8311_Read_from_Mic(void *datas, size_t len);
void bsp_es8311_Open();
void bsp_es8311_Close();

void bsp_es8311_set_vol(int vol);
void bsp_es8311_set_mute(bool mute);

#endif /* __bsp_ES8311_H__ */
