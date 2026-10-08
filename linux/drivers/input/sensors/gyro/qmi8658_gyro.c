/* drivers/input/sensors/access/qmi8658_gyro.c
 *
 * Copyright (C) 2012-2015 ROCKCHIP.
 * Author: ouenhui <oeh@rock-chips.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/sensor-dev.h>
#include <linux/qmi8658.h>

#define GYRO_IOCTL_MAGIC	'g'
#define ECS_GYRO_IOCTL_G _IOR(GYRO_IOCTL_MAGIC, 0x01, struct sensor_axis)

static struct i2c_client *this_client;
static struct miscdevice gyro_dev_device;
static int s_enable = 0;

static int qmi8658_set_rate(struct i2c_client *client, int rate)
{
	const short hz[] = {448, 224, 112, 56, 28};
	const int   d[] = {0x24, 0x25,0x26, 0x27, 0x28};
	int i =0, data =0, result =0;

	/* always use poll mode, no need to set rate */
	return 0;

	if ((rate < 1) || (rate > 480))
		return -1;

	while ((rate > hz[i]) && (i < 5))
		i++;
	data = d[i];

	result = sensor_write_reg(client, QMI8658_REG_CTRL2, data);
	if (result)
		return -1;

	return 0;
}

static int sensor_active(struct i2c_client *client, int enable, int rate)
{
//	struct sensor_private_data *sensor =
//	    (struct sensor_private_data *) i2c_get_clientdata(client);
	int result = 0;
	u8 databuf[2] = {0};

	s_enable = enable;
	databuf[0] = sensor_read_reg(client, QMI8658_REG_CTRL7);

	if (!enable) {
		databuf[0] = (databuf[0] & 0xFD) | 0x80; //accel disable;		
	} else {
		databuf[0] = (databuf[0] & 0xFD) | 0x82; //accel enable;		
		qmi8658_set_rate(client, rate);
	}

	result = sensor_write_reg(client, QMI8658_REG_CTRL7, databuf[0]);
	if (result) {
		dev_err(&client->dev, "%s:fail to set pwrm1\n", __func__);
		return -1;
	}
	
	dev_err(&client->dev, " %s:activer OK, enable=%d, result=%d\n", __func__, enable, result);

	return result;
}

static int sensor_init(struct i2c_client *client)
{
	int ret;
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);
	this_client = client;
	/* init on qmi8658_acc.c */
	ret = sensor->ops->active(client, 0, sensor->pdata->poll_delay_ms);
	if (ret) {
		dev_err(&client->dev, "%s:line=%d,error\n", __func__, __LINE__);
		return ret;
	}

	ret = misc_register(&gyro_dev_device);
	if (ret < 0) {
		dev_err(&client->dev,
			"%s:fail to register misc device %s(%d)\n",
			__func__, gyro_dev_device.name, ret);
		ret = -1;
	}

	return ret;
}

static int gyro_report_value(struct i2c_client *client, struct sensor_axis *axis)
{
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);

	if (sensor->status_cur == SENSOR_ON) {
		/* Report gyro sensor information */
		input_report_rel(sensor->input_dev, ABS_RX, axis->x);
		input_report_rel(sensor->input_dev, ABS_RY, axis->y);
		input_report_rel(sensor->input_dev, ABS_RZ, axis->z);
		input_sync(sensor->input_dev);
	}

	return 0;
}

static int sensor_report_value(struct i2c_client *client)
{
	struct sensor_private_data *sensor =
		(struct sensor_private_data *) i2c_get_clientdata(client);
	struct sensor_platform_data *pdata = sensor->pdata;
	int ret = 0;
	short x, y, z;
	struct sensor_axis axis;
	u8 buffer[6] = {0};
	char value = 0,count = 0;

	if (sensor->ops->read_len < 6) {
		dev_err(&client->dev, "%s:lenth is error,len=%d\n", __func__, sensor->ops->read_len);
		return -1;
	}

	memset(buffer, 0, 6);

	/* Data bytes from hardware xL, xH, yL, yH, zL, zH */
	while(((value & 0x03)!= 0x03)&&(count++ < 10)){
		value = sensor_read_reg(client, QMI8658_REG_STATUSINT);
	}
	*buffer = QMI8658_REG_ACC_XOUT_LSB;
	ret = sensor_rx_data(client, buffer, sensor->ops->read_len);
	*buffer = sensor->ops->read_reg;
	ret |= sensor_rx_data(client, buffer, sensor->ops->read_len);
	if (ret < 0)
		return ret;

	x = ((buffer[1] << 8) & 0xFF00) + (buffer[0] & 0xFF);
	y = ((buffer[3] << 8) & 0xFF00) + (buffer[2] & 0xFF);
	z = ((buffer[5] << 8) & 0xFF00) + (buffer[4] & 0xFF);

	axis.x = (pdata->orientation[0]) * x + (pdata->orientation[1]) * y + (pdata->orientation[2]) * z;
	axis.y = (pdata->orientation[3]) * x + (pdata->orientation[4]) * y + (pdata->orientation[5]) * z;
	axis.z = (pdata->orientation[6]) * x + (pdata->orientation[7]) * y + (pdata->orientation[8]) * z;

	gyro_report_value(client, &axis);

	mutex_lock(&(sensor->data_mutex));
	sensor->axis = axis;
	mutex_unlock(&(sensor->data_mutex));

	if ((sensor->pdata->irq_enable) && (sensor->ops->int_status_reg >= 0))
		value = sensor_read_reg(client, sensor->ops->int_status_reg);

	return ret;
}

static int gyro_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int gyro_dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long gyro_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct sensor_private_data *sensor =
		(struct sensor_private_data *) i2c_get_clientdata(this_client);
	struct i2c_client *client = this_client;
	void __user *argp = (void __user *)arg;
	struct sensor_platform_data *pdata = sensor->pdata;
	int ret = 0;
	short x, y, z;
	struct sensor_axis axis;
	u8 buffer[6] = {0};
	char value = 0,count = 0;

	if (sensor->ops->read_len < 6) {
		dev_err(&client->dev, "%s:lenth is error,len=%d\n", __func__, sensor->ops->read_len);
		return -1;
	}

	if(!s_enable) {
		sensor_active(this_client, 1, 0);
	}
	memset(buffer, 0, 6);

	/* Data bytes from hardware xL, xH, yL, yH, zL, zH */
	while(((value & 0x03)!= 0x03) && (count++ < 10)){
		value = sensor_read_reg(client, QMI8658_REG_STATUSINT);
	}
	*buffer = QMI8658_REG_ACC_XOUT_LSB;
	ret = sensor_rx_data(client, buffer, sensor->ops->read_len);
	*buffer = sensor->ops->read_reg;
	ret |= sensor_rx_data(client, buffer, sensor->ops->read_len);
	if (ret < 0)
		return ret;

	x = ((buffer[1] << 8) & 0xFF00) + (buffer[0] & 0xFF);
	y = ((buffer[3] << 8) & 0xFF00) + (buffer[2] & 0xFF);
	z = ((buffer[5] << 8) & 0xFF00) + (buffer[4] & 0xFF);

	axis.x = (pdata->orientation[0]) * x + (pdata->orientation[1]) * y + (pdata->orientation[2]) * z;
	axis.y = (pdata->orientation[3]) * x + (pdata->orientation[4]) * y + (pdata->orientation[5]) * z;
	axis.z = (pdata->orientation[6]) * x + (pdata->orientation[7]) * y + (pdata->orientation[8]) * z;
	switch(cmd){
		case ECS_GYRO_IOCTL_G:
			mutex_lock(&sensor->data_mutex);
			if (copy_to_user(argp, &axis, sizeof(axis))){
				mutex_unlock(&sensor->data_mutex);
				return -EFAULT;
			}
			mutex_unlock(&sensor->data_mutex);
		break;
	}

	return ret;
}

static const struct file_operations gyro_dev_fops = {
	.owner = THIS_MODULE,
	.open = gyro_dev_open,
	.release = gyro_dev_release,
	.unlocked_ioctl = gyro_dev_ioctl,
};

static struct miscdevice gyro_dev_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "gyro_dev",
	.fops = &gyro_dev_fops,
};

struct sensor_operate gyro_qmi8658_ops = {
	.name				= "qmi8658_gyro",
	.type				= SENSOR_TYPE_GYROSCOPE,
	.id_i2c				= GYRO_ID_QMI8658,
	.read_reg			= QMI8658_REG_GYR_XOUT_LSB,
	.read_len			= 6,
	.id_reg				= QMI8658_REG_WHO_AM_I,
	.id_data 			= SENSOR_CHIP_ID_QMI8658,
	.precision			= QMI8658_PRECISION,
	.ctrl_reg 			= QMI8658_REG_CTRL7,
	.int_status_reg 	= QMI8658_REG_STATUS0,
	.range				= {-32768, 32768},
	.trig				= IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
	.active				= sensor_active,
	.init				= sensor_init,
	.report 			= sensor_report_value,
};

/****************operate according to sensor chip:end************/
static int gyro_qmi8658_probe(struct i2c_client *client,
				 const struct i2c_device_id *devid)
{
	return sensor_register_device(client, NULL, devid, &gyro_qmi8658_ops);
}

static int gyro_qmi8658_remove(struct i2c_client *client)
{
	return sensor_unregister_device(client, NULL, &gyro_qmi8658_ops);
}

static const struct i2c_device_id gyro_qmi8658_id[] = {
	{"qmi8658_gyro", GYRO_ID_QMI8658},
	{}
};

static struct i2c_driver gyro_qmi8658_driver = {
	.probe = gyro_qmi8658_probe,
	.remove = gyro_qmi8658_remove,
	.shutdown = sensor_shutdown,
	.id_table = gyro_qmi8658_id,
	.driver = {
		.name = "gyro_qmi8658",
	#ifdef CONFIG_PM
		.pm = &sensor_pm_ops,
	#endif
	},
};

static int __init gyro_qmi8658_init(void)
{
	return i2c_add_driver(&gyro_qmi8658_driver);
}

static void __exit gyro_qmi8658_exit(void)
{
	i2c_del_driver(&gyro_qmi8658_driver);
}

/* must register after qmi8658_acc */
device_initcall_sync(gyro_qmi8658_init);
module_exit(gyro_qmi8658_exit);

MODULE_AUTHOR("ouenhui <oeh@rock-chips.com");
MODULE_DESCRIPTION("qmi8658_gyro 3-Axis Gyroscope driver");
MODULE_LICENSE("GPL");
