#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>

#include "../include/custom_bus.h"

MODULE_LICENSE("GPL");

static struct custom_device dev = {
	.name = "Custom - Device",
};

static __init int custom_device_init(void)
{
	int ret = 0;
	ret = custom_device_register(&dev);
	pr_info("Registered custom device : %d\n", ret);
	return ret;
}

static __exit void custom_device_exit(void)
{
	custom_unregister_device(&dev);
}
