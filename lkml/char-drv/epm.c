#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/spi/spi.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("DeathFire");
MODULE_DESCRIPTION("Char Dev Driver");

#define DEVICE_NAME "epm"


int generic_open (struct inode *in , struct file *filp);
int generic_release (struct inode *in, struct file *filp);
int epm_probe ( struct spi_device *);
int epm_remove ( struct spi_device *);


struct epm {
	char name[16];
	struct cdev dev;
	struct spi_device;
	struct platform_device;
	uint32_t bus_id;

} *epm_drv;


static struct file_operations fops = {

	.owner = THIS_MODULE,
	.open  = generic_open,
	.release = generic_release,
	/*  .read  = generic_read,
	  .write = generic_write,
	 */
};


dev_t epm_number;
struct class *epm_class;



int epm_cdev_init(void) {
	
	/* Requesting major number */

	if(alloc_chrdev_region(&epm_number, 0, 1, DEVICE_NAME) < 0) {
		printk(KERN_DEBUG "Cannot register device '%s'\n",DEVICE_NAME);
		return -1;
	}

	/* Populate SYSFS Entry , appears under /sys/class*/

	epm_class = class_create (THIS_MODULE, DEVICE_NAME);
	epm_drv = (struct epm *) kmalloc(sizeof(struct epm), GFP_KERNEL);
	if (epm_drv==NULL) {
		printk(KERN_ALERT "Kmalloc failed for  '%s'\n",DEVICE_NAME);
		return -1;
	}
	
	/* cdev_init Connects file node with file operations */
	cdev_init(&(epm_drv->dev), &fops);

	epm_drv->dev.owner = THIS_MODULE;

  	/* Driver is live and user accessible now */
	if(cdev_add(&epm_drv->dev,epm_number,1)) {
		printk(KERN_ALERT "cdev_add failed '%s'\n",DEVICE_NAME);
		return 1;
	}

	/* Device Creates the file node under /dev/node, but require a class, therefore class_create is called first */
	device_create(epm_class, NULL, epm_number, NULL, "epm");

	printk(KERN_INFO "Registered EPM Test  Driver\n"); 
	return 0;    // Non-zero return means that the module couldn't be loaded.

}


static const struct of_device_id epm_of_match_table[] = {
        {       .compatible = "cy,epm-adc-cy8c5568lti-114",
	},                                                             
        {}                                                             
};                                                                                                                                                                                    
                                                                                                                                                                                     
static struct spi_driver epm_driver = {                                                                                                                                .probe = epm_probe,
       .remove =epm_remove,
       .driver = {                                                                     .name = "EPM ADC DRV",                                                           .of_match_table = epm_of_match_table,                                           },                                                                                                                                                                      
};                                           




static int __init epm_init(void)
{
int ret=0;

ret = epm_cdev_init();
if(ret==-1) 
	goto exit_func;

ret = 0;
ret = spi_register_driver(&epm_driver);
if (ret)
	pr_err("EPM SPI Register Failed\n");

exit_func: return ret;

}





static void __exit epm_exit(void)
{
	cdev_del(&epm_drv->dev);
	unregister_chrdev_region(epm_number, 1);	
	device_destroy(epm_class, epm_number);
	class_destroy(epm_class);
	printk(KERN_ALERT "Unregistered HTC EPM TestDriver \n");

return;
}


/* SPI Probe driver Function */
int epm_probe(struct spi_device *spi_dev ) {

return 0;
}


/*SPI Remove driver Function */
int epm_remove(struct spi_device *spi_dev ) {

return 0;
}



int generic_open (struct inode *in, struct file *filp) {

printk("HTC EPM Test Open Function\n");

return 0;
}


int generic_release ( struct inode *in, struct file *filep) {
printk("HTC EPM TEST Release Function\n");


return 0;
}


module_init(epm_init);
module_exit(epm_exit);
