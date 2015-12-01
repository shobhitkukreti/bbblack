#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/sched.h>
#include<linux/fs.h>
#include<linux/device.h>
#include<linux/platform_device.h>
#include <linux/of.h>
#include <asm/io.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DeathFire");
MODULE_DESCRIPTION("DMTIMER Driver");

#define DEVICE_NAME "ARM DMTIMER"
#define TINT7 95
#define CM_PERS_REGBASE 0x44E00000
#define CM_PERS_REGLEN 0x400
#define DMTIMER7_REGBASE 0x4804A000
#define DMTIMER7_REGLEN 0x400

#define DEBUG 0


/* Struct Definition */
struct dmtimer {

struct platform_device *pdev;
void __iomem * enable_base_addr;
void __iomem *timer7_base;
unsigned int virq;
struct task_struct * my_thread;
} dmtimer_drv;


/* Driver specific functions */
static int dmtimer_probe (struct platform_device *);
static int dmtimer_remove (struct platform_device *); 
static int dmtimer_irq_handler (unsigned int irq, void * dev_id, struct pt_regs *reg);

/** Thread function prototype **/

static int read_timer (void * data);



/* compatible string much match with dts file src code */
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


/* Module Init functions 
 * Registers the platform driver
 */

static int __init dmtimer_init(void) {

	int ret=0;
	ret=platform_driver_register(&dmtimer);
	return ret;
}


/* Module cleanup function */
static void __exit dmtimer_exit(void) {

	if(dmtimer_drv.my_thread) 
		kthread_stop(dmtimer_drv.my_thread);
	free_irq(dmtimer_drv.virq, NULL);
	platform_driver_unregister(&dmtimer);
}



/* Probe functions, remaps io memory, sets up timer and irq handler */

static int dmtimer_probe(struct platform_device *pdev) {

	struct resource *r_mem;
	int ret=0;

	/* Get Memory Resource as definied in DTS file for Timer7 */
	r_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dmtimer_drv.virq = irq_of_parse_and_map(pdev->dev.of_node,0);
	if(!dmtimer_drv.virq) {
		return -1;
	}
		

	#ifdef DEBUG
	pr_info("IRQ NUMBER: %u\n", dmtimer_drv.virq);
	#endif

	if(!r_mem) {
		printk(KERN_ALERT "Cannot find DMTIMER Base address\n");
		return -1;
	}

	#ifdef DEBUG
	pr_info( "Start: %x END:%x , len:%x \n", (u32)r_mem->start, (u32)r_mem->end, \
	(u32)r_mem->end-r_mem->start +1);	
	#endif

	
	/* Confirm if IO Memory is available before remapping to virtual space*/

	if((request_mem_region(CM_PERS_REGBASE, CM_PERS_REGLEN , "CM_PERIPHERAL")) == NULL) {
		printk(KERN_ERR "CM PER Memory region already occupied\n");
		return -EBUSY;
	}

	if((request_mem_region(r_mem->start, r_mem->end - r_mem->start + 1 , DEVICE_NAME)) == NULL) {
		printk(KERN_ERR "DMTIMER Memory region already occupied\n");
		return -EBUSY;
	}


	/* Physical IO MEM to Virtual IO MEM. Usually an offset */
	dmtimer_drv.enable_base_addr = ioremap_nocache(CM_PERS_REGBASE, CM_PERS_REGLEN);
	dmtimer_drv.timer7_base = ioremap_nocache(DMTIMER7_REGBASE, DMTIMER7_REGLEN);
	
	#ifdef DEBUG
	pr_info("VMEM 1 : %x\n", (unsigned int)dmtimer_drv.timer7_base);
	pr_info("VMEM 2 : %x\n", (unsigned int)dmtimer_drv.enable_base_addr);
	#endif
	
	ret = request_irq(dmtimer_drv.virq, (irq_handler_t)dmtimer_irq_handler,IRQF_TIMER, "DMTIMER7_IRQ_HANDLER", NULL);	

	
	iowrite32(30002, (dmtimer_drv.enable_base_addr + 0x7c)); //enable timer 7 block
	iowrite32(0x02, dmtimer_drv.timer7_base + 0x2c); // enable timer 7 over flow intrpt
	iowrite32(0xFE91C9FF, dmtimer_drv.timer7_base + 0x3c); // write init value to tclr reg
	iowrite32(0xFE91C9FF, dmtimer_drv.timer7_base + 0x40); // put auto reload value 0x00 in TLDR reg
	iowrite32(0x03, dmtimer_drv.timer7_base + 0x38); // auto reload mode, enable timer 7
	

	dmtimer_drv.pdev = pdev;

	/* Optional kthread to read reg status */
	#if 0
	dmtimer_drv.my_thread = kthread_run( read_timer, (void *)NULL, "TIMER7-Read");
	#endif
	return 0;
}


/* DMTIMER Remove Function */
static int dmtimer_remove (struct platform_device *pdev) {


	printk(KERN_INFO "Removed DMTIMER");
	iowrite32(0x0, dmtimer_drv.timer7_base + 0x7c);
	iowrite32(0x00, dmtimer_drv.timer7_base + 0x2c);
	iounmap(dmtimer_drv.enable_base_addr);
	iounmap(dmtimer_drv.timer7_base);
	release_mem_region(CM_PERS_REGBASE, CM_PERS_REGLEN);
	release_mem_region(DMTIMER7_REGBASE, DMTIMER7_REGLEN);

	pr_info("DMTIMER 7 Addr Release %x\n", (unsigned int)dmtimer_drv.timer7_base);

	return 0;
}


/* DMTIMER_IRQ_HANDLER */

static int dmtimer_irq_handler (unsigned int irq, void *dev_id, struct pt_regs *regs) {

	iowrite32(0x02, dmtimer_drv.timer7_base + 0x30); // disable timer 7 interrupt
	iowrite32(0x02, dmtimer_drv.timer7_base + 0x28); // clear timer 7 int pending bit by writing 1
	iowrite32(0x02, dmtimer_drv.timer7_base + 0x2c); // enable timer 7 over flow intrpt

	/* Really bad idea to print in IRQ Handler, but this just a demo */
	pr_info("T7 - 1sec  \n");

	return (irq_handler_t) IRQ_HANDLED;

}



/* Kernel Thread Function Implementation to Read Timer value */

static int read_timer (void * data) {


	while(dmtimer_drv.enable_base_addr && dmtimer_drv.timer7_base) {
		if(kthread_should_stop()) 
			break;

		pr_alert("TCLR:%x\n", ioread32(dmtimer_drv.timer7_base +0x38));
		pr_alert("TCRR:%x\n", ioread32(dmtimer_drv.timer7_base +0x3C));
		pr_alert("TINT7-EN:%x\n", ioread32(dmtimer_drv.timer7_base +0x2c));
		pr_alert("TINT7-CLR:%x\n", ioread32(dmtimer_drv.timer7_base +0x30));
		pr_alert("TLDR:%x\n", ioread32(dmtimer_drv.timer7_base +0x40));
		pr_alert("TINT STAT:%x\n", ioread32(dmtimer_drv.timer7_base +0x28));

		msleep(1000);
	}

	return 0;
}


module_init(dmtimer_init);
module_exit(dmtimer_exit);
