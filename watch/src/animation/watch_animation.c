/* SPDX-License-Identifier: Apache-2.0 */
#include "animation/watch_animation.h"

void watch_animation_load(lv_obj_t *screen, bool backwards)
{
	lv_screen_load_anim(screen,
		backwards ? LV_SCR_LOAD_ANIM_MOVE_RIGHT : LV_SCR_LOAD_ANIM_MOVE_LEFT,
		260, 0, true);
}
