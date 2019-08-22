#include <linux/init.h>
/*������ʼ���궨���ͷ�ļ�,�����е�module_init��module_exit�ڴ��ļ���*/
#include <linux/module.h>
/*������ʼ������ģ���ͷ�ļ�,�����е�MODULE_LICENSE�ڴ�ͷ�ļ���*/

/*����module_param module_param_array��ͷ�ļ�*/
#include <linux/moduleparam.h>
/*����module_param module_param_array��perm��ͷ�ļ�*/
#include <linux/stat.h>

/*�����ַ��豸����*/
#include <linux/fs.h>
/*MKDEVת���豸���������͵ĺ궨��*/
#include <linux/kdev_t.h>
/*�����ַ��豸�Ľṹ��*/
#include <linux/cdev.h>
/*�����ڴ�ռ亯��ͷ�ļ�*/
#include <linux/slab.h>

/*��������device_create �ṹ��class��ͷ�ļ�*/
#include <linux/device.h>

/*Linux������GPIO��ͷ�ļ�*/
#include <linux/gpio.h>
/*����ƽ̨��GPIO���ú���ͷ�ļ�*/
/*����ƽ̨EXYNOSϵ��ƽ̨��GPIO���ò����궨��ͷ�ļ�*/
#include <plat/gpio-cfg.h>
/*����ƽ̨4412ƽ̨��GPIO�궨��ͷ�ļ�*/
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
/*�����ǿ�Դ�ģ�û���ں˰汾����*/
MODULE_AUTHOR("iTOPEET_dz");
/*��������*/

static int led_gpios[] = {
	EXYNOS4_GPL2(0),EXYNOS4_GPK1(1),
};

int numdev_major = DEV_MAJOR;
int numdev_minor = DEV_MINOR;

/*�������豸��*/
module_param(numdev_major,int,S_IRUSR);
/*������豸��*/
module_param(numdev_minor,int,S_IRUSR);

static struct class *ascdev_class;

struct reg_dev
{
	char *data;
	unsigned long size;
	struct cdev cdev;
	struct semaphore sem;     /* �����ź��� */
	wait_queue_head_t wait_queue;    /*�ȴ�����*/
};

struct reg_dev *my_devices;

bool have_data = false; /*�����豸���㹻���ݿɹ���*/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/*�򿪺���  ��Ҫ�������õ����豸�� �ŵ�filp��*/
int scdev_open (struct inode *inode, struct file *filp)
{
	struct reg_dev *dev;
    
    /*��ȡ���豸��*/
    int num = MINOR(inode->i_rdev);

    if (num >= DEVICE_MINOR_NUM) 
            return -ENODEV;
    dev = &my_devices[num];
    
    /*���豸�����ṹָ�븳ֵ���ļ�˽������ָ��*/
    filp->private_data = dev;
    
    return 0;
}

/*ppos:����λ��������ļ���ͷ��ƫ��*/
ssize_t scdev_read (struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
	int ret=0;
	unsigned long p =  *ppos;
	unsigned long count=size;
	struct reg_dev *dev;
	dev=filp->private_data;
    
	/*�ж�λ���Ƿ���Ч*/
	if(p>=REGDEV_SIZE)return 0;
	if(count+p>=REGDEV_SIZE)  count=REGDEV_SIZE-p;
	
	printk(KERN_EMERG "3d\n");
	/*��ȡ�ź���*/
	if(down_interruptible(&dev->sem))
		return -ERESTARTSYS;      /*���Ǳ��źŴ�������ѣ�ֱ�ӷ��ز����ж�*/
	while(!have_data)
	{
		if (filp->f_flags & O_NONBLOCK)
            return -EAGAIN;
		
		wait_event_interruptible(dev->wait_queue,have_data);
	}
	printk(KERN_EMERG "4d\n");
	

    if(copy_to_user(buf,dev->data+p,count))/*���������*/
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
	
	/*�ͷ��ź���*/
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
    
	/*�ж�λ���Ƿ���Ч*/
	if(p>=REGDEV_SIZE)return 0;
	if(count+p>=REGDEV_SIZE)  count=REGDEV_SIZE-p;
		
    printk(KERN_EMERG "1d\n");
	/*��ȡ�ź���*/
	// if(down_interruptible(&dev->sem))
		// return -ERESTARTSYS;      /*���Ǳ��źŴ�������ѣ�ֱ�ӷ��ز����ж�*/
	printk(KERN_EMERG "2d\n");
	
    if(copy_from_user(dev->data+p,buf,count))/*���������*/
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
	/*�ӵȴ������л���*/
	wake_up_interruptible(&dev->wait_queue);
	
	/*�ͷ��ź���*/
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

/*poll��������������  selectϵͳ���û����poll�����ķ���ֵ���վ����Ƿ�����*/
unsigned int ascdeb_poll (struct file *filp, struct poll_table_struct *wait)
{
	struct reg_dev *dev=filp->private_data;
	unsigned int mask=0;
	
	/*���ȴ�������ӵ�poll_table */
	poll_wait(filp,&dev->wait_queue,wait);
	
	if (have_data)         mask |= POLLIN | POLLRDNORM;  /* readable */

    return mask;
}


/*led0  IO����*/
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

/*led1  IO����*/
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


/*ע�ᵽϵͳ*/
static void reg_init_cdev(struct reg_dev *dev,int index)
{
	int err;
	dev_t devno = MKDEV(numdev_major,numdev_minor+index);
	/*���ݳ�ʼ��*/
	cdev_init(&dev->cdev,&mu_fops[index]);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &mu_fops[index];
	
	/*creates a device and registers it with sysfs   ע��device_create�ķ���ֵ*/
	device_create(ascdev_class, NULL,devno,NULL,DEVICE_NAME"%d", index);

	/*����豸��ϵͳ*/
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
		/*��̬ע���豸��*/
		ret = alloc_chrdev_region(&num_dev,numdev_minor,DEVICE_MINOR_NUM,DEVICE_NAME);
		/*������豸��*/
		numdev_major = MAJOR(num_dev);
		printk(KERN_EMERG "adev_region req %d !\n",numdev_major);
	}
		
	if(ret<0){
		printk(KERN_EMERG "register_chrdev_region req %d is failed!\n",numdev_major);		
	}
	/*Ϊ�豸�ṹ������ڴ�ռ�*/	
	my_devices = kmalloc(DEVICE_MINOR_NUM*sizeof(struct reg_dev),GFP_KERNEL);
	/*����class �������ڴ�ռ�*/
	ascdev_class = class_create(THIS_MODULE,DEVICE_NAME);
	
	if(!my_devices)
	{
		ret = -ENOMEM;
		goto fail;
	}
	memset(my_devices,0,DEVICE_MINOR_NUM*sizeof(struct reg_dev));
	/*�豸��ʼ��*/


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
		memset(my_devices[i].data,0,REGDEV_SIZE);  // memset(&my_devices[i],0,REGDEV_SIZE);  /*�˾��е����ں�BUG ʹ�����ַΪ0*/
		/*ע�ᵽϵͳ*/
		reg_init_cdev(&my_devices[i],i);
		/*IO��ʼ��*/
		ret=led_init(i);
		if(ret)   printk(KERN_EMERG "led%d_init failed!\n",i);
		
		sema_init (&my_devices[i].sem, 1);/* ��ʼ�������ź����ĳ�ֵ���������ź���sem��ֵΪval */
			
		init_waitqueue_head(&my_devices[i].wait_queue);/*���岢��ʼ���ȴ�����*/
        
	}

	
	printk(KERN_EMERG "scdev_init!\n");
	/*��ӡ��Ϣ��KERN_EMERG��ʾ������Ϣ*/
	return 0;
	
fail:
     /*ע���豸��*/
	unregister_chrdev_region(MKDEV(numdev_major,numdev_minor),DEVICE_MINOR_NUM);
	printk(KERN_EMERG "kmalloc is failed\n");
	return ret;
}

static void scdev_exit(void)
{
	int i;
	printk(KERN_EMERG "scdev_exit!\n");
	/*��ȥ�ַ��豸*/
	for (i=0;i<DEVICE_MINOR_NUM;i++)
	{
		cdev_del(&my_devices[i].cdev);
		/*ɾ���豸�ڵ�*/
		device_destroy(ascdev_class,MKDEV(numdev_major,numdev_minor+i));
		/*�ͷ�GPIO*/
		gpio_free(led_gpios[i]);
	}
	/*ɾ����*/
	class_destroy(ascdev_class);
	
	unregister_chrdev_region(MKDEV(numdev_major,numdev_minor),DEVICE_MINOR_NUM);
	
}


module_init(scdev_init);
/*��ʼ������*/
module_exit(scdev_exit);
/*ж�غ���*/