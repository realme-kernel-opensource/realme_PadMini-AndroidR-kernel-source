/*
 * Copyright (C) 2015-2016 Spreadtrum Communications Inc.
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
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/vmalloc.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/pm_runtime.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/pwm.h>
#include <linux/clk.h>
#include <linux/types.h>
#include <linux/pm_wakeup.h>

#include "sprd_img.h"
#include "flash_drv.h"

#ifdef pr_fmt
#undef pr_fmt
#endif
#define pr_fmt(fmt) "FLASH_OCP8135B: %d %d %s : " fmt, current->pid, __LINE__, __func__

#define FLASH_DRIVER_NAME "flash_ocp8135b"
#define FLASH_GPIO_ENF_NAME "flash-en-gpios"

#define CAMERA_TORCH_LIGHT_DUTY 75
#define FLASH_MAX_LEVEL 16

struct flash_driver_data {
    int gpio_139_ENF;
    int torch_led_index;
    struct pwm_device *pwm_chip;
    struct wakeup_source pd_wake_lock;
};

void pwm_set_ocp8135b_config( struct pwm_device *pwm, int duty_cycle)
{
    unsigned int duty_ns, period_ns;
    struct pwm_state state;

    pwm_get_state(pwm, &state);
    period_ns = state.period;
    duty_ns = duty_cycle * period_ns / 100;
    state.duty_cycle = duty_ns;

    if (duty_ns > 0) {
        state.enabled= true ;
    } else{
        state.enabled= false ;
    }
    pr_info("duty_cycle: %d",duty_cycle);
    pwm_apply_state(pwm, &state);
}

static int sprd_flash_ocp8135b_deinit(void *drvd)
{
    int ret = 0;
    int gpio_id = 0;
    struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
    if (!drv_data)
        return -EFAULT;

    if (IS_ERR(drv_data->pwm_chip))
        return -EFAULT;

    pr_info("E\n");
    gpio_id = drv_data->gpio_139_ENF;
    if (gpio_is_valid(gpio_id)) {
        ret = gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
        if (ret)
            goto exit;
    }

    pwm_set_ocp8135b_config(drv_data->pwm_chip, 0);

exit:
    pr_info("X\n");
    return ret;
}

static int sprd_flash_ocp8135b_open_torch(void *drvd, uint8_t idx)
{
    struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
    int gpio_id = 0;
    int ret = 0;

    if (IS_ERR(drv_data))
        return -EFAULT;

    if (IS_ERR(drv_data->pwm_chip))
        return -EFAULT;

    idx |= SPRD_FLASH_LED0;
    pr_info("E\n");
    if (SPRD_FLASH_LED0 & idx) {
        gpio_id = drv_data->gpio_139_ENF;
        if (gpio_is_valid(gpio_id)){
            gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
        }
        pwm_set_ocp8135b_config(drv_data->pwm_chip, 100);
        udelay(5 * 1000);
        #ifdef CAMERA_TORCH_LIGHT_DUTY
            pwm_set_ocp8135b_config(drv_data->pwm_chip, CAMERA_TORCH_LIGHT_DUTY);
        #else
            pwm_set_ocp8135b_config(drv_data->pwm_chip, drv_data->torch_led_index);
        #endif
    }
    __pm_stay_awake(&drv_data->pd_wake_lock);
    pr_info("X\n");
    return ret;
}

static int sprd_flash_ocp8135b_close_torch(void *drvd, uint8_t idx)
{
    int ret = 0;
    int gpio_id = 0;
    struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

    if (!drv_data)
        return -EFAULT;

    if (IS_ERR(drv_data->pwm_chip))
        return -EFAULT;

    pr_info("E");
    idx |= SPRD_FLASH_LED0;
    if (SPRD_FLASH_LED0 & idx) {
        gpio_id = drv_data->gpio_139_ENF;
        if (gpio_is_valid(gpio_id)) {
            gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
        }
        pwm_set_ocp8135b_config(drv_data->pwm_chip, 0);
        udelay(5 * 1000);
     }
    __pm_relax(&drv_data->pd_wake_lock);
    pr_info("X");
    return ret;
}

static int sprd_flash_ocp8135b_open_preflash(void *drvd, uint8_t idx)
{
    struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
    int gpio_id = 0;
    int ret = 0;

    if (IS_ERR(drv_data))
        return -EFAULT;

    if (IS_ERR(drv_data->pwm_chip))
        return -EFAULT;

    idx |= SPRD_FLASH_LED0;
    pr_info("E\n");
    if (SPRD_FLASH_LED0 & idx) {
        gpio_id = drv_data->gpio_139_ENF;
        if (gpio_is_valid(gpio_id)){
            gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
        }
        pwm_set_ocp8135b_config(drv_data->pwm_chip, 100);
        udelay(5 * 1000);
        pwm_set_ocp8135b_config(drv_data->pwm_chip, drv_data->torch_led_index);
    }
    pr_info("X\n");
    return ret;
}

static int sprd_flash_ocp8135b_open_highlight(void *drvd, uint8_t idx)
{
    int ret = 0;
    int gpio_id = 0;
    struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

    if (!drv_data)
        return -EFAULT;

    if (IS_ERR(drv_data->pwm_chip))
        return -EFAULT;

    pr_info("E");
    idx |= SPRD_FLASH_LED0;
    if (SPRD_FLASH_LED0 & idx) {
        gpio_id = drv_data->gpio_139_ENF;
        if (gpio_is_valid(gpio_id)) {
            gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
        }
        pwm_set_ocp8135b_config(drv_data->pwm_chip, drv_data->torch_led_index);
        udelay(1000);//<2.5ms

        if (gpio_is_valid(gpio_id)) {
            gpio_direction_output(gpio_id, SPRD_FLASH_ON);
        }
    }
    pr_info("X");
    return ret;
}

static int sprd_flash_ocp8135b_close_preflash(void *drvd, uint8_t idx)
{
    int ret = 0;
    int gpio_id = 0;
    struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;

    if (!drv_data)
        return -EFAULT;

    if (IS_ERR(drv_data->pwm_chip))
        return -EFAULT;

    pr_info("E");
    idx |= SPRD_FLASH_LED0;
    if (SPRD_FLASH_LED0 & idx) {
        gpio_id = drv_data->gpio_139_ENF;
        if (gpio_is_valid(gpio_id)) {
            gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
        }
        pwm_set_ocp8135b_config(drv_data->pwm_chip, 0);
        udelay(5 * 1000);
     }
    pr_info("X");
    return ret;
}

/**
ENF="0"
ENM="0"
Time(td)>= 5ms
*/

static int sprd_flash_ocp8135b_close_highlight(void *drvd, uint8_t idx)
{
    int ret = 0;
    int gpio_id = 0;
    struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
    if (!drv_data)
        return -EFAULT;

    if (IS_ERR(drv_data->pwm_chip))
        return -EFAULT;

    pr_info("E");
    idx |= SPRD_FLASH_LED0;
    if (SPRD_FLASH_LED0 & idx) {
        gpio_id = drv_data->gpio_139_ENF;
        if (gpio_is_valid(gpio_id)) {
            pr_info("SPRD_FLASH_OFF");
            gpio_direction_output(gpio_id, SPRD_FLASH_OFF);
        }
        udelay(100);
        pwm_set_ocp8135b_config(drv_data->pwm_chip, 0);
        udelay(5 * 1000);
    }
    pr_info("X");
    return ret;
}

static int sprd_flash_ocp8135b_cfg_value_torch(void *drvd, uint8_t idx,
    struct sprd_flash_element *element) {
    /*struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
    int level = 0;
    idx |= SPRD_FLASH_LED0;
    if (SPRD_FLASH_LED0 & idx){
        level = element->index + 1;
        if (level>= FLASH_MAX_LEVEL){
            level = FLASH_MAX_LEVEL;
        }
        drv_data->torch_led_index = (int) (90 * level/FLASH_MAX_LEVEL);
        pr_info("level:%d,torch_led_index:%d", level, drv_data->torch_led_index);
    }*/
    return 0;
}

static int sprd_flash_ocp8135b_cfg_value_preflash(void *drvd, uint8_t idx,
                    struct sprd_flash_element *element) {
    struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
    int level = 0;
    idx |= SPRD_FLASH_LED0;
    if (SPRD_FLASH_LED0 & idx){
        drv_data->torch_led_index = 100;
        pr_info("level:%d,duty:%d", level, drv_data->torch_led_index);
    }
    return 0;
}

static int sprd_flash_ocp8135b_cfg_value_highlight(void *drvd, uint8_t idx,
                               struct sprd_flash_element *element) {
    struct flash_driver_data *drv_data = (struct flash_driver_data *)drvd;
    int level = 0;
    idx |= SPRD_FLASH_LED0;
    if (SPRD_FLASH_LED0 & idx){
        level = element->index + 1;
        if (level >= FLASH_MAX_LEVEL){
            level = FLASH_MAX_LEVEL;
        }
        drv_data->torch_led_index = (int) (85 * level/FLASH_MAX_LEVEL);
        pr_info("level:%d, duty:%d", level, drv_data->torch_led_index);
    }

    return 0;
}


static const struct sprd_flash_driver_ops flash_gpio_ops = {
    .open_torch = sprd_flash_ocp8135b_open_torch,
    .close_torch = sprd_flash_ocp8135b_close_torch,
    .open_preflash = sprd_flash_ocp8135b_open_preflash,
    .close_preflash = sprd_flash_ocp8135b_close_preflash,
    .open_highlight = sprd_flash_ocp8135b_open_highlight,
    .close_highlight = sprd_flash_ocp8135b_close_highlight,
    .cfg_value_torch = sprd_flash_ocp8135b_cfg_value_torch,
    .cfg_value_preflash = sprd_flash_ocp8135b_cfg_value_preflash,
    .cfg_value_highlight = sprd_flash_ocp8135b_cfg_value_highlight,
};

static const struct of_device_id ocp8135b_flash_of_match_table[] = {
    {.compatible = "sprd,flash-pwm-ocp8135b"},
};

static int sprd_flash_ocp8135b_probe(struct platform_device *pdev)
{
    int ret = 0;
    u32 gpio_node = 0;
    struct flash_driver_data *drv_data = NULL;

    pr_err("E\n");
    if (IS_ERR_OR_NULL(pdev))
          return -EINVAL;

    if (!pdev->dev.of_node) {
        pr_err("no device node %s", __func__);
        return -ENODEV;
    }

    ret = of_property_read_u32(pdev->dev.of_node, "flash-ic", &gpio_node);
    if (ret || gpio_node != 8135) {
        pr_err("no ocp8135b flash\n");
        return -ENOMEM;
    }

    drv_data = devm_kzalloc(&pdev->dev, sizeof(*drv_data), GFP_KERNEL);
    if (!drv_data)
        return -ENOMEM;

    pdev->dev.platform_data = (void *)drv_data;

    drv_data->pwm_chip = devm_pwm_get(&pdev->dev,NULL);
    if (IS_ERR(drv_data->pwm_chip)) {
        dev_err(&pdev->dev, "get pwms device failed\n");
        goto exit;
    }

    drv_data->gpio_139_ENF = of_get_named_gpio(pdev->dev.of_node, FLASH_GPIO_ENF_NAME, 0);
    if (gpio_is_valid(drv_data->gpio_139_ENF)) {
        ret = devm_gpio_request(&pdev->dev,
                    drv_data->gpio_139_ENF, FLASH_GPIO_ENF_NAME);
        pr_err("gpio init gpio %d\n",drv_data->gpio_139_ENF);
        if (ret) {
            pr_err("flash gpio err\n");
            goto exit;
        }
     }

    ret = gpio_direction_output(drv_data->gpio_139_ENF, SPRD_FLASH_OFF);
    if (ret) {
        pr_err("flash gpio output err\n");
        goto exit;
    }

    ret = sprd_flash_register(&flash_gpio_ops, drv_data, SPRD_FLASH_REAR);
    if (ret)
          goto exit;

    wakeup_source_init(&drv_data->pd_wake_lock, "flash_pwm");
exit:
    pr_err("x\n");
    return ret;
}

static int sprd_flash_ocp8135b_remove(struct platform_device *pdev)
{
    int ret = 0;

    ret = sprd_flash_ocp8135b_deinit(pdev->dev.platform_data);
    if (ret)
        pr_err("flash deinit err\n");

    return ret;
}

static const struct platform_device_id ocp8135b_flash_id[] = {
    {"flash_ocp8135b", 0},
    {},
};

static struct platform_driver sprd_flash_ocp8135b_driver = {
    .probe = sprd_flash_ocp8135b_probe,
    .remove = sprd_flash_ocp8135b_remove,
    .driver = {
                .name = FLASH_DRIVER_NAME,
                .owner = THIS_MODULE,
                .of_match_table = of_match_ptr(ocp8135b_flash_of_match_table),
             },
    .id_table = ocp8135b_flash_id,
};

module_platform_driver(sprd_flash_ocp8135b_driver);
MODULE_DESCRIPTION("Spreadtrum OCP8135 Camera Flash Driver");
MODULE_LICENSE("GPL");
