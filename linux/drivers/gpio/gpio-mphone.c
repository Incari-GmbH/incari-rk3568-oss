#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/jiffies.h>
#define TAGLOG "NNNTIMES_XIXUN_GPIO"

#define DEBOUNCE_DELAY 200
#define INVALID_GPIO -1
//gps poweren -> 152
int array_high_gpio[] = { 152, 43, 21 };

int ntimes_gpio_active_l = 0;
int ntimes_gpio_active_h = 1;

static unsigned long last_pressed_jiffies;
struct work_struct wakeup_wq;
void wakeup_work(struct work_struct *work);

struct gpio_read_data {
	int index; // 0 或 1
	int level; //gpio 号
	int dir; //gpio 方向
} data;

static int ntimes_gpio_open(struct inode *inode, struct file *filp)
{
	//printk("%s %s line %d\n", TAGLOG, __FUNCTION__, __LINE__);
	return 0;
}
static ssize_t ntimes_gpio_read(struct file *filp, char __user *buf,
				size_t count, loff_t *fops)
{
	int i;
	int err = -1;
	if (copy_from_user(&data, buf, sizeof(data))) {
		return -EFAULT;
	}

	for (i = 0; i < sizeof(array_high_gpio) / sizeof(int); i++) {
		if (data.level == array_high_gpio[i]) {
			data.index = gpio_get_value(array_high_gpio[i]);
			goto find;
		}
	}

find:
	err = copy_to_user(buf, &data, sizeof(data));
	if (err) {
		//	printk("%s %s line %d\n", TAGLOG, __FUNCTION__, __LINE__);
	}
	return count;
}
static int ntimes_gpio_release(struct inode *inode, struct file *file)
{
	printk("%s %s line %d\n", TAGLOG, __FUNCTION__, __LINE__);
	return 0;
}

static ssize_t ntimes_gpio_write(struct file *filp, const char __user *buf,
				 size_t count, loff_t *fops)
{
	int ret;
	ret = copy_from_user(&data, buf, sizeof(data));
	if (ret) {
		return -EFAULT;
	}
	//printk("m555 copy from user data.index = %d , data.level = %d, data.dir = %d\n",data.index,data.level,data.dir);	//index 0,1   level gpionum  , dir 0(in) 1(out)
	if (data.dir == 0)
		gpio_direction_input(data.level);
	else if (data.dir == 1)
		gpio_direction_output(data.level, data.index);
	else
		return -1;
	return 0;
}

static const struct file_operations ntimes_gpio_fops = {
	.owner = THIS_MODULE,
	.open = ntimes_gpio_open,
	.read = ntimes_gpio_read,
	.release = ntimes_gpio_release,
	.write = ntimes_gpio_write,
	//.unlocked_ioctl = ntimes_gpio_ioctl,
	//.compat_ioctl	= ntimes_gpio_ioctl,
};

static struct miscdevice ntimes_gpio_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ntimes_gpio",
	.fops = &ntimes_gpio_fops,
};

static irqreturn_t wakeup_interrupt(int irq, void *dev_id)
{
	schedule_work(&wakeup_wq);
	return IRQ_HANDLED;
}

//reserved
void wakeup_do_work(struct work_struct *work)
{
	unsigned long current_jiffies = jiffies;

	if (time_before(current_jiffies,
			last_pressed_jiffies +
				msecs_to_jiffies(DEBOUNCE_DELAY)))
		printk("m555 misoperation ...\n");
	else
		printk("m555 system wakeup...\n");

	last_pressed_jiffies = current_jiffies;
}

static int ntimes_gpio_probe(struct platform_device *pdev)
{
	int ret = -1;
	int i = 0;
	//wakeup irq
	int gpio_wakeup;
	enum of_gpio_flags flags;
	//wakeup gpio nd
	struct device_node *wakeup_gpios = pdev->dev.of_node;

	/* register test_gpio dev file */
	ret = misc_register(&ntimes_gpio_dev);
	if (ret < 0) {
		printk("%s %s line %d\n", TAGLOG, __FUNCTION__, __LINE__);
		return ret;
	}

	for (i = 0; i < sizeof(array_high_gpio) / sizeof(int); i++) {
		if (!gpio_is_valid(array_high_gpio[i]))
			return -EFAULT;
		if (gpio_request(array_high_gpio[i], "ntimes_gpio"))
			return -EIO;
		gpio_direction_output(array_high_gpio[i],
				      ntimes_gpio_active_h); //max<3.4
	}

	gpio_wakeup = of_get_named_gpio_flags(wakeup_gpios, "wakeup-gpios", 0,
					      &flags);
	if (!gpio_is_valid(gpio_wakeup)) {
		printk("gpio_wakeup: %d is invalid\n", gpio_wakeup);
		return -ENODEV;
	}

	if (gpio_request(gpio_wakeup, "gpio_wakeup")) {
		printk("gpio_wakeup: %d request failed!\n", gpio_wakeup);
		gpio_free(gpio_wakeup);
		return -ENODEV;
	}

	gpio_direction_input(gpio_wakeup);

	ret = request_irq(gpio_to_irq(gpio_wakeup), wakeup_interrupt,
			  IRQF_TRIGGER_FALLING, "wakeup-gpio-irq", NULL);
	if (ret) {
		gpio_free(gpio_wakeup);
		return ret;
	}

	INIT_WORK(&wakeup_wq, wakeup_do_work);

	ret = enable_irq_wake(gpio_to_irq(gpio_wakeup));
	if (ret) {
		printk("m555 enable_irq_wake failed, %d\n", ret);
		return ret;
	}
	return 0;
}

static int ntimes_gpio_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id of_ntimes_gpio_match[] = {
	{ .compatible = "ntimes,mphone_gpio" },
	{ /* sentinel */ }
};

static struct platform_driver ntimes_gpio_driver = {
	.probe = ntimes_gpio_probe,/*(struct platform_device * pdev)*/
	.remove = ntimes_gpio_remove,
	.driver = {
				.name = "ntimes_gpio",
				.owner = THIS_MODULE,
				.of_match_table = of_ntimes_gpio_match,
				},
};

static int ntimes_gpio_init(void)
{
	platform_driver_register(&ntimes_gpio_driver);
	printk("%s %s line %d\n", TAGLOG, __FUNCTION__, __LINE__);
	return 0;
}

static void ntimes_gpio_exit(void)
{
	platform_driver_unregister(&ntimes_gpio_driver);
	printk("%s %s line %d\n", TAGLOG, __FUNCTION__, __LINE__);
}

module_init(ntimes_gpio_init);
module_exit(ntimes_gpio_exit);

MODULE_AUTHOR("ntimes-m5");
MODULE_DESCRIPTION("nimes gpio driver");
MODULE_LICENSE("GPL");
