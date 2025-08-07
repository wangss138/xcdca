#include "pro_mqtt.h"


// extern EventGroupHandle_t event;
esp_mqtt_client_handle_t pub_client;
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        MY_LOGE("Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    MY_LOGD("Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    pub_client = event->client;
    // int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        MY_LOGE("MQTT连接成功");
        // xEventGroupSetBits(event, 3);
        // msg_id = esp_mqtt_client_publish(client, "helllllo", "data_3", 0, 0, 0);

        break;
    case MQTT_EVENT_DISCONNECTED:
        MY_LOGI("MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED: // 成功订阅
        break;
    case MQTT_EVENT_UNSUBSCRIBED: // 成功取消订阅
        break;
    case MQTT_EVENT_PUBLISHED: // 成功发送消息
        // event->msg_id 字段会包含你发布消息时得到的那个消息 ID,这可以帮助你追踪哪条消息已经被成功发布
        break;
    case MQTT_EVENT_DATA:
        MY_LOGI("MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        MY_LOGI("MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            MY_LOGI("Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        MY_LOGI("Other event id:%d", event->event_id);
        break;
    }
}

void pro_mqtt_init()
{
    const char *esp_client_id = "esp32_xiaozhi";
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://192.168.56.48:1883",
        .credentials.client_id = esp_client_id,
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

char *pro_mqtt_create_speed_json(int speed_value,int status)
{
    cJSON *root = NULL;
    char *json_string = NULL;

    // 1. 创建 JSON 根对象
    root = cJSON_CreateObject();
    if (root == NULL)
    {
        return NULL; // 如果创建失败，返回空指针
    }

    // 2. 向根对象中添加键值对
    // 添加字符串类型的 "connection_type"
    cJSON_AddStringToObject(root, "connection_type", "modbus");

    // 添加整数类型的 "id"
    cJSON_AddNumberToObject(root, "id", 5);

    // 添加整数类型的 "speed"，值来自函数参数
    cJSON_AddNumberToObject(root, "speed", speed_value);

    // 添加整数类型的 "motorstatus"
    cJSON_AddNumberToObject(root, "motorstatus", status);

    // 3. 将 cJSON 对象序列化为字符串
    // 使用 cJSON_Print() 打印，它会包含格式化和换行
    json_string = cJSON_Print(root);
    if (json_string == NULL)
    {
        cJSON_Delete(root); // 如果打印失败，也要记得释放对象内存
        return NULL;
    }

    // 4. 释放 cJSON 对象所占用的内存
    cJSON_Delete(root);

    // 5. 返回生成的字符串。调用者必须负责释放这块内存
    return json_string;
}

void pro_mqtt_publish(const char * data,int len){
    esp_mqtt_client_publish(pub_client, "helllllo", data, len, 0, 1);
}