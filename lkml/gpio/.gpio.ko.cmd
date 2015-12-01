cmd_proj/gpio/gpio.ko := arm-eabi-ld -EL -r  -T ./scripts/module-common.lds --build-id  -o proj/gpio/gpio.ko proj/gpio/gpio.o proj/gpio/gpio.mod.o
