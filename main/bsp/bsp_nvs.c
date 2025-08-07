#include "bsp_nvs.h"

struct bsp_nvs
{
    nvs_handle_t bsp_handle;
};

bsp_nvs_t *bsp_nvs_init(void)
{
    bsp_nvs_t *bsp_nvs = (bsp_nvs_t *)malloc(sizeof(bsp_nvs_t));
    if (bsp_nvs == NULL)
    {
        return NULL;
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS分区被截断或包含新版本的数据格式
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ret = nvs_open("BSP", NVS_READWRITE, &bsp_nvs->bsp_handle);
    if (ret != ESP_OK)
    {
        return NULL;
    }

    return bsp_nvs;
}

esp_err_t bsp_nvs_write_nvs(bsp_nvs_t *bsp_nvs, const char *key, const char *value)
{
    esp_err_t err = nvs_set_str(bsp_nvs->bsp_handle, key, value);
    if (err != ESP_OK)
    {
        return err;
    }
    nvs_commit(bsp_nvs->bsp_handle);
    return err;
}
esp_err_t bsp_nvs_read_nvs(bsp_nvs_t *bsp_nvs, const char *key,  char *value, size_t *len)
{
    return nvs_get_str(bsp_nvs->bsp_handle, key, value, len);
}

void bsp_nvs_close(bsp_nvs_t *bsp_nvs)
{
    if (bsp_nvs != NULL)
    {
        nvs_close(bsp_nvs->bsp_handle);
        free(bsp_nvs);
    }
}

esp_err_t bsp_nvs_key_is_exist(bsp_nvs_t *bsp_nvs, const char *key)
{
    //out_type：可选参数，用于返回键对应的数据类型（如 NVS_TYPE_I32、NVS_TYPE_STR 等）。若为 NULL，则仅检查存在性
    return nvs_find_key(bsp_nvs->bsp_handle, key, NULL);
}