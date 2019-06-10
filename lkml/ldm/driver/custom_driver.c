#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <custom_bus.h>


MODULE_LICENSE("GPL");

extern struct bus_type custom_bus_type;
static int custom_register_driver(struct custom_driver *driver) 
{
	driver->driver.bus = &custom_bus_type;
	driver->driver.name = (const char *)(&driver->id);
	return driver_register(&driver->driver);

}

static int custom_unregister_driver(struct custom_driver *driver) 
{
	pr_info(" Custom Unregister Driver\n");
        driver_unregister(&driver->driver);
        return 0;
}


static int custom_probe (struct custom_device *dev)
{
	pr_info("Custom driver probe func:: %s\n", dev->name);
	return 0;

}
static int custom_remove (struct custom_device *dev)
{
	pr_info("Custom driver remove\n");
	return 0;	
}
static void custom_shutdown (struct custom_device *dev)
{	
	pr_info("Custom driver shutdown\n");
}

static struct custom_driver driver = {
	.id = "Custom-Device1",
	.probe = custom_probe,
	.remove = custom_remove,
	.shutdown = custom_shutdown,
};

static __init int custom_driver_init(void)
{
	int ret = 0;
	ret = custom_register_driver(&driver);
	pr_info("Registered Custom Driver : %d\n", ret);
	return ret;
}

static __exit void custom_driver_exit(void)
{
	custom_unregister_driver(&driver);
}

module_init(custom_driver_init);
module_exit(custom_driver_exit);
