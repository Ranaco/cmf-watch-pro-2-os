#ifndef CMF_WATCH_CACHE_H
#define CMF_WATCH_CACHE_H
#include <stdbool.h>
#include <stdint.h>
struct watch_cache_metadata {
	uint32_t version;
	uint64_t updated_at_ms;
	bool stale;
};
void watch_cache_init(void);
const struct watch_cache_metadata *watch_cache_status(void);
#endif
