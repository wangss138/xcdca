#ifndef __PRO_MQTT_H__
#define __PRO_MQTT_H__

#include "Com_Debug.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"
#include"cJSON.h"

void pro_mqtt_init();
char *pro_mqtt_create_speed_json(int speed_value,int status);
void pro_mqtt_publish(const char * data,int len);
#endif /* __PRO_MQTT_H__ */