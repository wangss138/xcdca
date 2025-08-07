#include "pro_websocket.h"

bin_callback websocket_bin_callback;
text_callback websocket_text_callback;
char session_id_str[9] = {0};
static esp_websocket_client_handle_t client;
EventGroupHandle_t event;
extern audio_process_t *audio_process;
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        MY_LOGE("Last error %s: 0x%x", message, error_code);
    }
}

static void websocket_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_websocket_event_data_t *data = (esp_websocket_event_data_t *)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_BEGIN:
        MY_LOGI("WEBSOCKET_EVENT_BEGIN");
        break;
    case WEBSOCKET_EVENT_CONNECTED:
        MY_LOGE("WEBSOCKET_EVENT_CONNECTED");
        xEventGroupSetBits(event, 1);
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        MY_LOGI("WEBSOCKET_EVENT_DISCONNECTED");
        log_error_if_nonzero("HTTP status code", data->error_handle.esp_ws_handshake_status_code);
        if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", data->error_handle.esp_transport_sock_errno);
        }
        break;
    case WEBSOCKET_EVENT_DATA:
        // MY_LOGE("WEBSOCKET_EVENT_DATA");
        // MY_LOGE("Received opcode=%d", data->op_code);
        if (data->op_code == 0x1) // 文本数据
        {
            // MY_LOGE("收到数据");
            websocket_text_callback(data->data_ptr, data->data_len);
        }
        else if (data->op_code == 0x2) // 语音数据  Opcode 0x2 indicates binary data
        {
            websocket_bin_callback(data->data_ptr, data->data_len);
        }

        break;
    case WEBSOCKET_EVENT_ERROR:
        MY_LOGI("WEBSOCKET_EVENT_ERROR");
        log_error_if_nonzero("HTTP status code", data->error_handle.esp_ws_handshake_status_code);
        if (data->error_handle.error_type == WEBSOCKET_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", data->error_handle.esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", data->error_handle.esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", data->error_handle.esp_transport_sock_errno);
        }
        break;
    case WEBSOCKET_EVENT_FINISH:
        MY_LOGI("WEBSOCKET_EVENT_FINISH");
        Com_status_set_state(IDLE);
        audio_process_reset_waknet(audio_process);
        bsp_ws2812_led_close();
        bsp_ledc_set_duty(0);
        break;
    }
}

void pro_websocket_init()
{
    esp_websocket_client_config_t websocket_cfg = {
        .uri = "wss://api.tenclass.net/xiaozhi/v1/",

        .buffer_size = 2048,                        // 增大缓冲区
        .transport = WEBSOCKET_TRANSPORT_OVER_SSL,  // WSS认证需要
        .crt_bundle_attach = esp_crt_bundle_attach, // WSS认证需要
        .network_timeout_ms = 5000,                 // 网络操作超时时间
        .reconnect_timeout_ms = 5000,
    };
    // 创建连接
    client = esp_websocket_client_init(&websocket_cfg);

    /*Bearer​：这是一种身份验证方案（Authentication Scheme），表示接下来提供的令牌(token)将允许访问资源。
    ​​<access_token>​​：这是一个占位符，实际使用中应该替换为服务器颁发给客户端的访问令牌*/
    esp_websocket_client_append_header(client, "Authorizatio", "Bearer test-token");
    esp_websocket_client_append_header(client, "Protocol-Version", "1");
    esp_websocket_client_append_header(client, "Device-Id", bsp_get_mac_addr());
    esp_websocket_client_append_header(client, "Client-Id", bsp_get_uuid());
    // 注册回调函数
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void *)client);
    event = xEventGroupCreate();
}

void pro_websocket_start()
{
    if (client != NULL && !esp_websocket_client_is_connected(client))
    {
        esp_websocket_client_start(client);
        // 做阻塞等待连接操作
        xEventGroupWaitBits(event, 1, pdTRUE, pdTRUE, portMAX_DELAY);
    }
}

void pro_websocket_close()
{
    if (client != NULL && esp_websocket_client_is_connected(client))
    {
        esp_websocket_client_close(client, portMAX_DELAY);
    }
}

void pro_websocket_hello()
{
    if (client != NULL && esp_websocket_client_is_connected(client))
    {
        cJSON *root_json = cJSON_CreateObject();
        // cJSON *features_json = cJSON_CreateObject();
        // cJSON_AddItemToObject(root_json, "features", features_json);
        // cJSON_AddBoolToObject(features_json, "mcp", true);
        cJSON *audio_json = cJSON_CreateObject();
        cJSON_AddItemToObject(root_json, "audio_params", audio_json);
        cJSON_AddStringToObject(root_json, "type", "hello");
        cJSON_AddStringToObject(root_json, "transport", "websocket");
        cJSON_AddNumberToObject(root_json, "version", 1);
        cJSON_AddStringToObject(audio_json, "format", "opus");
        cJSON_AddNumberToObject(audio_json, "sample_rate", 16000);
        cJSON_AddNumberToObject(audio_json, "channels", 1);
        cJSON_AddNumberToObject(audio_json, "frame_duration", 60);
        char *json = cJSON_PrintUnformatted(root_json);
        MY_LOGE("===========%.*s", strlen(json), json);
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
        free(json);
        cJSON_Delete(root_json);

        xEventGroupWaitBits(event, 2, true, true, portMAX_DELAY);
    }
}

void pro_websocket_send_wakenet_word()
{
    if (client != NULL && esp_websocket_client_is_connected(client))
    {
        cJSON *root_json = cJSON_CreateObject();

        cJSON_AddStringToObject(root_json, "type", "listen");
        cJSON_AddStringToObject(root_json, "state", "detect");
        cJSON_AddStringToObject(root_json, "text", "小龙小龙");

        cJSON_AddStringToObject(root_json, "session_id", session_id_str);

        char *json = cJSON_PrintUnformatted(root_json);
        MY_LOGE("===========%.*s", strlen(json), json);
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
        free(json);
        cJSON_Delete(root_json);
    }
}
void pro_websocket_send_start_listen()
{
    if (client != NULL && esp_websocket_client_is_connected(client))
    {
        cJSON *root_json = cJSON_CreateObject();

        cJSON_AddStringToObject(root_json, "type", "listen");
        cJSON_AddStringToObject(root_json, "state", "start");
        cJSON_AddStringToObject(root_json, "mode", "auto");
        cJSON_AddStringToObject(root_json, "session_id", session_id_str);

        char *json = cJSON_PrintUnformatted(root_json);
        MY_LOGE("===========%.*s", strlen(json), json);
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
        free(json);
        cJSON_Delete(root_json);
    }
}
// 停止监听
void pro_websocket_send_stop_listen()
{
    if (client != NULL && esp_websocket_client_is_connected(client))
    {
        cJSON *root_json = cJSON_CreateObject();
        cJSON_AddStringToObject(root_json, "session_id", session_id_str);
        cJSON_AddStringToObject(root_json, "type", "listen");
        cJSON_AddStringToObject(root_json, "state", "stop");

        char *json = cJSON_PrintUnformatted(root_json);
        MY_LOGE("===========%.*s", strlen(json), json);
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
        free(json);
        cJSON_Delete(root_json);
    }
}
// 中止消息
void pro_websocket_send_abort()
{
    if (client != NULL && esp_websocket_client_is_connected(client))
    {
        cJSON *root_json = cJSON_CreateObject();
        cJSON_AddStringToObject(root_json, "session_id", session_id_str);
        cJSON_AddStringToObject(root_json, "type", "abort");
        cJSON_AddStringToObject(root_json, "reason", "wake_word_detected");

        char *json = cJSON_PrintUnformatted(root_json);
        MY_LOGE("===========%.*s", strlen(json), json);
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
        free(json);
        cJSON_Delete(root_json);
    }
}

// 发送音频数据
void pro_websocket_send_opus(void *data, size_t len)
{
    if (client != NULL && esp_websocket_client_is_connected(client))
    {
        esp_websocket_client_send_bin(client, (char *)data, (int)len, portMAX_DELAY);
    }
}

void pro_websocket_bin_callback(bin_callback bin_cb)
{
    websocket_bin_callback = bin_cb;
}

void pro_websocket_text_callback(text_callback text_cb)
{
    websocket_text_callback = text_cb;
}

static char *create_iot_json(const char *session_id, cJSON_bool update_status)
{
    cJSON *root = NULL;
    cJSON *descriptors_array = NULL;
    cJSON *speaker_object = NULL;
    cJSON *methods_object = NULL;

    cJSON *set_mute_object = NULL;
    cJSON *set_volume_object = NULL;
    cJSON *set_screen_object = NULL;
    cJSON *set_motor_speed_object = NULL;
    cJSON *set_Led_object = NULL;
    cJSON *set_motor_object = NULL;

    cJSON *set_mute_parameters = NULL;
    cJSON *set_volume_parameters = NULL;
    cJSON *set_screen_parameters = NULL;
    cJSON *set_motor_speed_parameters = NULL;
    cJSON *set_Led_parameters = NULL;
    cJSON *set_motor_parameters = NULL;

    cJSON *mute_parameter = NULL;
    cJSON *volume_parameter = NULL;
    cJSON *screen_parameter = NULL;
    cJSON *motor_speed_parameter = NULL;
    cJSON *Led_parameter = NULL;
    cJSON *motor_parameter = NULL;

    cJSON *properties_object = NULL;

    cJSON *mute_property = NULL;
    cJSON *volume_property = NULL;
    cJSON *screen_property = NULL;
    cJSON *motor_speed_property = NULL;
    cJSON *Led_property = NULL;
    cJSON *motor_property = NULL;

    char *json_string = NULL;

    // 1. 创建根对象
    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto end;
    }

    // 2. 添加顶层键值对
    cJSON_AddStringToObject(root, "session_id", session_id);
    cJSON_AddStringToObject(root, "type", "iot");
    cJSON_AddBoolToObject(root, "update", update_status);

    // 3. 创建 descriptors 数组并添加到根对象
    descriptors_array = cJSON_CreateArray();
    if (descriptors_array == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(root, "descriptors", descriptors_array);

    // 4. 创建 descriptors 数组中的 "Speaker" 对象
    speaker_object = cJSON_CreateObject();
    if (speaker_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToArray(descriptors_array, speaker_object);

    // 5. 在 "Speaker" 对象中添加键值对
    cJSON_AddStringToObject(speaker_object, "description", "Speaker");
    cJSON_AddStringToObject(speaker_object, "name", "Speaker");

    // 6. 创建 "Speaker" 对象的 "methods" 对象
    methods_object = cJSON_CreateObject();
    if (methods_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(speaker_object, "methods", methods_object);

    // 7. 创建 "methods" 对象中的 "SetMute" 方法对象
    set_mute_object = cJSON_CreateObject();
    if (set_mute_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(methods_object, "SetMute", set_mute_object);
    cJSON_AddStringToObject(set_mute_object, "description", "Set mute status");
    set_mute_parameters = cJSON_CreateObject();
    if (set_mute_parameters == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_mute_object, "parameters", set_mute_parameters);
    mute_parameter = cJSON_CreateObject();
    if (mute_parameter == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_mute_parameters, "mute", mute_parameter);
    cJSON_AddStringToObject(mute_parameter, "description", "Mute status");
    cJSON_AddStringToObject(mute_parameter, "type", "boolean");

    // 8. 创建 "methods" 对象中的 "SetVolume" 方法对象
    set_volume_object = cJSON_CreateObject();
    if (set_volume_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(methods_object, "SetVolume", set_volume_object);
    cJSON_AddStringToObject(set_volume_object, "description", "Set volume level");
    set_volume_parameters = cJSON_CreateObject();
    if (set_volume_parameters == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_volume_object, "parameters", set_volume_parameters);
    volume_parameter = cJSON_CreateObject();
    if (volume_parameter == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_volume_parameters, "volume", volume_parameter);
    cJSON_AddStringToObject(volume_parameter, "description", "Volume level[0-100]");
    cJSON_AddStringToObject(volume_parameter, "type", "number");

    // 9. 创建 "methods" 对象中的 "Setmotorspeed" 方法对象
    set_motor_speed_object = cJSON_CreateObject();
    if (set_motor_speed_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(methods_object, "SetMotorspeed", set_motor_speed_object);
    cJSON_AddStringToObject(set_motor_speed_object, "description", "Set motor speed");
    set_motor_speed_parameters = cJSON_CreateObject();
    if (set_motor_speed_parameters == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_motor_speed_object, "parameters", set_motor_speed_parameters);
    motor_speed_parameter = cJSON_CreateObject();
    if (motor_speed_parameter == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_motor_speed_parameters, "motor_speed", motor_speed_parameter);
    cJSON_AddStringToObject(motor_speed_parameter, "description", "motor_speed[-220-220]");
    cJSON_AddStringToObject(motor_speed_parameter, "type", "number");

    // 10. 创建 "methods" 对象中的 "SetScreen" 方法对象
    set_screen_object = cJSON_CreateObject();
    if (set_screen_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(methods_object, "SetScreen", set_screen_object);
    cJSON_AddStringToObject(set_screen_object, "description", "Set screen level");
    set_screen_parameters = cJSON_CreateObject();
    if (set_screen_parameters == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_screen_object, "parameters", set_screen_parameters);
    screen_parameter = cJSON_CreateObject();
    if (screen_parameter == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_screen_parameters, "screen", screen_parameter);
    cJSON_AddStringToObject(screen_parameter, "description", "Screen level[0-100]");
    cJSON_AddStringToObject(screen_parameter, "type", "number");

    // 11. 创建 "methods" 对象中的 "SetLed" 方法对象
    set_Led_object = cJSON_CreateObject();
    if (set_Led_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(methods_object, "SetLed", set_Led_object);
    cJSON_AddStringToObject(set_Led_object, "description", "set Led status");
    set_Led_parameters = cJSON_CreateObject();
    if (set_Led_parameters == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_Led_object, "parameters", set_Led_parameters);
    Led_parameter = cJSON_CreateObject();
    if (Led_parameter == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_Led_parameters, "Led", Led_parameter);
    cJSON_AddStringToObject(Led_parameter, "description", "Ledstatus");
    cJSON_AddStringToObject(Led_parameter, "type", "boolean");

    // 12. 创建 "methods" 对象中的 "motor" 方法对象
    set_motor_object = cJSON_CreateObject();
    if (set_motor_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(methods_object, "Setmotor", set_motor_object);
    cJSON_AddStringToObject(set_motor_object, "description", "set motor status");
    set_motor_parameters = cJSON_CreateObject();
    if (set_motor_parameters == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_motor_object, "parameters", set_motor_parameters);
    motor_parameter = cJSON_CreateObject();
    if (motor_parameter == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(set_motor_parameters, "motor", motor_parameter);
    cJSON_AddStringToObject(motor_parameter, "description", "motorstatus");
    cJSON_AddStringToObject(motor_parameter, "type", "boolean");
    // 13. 创建 "Speaker" 对象的 "properties" 对象
    properties_object = cJSON_CreateObject();
    if (properties_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(speaker_object, "properties", properties_object);

    // 14. 创建 "properties" 对象中的 "mute" 属性对象
    mute_property = cJSON_CreateObject();
    if (mute_property == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(properties_object, "mute", mute_property);
    cJSON_AddStringToObject(mute_property, "description", "Mute status");
    cJSON_AddStringToObject(mute_property, "type", "boolean");

    // 15. 创建 "properties" 对象中的 "volume" 属性对象
    volume_property = cJSON_CreateObject();
    if (volume_property == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(properties_object, "volume", volume_property);
    cJSON_AddStringToObject(volume_property, "description", "Volume level[0-100]");
    cJSON_AddStringToObject(volume_property, "type", "number");

    // 16. 创建 "properties" 对象中的 "motorspeed" 属性对象
    motor_speed_property = cJSON_CreateObject();
    if (motor_speed_property == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(properties_object, "motor_speed", motor_speed_property);
    cJSON_AddStringToObject(motor_speed_property, "description", "motor_speed[-220-220]");
    cJSON_AddStringToObject(motor_speed_property, "type", "number");

    // 17. 创建 "properties" 对象中的 "screen" 属性对象
    screen_property = cJSON_CreateObject();
    if (screen_property == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(properties_object, "screen", screen_property);
    cJSON_AddStringToObject(screen_property, "description", "Screen level[0-100]");
    cJSON_AddStringToObject(screen_property, "type", "number");
    // 18. 创建 "properties" 对象中的 "Led" 属性对象
    Led_property = cJSON_CreateObject();
    if (Led_property == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(properties_object, "Led", Led_property);
    cJSON_AddStringToObject(Led_property, "description", "Led status");
    cJSON_AddStringToObject(Led_property, "type", "boolean");
    // 17. 将cJSON对象转换为可打印的字符串
    json_string = cJSON_PrintUnformatted(root);

    // 19. 创建 "properties" 对象中的 "Led" 属性对象
    Led_property = cJSON_CreateObject();
    if (Led_property == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(properties_object, "Led", Led_property);
    cJSON_AddStringToObject(Led_property, "description", "Led status");
    cJSON_AddStringToObject(Led_property, "type", "boolean");

    // 20. 创建 "properties" 对象中的 "motor" 属性对象
    motor_property = cJSON_CreateObject();
    if (motor_property == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(properties_object, "motor", motor_property);
    cJSON_AddStringToObject(motor_property, "description", "motor status");
    cJSON_AddStringToObject(motor_property, "type", "boolean");
    // 21. 将cJSON对象转换为可打印的字符串
    json_string = cJSON_PrintUnformatted(root);
end:
    // 22. 释放cJSON对象占用的内存，但不释放json_string
    // cJSON_Delete会递归地释放所有子对象
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    // 23. 返回最终生成的字符串
    return json_string;
}

// iot信息
void pro_websocket_send_iot()
{
    if (client != NULL && esp_websocket_client_is_connected(client))
    {

        char *json = create_iot_json(session_id_str, true);
        MY_LOGE("===========%.*s", strlen(json), json);
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
        free(json);
    }
}

static char *create_iot_state_json(const char *session_id, cJSON_bool update_status, cJSON_bool mute_status, cJSON_bool Led_status, int volume_level, int screen_level)
{
    cJSON *root = NULL;
    cJSON *states_array = NULL;
    cJSON *speaker_state_item = NULL;
    cJSON *state_object = NULL;
    char *json_string = NULL;

    // 1. 创建根对象
    root = cJSON_CreateObject();
    if (root == NULL)
    {
        goto end;
    }

    // 2. 添加顶层键值对
    cJSON_AddStringToObject(root, "session_id", session_id);
    cJSON_AddStringToObject(root, "type", "iot");
    cJSON_AddBoolToObject(root, "update", update_status);

    // 3. 创建 "states" 数组
    states_array = cJSON_CreateArray();
    if (states_array == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(root, "states", states_array);

    // 4. 创建 "states" 数组中的第一个（也是唯一一个）对象
    speaker_state_item = cJSON_CreateObject();
    if (speaker_state_item == NULL)
    {
        goto end;
    }
    cJSON_AddItemToArray(states_array, speaker_state_item);

    // 5. 添加 "Speaker" 的名称键
    cJSON_AddStringToObject(speaker_state_item, "name", "Speaker");

    // 6. 创建 "state" 对象
    state_object = cJSON_CreateObject();
    if (state_object == NULL)
    {
        goto end;
    }
    cJSON_AddItemToObject(speaker_state_item, "state", state_object);

    // 7. 在 "state" 对象中添加 mute 和 volume 和 Led 和 screen 和 motorspeed 和motorstatus
    cJSON_AddBoolToObject(state_object, "mute", mute_status);
    cJSON_AddNumberToObject(state_object, "volume", volume_level);
    cJSON_AddNumberToObject(state_object, "screen", screen_level);
    cJSON_AddNumberToObject(state_object, "motor_speed", 0);
    cJSON_AddBoolToObject(state_object, "motor", 0);
    cJSON_AddBoolToObject(state_object, "Led", Led_status);

    // 8. 将cJSON对象转换为可打印的字符串
    json_string = cJSON_PrintUnformatted(root);

end:
    // 9. 释放cJSON对象占用的内存，但不释放json_string
    if (root != NULL)
    {
        cJSON_Delete(root);
    }

    // 10. 返回最终生成的字符串
    return json_string;
}

// 发送设备状态信息
void pro_websocket_send_equipment_state()
{
    if (client != NULL && esp_websocket_client_is_connected(client))
    {

        char *json = create_iot_state_json(session_id_str, true, false, false, 30, 50);
        MY_LOGE("===========%.*s", strlen(json), json);
        esp_websocket_client_send_text(client, json, strlen(json), portMAX_DELAY);
        free(json);
    }
}