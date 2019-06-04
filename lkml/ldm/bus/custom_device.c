#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/init.h>
#include "../include/custom_bus.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("skukreti");
MODULE_DESCRIPTION(" Custom Bus Creation ");

//#define to_custom_driver(d) container_of(d, struct packt_driver, driver);

/**
* @brief 
*/

/*
struct custom_device {
	struct module *owner;
	unsigned char name[30];
	unsigned long price;
	struct device dev;
};


struct custom_driver {
	int (*probe) (struct custom_device *dev);
	int (*remove) (struct custom_device *dev);
	void (*shutdown) (struct custom_device *dev);
	struct device_driver driver;
};
*/

static void custom_bus_release(struct device *dev)
{
	pr_info("custom bus parent release\n");

}

static int custom_hotplug(struct device *dev, char**envp, int num_envp, char *buffer, int buffer_size)
{
	envp[0] = buffer;
	if(snprintf(buffer, buffer_size, "CUSTOM_BUS_VERSION=%d", 1234) >=buffer_size)
		return -ENOMEM;
	envp[1] = NULL;
	return 0;
}

static int custom_match(struct device *dev, struct device_driver *driver)
{
	return !strncmp(dev_name(dev), driver->name, strlen(driver->name));
}

static int custom_probe(struct device *dev){

	pr_info("Custom Bus Probe Called \n");
	return 0;
}

struct bus_type custom_bus_type = {
	.name = "custom",
	.match = custom_match,
	.probe = custom_probe,
//	.hotplug = custom_hotplug,
};
EXPORT_SYMBOL(custom_bus_type);

struct device custom_bus_parent = {
	.init_name = "custom0",
	.release = custom_bus_release,
};

int custom_register_driver(struct custom_driver *driver) 
{
	driver->driver.bus = &custom_bus_type;
	return driver_register(&driver->driver);

}
EXPORT_SYMBOL(custom_register_driver);

int custom_unregister_driver(struct custom_driver *driver) 
{

	driver_unregister(&driver->driver);
	return 0;
}
EXPORT_SYMBOL(custom_unregister_driver);

int custom_register_device(struct custom_device *dev)
{
	dev->dev.parent = &custom_bus_parent;
	dev->dev.bus = &custom_bus_type;
	return device_register(&dev->dev);
}
EXPORT_SYMBOL(custom_register_device);

void custom_unregister_device(struct custom_device *dev) 
{
	device_unregister(&dev->dev);
}
EXPORT_SYMBOL(custom_unregister_device);

struct custom_device * custom_device_alloc(const char *name, int id) 
{
	struct custom_device *dev;
	int status;
	dev = kzalloc(sizeof (*dev), GFP_KERNEL);
	if(!dev)
		return NULL;
	strcpy(dev->name, name);
	dev->dev.id = id;
	dev_dbg(&dev->dev, "Device [%s] registered with custom bus \n", dev->name);

	return dev;

}
EXPORT_SYMBOL(custom_device_alloc);

static struct custom_device cus_dev = {
	.name = "Custom - Device",
};


static int __init custom_bus_init(void) 
{
	int ret = 0;
	ret = bus_register(&custom_bus_type);
	if (ret < 0)
		goto err0;

//	ret = class_register(&custom_master_class);
//	if (ret < 0)
//		goto err1;
	
	// register the parent device
	device_register(&custom_bus_parent);

	custom_register_device(&cus_dev);	
	return 0;

err1:
	bus_unregister(&custom_bus_type);
err0:
	return ret;
		
}

static void  custom_bus_exit(void){

	custom_unregister_device(&cus_dev);
	device_unregister(&custom_bus_parent);
	bus_unregister(&custom_bus_type);
}

module_init(custom_bus_init);
module_exit(custom_bus_exit);
