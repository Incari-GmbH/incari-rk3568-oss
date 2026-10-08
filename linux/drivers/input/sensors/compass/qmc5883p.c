/* drivers/input/sensors/compass/qmc5883p.c
 *
 * Copyright (C) 2024-2025 Xtreaner.
 * Author: weihq <weihq@xtreamer.net.cn>
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
#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/of_gpio.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif
#include <linux/sensor-dev.h>

#define COMPASS_IOCTL_MAGIC		'c'

/* IOCTLs for AKM library */
#define ECS_IOCTL_WRITE			    _IOW(COMPASS_IOCTL_MAGIC, 0x01, char*)
#define ECS_IOCTL_READ			    _IOWR(COMPASS_IOCTL_MAGIC, 0x02, char*)
#define ECS_IOCTL_RESET			    _IO(COMPASS_IOCTL_MAGIC, 0x03) /* NOT used in AK8975 */
#define ECS_IOCTL_SET_MODE		    _IOW(COMPASS_IOCTL_MAGIC, 0x04, short)
#define ECS_IOCTL_GETDATA			_IOR(COMPASS_IOCTL_MAGIC, 0x05, char[6])
#define ECS_IOCTL_GET_OPEN_STATUS	_IOR(COMPASS_IOCTL_MAGIC, 0x07, int)
#define ECS_IOCTL_GET_CLOSE_STATUS	_IOR(COMPASS_IOCTL_MAGIC, 0x08, int)
#define ECS_IOCTL_GET_LAYOUT		_IOR(COMPASS_IOCTL_MAGIC, 0x09, char)
#define ECS_IOCTL_GET_ACCEL			_IOR(COMPASS_IOCTL_MAGIC, 0x0A, short[3])
#define ECS_IOCTL_GET_OUTBIT		_IOR(COMPASS_IOCTL_MAGIC, 0x0B, char)
#define ECS_IOCTL_GET_INFO			_IOR(COMPASS_IOCTL_MAGIC, 0x27, unsigned char[AK09911_INFO_SIZE])
#define ECS_IOCTL_GET_CONF			_IOR(COMPASS_IOCTL_MAGIC, 0x28, unsigned char[AK09911_CONF_SIZE])
#define ECS_IOCTL_GET_PLATFORM_DATA	_IOR(COMPASS_IOCTL_MAGIC, 0x0E, struct akm_platform_data)
#define ECS_IOCTL_GET_DELAY			_IOR(COMPASS_IOCTL_MAGIC, 0x30, short)
#define ECS_COMPASS_IOCTL_G			_IOR(COMPASS_IOCTL_MAGIC, 0x0C, struct sensor_axis)

#define YPR_DATA_SIZE			12
#define RWBUF_SIZE			16
#define SENSOR_DATA_SIZE            (6)
#define QMC5883P_DEVICE_ID           (0x80)
#define QMC5883P_REG_CHIPID         (0x00)
#define QMC5883P_REG_DATAOUT_X_L    (0x01)
#define QMC5883P_REG_DATAOUT_X_M    (0x02)
#define QMC5883P_REG_DATAOUT_Y_L    (0x03)
#define QMC5883P_REG_DATAOUT_Y_M    (0x04)
#define QMC5883P_REG_DATAOUT_Z_L    (0x05)
#define QMC5883P_REG_DATAOUT_Z_M    (0x06)

#define QMC5883P_REG_STATUS         (0x09)
#define QMC5883P_REG_CTRL           (0x0A)
#define QMC5883P_REG_RESET          (0x0B)
#define QMC5883P_REG_AXIS_DEFINE    (0x29)

#define MAG_DATA_FLAG		(1)
#define MAG_DATA_READY		(1 << (MAG_DATA_FLAG))

enum {
	AXIS_X = 0,
	AXIS_Y = 1,
	AXIS_Z = 2,
	AXIS_TOTAL
};

typedef struct{
	signed char sign[3];
	unsigned char map[3];
} QMC5883P_map;

static QMC5883P_map c_map;

static struct i2c_client *this_client;
static struct miscdevice compass_device;
static int s_enable = 0;

static int __attribute__((unused)) g_qmc_rbuf_ready = 0;
static int __attribute__((unused)) g_qmc_rbuf[12] = {0};

static int sensor_active(struct i2c_client *client, int enable, int rate)
{
	int result = 0;
	s_enable = enable;
    result |= sensor_write_reg(client, QMC5883P_REG_CTRL, 0x00);
	if (enable) {
        mdelay(1);
        result |= sensor_write_reg(client, 0x0d, 0x40);
        result |= sensor_write_reg(client, QMC5883P_REG_AXIS_DEFINE, 0x06);
        result |= sensor_write_reg(client, QMC5883P_REG_RESET, 0x00);//30 GS
        result |= sensor_write_reg(client, QMC5883P_REG_CTRL, 0xc3);
        mdelay(1);
    }

	if (result)
		printk("%s:fail to active sensor\n", __func__);

	return result;
}

static int confirm_chip_id(struct i2c_client *client)
{
	unsigned short chip_id = 0;
	if((chip_id = sensor_read_reg(client, QMC5883P_REG_CHIPID)) == 0x80){	
		printk("qmc5883p read chip id OK slave:0x%x id:0x%x\n", QMC5883P_REG_CHIPID, chip_id);
		return 0;
    }

    return -1;
}

static int sensor_init(struct i2c_client *client)
{
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *)i2c_get_clientdata(client);
	int result = 0;
	this_client = client;
    if(confirm_chip_id(client)) {
        return -1;
    }

	result = sensor->ops->active(client, 1, 0);
	if (result) {
		pr_err("%s:line=%d,error\n", __func__, __LINE__);
		return result;
	}

	sensor->status_cur = SENSOR_OFF;
	result = misc_register(&compass_device);
	if (result < 0) {
		pr_err("%s:fail to register misc device %s\n", __func__, compass_device.name);
		result = -1;
	}
	sensor->status_cur = SENSOR_ON;
	return result;
}

static int gsensor_report_value(struct i2c_client *client, struct sensor_axis *axis)
{
	struct sensor_private_data *sensor =
	    (struct sensor_private_data *) i2c_get_clientdata(client);

	if (sensor->status_cur == SENSOR_ON) {
		/* Report acceleration sensor information */
		input_report_abs(sensor->input_dev, ABS_X, axis->x);
		input_report_abs(sensor->input_dev, ABS_Y, axis->y);
		input_report_abs(sensor->input_dev, ABS_Z, axis->z);
		input_sync(sensor->input_dev);
	}

	return 0;
}

static int sensor_report_value(struct i2c_client *client)
{
	struct sensor_private_data *sensor = (struct sensor_private_data *) i2c_get_clientdata(client);
	//struct sensor_platform_data *pdata = sensor->pdata;
	int ret = 0;
	struct sensor_axis axis;
	u8 buffer[6] = {0};
	char value = 0, count = 0;

	if (sensor->ops->read_len < 6) {
		dev_err(&client->dev, "%s:lenth is error,len=%d\n", __func__, sensor->ops->read_len);
		return -1;
	}

	memset(buffer, 0, 6);

	/* Data bytes from hardware xL, xH, yL, yH, zL, zH */
	while(((value & 0x01) != 0x01) &&(count ++ < 10)){
		value = sensor_read_reg(client, QMC5883P_REG_STATUS);
	}

	*buffer = sensor->ops->read_reg;
	ret = sensor_rx_data(client, buffer, sensor->ops->read_len);
	if (ret < 0)
		return ret;

	axis.x = (short)(((buffer[1]) << 8) | buffer[0]);
	axis.y = (short)(((buffer[3]) << 8) | buffer[2]);
	axis.z = (short)(((buffer[5]) << 8) | buffer[4]);

	gsensor_report_value(client, &axis);

	mutex_lock(&(sensor->data_mutex));
	sensor->axis = axis;
	mutex_unlock(&(sensor->data_mutex));

	if ((sensor->pdata->irq_enable) && (sensor->ops->int_status_reg >= 0))
		value = sensor_read_reg(client, sensor->ops->int_status_reg);

	return ret;
}

static void __attribute__((unused)) compass_set_YPR(int *rbuf)
{
	/* No events are reported */
	if (!rbuf[0]) {
		pr_err("%s:Don't waste a time.", __func__);
		return;
	}

	g_qmc_rbuf_ready = 1;
	memcpy(&g_qmc_rbuf[0], rbuf, 12 * sizeof(int));
}

static int compass_dev_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int compass_dev_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int __attribute__((unused)) compass_qmc_set_mode(struct i2c_client *client, char mode)
{
	return 0;
}

static void __attribute__((unused)) qmc5883p_set_layout(int layout)
{
	c_map.map[AXIS_X] = AXIS_X;
	c_map.map[AXIS_Y] = AXIS_Y;
	c_map.map[AXIS_Z] = AXIS_Z;
	switch (layout) {
		case 0:
			c_map.sign[AXIS_X] = 1;
			c_map.sign[AXIS_Y] = 1;
			c_map.sign[AXIS_Z] = 1;
			break;
		case 1:
			c_map.sign[AXIS_X] = -1;
			c_map.sign[AXIS_Y] = 1;
			c_map.sign[AXIS_Z] = 1;
			break;
		case 2:
			c_map.sign[AXIS_X] = -1;
			c_map.sign[AXIS_Y] = -1;
			c_map.sign[AXIS_Z] = 1;
			break;
		case 3:
			c_map.sign[AXIS_X] = -1;
			c_map.sign[AXIS_Y] = 1;
			c_map.sign[AXIS_Z] = -1;
			break;
		case 4:
			c_map.sign[AXIS_X] = -1;
			c_map.sign[AXIS_Y] = 1;
			c_map.sign[AXIS_Z] = -1;
			break;
		case 5:
			c_map.sign[AXIS_X] = 1;
			c_map.sign[AXIS_Y] = 1;
			c_map.sign[AXIS_Z] = -1;
			break;
		case 6:
			c_map.sign[AXIS_X] = 1;
			c_map.sign[AXIS_Y] = -1;
			c_map.sign[AXIS_Z] = -1;
			break;
		case 7:
			c_map.sign[AXIS_X] = -1;
			c_map.sign[AXIS_Y] = -1;
			c_map.sign[AXIS_Z] = -1;
			break;
		default:
			c_map.sign[AXIS_X] = 1;
			c_map.sign[AXIS_Y] = 1;
			c_map.sign[AXIS_Z] = 1;
			break;
	}
}

static int compass_qmc_get_openstatus(void)
{
	struct sensor_private_data *sensor = (struct sensor_private_data *)i2c_get_clientdata(this_client);
	wait_event_interruptible(sensor->flags.open_wq, (atomic_read(&sensor->flags.open_flag) != 0));
	return atomic_read(&sensor->flags.open_flag);
}

static int compass_qmc_get_closestatus(void)
{
	struct sensor_private_data *sensor = (struct sensor_private_data *)i2c_get_clientdata(this_client);
	wait_event_interruptible(sensor->flags.open_wq, (atomic_read(&sensor->flags.open_flag) <= 0));
	return atomic_read(&sensor->flags.open_flag);
}

/* ioctl - I/O control */
static long compass_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	struct sensor_private_data *sensor = (struct sensor_private_data *)i2c_get_clientdata(this_client);
	struct i2c_client *client = this_client;
	void __user *argp = (void __user *)arg;
	int result = 0;
	struct akm_platform_data compass;
	struct sensor_axis axis;
	/* NOTE: In this function the size of "char" should be 1-byte. */
	//char compass_data[SENSOR_DATA_SIZE];	/* for GETDATA */
	char rwbuf[RWBUF_SIZE];	/* for READ/WRITE */
	char datebuf[6];	/* for READ/WRITE */
	char mode;				/* for SET_MODE*/
	//int value[YPR_DATA_SIZE];		/* for SET_YPR */
	int status;				/* for OPEN/CLOSE_STATUS */
	int ret = -1;				/* Return value. */
	char layout;		/* for GET_LAYOUT */
	unsigned char buffer[6] = {0};
	char _value = 0, count = 0;

	switch (cmd) {
		case ECS_IOCTL_WRITE:
		case ECS_IOCTL_READ:
			if (!argp)
				return -EINVAL;
			if (copy_from_user(&rwbuf, argp, sizeof(rwbuf)))
				return -EFAULT;
			break;
		case ECS_IOCTL_SET_MODE:
			if (!argp)
				return -EINVAL;
			if (copy_from_user(&mode, argp, sizeof(mode)))
				return -EFAULT;
			break;
		case ECS_IOCTL_GETDATA:
		case ECS_IOCTL_GET_OPEN_STATUS:
		case ECS_IOCTL_GET_CLOSE_STATUS:
		case ECS_IOCTL_GET_LAYOUT:
		case ECS_IOCTL_GET_OUTBIT:
			/* Just check buffer pointer */
			if (!argp) {
				dev_err(&client->dev,
					"%s: invalid argument\n", __func__);
				return -EINVAL;
			}
			break;
		default:
			break;
	}

	switch (cmd) {
		case ECS_IOCTL_WRITE:
			mutex_lock(&sensor->operation_mutex);
			if ((rwbuf[0] < 2) || (rwbuf[0] > (RWBUF_SIZE - 1))) {
				mutex_unlock(&sensor->operation_mutex);
				return -EINVAL;
			}
			ret = sensor_tx_data(client, &rwbuf[1], rwbuf[0]);
			if (ret < 0) {
				mutex_unlock(&sensor->operation_mutex);
				dev_err(&client->dev,
					"%s: line = %d, fail to tx data(%d)\n",
					__func__, __LINE__, ret);
				return ret;
			}
			mutex_unlock(&sensor->operation_mutex);
			break;
		case ECS_IOCTL_READ:
			mutex_lock(&sensor->operation_mutex);
			if ((rwbuf[0] < 1) || (rwbuf[0] > (RWBUF_SIZE - 1))) {
				mutex_unlock(&sensor->operation_mutex);
				dev_err(&client->dev,
					"%s: data is error\n", __func__);
				return -EINVAL;
			}

			ret = sensor_rx_data(client, &rwbuf[1], rwbuf[0]);
			if (ret < 0) {
				mutex_unlock(&sensor->operation_mutex);
				dev_err(&client->dev,
					"%s: line = %d, fail to rx data(%d)\n",
					__func__, __LINE__, ret);
				return ret;
			}
			mutex_unlock(&sensor->operation_mutex);
			break;
		case ECS_IOCTL_SET_MODE:
			break;
		case ECS_IOCTL_GETDATA:
			mutex_lock(&sensor->operation_mutex);
			datebuf[0] = 0x01;
			ret = sensor_rx_data(client, &datebuf[0], 6);
			if (ret < 0) {
				mutex_unlock(&sensor->operation_mutex);
				dev_err(&client->dev,
					"%s: line = %d, fail to rx data(%d)\n",
					__func__, __LINE__, ret);
				return ret;
			}
			mutex_unlock(&sensor->operation_mutex);
			break;
		case ECS_IOCTL_GET_OPEN_STATUS:
			status = compass_qmc_get_openstatus();
			break;
		case ECS_IOCTL_GET_CLOSE_STATUS:
			status = compass_qmc_get_closestatus();
			break;
		case ECS_IOCTL_GET_DELAY:
			break;

		case ECS_IOCTL_GET_PLATFORM_DATA:
			ret = copy_to_user(argp, &compass, sizeof(compass));
			if (ret < 0) {
				dev_err(&client->dev, "%s: line = %d, ret = %d\n",
					__func__, __LINE__, ret);
				return ret;
			}
			break;
		case ECS_IOCTL_GET_LAYOUT:
		case ECS_IOCTL_RESET:
			break;
		case ECS_COMPASS_IOCTL_G:
			if (sensor->ops->read_len < 6) {
				dev_err(&client->dev, "%s:lenth is error,len=%d\n", __func__, sensor->ops->read_len);
				return -1;
			}

			if(!s_enable) {
				sensor_active(this_client, 1, 0);
			}
			memset(buffer, 0, 6);
			/* Data bytes from hardware xL, xH, yL, yH, zL, zH */
			while(((_value & 0x01) != 0x01) && (count ++ < 10)){
				_value = sensor_read_reg(client, QMC5883P_REG_STATUS);
			}

			*buffer = sensor->ops->read_reg;
			ret = sensor_rx_data(client, buffer, sensor->ops->read_len);
			if (ret < 0)
				return ret;

			axis.x = (short)(((buffer[1]) << 8 & 0xFF00) | buffer[0]);
			axis.y = (short)(((buffer[3]) << 8 & 0xFF00) | buffer[2]);
			axis.z = (short)(((buffer[5]) << 8 & 0xFF00) | buffer[4]);
			break;
		default:
			return -ENOTTY;
	}

	switch (cmd) {
		case ECS_IOCTL_READ:
			if (copy_to_user(argp, &rwbuf, rwbuf[0] + 1))
				return -EFAULT;
			break;
		case ECS_IOCTL_GETDATA:
			if (copy_to_user(argp, &datebuf, sizeof(datebuf)))
				return -EFAULT;
			break;
		case ECS_IOCTL_GET_OPEN_STATUS:
		case ECS_IOCTL_GET_CLOSE_STATUS:
			if (copy_to_user(argp, &status, sizeof(status)))
				return -EFAULT;
			break;
		case ECS_IOCTL_GET_LAYOUT:
			if (copy_to_user(argp, &layout, sizeof(layout))) {
				dev_err(&client->dev, "%s: line = %d, ret = %d\n",
					__func__, __LINE__, ret);
				return -EFAULT;
			}
			break;
		case ECS_COMPASS_IOCTL_G:
			if (copy_to_user(argp, &axis, sizeof(axis))){
				return -EFAULT;
			}
			mutex_unlock(&sensor->data_mutex);
		break;
		default:
			break;
	}

	return result;
}

static const struct file_operations compass_dev_fops = {
	.owner = THIS_MODULE,
	.open = compass_dev_open,
	.release = compass_dev_release,
	.unlocked_ioctl = compass_dev_ioctl,
};

static struct miscdevice compass_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mag_dev",
	.fops = &compass_dev_fops,
};

struct sensor_operate compass_qmc5883p_ops = {
	.name				= "qmc5883p",
	.type				= SENSOR_TYPE_COMPASS,
	.id_i2c				= COMPASS_ID_QMC5883P,
	.read_reg			= QMC5883P_REG_DATAOUT_X_L,
	.read_len			= SENSOR_DATA_SIZE,
	.id_reg				= QMC5883P_REG_CHIPID,
	.id_data			= QMC5883P_DEVICE_ID,
	.precision			= 8,
	.ctrl_reg			= QMC5883P_REG_CTRL,
	.int_status_reg		= SENSOR_UNKNOW_DATA,
	.range				= {-0xffff, 0xffff},
	.trig				= IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
	.active				= sensor_active,
	.init				= sensor_init,
	.report				= sensor_report_value,
	.misc_dev			= NULL,
};

/****************operate according to sensor chip:end************/
static int compass_qmc5883p_probe(struct i2c_client *client,
				  const struct i2c_device_id *devid)
{
	return sensor_register_device(client, NULL, devid, &compass_qmc5883p_ops);
}

static int compass_qmc5883p_remove(struct i2c_client *client)
{
	return sensor_unregister_device(client, NULL, &compass_qmc5883p_ops);
}

static const struct i2c_device_id compass_qmc5883p_id[] = {
	{"qmc5883p", COMPASS_ID_QMC5883P},
	{}
};

static struct i2c_driver compass_qmc5883p_driver = {
	.probe = compass_qmc5883p_probe,
	.remove = compass_qmc5883p_remove,
	.shutdown = sensor_shutdown,
	.id_table = compass_qmc5883p_id,
	.driver = {
		.name = "compass_qmc5883p",
	#ifdef CONFIG_PM
		.pm = &sensor_pm_ops,
	#endif
	},
};

module_i2c_driver(compass_qmc5883p_driver);

MODULE_AUTHOR(" weihq <weihq@xtreamer.net.cn>");
MODULE_DESCRIPTION("qmc5883p 3-Axis compasss driver");
MODULE_LICENSE("GPL");