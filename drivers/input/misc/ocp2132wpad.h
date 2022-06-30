#ifndef __OCP2132WPAD__
#define __OCP2132WPAD__

struct i2c_client* ocp2132wpad_i2c_client  = NULL;;
#define OCP2132_GPIO_OUTPUT(pin, level)      gpio_direction_output(pin, level)
#define OCP2132PWAD_NAME                     "ocp2132wpad"
#define OCP_2132_RETRY_COUNT 		         3
//+5.5
#define OCP2131_AVDD_ADDR			0x00
#define OCP2131_AVDD_5V5			0x14 //0x11

//-5.5
#define OCP2131_AVEE_ADDR			0x01
#define OCP2131_AVEE_5V5			0x14 //0x11

int ocp_p_gpio;
int ocp_n_gpio;
int ocp2132wpad_i2c_write(struct i2c_client *client, uint8_t regaddr, uint8_t data, uint8_t txbyte);
int Ocp2132wpad_I2C_Write(struct i2c_client *client, uint8_t regaddr, uint8_t data);
void ocp2132wpad_gpio_opt(int pin_num,int state);
void ocp2132_lcd_resume(void);
void ocp2132_lcd_suspend(void);


#endif
