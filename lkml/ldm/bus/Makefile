MODULE=custom_device.o

obj-m += ${MODULE}
${MODULE}-objs = custom_module.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
