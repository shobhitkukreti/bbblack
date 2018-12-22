CROSS_COMPILE?=aarch64-linux-gnu-
prefix?=.
all: grabber.o
	${CROSS_COMPILE}gcc grabber.c -o grabber
	sudo mv grabber ${prefix}/frame

.PHONY: clean
clean:
	rm *.o grabber
