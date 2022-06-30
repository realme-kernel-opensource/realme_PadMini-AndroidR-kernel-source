#ifdef CONFIG_CTS_I2C_HOST
#define LOG_TAG         "I2CDrv"
#else
#define LOG_TAG         "SPIDrv"
#endif

#include "cts_config.h"
#include "cts_platform.h"
#include "cts_core.h"
#include "cts_sysfs.h"
#include "cts_charger_detect.h"
#include "cts_earjack_detect.h"
#include <linux/power_supply.h>
#include <linux/string.h>
#include "cts_oem.h"
#include "cts_proc.h"
#include <linux/notifier.h>
#include "../../../gpu/drm/sprd/sprd_panel.h"

#include "cts_strerror.h"
#include "../touch.h"

extern struct touch_panel tp_interface;
struct cts_device *cts_de;

extern const char *lcd_name;
bool cts_show_debug_log = false;

struct chipone_ts_data *cts_esd ;





extern int esd_tp_reset_notifier_register(struct notifier_block *nb);
extern int esd_tp_reset_notifier_unregister(struct notifier_block *nb);




module_param_named(debug_log, cts_show_debug_log, bool, 0660);
MODULE_PARM_DESC(debug_log, "Show debug log control");

struct proc_dir_entry *CTS_proc_touchpanel_dir;
int cts_suspend(struct chipone_ts_data *cts_data)
{
    int ret;

    cts_info("Suspend");
	cts_data->suspend = true;

    cts_lock_device(&cts_data->cts_dev);
    ret = cts_suspend_device(&cts_data->cts_dev);
    cts_unlock_device(&cts_data->cts_dev);

    if (ret) {
        cts_err("Suspend device failed %d", ret);
        // TODO:
        //return ret;
    }

    ret = cts_stop_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Stop device failed %d", ret);
        return ret;
    }

#ifdef CFG_CTS_GESTURE
    /* Enable IRQ wake if gesture wakeup enabled */
    if (cts_is_gesture_wakeup_enabled(&cts_data->cts_dev) || tp_gesture) {
        ret = cts_plat_enable_irq_wake(cts_data->pdata);
        if (ret) {
            cts_err("Enable IRQ wake failed %d", ret);
            return ret;
        }
        ret = cts_plat_enable_irq(cts_data->pdata);
        if (ret){
            cts_err("Enable IRQ failed %d",ret);
            return ret;
        }
    }
#endif /* CFG_CTS_GESTURE */

    /** - To avoid waking up while not sleeping,
            delay 20ms to ensure reliability */
    msleep(20);

    return 0;
}

int cts_resume(struct chipone_ts_data *cts_data)
{
    int ret;

    cts_info("Resume");
	cts_data->suspend = false;

#ifdef CFG_CTS_GESTURE
    if (cts_is_gesture_wakeup_enabled(&cts_data->cts_dev)|| tp_gesture) {
        ret = cts_plat_disable_irq_wake(cts_data->pdata);
        if (ret) {
            cts_warn("Disable IRQ wake failed %d", ret);
            //return ret;
        }
        if ((ret = cts_plat_disable_irq(cts_data->pdata)) < 0) {
            cts_err("Disable IRQ failed %d", ret);
            //return ret;
        }
    }
#endif /* CFG_CTS_GESTURE */

    cts_lock_device(&cts_data->cts_dev);
    ret = cts_resume_device(&cts_data->cts_dev);
    cts_unlock_device(&cts_data->cts_dev);
    if(ret) {
        cts_warn("Resume device failed %d", ret);
        return ret;
    }

    ret = cts_start_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Start device failed %d", ret);
        return ret;
    }

    return 0;
}

static int esd_tp_reset_notify_callback(struct notifier_block *nb,
                                unsigned long action, void *data)
{


        switch (action) {
        case ESD_EVENT_TP_SUSPEND:
            //cts_suspend(cts_esd);
            cts_err("CTS ESD_EVENT_TP_SUSPEND\n");
            break;
        case ESD_EVENT_TP_RESUME:        
            //cts_resume(cts_esd);
            cts_err("CTS ESD_EVENT_TP_RESUME\n");
            break;
        default:
            return NOTIFY_DONE;
        }

        return NOTIFY_OK;
}

static struct notifier_block esd_tp_reset_notifier = {
  .notifier_call = esd_tp_reset_notify_callback,
};



#ifdef CONFIG_CTS_PM_FB_NOTIFIER
#ifdef CFG_CTS_DRM_NOTIFIER
static int fb_notifier_callback(struct notifier_block *nb,
        unsigned long action, void *data)
{
    volatile int blank;
    const struct cts_platform_data *pdata =
        container_of(nb, struct cts_platform_data, fb_notifier);
    struct chipone_ts_data *cts_data =
        container_of(pdata->cts_dev, struct chipone_ts_data, cts_dev);
    struct fb_event *evdata = data;

    cts_info("FB notifier callback");

    if (evdata && evdata->data) {
        if (action == MSM_DRM_EVENT_BLANK) {
            blank = *(int *)evdata->data;
            if (blank == MSM_DRM_BLANK_UNBLANK) {
                cts_resume(cts_data);
                return NOTIFY_OK;
            }
        } else if (action == MSM_DRM_EARLY_EVENT_BLANK) {
            blank = *(int *)evdata->data;
            if (blank == MSM_DRM_BLANK_POWERDOWN) {
                cts_suspend(cts_data);
                return NOTIFY_OK;
            }
        }
    }

    return NOTIFY_DONE;
}
#else
static int fb_notifier_callback(struct notifier_block *nb,
        unsigned long action, void *data)
{
    volatile int blank;
    const struct cts_platform_data *pdata =
        container_of(nb, struct cts_platform_data, fb_notifier);
    struct chipone_ts_data *cts_data =
        container_of(pdata->cts_dev, struct chipone_ts_data, cts_dev);
    struct fb_event *evdata = data;

    cts_info("FB notifier callback");

    if (evdata && evdata->data) {
        if (action == FB_EVENT_BLANK) {
            blank = *(int *)evdata->data;
            if (blank == FB_BLANK_UNBLANK) {
                cts_resume(cts_data);
                return NOTIFY_OK;
            }
        } else if (action == FB_EARLY_EVENT_BLANK) {
            blank = *(int *)evdata->data;
            if (blank == FB_BLANK_POWERDOWN) {
                cts_suspend(cts_data);
                return NOTIFY_OK;
            }
        }
    }

    return NOTIFY_DONE;
}
#endif

static int cts_init_pm_fb_notifier(struct chipone_ts_data * cts_data)
{
    cts_info("Init FB notifier");

    cts_data->pdata->fb_notifier.notifier_call = fb_notifier_callback;

#ifdef CFG_CTS_DRM_NOTIFIER
    return msm_drm_register_client(&cts_data->pdata->fb_notifier);
#else
    return fb_register_client(&cts_data->pdata->fb_notifier);
#endif
}

static int cts_deinit_pm_fb_notifier(struct chipone_ts_data * cts_data)
{
    cts_info("Deinit FB notifier");
#ifdef CFG_CTS_DRM_NOTIFIER
    return msm_drm_unregister_client(&cts_data->pdata->fb_notifier)
#else
    return fb_unregister_client(&cts_data->pdata->fb_notifier);
#endif
}
#endif /* CONFIG_CTS_PM_FB_NOTIFIER */

#ifdef CONFIG_CTS_I2C_HOST
static int cts_driver_probe(struct i2c_client *client,
        const struct i2c_device_id *id)
#else
static int cts_driver_probe(struct spi_device *client)
#endif
{
    struct chipone_ts_data *cts_data = NULL;
    int ret = 0;
	
#ifdef CONFIG_CTS_I2C_HOST
    cts_info("Probe i2c client: name='%s' addr=0x%02x flags=0x%02x irq=%d",
        client->name, client->addr, client->flags, client->irq);

#if !defined(CONFIG_MTK_PLATFORM)
    if (client->addr != CTS_DEV_NORMAL_MODE_I2CADDR) {
        cts_err("Probe i2c addr 0x%02x != driver config addr 0x%02x",
            client->addr, CTS_DEV_NORMAL_MODE_I2CADDR);
        return -ENODEV;
    };
#endif

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        cts_err("Check functionality failed");
        return -ENODEV;
    }
#endif

    cts_data = (struct chipone_ts_data *)kzalloc(sizeof(*cts_data), GFP_KERNEL);
    if (cts_data == NULL) {
        cts_err("Allocate chipone_ts_data failed");
        return -ENOMEM;
    }

    cts_data->pdata = (struct cts_platform_data *)kzalloc(
            sizeof(struct cts_platform_data), GFP_KERNEL);
    if (cts_data->pdata == NULL) {
        cts_err("Allocate cts_platform_data failed");
        ret = -ENOMEM;
        goto err_free_cts_data;
    }

#ifdef CONFIG_CTS_I2C_HOST
    i2c_set_clientdata(client, cts_data);
    cts_data->i2c_client = client;
    cts_data->device = &client->dev;
#else
    spi_set_drvdata(client, cts_data);
    cts_data->spi_client = client;
    cts_data->device = &client->dev;
#endif

    cts_init_platform_data(cts_data->pdata, client);

    cts_data->cts_dev.pdata = cts_data->pdata;
    cts_data->pdata->cts_dev = &cts_data->cts_dev;

    cts_data->workqueue = create_singlethread_workqueue(CFG_CTS_DEVICE_NAME "-workqueue");
    if (cts_data->workqueue == NULL) {
        cts_err("Create workqueue failed");
        ret = -ENOMEM;
        goto err_deinit_platform_data;
    }

#ifdef CONFIG_CTS_ESD_PROTECTION
    cts_data->esd_workqueue = create_singlethread_workqueue(CFG_CTS_DEVICE_NAME "-esd_workqueue");
    if (cts_data->esd_workqueue == NULL) {
        cts_err("Create esd workqueue failed");
        ret = -ENOMEM;
        goto err_destroy_workqueue;
    }
#endif
    ret = cts_plat_request_resource(cts_data->pdata);
    if (ret < 0) {
        cts_err("Request resource failed %d", ret);
        goto err_destroy_esd_workqueue;
    }

    ret = cts_plat_reset_device(cts_data->pdata);
    if (ret < 0) {
        cts_err("Reset device failed %d", ret);
        goto err_free_resource;
    }

    ret = cts_probe_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Probe device failed %d", ret);
        goto err_free_resource;
    }

    ret = cts_plat_init_touch_device(cts_data->pdata);
    if (ret < 0) {
        cts_err("Init touch device failed %d", ret);
        goto err_free_resource;
    }

    ret = cts_plat_init_vkey_device(cts_data->pdata);
    if (ret < 0) {
        cts_err("Init vkey device failed %d", ret);
        goto err_deinit_touch_device;
    }

    ret = cts_plat_init_gesture(cts_data->pdata);
    if (ret < 0) {
        cts_err("Init gesture failed %d", ret);
        goto err_deinit_vkey_device;
    }

    cts_init_esd_protection(cts_data);

    ret = cts_tool_init(cts_data);
    if (ret < 0) {
        cts_warn("Init tool node failed %d", ret);
    }

    ret = cts_sysfs_add_device(&client->dev);
    if (ret < 0) {
        cts_warn("Add sysfs entry for device failed %d", ret);
    }

#ifdef CONFIG_CTS_PM_FB_NOTIFIER
    ret = cts_init_pm_fb_notifier(cts_data);
    if (ret) {
        cts_err("Init FB notifier failed %d", ret);
        goto err_deinit_sysfs;
    }
#endif /* CONFIG_CTS_PM_FB_NOTIFIER */

    ret = cts_plat_request_irq(cts_data->pdata);
    if (ret < 0) {
        cts_err("Request IRQ failed %d", ret);
        goto err_register_fb;
    }

    ret = cts_init_charger_detect(cts_data);
    if (ret) {
        cts_err("Init charger detect failed %d", ret);
        // Ignore this error
    }

    ret = cts_init_earjack_detect(cts_data);
    if (ret) {
        cts_err("Init earjack detect failed %d", ret);
        // Ignore this error
    }

	CTS_proc_touchpanel_dir = proc_mkdir(CTS_PROC_TOUCHPANEL_FOLDER, NULL);
		 printk("kaoshan %s %d ###\n",__func__,__LINE__);
    if (CTS_proc_touchpanel_dir == NULL) {
   		 cts_err(" %s: CTS_proc_touchpanel_dir file create failed!\n", __func__);
   	 	return -ENOMEM;
    }
    printk("cts %s %d ###\n",__func__,__LINE__);

    ret = cts_oem_init(cts_data);
    if (ret < 0) {
        cts_warn("Init oem specific faild %d", ret);
		goto err_oem_init_failed;
    }

	ret = cts_proc_init(cts_data);
	if (ret < 0) {
		cts_warn("Init proc init fialed %d", ret);
		goto err_proc_init_failed;
	}
	
    /* Init firmware upgrade work and schedule */
    INIT_DELAYED_WORK(&cts_data->fw_upgrade_work, cts_firmware_upgrade_work);
    queue_delayed_work(cts_data->workqueue, &cts_data->fw_upgrade_work, msecs_to_jiffies(3000));

    esd_tp_reset_notifier_register(&esd_tp_reset_notifier);

#if 0
    ret = cts_start_device(&cts_data->cts_dev);
    if (ret) {
        cts_err("Start device failed %d", ret);
        goto err_deinit_earjack_detect;
    }
#endif


	cts_esd = cts_data;
    //tp_create_sysfs(cts_data->device);
	cts_de = cts_data->pdata->cts_dev;
	get_hardware_info_data(HWID_CTP_DRIVER,lcd_name);
	tp_interface.charger_mode_switch_status = cts_charge_mode_switch;
    tp_interface.headset_switch_status = cts_headset_mode_switch;


    return 0;

err_proc_init_failed:
	cts_proc_deinit(cts_data);

err_oem_init_failed:
	cts_oem_deinit(cts_data);

//err_deinit_earjack_detect:
    cts_deinit_earjack_detect(cts_data);
    cts_deinit_charger_detect(cts_data);
    cts_plat_free_irq(cts_data->pdata);

err_register_fb:
#ifdef CONFIG_CTS_PM_FB_NOTIFIER
    cts_deinit_pm_fb_notifier(cts_data);
err_deinit_sysfs:
#endif /* CONFIG_CTS_PM_FB_NOTIFIER */
    cts_sysfs_remove_device(&client->dev);
#ifdef CONFIG_CTS_LEGACY_TOOL
    cts_tool_deinit(cts_data);
#endif /* CONFIG_CTS_LEGACY_TOOL */

#ifdef CONFIG_CTS_ESD_PROTECTION
    cts_deinit_esd_protection(cts_data);
#endif /* CONFIG_CTS_ESD_PROTECTION */

#ifdef CFG_CTS_GESTURE
    cts_plat_deinit_gesture(cts_data->pdata);
#endif /* CFG_CTS_GESTURE */

err_deinit_vkey_device:
#ifdef CONFIG_CTS_VIRTUALKEY
    cts_plat_deinit_vkey_device(cts_data->pdata);
#endif /* CONFIG_CTS_VIRTUALKEY */

err_deinit_touch_device:
    cts_plat_deinit_touch_device(cts_data->pdata);

err_free_resource:
    cts_plat_free_resource(cts_data->pdata);
err_destroy_esd_workqueue:
#ifdef CONFIG_CTS_ESD_PROTECTION
    destroy_workqueue(cts_data->esd_workqueue);
err_destroy_workqueue:
#endif
    destroy_workqueue(cts_data->workqueue);
err_deinit_platform_data:
    cts_deinit_platform_data(cts_data->pdata);
//err_free_pdata:
    kfree(cts_data->pdata);
err_free_cts_data:
    kfree(cts_data);

    cts_err("Probe failed %d", ret);

    return ret;
}

#ifdef CONFIG_CTS_I2C_HOST
static int cts_driver_remove(struct i2c_client *client)
#else
static int cts_driver_remove(struct spi_device *client)
#endif
{
    struct chipone_ts_data *cts_data;
    int ret = 0;

    cts_info("Remove");
    esd_tp_reset_notifier_unregister(&esd_tp_reset_notifier);

#ifdef CONFIG_CTS_I2C_HOST
    cts_data = (struct chipone_ts_data *)i2c_get_clientdata(client);
#else
    cts_data = (struct chipone_ts_data *)spi_get_drvdata(client);
#endif
    if (cts_data) {
        ret = cts_stop_device(&cts_data->cts_dev);
        if (ret) {
            cts_warn("Stop device failed %d", ret);
        }

        cts_deinit_charger_detect(cts_data);
        cts_deinit_earjack_detect(cts_data);

        cts_plat_free_irq(cts_data->pdata);

#ifdef CONFIG_CTS_PM_FB_NOTIFIER
        cts_deinit_pm_fb_notifier(cts_data);
#endif /* CONFIG_CTS_PM_FB_NOTIFIER */

        cts_tool_deinit(cts_data);

        cts_sysfs_remove_device(&client->dev);

        cts_deinit_esd_protection(cts_data);

        cts_plat_deinit_touch_device(cts_data->pdata);

        cts_plat_deinit_vkey_device(cts_data->pdata);

        cts_plat_deinit_gesture(cts_data->pdata);

        cts_plat_free_resource(cts_data->pdata);

		cts_oem_deinit(cts_data);

		cts_proc_deinit(cts_data);

#ifdef CONFIG_CTS_ESD_PROTECTION
        if (cts_data->esd_workqueue) {
            destroy_workqueue(cts_data->esd_workqueue);
        }
#endif

        if (cts_data->workqueue) {
            destroy_workqueue(cts_data->workqueue);
        }

        cts_deinit_platform_data(cts_data->pdata);

        if (cts_data->pdata) {
            kfree(cts_data->pdata);
        }
        kfree(cts_data);
    }else {
        cts_warn("Chipone i2c driver remove while NULL chipone_ts_data");
        return -EINVAL;
    }

    return ret;
}

#ifdef CONFIG_CTS_PM_LEGACY
static int cts_i2c_driver_suspend(struct device *dev, pm_message_t state)

{
    cts_info("Suspend by legacy power management");
    return cts_suspend(dev_get_drvdata(dev));
}

static int cts_i2c_driver_resume(struct device *dev)
{
    cts_info("Resume by legacy power management");
    return cts_resume(dev_get_drvdata(dev));
}
#endif /* CONFIG_CTS_PM_LEGACY */

#ifdef CONFIG_CTS_PM_GENERIC
static int cts_i2c_driver_pm_suspend(struct device *dev)
{
    cts_info("Suspend by bus power management");
    return cts_suspend(dev_get_drvdata(dev));
}

static int cts_i2c_driver_pm_resume(struct device *dev)
{
    cts_info("Resume by bus power management");
    return cts_resume(dev_get_drvdata(dev));
}

/* bus control the suspend/resume procedure */
static const struct dev_pm_ops cts_i2c_driver_pm_ops = {
    .suspend = cts_i2c_driver_pm_suspend,
    .resume = cts_i2c_driver_pm_resume,
};
#endif /* CONFIG_CTS_PM_GENERIC */

#ifdef CONFIG_CTS_SYSFS
static ssize_t reset_pin_show(struct device_driver *driver, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CFG_CTS_HAS_RESET_PIN: %c\n",
#ifdef CFG_CTS_HAS_RESET_PIN
        'Y'
#else
        'N'
#endif
        );
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(reset_pin, S_IRUGO, reset_pin_show, NULL);
#else
static DRIVER_ATTR_RO(reset_pin);
#endif

static ssize_t swap_xy_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CFG_CTS_SWAP_XY: %c\n",
#ifdef CFG_CTS_SWAP_XY
        'Y'
#else
        'N'
#endif
        );
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(swap_xy, S_IRUGO, swap_xy_show, NULL);
#else
static DRIVER_ATTR_RO(swap_xy);
#endif

static ssize_t wrap_x_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CFG_CTS_WRAP_X: %c\n",
#ifdef CFG_CTS_WRAP_X
        'Y'
#else
        'N'
#endif
        );
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(wrap_x, S_IRUGO, wrap_x_show, NULL);
#else
static DRIVER_ATTR_RO(wrap_x);
#endif

static ssize_t wrap_y_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CFG_CTS_WRAP_Y: %c\n",
#ifdef CFG_CTS_WRAP_Y
        'Y'
#else
        'N'
#endif
        );
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(wrap_y, S_IRUGO, wrap_y_show, NULL);
#else
static DRIVER_ATTR_RO(wrap_y);
#endif

static ssize_t force_update_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CFG_CTS_HAS_RESET_PIN: %c\n",
#ifdef CFG_CTS_FIRMWARE_FORCE_UPDATE
        'Y'
#else
        'N'
#endif
        );
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(force_update, S_IRUGO, force_update_show, NULL);
#else
static DRIVER_ATTR_RO(force_update);
#endif

static ssize_t max_touch_num_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CFG_CTS_MAX_TOUCH_NUM: %d\n",
        CFG_CTS_MAX_TOUCH_NUM);
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(max_touch_num, S_IRUGO, max_touch_num_show, NULL);
#else
static DRIVER_ATTR_RO(max_touch_num);
#endif

static ssize_t vkey_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CONFIG_CTS_VIRTUALKEY: %c\n",
#ifdef CONFIG_CTS_VIRTUALKEY
        'Y'
#else
        'N'
#endif
        );
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(vkey, S_IRUGO, vkey_show, NULL);
#else
static DRIVER_ATTR_RO(vkey);
#endif

static ssize_t gesture_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CFG_CTS_GESTURE: %c\n",
#ifdef CFG_CTS_GESTURE
        'Y'
#else
        'N'
#endif
        );
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(gesture, S_IRUGO, gesture_show, NULL);
#else
static DRIVER_ATTR_RO(gesture);
#endif

static ssize_t esd_protection_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CONFIG_CTS_ESD_PROTECTION: %c\n",
#ifdef CONFIG_CTS_ESD_PROTECTION
        'Y'
#else
        'N'
#endif
        );
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(esd_protection, S_IRUGO, esd_protection_show, NULL);
#else
static DRIVER_ATTR_RO(esd_protection);
#endif

static ssize_t slot_protocol_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "CONFIG_CTS_SLOTPROTOCOL: %c\n",
#ifdef CONFIG_CTS_SLOTPROTOCOL
        'Y'
#else
        'N'
#endif
        );
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(slot_protocol, S_IRUGO, slot_protocol_show, NULL);
#else
static DRIVER_ATTR_RO(slot_protocol);
#endif

static ssize_t max_xfer_size_show(struct device_driver *dev, char *buf)
{
#ifdef CONFIG_CTS_I2C_HOST
    return scnprintf(buf, PAGE_SIZE, "CFG_CTS_MAX_I2C_XFER_SIZE: %d\n",
        CFG_CTS_MAX_I2C_XFER_SIZE);
#else
    return scnprintf(buf, PAGE_SIZE, "CFG_CTS_MAX_SPI_XFER_SIZE: %d\n",
        CFG_CTS_MAX_SPI_XFER_SIZE);
#endif
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(max_xfer_size, S_IRUGO, max_xfer_size_show, NULL);
#else
static DRIVER_ATTR_RO(max_xfer_size);
#endif

static ssize_t driver_info_show(struct device_driver *dev, char *buf)
{
    return scnprintf(buf, PAGE_SIZE, "Driver version: %s\n", CFG_CTS_DRIVER_VERSION);
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,14,0)
static DRIVER_ATTR(driver_info, S_IRUGO, driver_info_show, NULL);
#else
static DRIVER_ATTR_RO(driver_info);
#endif

static struct attribute *cts_i2c_driver_config_attrs[] = {
    &driver_attr_reset_pin.attr,
    &driver_attr_swap_xy.attr,
    &driver_attr_wrap_x.attr,
    &driver_attr_wrap_y.attr,
    &driver_attr_force_update.attr,
    &driver_attr_max_touch_num.attr,
    &driver_attr_vkey.attr,
    &driver_attr_gesture.attr,
    &driver_attr_esd_protection.attr,
    &driver_attr_slot_protocol.attr,
    &driver_attr_max_xfer_size.attr,
    &driver_attr_driver_info.attr,
    NULL
};

static const struct attribute_group cts_i2c_driver_config_group = {
    .name = "config",
    .attrs = cts_i2c_driver_config_attrs,
};

static const struct attribute_group *cts_i2c_driver_config_groups[] = {
    &cts_i2c_driver_config_group,
    NULL,
};
#endif /* CONFIG_CTS_SYSFS */

#ifdef CONFIG_CTS_OF
static const struct of_device_id cts_i2c_of_match_table[] = {
    {.compatible = CFG_CTS_OF_DEVICE_ID_NAME,},
    { },
};
MODULE_DEVICE_TABLE(of, cts_i2c_of_match_table);
#endif /* CONFIG_CTS_OF */

#ifdef CONFIG_CTS_I2C_HOST
static const struct i2c_device_id cts_device_id_table[] = {
    {CFG_CTS_DEVICE_NAME, 0},
    {}
};
#else
static const struct spi_device_id cts_device_id_table[] = {
    {CFG_CTS_DEVICE_NAME, 0},
    {}
};
#endif

#ifdef CONFIG_CTS_I2C_HOST
static struct i2c_driver cts_i2c_driver = {
#else
static struct spi_driver cts_spi_driver = {
#endif
    .probe = cts_driver_probe,
    .remove = cts_driver_remove,
    .driver = {
        .name = CFG_CTS_DRIVER_NAME,
        .owner = THIS_MODULE,
#ifdef CONFIG_CTS_OF
        .of_match_table = of_match_ptr(cts_i2c_of_match_table),
#endif /* CONFIG_CTS_OF */
#ifdef CONFIG_CTS_SYSFS
        .groups = cts_i2c_driver_config_groups,
#endif /* CONFIG_CTS_SYSFS */
#ifdef CONFIG_CTS_PM_LEGACY
        .suspend = cts_i2c_driver_suspend,
        .resume  = cts_i2c_driver_resume,
#endif /* CONFIG_CTS_PM_LEGACY */
#ifdef CONFIG_CTS_PM_GENERIC
        .pm = &cts_i2c_driver_pm_ops,
#endif /* CONFIG_CTS_PM_GENERIC */

    },
    .id_table = cts_device_id_table,
};

static int __init cts_driver_init(void)
{

    cts_info("deng_Init");
	if  (lcd_name == NULL){
         printk("lcd name  is null return \n");
         return 0 ;
        }
    if (strncmp("lcd_icnl9911C_boe_mipi_hd", lcd_name, 28) == 0) {
#ifdef CONFIG_CTS_I2C_HOST
        cts_info(" start cts_i2c_driver");
        return i2c_add_driver(&cts_i2c_driver);
#else
        cts_info(" start cts_spi_driver");
        return spi_register_driver(&cts_spi_driver);
#endif
    } else {
        cts_err(" cts lcd not match");
        return 0;
    }
}

static void __exit cts_driver_exit(void)
{
    cts_info("Exit");

    if (strncmp("lcd_icnl9911C_boe_mipi_hd", lcd_name, 28) == 0) {
#ifdef CONFIG_CTS_I2C_HOST
    i2c_del_driver(&cts_i2c_driver);
#else
    spi_unregister_driver(&cts_spi_driver);
#endif
    }
}

module_init(cts_driver_init);
module_exit(cts_driver_exit);

MODULE_DESCRIPTION("Chipone TDDI touchscreen Driver for QualComm platform");
MODULE_VERSION(CFG_CTS_DRIVER_VERSION);
MODULE_AUTHOR("Miao Defang <dfmiao@chiponeic.com>");
MODULE_LICENSE("GPL");



