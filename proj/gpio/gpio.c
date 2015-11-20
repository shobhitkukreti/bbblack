#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("SKUK");


static unsigned button = 115;
//static unsigned btn_press = 0;
static unsigned int irq_number;

static irq_handler_t gpio_handler(unsigned int irq, void *dev_id, struct pt_regs *reg);

static int gpio_init(void) {

int ret=0;

ret = gpio_request(button, "sysfs");

gpio_direction_input(button);
gpio_export(button, false);
gpio_set_debounce (button, 200);

printk(KERN_INFO "GPIO Rquesr: %d\n", ret);

irq_number = gpio_to_irq(button);

printk(KERN_INFO "Alloted GPIO IRQ :%u\n", irq_number);

return 0;
}


static void gpio_exit(void) {

gpio_unexport(button);
gpio_free(button);
free_irq(irq_number,NULL);
printk(KERN_INFO "Exit GPIO Button Demo \n");

}


static irq_handler_t gpio_handler(unsigned int irq, void *dev_id, struct pt_regs *reg) {

return (irq_handler_t) IRQ_HANDLED;
}


module_init(gpio_init);
module_exit(gpio_exit);

