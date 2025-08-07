#ifndef __PRO_WEBSOCKET_H__
#define __PRO_WEBSOCKET_H__

#include <stdio.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_crt_bundle.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include"Com_Debug.h"
#include "esp_log.h"
#include "esp_websocket_client.h"
#include "esp_event.h"
#include <cJSON.h>
#include"bsp.h"
#include"Com_status.h"
//定义文本和语音回调函数

typedef void (*bin_callback)(const char *bin_data,size_t len);
typedef void (*text_callback)(const char *text_data,size_t len);
void pro_websocket_init();
void pro_websocket_start();
void pro_websocket_close();
void pro_websocket_hello();

void pro_websocket_send_wakenet_word();
void pro_websocket_send_start_listen();

void pro_websocket_send_stop_listen();
//中止消息
void pro_websocket_send_abort();
//发送音频数据
void pro_websocket_send_opus( void *data,size_t len);

void pro_websocket_bin_callback(bin_callback bin_cb);
void pro_websocket_text_callback(text_callback text_cb);

void pro_websocket_send_equipment_state();
void pro_websocket_send_iot();
#endif /* __PRO_WEBSOCKET_H__ */