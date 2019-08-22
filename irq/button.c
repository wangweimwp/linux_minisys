#include <linux/init.h>
#include <linux/module.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
//#include <mach/gpio-bank.h>
#include <mach/regs-gpio.h>
#include <asm/io.h>
#include <linux/regulator/consumer.h>

#include <linux/delay.h>

//中断头文件
#include <linux/irq.h>
#include <linux/interrupt.h>

/*Linux中申请GPIO的头文件*/
#include <linux/gpio.h>
/*三星平台的GPIO配置函数头文件*/
/*三星平台EXYNOS系列平台，GPIO配置参数宏定义头文件*/
#include <plat/gpio-cfg.h>
/*三星平台4412平台，GPIO宏定义头文件*/
#include <mach/gpio-exynos4.h>

#include <linux/input.h>
/*需要卸载开发板上原有的按键中断程序*/
/*Home  GPX1_1   EXYNOS4_GPX1(1) IRQ_EINT(9) 
  Back  GPX1_2	 EXYNOS4_GPX1(2) IRQ_EINT(10)*/
  
MODULE_AUTHOR("wangwei");
MODULE_LICENSE("GPL"); 

struct input_dev *button_dev;
struct button_irq_desc {
    int irq;
    int pin;
    int pin_setting;
    int number;
    char *name;	
};
static struct button_irq_desc button_irqs [] = {
    {IRQ_EINT(9),  EXYNOS4_GPX1(1) ,  IRQ_EINT9  , 0, "HOME"},  
    {IRQ_EINT(10), EXYNOS4_GPX1(2) ,  IRQ_EINT10 , 1, "BACK"},
    
};

//static int key_values = 0;
/***********************************************************************/
/***********************************************************************/
/***********************************************************************/

/*中断处理程序*/
static irqreturn_t buttons_interrupt(int irq, void *dev_id)
{
	struct button_irq_desc *button_irq=(struct button_irq_desc *)dev_id;
	int down;
	
/* 	down = !gpio_get_value(EXYNOS4_GPX1(2));
	printk (KERN_EMERG "gpio EXYNOS4_GPX1(2)%d value is %d \n",EXYNOS4_GPX1(2),down); */
	down = gpio_get_value(button_irq->pin);
	printk (KERN_EMERG "gpio %d value is %d \n",button_irq->pin,down);
/* 	if(!down)

	{
		switch(button_irq->number)
		{
			case 0:
			input_report_key(button_dev, BTN_0, 0);
			break;
			case 1:
			input_report_key(button_dev, BTN_1, 0);
			break;
			default:
			;
		}
	}
	else
	{
		switch(button_irq->number)
		{
			case 0:
			input_report_key(button_dev, BTN_0, 1);
			break;
			case 1:
			input_report_key(button_dev, BTN_1, 1);				
			break;
			default:
			;
		}
	}
	
	input_sync(button_dev);*//*输入系统同步*/
	if (down) { 
	
	/*报告事件*/

	printk("====>rising key_values=%d\n",button_irq->number);
  if(button_irq->number==0)
  	input_report_key(button_dev, BTN_0, 0);
  if(button_irq->number==1)
  	input_report_key(button_dev, BTN_1, 0);
 
  
  input_sync(button_dev);      
   	 }
  
     else {
	 

        printk("====>falling key_values=%d\n",button_irq->number);
  if(button_irq->number==0)
        input_report_key(button_dev, BTN_0, 1);
  if(button_irq->number==1)
        input_report_key(button_dev, BTN_1, 1);

  input_sync(button_dev);      

     }
	printk (KERN_EMERG "input_sync key %d \n",button_irq->number); 
	return IRQ_RETVAL(IRQ_HANDLED);
	
}

/*申请中断号*/
static int EXYNOS4_request_irq(void)
{
	int i,err=0;
	
	for(i=0;i<sizeof(button_irqs)/sizeof(button_irqs[0]);i++)
	{
		err=request_irq(button_irqs[i].irq,buttons_interrupt,IRQ_TYPE_EDGE_BOTH,
						button_irqs[i].name,(void *)&button_irqs[i]);
		if(err)
		{
			printk (KERN_EMERG "request_irq %d is faild err %d \n",i,err);
			break;
		}
		err = gpio_request(button_irqs[i].pin,button_irqs[i].name);
		if(err)
		{
			printk (KERN_EMERG "gpio_request %d is faild err %d \n",i,err);
			break;
		}
/* 		err = s3c_gpio_cfgpin(button_irqs[i].pin,button_irqs[i].pin_setting);
		if(err)
		{
			printk (KERN_EMERG "s3c_gpio_cfgpin %d is faild err %d \n",i,err);
			break;
		} */
	}
	if(err)
	{
		i--;
        for (; i >= 0; i--)
		{
			if (button_irqs[i].irq < 0) 
				continue;
			
			disable_irq(button_irqs[i].irq);
			free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
        }
        return -EBUSY;
	}
	
	return 0;
}

static int dev_init(void)
{
	int ret;
	EXYNOS4_request_irq();

	button_dev=input_allocate_device();/*为输入结构体分配内存*/
	if(button_dev==NULL)
	{
		printk(KERN_EMERG "Unable to allocate the input device !!\n");
		return -ENOMEM;
	}
	
	set_bit(EV_KEY, button_dev->evbit);//支持EV_KEY事件
	set_bit(BTN_0, button_dev->keybit);
	set_bit(BTN_1, button_dev->keybit);
	ret=input_register_device(button_dev); //注册input设备
	if(ret)
	{
		printk (KERN_EMERG "input_register_device failed err is %d\n",ret);
		return ret;
	}
	printk (KERN_EMERG "initialized\n");

	return 0;
}
static void dev_exit(void)
{
  int i;
    
  for (i = 0; i < sizeof(button_irqs)/sizeof(button_irqs[0]); i++) {
		if (button_irqs[i].irq < 0) {
	  	  continue;
			}
		free_irq(button_irqs[i].irq, (void *)&button_irqs[i]);
		gpio_free(button_irqs[i].pin);
   }

	input_unregister_device(button_dev);
	printk (KERN_EMERG "dev_exit\n");
} 
module_init(dev_init);
module_exit(dev_exit); 
