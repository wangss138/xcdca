#ifndef __BSP_DISPLAY_H__
#define __BSP_DISPLAY_H__

#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include"Com_Debug.h"
#include"bsp_lcd.h"
#include"font_puhui.h"
#include"font_emoji.h"
#include"string.h"
#define LCD_H_RES   (240)
#define LCD_V_RES   (320)


void bsp_lvgl_init();
void bsp_display_init();
void bsp_display_set_text(char *text);
void bsp_display_set_emotion(char *emotion);

#endif /* __BSP_DISPLAY_H__ */