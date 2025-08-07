#ifndef __BSP_H__
#define __BSP_H__
#include "bsp_es8311.h"
#include "audio_process.h"
#include "bsp_wifi.h"
#include "bsp_ws2812.h"
#include "bsp_nvs.h"
#include "time.h"
#include "bsp_lcd.h"
#include "bsp_display.h"
#include "bsp_ledc.h"
#include"pro_mqtt.h"
void bsp_init();

char *bsp_get_mac_addr();

char *bsp_get_uuid();

#endif /* __BSP_H__ */