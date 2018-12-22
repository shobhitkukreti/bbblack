obj-m += vid4linux.o
prefix?=.

KERNEL?=/lib/modules/$(shell uname -r)/build

all:
	make -C ${KERNEL} M=${PWD} modules
	sudo mv *.ko $(prefix)/v4l2-sk.ko
clean:
	make -C ${KERNEL} M=${PWD} clean
