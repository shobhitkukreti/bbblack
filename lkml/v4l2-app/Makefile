CROSS_COMPILE?=aarch64-linux-gnu-

all: grabber.o
	${CROSS_COMPILE}gcc grabber.c -o grabber

.PHONY: clean
clean:
	rm *.o grabber
