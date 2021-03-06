// SPDX-License-Identifier: GPL-2.0-only
/*
 * Unisoc sipa driver
 *
 * Copyright (C) 2020 Unisoc, Inc.
 * Author: Qingsheng Li <qingsheng.li@unisoc.com>
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/sipa.h>
#include <linux/inet.h>
#include <linux/netdevice.h>
#include <linux/unistd.h>

#include "sipa_rm_res.h"
#include "sipa_debug.h"
#include "sipa_priv.h"
#include "sipa_hal.h"

static int sipa_flow_ctrl_show(struct seq_file *s, void *unused)
{
	struct sipa_plat_drv_cfg *ipa = s->private;

	if (ipa->is_bypass) {
		int i;

		for (i = 0; i < SIPA_FIFO_MAX; i++)
			seq_printf(s, "fifo %d enter cnt = %u exit cnt = %u\n",
				   i, ipa->cmn_fifo_cfg[i].enter_flow_ctrl_cnt,
				   ipa->cmn_fifo_cfg[i].exit_flow_ctrl_cnt);
		return 0;
	}

	seq_printf(s, "[SENDER] left_cnt = %d\n",
		   atomic_read(&ipa->sender->left_cnt));

	seq_printf(s, "[SENDER] no_mem_cnt = %d no_free_cnt = %d\n",
		   ipa->sender->no_mem_cnt, ipa->sender->no_free_cnt);

	seq_printf(s, "[SENDER] enter_flow_ctrl_cnt = %d\n",
		   ipa->sender->enter_flow_ctrl_cnt);

	seq_printf(s, "[SENDER] exit_flow_ctrl_cnt = %d\n",
		   ipa->sender->exit_flow_ctrl_cnt);

	seq_printf(s, "[RECEIVER] tx_danger_cnt = %d rx_danger_cnt = %d\n",
		   ipa->receiver->tx_danger_cnt, ipa->receiver->rx_danger_cnt);

	return 0;
}

static int sipa_flow_ctrl_open(struct inode *inode, struct file *file)
{
	return single_open(file, sipa_flow_ctrl_show, inode->i_private);
}

static const struct file_operations sipa_flow_ctrl_fops = {
	.open = sipa_flow_ctrl_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sipa_no_recycled_show(struct seq_file *s, void *unused)
{
	struct sipa_plat_drv_cfg *ipa = s->private;
	struct sipa_skb_dma_addr_pair *iter, *_iter;

	if (list_empty(&ipa->sender->sending_list)) {
		seq_puts(s, "[SENDER] sending list is empty\n");
	} else {
		list_for_each_entry_safe(iter, _iter,
					 &ipa->sender->sending_list, list)
			seq_printf(s,
				   "[SENDER] skb = 0x%p, phy addr = 0x%llx\n",
				   iter->skb, (u64)iter->dma_addr);
	}

	return 0;
}

static int sipa_no_recycled_open(struct inode *inode, struct file *file)
{
	return single_open(file, sipa_no_recycled_show, inode->i_private);
}

static const struct file_operations sipa_no_recycled_fops = {
	.open = sipa_no_recycled_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sipa_nic_debug_show(struct seq_file *s, void *unused)
{
	int i;
	struct sipa_plat_drv_cfg *ipa = s->private;

	seq_printf(s, "suspend_stage = 0x%x rc = %d sc = %d pf = %d\n",
		   ipa->suspend_stage, ipa->resume_cnt,
		   ipa->suspend_cnt, ipa->power_flag);
	seq_printf(s, "mode_state = 0x%x is_bypass = 0x%x\n",
		   ipa->mode_state, ipa->is_bypass);

	for (i = 0; i < SIPA_NIC_MAX; i++) {
		if (!ipa->nic[i])
			continue;

		seq_printf(s, "open = %d src_mask = 0x%x netid = %d fcs = %d",
			   atomic_read(&ipa->nic[i]->status),
			   ipa->nic[i]->src_mask,
			   ipa->nic[i]->netid,
			   ipa->nic[i]->flow_ctrl_status);
		seq_printf(s, "rm_flow_ctrl = %d rr = %d rw = %d\n",
			   ipa->nic[i]->rm_res.rm_flow_ctrl,
			   ipa->nic[i]->rm_res.resource_requested,
			   ipa->nic[i]->rm_res.reschedule_work);
		seq_printf(s, "release_in_progress = %d nrt = %d rip = %d\n",
			   ipa->nic[i]->rm_res.release_in_progress,
			   ipa->nic[i]->rm_res.need_request,
			   ipa->nic[i]->rm_res.request_in_progress);
	}

	return 0;
}

static int sipa_nic_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, sipa_nic_debug_show, inode->i_private);
}

static const struct file_operations sipa_nic_debug_fops = {
	.open = sipa_nic_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sipa_rm_res_debug_show(struct seq_file *s, void *unused)
{
	int i;
	struct sipa_rm_resource **res = sipa_rm_get_all_resource();

	seq_puts(s, "/********************************************\n");
	seq_puts(s, "type: 0 -> producer  1 -> consumer\n");
	seq_puts(s, "state: 0 -> released 1 -> request_in_progress\n");
	seq_puts(s, "state: 2 -> granted 3 -> release_in_progress\n");
	seq_puts(s, "*********************************************/\n");

	for (i = 0; i < SIPA_RM_RES_MAX; i++) {
		if (!res[i])
			continue;

		seq_printf(s, "[res] name = %s type = %d ref_count = %d state = %d\n",
			   sipa_rm_res_str(res[i]->name), res[i]->type,
			   res[i]->ref_count, res[i]->state);
	}

	return 0;
}

static int sipa_rm_res_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, sipa_rm_res_debug_show, inode->i_private);
}

static const struct file_operations sipa_rm_res_debug_fops = {
	.open = sipa_rm_res_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sipa_fifo_cfg_show(struct seq_file *s, void *unused)
{
	int i;
	struct sipa_plat_drv_cfg *ipa = s->private;

	for (i = 0; i < SIPA_FIFO_MAX; i++) {
		struct sipa_cmn_fifo_cfg_tag *fifo_cfg = &ipa->cmn_fifo_cfg[i];

		seq_printf(s, "id = %d [RX] iram = 0x%x d = 0x%x v_addr = %p\n",
			   i, fifo_cfg->rx_fifo.in_iram,
			   fifo_cfg->rx_fifo.depth,
			   fifo_cfg->rx_fifo.virt_addr);
		seq_printf(s, "id = %d [TX] iram = 0x%x d = 0x%x v_addr = %p\n",
			   i, fifo_cfg->tx_fifo.in_iram,
			   fifo_cfg->tx_fifo.depth,
			   fifo_cfg->tx_fifo.virt_addr);
	}

	return 0;
}

static int sipa_fifo_cfg_open(struct inode *inode, struct file *file)
{
	return single_open(file, sipa_fifo_cfg_show, inode->i_private);
}

static const struct file_operations sipa_fifo_cfg_fops = {
	.open = sipa_fifo_cfg_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

int sipa_init_debugfs(struct sipa_plat_drv_cfg *ipa)
{
	struct dentry *root;
	struct dentry *file;
	int ret;

	root = debugfs_create_dir(dev_name(ipa->dev), NULL);
	if (!root)
		return -ENOMEM;

	file = debugfs_create_file("flow_ctrl", 0444, root, ipa,
				   &sipa_flow_ctrl_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("nic", 0444, root, ipa,
				   &sipa_nic_debug_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("no_recycled", 0444, root, ipa,
				   &sipa_no_recycled_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("rm_res", 0444, root, ipa,
				   &sipa_rm_res_debug_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	file = debugfs_create_file("fifo_cfg", 0444, root, ipa,
				   &sipa_fifo_cfg_fops);
	if (!file) {
		ret = -ENOMEM;
		goto err1;
	}

	debugfs_create_symlink("sipa", NULL,
			       "/sys/kernel/debug/25220000.sipa");

	ipa->debugfs_root = root;

	return 0;

err1:
	debugfs_remove_recursive(root);

	return ret;
}

void sipa_exit_debugfs(struct sipa_plat_drv_cfg *sipa)
{
	debugfs_remove_recursive(sipa->debugfs_root);
}
