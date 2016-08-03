/*
 * DTS disables ehrpwm by default 
 * Adding a platform device
 */
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("skukreti");
MODULE_DESCRIPTION("WS2812B Driver with TI PWM");


#define PINMUX_BASE_ADDR 	 0x44e10000
#define PWMSS_START_ADDRESS  0x48300000
#define PWMSS_END_ADDRESS    0x483000FF
#define EHRPWM_START_ADDRESS 0x48300200
#define EHRPWM_END_ADDRESS   0x4830025F



struct pwmsubsys {
	struct pwm_chip ti_pwm;
	unsigned int data;
	struct platform_device *pdev;
}pwm_data;



void pwm_cleanup(struct device *dev)
{
	pr_info("PWM Release Function \n");

}

int  pwm_probe ( struct platform_device *pdev ) 
{
	struct resource *r_mem_pwmss, *r_mem_ehrpwm; 

	pr_info("PWM0 Probe Called\n");
	r_mem_pwmss = platform_get_resource (pdev, IORESOURCE_MEM, 0);

	pr_info("1\n");
	if(!r_mem_pwmss) 
		pr_err("Resource MEM PWMSS not available \n");

	r_mem_ehrpwm = platform_get_resource (pdev, IORESOURCE_MEM, 1);
	pr_info("2\n");

	if(!r_mem_ehrpwm) 
		pr_err("Resource MEM EHRPWM not available \n");


	/* Check if IO Memory is available for use by the driver */

	if((request_mem_region(r_mem_pwmss->start, r_mem_pwmss->end - \
						r_mem_pwmss->start +1 , "EHRPWM_LED"))) {
		pr_err (" %s, PWMSS Memory  Used by Anoter Driver\n", __func__);
	
	}
	pr_info("3\n");

	if((request_mem_region(r_mem_ehrpwm->start, r_mem_ehrpwm->end - \
						r_mem_ehrpwm->start +1 , "EHRPWM_LED"))) {
		pr_err (" %s, EHRPWM Memory  Used by Anoter Driver\n", __func__);
	
	}

	pr_info("4\n");

	pr_info("All is good \n");

	pwm_data.pdev = pdev;
	return 0;
}

int pwm_remove (struct platform_device *pdev)
{
	pr_info("PWM Remove\n");
	return 0;
}


struct platform_driver pwm_driver = {

	.probe = &pwm_probe,
	.remove = &pwm_remove,
	.driver = {
		.name = "sk, pwm-dev",
	
	},

};



static struct resource pwmsubsys0_resource[] = {
	{
		.start = PWMSS_START_ADDRESS,
		.end   = PWMSS_END_ADDRESS,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = EHRPWM_START_ADDRESS,
		.end = EHRPWM_END_ADDRESS,
		.flags = IORESOURCE_MEM,
	}

};


static struct platform_device pwm_device = {
	.name="sk, pwm-dev",
	.id = 0,
	.dev = {
			.platform_data = &pwm_data,
			.release = pwm_cleanup,
	},
	.resource = pwmsubsys0_resource,
	.num_resources = ARRAY_SIZE(pwmsubsys0_resource),
	
	
};


static int __init pwm_init(void)
{
	int ret = 0;
	ret = platform_device_register(&pwm_device);
	if(ret < 0) 
		pr_info(" PWM Device Reg failed\n");
	ret = platform_driver_register(&pwm_driver);
	if(ret < 0) 
		pr_info(" PWM Driver Reg failed\n");

return ret;
}


static void __exit pwm_exit(void) 
{
	platform_driver_unregister(&pwm_driver);
	platform_device_unregister(&pwm_device);

}


module_init(pwm_init);
module_exit(pwm_exit);
