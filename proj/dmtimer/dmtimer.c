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
void __iomem * enable_base_addr;
void __iomem *timer7_base;
u32 addr_len;
u32 start,end;

} dmtimer_drv;

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
	struct resource *r_mem;
	void __iomem * io = NULL;
	struct device *dev = &pdev->dev;

	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if(!r_mem) {
		printk(KERN_ALERT "Cannot find DMTIMER Base address\n");
		return -1;
	}
	dmtimer_drv.start = 0x44E00000;
	dmtimer_drv.end =   0X44E003FF;

	pr_emerg( "Start: %x END:%x , len:%x \n", (u32)r_mem->start, (u32)r_mem->end, \
	(u32)r_mem->end-r_mem->start +1);	

	if((request_mem_region(0x44E00000, 0x400 , "CM_PERIPHERAL")) == NULL) {
		printk(KERN_ERR "CM PER Memory region already occupied\n");
		return -EBUSY;
	}
	else 
		printk(KERN_ALERT "CM PER Memory Region available\n");

	if((request_mem_region(0x4804A000, 0x400 , DEVICE_NAME)) == NULL) {
		printk(KERN_ERR "DMTIMER Memory region already occupied\n");
		return -EBUSY;
	}
	else 
		printk(KERN_ALERT "DMIMER Memory Region available\n");


	io = ioremap_nocache(0x44E00000, 0x400);
	dmtimer_drv.timer7_base = ioremap_nocache(0x4804A000, 0x400);
	pr_emerg("VMEM 1 : %x\n", dmtimer_drv.timer7_base);
	pr_emerg("VMEM 2 : %x\n", io);

	dmtimer_drv.enable_base_addr=io;
	iowrite32(30002, (io + 0x7c));

	if(!io) {
		pr_emerg("IOREMAP Failed \n");
		return -1;
	}
	else 		
		pr_emerg("CM_PER REF %x\n", ioread32(io + 0x7c));
	
	pr_alert("Reading timer 7 counter reg \n");
	pr_emerg("Timer 7 Counter Reg : %x", ioread32(dmtimer_drv.timer7_base));
		

	return 0;
}


/* DMTIMER Remove Function */
static int dmtimer_remove (struct platform_device *pdev) {


printk(KERN_INFO "Removed DMTIMER");
iounmap(dmtimer_drv.enable_base_addr);
iounmap(dmtimer_drv.timer7_base);
release_mem_region(0x44E00000, 0x400);
release_mem_region(0x4804A000, 0x400);
pr_alert("Base Addr Release %x\n", dmtimer_drv.timer7_base);

return 0;
}

module_init(dmtimer_init);
module_exit(dmtimer_exit);
