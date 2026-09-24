/* SPDX-License-Identifier: Apache-2.0 */
#include "cache/watch_cache.h"
static struct watch_cache_metadata metadata;

void watch_cache_init(void)
{
	metadata = (struct watch_cache_metadata) {
		.version = 1,
		.updated_at_ms = 0,
		.stale = true,
	};
}

const struct watch_cache_metadata *watch_cache_status(void)
{
	return &metadata;
}
