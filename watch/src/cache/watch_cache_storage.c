/* SPDX-License-Identifier: Apache-2.0 */
#include "cache/watch_cache_storage.h"

#include <errno.h>
#include <zephyr/kernel.h>

#if defined(CONFIG_BOARD_NATIVE_SIM)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char *cache_path(void)
{
	const char *path = getenv("CMF_WATCH_CACHE_PATH");
	return path != NULL && path[0] != '\0' ? path : NULL;
}

int watch_cache_storage_load(struct watch_cache_record *cache)
{
	const char *path = cache_path();
	if (path == NULL) return -ENOTSUP;
	FILE *file = fopen(path, "rb");
	if (file == NULL) return errno == ENOENT ? -ENOENT : -EIO;
	struct watch_cache_record candidate;
	size_t count = fread(&candidate, 1U, sizeof(candidate), file);
	int trailing = fgetc(file);
	int close_result = fclose(file);
	if (count != sizeof(candidate) || trailing != EOF || close_result != 0 ||
	    !watch_cache_valid(&candidate)) return -EBADMSG;
	*cache = candidate;
	return 0;
}

int watch_cache_storage_save(const struct watch_cache_record *cache)
{
	const char *path = cache_path();
	if (path == NULL) return -ENOTSUP;
	char temporary[512];
	int length = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
	if (length <= 0 || (size_t)length >= sizeof(temporary)) return -ENAMETOOLONG;
	FILE *file = fopen(temporary, "wb");
	if (file == NULL) return -EIO;
	bool written = fwrite(cache, 1U, sizeof(*cache), file) == sizeof(*cache);
	bool flushed = fflush(file) == 0;
	bool closed = fclose(file) == 0;
	if (!written || !flushed || !closed) {
		remove(temporary);
		return -EIO;
	}
	if (rename(temporary, path) != 0) {
		remove(temporary);
		return -EIO;
	}
	return 0;
}

uint64_t watch_cache_storage_now_ms(void)
{
	time_t now = time(NULL);
	if (now != (time_t)-1) return (uint64_t)now * 1000U;
	return (uint64_t)k_uptime_get();
}

#else

int watch_cache_storage_load(struct watch_cache_record *cache)
{
	ARG_UNUSED(cache);
	return -ENOTSUP;
}

int watch_cache_storage_save(const struct watch_cache_record *cache)
{
	ARG_UNUSED(cache);
	return -ENOTSUP;
}

uint64_t watch_cache_storage_now_ms(void)
{
	return (uint64_t)k_uptime_get();
}

#endif
