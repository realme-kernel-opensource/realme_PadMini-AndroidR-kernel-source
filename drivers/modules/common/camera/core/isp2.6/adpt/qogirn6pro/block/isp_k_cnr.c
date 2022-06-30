/*
 * Copyright (C) 2020-2021 UNISOC Communications Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/uaccess.h>
#include <sprd_mm.h>

#include "isp_hw.h"
#include "isp_reg.h"
#include "cam_types.h"
#include "cam_block.h"
#include "isp_core.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "CNR: %d %d %s : "fmt, current->pid, __LINE__, __func__

static int isp_k_cnr_block(struct isp_io_param *param,
	struct isp_k_block *isp_k_param, uint32_t idx)
{
	int ret = 0;
	struct isp_dev_cnr_h_info *cnr = NULL;

	cnr = &isp_k_param->cnr_info;
	ret = copy_from_user((void *)cnr, param->property_param,
			sizeof(struct isp_dev_cnr_h_info));
	if (ret != 0) {
		pr_err("fail to copy from user, ret = %d\n", ret);
		return ret;
	}

	return ret;
}

int isp_k_cfg_cnr(struct isp_io_param *param,
	struct isp_k_block *isp_k_param, uint32_t idx)
{
	int ret = 0;

	switch (param->property) {
	case ISP_PRO_CNR_H_BLOCK:
		ret = isp_k_cnr_block(param, isp_k_param, idx);
		break;
	default:
		pr_err("fail to support cmd id = %d\n", param->property);
		break;
	}

	return ret;
}

int isp_k_update_cnr(void *handle)
{
	int ret = 0;
	uint32_t radius = 0, radius_limit = 0;
	uint32_t idx = 0, new_width = 0, old_width = 0, sensor_width = 0;
	uint32_t new_height = 0, old_height = 0, sensor_height = 0;
	struct isp_dev_cnr_h_info *cnr_info_h = NULL;
	struct isp_cnr_h_info *param_cnr_info = NULL;
	struct isp_sw_context *pctx = NULL;

	if (!handle) {
		pr_err("fail to get invalid in ptr\n");
		return -EFAULT;
	}

	pctx = (struct isp_sw_context *)handle;
	idx = pctx->ctx_id;
	new_width = pctx->isp_k_param.blkparam_info.new_width;
	new_height = pctx->isp_k_param.blkparam_info.new_height;
	old_width = pctx->isp_k_param.blkparam_info.old_width;
	old_height = pctx->isp_k_param.blkparam_info.old_height;
	sensor_width = pctx->uinfo.sn_size.w;
	sensor_height = pctx->uinfo.sn_size.h;

	cnr_info_h = &pctx->isp_k_param.cnr_info;
	param_cnr_info = &cnr_info_h->layer_cnr_h[0];

	if (cnr_info_h->bypass)
		return 0;

	if (cnr_info_h->radius_base == 0)
		cnr_info_h->radius_base = 1024;

	radius = ((sensor_width + sensor_height) / 4) * param_cnr_info->base_radius_factor / cnr_info_h->radius_base;
	radius_limit = (new_height + new_width) / 4;
	radius = (radius < radius_limit) ? radius : radius_limit;
	radius = new_height * radius / old_height;

	pctx->isp_k_param.cnr_radius = radius;
	pr_debug("base %d, factor %d, radius %d\n", cnr_info_h->radius_base, param_cnr_info->base_radius_factor, radius);

	return ret;
}
