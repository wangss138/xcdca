#ifndef __PRO_HTTPS_H__
#define __PRO_HTTPS_H__

#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "esp_tls.h"
#include "bsp_cache.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "Com_Debug.h"
#include "bsp.h"
#include "cJSON.h"
#include "esp_app_desc.h"

#include <assert.h>
#include <sys/param.h>
#include "esp_ota_ops.h"
#include "esp_attr.h"
#include "sdkconfig.h"

#define MAX_HTTP_RECV_BUFFER 512
#define MAX_HTTP_OUTPUT_BUFFER 2048

void pro_https_send_reg();

#endif /* __PRO_HTTPS_H__ */