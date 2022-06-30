/***************************************************************
** Copyright (C),  2018,  OPPO Mobile Comm Corp.,  Ltd
** VENDOR_EDIT
** File : display_private_api.h
** Description : display private api implement
** Version : 1.0
** Date : 2018/03/20
** Author : Jie.Hu@PSW.MM.Display.Stability
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**   Hu.Jie          2018/03/20        1.0           Build this moudle
******************************************************************/
#include "display_private_api.h"

#include <linux/fb.h>
#include <linux/time.h>
#include <linux/timekeeping.h>


/*
 * we will create a sysfs which called /sys/kernel/display,
 * In that directory, display private api can be called
 */

unsigned long CABC_mode = 0;
unsigned long CABC_auto_mode = 0;
unsigned long CABC_sprd_mode = 0;

/* wangcheng@ODM.Multimedia.LCD  2021/04/12 solve cabc dump  start*/
extern unsigned int flag_bl;
/* wangcheng@ODM.Multimedia.LCD  2021/04/12 solve cabc dump  end*/

unsigned long tp_gesture = 0;

// extern int sprd_panel_cabc_mode(unsigned int level);
extern int sprd_panel_cabc(unsigned int level);
static ssize_t cabc_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
    printk("%s CABC_mode=%ld\n", __func__, CABC_mode);
    return sprintf(buf, "%ld\n", CABC_mode);
}

static ssize_t cabc_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t num)
{
    int ret = 0;
    ret = kstrtoul(buf, 10, &CABC_mode);
    printk("%s CABC_mode=%ld\n", __func__, CABC_mode);
  //  if (flag_bl ==0) {
  //      return 0;
	//}
	///ret = sprd_panel_cabc((unsigned int)CABC_mode);
	return num;
}

static struct kobj_attribute dev_attr_cabc =
	__ATTR(cabc, 0664, cabc_show, cabc_store);

static ssize_t cabc_auto_show(struct kobject *kobj, struct kobj_attribute *attr,
		 char *buf)
{
 printk("%s CABC_auto_mode=%ld\n", __func__, CABC_auto_mode);
 return sprintf(buf, "%ld\n", CABC_auto_mode);
}

static ssize_t cabc_auto_store(struct kobject *kobj, struct kobj_attribute *attr,
		  const char *buf, size_t num)
{
 int ret = 0;
 ret = kstrtoul(buf, 10, &CABC_auto_mode);
 printk("%s CABC_auto_mode=%ld\n", __func__, CABC_auto_mode);
 return num;
}
 
 static struct kobj_attribute dev_attr_cabc_auto =
	 __ATTR(cabc_auto, 0664, cabc_auto_show, cabc_auto_store);

 static ssize_t cabc_sprd_show(struct kobject *kobj, struct kobj_attribute *attr,
		  char *buf)
 {
  printk("%s CABC_sprd_mode=%ld\n", __func__, CABC_sprd_mode);
  return sprintf(buf, "%ld\n", CABC_sprd_mode);
 }
 
 static ssize_t cabc_sprd_store(struct kobject *kobj, struct kobj_attribute *attr,
		   const char *buf, size_t num)
 {
  int ret = 0;
  ret = kstrtoul(buf, 10, &CABC_sprd_mode);
  printk("%s CABC_sprd_mode=%ld\n", __func__, CABC_sprd_mode);
  return num;
 }
  
  static struct kobj_attribute dev_attr_cabc_sprd =
	  __ATTR(cabc_sprd, 0664, cabc_sprd_show, cabc_sprd_store);

/*add /sys/kernel/display/tp_gesture*/
static ssize_t tp_gesture_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
    printk("%s tp_gesture=%ld\n", __func__, tp_gesture);
    return sprintf(buf, "%ld\n", tp_gesture);
}

static ssize_t tp_gesture_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t num)
{
    int ret = 0;
    ret = kstrtoul(buf, 10, &tp_gesture);
    printk("%s tp_gesture_mode=%ld\n", __func__, tp_gesture);

	return num;
}


static struct kobj_attribute dev_attr_tp_gesture =
	__ATTR(tp_gesture, 0664, tp_gesture_show, tp_gesture_store);


static struct kobject *display_kobj;

//static DEVICE_ATTR_RW(cabc);

/*
 * Create a group of attributes so that we can create and destroy them all
 * at once.
 */
static struct attribute *display_attrs[] = {
	&dev_attr_cabc.attr,
	&dev_attr_tp_gesture.attr,
	&dev_attr_cabc_sprd.attr,
	&dev_attr_cabc_auto.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group display_attr_group = {
	.attrs = display_attrs,
};

static int __init display_private_api_init(void)
{
	int retval;

	display_kobj = kobject_create_and_add("display", kernel_kobj);
	if (!display_kobj)
		return -ENOMEM;

	/* Create the files associated with this kobject */
	retval = sysfs_create_group(display_kobj, &display_attr_group);
	if (retval)
		kobject_put(display_kobj);

	return retval;
}

static void __exit display_private_api_exit(void)
{
	kobject_put(display_kobj);
}

module_init(display_private_api_init);
module_exit(display_private_api_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Hujie <hujie@oppo.com>");
