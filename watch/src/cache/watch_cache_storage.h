#ifndef CMF_WATCH_CACHE_STORAGE_H
#define CMF_WATCH_CACHE_STORAGE_H

#include <stdint.h>
#include "cache/watch_cache.h"

int watch_cache_storage_load(struct watch_cache_record *cache);
int watch_cache_storage_save(const struct watch_cache_record *cache);
uint64_t watch_cache_storage_now_ms(void);

#endif
