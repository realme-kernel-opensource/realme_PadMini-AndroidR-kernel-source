/*
 * ASoC SPRD sound card support
 *
 * Copyright (C) 2015 Renesas Solutions Corp.
 * Kuninori Morimoto <kuninori.morimoto.gx@renesas.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "sprd-asoc-debug.h"
#define pr_fmt(fmt) pr_sprd_fmt("BOARD")""fmt

#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#include "sprd-asoc-card-utils.h"
#include "sprd-asoc-common.h"

struct sprd_asoc_ext_hook_map {
	const char *name;
	sprd_asoc_hook_func hook;
	int en_level;
};

enum {
	/* ext_ctrl_type */
	CELL_CTRL_TYPE,
	/* pa type select */
	CELL_HOOK,
	/* select mode */
	CELL_PRIV,
	/* share gpio with  */
	CELL_SHARE_GPIO,
	CELL_NUMBER,
};

struct sprd_asoc_hook_spk_priv {
	int gpio[BOARD_FUNC_MAX];
	int priv_data[BOARD_FUNC_MAX];
	spinlock_t lock;
};

static struct sprd_asoc_hook_spk_priv hook_spk_priv;

#define GENERAL_SPK_MODE 10

#define EN_LEVEL 1

static int select_mode;
//#ifdef ODM_HQ_EDIT
//liyunfan@ODM.HQ.Multimeida.Audio,algorithm compatible of second pa,2021/9/23
static int det_type;
//#endif /* ODM_HQ_EDIT */

static ssize_t select_mode_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buff)
{
	return sprintf(buff, "%d\n", select_mode);
}

static ssize_t select_mode_store(struct kobject *kobj,
				 struct kobj_attribute *attr,
				 const char *buff, size_t len)
{
	unsigned long level;
	int ret;


	ret = kstrtoul(buff, 10, &level);
	if (ret) {
		pr_err("%s kstrtoul failed!(%d)\n", __func__, ret);
		return len;
	}
	select_mode = level;
	pr_info("speaker ext pa select_mode = %d\n", select_mode);

	return len;
}
//#ifdef ODM_HQ_EDIT
//liyunfan@ODM.HQ.Multimeida.Audio,algorithm compatible of second pa,2021/9/23
static ssize_t pa_info_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buff)
{
	return sprintf(buff, "%s\n", det_type?"awxxx":"siaxx");
}
//#endif /* ODM_HQ_EDIT */
static int ext_debug_sysfs_init(void)
{
	int ret;
	static struct kobject *ext_debug_kobj;
	static struct kobj_attribute ext_debug_attr =
		__ATTR(select_mode, 0644,
		select_mode_show,
		select_mode_store);
    //#ifdef ODM_HQ_EDIT
    //liyunfan@ODM.HQ.Multimeida.Audio,algorithm compatible of second pa,2021/9/23
    static struct kobj_attribute ext_info_attr =
        __ATTR(pa_info, 0644,
        pa_info_show,
        NULL);
    //#endif /* ODM_HQ_EDIT */
	if (ext_debug_kobj)
		return 0;
	ext_debug_kobj = kobject_create_and_add("extpa", kernel_kobj);
	if (ext_debug_kobj == NULL) {
		ret = -ENOMEM;
		pr_err("register sysfs failed. ret = %d\n", ret);
		return ret;
	}

	ret = sysfs_create_file(ext_debug_kobj, &ext_debug_attr.attr);
	if (ret) {
		pr_err("create sysfs failed. ret = %d\n", ret);
		return ret;
	}
    //#ifdef ODM_HQ_EDIT
    //liyunfan@ODM.HQ.Multimeida.Audio,algorithm compatible of second pa,2021/9/23
    ret = sysfs_create_file(ext_debug_kobj, &ext_info_attr.attr);
    if (ret) {
        pr_err("create sysfs failed. ret = %d\n", ret);
        return ret;
    }
    //#endif /* ODM_HQ_EDIT */
	return ret;
}
#ifdef CONFIG_SND_SOC_AW87XXX

enum aw87xxx_scene_mode {
 AW87XXX_OFF_MODE = 0,
 AW87XXX_MUSIC_MODE = 1,
 AW87XXX_VOICE_MODE = 2,
 AW87XXX_FM_MODE = 3,
 AW87XXX_RCV_MODE = 4,
 AW87XXX_MODE_MAX = 5,
};

enum {
 AW87XXX_LEFT_CHANNEL = 0,
 AW87XXX_RIGHT_CHANNEL = 1,
};

extern unsigned char aw87xxx_show_current_mode(int32_t channel);
extern int aw87xxx_audio_scene_load(uint8_t mode, int32_t channel);

static int audio_mode[2] = {0};    //0:off; 1:music; 2:voice; ...

int aw87xxx_audio_mode_set( int id, int mode)
{
    sp_asoc_pr_info("[%s] set spk%d mode:%d\n", __func__, id, mode);
    audio_mode[id] = mode;
    return 0;
}

int aw87xxx_audio_mode_get( int id)
{
    sp_asoc_pr_info("[%s] get spk%d mode:%d\n", __func__, id, audio_mode[id]);
    return audio_mode[id];
}

#if 0
static int hook_spk_aw87xx(int id, int on)
{
 pr_err("%s id: %d, on: %d\n", __func__, id, on);
 if(on)
	aw87xxx_audio_scene_load(AW87XXX_MUSIC_MODE, AW87XXX_LEFT_CHANNEL);
 else
	aw87xxx_audio_scene_load(AW87XXX_OFF_MODE, AW87XXX_LEFT_CHANNEL);

 return HOOK_OK;
}
#endif

//#else
#endif

static void hook_gpio_pulse_control(unsigned int gpio, unsigned int mode)
{
	int i = 1;
	spinlock_t *lock = &hook_spk_priv.lock;
	unsigned long flags;

	spin_lock_irqsave(lock, flags);
	for (i = 1; i < mode; i++) {
		gpio_set_value(gpio, EN_LEVEL);
		udelay(2);
		gpio_set_value(gpio, !EN_LEVEL);
		udelay(2);
	}

	gpio_set_value(gpio, EN_LEVEL);
	spin_unlock_irqrestore(lock, flags);
}

static int hook_general_spk(int id, int on)
{
	int gpio, mode;

	gpio = hook_spk_priv.gpio[id];
	if (gpio < 0) {
		pr_err("%s gpio is invalid!\n", __func__);
		return -EINVAL;
	}
	mode = hook_spk_priv.priv_data[id];
	if (mode > GENERAL_SPK_MODE)
		mode = 0;
	pr_info("%s id: %d, gpio: %d, mode: %d, on: %d\n",
		 __func__, id, gpio, mode, on);

	/* Off */
	if (!on) {
		gpio_set_value(gpio, !EN_LEVEL);
		return HOOK_OK;
	}

	/* On */
	if (select_mode) {
		mode = select_mode;
		pr_info("%s mode: %d, select_mode: %d\n",
			__func__, mode, select_mode);
	}
	hook_gpio_pulse_control(gpio, mode);

	/* When the first time open speaker path and play a very short sound,
	 * the sound can't be heard. So add a delay here to make sure the AMP
	 * is ready.
	 */
	msleep(22);

	return HOOK_OK;
}

//#endif
static int hook_general_i2c(int id, int on)
{
	int mode;

	mode = hook_spk_priv.priv_data[id];
	pr_info("%s id: %d, mode: %d, on: %d\n",
		 __func__, id, mode, on);

    /*******************************************
    id :
        enum {
            BOARD_FUNC_SPK = 0,
            BOARD_FUNC_SPK1,
            BOARD_FUNC_EAR,
            BOARD_FUNC_HP,
            BOARD_FUNC_MUTE_MAX,
            BOARD_FUNC_LINE = BOARD_FUNC_MUTE_MAX,
            BOARD_FUNC_MIC,
            BOARD_FUNC_AUXMIC,
            BOARD_FUNC_HP_MIC,
            BOARD_FUNC_DMIC,
            BOARD_FUNC_DMIC1,
            BOARD_FUNC_DFM,
            BOARD_FUNC_MAX
        };
    *******************************************/ 
    #if 0
	if (!on) {
	/* Off */
		/// i2c disable pa with id
		aw87xxx_audio_scene_load(AW87XXX_OFF_MODE, AW87XXX_LEFT_CHANNEL);
		aw87xxx_audio_scene_load(AW87XXX_OFF_MODE, AW87XXX_RIGHT_CHANNEL);
	} else {
	/* On */
        /// i2c enable pa with id
		aw87xxx_audio_scene_load(AW87XXX_MUSIC_MODE, AW87XXX_LEFT_CHANNEL);
		aw87xxx_audio_scene_load(AW87XXX_MUSIC_MODE, AW87XXX_RIGHT_CHANNEL);
    }
    #endif

    if(on) {
        aw87xxx_audio_scene_load(audio_mode[id], id);
    } else {
        aw87xxx_audio_scene_load(AW87XXX_OFF_MODE, id);
    }
	return HOOK_OK;
}
static struct sprd_asoc_ext_hook_map ext_hook_arr[] = {
	{"general_speaker", hook_general_spk, EN_LEVEL},
	{"general_i2c", hook_general_i2c, EN_LEVEL},
	
//  #endif
};

static int sprd_asoc_card_parse_hook(struct device *dev,
					 struct sprd_asoc_ext_hook *ext_hook)
{
	struct device_node *np = dev->of_node;
	const char *prop_pa_info = "sprd,spk-ext-pa-info";
	const char *prop_pa_gpio = "sprd,spk-ext-pa-gpio";
#if 0
	const char *prop_pa_det = "sprd,spk-ext-pa-det";
	int det_gpio;
#endif
	int spk_cnt, elem_cnt, i;
	int ret = 0;
	unsigned long gpio_flag;
	unsigned int ext_ctrl_type, share_gpio, hook_sel, priv_data;
	u32 *buf;

	elem_cnt = of_property_count_u32_elems(np, prop_pa_info);
	if (elem_cnt <= 0) {
		dev_info(dev,
			"Count '%s' failed!(%d)\n", prop_pa_info, elem_cnt);
		return -EINVAL;
	}

	if (elem_cnt % CELL_NUMBER) {
		dev_err(dev, "Spk pa info is not a multiple of %d.\n",
			CELL_NUMBER);
		return -EINVAL;
	}

	spk_cnt = elem_cnt / CELL_NUMBER;
	if (spk_cnt > BOARD_FUNC_MAX) {
		dev_warn(dev, "Speaker count %d is greater than %d!\n",
			 spk_cnt, BOARD_FUNC_MAX);
		spk_cnt = BOARD_FUNC_MAX;
	}

	spin_lock_init(&hook_spk_priv.lock);

	buf = devm_kmalloc(dev, elem_cnt * sizeof(u32), GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, prop_pa_info, buf, elem_cnt);
	if (ret < 0) {
		dev_err(dev, "Read property '%s' failed!\n", prop_pa_info);
		//return ret;
	}

	for (i = 0; i < spk_cnt; i++) {
		int num = i * CELL_NUMBER;

		/* Get the ctrl type */
		ext_ctrl_type = buf[CELL_CTRL_TYPE + num];
		if (ext_ctrl_type >= BOARD_FUNC_MAX) {
			dev_err(dev, "Ext ctrl type %d is invalid!\n",
				ext_ctrl_type);
			return -EINVAL;
		}

		/* Get the selection of hook function */
		hook_sel = buf[CELL_HOOK + num];
		if (hook_sel >= ARRAY_SIZE(ext_hook_arr)) {
			dev_err(dev,
				"Hook selection %d is invalid!\n", hook_sel);
			return -EINVAL;
		}
		ext_hook->ext_ctrl[ext_ctrl_type] = ext_hook_arr[hook_sel].hook;

		/* Get the private data */
		priv_data = buf[CELL_PRIV + num];
		hook_spk_priv.priv_data[ext_ctrl_type] = priv_data;
		dev_err(dev, "sprd_asoc_card_parse_hook hook_sel =%d \n", hook_sel);
		 if (hook_sel == 1) {
            dev_info(dev, "I2C control PA, skip the process of gpio\n");
            continue;
       }

        if (2 == hook_sel) {
            pr_warn("%s pa use i2c\n", __func__);
            continue;
        }
		/* Process the shared gpio */
		share_gpio = buf[CELL_SHARE_GPIO + num];
		if (share_gpio > 0) {
			if (share_gpio > spk_cnt) {
				dev_err(dev, "share_gpio %d is bigger than spk_cnt!\n",
					share_gpio);
				ext_hook->ext_ctrl[ext_ctrl_type] = NULL;
				return -EINVAL;
			}
			hook_spk_priv.gpio[ext_ctrl_type] =
				hook_spk_priv.gpio[share_gpio - 1];
			continue;
		}

		ret = of_get_named_gpio_flags(np, prop_pa_gpio, i, NULL);
		if (ret < 0) {
			dev_err(dev, "Get gpio failed:%d!\n", ret);
			ext_hook->ext_ctrl[ext_ctrl_type] = NULL;
			return ret;
		}
		hook_spk_priv.gpio[ext_ctrl_type] = ret;

		pr_info("ext_ctrl_type %d hook_sel %d priv_data %d gpio %d",
			ext_ctrl_type, hook_sel, priv_data, ret);

		gpio_flag = GPIOF_DIR_OUT;
		gpio_flag |= ext_hook_arr[hook_sel].en_level ?
			GPIOF_INIT_HIGH : GPIOF_INIT_LOW;
		ret = gpio_request_one(hook_spk_priv.gpio[ext_ctrl_type],
				       gpio_flag, NULL);
		if (ret < 0) {
			dev_err(dev, "Gpio request[%d] failed:%d!\n",
				ext_ctrl_type, ret);
			ext_hook->ext_ctrl[ext_ctrl_type] = NULL;
			return ret;
		}
#if 0
            //#ifdef ODM_HQ_EDIT
            //liyunfan@ODM.HQ.Multimeida.Audio,algorithm compatible of second pa,2021/9/23
            // det
            det_gpio = of_get_named_gpio_flags(np, prop_pa_det, i, NULL);
            if (det_gpio < 0) {
                dev_err(dev, "%s not exit! skip\n", prop_pa_det);
            } else {
                // detect pa type
                gpio_flag = GPIOF_DIR_IN;
                ret = gpio_request_one(det_gpio, gpio_flag, "det_gpio");
                if (ret < 0) {
                    dev_err(dev, "det_gpio det request failed:%d!\n", ret);
                    return ret;
                }
                det_type = gpio_get_value(det_gpio);
                dev_info(dev, "det_gpio det:%d\n", det_type);
            }
            //#endif /* ODM_HQ_EDIT */
#endif
       }
#if 0
    //#ifdef ODM_HQ_EDIT
    //liyunfan@ODM.HQ.Multimeida.Audio,algorithm compatible of second pa,2021/9/23
    dev_info(dev, "det_gpio det :%d,mode = %d\n", det_type,hook_spk_priv.priv_data[0]);
    if(!det_type){
        hook_spk_priv.priv_data[0] = 2;
        dev_info(dev, "det_gpio det2 :%d,mode = %d\n", det_type,hook_spk_priv.priv_data[0]);
    }
    //#endif /* ODM_HQ_EDIT */
#endif
    return 0;
}
int sprd_asoc_card_parse_ext_hook(struct device *dev,
				  struct sprd_asoc_ext_hook *ext_hook)
{
	ext_debug_sysfs_init();
	return sprd_asoc_card_parse_hook(dev, ext_hook);
}

MODULE_ALIAS("platform:asoc-sprd-card");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("ASoC SPRD Sound Card Utils - Hooks");
MODULE_AUTHOR("Peng Lee <peng.lee@spreadtrum.com>");
