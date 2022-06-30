/*
 * Copyright (C) 2021 Unisoc Communications Inc.
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

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/resource.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/wait.h>

#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/jiffies.h>
#include "vdsp_mailbox_drv.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "sprd-vdsp: vdsp_mailbox %d %d %s : "\
        fmt, current->pid, __LINE__, __func__

static struct mbox_dts_cfg_tag mbox_dts_cfg;
struct mbox_device_tag *mbox_ops;
static u8 g_mbox_inited;

static spinlock_t mbox_lock;

static int vdsp_mbox_register_irq_handle(u8 target_id,
		MBOX_FUNCALL irq_handler,
		void *priv_data)
{
	unsigned long flags;
	int ret;

	pr_debug("mbox:%s,target_id =%d\n", __func__, target_id);

	if (!g_mbox_inited) {
		pr_err("mbox:ERR on line %d!\n", __LINE__);
		return -EINVAL;
	}

	spin_lock_irqsave(&mbox_lock, flags);

	ret = mbox_ops->fops->phy_register_irq_handle(target_id, irq_handler, priv_data);

	spin_unlock_irqrestore(&mbox_lock, flags);
	pr_debug("mbox:ret:%d", ret);

	if (ret < 0)
		return ret;

	/* must do it, Ap may be have already revieved some msgs */
	mbox_ops->fops->process_bak_msg();

	return ret;
}

static int vdsp_mbox_unregister_irq_handle(u8 target_id)
{
	int ret;

	if (!g_mbox_inited) {
		pr_err("mbox:ERR on line %d!\n", __LINE__);
		return -EINVAL;
	}

	spin_lock(&mbox_lock);

	ret = mbox_ops->fops->phy_unregister_irq_handle(target_id);

	spin_unlock(&mbox_lock);

	return ret;
}

static irqreturn_t recv_irq_handler(int irq , void* dev)
{
	pr_debug("func:%s enter irq:%d\n" , __func__ , irq);
	return mbox_ops->fops->recv_irqhandle(irq , dev);
}

static int vdsp_mbox_send_irq(u8 core_id, uint64_t msg)
{
	return mbox_ops->fops->phy_send(core_id, msg);
}

static int mbox_parse_dts(void)
{
	int ret;
	struct device_node *np;
	//u32 syscon_args[2];

	np = of_find_compatible_node(NULL, NULL, "sprd,vdsp-mailbox");
	if (!np) {
		pr_err("mbox: can't find %s!\n", "sprd,vdsp-mailbox");
		return(-EINVAL);
	}

	mbox_dts_cfg.gpr = syscon_regmap_lookup_by_name(np, "clk");

	/* parse inbox base */
	of_address_to_resource(np, 0, &mbox_dts_cfg.inboxres);
	pr_debug("func:%s inborxres:%d\n", __func__, mbox_dts_cfg.inboxres);
	/* parse outbox base */
	of_address_to_resource(np, 1, &mbox_dts_cfg.outboxres);

	/* parse core cnt */
	ret = of_property_read_u32(np, "sprd,vdsp-core-cnt", &mbox_dts_cfg.core_cnt);
	if (ret) {
		pr_err("mbox:ERR on line %d!\n", __LINE__);
		return -EINVAL;
	}
	/* parse mbox version */
	ret = of_property_read_u32(np, "sprd,vdsp-version", &mbox_dts_cfg.version);
	if (ret) {
		pr_debug("mbox: not found version!\n");
		mbox_dts_cfg.version = 2;
	}

	return 0;
}

static int vdsp_mbox_init(void)
{
	int ret;

	pr_debug("mbox:%s!\n", __func__);

	ret = mbox_parse_dts();
	if (ret != 0){
		pr_err("mbox_parse_dts failed:%d", ret);
		return -EINVAL;
	}

	mbox_get_phy_device(&mbox_ops);

	spin_lock_init(&mbox_lock);

	ret = mbox_ops->fops->cfg_init(&mbox_dts_cfg, &g_mbox_inited);
	if (ret != 0) {
		pr_err("cfg_init failed:%d", ret);
		return -EINVAL;
	}
	return 0;
}

static int vdsp_mbox_enable(struct vdsp_mbox_ctx_desc *ctx)
{
	return mbox_ops->fops->enable(ctx);
}

static int vdsp_mbox_disable(struct vdsp_mbox_ctx_desc *ctx)
{
	return mbox_ops->fops->disable(ctx);
}

struct vdsp_mbox_ops vdsp_mbox_ops = {
		.ctx_init = vdsp_mbox_enable,
		.ctx_deinit = vdsp_mbox_disable,
		.irq_handler = recv_irq_handler,
		.irq_register = vdsp_mbox_register_irq_handle,
		.irq_unregister = vdsp_mbox_unregister_irq_handle,
		.irq_send = vdsp_mbox_send_irq,
		.mbox_init = vdsp_mbox_init,
};

static struct vdsp_mbox_ctx_desc s_mbox_desc = {
		.ops = &vdsp_mbox_ops,
};

struct vdsp_mbox_ctx_desc *get_vdsp_mbox_ctx_desc(void)
{
	return &s_mbox_desc;
}

int vdsp_commu_hw_resume(void)
{
	return vdsp_mbox_enable(&s_mbox_desc);
}

int vdsp_commu_hw_suspend(void)
{
	return vdsp_mbox_disable(&s_mbox_desc);
}


