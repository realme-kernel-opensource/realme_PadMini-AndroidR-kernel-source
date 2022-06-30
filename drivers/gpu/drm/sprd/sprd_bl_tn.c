/*
 *Copyright (C) 2019 Spreadtrum Communications Inc.
 *
 *This software is licensed under the terms of the GNU General Public
 *License version 2, as published by the Free Software Foundation, and
 *may be copied, distributed, and modified under those terms.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 */

#define pr_fmt(fmt) "sprd-backlight-tn: " fmt

#include <linux/backlight.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#include "sprd_bl.h"

#define U_MAX_LEVEL	2047
#define U_MIN_LEVEL	0

void sprd_backlight_normalize_map(struct backlight_device *bd, u16 *level)
{
		*level = bd->props.brightness;

}

int sprd_cabc_backlight_update(struct backlight_device *bd)
{
	int ret = -ENOENT;
	struct sprd_backlight *bl = bl_get_data(bd);

	mutex_lock(&bd->update_lock);
	if (bd->ops && bd->ops->update_status_cabc)
		ret = bd->ops->update_status_cabc(bd);
	mutex_unlock(&bd->update_lock);
	pr_err("cabc brightness level: %u\n", bl->cabc_level);


	return ret;

}



