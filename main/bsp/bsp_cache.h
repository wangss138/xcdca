#ifndef __BSP_CACHE_H__
#define __BSP_CACHE_H__

#include "string.h"
#include "stdint.h"
#include"stdlib.h"
#include"esp_heap_caps.h"
typedef struct bsp_cache bsp_cache_t;
bsp_cache_t *bsp_cache_init(void);
void bsp_cache_append_data(bsp_cache_t *bsp_cache, char *data, size_t len);
char*bsp_cache_get_data(bsp_cache_t *bsp_cache);
void bsp_cache_free_data(bsp_cache_t *bsp_cache);

#endif /* __cache_H__ */