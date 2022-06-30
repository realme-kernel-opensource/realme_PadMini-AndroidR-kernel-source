/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifdef CONFIG_T_PRODUCT_INFO

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/mmc/card.h>
#include <linux/mmc/mmc.h>             
#include <linux/mm.h>
#include "dev_info.h"
//#include <linux/dev_info.h>

/* /sys/devices/platform/$PRODUCT_DEVICE_INFO */
#define PRODUCT_DEVICE_INFO    "product-device-info"

///////////////////////////////////////////////////////////////////
static int dev_info_probe(struct platform_device *pdev);
static int dev_info_remove(struct platform_device *pdev);
static ssize_t store_product_dev_info(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);
static ssize_t show_product_dev_info(struct device *dev, struct device_attribute *attr, char *buf);
///////////////////////////////////////////////////////////////////
static product_info_arr prod_dev_array[ID_MAX];
static product_info_arr *pi_p = prod_dev_array;

static struct platform_driver dev_info_driver = {
	.probe = dev_info_probe,
	.remove = dev_info_remove,
	.driver = {
		   .name = PRODUCT_DEVICE_INFO,
	},
};

static struct platform_device dev_info_device = {
    .name = PRODUCT_DEVICE_INFO,
    .id = -1,
};

#define PRODUCT_DEV_INFO_ATTR(_name)                         \
{                                       \
	.attr = { .name = #_name, .mode = S_IRUGO | S_IWUSR | S_IWGRP,},  \
	.show = show_product_dev_info,                  \
	.store = store_product_dev_info,                              \
}

// emmc     /sys/block/mmcblk0/device/chipinfo
// battery  /sys/class/power_supply/battery/voltage_now

static struct device_attribute product_dev_attr_array[] = {
    PRODUCT_DEV_INFO_ATTR(info_lcd),
    PRODUCT_DEV_INFO_ATTR(info_tp),
    PRODUCT_DEV_INFO_ATTR(info_gyro),
    PRODUCT_DEV_INFO_ATTR(info_gsensor),
    PRODUCT_DEV_INFO_ATTR(info_psensor),
    PRODUCT_DEV_INFO_ATTR(info_msensor),
    PRODUCT_DEV_INFO_ATTR(info_sub_camera),
    PRODUCT_DEV_INFO_ATTR(info_main_camera),
    PRODUCT_DEV_INFO_ATTR(info_fingerprint),
    PRODUCT_DEV_INFO_ATTR(info_secboot),
    PRODUCT_DEV_INFO_ATTR(info_bl_lock_status),
    PRODUCT_DEV_INFO_ATTR(info_nfc),
    PRODUCT_DEV_INFO_ATTR(info_hall),
    PRODUCT_DEV_INFO_ATTR(info_flash),
    PRODUCT_DEV_INFO_ATTR(info_tp_check),
// add new ...
    
};

//add by wuhai for efuse check begin
static int efuse_id = 0;
static int get_secboot_from_uboot(char *str)
{
    klog("geroge  %s: %s\n",__func__,str);
    
    if(strcmp(str,"=5") == 0) efuse_id = 5;
    
    if(strcmp(str,"=1") == 0) efuse_id = 1;
    
    klog("geroge 1 efuse_id = %d\n",efuse_id);
    
    FULL_PRODUCT_DEVICE_INFO(ID_SECBOOT, (str != NULL ? str : "unknow"));
    return 1;
}

int get_sprd_secboot_cb(char *buf, void *args) 
{
   
    klog("geroge 2 efuse_id = %d\n",efuse_id); 
    
    if(efuse_id == 5)  		return sprintf(buf, "efuse: (%d)", 1);
    else if(efuse_id == 1)	return sprintf(buf, "efuse: (%d)", 0);
    else 	       		return sprintf(buf, "efuse: (%d)",-1);
}

__setup("secbootefuse.enable", get_secboot_from_uboot);
//add by wuhai for efuse check end


#define emmc_file "/sys/bus/platform/drivers/ufshcd-sprd/20200000.ufs/geometry_descriptor/raw_device_capacity"
//#define emmc_file "/sys/block/mmcblk0/size"
#define emmc_len  18
#define EMMC_VENDOR_CMP_SIZE  2

static unsigned int get_emmc_size(void)
{
    unsigned int emmc_size = 32;
    struct file *pfile = NULL;
    mm_segment_t old_fs;
    loff_t pos;
    ssize_t ret = 0;

    unsigned long long Size_buf=0;
    char buf_size[emmc_len];
    memset(buf_size, 0, sizeof(buf_size));

    pfile = filp_open(emmc_file, O_RDONLY, 0);
    if (IS_ERR(pfile)) {
        printk("[HWINFO]: open emmc size file failed!\n");
        goto ERR_0;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;

    ret = vfs_read(pfile, buf_size, emmc_len, &pos);
    if(ret <= 0) {
        printk("[HWINFO]: read emmc size file failed!\n");
        goto ERR_1;
    }

    Size_buf = simple_strtoull(buf_size, NULL, 0);
    Size_buf >>= 1; //Switch to KB
    emmc_size = (((unsigned int)Size_buf) / 1024) / 1024;

    if (emmc_size > 64) {
        emmc_size = 128;
    } else if (emmc_size > 32) {
        emmc_size = 64;
    } else if (emmc_size > 16) {
        emmc_size = 32;
    } else if (emmc_size > 8) {
        emmc_size = 16;
    } else if (emmc_size > 6) {
        emmc_size = 8;
    } else if (emmc_size > 4) {
        emmc_size = 6;
    } else if (emmc_size > 3) {
        emmc_size = 4;
    } else {
        emmc_size = 0;
    }

ERR_1:

    filp_close(pfile, NULL);

    set_fs(old_fs);

    return emmc_size;

ERR_0:
    return emmc_size;
}
#define EMMC_VENDOR_NAME "/sys/bus/platform/drivers/ufshcd-sprd/20200000.ufs/string_descriptors/manufacturer_name"
static int get_emmc_vendor_name(char* buff_name)
{
    struct file *pfile = NULL;
    mm_segment_t old_fs;
    loff_t pos;
	int len;

    ssize_t ret = 0;
    char vendor_name[emmc_len];
    memset(vendor_name, 0, sizeof(vendor_name));

    if(buff_name == NULL){
        return -1;
    }

    pfile = filp_open(EMMC_VENDOR_NAME, O_RDONLY, 0);
    if (IS_ERR(pfile)) {
        printk("[HWINFO]: open EMMC_VENDOR_NAME file failed!\n");
        goto ERR_0;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;

    ret = vfs_read(pfile, vendor_name, emmc_len, &pos);
    if(ret <= 0) {
        printk("[HWINFO]: read EMMC_VENDOR_NAME  file failed!\n");
        goto ERR_1;
    }
	len=strlen(vendor_name);
	vendor_name[len-1]=0;
	sprintf(buff_name,"%s",vendor_name);
ERR_1:

    filp_close(pfile, NULL);

    set_fs(old_fs);

    return 0;

ERR_0:
    return -1;
}
#define EMMC_VENDOR_VERSION "/sys/bus/platform/drivers/ufshcd-sprd/20200000.ufs/string_descriptors/product_revision"

static unsigned long long get_emmc_version(char *emmc_buf_size)
{
    struct file *pfile = NULL;
    mm_segment_t old_fs;
    loff_t pos;
    ssize_t ret = 0;

    unsigned long long Size_buf = 0;
    memset(emmc_buf_size, 0, 16);

    pfile = filp_open(EMMC_VENDOR_VERSION, O_RDONLY, 0);
    if (IS_ERR(pfile)) {
        printk("[HWINFO]: open emmc size file failed!\n");
        goto ERR_0;
    }

    old_fs = get_fs();
    set_fs(KERNEL_DS);
    pos = 0;

    ret = vfs_read(pfile, emmc_buf_size, 16, &pos);
    if(ret <= 0) {
        printk("[HWINFO]: read emmc size file failed!\n");
        goto ERR_1;
    }
	printk("Emmc_version %s",emmc_buf_size);

ERR_1:

    filp_close(pfile, NULL);

    set_fs(old_fs);

    return Size_buf;

ERR_0:
    return Size_buf;
}

int get_mmc_chip_info(char *buf, void *arg0)
{										
    //struct mmc_card *card = (struct mmc_card *)arg0;
    char tempID[64] = "";
    char vendorName[emmc_len] = "";
	char version[16] = "";
    char romsize[16] = "";
    char ramsize[8] = "";
	int size;
    struct sysinfo si;
    si_meminfo(&si);
    if(si.totalram > 1572864 )				   // 6G = 1572864 	(256 *1024)*6
   		strcpy(ramsize , "8G");
    else if(si.totalram > 1048576)			  // 4G = 786432 	(256 *1024)*4
    		strcpy(ramsize , "6G");
    else if(si.totalram > 786432)			 // 3G = 786432 	(256 *1024)*3
    		strcpy(ramsize , "4G");
    else if(si.totalram > 524288)			// 2G = 524288 	(256 *1024)*2
    		strcpy(ramsize , "3G");
    else if(si.totalram > 393216)               // 1.5G = 393216		(256 *1024)*1.5     4K page 
    		strcpy(ramsize , "2G");
    else if(si.totalram > 262144)             // 1G = 262144		(256 *1024)     4K page sizesize
    		strcpy(ramsize , "1.5G");
    else if(si.totalram > 131072)               // 512M = 131072		(256 *1024/2)   4K page size
    		strcpy(ramsize , "1G");
    else
    		strcpy(ramsize , "512M");
	size=get_emmc_size();

	memset(romsize,0,16);
	sprintf(romsize, "%dGB",size );

        printk("[HWINFO]: emmc size %d\n",size);
	get_emmc_vendor_name(vendorName);
	get_emmc_version(version);
    memset(tempID, 0, sizeof(tempID));
    sprintf(tempID,"%s_%s+%s,ver:%s",vendorName,romsize,ramsize,version);
   
    return sprintf(buf,"%s", tempID);					
}
///////////////////////////////////////////////////////////////////////////////////////////
/*
* int full_product_device_info(int id, const char *info, int (*cb)(char* buf, void *args), void *args);
*/
int full_product_device_info(int id, const char *info, FuncPtr cb, void *args) {   
    klog("%s: - [%d, %s, %pf]\n", __func__, id, info, cb); 

    if (id >= 0 &&  id < ID_MAX ) {
        memset(pi_p[id].show, 0, show_content_len);
        if (cb != NULL && pi_p[id].cb == NULL) {
            pi_p[id].cb = cb;
            pi_p[id].args = args; 
        }
        else if (info != NULL) {
            strcpy(pi_p[id].show, info);
            pi_p[id].cb = NULL;
            pi_p[id].args = NULL;
        }
        return 0;
    }
    return -1;
}

static ssize_t store_product_dev_info(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {
    return count;
}

static ssize_t show_product_dev_info(struct device *dev, struct device_attribute *attr, char *buf) {
    int i = 0;
    char *show = NULL;
    const ptrdiff_t x = (attr - product_dev_attr_array);
   
    if (x >= ID_MAX) {
        BUG_ON(1);
    }

    show = pi_p[x].show;
    if (pi_p[x].cb != NULL) {
        pi_p[x].cb(show, pi_p[x].args);
    }

    klog("%s: - offset(%d): %s\n", __func__, (int)x, show);
    if (strlen(show) > 0) {
        i = sprintf(buf, "%s ", show);
    }
    else {
        klog("%s - offset(%d): NULL!\n", __func__, (int)x);
    }

    return i;
}

static int dev_info_probe(struct platform_device *pdev) {	
    int i, rc;

    __FUN(); 
    for (i = 0; i < ARRAY_SIZE(product_dev_attr_array); i++) {
        rc = device_create_file(&pdev->dev, &product_dev_attr_array[i]);
        if (rc) {
            klog( "%s, create_attrs_failed:%d,%d\n", __func__, i, rc);
        }
    }
    //add by wuhai for efuse check begin
    FULL_PRODUCT_DEVICE_CB(ID_SECBOOT, get_sprd_secboot_cb, pdev);
    //add by wuhai for efuse check end
    return 0;
}

static int dev_info_remove(struct platform_device *pdev) {    
    int i;
	
     __FUN();
    for (i = 0; i < ARRAY_SIZE(product_dev_attr_array); i++) {
        device_remove_file(&pdev->dev, &product_dev_attr_array[i]);
    }
    return 0;
}

static int __init dev_info_drv_init(void) {
    __FUN();

    if (platform_device_register(&dev_info_device) != 0) {
        klog( "device_register fail!.\n");
        return -1;
    
    }
	
    if (platform_driver_register(&dev_info_driver) != 0) {
        klog( "driver_register fail!.\n");
        return -1;
    }
	
    return 0;
}

static void __exit dev_info_drv_exit(void) {
	__FUN();
	platform_driver_unregister(&dev_info_driver);
	platform_device_unregister(&dev_info_device);
}

///////////////////////////////////////////////////////////////////
late_initcall(dev_info_drv_init);
module_exit(dev_info_drv_exit);

#endif

