/* SPDX-License-Identifier: Apache-2.0 */
#include "apps/app_registry.h"

static const struct watch_app_descriptor apps[] = {
#include "apps/app_registry.generated.inc"
};

size_t app_registry_count(void)
{
	return sizeof(apps) / sizeof(apps[0]);
}

const struct watch_app_descriptor *app_registry_at(size_t index)
{
	return index < app_registry_count() ? &apps[index] : NULL;
}

const struct watch_app_descriptor *app_registry_find(enum watch_screen screen)
{
	for (size_t index = 0; index < app_registry_count(); ++index) {
		if (apps[index].screen == screen) {
			return &apps[index];
		}
	}
	return NULL;
}
