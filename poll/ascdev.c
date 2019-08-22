#include <linux/init.h>
/*包含初始化宏定义的头文件,代码中的module_init和module_exit在此文件中*/
#include <linux/module.h>
/*包含初始化加载模块的头文件,代码中的MODULE_LICENSE在此头文件中*/

/*定义module_param module_param_array的头文件*/
#include <linux/moduleparam.h>
/*定义module_param module_param_array中perm的头文件*/
#include <linux/stat.h>

/*三个字符设备函数*/
#include <linux/fs.h>
/*MKDEV转换设备号数据类型的宏定义*/
#include <linux/kdev_t.h>
/*定义字符设备的结构体*/
#include <linux/cdev.h>
/*分配内存空间函数头文件*/
#include <linux/slab.h>

/*包含函数device_create 结构体class等头文件*/
#include <linux/device.h>

/*Linux中申请GPIO的头文件*/
#include <linux/gpio.h>
/*三星平台的GPIO配置函数头文件*/
/*三星平台EXYNOS系列平台，GPIO配置参数宏定义头文件*/
#include <plat/gpio-cfg.h>
/*三星平台4412平台，GPIO宏定义头文件*/
#include <mach/gpio-exynos4.h>

#include <asm/uaccess.h>

#include <linux/wait.h>
#include <asm/system.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/sched.h>

#include <linux/poll.h>

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

#define DEVICE_NAME "ascdev"
#define DEVICE_MINOR_NUM 2
#define DEV_MAJOR 0
#define DEV_MINOR 0
#define REGDEV_SIZE 3000

MODULE_LICENSE("Dual BSD/GPL");
/*声明是开源的，没有内核版本限制*/
MODULE_AUTHOR("iTOPEET_dz");
/*声明作者*/

static int led_gpios[] = {
	EXYNOS4_GPL2(0),EXYNOS4_GPK1(1),
};

int numdev_major = DEV_MAJOR;
int numdev_minor = DEV_MINOR;

/*输入主设备号*/
module_param(numdev_major,int,S_IRUSR);
/*输入次设备号*/
module_param(numdev_minor,int,S_IRUSR);

static struct class *ascdev_class;

struct reg_dev
{
	char *data;
	unsigned long size;
	struct cdev cdev;
	struct semaphore sem;     /* 定义信号量 */
	wait_queue_head_t wait_queue;    /*等待队列*/
};

struct reg_dev *my_devices;

bool have_data = false; /*表明设备有足够数据可供读*/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*打开函数  主要任务是拿到次设备号 放到filp中*/
int scdev_open (struct inode *inode, struct file *filp)
{
	struct reg_dev *dev;
    
    /*获取次设备号*/
    int num = MINOR(inode->i_rdev);

    if (num >= DEVICE_MINOR_NUM) 
            return -ENODEV;
    dev = &my_devices[num];
    
    /*将设备描述结构指针赋值给文件私有数据指针*/
    filp->private_data = dev;
    
    return 0;
}

/*ppos:读的位置相对于文件开头的偏移*/
ssize_t scdev_read (struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	int ret=0;
	unsigned long p =  *ppos;
	unsigned long count=size;
	struct reg_dev *dev;
	dev=filp->private_data;
    
	/*判断位置是否有效*/
	if(p>=REGDEV_SIZE)return 0;
	if(count+p>=REGDEV_SIZE)  count=REGDEV_SIZE-p;
	
	printk(KERN_EMERG "3d\n");
	/*获取信号量*/
	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;      /*若是被信号打断引起唤醒，直接返回不进行读*/
	while(!have_data)
	{
		if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
		
		wait_event_interruptible(dev->wait_queue,have_data);
	}
	printk(KERN_EMERG "4d\n");
	

    if(copy_to_user(buf,dev->data+p,count))/*带参数检测*/
	{
		printk(KERN_EMERG "copy_to_user is failed! %d\n",ret);
		ret =  - EFAULT;
	}
	else
	{
		*ppos+=count;
		ret=count;
		printk(KERN_EMERG "read %d bytes(s) from %d\n", count, p);
	}
	
	/*释放信号量*/
	up(&dev->sem);
	return ret;
}


ssize_t scdev_write (struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	int ret=0;
	unsigned long p =  *ppos;
	unsigned long count=size;
	struct reg_dev *dev;
	dev=filp->private_data;
    
	/*判断位置是否有效*/
	if(p>=REGDEV_SIZE)return 0;
	if(count+p>=REGDEV_SIZE)  count=REGDEV_SIZE-p;
		
    printk(KERN_EMERG "1d\n");
	/*获取信号量*/
	// if(down_interruptible(&dev->sem))
		// return -ERESTARTSYS;      /*若是被信号打断引起唤醒，直接返回不进行读*/
	printk(KERN_EMERG "2d\n");
	
    if(copy_from_user(dev->data+p,buf,count))/*带参数检测*/
	{
		printk(KERN_EMERG "copy_from_user is failed! %d\n",ret);
		ret =  - EFAULT;
	}
	else
	{
		*ppos+=count;
		ret=count;
		printk(KERN_EMERG "write %d bytes(s) from %d\n", count, p);
	}
	
	have_data=true;
	/*从等待队列中唤醒*/
	wake_up_interruptible(&dev->wait_queue);
	
	/*释放信号量*/
	// up(&dev->sem);
	return ret;
}

loff_t scdev_llseek (struct file *filp, loff_t offset, int whence)
{
	loff_t newpos;
	switch(whence)
	{
	  case 0:/* SEEK_SET */
		newpos=offset;
		break;
	  case 1:/* SEEK_CUR */
	    newpos=filp->f_pos+offset;
		break;
	  case 2:/* SEEK_END */
	    newpos=REGDEV_SIZE -1 + offset;
		break;
	  default:
	    return -EINVAL;
	}
	
	if ((newpos<0) || (newpos>REGDEV_SIZE))
    	return -EINVAL;
	
    filp->f_pos = newpos;
    return newpos;	
}

/*poll函数本身不会阻塞  select系统调用会根据poll函数的返回值最终决定是否阻塞*/
unsigned int ascdeb_poll (struct file *filp, struct poll_table_struct *wait)
{
	struct reg_dev *dev=filp->private_data;
	unsigned int mask=0;
	
	/*将等待队列添加到poll_table */
	poll_wait(filp,&dev->wait_queue,wait);
	
	if (have_data)         mask |= POLLIN | POLLRDNORM;  /* readable */

    return mask;
}


/*led0  IO操作*/
long led0_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk("led0 cmd is %d, arg is %d\n", cmd,arg);
	if (cmd>1)
	{
		printk(KERN_EMERG "led0 cmd is 0 or 1 \n");
		return -EINVAL;
	}
	if (arg >1 )
	{
		printk(KERN_EMERG "led0 arg is only 1 \n");
		return -EINVAL;
	}
	
	gpio_set_value(led_gpios[0],cmd);
	
	return 0;
}

/*led1  IO操作*/
long led1_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	printk("led1 cmd is %d, arg is %d\n", cmd,arg);
	if (cmd>1)
	{
		printk(KERN_EMERG "cmd is 0 or 1 \n");
		return -EINVAL;
	}
	if (arg >1 )
	{
		printk(KERN_EMERG "arg is only 1 \n");
		return -EINVAL;
	}
	
	gpio_set_value(led_gpios[1],cmd);
	
	return 0;
}

int scdev_release (struct inode *inode, struct file *filp)
{
	return 0;
}


struct file_operations mu_fops[2];


/*注册到系统*/
static void reg_init_cdev(struct reg_dev *dev,int index)
{
	int err;
	dev_t devno = MKDEV(numdev_major,numdev_minor+index);
	/*数据初始化*/
	cdev_init(&dev->cdev,&mu_fops[index]);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &mu_fops[index];
	
	/*creates a device and registers it with sysfs   注意device_create的返回值*/
	device_create(ascdev_class, NULL,devno,NULL,DEVICE_NAME"%d", index);

	/*添加设备到系统*/
	err = cdev_add(&dev->cdev,devno,1);
	
	if (err)
	{
		printk(KERN_EMERG "cdev_add %d is failed! %d\n",index,err);
	}
	else 
	{
		printk(KERN_EMERG "cdev_add %d is success!\n",index);
	}
}


static int led_init(int index)
{
	int ret;
	
	
	
	ret = gpio_request(led_gpios[index],"LEDS");
	if (ret<0)
	{
		printk(KERN_EMERG "gpio_request led%d failed\n",index);
		return ret;
	}
	
	s3c_gpio_cfgpin(led_gpios[index],S3C_GPIO_OUTPUT);
	gpio_set_value(led_gpios[index],0);
    printk(KERN_EMERG "led%d tinitialize successfully\n",index);
	return 0;
}

static int scdev_init(void)
{
	int ret = 0;
	int i;
	dev_t num_dev;
	
	
	printk(KERN_EMERG "numdev_major is %d!\n",numdev_major);
	printk(KERN_EMERG "numdev_minor is %d!\n",numdev_minor);
	
	if(numdev_major){
		num_dev = MKDEV(numdev_major,numdev_minor);
		ret = register_chrdev_region(num_dev,DEVICE_MINOR_NUM,DEVICE_NAME);
	}
	else{
		/*动态注册设备号*/
		ret = alloc_chrdev_region(&num_dev,numdev_minor,DEVICE_MINOR_NUM,DEVICE_NAME);
		/*获得主设备号*/
		numdev_major = MAJOR(num_dev);
		printk(KERN_EMERG "adev_region req %d !\n",numdev_major);
	}
		
	if(ret<0){
		printk(KERN_EMERG "register_chrdev_region req %d is failed!\n",numdev_major);		
	}
	/*为设备结构体分配内存空间*/	
	my_devices = kmalloc(DEVICE_MINOR_NUM*sizeof(struct reg_dev),GFP_KERNEL);
	/*创建class 并分配内存空间*/
	ascdev_class = class_create(THIS_MODULE,DEVICE_NAME);
	
	if(!my_devices)
	{
		ret = -ENOMEM;
		goto fail;
	}
	memset(my_devices,0,DEVICE_MINOR_NUM*sizeof(struct reg_dev));
	/*设备初始化*/


	mu_fops[0].owner = THIS_MODULE;
	mu_fops[0].llseek = scdev_llseek;
    mu_fops[0].read = scdev_read;
    mu_fops[0].write = scdev_write;
    mu_fops[0].open = scdev_open;
    mu_fops[0].release = scdev_release;
	mu_fops[0].unlocked_ioctl=led0_ioctl;
	mu_fops[0].poll=ascdeb_poll;

	mu_fops[1].owner = THIS_MODULE;
	mu_fops[1].llseek = scdev_llseek;
    mu_fops[1].read = scdev_read;
    mu_fops[1].write = scdev_write;
    mu_fops[1].open = scdev_open;
    mu_fops[1].release = scdev_release;
	mu_fops[1].unlocked_ioctl=led1_ioctl;
	mu_fops[0].poll=ascdeb_poll;

	for (i=0;i<DEVICE_MINOR_NUM;i++)
	{
		my_devices[i].data = kmalloc(REGDEV_SIZE,GFP_KERNEL);
		memset(my_devices[i].data,0,REGDEV_SIZE);  // memset(&my_devices[i],0,REGDEV_SIZE);  /*此句有导致内核BUG 使虚拟地址为0*/
		/*注册到系统*/
		reg_init_cdev(&my_devices[i],i);
		/*IO初始化*/
		ret=led_init(i);
		if(ret)   printk(KERN_EMERG "led%d_init failed!\n",i);
		
		sema_init (&my_devices[i].sem, 1);/* 初始化设置信号量的初值，它设置信号量sem的值为val */
			
		init_waitqueue_head(&my_devices[i].wait_queue);/*定义并初始化等待队列*/
        
	}

	
	printk(KERN_EMERG "scdev_init!\n");
	/*打印信息，KERN_EMERG表示紧急信息*/
	return 0;
	
fail:
     /*注销设备号*/
	unregister_chrdev_region(MKDEV(numdev_major,numdev_minor),DEVICE_MINOR_NUM);
	printk(KERN_EMERG "kmalloc is failed\n");
	return ret;
}

static void scdev_exit(void)
{
	int i;
	printk(KERN_EMERG "scdev_exit!\n");
	/*除去字符设备*/
	for (i=0;i<DEVICE_MINOR_NUM;i++)
	{
		cdev_del(&my_devices[i].cdev);
		/*删除设备节点*/
		device_destroy(ascdev_class,MKDEV(numdev_major,numdev_minor+i));
		/*释放GPIO*/
		gpio_free(led_gpios[i]);
	}
	/*删除类*/
	class_destroy(ascdev_class);
	
	unregister_chrdev_region(MKDEV(numdev_major,numdev_minor),DEVICE_MINOR_NUM);
	
}


module_init(scdev_init);
/*初始化函数*/
module_exit(scdev_exit);
/*卸载函数*/