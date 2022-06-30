/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */



#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/hwspinlock.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <mali_kbase_debug.h>
//For fix r24p0 --> r27p0 compile error.
//#include <backend/gpu/mali_kbase_device_internal.h>
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/of_address.h>
#ifdef KBASE_PM_RUNTIME
#include <linux/pm_runtime.h>
#endif
#ifdef CONFIG_OF
#include <linux/of.h>
#endif
#include <linux/regulator/consumer.h>

#include <linux/smp.h>
#include <trace/events/power.h>
#include <linux/soc/sprd/hwfeature.h>
#include <linux/nvmem-consumer.h>
#include <gpu,qogirn6pro-regs.h>
#include <gpu,qogirn6pro-mask.h>

#define VENDOR_FTRACE_MODULE_NAME    "unisoc-gpu"

#define DTS_CLK_OFFSET      2
#define PM_RUNTIME_DELAY_MS 50
#define UP_THRESHOLD        9/10
#define FREQ_KHZ            1000
#define GPU_MIN_FREQ_INDEX  3
#define GPU_26M_FREQ        26000000
#define GPU_76M8_FREQ       76800000
#define GPU_153M6_FREQ      153600000

#if 0
struct gpu_qos_config {
	u8 arqos;
	u8 awqos;
	u8 arqos_threshold;
	u8 awqos_threshold;
};
#endif

struct gpu_freq_info {
	struct clk* clk_src;
	int freq;	//kHz
	int volt;	//uV
	int dvfs_index;
};

struct gpu_reg_info {
	struct regmap* regmap_ptr;
	uint32_t args[2];
};

struct gpu_dvfs_context {
	int gpu_clock_on;
	int gpu_power_on;
	int cur_voltage;

	struct clk*  clk_gpu_i;
	struct clk*  clk_gpu_core_eb;
	struct clk** gpu_clk_src;
	int gpu_clk_num;

	struct gpu_freq_info* freq_list;
	int freq_list_len;

	int cur_index;
	const struct gpu_freq_info* freq_cur;
	const struct gpu_freq_info* freq_default;

	struct semaphore* sem;

	struct gpu_reg_info top_dvfs_cfg_reg;
	struct gpu_reg_info gpu_sw_dvfs_ctrl_reg;
	struct gpu_reg_info top_force_reg;
	struct gpu_reg_info soft_rst_reg;
#if 0
	struct gpu_reg_info gpu_qos_sel;
	struct gpu_reg_info gpu_qos;
#endif
	struct gpu_reg_info dvfs_index_cfg;
	struct gpu_reg_info sw_dvfs_ctrl;
	struct gpu_reg_info freq_upd_cfg;

	struct regmap* gpu_apb_base_ptr;
	struct regmap* gpu_dvfs_apb_base_ptr;

	struct regulator *gpu_reg_ptr;
};

DEFINE_SEMAPHORE(gpu_dfs_sem);
static struct gpu_dvfs_context gpu_dvfs_ctx=
{
	.gpu_clock_on=0,
	.gpu_power_on=0,

	.sem=&gpu_dfs_sem,
};

#if 0
static struct gpu_qos_config gpu_qos_cfg=
{
	.arqos=0,
	.awqos=0,
	.arqos_threshold=0,
	.awqos_threshold=0,
};
#endif

#ifdef CONFIG_MALI_DEVFREQ
static void InitFreqStats(struct kbase_device *kbdev)
{
	int i = 0;

	kbdev->enable_freq_stats = 0;
	kbdev->freq_num = gpu_dvfs_ctx.freq_list_len;
	kbdev->freq_stats = vmalloc(sizeof(struct kbase_devfreq_stats) * kbdev->freq_num);
	KBASE_DEBUG_ASSERT(kbdev->freq_stats);

	for (i = 0; i < kbdev->freq_num; i++)
	{
		kbdev->freq_stats[i].freq = gpu_dvfs_ctx.freq_list[i].freq * FREQ_KHZ;
		kbdev->freq_stats[i].busy_time = 0;
		kbdev->freq_stats[i].total_time = 0;
	}
}

static void DeinitFreqStats(struct kbase_device *kbdev)
{
	if (NULL != kbdev->freq_stats)
	{
		vfree(kbdev->freq_stats);
		kbdev->freq_stats = NULL;
	}
}

#endif

static inline void mali_freq_init(struct device *dev)
{
	int i = 0, clk_cnt = 0;
	//struct device_node *qos_node = NULL;

	gpu_dvfs_ctx.gpu_reg_ptr = devm_regulator_get(dev, "gpu");
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.gpu_reg_ptr);

	gpu_dvfs_ctx.top_dvfs_cfg_reg.regmap_ptr = syscon_regmap_lookup_by_name(dev->of_node,"top_dvfs_cfg");
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.top_dvfs_cfg_reg.regmap_ptr);
	syscon_get_args_by_name(dev->of_node,"top_dvfs_cfg", 2, (uint32_t *)gpu_dvfs_ctx.top_dvfs_cfg_reg.args);

	gpu_dvfs_ctx.gpu_sw_dvfs_ctrl_reg.regmap_ptr = syscon_regmap_lookup_by_name(dev->of_node,"gpu_sw_dvfs_ctrl");
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.gpu_sw_dvfs_ctrl_reg.regmap_ptr);
	syscon_get_args_by_name(dev->of_node,"gpu_sw_dvfs_ctrl", 2, (uint32_t *)gpu_dvfs_ctx.gpu_sw_dvfs_ctrl_reg.args);

	//enable gpu hw dvfs
	regmap_update_bits(gpu_dvfs_ctx.top_dvfs_cfg_reg.regmap_ptr, gpu_dvfs_ctx.top_dvfs_cfg_reg.args[0], gpu_dvfs_ctx.top_dvfs_cfg_reg.args[1], ~gpu_dvfs_ctx.top_dvfs_cfg_reg.args[1]);
	regmap_update_bits(gpu_dvfs_ctx.gpu_sw_dvfs_ctrl_reg.regmap_ptr, gpu_dvfs_ctx.gpu_sw_dvfs_ctrl_reg.args[0], gpu_dvfs_ctx.gpu_sw_dvfs_ctrl_reg.args[1], ~gpu_dvfs_ctx.gpu_sw_dvfs_ctrl_reg.args[1]);

	gpu_dvfs_ctx.top_force_reg.regmap_ptr = syscon_regmap_lookup_by_name(dev->of_node,"top_force_shutdown");
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.top_force_reg.regmap_ptr);
	syscon_get_args_by_name(dev->of_node,"top_force_shutdown", 2, (uint32_t *)gpu_dvfs_ctx.top_force_reg.args);

	gpu_dvfs_ctx.soft_rst_reg.regmap_ptr = syscon_regmap_lookup_by_name(dev->of_node,"gpu_soft_rst");
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.soft_rst_reg.regmap_ptr);
	syscon_get_args_by_name(dev->of_node,"gpu_soft_rst", 2, (uint32_t *)gpu_dvfs_ctx.soft_rst_reg.args);

	gpu_dvfs_ctx.gpu_apb_base_ptr = syscon_regmap_lookup_by_phandle(dev->of_node, "sprd,gpu-apb-syscon");
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.gpu_apb_base_ptr);
	gpu_dvfs_ctx.gpu_dvfs_apb_base_ptr = syscon_regmap_lookup_by_phandle(dev->of_node, "sprd,gpu-dvfs-apb-syscon");
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.gpu_dvfs_apb_base_ptr);

#if 0
	//qos
	gpu_dvfs_ctx.gpu_qos_sel.regmap_ptr = gpu_dvfs_ctx.gpu_apb_base_ptr;
	gpu_dvfs_ctx.gpu_qos_sel.args[0] = REG_GPU_APB_RF_GPU_NIC400_QOS;
	gpu_dvfs_ctx.gpu_qos_sel.args[1] = MASK_GPU_APB_RF_GPU_QOS_SEL;

	gpu_dvfs_ctx.gpu_qos.regmap_ptr = gpu_dvfs_ctx.gpu_apb_base_ptr;
	gpu_dvfs_ctx.gpu_qos.args[0] = REG_GPU_APB_RF_GPU_NIC400_QOS;
	gpu_dvfs_ctx.gpu_qos.args[1] = MASK_GPU_APB_RF_AWQOS_THRESHOLD_GPU | MASK_GPU_APB_RF_ARQOS_THRESHOLD_GPU | MASK_GPU_APB_RF_AWQOS_GPU | MASK_GPU_APB_RF_ARQOS_GPU;

	/* qos dts parse */
	qos_node = of_parse_phandle(dev->of_node, "sprd,qos", 0);
	if (qos_node)
	{
		if (of_property_read_u8(qos_node, "arqos", &gpu_qos_cfg.arqos)) {
			pr_warn("gpu arqos config reading fail.\n");
		}
		if (of_property_read_u8(qos_node, "awqos", &gpu_qos_cfg.awqos)) {
			pr_warn("gpu awqos config reading fail.\n");
		}
		if (of_property_read_u8(qos_node, "arqos-threshold", &gpu_qos_cfg.arqos_threshold)) {
			pr_warn("gpu arqos_threshold config reading fail.\n");
		}
		if (of_property_read_u8(qos_node, "awqos-threshold", &gpu_qos_cfg.awqos_threshold)) {
			pr_warn("gpu awqos_threshold config reading fail.\n");
		}
	} else {
		pr_warn("can't find gpu qos config node\n");
	}
#endif

	//gpu index cfg
	gpu_dvfs_ctx.dvfs_index_cfg.regmap_ptr = gpu_dvfs_ctx.gpu_dvfs_apb_base_ptr;
	gpu_dvfs_ctx.dvfs_index_cfg.args[0] = REG_GPU_DVFS_APB_RF_GPU_DVFS_INDEX_CFG;
	gpu_dvfs_ctx.dvfs_index_cfg.args[1] = MASK_GPU_DVFS_APB_RF_GPU_DVFS_INDEX;

	//sw dvfs ctrl
	gpu_dvfs_ctx.sw_dvfs_ctrl.regmap_ptr = gpu_dvfs_ctx.gpu_dvfs_apb_base_ptr;
	gpu_dvfs_ctx.sw_dvfs_ctrl.args[0] = REG_GPU_DVFS_APB_RF_GPU_SW_DVFS_CTRL;
	gpu_dvfs_ctx.sw_dvfs_ctrl.args[1] = MASK_GPU_DVFS_APB_RF_GPU_DVFS_ACK | MASK_GPU_DVFS_APB_RF_GPU_DVFS_VOLTAGE_SW | MASK_GPU_DVFS_APB_RF_GPU_DVFS_REQ_SW;

	//freq update cfg
	gpu_dvfs_ctx.freq_upd_cfg.regmap_ptr = gpu_dvfs_ctx.gpu_dvfs_apb_base_ptr;
	gpu_dvfs_ctx.freq_upd_cfg.args[0] = REG_GPU_DVFS_APB_RF_GPU_FREQ_UPD_TYPE_CFG0;
	gpu_dvfs_ctx.freq_upd_cfg.args[1] = MASK_GPU_DVFS_APB_RF_GPU_FREQ_UPD_HDSK_EN | MASK_GPU_DVFS_APB_RF_GPU_FREQ_UPD_DELAY_EN;

	gpu_dvfs_ctx.clk_gpu_i = of_clk_get(dev->of_node, 0);
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.clk_gpu_i);

	gpu_dvfs_ctx.clk_gpu_core_eb = of_clk_get(dev->of_node, 1);
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.clk_gpu_core_eb);

	clk_cnt = of_clk_get_parent_count(dev->of_node);
	gpu_dvfs_ctx.gpu_clk_num = clk_cnt - DTS_CLK_OFFSET;

	gpu_dvfs_ctx.gpu_clk_src = vmalloc(sizeof(struct clk*) * gpu_dvfs_ctx.gpu_clk_num);
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.gpu_clk_src);

	for (i = 0; i < gpu_dvfs_ctx.gpu_clk_num; i++)
	{
		gpu_dvfs_ctx.gpu_clk_src[i] = of_clk_get(dev->of_node, i+DTS_CLK_OFFSET);
		KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.gpu_clk_src[i]);
	}

	gpu_dvfs_ctx.freq_list_len = of_property_count_elems_of_size(dev->of_node,"operating-points",2*sizeof(u32));
	gpu_dvfs_ctx.freq_list = vmalloc(sizeof(struct gpu_freq_info) * gpu_dvfs_ctx.freq_list_len);
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.freq_list);

	for(i=0; i<gpu_dvfs_ctx.freq_list_len; i++)
	{
		gpu_dvfs_ctx.freq_list[i].clk_src = gpu_dvfs_ctx.gpu_clk_src[i];
		KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.freq_list[i].clk_src);
		of_property_read_u32_index(dev->of_node, "operating-points", 2*i,   &gpu_dvfs_ctx.freq_list[i].freq);
		of_property_read_u32_index(dev->of_node, "operating-points", 2*i+1, &gpu_dvfs_ctx.freq_list[i].volt);
		gpu_dvfs_ctx.freq_list[i].dvfs_index = i;
	}

	of_property_read_u32(dev->of_node, "sprd,dvfs-default", &i);
	gpu_dvfs_ctx.freq_default = &gpu_dvfs_ctx.freq_list[i];
	KBASE_DEBUG_ASSERT(gpu_dvfs_ctx.freq_default);

	gpu_dvfs_ctx.cur_index = GPU_MIN_FREQ_INDEX;
	gpu_dvfs_ctx.freq_cur = &gpu_dvfs_ctx.freq_list[GPU_MIN_FREQ_INDEX];
	gpu_dvfs_ctx.cur_voltage = gpu_dvfs_ctx.freq_cur->volt;
}

static inline void mali_power_on(void)
{
	//gpu regulator enable
	if (!regulator_is_enabled(gpu_dvfs_ctx.gpu_reg_ptr))
	{
		regulator_enable(gpu_dvfs_ctx.gpu_reg_ptr);
		udelay(400);
	}

	regmap_update_bits(gpu_dvfs_ctx.top_force_reg.regmap_ptr, gpu_dvfs_ctx.top_force_reg.args[0], gpu_dvfs_ctx.top_force_reg.args[1], ~gpu_dvfs_ctx.top_force_reg.args[1]);

	//reset gpu sys
	regmap_update_bits(gpu_dvfs_ctx.soft_rst_reg.regmap_ptr, gpu_dvfs_ctx.soft_rst_reg.args[0], gpu_dvfs_ctx.soft_rst_reg.args[1], gpu_dvfs_ctx.soft_rst_reg.args[1]);
	regmap_update_bits(gpu_dvfs_ctx.soft_rst_reg.regmap_ptr, gpu_dvfs_ctx.soft_rst_reg.args[0], gpu_dvfs_ctx.soft_rst_reg.args[1], ~gpu_dvfs_ctx.soft_rst_reg.args[1]);
	udelay(100);

	gpu_dvfs_ctx.gpu_power_on = 1;
}


static inline void mali_power_off(void)
{
	gpu_dvfs_ctx.gpu_power_on = 0;

	regmap_update_bits(gpu_dvfs_ctx.top_force_reg.regmap_ptr, gpu_dvfs_ctx.top_force_reg.args[0], gpu_dvfs_ctx.top_force_reg.args[1], gpu_dvfs_ctx.top_force_reg.args[1]);

	//gpu regulator disable
	if (regulator_is_enabled(gpu_dvfs_ctx.gpu_reg_ptr))
	{
		regulator_disable(gpu_dvfs_ctx.gpu_reg_ptr);
		udelay(10);
	}
}

#if 0
static void maliQosConfig(void)
{
	regmap_update_bits(gpu_dvfs_ctx.gpu_qos_sel.regmap_ptr, gpu_dvfs_ctx.gpu_qos_sel.args[0], gpu_dvfs_ctx.gpu_qos_sel.args[1], gpu_dvfs_ctx.gpu_qos_sel.args[1]);
	regmap_update_bits(gpu_dvfs_ctx.gpu_qos.regmap_ptr, gpu_dvfs_ctx.gpu_qos.args[0], gpu_dvfs_ctx.gpu_qos.args[1], ((gpu_qos_cfg.awqos_threshold << 12) | (gpu_qos_cfg.arqos_threshold << 8) | (gpu_qos_cfg.awqos << 4) | gpu_qos_cfg.arqos));
}
#endif

static inline void mali_clock_on(void)
{
	int i;

	//enable all clocks
	for(i=0;i<gpu_dvfs_ctx.gpu_clk_num;i++)
	{
		clk_prepare_enable(gpu_dvfs_ctx.gpu_clk_src[i]);
	}
	clk_prepare_enable(gpu_dvfs_ctx.clk_gpu_i);


	//enable gpu clock
	clk_prepare_enable(gpu_dvfs_ctx.clk_gpu_core_eb);
	udelay(200);

	//update freq cfg
	regmap_update_bits(gpu_dvfs_ctx.freq_upd_cfg.regmap_ptr, gpu_dvfs_ctx.freq_upd_cfg.args[0], gpu_dvfs_ctx.freq_upd_cfg.args[1], 1);

	//init freq
	regmap_update_bits(gpu_dvfs_ctx.dvfs_index_cfg.regmap_ptr, gpu_dvfs_ctx.dvfs_index_cfg.args[0], gpu_dvfs_ctx.dvfs_index_cfg.args[1], gpu_dvfs_ctx.freq_list[gpu_dvfs_ctx.cur_index].dvfs_index);

#if 0
	//qos
	maliQosConfig();
#endif

	gpu_dvfs_ctx.gpu_clock_on = 1;
}

static inline void mali_clock_off(void)
{
	int i;

	gpu_dvfs_ctx.gpu_clock_on = 0;

	//disable gpu clock
	clk_disable_unprepare(gpu_dvfs_ctx.clk_gpu_core_eb);
	clk_disable_unprepare(gpu_dvfs_ctx.clk_gpu_i);

	//disable all clocks
	for(i=0;i<gpu_dvfs_ctx.gpu_clk_num;i++)
	{
		clk_disable_unprepare(gpu_dvfs_ctx.gpu_clk_src[i]);
	}
}

static int mali_platform_init(struct kbase_device *kbdev)
{
	//gpu freq
	mali_freq_init(kbdev->dev);

#ifdef CONFIG_MALI_DEVFREQ
	InitFreqStats(kbdev);
	kbase_pm_statistics_FreqInit(kbdev);
#endif

	mali_power_on();

	//clock on
	mali_clock_on();

	return 0;
}

static void mali_platform_term(struct kbase_device *kbdev)
{
	down(gpu_dvfs_ctx.sem);

	//clock off
	mali_clock_off();

	//power off
	mali_power_off();

#ifdef CONFIG_MALI_DEVFREQ
	kbase_pm_statistics_FreqDeinit(kbdev);
	DeinitFreqStats(kbdev);
#endif

	//free
	vfree(gpu_dvfs_ctx.freq_list);
	vfree(gpu_dvfs_ctx.gpu_clk_src);

	up(gpu_dvfs_ctx.sem);
}

struct kbase_platform_funcs_conf platform_qogirn6pro_funcs = {
	.platform_init_func = mali_platform_init,
	.platform_term_func = mali_platform_term
};

static void mali_power_mode_change(struct kbase_device *kbdev, int power_mode)
{
	down(gpu_dvfs_ctx.sem);
	//dev_info(kbdev->dev, "mali_power_mode_change: %d, gpu_power_on=%d gpu_clock_on=%d",power_mode,gpu_dvfs_ctx.gpu_power_on,gpu_dvfs_ctx.gpu_clock_on);
	switch (power_mode)
	{
		case 0://power on
			if (!gpu_dvfs_ctx.gpu_power_on)
			{
				mali_power_on();
				mali_clock_on();
			}

			if (!gpu_dvfs_ctx.gpu_clock_on)
			{
				mali_clock_on();
			}
			break;

		case 1://light sleep
		case 2://deep sleep
			if(gpu_dvfs_ctx.gpu_clock_on)
			{
				mali_clock_off();
			}

			if(gpu_dvfs_ctx.gpu_power_on)
			{
				mali_power_off();
			}
			break;

		default:
			break;
	}
	kbase_pm_set_statistics(kbdev, power_mode);
	up(gpu_dvfs_ctx.sem);
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
#ifdef KBASE_PM_RUNTIME
	int res;

	res = pm_runtime_put_sync(kbdev->dev);
	if (res < 0)
	{
		printk(KERN_ERR "mali----pm_runtime_put_sync return (%d)\n", res);
	}
#endif

	mali_power_mode_change(kbdev, 1);
}

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	mali_power_mode_change(kbdev, 0);

#ifdef KBASE_PM_RUNTIME
	{
		int res;

		res = pm_runtime_get_sync(kbdev->dev);
		if (res < 0)
		{
			printk(KERN_ERR "mali----pm_runtime_get_sync return (%d)\n", res);
		}
	}
#endif

	return 1;
}

static void pm_callback_power_suspend(struct kbase_device *kbdev)
{
	mali_power_mode_change(kbdev, 2);
}

static void pm_callback_power_resume(struct kbase_device *kbdev)
{
	mali_power_mode_change(kbdev, 0);
}

#ifdef KBASE_PM_RUNTIME
static int pm_callback_power_runtime_init(struct kbase_device *kbdev)
{
	pm_runtime_set_active(kbdev->dev);
	pm_suspend_ignore_children(kbdev->dev, true);
	pm_runtime_set_autosuspend_delay(kbdev->dev, PM_RUNTIME_DELAY_MS);
	pm_runtime_use_autosuspend(kbdev->dev);
	pm_runtime_enable(kbdev->dev);

	return 0;
}

static void pm_callback_power_runtime_term(struct kbase_device *kbdev)
{
	pm_runtime_disable(kbdev->dev);
}
#endif/*CONFIG_PM_RUNTIME*/

struct kbase_pm_callback_conf pm_qogirn6pro_callbacks = {
	.power_off_callback = pm_callback_power_off,
	.power_on_callback = pm_callback_power_on,
	.power_suspend_callback = pm_callback_power_suspend,
	.power_resume_callback = pm_callback_power_resume,
#ifdef KBASE_PM_RUNTIME
	.power_runtime_init_callback = pm_callback_power_runtime_init,
	.power_runtime_term_callback = pm_callback_power_runtime_term,
	.power_runtime_off_callback = NULL,
	.power_runtime_on_callback = NULL
#endif
};


static struct kbase_platform_config versatile_platform_config = {
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &versatile_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}

#if defined(CONFIG_MALI_DEVFREQ)
static int freq_search(struct gpu_freq_info freq_list[], int len, int key)
{
	int low=0, high=len-1, mid;

	if (0 > key)
	{
		return -1;
	}

	while(low <= high)
	{
		mid = (low+high)/2;
		if(key == freq_list[mid].freq)
		{
			return mid;
		}

		if(key < freq_list[mid].freq)
		{
			high = mid-1;
		}
		else
		{
			low = mid+1;
		}
	}
	return -1;
}

int kbase_platform_get_init_freq(void)
{
	return (gpu_dvfs_ctx.freq_list[GPU_MIN_FREQ_INDEX].freq * FREQ_KHZ);
}

int kbase_platform_get_min_freq(void)
{
	return (gpu_dvfs_ctx.freq_list[GPU_MIN_FREQ_INDEX].freq * FREQ_KHZ);
}

int kbase_platform_get_max_freq(void)
{
	return (gpu_dvfs_ctx.freq_list[gpu_dvfs_ctx.freq_list_len -1].freq * FREQ_KHZ);
}

void kbase_platform_limit_max_freq(struct device *dev)
{
	//disable 26M/76.8M
	dev_pm_opp_disable(dev, GPU_26M_FREQ);
	dev_pm_opp_disable(dev, GPU_76M8_FREQ);
	dev_pm_opp_disable(dev, GPU_153M6_FREQ);
}

int kbase_platform_set_freq_volt(int freq, int volt)
{
	int index = -1;

	freq = freq/FREQ_KHZ;
	index = freq_search(gpu_dvfs_ctx.freq_list, gpu_dvfs_ctx.freq_list_len, freq);
	printk(KERN_ERR "mali GPU_DVFS %s index=%d cur_freq=%d cur_volt=%d --> freq=%d volt=%d gpu_power_on=%d gpu_clock_on=%d \n",
		__func__, index, gpu_dvfs_ctx.freq_cur->freq, gpu_dvfs_ctx.cur_voltage, freq, volt,
		gpu_dvfs_ctx.gpu_power_on, gpu_dvfs_ctx.gpu_clock_on);
	if (0 <= index)
	{
		down(gpu_dvfs_ctx.sem);

		//set frequency
		if (gpu_dvfs_ctx.gpu_power_on && gpu_dvfs_ctx.gpu_clock_on)
		{
			//set dvfs index, 0: 384M 1:512M 2:614.4M 3:768M 4:800M
			regmap_update_bits(gpu_dvfs_ctx.dvfs_index_cfg.regmap_ptr, gpu_dvfs_ctx.dvfs_index_cfg.args[0], gpu_dvfs_ctx.dvfs_index_cfg.args[1], gpu_dvfs_ctx.freq_list[index].dvfs_index);
		}
		gpu_dvfs_ctx.cur_index = index;
		gpu_dvfs_ctx.freq_cur = &gpu_dvfs_ctx.freq_list[index];
		trace_clock_set_rate(VENDOR_FTRACE_MODULE_NAME, gpu_dvfs_ctx.freq_cur->freq, raw_smp_processor_id());
		up(gpu_dvfs_ctx.sem);
	}

	return 0;
}
#endif
