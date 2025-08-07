#include "pro_https.h"
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    // 在你的 HTTP_EVENT_ON_DATA 处理中，user_data 就是用来指向你自己的输出缓冲区
    //  (MAX_HTTP_OUTPUT_BUFFER)，以便将 evt->data 中的数据复制到这个缓冲区中进行累积
    bsp_cache_t *cache = (bsp_cache_t *)evt->user_data;

    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        MY_LOGD("HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        MY_LOGD("HTTP_EVENT_ON_CONNECTED");
        break;
    // HTTP 请求的头部信息已经成功发送到服务器
    case HTTP_EVENT_HEADER_SENT:
        MY_LOGD("HTTP_EVENT_HEADER_SENT");
        break;
    // 表示客户端正在接收到服务器响应的头部信息。每次接收到一个头部字段
    // （例如 Content-Type、Content-Length 等），都会触发此事件
    case HTTP_EVENT_ON_HEADER:
        MY_LOGD("HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    // 接收数据事件
    case HTTP_EVENT_ON_DATA:
        MY_LOGD("HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        bsp_cache_append_data(cache, evt->data, evt->data_len);
        break;
    // 事件完成，所有数据接收完毕
    case HTTP_EVENT_ON_FINISH:
        MY_LOGD("HTTP_EVENT_ON_FINISH");
        char *response_data = bsp_cache_get_data(cache);
        MY_LOGE("收到的数据：%.*s", strlen(response_data), response_data);
        break;
    // 断开连接
    case HTTP_EVENT_DISCONNECTED:
        MY_LOGI("HTTP_EVENT_DISCONNECTED");
        break;
    // 表示服务器返回了重定向响应
    case HTTP_EVENT_REDIRECT:
        MY_LOGD("HTTP_EVENT_REDIRECT");
        break;
    }
    return ESP_OK;
}

char *generate_str_json()
{

    const esp_app_desc_t *desc = esp_app_get_description();

    cJSON *root_json = cJSON_CreateObject();
    cJSON *app_info = cJSON_CreateObject();
    cJSON *board_info = cJSON_CreateObject();
    cJSON_AddItemToObject(root_json, "application", app_info);
    cJSON_AddStringToObject(app_info, "version", desc->version);
    cJSON_AddStringToObject(app_info, "elf_sha256", esp_app_get_elf_sha256_str());
    // cJSON_AddStringToObject(app_info, "elf_sha256", desc->app_elf_sha256);

    cJSON_AddItemToObject(root_json, "board", board_info);
    cJSON_AddStringToObject(board_info, "type", "bread-compact_wifi");
    cJSON_AddStringToObject(board_info, "name", "bread-compact_wifi");
    cJSON_AddStringToObject(board_info, "ssid", "ggb_wifi");
    cJSON_AddNumberToObject(board_info, "rssi", 30);
    char *json_str = cJSON_PrintUnformatted(root_json);
    cJSON_Delete(root_json);
    return json_str;
}

void pro_https_send_reg()
{
    bsp_cache_t *bsp_cache = bsp_cache_init();

    esp_http_client_config_t config = {
        .url = "https://api.tenclass.net/xiaozhi/ota/",
        .event_handler = _http_event_handler,
        .user_data = bsp_cache,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .method = HTTP_METHOD_POST,
    };
    // 建立客户端连接
    esp_http_client_handle_t client = esp_http_client_init(&config);
    // 发送头文件
    // 这行代码告诉服务器，客户端发送的请求体（如POST请求的数据）为JSON格式
    esp_http_client_set_header(client, "Content-Type", "application/json");
    // 客户端期望收到json格式数据
    esp_http_client_set_header(client, "Accept", "application/json");

    esp_http_client_set_header(client, "Device-Id", bsp_get_mac_addr());
    esp_http_client_set_header(client, "Client-Id", bsp_get_uuid());
    esp_http_client_set_header(client, "Accept-Language", "zh-CN");
    esp_http_client_set_header(client, "User-Agent", "bread-compact_wifi/1.0.0");
    // 生成json字符串
    char *json_data = generate_str_json();
    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    MY_LOGE("post请求数据:%.*s", strlen(json_data), json_data);
    //  发送请求
    esp_http_client_perform(client);
    // 等待响应码
    int code = esp_http_client_get_status_code(client);
    if (code == 200)
    {
        MY_LOGE("发送post请求成功");
    }
    else
    {
        MY_LOGE("发送post请求失败");
    }

    // 释放资源
    free(json_data);
    // 释放资源
    esp_http_client_cleanup(client);
    bsp_cache_free_data(bsp_cache);
}
