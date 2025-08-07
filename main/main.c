#include <stdio.h>
#include "audio/audio_process.h"
#include "protocol/pro_https.h"
#include "bsp.h"
#include "pro_websocket.h"
#include "Com_status.h"
#include "bsp_display.h"
#include "bsp_lcd.h"
audio_process_t *audio_process;
extern char session_id_str[9];
extern EventGroupHandle_t event;
static int motor_status = 0;
static int speed = 0;
// 解析发送的json文本数据
void app_websocket_receive_text(const char *data, size_t len)
{
    cJSON *json_str = cJSON_Parse(data);
    cJSON *type_json = cJSON_GetObjectItem(json_str, "type");
    char *type = cJSON_GetStringValue(type_json);

    if (strcmp(type, "hello") == 0)
    {
        cJSON *session_id_json = cJSON_GetObjectItem(json_str, "session_id");
        char *session_id = cJSON_GetStringValue(session_id_json);
        strncpy(session_id_str, session_id, sizeof(session_id_str) - 1);
        // 确保字符串以空字符结尾
        session_id_str[sizeof(session_id_str) - 1] = '\0';
        // MY_LOGE("Session-ID = %s", session_id_str);
        MY_LOGE("接收到hello信息%.*s", len, data);

        xEventGroupSetBits(event, 2);
    }
    // 语音识别结果
    else if (strcmp(type, "stt") == 0)
    {
        MY_LOGE("接收到stt信息%.*s", len, data);
    }
    // 大模型意图识别表情回复
    else if (strcmp(type, "llm") == 0)
    {
        cJSON *emotion_json = cJSON_GetObjectItem(json_str, "emotion");
        char *emotion = cJSON_GetStringValue(emotion_json);
        bsp_display_set_emotion(emotion);
        MY_LOGE("接收到llm信息%.*s", len, data);
    }
    // 文本信息
    else if (strcmp(type, "tts") == 0) // 文本转音频回复
    {
        cJSON *type_json = cJSON_GetObjectItem(json_str, "state");
        char *state = cJSON_GetStringValue(type_json);
        if (strcmp(state, "start") == 0)
        {
            MY_LOGE("状态=start");
            MY_LOGE("接收到tts->start信息%.*s", len, data);
            Com_status_set_state(SPEECH);
        }
        else if (strcmp(state, "sentence_start") == 0)
        {
            cJSON *text_json = cJSON_GetObjectItem(json_str, "text");
            char *text = cJSON_GetStringValue(text_json);
            bsp_display_set_text(text);
            MY_LOGE("状态=sentence_start");
            MY_LOGE("接收到tts->sentence_start信息%.*s", len, data);
        }
        else if (strcmp(state, "stop") == 0)
        {
            MY_LOGE("状态=stop");
            MY_LOGE("接收到tts->stop信息%.*s", len, data);
            Com_status_set_state(IDLE);
        }
    }
    else if (strcmp(type, "iot") == 0)
    {
        cJSON *commands_json = cJSON_GetObjectItem(json_str, "commands");
        int size = cJSON_GetArraySize(commands_json);
        for (uint8_t i = 0; i < size; i++)
        {
            cJSON *com_json = cJSON_GetArrayItem(commands_json, i);

            cJSON *method_json = cJSON_GetObjectItem(com_json, "method");
            char *method = cJSON_GetStringValue(method_json);

            cJSON *parameter_json = cJSON_GetObjectItem(com_json, "parameters");
            if (strcmp(method, "SetMute") == 0)
            {
                cJSON *mute_json = cJSON_GetObjectItem(parameter_json, "mute");
                bool mute = cJSON_IsTrue(mute_json);
                MY_LOGI("mutestatus==========%d", mute);

                if (mute == 0)
                {
                    bsp_es8311_set_mute(false);
                }
                else if (mute == 1)
                {
                    bsp_es8311_set_mute(true);
                }
            }
            else if (strcmp(method, "SetLed") == 0)
            {
                cJSON *Led_json = cJSON_GetObjectItem(parameter_json, "Led");
                bool Led = cJSON_IsTrue(Led_json);

                MY_LOGI("ledstatus==========%d", Led);
                if (Led == 0)
                {
                    bsp_ws2812_led_close();
                }
                else if (Led == 1)
                {
                    bsp_ws2812_led_open();
                }
            }
            else if (strcmp(method, "SetVolume") == 0)
            {
                cJSON *volum_json = cJSON_GetObjectItem(parameter_json, "volume");
                int volume = cJSON_GetNumberValue(volum_json);
                MY_LOGI("SetVolume==========%d", volume);
                bsp_es8311_set_vol(volume);
            }
            else if (strcmp(method, "SetScreen") == 0)
            {
                cJSON *screen_json = cJSON_GetObjectItem(parameter_json, "screen");
                int screen = cJSON_GetNumberValue(screen_json);
                MY_LOGI("SetScreen==========%d", screen);
                bsp_ledc_set_duty(screen);
            }

            else if (strcmp(method, "SetMotorspeed") == 0)
            {
                cJSON *speed_json = cJSON_GetObjectItem(parameter_json, "motor_speed");
                speed = cJSON_GetNumberValue(speed_json);
                MY_LOGI("Motorspeed==========%d", speed);
                const char *motor_speed = pro_mqtt_create_speed_json(speed, motor_status);
                MY_LOGI("===============%s", motor_speed);
                pro_mqtt_publish(motor_speed, strlen(motor_speed));
            }
            else if (strcmp(method, "Setmotor") == 0)
            {
                cJSON *motor_json = cJSON_GetObjectItem(parameter_json, "motor");
                bool motor = cJSON_IsTrue(motor_json);
                MY_LOGI("ledstatus==========%d", motor);
                if (motor == 0)
                {
                    motor_status = 0;
                    const char *motor_speed = pro_mqtt_create_speed_json(speed, motor_status);
                    MY_LOGI("===============%s", motor_speed);
                    pro_mqtt_publish(motor_speed, strlen(motor_speed));
                }
                else if (motor == 1)
                {
                    motor_status = 1;
                    const char *motor_speed = pro_mqtt_create_speed_json(speed, motor_status);
                    MY_LOGI("===============%s", motor_speed);
                    pro_mqtt_publish(motor_speed, strlen(motor_speed));
                }
            }
        }

        MY_LOGI("接收到iot信息%.*s", len, data);
    }
    cJSON_Delete(json_str);
}
void app_websocket_receive_bin(const char *data, size_t len)
{
    audio_process_write_buffer(audio_process, (uint8_t *)data, len);
}

// 传递 audio_sr 实例的目的是让回调函数能访问音频处理上下文
void app_audio_sr_waknet_callback(void *args)
{
    if (state == IDLE)
    {
        Com_status_set_state(CONNECT);
        pro_websocket_start();
        pro_websocket_hello();
        pro_websocket_send_iot();
        pro_websocket_send_equipment_state();
    }
    else if (state == SPEECH)
    {
        pro_websocket_send_abort();
    }
    // MY_LOGE("小龙被唤醒");
    pro_websocket_send_wakenet_word();
    bsp_ledc_set_duty(50);
    Com_status_set_state(IDLE);
}
void app_audio_sr_vad_callback(void *args, vad_state_t vad_state)
{
    if (vad_state == VAD_SILENCE)
    {
        if (state == LISTEN)
        {
            pro_websocket_send_stop_listen();
            Com_status_set_state(IDLE);
        }
    }
    else
    {
        if (state == IDLE)
        {
            pro_websocket_send_start_listen();
            Com_status_set_state(LISTEN);
        }

        // MY_LOGE("此时状态为讲话");
    }
}
void app_main(void)
{

    bsp_init();
    pro_https_send_reg();

    pro_websocket_init();
    // 注册websocket回调函数
    pro_websocket_bin_callback(app_websocket_receive_bin);
    pro_websocket_text_callback(app_websocket_receive_text);

    // pro_websocket_start();

    // 发送测试消息
    // pro_websocket_hello();
    // pro_websocket_send_wakenet_word();
    // pro_websocket_send_start_listen();

    audio_process = audio_process_Init();
    audio_process_set_wakenet_callback(audio_process, app_audio_sr_waknet_callback);
    audio_process_set_vad_callback(audio_process, app_audio_sr_vad_callback);
    audio_process_Start(audio_process);

    size_t len = 0;
    while (1)
    {
        // 从编码器输出缓冲区取数据
        void *datas = audio_process_read_buffer(audio_process, &len);
        if (len > 0)
        {
            // audio_process_write_buffer(audio_process, (uint8_t *)datas, len);
            pro_websocket_send_opus(datas, len);
        }
        free(datas);
        vTaskDelay(20);
    }
}
