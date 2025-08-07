#include "bsp_wifi.h"

static EventGroupHandle_t wifi_event_group;
const int WIFI_CONNECTED_EVENT = BIT0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    static int retries;
    static int wifi_conn_retries;
    if (event_base == WIFI_PROV_EVENT)
    {
        switch (event_id)
        {
        case WIFI_PROV_START:
            MY_LOGI("Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV:
        {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            MY_LOGI("Received Wi-Fi credentials"
                    "\n\tSSID     : %s\n\tPassword : %s",
                    (const char *)wifi_sta_cfg->ssid,
                    (const char *)wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL:
        {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            MY_LOGE("Provisioning failed!\n\tReason : %s"
                    "\n\tPlease reset to factory and retry provisioning",
                    (*reason == WIFI_PROV_STA_AUTH_ERROR) ? "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            retries++;
            if (retries >= 5)
            {
                MY_LOGI("Failed to connect with provisioned AP, resetting provisioned credentials");
                wifi_prov_mgr_reset_sm_state_on_failure();
                retries = 0;
            }
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            MY_LOGI("Provisioning successful");
            retries = 0;
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    }
    else if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            MY_LOGI("Disconnected. Connecting to the AP again...");
            wifi_conn_retries++;
            if (wifi_conn_retries >= 5)
            {
                //重置设备的 Wi-Fi 配网状态
                wifi_prov_mgr_reset_provisioning();
                //重启esp32系统
                esp_restart();
            }
            esp_wifi_connect();
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            MY_LOGI("SoftAP transport: Connected!");
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            MY_LOGI("SoftAP transport: Disconnected!");
            break;
        default:
            break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        MY_LOGI("Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_EVENT);
    }
    else if (event_base == PROTOCOMM_TRANSPORT_BLE_EVENT)
    {
        switch (event_id)
        {
        case PROTOCOMM_TRANSPORT_BLE_CONNECTED:
            MY_LOGI("BLE transport: Connected!");
            break;
        case PROTOCOMM_TRANSPORT_BLE_DISCONNECTED:
            MY_LOGI("BLE transport: Disconnected!");
            break;
        default:
            break;
        }
    }
    else if (event_base == PROTOCOMM_SECURITY_SESSION_EVENT)
    {
        switch (event_id)
        {
        case PROTOCOMM_SECURITY_SESSION_SETUP_OK:
            MY_LOGI("Secured session established!");
            break;
        case PROTOCOMM_SECURITY_SESSION_INVALID_SECURITY_PARAMS:
            MY_LOGE("Received invalid security parameters for establishing secure session!");
            break;
        case PROTOCOMM_SECURITY_SESSION_CREDENTIALS_MISMATCH:
            MY_LOGE("Received incorrect username and/or PoP for establishing secure session!");
            break;
        default:
            break;
        }
    }
}

static void wifi_init_sta(void)
{
    /* Start Wi-Fi in station mode */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

/* Handler for the optional provisioning endpoint registered by the application.
 * The data format can be chosen by applications. Here, we are using plain ascii text.
 * Applications can choose to use other formats like protobuf, JSON, XML, etc.
 * Note that memory for the response buffer must be allocated using heap as this buffer
 * gets freed by the protocomm layer once it has been sent by the transport layer.
 */
esp_err_t custom_prov_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen,
                                   uint8_t **outbuf, ssize_t *outlen, void *priv_data)
{
    if (inbuf)
    {
        MY_LOGI("Received data: %.*s", inlen, (char *)inbuf);
    }
    char response[] = "SUCCESS";
    *outbuf = (uint8_t *)strdup(response);
    if (*outbuf == NULL)
    {
        MY_LOGE("System out of memory");
        return ESP_ERR_NO_MEM;
    }
    *outlen = strlen(response) + 1; /* +1 for NULL terminating byte */

    return ESP_OK;
}

static void wifi_prov_print_qr(const char *name, const char *username, const char *pop, const char *transport)
{
    if (!name || !transport)
    {
        MY_LOGW("Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150] = {0};
    if (pop)
    {
    }
    else
    {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\""
                                           ",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, transport);
    }
    MY_LOGI("Scan this QR code from the provisioning application for Provisioning.");
    esp_qrcode_config_t cfg = ESP_QRCODE_CONFIG_DEFAULT();
    esp_qrcode_generate(&cfg, payload);
    MY_LOGI("If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}
void bsp_wifi_Init(void)
{
    // 初始化NVS分区
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    // 初始化TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());
    // 创建默认的ESP时间循环
    /* Initialize the event loop */
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // 创建一个FreeRTOS事件组，用于Wi-Fi连接事件的同步
    wifi_event_group = xEventGroupCreate();
    // 注册事件
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_SECURITY_SESSION_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(PROTOCOMM_TRANSPORT_BLE_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    /* Initialize Wi-Fi including netif with default config */
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* Configuration for the provisioning manager */
    wifi_prov_mgr_config_t config = {
        /* What is the Provisioning Scheme that we want ?
         * wifi_prov_scheme_softap or wifi_prov_scheme_ble */

        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM,

    };

    // 使用上述初始化配置管理器
    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));

    bool provisioned = false;
#ifdef CONFIG_EXAMPLE_RESET_PROVISIONED
    wifi_prov_mgr_reset_provisioning();
#else
    /* Let's find out if the device is provisioned */
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));

#endif
    /* If device is not yet provisioned start provisioning service */
    if (!provisioned)
    {
        MY_LOGI("开始配置");
        char service_name[12];
        // 生成唯一的设备服务名称
        get_device_service_name(service_name, sizeof(service_name));
        // 不使用加密方式传输数据
        wifi_prov_security_t security = WIFI_PROV_SECURITY_0;
        const char *service_key = NULL;

        uint8_t custom_service_uuid[] = {
            /* LSB <---------------------------------------
             * ---------------------------------------> MSB */
            0xb4,
            0xdf,
            0x5a,
            0x1c,
            0x3f,
            0x6b,
            0xf4,
            0xbf,
            0xea,
            0x4a,
            0x82,
            0x03,
            0x04,
            0x90,
            0x1a,
            0x02,
        };
        //(如果使用BLE传输) 设置自定义的BLE服务UUID
        wifi_prov_scheme_ble_set_service_uuid(custom_service_uuid);
        // 创建一个名为“custom-data”的自定义配置端
        wifi_prov_mgr_endpoint_create("custom-data");
        wifi_prov_mgr_start_provisioning(security, NULL, service_name, service_key);

        wifi_prov_mgr_endpoint_register("custom-data", custom_prov_data_handler, NULL);

        wifi_prov_print_qr(service_name, NULL, NULL, PROV_TRANSPORT_BLE);
    }
    else
    {
        MY_LOGI("Already provisioned, starting Wi-Fi STA");
        wifi_prov_mgr_deinit();
        wifi_init_sta();
    }
    // 等待wifi连接
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_EVENT, true, true, portMAX_DELAY);
    MY_LOGE("Hello World,连接成功!");

    // while (1)
    // {
    //     MY_LOGI("Hello World!");
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
}