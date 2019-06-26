obj-m += fb_pl111.o
prefix?=.

KERNEL?=/lib/modules/$(shell uname -r)/build

.phony: clean

all:
	make -C ${KERNEL} M=${PWD} modules
clean:
	make -C ${KERNEL} M=${PWD} clean
