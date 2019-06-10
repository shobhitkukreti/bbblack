#ifndef CUSTOM_BUS_H_
#define CUSTOM_BUS_H_

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
	char id[30];
};


//extern int custom_register_driver (struct custom_driver *driver);
//extern int custom_unregister_driver (struct custom_driver *driver);
//extern int custom_register_device (struct custom_device *dev);
//extern int custom_unregister_device (struct custom_device *dev);
#endif
