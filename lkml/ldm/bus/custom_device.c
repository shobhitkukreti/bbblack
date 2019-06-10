#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <../include/custom_bus.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("skukreti");
MODULE_DESCRIPTION(" Custom Bus Creation ");

#define to_custom_device(d) container_of(d, struct custom_device, dev);
#define to_custom_driver(d) container_of(d, struct custom_driver, driver);


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
	pr_info("%s, %s, %s\n", __func__, dev_name(dev), driver->name);
	return !strncmp(dev_name(dev), driver->name, strlen(driver->name));
}

static int custom_probe(struct device *dev){

	pr_info("Custom Bus Probe Called \n");
	/* Once matched, struct device should have an instance of 
	 * struct device_driver. That can be used to extract the custom_driver
	 * and call the probe function. 
	 */
	struct custom_driver *cdriver = to_custom_driver(dev->driver);
	struct custom_device *cdevice = to_custom_device(dev);
	/* Let's call the driver probe function from dev->driver->probe() */
	
	
	if (cdriver && cdriver->probe && cdriver->probe(cdevice)==0)
		pr_info(" Custom Driver Probe Successful \n");
	else
		pr_err("Custom Driver Probe Failure\n");
	return 0;
}


struct device custom_bus_parent = {
	.init_name = "custom0",
	.release = custom_bus_release,
};



struct bus_type custom_bus_type = {
        .name = "custom",
        .match = custom_match,
        .probe = custom_probe,
//      .hotplug = custom_hotplug,
};


EXPORT_SYMBOL(custom_bus_type);


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

static void custom_dev_release(struct device *dev){
	pr_info("Custom Device Release\n");
}

int custom_register_device(struct custom_device *dev)
{
	dev->dev.parent = &custom_bus_parent;
	dev->dev.bus = &custom_bus_type;
	dev->dev.release = custom_dev_release;
	pr_info("Register Custom Device %s", dev->name);
	/* set the name in the struct device or else it will cause kernel panic */
	dev_set_name(&dev->dev, dev->name);
	return device_register(&dev->dev);
}
EXPORT_SYMBOL(custom_register_device);

void custom_unregister_device(struct custom_device *dev) 
{
	pr_err("Unregister custom device %s\n", dev->name);
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
	.name = "Custom-Device1",
};


static int __init custom_bus_init(void) 
{
	int ret = 0;
	ret = bus_register(&custom_bus_type);
	if (ret < 0)
		goto err0;
	// register the parent device
	ret = device_register(&custom_bus_parent);
	if(ret <0){
		pr_err("Failed to Register Custom Bus Parent\n");
		goto err1;
	}

	ret = custom_register_device(&cus_dev);	
	if(ret < 0){
		pr_err("Failed to Create Custom Device\n");
		device_unregister(&custom_bus_parent);
		goto err1;
		
	}

	return 0;

err1:
	bus_unregister(&custom_bus_type);
err0:
	pr_err("Custom Insert Mod Error :%d\n", ret);
	return ret;
		
}

static void  custom_bus_exit(void){

	custom_unregister_device(&cus_dev);
	pr_info("Now Unregister Custom Bus\n");
	device_unregister(&custom_bus_parent);
	bus_unregister(&custom_bus_type);
}

module_init(custom_bus_init);
module_exit(custom_bus_exit);
