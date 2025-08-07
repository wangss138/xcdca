#ifndef __BSP_WS2812_H__
#define __BSP_WS2812_H__

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "Com_Debug.h"
#include "esp_err.h"

//禁用DMA通道驱使灯带
#define LED_STRIP_USE_DMA  0
//灯带数量
#define LED_STRIP_LED_COUNT 2
#define LED_STRIP_MEMORY_BLOCK_WORDS 0  //由驱动自动选择合适的内存块大小


#define LED_STRIP_GPIO_PIN  46
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)

void bsp_ws2812_Init();
void bsp_ws2812_led_open();
void bsp_ws2812_led_close();
#endif /* __bsp_WS2812_H__ */