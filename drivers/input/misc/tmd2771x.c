/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
*---------------------------------------------------------------------------------------------
*    when          who                     why                           comment_tag
*---------------------------------------------------------------------------------------------
* 2013-06-19  WangQingchuan   Add support for tmd2772(1/3)     FEIXUN_SENSOR_WANGQINGCHUAN_000
*---------------------------------------------------------------------------------------------
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/input/tmd2771x.h>
#include <linux/wakelock.h>
#include <linux/jiffies.h>
#include <mach/gpio.h>
#include <linux/gpio.h>
#include <linux/kthread.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/of_gpio.h>
#include <linux/sensors.h>

#ifdef CONFIG_DEVICE_VERSION
#include <mach/fdv.h>
#endif
#define TMD2771X_DRV_NAME	"tmd2771x"
#define DRIVER_VERSION		"1.0"

#define TMD2771X__POWER_ONOFF(pdata, onoff) \
        do { \
                if ((pdata)->power_onoff) { \
                        (pdata)->power_onoff(onoff); \
                } \
        } \
        while(0)

#define TMD2771X__ENABLE_REG	0x00
#define TMD2771X__ATIME_REG	0x01
#define TMD2771X__PTIME_REG	0x02
#define TMD2771X__WTIME_REG	0x03
#define TMD2771X__AILTL_REG	0x04
#define TMD2771X_AILTH_REG	0x05
#define TMD2771X_AIHTL_REG	0x06
#define TMD2771X_AIHTH_REG	0x07
#define TMD2771X_PILTL_REG	0x08
#define TMD2771X_PILTH_REG	0x09
#define TMD2771X_PIHTL_REG	0x0A
#define TMD2771X_PIHTH_REG	0x0B
#define TMD2771X_PERS_REG	0x0C
#define TMD2771X_CONFIG_REG	0x0D
#define TMD2771X_PPCOUNT_REG	0x0E
#define TMD2771X_CONTROL_REG	0x0F
#define TMD2771X_REV_REG	0x11
#define TMD2771X_ID_REG		0x12
#define TMD2771X_STATUS_REG	0x13
#define TMD2771X_CDATAL_REG	0x14
#define TMD2771X_CDATAH_REG	0x15
#define TMD2771X_IRDATAL_REG	0x16
#define TMD2771X_IRDATAH_REG	0x17
#define TMD2771X_PDATAL_REG	0x18
#define TMD2771X_PDATAH_REG	0x19

#define CMD_BYTE	0x80
#define CMD_WORD	0xA0
#define CMD_SPECIAL	0xE0

#define CMD_CLR_PS_INT	0xE5
#define CMD_CLR_ALS_INT	0xE6
#define CMD_CLR_PS_ALS_INT	0xE7
#define TMD2771x_I2C_ADDR 0x39
#define DESC                            "taos_alsprox_sensor"

/*
 * Structs
 */

struct tmd2771x_data {
	//put the platform special resource in platform code
	struct tmd2771x_platform_data *pdata;
	struct i2c_client *client;
	struct mutex update_lock;
	struct sensors_classdev als_cdev;
	struct sensors_classdev ps_cdev;
	//use a dedicated spinlock instead of dwork.wait_lock
	spinlock_t wq_lock;
	struct delayed_work	dwork;	/* for PS interrupt */
	struct delayed_work als_dwork; /* for ALS polling */
	struct input_dev *input_dev_als;
	struct input_dev *input_dev_ps;
	struct wake_lock prx_wake_lock;

	unsigned int enable;
	unsigned int atime;
	unsigned int ptime;
	unsigned int wtime;
	unsigned int ailt;
	unsigned int aiht;
	unsigned int pilt;
	unsigned int piht;
	unsigned int pers;
	unsigned int config;
	unsigned int ppcount;
	unsigned int control;

	struct pinctrl        *pinctrl;
        struct pinctrl_state  *pin_default;
        struct pinctrl_state  *pin_sleep;
	bool         power_enabled;

	/* control flag from HAL */
	unsigned int enable_ps_sensor;
	unsigned int enable_als_sensor;

	/* PS parameters */
	unsigned int ps_threshold;
	unsigned int ps_hysteresis_threshold; /* always lower than ps_threshold */
	unsigned int ps_detection;		/* 1 = near-to-far; 0 = far-to-near */
	unsigned int ps_data;			/* to store PS data */

	/* ALS parameters */
	unsigned int als_threshold_l;	/* low threshold */
	unsigned int als_threshold_h;	/* high threshold */
	unsigned int als_data;			/* to store ALS data */

	unsigned int als_gain;			/* needed for Lux calculation */
	unsigned int als_poll_delay;	/* needed for light sensor polling : micro-second (us) */
	unsigned int als_atime;			/* storage for als integratiion time */
	unsigned int ps_det_thld;
	unsigned int ps_hsyt_thld;
	unsigned int als_hsyt_thld;
};

/*
 * Global data
 */
static int light_data;
static int proximity_data;
static struct tmd2771x_data *data;
static unsigned int prox_on = 0;

#define CAL_MAX	1
#define CAL_MIN 2	
static int start_cal = 0;
static int pdata_temp_max[10];
static int pdata_temp_min[10];
static int pdata_max = 0;
static int pdata_min = 0;
static bool near_flag=false;

static struct sensors_classdev sensors_light_cdev = {
	
		.name = "TMD2771X-als",
		.vendor = "taos",
		.version = 1,
		.handle = SENSORS_LIGHT_HANDLE,
		.type = SENSOR_TYPE_LIGHT,
		.max_range = "(powf(10, (280.0f / 47.0f)) * 4)",
		.resolution = "1.0f",
		.sensor_power = "0.75f",
		.min_delay = 0,
		.fifo_reserved_event_count = 0,
		.fifo_max_event_count = 0,
		.enabled = 0,
		.delay_msec = 200,
		.sensors_enable = NULL,
		.sensors_poll_delay = NULL,	
};
static struct sensors_classdev sensors_proximity_cdev ={
	
		.name = "TMD2771X-ps",
		.vendor = "taos",
		.version = 1,
		.handle = SENSORS_PROXIMITY_HANDLE,
		.type = SENSOR_TYPE_PROXIMITY,
		.max_range = "5.0f",
		.resolution = "5.0f",
		.sensor_power = "0.75",
		.min_delay = 0,
		.fifo_reserved_event_count = 0,
		.fifo_max_event_count = 0,
		.enabled = 0,		
		.delay_msec = 200,
		.sensors_enable = NULL,
		.sensors_poll_delay = NULL,
};
//static struct timer_list readcal_timer;

/*
 * Management functions
 */

static int tmd2771x_set_command(struct i2c_client *client, int command)
{
	int ret;
	int clearInt;

	if (command == 0)
		clearInt = CMD_CLR_PS_INT;
	else if (command == 1)
		clearInt = CMD_CLR_ALS_INT;
	else
		clearInt = CMD_CLR_PS_ALS_INT;

	//It`s unnecessary to lock mutex before i2c_smbus_write_byte, which is thread-safe and SMP safe.
	ret = i2c_smbus_write_byte(client, clearInt);

	return ret;
}

static int tmd2771x_set_enable(struct i2c_client *client, int enable)
{
	int ret;
	//Yes, the mutex is necessary for modify data->data, but I will lock it before call this function
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X__ENABLE_REG, enable);
	data->enable = enable;

	return ret;
}

static int tmd2771x_set_atime(struct i2c_client *client, int atime)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X__ATIME_REG, atime);
	data->atime = atime;

	return ret;
}

static int tmd2771x_set_ptime(struct i2c_client *client, int ptime)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X__PTIME_REG, ptime);
	data->ptime = ptime;

	return ret;
}

static int tmd2771x_set_wtime(struct i2c_client *client, int wtime)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X__WTIME_REG, wtime);
	data->wtime = wtime;

	return ret;
}

static int tmd2771x_set_ailt(struct i2c_client *client, int threshold)
{
	int ret;
	ret = i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X__AILTL_REG, threshold);
	data->ailt = threshold;

	return ret;
}

static int tmd2771x_set_aiht(struct i2c_client *client, int threshold)
{
	int ret;
	ret = i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_AIHTL_REG, threshold);
	data->aiht = threshold;

	return ret;
}

static int tmd2771x_set_pilt(struct i2c_client *client, int threshold)
{
	int ret;
	ret = i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PILTL_REG, threshold);
	data->pilt = threshold;

	return ret;
}

static int tmd2771x_set_piht(struct i2c_client *client, int threshold)
{
	int ret;
	ret = i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PIHTL_REG, threshold);
	data->piht = threshold;

	return ret;
}

static int tmd2771x_set_pers(struct i2c_client *client, int pers)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X_PERS_REG, pers);
	data->pers = pers;

	return ret;
}

static int tmd2771x_set_config(struct i2c_client *client, int config)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X_CONFIG_REG, config);
	data->config = config;

	return ret;
}

static int tmd2771x_set_ppcount(struct i2c_client *client, int ppcount)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X_PPCOUNT_REG, ppcount);
	data->ppcount = ppcount;

	return ret;
}

static int tmd2771x_set_control(struct i2c_client *client, int control)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X_CONTROL_REG, control);
	data->control = control;

	/* obtain ALS gain value */
	if ((control&0x03) == 0x00) /* 1X Gain */
		data->als_gain = 1;
	else if ((control&0x03) == 0x01) /* 8X Gain */
		data->als_gain = 8;
	else if ((control&0x03) == 0x02) /* 16X Gain */
		data->als_gain = 16;
	else  /* 120X Gain */
		data->als_gain = 120;

	return ret;
}

static int LuxCalculation(struct i2c_client *client, int cdata, int irdata)
{
	int luxValue=0;

	int IAC1=0;
	int IAC2=0;
	int IAC=0;
	int GA=1064;			/* 0.48 without glass window */
	int COE_B=210;		/* 2.23 without glass window */
	int COE_C=29;		/* 0.70 without glass window */
	int COE_D=57;		/* 1.42 without glass window */
	int DF=52;

	IAC1 = (cdata - (COE_B*irdata)/100);	// re-adjust COE_B to avoid 2 decimal point
	IAC2 = ((COE_C*cdata)/100 - (COE_D*irdata)/100); // re-adjust COE_C and COE_D to void 2 decimal point

	if (IAC1 > IAC2)
		IAC = IAC1;
	else if (IAC1 <= IAC2)
		IAC = IAC2;
	else
		IAC = 0;

	luxValue = ((IAC*GA*DF)/100)/(((272*(256-data->atime))/100)*data->als_gain);

	return luxValue;
}

static void tmd2771x_change_ps_threshold(struct i2c_client *client)
{
	int retval;

	retval = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_PDATAL_REG);
	if (retval < 0)
		return;

	proximity_data = retval;
	data->ps_data = retval;
	if (prox_on ||( (data->ps_data <= data->pilt) && (data->ps_data < data->piht) )) {
		/* near-to-far detected */
		data->ps_detection = 0;

		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PIHTL_REG, data->ps_threshold);

		data->pilt = 0;
		data->piht = data->ps_threshold;
		
		near_flag=false;
		printk("near-to-far detected\n");
	}
	else if ( (data->ps_data > data->pilt) && (data->ps_data >= data->piht) ) {
		/* far-to-near detected */
		data->ps_detection = 1;
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 0);/* FAR-to-NEAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PILTL_REG, data->ps_hysteresis_threshold);
		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PIHTL_REG, 1023);

		data->pilt = data->ps_hysteresis_threshold;
		data->piht = 1023;
		
		near_flag=true;
		printk("far-to-near detected\n");
	}else {
		printk("data->ps_data = %d, data->pilt = %d, data->piht = %d\n", data->ps_data, data->pilt, data->piht);
	}
	prox_on = 0;
}
#if 0
static void tmd2771x_change_ps_threshold(struct i2c_client *client)
{
	int retval;

	retval = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_PDATAL_REG);
	if (retval < 0)
		return;

	proximity_data = retval;
	data->ps_data = retval;
	if ( (data->ps_data > data->pilt) && (data->ps_data >= data->piht) ) {
		/* far-to-near detected */
		data->ps_detection = 1;

		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 0);/* FAR-to-NEAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PILTL_REG, data->ps_hysteresis_threshold);
		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PIHTL_REG, 1023);

		data->pilt = data->ps_hysteresis_threshold;
		data->piht = 1023;
		printk("far-to-near detected\n");
	}
	else if ( (data->ps_data <= data->pilt) && (data->ps_data < data->piht) ) {
		/* near-to-far detected */
		data->ps_detection = 0;

		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PIHTL_REG, data->ps_threshold);

		data->pilt = 0;
		data->piht = data->ps_threshold;

		printk("near-to-far detected\n");
	} else {
		printk("data->ps_data = %d, data->pilt = %d, data->piht = %d\n", data->ps_data, data->pilt, data->piht);
	}
}
#endif

static void tmd2771x_change_als_threshold(struct i2c_client *client)
{
	int cdata, irdata;
	int luxValue=0;

	cdata = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_CDATAL_REG);
	irdata = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_IRDATAL_REG);

	luxValue = LuxCalculation(client, cdata, irdata);

	luxValue = luxValue>0 ? luxValue : 0;
	luxValue = luxValue<10000 ? luxValue : 10000;
	light_data = luxValue;
	// check PS under sunlight
	if ( (data->ps_detection == 1) && (cdata > (75*(1024*(256-data->atime)))/100))	// PS was previously in far-to-near condition
	{
		// need to inform input event as there will be no interrupt from the PS
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PIHTL_REG, data->ps_threshold);

		data->pilt = 0;
		data->piht = data->ps_threshold;

		data->ps_detection = 0;	/* near-to-far detected */

		printk("tmd2771x_proximity_handler = FAR\n");
	}

	input_report_abs(data->input_dev_als, ABS_MISC, luxValue); // report the lux level
	input_sync(data->input_dev_als);

	data->als_data = cdata;
	data->als_threshold_l = (data->als_data * (100-data->als_hsyt_thld) ) /100;
	data->als_threshold_h = (data->als_data * (100+data->als_hsyt_thld) ) /100;

	if (data->als_threshold_h >= 65535) data->als_threshold_h = 65535;

	i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X__AILTL_REG, data->als_threshold_l);
	i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_AIHTL_REG, data->als_threshold_h);
}

/* ALS polling routine */
static void tmd2771x_als_polling_work_handler(struct work_struct *work)
{
	struct i2c_client *client=data->client;
	int cdata, irdata, pdata;
	int luxValue=0;

	//1. work queue is reentrant in SMP.
	//2. serveral function here calls need mutex.
	mutex_lock(&data->update_lock);
	cdata = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_CDATAL_REG);
	irdata = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_IRDATAL_REG);
	pdata = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_PDATAL_REG);

	if ((cdata < 0) || (irdata < 0) || (pdata < 0)) {
		mutex_unlock(&data->update_lock);
	}

	luxValue = LuxCalculation(client, cdata, irdata);
	luxValue = luxValue>0 ? luxValue : 0;
	luxValue = luxValue<10000 ? luxValue : 10000;
	light_data = luxValue;
	//printk("polling***************%s: lux = %d cdata = %x  irdata = %x pdata = %x \n", __func__, luxValue, cdata, irdata, pdata);
	// check PS under sunlight
	if ( (data->ps_detection == 1) && (cdata > (75*(1024*(256-data->atime)))/100))	// PS was previously in far-to-near condition
	{
		// need to inform input event as there will be no interrupt from the PS
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|TMD2771X_PIHTL_REG, data->ps_threshold);

		data->pilt = 0;
		data->piht = data->ps_threshold;

		data->ps_detection = 0;	/* near-to-far detected */

		printk("tmd2771x_proximity_handler = FAR\n");
	}

	input_report_abs(data->input_dev_als, ABS_MISC, luxValue); // report the lux level
	input_sync(data->input_dev_als);
	schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// restart timer
	mutex_unlock(&data->update_lock);
}

/* PS interrupt routine */
static void tmd2771x_work_handler(struct work_struct *work)
{
	struct i2c_client *client=data->client;
	int	status;
	int cdata;
	int retry_count = 3;


retry:
	status = i2c_smbus_read_byte_data(client, CMD_BYTE|TMD2771X_STATUS_REG);
	if (status < 0) {
		printk("fail to read data,status = %x\n", status);
		if (retry_count--) {
			usleep(50000);
			goto retry;
		} else {
			return;
		}
	}

	i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X__ENABLE_REG, 1);	/* disable 2771x's ADC first */

	printk("*********tmd2771x_work_handler****status = %x, enable = %x\n", status, data->enable);
	//Interrupt is not reentrant both in UP and MP. But some variant/function here need to thread-safe.
	mutex_lock(&data->update_lock);

	if ((status & data->enable & 0x30) == 0x30) {
		/* both PS and ALS are interrupted */
		tmd2771x_change_als_threshold(client);

		cdata = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_CDATAL_REG);
		if (cdata < (75*(1024*(256-data->atime)))/100)
			tmd2771x_change_ps_threshold(client);
		else {
			if (data->ps_detection == 1) {
				tmd2771x_change_ps_threshold(client);
			}
			else {
				printk("Triggered by background ambient noise\n");
			}
		}

		tmd2771x_set_command(client, 2);	/* 2 = CMD_CLR_PS_ALS_INT */
	}
	else if ((status & data->enable & 0x20) == 0x20) {
		/* only PS is interrupted */
		/* check if this is triggered by background ambient noise */
		cdata = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_CDATAL_REG);
		if (cdata < (75*(1024*(256-data->atime)))/100)
			tmd2771x_change_ps_threshold(client);
		else {
			if (data->ps_detection == 1) {
				tmd2771x_change_ps_threshold(client);
			}
			else {
				printk("Triggered by background ambient noise\n");
			}
		}

		tmd2771x_set_command(client, 0);	/* 0 = CMD_CLR_PS_INT */
	}
	else if ((status & data->enable & 0x10) == 0x10) {
		/* only ALS is interrupted */
		tmd2771x_change_als_threshold(client);
		tmd2771x_set_command(client, 1);	/* 1 = CMD_CLR_ALS_INT */
	}
	i2c_smbus_write_byte_data(client, CMD_BYTE|TMD2771X__ENABLE_REG, data->enable);
	mutex_unlock(&data->update_lock);
}

/* assume this is ISR */
static irqreturn_t tmd2771x_interrupt(int vec, void *info)
{
	struct i2c_client *client=(struct i2c_client *)info;
	data = i2c_get_clientdata(client);
	printk("==> tmd2771x_interrupt (timeout)\n");
	
	if(data->enable_ps_sensor==0) {
	    return IRQ_HANDLED; 
	} 
	wake_lock_timeout(&data->prx_wake_lock, HZ / 2);
	//Seems linux-3.0 trends to use threaded-interrupt, so we can call the work directly.
	tmd2771x_work_handler(&data->dwork.work);

	return IRQ_HANDLED;
}


/*
 * SysFS support
 */
static ssize_t tmd2771x_show_ps_sensor_thld(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d %d\n", data->ps_hsyt_thld, data->ps_det_thld);
}

static ssize_t tmd2771x_store_ps_sensor_thld(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	char *next_buf;
	unsigned long hsyt_val = simple_strtoul(buf, &next_buf, 10);
	unsigned long det_val = simple_strtoul(++next_buf, NULL, 10);

	if ((det_val < 0) || (det_val > 1023) || (hsyt_val < 0) || (hsyt_val >= det_val)) {
		printk("%s:store unvalid det_val=%ld, hsyt_val=%ld\n", __func__, det_val, hsyt_val);
		return -EINVAL;
	}
	mutex_lock(&data->update_lock);
	data->ps_det_thld = det_val;
	data->ps_hsyt_thld = hsyt_val;
	mutex_unlock(&data->update_lock);

	return count;
}

static DEVICE_ATTR(ps_sensor_thld, S_IWUGO | S_IRUGO,
				   tmd2771x_show_ps_sensor_thld, tmd2771x_store_ps_sensor_thld);

static ssize_t tmd2771x_show_proximity_enable(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", data->enable_ps_sensor);
}

static void tmd2771x_power(struct i2c_client *client, bool enable);

static int tmd2771x_enable_ps_sensor(struct i2c_client *client, int val)
{
	struct tmd2771x_data *data = i2c_get_clientdata(client);
	unsigned long flags;
	if ((val != 0) && (val != 1)) {
		printk("%s: unvalid value=%d\n", __func__, val);
	}
	
	mutex_lock(&data->update_lock);

	if(val == 1) {
		//turn on p sensor
		if (data->enable_ps_sensor==0) {
		    tmd2771x_power(client, true);
			prox_on = 1;
			data->enable_ps_sensor= 1;
			tmd2771x_set_enable(client,0); /* Power Off */
			tmd2771x_set_atime(client, 0xf6); /* 27.2ms */
			tmd2771x_set_ptime(client, 0xff); /* 2.72ms */
			tmd2771x_set_ppcount(client, 8); /* 8-pulse */
			tmd2771x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
			tmd2771x_set_pilt(client, 0); //modify by zhll for first report value
			tmd2771x_set_piht(client, 0);
		        //tmd2771x_set_pilt(client, 0);		// init threshold for proximity
		        //tmd2771x_set_piht(client, data->pdata->ps_det_thld);
			data->ps_threshold = data->ps_det_thld;
			data->ps_hysteresis_threshold = data->ps_hsyt_thld;
			tmd2771x_set_ailt( client, 0);
			tmd2771x_set_aiht( client, 0xffff);
			tmd2771x_set_pers(client, 0x33); /* 3 persistence */
			if (data->enable_als_sensor==0) {

				spin_lock_irqsave(&data->wq_lock, flags);

				cancel_delayed_work(&data->als_dwork);
				schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

				spin_unlock_irqrestore(&data->wq_lock, flags);
				tmd2771x_set_enable(client, 0x25);	 /* only enable PS interrupt */
			} else {
				tmd2771x_set_enable(client, 0x27);	 /* only enable PS interrupt */
			}
		}
	}
	else {
			data->enable_ps_sensor = 0;
			if (data->enable_als_sensor) {
				tmd2771x_set_enable(client,0); /* Power Off */
				tmd2771x_set_atime(client, data->als_atime);  /* previous als poll delay */
				tmd2771x_set_ailt( client, 0);
				tmd2771x_set_aiht( client, 0xffff);
				tmd2771x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
				tmd2771x_set_pers(client, 0x33); /* 3 persistence */
				tmd2771x_set_enable(client, 0x3);	 /* only enable light sensor */
				spin_lock_irqsave(&data->wq_lock, flags);

				cancel_delayed_work(&data->als_dwork);
				schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

				spin_unlock_irqrestore(&data->wq_lock, flags);
			}
			else {
				tmd2771x_set_enable(client, 0);
				spin_lock_irqsave(&data->wq_lock, flags);

				cancel_delayed_work(&data->als_dwork);

				spin_unlock_irqrestore(&data->wq_lock, flags);
			}
			if(near_flag) {
			    input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		            input_sync(data->input_dev_ps);
				
			}
			
	}

	mutex_unlock(&data->update_lock);
	return 0;
}


static int tmd2771x_proximity_enable_set(struct sensors_classdev *sensors_cdev,
			   	unsigned int enable)
{
	
	struct tmd2771x_data *data = container_of(sensors_cdev,
				   	struct tmd2771x_data, ps_cdev);

	if ((enable != 0) && (enable != 1)) {
		printk("%s: unvalid value=%d\n", __func__, enable);
	}
	
	return tmd2771x_enable_ps_sensor(data->client, enable);
}

static ssize_t tmd2771x_store_proximity_enable(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = data->client;
	unsigned long val = simple_strtoul(buf, NULL, 10);
	unsigned long flags;
	printk("%s: enable ps senosr ( %ld)\n", __func__, val);

	if ((val != 0) && (val != 1)) {
		printk("%s:store unvalid value=%ld\n", __func__, val);
		return count;
	}
	//some variant/function here need to thread-safe. And it`s better to finish all the steps in one time.
	mutex_lock(&data->update_lock);

	if(val == 1) {
		//turn on p sensor
		if (data->enable_ps_sensor==0) {
		        tmd2771x_power(client, true);
			prox_on = 1;
			data->enable_ps_sensor= 1;
			tmd2771x_set_enable(client,0); /* Power Off */
			tmd2771x_set_atime(client, 0xf6); /* 27.2ms */
			tmd2771x_set_ptime(client, 0xff); /* 2.72ms */
			tmd2771x_set_ppcount(client, 8); /* 8-pulse */
			tmd2771x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
			tmd2771x_set_pilt(client, 0); //modify by zhll for first report value
			tmd2771x_set_piht(client, 0);
		        //tmd2771x_set_pilt(client, 0);		// init threshold for proximity
		        //tmd2771x_set_piht(client, data->pdata->ps_det_thld);
			data->ps_threshold = data->ps_det_thld;
			data->ps_hysteresis_threshold = data->ps_hsyt_thld;
			tmd2771x_set_ailt( client, 0);
			tmd2771x_set_aiht( client, 0xffff);
			tmd2771x_set_pers(client, 0x33); /* 3 persistence */

			if (data->enable_als_sensor==0) {

				/* we need this polling timer routine for sunlight canellation */
				spin_lock_irqsave(&data->wq_lock, flags);

				/*
				 * If work is already scheduled then subsequent schedules will not
				 * change the scheduled time that's why we have to cancel it first.
				 */
				cancel_delayed_work(&data->als_dwork);
				schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

				spin_unlock_irqrestore(&data->wq_lock, flags);
			}
			tmd2771x_set_enable(client, 0x27);	 /* only enable PS interrupt */
		}
	}
	else {
		//turn off p sensor - kk 25 Apr 2011 we can't turn off the entire sensor, the light sensor may be needed by HAL
			data->enable_ps_sensor = 0;
			if (data->enable_als_sensor) {
				// reconfigute light sensor setting
				tmd2771x_set_enable(client,0); /* Power Off */
				tmd2771x_set_atime(client, data->als_atime);  /* previous als poll delay */
				tmd2771x_set_ailt( client, 0);
				tmd2771x_set_aiht( client, 0xffff);
				tmd2771x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
				tmd2771x_set_pers(client, 0x33); /* 3 persistence */
				tmd2771x_set_enable(client, 0x3);	 /* only enable light sensor */
				/*
				 * If work is already scheduled then subsequent schedules will not
				 * change the scheduled time that's why we have to cancel it first.
				 */
				spin_lock_irqsave(&data->wq_lock, flags);

				cancel_delayed_work(&data->als_dwork);
				schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

				spin_unlock_irqrestore(&data->wq_lock, flags);
			}
			else {
				tmd2771x_set_enable(client, 0);
				/*
				 * If work is already scheduled then subsequent schedules will not
				 * change the scheduled time that's why we have to cancel it first.
				 */
				spin_lock_irqsave(&data->wq_lock, flags);

				cancel_delayed_work(&data->als_dwork);

				spin_unlock_irqrestore(&data->wq_lock, flags);
			}
	}

	mutex_unlock(&data->update_lock);

	return count;
}

static struct device_attribute dev_attr_proximity_enable = __ATTR(enable, S_IWUSR | S_IWGRP | S_IRUGO,
				   tmd2771x_show_proximity_enable, tmd2771x_store_proximity_enable);

static ssize_t tmd2771x_show_light_enable(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", data->enable_als_sensor);
}

static ssize_t tmd2771x_store_light_enable(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = data->client;
	unsigned long val = simple_strtoul(buf, NULL, 10);
	unsigned long flags;

	printk("%s: enable als sensor ( %ld)\n", __func__, val);

	if ((val != 0) && (val != 1))
	{
		printk("%s: enable als sensor=%ld\n", __func__, val);
		return count;
	}

	mutex_lock(&data->update_lock);

	if(val == 1) {
		//turn on light  sensor
		if (data->enable_als_sensor==0) {
			data->enable_als_sensor = 1;
			tmd2771x_set_enable(client,0); /* Power Off */
			tmd2771x_set_atime(client, data->als_atime);  /* 100.64ms */
			tmd2771x_set_ailt( client, 0);
			tmd2771x_set_aiht( client, 0xffff);
			tmd2771x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
			tmd2771x_set_pers(client, 0x33); /* 3 persistence */

			if (data->enable_ps_sensor) {
				tmd2771x_set_ptime(client, 0xff); /* 2.72ms */
				tmd2771x_set_ppcount(client, 8); /* 8-pulse */
				tmd2771x_set_enable(client, 0x27);	 /* if prox sensor was activated previously */
			}
			else {
				tmd2771x_set_enable(client, 0x3);	 /* only enable light sensor */
			}
			/*
			 * If work is already scheduled then subsequent schedules will not
			 * change the scheduled time that's why we have to cancel it first.
			 */
			spin_lock_irqsave(&data->wq_lock, flags);

			cancel_delayed_work(&data->als_dwork);
			schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));

			spin_unlock_irqrestore(&data->wq_lock, flags);
		}
	}
	else {
		if (data->enable_als_sensor==1) {
			data->enable_als_sensor = 0;
			if (data->enable_ps_sensor) {
				tmd2771x_set_enable(client,0); /* Power Off */
				tmd2771x_set_atime(client, 0xf6);  /* 27.2ms */
				tmd2771x_set_ptime(client, 0xff); /* 2.72ms */
				tmd2771x_set_ppcount(client, 8); /* 8-pulse */
				tmd2771x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
				tmd2771x_set_piht(client, 0);
				tmd2771x_set_piht(client, data->ps_det_thld);
				tmd2771x_set_ailt( client, 0);
				tmd2771x_set_aiht( client, 0xffff);
				tmd2771x_set_pers(client, 0x33); /* 3 persistence */
				tmd2771x_set_enable(client, 0x27);	 /* only enable prox sensor with interrupt */
			}
			else {
				tmd2771x_set_enable(client, 0);
			}

			/*
			 * If work is already scheduled then subsequent schedules will not
			 * change the scheduled time that's why we have to cancel it first.
			 */
			spin_lock_irqsave(&data->wq_lock, flags);

			cancel_delayed_work(&data->als_dwork);

			spin_unlock_irqrestore(&data->wq_lock, flags);
		}
	}
	mutex_unlock(&data->update_lock);

	return count;
}

static struct device_attribute dev_attr_light_enable = __ATTR(enable, S_IWUSR | S_IWGRP | S_IRUGO,
				   tmd2771x_show_light_enable, tmd2771x_store_light_enable);

static int tmd2771x_enable_als_sensor(struct i2c_client *client, int val) 
{
        struct tmd2771x_data *data = i2c_get_clientdata(client);
        unsigned long flags;
        if ((val != 0) && (val != 1)) {
                printk("%s: unvalid value=%d\n", __func__, val);
        }    
        mutex_lock(&data->update_lock);

        if(val == 1) { 
                //turn on light  sensor
                if (data->enable_als_sensor==0) {
                        data->enable_als_sensor = 1; 
                        tmd2771x_set_enable(client,0); /* Power Off */
                        tmd2771x_set_atime(client, data->als_atime);  /* 100.64ms */
                        tmd2771x_set_ailt( client, 0);
                        tmd2771x_set_aiht( client, 0xffff);
                        tmd2771x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
                        tmd2771x_set_pers(client, 0x33); /* 3 persistence */

                        if (data->enable_ps_sensor) {
                                tmd2771x_set_ptime(client, 0xff); /* 2.72ms */
                                tmd2771x_set_ppcount(client, 8); /* 8-pulse */
                                tmd2771x_set_enable(client, 0x27);       /* if prox sensor was activated previously */
                        }    
                        else {
                                tmd2771x_set_enable(client, 0x3);        /* only enable light sensor */
                        }    
                        spin_lock_irqsave(&data->wq_lock, flags);

                        cancel_delayed_work(&data->als_dwork);
                        schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));

                        spin_unlock_irqrestore(&data->wq_lock, flags);
                }    
        }
	else {
                if (data->enable_als_sensor==1) {
                        data->enable_als_sensor = 0;
                        if (data->enable_ps_sensor) {
                                tmd2771x_set_enable(client,0); /* Power Off */
                                tmd2771x_set_atime(client, 0xf6);  /* 27.2ms */
                                tmd2771x_set_ptime(client, 0xff); /* 2.72ms */
                                tmd2771x_set_ppcount(client, 8); /* 8-pulse */
                                tmd2771x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
                                tmd2771x_set_piht(client, 0);
                                tmd2771x_set_piht(client, data->ps_det_thld);
                                tmd2771x_set_ailt( client, 0);
                                tmd2771x_set_aiht( client, 0xffff);
                                tmd2771x_set_pers(client, 0x33); /* 3 persistence */
                                tmd2771x_set_enable(client, 0x25);       /* only enable prox sensor with interrupt */
                        }
                        else {
                                tmd2771x_set_enable(client, 0);
                        }

                        spin_lock_irqsave(&data->wq_lock, flags);

                        cancel_delayed_work(&data->als_dwork);

                        spin_unlock_irqrestore(&data->wq_lock, flags);
                }
        }
        mutex_unlock(&data->update_lock);
        return 0;
}
 

static ssize_t tmd2771x_als_enable_set(struct sensors_classdev *sensors_cdev, 
                                unsigned int enable)
{
        struct tmd2771x_data *data = container_of(sensors_cdev, 
                                        struct tmd2771x_data, als_cdev);
        printk("%s: enable als sensor ( %d)\n", __func__, enable);
        if ((enable!= 0) && (enable != 1))
        {    
                printk("%s: enable als sensor=%d\n", __func__, enable);
        }    
        return tmd2771x_enable_als_sensor(data->client, enable);
}

static ssize_t tmd2771x_show_als_poll_delay(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", data->als_poll_delay*1000);	// return in micro-second
}

static ssize_t tmd2771x_store_als_poll_delay(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = data->client;
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;
	int poll_delay=0;
//	unsigned long flags;

	if (val<5000)
		val = 5000;	// minimum 5ms

	mutex_lock(&data->update_lock);
	data->als_poll_delay = val/1000;	// convert us => ms

	poll_delay = 256 - (val/2720);	// the minimum is 2.72ms = 2720 us, maximum is 696.32ms

	if(data->enable_ps_sensor){      // mod by zhll for MUS-139
	    tmd2771x_set_atime(client,0xf6);
	}else {
	    if (poll_delay >= 256)
		data->als_atime = 255;
		else if (poll_delay < 0)
			data->als_atime = 0;
		else
			data->als_atime = poll_delay;
		ret = tmd2771x_set_atime(client, data->als_atime);
		if (ret < 0)
			return ret;
	}

	/* we need this polling timer routine for sunlight canellation */
	/*
	 * If work is already scheduled then subsequent schedules will not
	 * change the scheduled time that's why we have to cancel it first.
	 */
//	spin_lock_irqsave(&data->wq_lock, flags);
//	__cancel_delayed_work(&data->als_dwork);
//	schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

//	spin_unlock_irqrestore(&data->wq_lock, flags);

	mutex_unlock(&data->update_lock);

	return count;
}
#if 0
static ssize_t tmd2771x_store_als_poll_delay(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = data->client;
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;
	int poll_delay=0;
	unsigned long flags;

	if (val<5000)
		val = 5000;	// minimum 5ms

	mutex_lock(&data->update_lock);
	data->als_poll_delay = val/1000;	// convert us => ms

	poll_delay = 256 - (val/2720);	// the minimum is 2.72ms = 2720 us, maximum is 696.32ms
	if (poll_delay >= 256)
		data->als_atime = 255;
	else if (poll_delay < 0)
		data->als_atime = 0;
	else
		data->als_atime = poll_delay;
	ret = tmd2771x_set_atime(client, data->als_atime);

	if (ret < 0)
		return ret;

	/* we need this polling timer routine for sunlight canellation */
	/*
	 * If work is already scheduled then subsequent schedules will not
	 * change the scheduled time that's why we have to cancel it first.
	 */
	spin_lock_irqsave(&data->wq_lock, flags);
	cancel_delayed_work(&data->als_dwork);
	schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

	spin_unlock_irqrestore(&data->wq_lock, flags);

	mutex_unlock(&data->update_lock);

	return count;
}
#endif

/*****als sysfs delay  by han.xu**************************************************/
static int als_poll_delay_set(struct sensors_classdev *sensors_cdev,
                unsigned int delay_msec)
{
        struct tmd2771x_data *als_data = container_of(sensors_cdev,
                        struct tmd2771x_data, als_cdev);
	if(als_data==NULL){
	    printk("als_poll_delay_set get tmd2771x_data error\n");
	    return 0;
	}
        mutex_lock(&als_data->update_lock);
	if(delay_msec<5){
	    als_data->als_poll_delay=5;
	}
	else {
	    als_data->als_poll_delay=delay_msec;
	}
        mutex_unlock(&als_data->update_lock);
//	printk("xuhan-----als_poll_delay_set: %d\n",als_data->als_poll_delay);
        return 0;
}


static ssize_t tmd2771x_show_ps_sensor_data(struct device *dev,struct device_attribute *attr, char *buf)
{
	int pdata=0;
		
	pdata = i2c_smbus_read_word_data(data->client, CMD_WORD|TMD2771X_PDATAL_REG);
	return sprintf(buf, "%d\n", pdata);	// read ps-sensor data for debugging
}


static DEVICE_ATTR(ps_sensor_data, S_IWUGO | S_IRUGO,tmd2771x_show_ps_sensor_data,NULL);


static DEVICE_ATTR(poll_delay, S_IWUSR | S_IWGRP | S_IRUGO,
				   tmd2771x_show_als_poll_delay, tmd2771x_store_als_poll_delay);

static struct attribute *tmd2771x_proximity_attributes[] = {
	&dev_attr_proximity_enable.attr,
	&dev_attr_ps_sensor_thld.attr,
	&dev_attr_ps_sensor_data.attr,
	NULL
};

static struct attribute *tmd2771x_light_attributes[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static const struct attribute_group tmd2771x_ps_attr_group = {
	.attrs = tmd2771x_proximity_attributes,
};

static const struct attribute_group tmd2771x_als_attr_group = {
	.attrs = tmd2771x_light_attributes,
};
/*
 * Initialization function
 */
#if 0
static int my_thread(void *param)
{
	struct i2c_client *client = (struct i2c_client *)param;
        int id;
	while(true){
	    id = i2c_smbus_read_byte_data(client, CMD_BYTE|TMD2771X_ID_REG);
	    if (id == 0x20) {
		printk("TMD27711\n");
	    }
	    else if (id == 0x29) {
		printk("TMD27713\n");
	    }
	    /* FEIXUN_SENSOR_WANGQINGCHUAN_000 start + */
	    // Add support for TMD27721 and TMD27723
	    else if (id == 0x30) {
		printk("TMD27721\n");
	    }
	    else if (id == 0x39) {
		printk("TMD27723\n");
	    }
	    usleep(500*1000);
	}
	return 0;
}
#endif
static int tmd2771x_init_client(struct i2c_client *client)
{
	int err;
	int id;
	//  kthread_run(my_thread, client, "my_thread");
	printk("**********%s,%d\n",__func__,__LINE__);
	err = tmd2771x_set_enable(client, 0);
	printk("hexin**********%s,%d,%d\n",__func__,__LINE__,err);

	if (err < 0)
		return err;

	id = i2c_smbus_read_byte_data(client, CMD_BYTE|TMD2771X_ID_REG);
	if (id == 0x20) {
		printk("TMD27711\n");
	}
	else if (id == 0x29) {
		printk("TMD27713\n");
	}
	/* FEIXUN_SENSOR_WANGQINGCHUAN_000 start + */
	// Add support for TMD27721 and TMD27723
	else if (id == 0x30) {
	     printk("TMD27721\n");
	}
	else if (id == 0x39) {
	    printk("TMD27723\n");
	}
	else {
	    printk("Neither TMD2771(1/3) nor TMD2772(1/3): id-0x%x\n", id);
	    return -EIO;
	}
	/* FEIXUN_SENSOR_WANGQINGCHUAN_000 end   - */

	tmd2771x_set_atime(client, 0xDB);	// 100.64ms ALS integration time
	tmd2771x_set_ptime(client, 0xFF);	// 2.72ms Prox integration time
	tmd2771x_set_wtime(client, 0xFF);	// 2.72ms Wait time

	tmd2771x_set_ppcount(client, 0x08);	// 8-Pulse for proximity
	tmd2771x_set_config(client, 0);		// no long wait
	tmd2771x_set_control(client, 0x60);	// 100mA, IR-diode, 1X PGAIN, 1X AGAIN

	tmd2771x_set_pilt(client, 0);		// init threshold for proximity
	tmd2771x_set_piht(client, data->ps_det_thld);

	data->ps_threshold = data->ps_det_thld;
	data->ps_hysteresis_threshold = data->ps_hsyt_thld;

	tmd2771x_set_ailt(client, 0);		// init threshold for als
	tmd2771x_set_aiht(client, 0xFFFF);

	tmd2771x_set_pers(client, 0x22);	// 2 consecutive Interrupt persistence

	// sensor is in disabled mode but all the configurations are preset

	return 0;
}

#define TMD2771X_GPIO_INT 80
struct regulator *vdd_reg = NULL;
struct regulator *vio_reg = NULL;

#if 0
static void tmd2771x_proximity_enable(struct tmd2771x_data *data)
{
	struct i2c_client *client = data->client;
	prox_on = 1;
	data->enable_ps_sensor= 1;
	tmd2771x_set_enable(client,0); /* Power Off */
	tmd2771x_set_atime(client, 0xf6); /* 27.2ms */
	tmd2771x_set_ptime(client, 0xff); /* 2.72ms */
	tmd2771x_set_ppcount(client, 128); /* 8-pulse */
	tmd2771x_set_control(client, 0x2c); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
	tmd2771x_set_pilt(client, 0); //modify by zhll for first report value
	tmd2771x_set_piht(client, 0);
	//tmd2771x_set_pilt(client, 0);		// init threshold for proximity
	//tmd2771x_set_piht(client, data->pdata->ps_det_thld);
	data->ps_threshold = data->ps_det_thld;
	data->ps_hysteresis_threshold = data->ps_hsyt_thld;
	tmd2771x_set_ailt( client, 0);
	tmd2771x_set_aiht( client, 0xffff);
	tmd2771x_set_pers(client, 0x33); /* 3 persistence */

	cancel_delayed_work(&data->als_dwork);
	schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

	tmd2771x_set_enable(client, 0x27);	 /* only enable PS interrupt */
	printk(KERN_ERR "********************tmd2772x_proximity_enable\n");
}

#endif

static int tmd277x_pinctrl_init(void)
{
	int err=0;
        struct i2c_client *client =data->client;

        data->pinctrl = devm_pinctrl_get(&client->dev);
        if (IS_ERR_OR_NULL(data->pinctrl)) {
                dev_err(&client->dev, "Failed to get pinctrl\n");
                return PTR_ERR(data->pinctrl);
        }

        data->pin_default = pinctrl_lookup_state(data->pinctrl,
                        "default");
        if (IS_ERR_OR_NULL(data->pin_default)) {
                dev_err(&client->dev, "Failed to look up default state\n");
                return PTR_ERR(data->pin_default);
        }

        data->pin_sleep = pinctrl_lookup_state(data->pinctrl,
                        "sleep");
        if (IS_ERR_OR_NULL(data->pin_sleep)) {
                dev_err(&client->dev, "Failed to look up sleep state\n");
                return PTR_ERR(data->pin_sleep);
        }

       err = pinctrl_select_state(data->pinctrl, data->pin_default);
       if (err) {
           dev_err(&client->dev, "Can't select pinctrl state\n");
	   return err;
       }


        return 0;
}


static int tmd2771x_parse_dt(struct i2c_client *client, struct tmd2771x_platform_data *pdata)
{
	/* TODO dts */
	int err;
	
	struct device_node *np = client->dev.of_node;
	 pdata->tmd_irq_gpio = of_get_named_gpio_flags(np, "taos,gpio-int1",
					 0, NULL);
	 if(gpio_is_valid(pdata->tmd_irq_gpio)) {	
	     err=gpio_request(pdata->tmd_irq_gpio,"tmd_irq_gpio");
	     if(err) {
	         dev_err(&client->dev,"request irq gpio error\n");
	     }
	 } else {
	    dev_err(&client->dev,"p-sensor request irq gpio error");
	 }
	 
	 gpio_direction_input(pdata->tmd_irq_gpio);

	 pdata->irq=client->irq;
	 if (pdata->irq < 0)
	     return pdata->irq;

	#ifdef CONFIG_PHICOMM_BOARD_E651Lt	
	pdata->ps_det_thld = 750;
	pdata->ps_hsyt_thld = 600;
	#elif defined (CONFIG_PHICOMM_BOARD_C520Lw)
	pdata->ps_det_thld = 15;
	pdata->ps_hsyt_thld = 5;
	#elif defined (CONFIG_PHICOMM_BOARD_E653Lw)
	pdata->ps_det_thld = 110;
	pdata->ps_hsyt_thld = 50;
	#else
	pdata->ps_det_thld = 300;
	pdata->ps_hsyt_thld = 200;
	#endif
	pdata->als_hsyt_thld = 20;

	return 0;
}
static void tmd2771x_get_power(struct i2c_client *client)
{
    vdd_reg = devm_regulator_get(&client->dev, "vdd");
    if(vdd_reg == NULL){
	goto VIO_GET;
    }
    regulator_set_voltage(vdd_reg, 2850000, 2850000);

VIO_GET:
    vio_reg = devm_regulator_get(&client->dev, "vio");
    if(vio_reg == NULL) {
	printk("Error of setting voltage\n");
	return;
    }
    regulator_set_voltage(vio_reg, 1800000, 1800000);
    printk("*******tmd2771x get power ok!\n");
}

static void tmd2771x_put_power(struct i2c_client *client)
{
    if(vdd_reg){
	devm_regulator_put(vdd_reg);
    }
    if(vio_reg){
	devm_regulator_put(vio_reg);
    }
}

static void tmd2771x_power(struct i2c_client *client, bool enable)
{
    int err = 0;
    if(enable && !data->power_enabled) {
	if(vdd_reg){
	    err = regulator_enable(vdd_reg);
	}
	if(vio_reg){
	    err = regulator_enable(vio_reg);
	}
	data->power_enabled = enable;

    } else if(!enable && data->power_enabled){
	if(vdd_reg){
	    regulator_disable(vdd_reg);
	}
	if(vio_reg){
	    regulator_disable(vio_reg);
	}
	data->power_enabled = enable;
     }
}


//gxh add for debug. store printk_buf into /data/panic_log.txt when kernel panic happened.
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>
//#include <linux/timer.h>
//#include <linux/timex.h>
//#include <linux/rtc.h>


static void read_file(char *filename, char *data, int len)
{
  struct file *file;
  int fd;
  loff_t pos = 0;

  mm_segment_t old_fs = get_fs();
  set_fs(KERNEL_DS);

  fd = sys_open(filename, O_RDONLY, 0);
  if (fd >= 0) {
    //sys_write(fd, data, PK_BUF_SIZE);
    file = fget(fd);
    if (file) {
      //vfs_read(file, data, file->f_dentry->d_inode->i_size, &pos);
      vfs_read(file, data, len, &pos);
      fput(file);
    }
    sys_close(fd);
  }
  set_fs(old_fs);
}

static void write_file(char *filename, char *data, int len)
{
	struct file *file = NULL;
  	struct inode *inode = NULL;
  	int fd;
  	loff_t pos = 0;

  	mm_segment_t old_fs = get_fs();
  	set_fs(KERNEL_DS);

	pos = 0;
  	fd = sys_open(filename, O_RDWR|O_CREAT, 0644);
  	if (fd >= 0) {
	   //sys_write(fd, data, PK_BUF_SIZE);
    	   file = fget(fd);
    	   if (file) {
	   	inode = file->f_dentry->d_inode;
  	   	//pos += inode->i_size;
      	   	vfs_write(file, data, len, &pos);
      	   	fput(file);
	     		pos += len;
    	   }
    	   sys_close(fd);
  	}
  	set_fs(old_fs);
}


//void save_panic_buf(char *buf)
//{
//	if(buf == NULL)
//	{
//		//just for test
//		write_file("/data/panic_test.txt", "1234567890", 11);
//		return;
//	}
//	//read_file("/system/build.prop", ver_buf, 0);
//
// 	write_file("/data/panic_log.txt", panic_buf, pb_pos);
//}
//EXPORT_SYMBOL(save_panic_buf);
//gxh add end.

void tmd_cal(void)
{

    struct i2c_client *client=data->client;
    int i;
    
    for(i = 0; i < 10 ; i++)
    {
	mutex_lock(&data->update_lock);
	if(start_cal == CAL_MAX)
	   pdata_temp_max[i] = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_PDATAL_REG);
	else if(start_cal == CAL_MIN)
	   pdata_temp_min[i] = i2c_smbus_read_word_data(client, CMD_WORD|TMD2771X_PDATAL_REG);
    	mutex_unlock(&data->update_lock);

	mdelay(100);
    }

}

struct delayed_work caldata_read_dwork; 

static void read_caldata(struct work_struct *work)
{
    char pdata_temp[5] = {'\0'};
    int thld = 0;
    static int caldata_read_time = 0;

    read_file("/persist/tmd_cal.txt", pdata_temp, 4);

    if(strlen(pdata_temp) > 0)
    {
	sscanf(pdata_temp, "%d", &thld);
	if((thld > 0) && (thld < 1024))
	{
	    data->ps_det_thld = thld + 10; 
	    data->ps_hsyt_thld = thld - 10;
	}
    
	printk("tmd ps thld = %d\n", data->ps_det_thld);
	return;
    }
    caldata_read_time++;//add if we do not calibrate P-sensor
    if(caldata_read_time < 3)
	{
	    schedule_delayed_work(&caldata_read_dwork, HZ);// restart timer
	}
    else
	return;
}


//static void caldata_read_timer(unsigned long _data)
//{
//    schedule_delayed_work(&caldata_read_dwork, HZ);	// restart timer
//}

//void save_tmd_caldata(void)
static void save_tmd_caldata(struct work_struct *work)
{
    char temp[20] = {'\0'};
    int i;
    int pdata_temp = 0;
    int n = 0;

    write_file("/persist/tmd_cal.txt", temp, 20);

    pdata_max = 0;
    pdata_min = 0;

    for(i = 0; i < 10 ; i++)
    {
	pdata_max += pdata_temp_max[i]; 
	pdata_min += pdata_temp_min[i]; 
    }
    
    pdata_max /= 10;
    pdata_min /= 10;

    pdata_temp = pdata_min + (pdata_max - pdata_min) / 3;

    if((pdata_temp > 0) && (pdata_temp < 1024))
    {
	data->ps_det_thld = pdata_temp + 10; 
    	data->ps_hsyt_thld = pdata_temp - 10;
    	
    	n = sprintf(temp, "%d", pdata_temp);

    	write_file("/persist/tmd_cal.txt", temp, n);
    }   
    for(i = 0; i < 10 ; i++)
    {
	pdata_temp_max[i] = 0; 
	pdata_temp_min[i] = 0; 
    }
}

static DECLARE_WORK(caldata_save_dwork, save_tmd_caldata);


static int cal_max_num = 0;
static int cal_min_num = 0;

static s32 tmdcal_write_proc(struct file *filp, const char __user *buff, size_t count, loff_t *ppos)
{
    char flag[64];
    int ret =0;

    ret = copy_from_user(flag, buff, 64);
    if(ret)
    {
	printk("copy_from_user failed.");
    }
    //write_file("/system/panic_test.txt", "1234567890", 11);

    if(!strncmp(flag, "startmax", 8))
    {
	start_cal = CAL_MAX;
	cal_max_num++;
	printk("start cal max\n");
	tmd_cal();
    }
    else if(!strncmp(flag, "startmin", 8))
    {
	start_cal = CAL_MIN;
	cal_min_num++;
	printk("start cal min\n");
	tmd_cal();
    }
    else if(!strncmp(flag, "stop", 4))
    {
	start_cal = 0;
	printk("stop cal\n");
	
	if((cal_max_num >= 1) && (cal_min_num >= 1))
	{
	    cal_min_num = 0;
	    cal_max_num = 0;
	    //save_tmd_caldata();
	    printk("schedule to save caldata\n");
	    schedule_work(&caldata_save_dwork);	// restart timer
	}
    }

    return strlen(flag);
}

static s32 tmdcal_read_proc( struct file *filp, char __user *buff, size_t count, loff_t *ppos )
{
    int ret = 0;
	int data_len = 0;
	u8 buf[32];

    data_len = sprintf(buf, "thld:%d, max:%d, min:%d\n", data->ps_det_thld, pdata_max, pdata_min);
	ret = simple_read_from_buffer(buff, count, ppos,buf, data_len);

    return ret;
}


#include <linux/proc_fs.h>

static const struct file_operations tmdcal_proc_fops = {
	.write      = tmdcal_write_proc,
	.read		= tmdcal_read_proc,
	.owner      = THIS_MODULE,
};
static struct proc_dir_entry *tmdcal_proc_entry;
static struct i2c_driver tmd2771x_driver;
static int  tmd2771x_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	int err = 0;
	struct tmd2771x_platform_data *pdata;
	struct input_dev *input_dev;
	struct input_dev *input_dev_light;

	tmdcal_proc_entry = proc_create("tmd_cal", 0666, NULL,&tmdcal_proc_fops);
	if(tmdcal_proc_entry == NULL){
	
		printk(KERN_INFO "fdv proc file create failed!\n");
	}
	INIT_DELAYED_WORK(&caldata_read_dwork, read_caldata);
       
	schedule_delayed_work(&caldata_read_dwork, 30 * HZ);	// restart timer
	//init_timer(&readcal_timer);
	//
	//readcal_timer.function = caldata_read_timer;
	//readcal_timer.expires = jiffies + HZ * 40 ;
	//add_timer(&readcal_timer);

	
	printk("**************************tmd2772x enter!\n");
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		err = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct tmd2771x_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}

	pdata = kzalloc(sizeof(struct tmd2771x_platform_data), GFP_KERNEL);
        if (!pdata) {
	        pr_err("%s: platform resource is no enough!\n", __func__);
		return -ENODEV;
	}
	tmd2771x_parse_dt(client, pdata);

        data->ps_det_thld = pdata->ps_det_thld;
        data->ps_hsyt_thld = pdata->ps_hsyt_thld;
	data->als_hsyt_thld = pdata->als_hsyt_thld;
	data->pdata = pdata;
 	data->power_enabled=0;	
	data->client = client;
	i2c_set_clientdata(client, data);

	data->enable = 0;	/* default mode is standard */
	data->ps_threshold = 0;
	data->ps_hysteresis_threshold = 0;
	data->ps_detection = 0;	/* default to no detection */
	data->enable_als_sensor = 0;	// default to 0
	data->enable_ps_sensor = 0;	// default to 0
	data->als_poll_delay = 500;	// default to 100ms
	data->als_atime	= 0xdb;			// work in conjuction with als_poll_delay

	printk("enable = %x\n", data->enable);

	mutex_init(&data->update_lock);
	spin_lock_init(&data->wq_lock);

	wake_lock_init(&data->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");

	printk("%s interrupt is hooked\n", __func__);


	err=tmd277x_pinctrl_init();
	if(err) {
	   dev_err(&client->dev,"pinctrl init error");
	}	

	/* proximity */
	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		err = -ENOMEM;
		printk( "proximity input device allocate failed\n");
		goto exit_kfree;
	}

	input_set_drvdata(input_dev, data);
	input_dev->name = "proximity";
	input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);
	data->input_dev_ps = input_dev;

	err = input_register_device(input_dev);
	if (err) {
		err = -ENOMEM;
		printk("Unable to register input device ps: %s\n",
		       input_dev->name);
		goto exit_free_ps_dev;
	}

	err = sysfs_create_group(&input_dev->dev.kobj,
				 &tmd2771x_ps_attr_group);
	if (err) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto exit_unregister_ps_dev;
	}
	/* light */
	input_dev_light = input_allocate_device();
	if (input_dev_light == NULL) {
		err = -ENOMEM;
		printk("light input device allocate failed\n");
		goto exit_remove_ps_sysfs;
	}
	input_set_drvdata(input_dev_light, data);
	input_dev_light->name = "light";
	input_set_capability(input_dev_light, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev_light, ABS_MISC, 0, 10000, 0, 0);
	data->input_dev_als = input_dev_light;

	err = input_register_device(input_dev_light);
	if (err) {
		err = -ENOMEM;
		printk("Unable to register input device als: %s\n",
		       input_dev_light->name);
		goto exit_free_als_dev;
	}
	err = sysfs_create_group(&input_dev_light->dev.kobj,
				 &tmd2771x_als_attr_group);
	if (err) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto exit_unregister_als_dev;
	}
	tmd2771x_get_power(client);
	tmd2771x_power(client,true);
	INIT_DELAYED_WORK(&data->dwork, tmd2771x_work_handler);
	INIT_DELAYED_WORK(&data->als_dwork, tmd2771x_als_polling_work_handler);

	/* Initialize the tmd2771x chip */
	err = tmd2771x_init_client(client);
	if (err)
		goto exit_power;
	err = request_threaded_irq(client->irq, NULL, tmd2771x_interrupt, IRQ_TYPE_EDGE_FALLING | IRQF_ONESHOT,
		TMD2771X_DRV_NAME, (void *)client);
	if (err) {
		printk("%s Could not allocate irq(%d) !\n", __func__, data->pdata->irq);
		goto exit_power;
	}

	device_init_wakeup(&client->dev, 1);
//	tmd2771x_proximity_enable(data);
	data->ps_cdev = sensors_proximity_cdev;
	data->ps_cdev.sensors_enable = tmd2771x_proximity_enable_set;
	data->ps_cdev.sensors_poll_delay = NULL;
	err = sensors_classdev_register(&client->dev, &data->ps_cdev);
	if(err){
		dev_err(&client->dev, "class device create failed: %d\n", err);
		goto exit_power;
	}
	data->als_cdev = sensors_light_cdev;
	data->als_cdev.sensors_enable = tmd2771x_als_enable_set;
	data->als_cdev.sensors_poll_delay = als_poll_delay_set;
	err = sensors_classdev_register(&client->dev, &data->als_cdev);
	if(err){
		dev_err(&client->dev, "class device create failed: %d\n", err);
		goto exit_power;
	}

#ifdef CONFIG_DEVICE_VERSION
        confirm_fdv(DEV_L_P,MANUF_TAOS,MANUF_TAOS_ID|TMD2771x_I2C_ADDR);
#endif
	return 0;

exit_power:
	tmd2771x_power(client, false);
	tmd2771x_put_power(client);
	sysfs_remove_group(&(data->input_dev_als)->dev.kobj,
                 &tmd2771x_als_attr_group);
exit_unregister_als_dev:
	input_unregister_device(data->input_dev_als);
exit_free_als_dev:
	input_free_device(data->input_dev_als);
exit_remove_ps_sysfs:
	sysfs_remove_group(&(data->input_dev_ps)->dev.kobj,
                 &tmd2771x_ps_attr_group);
exit_unregister_ps_dev:
	input_unregister_device(data->input_dev_ps);
exit_free_ps_dev:
	input_free_device(data->input_dev_ps);
exit_kfree:
	wake_lock_destroy(&data->prx_wake_lock);
	 if (gpio_is_valid(data->pdata->tmd_irq_gpio))
                gpio_free(data->pdata->tmd_irq_gpio);
	kfree(data);
exit:
	return err;
}

static int tmd2771x_remove(struct i2c_client *client)
{
	disable_irq(data->pdata->irq);
	device_init_wakeup(&client->dev, 0);
	cancel_delayed_work_sync(&data->als_dwork);
	/* Power down the device */
	tmd2771x_set_enable(client, 0);

	sysfs_remove_group(&(data->input_dev_als)->dev.kobj,
                 &tmd2771x_als_attr_group);
	sysfs_remove_group(&(data->input_dev_ps)->dev.kobj,
                 &tmd2771x_ps_attr_group);

	input_unregister_device(data->input_dev_als);
	input_unregister_device(data->input_dev_ps);

	input_free_device(data->input_dev_als);
	input_free_device(data->input_dev_ps);

	free_irq(data->pdata->irq, client);

	wake_lock_destroy(&data->prx_wake_lock);
	kfree(data);

	return 0;
}
#if 1
//#ifdef CONFIG_PM

static int tmd2771x_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int enable = 0;
#ifdef CONFIG_SUSPEND_DEBUG
	printk("------%s-----:i2c_client:%p\n",__func__,client);
	printk("------%s-----:data = %p\n",__func__,data);
	printk("------%s-----:pdata = %p\n",__func__,data->pdata);
#endif
	gpio_direction_output(data->pdata->tmd_irq_gpio, 0);
	disable_irq(data->pdata->irq);

	if (data->enable_als_sensor) {
	cancel_delayed_work_sync(&data->als_dwork);
	}

	if (data->enable_ps_sensor) {
		enable |= 0x25;
		enable_irq_wake(data->pdata->irq);
	}

	if (enable != data->enable)
	tmd2771x_set_enable(client, enable);
	tmd2771x_power(client,false);
	return 0;
}

static int tmd2771x_resume(struct i2c_client *client)
{
	int enable = 0;
#ifdef CONFIG_SUSPEND_DEBUG
	printk("----%s----:i2c_client:%p\n",__func__,client);
	printk("----%s----:data = %p\n",__func__,data);
	printk("----%s----:pdata = %p\n",__func__,data->pdata);
#endif
	tmd2771x_power(client,true);
	msleep(50);

	if (data->enable_als_sensor)
		enable |= 0x03;

	if (data->enable_ps_sensor) {
		enable |= 0x25;
		disable_irq_wake(data->pdata->irq);
	}

//	if (enable != 0x27)
	tmd2771x_set_enable(client, enable);
//added by han.xu 2014-10-11
	if(data->enable_als_sensor) {
	    schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));
	}
	enable_irq(data->pdata->irq);
	gpio_direction_input(data->pdata->tmd_irq_gpio);

	return 0;
}

#else

#define tmd2771x_suspend	NULL
#define tmd2771x_resume		NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id tmd2771x_id[] = {
	{ "tmd2771x", 0 },
	{ }
};
 static struct of_device_id tmd2771x_match_table[] = {
             { .compatible = "TMD2772X,tmd2772x",},
             { },
     };
MODULE_DEVICE_TABLE(i2c, tmd2771x_id);

static struct i2c_driver tmd2771x_driver = {
	.driver = {
		.name	= TMD2771X_DRV_NAME,
		.owner	= THIS_MODULE,
	        .of_match_table = tmd2771x_match_table,
	},
	.suspend = tmd2771x_suspend,
	.resume	= tmd2771x_resume,
	.probe	= tmd2771x_probe,
	.remove	= tmd2771x_remove,
	.id_table = tmd2771x_id,
};

static int __init tmd2771x_init(void)
{
#ifdef CONFIG_DEVICE_VERSION
        register_fdv_with_desc(DEV_L_P,MANUF_TAOS,MANUF_TAOS_ID|TMD2771x_I2C_ADDR,DESC);
#endif
	return i2c_add_driver(&tmd2771x_driver);
}

static void __exit tmd2771x_exit(void)
{
	i2c_del_driver(&tmd2771x_driver);
}

MODULE_DESCRIPTION("TAOS tmd2771x ps and als sensor");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);

module_init(tmd2771x_init);
module_exit(tmd2771x_exit);

