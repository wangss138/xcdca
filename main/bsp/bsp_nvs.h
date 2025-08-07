#ifndef __BSP_NVS_H__
#define __BSP_NVS_H__
#include "nvs_flash.h"
typedef struct bsp_nvs bsp_nvs_t;

bsp_nvs_t *bsp_nvs_init(void);
esp_err_t bsp_nvs_write_nvs(bsp_nvs_t *bsp_nvs, const char *key, const char *value);
esp_err_t bsp_nvs_read_nvs(bsp_nvs_t *bsp_nvs, const char *key, char *value, size_t *len);
void bsp_nvs_close(bsp_nvs_t *bsp_nvs);
esp_err_t bsp_nvs_key_is_exist(bsp_nvs_t *bsp_nvs, const char *key);
#endif /* __BSP_NVS_H__ */