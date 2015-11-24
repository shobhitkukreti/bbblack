#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/sched.h>
#include<linux/fs.h>
#include<linux/device.h>
#include<linux/platform_device.h>
#include <linux/of.h>
#include <asm/io.h>
#include <linux/ioport.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DeathFire");
MODULE_DESCRIPTION("ARCH TIMER");

#define DEVICE_NAME "ARM DMTIMER"

#if 1

struct dmtimer {
//struct platform_device pdev;
//struct cdev cdev;
unsigned long *base_addr;
unsigned int addr_len;

} *dmtimer_drv;

#endif

static int dmtimer_probe (struct platform_device *);
static int dmtimer_remove (struct platform_device *); 


static const struct of_device_id dmtimer_match[]={
	{.compatible = "sk,timer-dev", },
	{},
};

MODULE_DEVICE_TABLE(of, dmtimer_match);

static struct platform_driver dmtimer = {
	.probe = dmtimer_probe,
	.remove = dmtimer_remove,
	.driver = {
			.name = DEVICE_NAME,
			.of_match_table = of_match_ptr(dmtimer_match),
		},
};

static int __init dmtimer_init(void) {

int ret=0;
ret=platform_driver_register(&dmtimer);
return ret;
}

static void __exit dmtimer_exit(void) {
platform_driver_unregister(&dmtimer);
}

static int dmtimer_probe(struct platform_device *pdev) {

unsigned long base_addr;
printk(KERN_ALERT "DMTIMER Probe\n");


base_addr = 0x4804a000;
dmtimer_drv->addr_len = (unsigned int) 0x400;

#if 0

if((request_mem_region(base_addr, dmtimer_drv->addr_len, DEVICE_NAME)) == NULL) {
	printk(KERN_ALERT "DMTIMER Memory region already occupied\n");
	return -EBUSY;
}
else 
	printk(KERN_ALERT "DMIMER 7 Unused\n");
#endif

dmtimer_drv->base_addr = (unsigned long *) ioremap(base_addr, 0x400);
printk(KERN_INFO "DMTIMER Remapped REG %lu\n", (unsigned long *)dmtimer_drv->base_addr);

printk(KERN_INFO "Reading REG TIDR %u\n", ioread32(dmtimer_drv->base_addr));

return 0;
}

static int dmtimer_remove (struct platform_device *pdev) {


printk(KERN_INFO "Removed DMTIMER");
iounmap((unsigned int *)dmtimer_drv->base_addr);
release_mem_region(0x4804a000, 0x400);

return 0;
}

module_init(dmtimer_init);
module_exit(dmtimer_exit);
