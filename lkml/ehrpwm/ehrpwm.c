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

#include "pwm.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("skukreti");
MODULE_DESCRIPTION("WS2812B Driver with TI PWM");

#define PWM_IRQ 86

struct ehrpwm_struct {
struct platform_device *pdev;
struct resource *r_mem;
void __iomem *pwm_base;
unsigned int virq;
}epwm;

static int  pwm_probe ( struct platform_device *pdev ) 
{
	uint16_t reg=0;
	printk(KERN_INFO"PWM_PROBE\n");

	epwm.r_mem = platform_get_resource (pdev, IORESOURCE_MEM, 0);
	pr_info("2\n");

	if(!epwm.r_mem) 
		pr_err("Resource MEM EHRPWM not available \n");


	/* Check if IO Memory is available for use by the driver */

	if(!(request_mem_region(epwm.r_mem->start, resource_size(epwm.r_mem),"WS2812B")))
	{
		pr_err ("%s, EHRPWM Memory  Used by Anoter Driver\n",__func__);
	
	}
	epwm.pwm_base = ioremap_nocache(epwm.r_mem->start, resource_size(epwm.r_mem));

//	epwm.virq = irq_create_mapping (NULL, PWM_IRQ);
//	printk( KERN_INFO "EPWM0 IRQ Mapped to %d\n", epwm.virq);	

	/* TBCTL Register */
	reg = 1;
	reg |= 3<<7;
	reg |= 3<<10; 
	iowrite16(reg, epwm.pwm_base + TBCTL);
	iowrite16(0XFFFF, epwm.pwm_base + TBCNT);
	iowrite16(0x00, epwm.pwm_base + CMPCTL); /* Default value */
	reg =0;
	iowrite16(0xFF00, epwm.pwm_base + CMPA);
	iowrite16(0xFF00, epwm.pwm_base +CMPB);
	
	/* Set Action Qualifier */
	reg =1;
	reg |= 3<<6;
	iowrite16(reg, epwm.pwm_base + AQCTLB);
		

	printk(KERN_INFO"All is good \n");

	return 0;
}

static int pwm_remove (struct platform_device *pdev) 
{

iounmap(epwm.pwm_base);
release_mem_region(epwm.r_mem->start, resource_size(epwm.r_mem));
printk(KERN_INFO"SK,PWM-DEV Remove");

return 0;
}

static const struct of_device_id pwm_match[]={
        {.compatible = "sk,pwm-dev", },
        {},
};

MODULE_DEVICE_TABLE(of, pwm_match);


struct platform_driver pwm_driver = {

	.probe = &pwm_probe,
	.remove = &pwm_remove,
	.driver = {
		.name = "sk,pwm-dev",
		.of_match_table = of_match_ptr(pwm_match),
	
	},

};


static int __init pwm_init(void) {

        int ret=0;
	printk(KERN_INFO"PWM_INIT"); 
        ret=platform_driver_register(&pwm_driver);
        return ret;
}


static void __exit pwm_exit(void) 
{
	printk(KERN_INFO"PWM_EXIT\n");
	platform_driver_unregister(&pwm_driver);

}


module_init(pwm_init);
module_exit(pwm_exit);
