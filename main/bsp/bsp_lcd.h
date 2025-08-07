#ifndef __BSP_LCD_H__
#define __BSP_LCD_H__

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "lvgl.h"
#include"Com_Debug.h"
#define LCD_PIXEL_CLOCK_HZ     (20 * 1000 * 1000)
#define LCD_BK_LIGHT_ON_LEVEL  1
#define LCD_BK_LIGHT_OFF_LEVEL !LCD_BK_LIGHT_ON_LEVEL

#define PIN_NUM_SCLK           47
#define PIN_NUM_MOSI           48
#define PIN_NUM_MISO           18
#define PIN_NUM_LCD_DC         45

#define PIN_NUM_LCD_RST        16
#define PIN_NUM_LCD_CS         21
#define PIN_NUM_BK_LIGHT       40
// #define PIN_NUM_TOUCH_CS       15

#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8
void bsp_lcd_init();
void bsp_lcd_set_close();


#endif /* __BSP_LCD_H__ */