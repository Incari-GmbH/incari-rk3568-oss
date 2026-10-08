#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

#define LED_PWM_CMD_SET_DUTY         0x01
#define LED_PWM_CMD_SET_PERIOD       0x02
#define LED_PWM_CMD_SET_BOTH         0x03
#define LED_PWM_CMD_ENABLE           0x04
#define LED_PWM_CMD_DISABLE          0x05

struct led_pwm_param {
    int duty_ns;
    int period_ns;
};

struct red_led_dev {
    dev_t dev_no;                    
    struct cdev chrdev;            
    struct class *led_class;
    struct device_node *dev_node;
    struct pwm_device *red_led_pwm;
};

static struct led_pwm_param led_pwm;
static struct red_led_dev led_dev;

static int red_led_drv_open (struct inode *node, struct file *file)
{
    int ret = 0;
    
	//pwm_set_periodnnn(led_dev.red_led_pwm, PWM_POLARITY_INVERSED);//设置PWM信号的极性
	pwm_enable(led_dev.red_led_pwm);//启用指定PWM设备，使其开始输出PWM信号。

    return ret;
}

static ssize_t red_led_drv_write (struct file *file, const char __user *buf, size_t size, loff_t *offset)
{
    int err;

    if (size != sizeof(led_pwm)) return -EINVAL;

	err = copy_from_user(&led_pwm, buf, size);
    if (err > 0) return -EFAULT;
	printk("m555 copy_from_user duty = %d\t period = %d\n",led_pwm.duty_ns,led_pwm.period_ns);
	pwm_config(led_dev.red_led_pwm, led_pwm.duty_ns, led_pwm.period_ns);//配置PWM设备的基本参数，如频率、占空比等。

	return 1;
}

static long drv_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    void __user *my_user_space = (void __user *)arg;
    
    switch (cmd)
    {
        case LED_PWM_CMD_SET_DUTY:
            ret = copy_from_user(&led_pwm.duty_ns, my_user_space, sizeof(led_pwm.duty_ns));
            if (ret > 0) return -EFAULT;
            pwm_config(led_dev.red_led_pwm, led_pwm.duty_ns, led_pwm.period_ns);
            break;
        case LED_PWM_CMD_SET_PERIOD:
            ret = copy_from_user(&led_pwm.period_ns, my_user_space, sizeof(led_pwm.period_ns));
            if (ret > 0) return -EFAULT;
            pwm_config(led_dev.red_led_pwm, led_pwm.duty_ns, led_pwm.period_ns);
            break;
        case LED_PWM_CMD_SET_BOTH: 
            ret = copy_from_user(&led_pwm, my_user_space, sizeof(led_pwm));
            if (ret > 0) return -EFAULT;
            pwm_config(led_dev.red_led_pwm, led_pwm.duty_ns, led_pwm.period_ns);
            break;
        case LED_PWM_CMD_ENABLE:
            pwm_enable(led_dev.red_led_pwm);
            break;
        case LED_PWM_CMD_DISABLE:
            pwm_disable(led_dev.red_led_pwm);
            break;
    }
    return 0;
}

static int red_led_drv_release(struct inode *node, struct file *filp)
{
    int ret = 0;

    //pwm_config(led_dev.red_led_pwm, 0, 5000);

	//pwm_disable(led_dev.red_led_pwm);
    return ret;
}

static struct file_operations red_led_drv = {
	.owner	 = THIS_MODULE,
	.open    = red_led_drv_open,
	.write   = red_led_drv_write,
    .unlocked_ioctl = drv_ioctl,
    .release  = red_led_drv_release,
};

static struct of_device_id dts_match_table[] = {
    {.compatible = "ntimes,pwm2_ctrl", },
};


static int led_red_driver_probe(struct platform_device *pdev)
{
    int err;
    int ret;
    struct device *tdev;
    struct device_node *child;

    tdev = &pdev->dev;
    child = of_get_next_child(tdev->of_node, NULL);
	if (!child) {
        return -EINVAL;
    }

    led_dev.red_led_pwm = devm_of_pwm_get(tdev, child, NULL);
    if (IS_ERR(led_dev.red_led_pwm)) {
        printk(KERN_ERR"can't get pwm2_ctrl!!\n");
        return -EFAULT;
    }
	pwm_config(led_dev.red_led_pwm, 12500, 25000);//opposite polarity 40khz, 50% = (Th/T *100)
	pwm_enable(led_dev.red_led_pwm);

    ret = alloc_chrdev_region(&led_dev.dev_no, 0, 1, "pwm2_ctrl");
	if (ret < 0) {
		pr_err("Error: failed to register mbochs_dev, err: %d\n", ret);
		return ret;
	}

	cdev_init(&led_dev.chrdev, &red_led_drv);

	cdev_add(&led_dev.chrdev, led_dev.dev_no, 1);

    led_dev.led_class = class_create(THIS_MODULE, "pwm2_ctrl");
	err = PTR_ERR(led_dev.led_class);
	if (IS_ERR(led_dev.led_class)) {
        goto failed1;
	}

    tdev = device_create(led_dev.led_class , NULL, led_dev.dev_no, NULL, "pwm2_ctrl");
    if (IS_ERR(tdev)) {
        ret = -EINVAL;
		goto failed2;
	}

   	printk(KERN_INFO"%s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    
    return 0;
failed2:
    device_destroy(led_dev.led_class, led_dev.dev_no);
    class_destroy(led_dev.led_class);
failed1:
    cdev_del(&led_dev.chrdev);
	unregister_chrdev_region(led_dev.dev_no, 1);
    return ret;
}

int led_red_driver_remove9(struct platform_device *dev)
{
    // pwm_disable(led_dev.red_led_pwm);
    // pwm_free(led_dev.red_led_pwm);
    printk(KERN_INFO"driver remove %s %s line %d\n", __FILE__, __FUNCTION__, __LINE__);
    device_destroy(led_dev.led_class, led_dev.dev_no);
	class_destroy(led_dev.led_class);
	unregister_chrdev_region(led_dev.dev_no, 1);
    cdev_del(&led_dev.chrdev);
     
    return 0;
}

static struct platform_driver red_led_platform_driver9 = {
      .probe = led_red_driver_probe,
      .remove = led_red_driver_remove9,
      .driver = {
        .name = "ntimes,pwm2_ctrl",
        .owner = THIS_MODULE,
        .of_match_table = dts_match_table,
      },
};

module_platform_driver(red_led_platform_driver9);

MODULE_AUTHOR("ntimes-m5");
MODULE_LICENSE("GPL");
