#include <linux/init.h>
#include <linux/module.h>
/*驱动注册的头文件  包含结构体 驱动注册和卸载的函数*/
#include <linux/platform_device.h>
/*注册杂项设备的头文件*/
#include <linux/miscdevice.h>
/*注册设备节点的文件结构体*/
#include <linux/fs.h>

/*linux中申请GPIO的头文件*/
//#include <linux/gpio.h>
/*三星平台的GPIO配置函数头文件*/
/*三星平台EXYNOS系列的GPIO配置参数宏定义头文件*/
#include <plat/gpio-cfg.h>

#include <mach/gpio.h>
/*三星平台4412平台的GPIO宏定义的头文件*/
#include <mach/gpio-exynos4.h>


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("WangWei");

#define DRIVER_NAME "hello_ctl"    /*驱动名称*/
#define DEVICE_NAME "hello_ctl"   /*设备节点的名称*/

static int hello_open (struct inode *inode, struct file *file)
{
	printk(KERN_EMERG "hello_open\n");
	return 0;
}

static int hello_release (struct inode *inode, struct file *file)
{
	printk(KERN_EMERG "hello_release\n");
	return 0;
}

static long hello_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("cmd is %d, arg is %d\n", cmd,arg);
	if (cmd>1)
	{
		printk(KERN_EMERG "cmd is 0 or 1 \n");
	}
	if (arg >1 )
	{
		printk(KERN_EMERG "arg is only 1 \n");
	}
	
	gpio_set_value(EXYNOS4_GPL2(0),cmd);
	
	return 0;
}


static struct file_operations hello_ops = 
{
	.owner = THIS_MODULE,
	.open = hello_open,
	.release = hello_release,
	.unlocked_ioctl = hello_ioctl,
};


static struct miscdevice hello_dev = 
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &hello_ops,
};


static int hello_probe(struct platform_device *pdv)
{
	int ret;
	printk(KERN_EMERG "\tinitialized\n");
	
	ret = gpio_request(EXYNOS4_GPL2(0),"LEDS");
	if (ret<0)
	{
		printk(KERN_EMERG "gpio_request EXYNOS4_GPL2(0) failed\n");
		return ret;
	}
	
	s3c_gpio_cfgpin(EXYNOS4_GPL2(0),S3C_GPIO_OUTPUT);
	gpio_set_value(EXYNOS4_GPL2(0),0);
	
	
	misc_register(&hello_dev);
	return 0;
}


static int hello_remove(struct platform_device *pdv)
{
	printk(KERN_EMERG "\tremove\n");
	misc_deregister(&hello_dev);
	return 0;
}

static void hello_shutdown(struct platform_device *pdv)
{
	;
}

static int hello_suspend(struct platform_device *pdv,pm_message_t pmt)
{
	return 0;
}

static int hello_resume(struct platform_device *pdv)
{
	return 0;
}

struct platform_driver hello_driver={
	.probe = hello_probe,
	.remove = hello_remove,
	.shutdown = hello_shutdown,
	.suspend = hello_suspend,
	.resume = hello_resume,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	}
};


static int hello_init(void)
{
	int Driverstate = 3;
	printk(KERN_EMERG "HELLO WPRLD enter\n");
	
	Driverstate = platform_driver_register(&hello_driver);
	printk(KERN_EMERG "\tDriverstate is %d\n",Driverstate);
	return 0;
}
static void hello_exit(void)
{
	printk(KERN_EMERG "HELLO WPRLD exit\n");
	platform_driver_unregister(&hello_driver);
	
}

module_init(hello_init);
module_exit(hello_exit);



