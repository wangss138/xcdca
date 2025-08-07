#include "bsp_cache.h"

struct bsp_cache
{
    char *buf;
    size_t len;
};

bsp_cache_t *bsp_cache_init(void)
{
    bsp_cache_t *bsp_cache = (bsp_cache_t *)heap_caps_malloc(sizeof(bsp_cache_t), MALLOC_CAP_SPIRAM);
    memset((void *)bsp_cache, 0, sizeof(bsp_cache_t));
    bsp_cache->buf = NULL; // 显式初始化
    bsp_cache->len = 0;
    return bsp_cache;
}
void bsp_cache_append_data(bsp_cache_t *bsp_cache, char *data, size_t len)
{
    size_t new_size = bsp_cache->len + len;
    char *buf = (char *)heap_caps_realloc(bsp_cache->buf, new_size + 1, MALLOC_CAP_SPIRAM);
    memcpy((void *)buf + bsp_cache->len, data, len);

    bsp_cache->buf = buf;
    bsp_cache->len = new_size;
    bsp_cache->buf[bsp_cache->len] = 0;
}
char *bsp_cache_get_data(bsp_cache_t *bsp_cache)
{
    return bsp_cache->buf;
}
void bsp_cache_free_data(bsp_cache_t *bsp_cache)
{
    heap_caps_free(bsp_cache->buf);
    bsp_cache->len = 0;
}