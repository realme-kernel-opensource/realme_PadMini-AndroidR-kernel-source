#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/param.h>
#include <linux/stat.h>
#include <linux/irq.h>
#include <linux/of_gpio.h>
//#include <linux/mfd/sprd/pmic_glb_reg.h>
#include <linux/rtc.h>
#include "ocp2132wpad.h"


void ocp2132_lcd_resume(void)
{
	ocp2132wpad_gpio_opt(ocp_p_gpio,1);
	mdelay(1); // 10
	Ocp2132wpad_I2C_Write(ocp2132wpad_i2c_client, OCP2131_AVDD_ADDR, OCP2131_AVDD_5V5);
	//mdelay(3);
	Ocp2132wpad_I2C_Write(ocp2132wpad_i2c_client, OCP2131_AVEE_ADDR, OCP2131_AVEE_5V5);
	//mdelay(3);
	Ocp2132wpad_I2C_Write(ocp2132wpad_i2c_client, 0xFF, 0x80);
	mdelay(1); // 3
	ocp2132wpad_gpio_opt(ocp_n_gpio,1);
	mdelay(1); // 3
           //printk("ocp2132_lcd_resume \n");
}
//extern uint8_t HX_SMWP_EN;
void ocp2132_lcd_suspend(void)
{
	//if(HX_SMWP_EN == 0){
	ocp2132wpad_gpio_opt(ocp_n_gpio,0);
	mdelay(2); // 22
	ocp2132wpad_gpio_opt(ocp_p_gpio,0);
	mdelay(1);
		//printk("ocp2132_lcd_suspend gesture off \n");
	//}else{
           	//printk("ocp2132_lcd_suspend gesture on nothing to do\n");
	//}

}

int ocp2132wpad_i2c_write(struct i2c_client *client, uint8_t regaddr, uint8_t data, uint8_t txbyte)
{
    uint8_t buffer[2];
    int ret = 0;
    int retry;

	if(!client)  return -1;
    buffer[0] = regaddr ;
    buffer[1] = data;

    for(retry = 0; retry < OCP_2132_RETRY_COUNT; retry++)
    {
        ret = i2c_master_send(client, buffer, txbyte);
        if (ret == txbyte)
        {
            break;
        }

        //printk("i2c write error,TXBYTES %d\n",ret);
        mdelay(10);
    }

    if(retry>=OCP_2132_RETRY_COUNT)
    {
        //printk("i2c write retry over %d\n", OCP_2132_RETRY_COUNT);
        return -EINVAL;
    }
    return ret;
}

int Ocp2132wpad_I2C_Write(struct i2c_client *client, uint8_t regaddr, uint8_t data)
{
    int ret = 0;
    ret = ocp2132wpad_i2c_write(client, regaddr, data, 0x02);
    return ret;
}

void ocp2132wpad_gpio_opt(int pin_num,int state)
{

	OCP2132_GPIO_OUTPUT(pin_num, state);
    msleep(2);
}

static s32 ocp2132wpad_request_io_port(void)
{
	s32 ret = 0;
	ret = gpio_request(ocp_p_gpio, "OCP_P_GPIO");
	if (ret < 0) {
		//printk("Failed to request GPIO:%d",ocp_p_gpio);
		ret = -1;
	} 
	ret = gpio_request(ocp_n_gpio, "OCP_N_GPIO");
	if (ret < 0) {
		//printk("Failed to request GPIO:%d",ocp_n_gpio);
		ret = -1;
	} 
	//printk("request GPIO1:%d GPIO2:%d",ocp_p_gpio,ocp_n_gpio);
	if (ret < 0) {
		gpio_free(ocp_p_gpio);
		gpio_free(ocp_n_gpio);
	}

	return ret;
}

static int ocp2132wpad_parse_dt(struct device *dev)
{
	struct device_node *np;
    if (!dev)
        return -ENODEV;

    np = dev->of_node;
	ocp_p_gpio = of_get_named_gpio(np, "ocp,p-gpio", 0);
	ocp_n_gpio = of_get_named_gpio(np, "ocp,n-gpio", 0);

    if (!gpio_is_valid(ocp_p_gpio) || !gpio_is_valid(ocp_n_gpio)) {
        printk("Invalid GPIO, ocp_p_gpio:%d, ocp_n_gpio:%d\n", ocp_p_gpio, ocp_n_gpio);
        return -EINVAL;
    } else
	    printk("ocp2132wpad_parse_dt ocp_p_gpio = %d ,ocp_n_gpio=%d\n",ocp_p_gpio,ocp_n_gpio);
	
    return 0;
}

static int ocp2132wpad_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	ocp2132wpad_i2c_client = client;
	
	if (client->dev.of_node) {
		ocp2132wpad_parse_dt(&client->dev);
	} 
	ret = ocp2132wpad_request_io_port();
	if (ret < 0) {
		//printk("ocp2132wpad request IO port failed.");
	}

	return 0;
}
static int ocp2132wpad_remove(struct i2c_client *client)
{	
	gpio_free(ocp_p_gpio);
	gpio_free(ocp_n_gpio);
	return 0;
}
static const struct i2c_device_id ocp2132wpad_id[] = {
	{OCP2132PWAD_NAME, 0},
	{}
};
static const struct of_device_id ocp2132wpad_of_match[] = {
	{.compatible = "ocp,ocp_wpad",},
	{}
};

static struct i2c_driver ocp2132wpad_i2c_driver = {
	.probe = ocp2132wpad_probe,
	.remove = ocp2132wpad_remove,
	.id_table = ocp2132wpad_id,
	.driver = {
		   .name = "ocp,ocp_wpad",
		   .owner = THIS_MODULE,
		   .of_match_table = ocp2132wpad_of_match,
		   },
};

static int  __init ocp2132wpad_i2c_init(void)
{
	/*
	struct rtc_time tm1; 
	struct timeval tv1 = { 0 };
	do_gettimeofday(&tv1); 
	rtc_time_to_tm(tv1.tv_sec, &tm1); 
	printk("lizhen ocp2132wpad_i2c_init %d-%02d-%02d%02d:%02d:%02d.%u; \n",
		tm1.tm_year + 1900, tm1.tm_mon + 1,tm1.tm_mday, 
		tm1.tm_hour, tm1.tm_min, tm1.tm_sec, (unsigned int)tv1.tv_usec );
	*/	
	return i2c_add_driver(&ocp2132wpad_i2c_driver);
}

static void  __exit ocp2132wpad_i2c_exit(void)
{
	i2c_del_driver(&ocp2132wpad_i2c_driver);
}

subsys_initcall_sync(ocp2132wpad_i2c_init);
module_exit(ocp2132wpad_i2c_exit);

MODULE_LICENSE("GPL");
