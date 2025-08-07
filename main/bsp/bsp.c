#include "bsp.h"

static bsp_nvs_t *bsp_nvs;
char *uuid = NULL;
char *mac = NULL;
void bsp_init()
{
    bsp_lcd_init();
    bsp_lvgl_init();
    bsp_display_init();
    bsp_ledc_init();
    
    bsp_wifi_Init();
    pro_mqtt_init();
    
    bsp_es8311_Init();
    bsp_es8311_Open();
    bsp_ws2812_Init();

    bsp_nvs = bsp_nvs_init();
}

char *bsp_get_mac_addr()
{
    if (mac != NULL)
    {
        return mac;
    }
    mac = (char *)malloc(18);
    uint8_t eth_mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);

    snprintf(mac, 18, "%02x:%02x:%02x:%02x:%02x:%02x",
             eth_mac[0], eth_mac[1], eth_mac[2], eth_mac[3], eth_mac[4], eth_mac[5]);
    mac[17] = '\0';
    return mac;
}

void generate_strict_uuid_v4(char *uuid_str)
{
    uint8_t bytes[16]; // 16 bytes to store the binary UUID data
    int i;

    // Use current time as seed. Note: May generate same sequence if called rapidly.
    srand((unsigned int)time(NULL));

    // Fill all 16 bytes with random data
    for (i = 0; i < 16; i++)
    {
        bytes[i] = rand() % 256;
    }

    // Set UUID version to 4 (0100)
    // High 4 bits of the 7th byte (index 6)
    bytes[6] = (bytes[6] & 0x0F) | 0x40;

    // Set UUID variant to 1 (10xx)
    // High 2 bits of the 9th byte (index 8)
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    // Format the byte array into a UUID string
    sprintf(uuid_str,
            "%02x%02x%02x%02x-"
            "%02x%02x-"
            "%02x%02x-"
            "%02x%02x-"
            "%02x%02x%02x%02x%02x%02x",
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5],
            bytes[6], bytes[7],
            bytes[8], bytes[9],
            bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
}

char *bsp_get_uuid()
{
    // 检查内存中是否已存在uuid
    if (uuid != NULL)
    {
        return uuid;
    }
    uuid = (char *)malloc(37);
    // 检查nvs是否存在uuid
    esp_err_t err = bsp_nvs_key_is_exist(bsp_nvs, "uuid");
    if (err == ESP_OK) // 从nvs中读出uuid
    {
        size_t len = 37;
        bsp_nvs_read_nvs(bsp_nvs, "uuid", uuid, &len);
        return uuid;
    }
    else
    {
        generate_strict_uuid_v4(uuid);
        bsp_nvs_write_nvs(bsp_nvs, "uuid", uuid);
        return uuid;
    }
}