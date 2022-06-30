#include "../../../../fs/proc/internal.h"
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/device_info.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define DEVINFO_NAME "devinfo"
#define INFO_BUF_LEN 64
/**for definfo log**/
#define log_fmt(fmt) "[line:%d][module:%s][%s] " fmt

#define DEVINFO_ERR(a, arg...)                                           \
	do {                                                                 \
		printk(KERN_NOTICE log_fmt(a), __LINE__, DEVINFO_NAME, __func__, \
			   ##arg);                                                   \
	} while (0)

#define DEVINFO_MSG(a, arg...)                                                 \
	do {                                                                       \
		printk(KERN_INFO log_fmt(a), __LINE__, DEVINFO_NAME, __func__, ##arg); \
	} while (0)

/**definfo log end**/
static char *fuse_flag;
static char *devinfo_prj_name;
static char *devinfo_pcb_version;
int dtsi_name = -1;
int temp_value = -1;
typedef struct
{
	char *pcba_type_name;
} BOARD_TYPE_TABLE;

static BOARD_TYPE_TABLE pcba_type_table_nico[] = {
		{.pcba_type_name = "EVB"},
		{.pcba_type_name = "T0-1"},
		{.pcba_type_name = "T0-2"},
		{.pcba_type_name = "EVT1"},
		{.pcba_type_name = "DVT"},
		{.pcba_type_name = "PVT"},
		{.pcba_type_name = "MP"},
};

static BOARD_TYPE_TABLE pcba_type_table_nicky[] = {
		{.pcba_type_name = "EVB"},
		{.pcba_type_name = "T0-1"},
		{.pcba_type_name = "EVT1"},
		{.pcba_type_name = "EVT2"},
		{.pcba_type_name = "DVT"},
		{.pcba_type_name = "PVT"},
		{.pcba_type_name = "MP"},
};

static struct of_device_id devinfo_id[] = {
	{
		.compatible = "oplus, device_info",
	},
	{},
};

struct devinfo_data {
	struct platform_device *devinfo;
	struct pinctrl *pinctrl;
	int hw_id1_gpio;
	int hw_id2_gpio;
	int hw_id3_gpio;
	int sub_hw_id1;
	int sub_hw_id2;
	int main_hw_id5;
	int main_hw_id6;
	int sub_board_id;
	int sdcard_gpio;
	struct pinctrl_state *hw_sub_id_sleep;
	struct pinctrl_state *hw_sub_id_active;
	struct pinctrl_state *hw_main_id5_active;
	struct pinctrl_state *hw_main_id6_active;
	int ant_select_gpio;
	struct manufacture_info sub_mainboard_info;
};

static struct devinfo_data *dev_info;
static struct proc_dir_entry *parent = NULL;
char buff[128];
char sensor_vendor[3][80];
char fuse_vendor[80];

struct sensor_devinfo sensordevinfo[] = {
	{"light_stk3a5x", "SensorTek"},
	{"light_mn78911", "mn"},
	{"accelerometer_mc3416", "MCUBE"},
	{"accelerometer_sc7a20", "SLANWEI"},
	{"magnetic_mmc5603", "MMC"},
	{"magnetic_akm09919", "AKM"},
};

static void hq_parse_sensor_devinfo(int type, char ic_name[], char vendor[])
{
	sprintf(sensor_vendor[type], "Device version:\t\t%s\nDevice manufacture:\t\t%s\n",
			ic_name, vendor);
}

void hq_register_sensor_info(int type, char ic_name[])
{
	int i;
	if (type >= 0 && type < 3) {
		for (i = 0; i < sizeof(sensordevinfo) / sizeof(struct sensor_devinfo); i++) {
			/* Jianmin.Niu@ODM.HQ.BSP.Sensors.Config 2019/2/1 Update string compare */
			if (!strncmp(ic_name, sensordevinfo[i].ic_name, strlen(ic_name))) {
				hq_parse_sensor_devinfo(type, sensordevinfo[i].ic_name, sensordevinfo[i].vendor_name);
				break;
			}
		}
	}
	return;
}

static const char *const devinfo_proc_list[] = {
	"Sensor_accel",
	"Sensor_alsps",
	"Sensor_gyro",
	"Sensor_msensor",
	"Fuse_flag",
};

HQ_DEVINFO_ATTR(Sensor_accel, "%s", sensor_vendor[ACCEL_HQ]);
HQ_DEVINFO_ATTR(Sensor_alsps, "%s", sensor_vendor[ALSPS_HQ]);
HQ_DEVINFO_ATTR(Sensor_gyro, "%s", sensor_vendor[ACCEL_HQ]);
HQ_DEVINFO_ATTR(Sensor_msensor, "%s", sensor_vendor[MSENSOR_HQ]);
HQ_DEVINFO_ATTR(Fuse_flag, "%s", fuse_vendor);

static const struct file_operations *proc_fops_list[] = {
	&Sensor_accel_fops,
	&Sensor_alsps_fops,
	&Sensor_gyro_fops,
	&Sensor_msensor_fops,
	&Fuse_flag_fops,
};

static int __init fuse_flag_setup(char *str)
{
	fuse_flag = str;
	sprintf(fuse_vendor, "Fuse flag :%s\n", fuse_flag);
	return 0;
}
__setup("isfuse=", fuse_flag_setup);

static int __init op_prj_name_setup(char *str)
{
	devinfo_prj_name = str;

	return 0;
}
__setup("prj_name=", op_prj_name_setup);

static unsigned int atoi(const char *str)
{
	unsigned int value = 0;

	while (*str >= '0' && *str <= '9') {
		value *= 10;
		value += *str - '0';
		str++;
	}
	return value;
}

static int __init op_pcb_version_setup(char *str)
{
	temp_value = atoi(str);
	return 0;
}

static int __init op_pcb_dtsi_name_setup(char *str)
{
	dtsi_name = atoi(str);
	return 0;
}
__setup("dtsi_name=", op_pcb_dtsi_name_setup);
__setup("pcb_version=", op_pcb_version_setup);

static void *device_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter = 0;
	if (*pos == 0) {
		return &counter;
	} else {
		*pos = 0;
		return NULL;
	}
}

static void *device_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	return NULL;
}

static void device_seq_stop(struct seq_file *s, void *v) { return; }

static int device_seq_show(struct seq_file *s, void *v)
{
	struct proc_dir_entry *pde = s->private;
	struct manufacture_info *info = pde->data;
	if (info) {
		seq_printf(s, "Device version:\t\t%s\nDevice manufacture:\t\t%s\n",
				   info->version, info->manufacture);
		if (info->fw_path)
			seq_printf(s, "Device fw_path:\t\t%s\n", info->fw_path);
	}
	return 0;
}

static struct seq_operations device_seq_ops = {.start = device_seq_start,
											   .next = device_seq_next,
											   .stop = device_seq_stop,
											   .show = device_seq_show};

static int device_proc_open(struct inode *inode, struct file *file)
{
	int ret = seq_open(file, &device_seq_ops);
	pr_err("%s is called\n", __func__);

	if (!ret) {
		struct seq_file *sf = file->private_data;
		sf->private = PDE(inode);
	}

	return ret;
}

static const struct file_operations device_node_fops = {
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
	.open = device_proc_open,
	.owner = THIS_MODULE,
};

int register_device_proc(char *name, char *version, char *manufacture)
{
	struct proc_dir_entry *d_entry;
	struct manufacture_info *info;

	if (!parent) {
		parent = proc_mkdir("devinfo", NULL);
		if (!parent) {
			pr_err("can't create devinfo proc\n");
			return -ENOENT;
		}
	}

	info = kzalloc(sizeof *info, GFP_KERNEL);
	info->version = version;
	info->manufacture = manufacture;
	d_entry = proc_create_data(name, S_IRUGO, parent, &device_node_fops, info);
	if (!d_entry) {
		DEVINFO_ERR("create %s proc failed.\n", name);
		kfree(info);
		return -ENOENT;
	}
	return 0;
}

int register_devinfo(char *name, struct manufacture_info *info)
{
	struct proc_dir_entry *d_entry;
	if (!parent) {
		parent = proc_mkdir("devinfo", NULL);
		if (!parent) {
			pr_err("can't create devinfo proc\n");
			return -ENOENT;
		}
	}

	d_entry = proc_create_data(name, S_IRUGO, parent, &device_node_fops, info);
	if (!d_entry) {
		pr_err("create %s proc failed.\n", name);
		return -ENOENT;
	}
	return 0;
}

static int mainboard_init(void)
{
	unsigned int max_value;
	max_value = ARRAY_SIZE(pcba_type_table_nicky);
	if (temp_value < 0 && temp_value > max_value) {
		pr_err("temp_value error %d\n", temp_value);
		return register_device_proc("mainboard", "UNKNOW", devinfo_prj_name);
	}
	if (dtsi_name == 21660) {
		devinfo_pcb_version = pcba_type_table_nico[temp_value].pcba_type_name;
	} else if (dtsi_name == 0x2171A || dtsi_name == 21723) {
		devinfo_pcb_version = pcba_type_table_nicky[temp_value].pcba_type_name;
	} else {
		pr_err("dtsi_name error %d\n", dtsi_name);
		return register_device_proc("mainboard", "DTSI_NAME UNKNOW", devinfo_prj_name);
	}
	return register_device_proc("mainboard", devinfo_pcb_version, devinfo_prj_name);
}

static int subboard_init(struct devinfo_data *const devinfo_data)
{
	return register_device_proc("sub_mainboard", "SPRD", "sub-match");
}

static int devinfo_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct devinfo_data *const devinfo_data =
		devm_kzalloc(&pdev->dev, sizeof(struct devinfo_data), GFP_KERNEL);

	if (IS_ERR_OR_NULL(devinfo_data)) {
		printk("devinfo_data kzalloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	devinfo_data->devinfo = pdev;
	dev_info = devinfo_data;

	if (!parent) {
		parent = proc_mkdir("devinfo", NULL);
		if (!parent) {
			printk("can't create devinfo proc\n");
			ret = -ENOENT;
		}
	}
	proc_create("devinfo/Sensor_accel", 0444, NULL, proc_fops_list[0]);
	proc_create("devinfo/Sensor_alsps", 0444, NULL, proc_fops_list[1]);
	proc_create("devinfo/Sensor_gyro", 0444, NULL, proc_fops_list[2]);
	proc_create("devinfo/Sensor_msensor", 0444, NULL, proc_fops_list[3]);
	proc_create("devinfo/Fuse_flag", 0444, NULL, proc_fops_list[4]);
	ret = register_device_proc("audio_mainboard", "SPRD", "sub-match");
	if (ret < 0) {
		DEVINFO_ERR("register audio_mainboard failed\n");
	}

	ret = subboard_init(devinfo_data);
	if (ret < 0) {
		DEVINFO_ERR("register subboard failed\n");
	}

	ret = mainboard_init();
	if (ret < 0) {
		DEVINFO_ERR("register mainboard failed\n");
	}
	return ret;
}

static int devinfo_remove(struct platform_device *dev)
{
	remove_proc_entry(DEVINFO_NAME, NULL);
	return 0;
}

static struct platform_driver devinfo_platform_driver = {
	.probe = devinfo_probe,
	.remove = devinfo_remove,
	.driver =
		{
			.name = DEVINFO_NAME,
			.of_match_table = devinfo_id,
		},
};

module_platform_driver(devinfo_platform_driver);

MODULE_DESCRIPTION("oplus device info");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lzt<linzhengtao@vanyol.com>");
