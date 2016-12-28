#include <linux/module.h>    // included for all kernel modules
#include <linux/kernel.h>    // included for KERN_INFO
#include <linux/init.h>      // included for __init and __exit macros
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/mm.h>


MODULE_LICENSE("GPL");
MODULE_AUTHOR("T-1000");
MODULE_DESCRIPTION("Char Dev Driver");

#define DEVICE_NAME "pci_mmap"


struct pci_mmap_struct {
	dev_t drv_num;
	struct cdev cdev_struct;
};

struct pci_mmap_struct pci_mmap;

struct class *class_desc;


int pci_open (struct inode *in, struct file *filp) {

printk("PCI Test Open Function\n");

return 0;
}


int pci_release ( struct inode *in, struct file *filep) {
printk("PCI TEST Release Function\n");


return 0;
}

void pci_mmap_open_fn ( struct vm_area_struct *vma )
{
	printk(KERN_INFO "PCI_VMA_OPEN, Virt %lx, Phy %lx\n",
		vma->vm_start, vma->vm_pgoff << PAGE_SHIFT);
}

void pci_mmap_close_fn ( struct vm_area_struct *vma )
{

}


struct vm_operations_struct pci_mmap_ops = 
{
	.open = NULL, //mmap_open,
	.close = NULL, // mmap_close,
	.fault = NULL, //mmap_fault,

};

int pci_mmap_fn(struct file *filp, struct vm_area_struct *vma) 
{
	vma->vm_ops = &pci_mmap_ops;
	vma->vm_flags |= VM_HUGEPAGE;
	vma->vm_private_data = filp->private_data;

return 0;
}


static struct file_operations fops = {

        .owner = THIS_MODULE,
        .open  = pci_open,
        .release = pci_release,
	.mmap	= pci_mmap_fn,
        /*  .read  = generic_read,
          .write = generic_write,
         */
	
};

static int __init char_init(void)
{
	
	class_desc = class_create(THIS_MODULE, DEVICE_NAME);
	if(alloc_chrdev_region(&pci_mmap.drv_num, 0, 1, DEVICE_NAME) < 0) 
	{
                printk(KERN_DEBUG "Cannot register device '%s'\n",DEVICE_NAME);
                return -1;
        }

	cdev_init(&(pci_mmap.cdev_struct), &fops);
	pci_mmap.cdev_struct.owner = THIS_MODULE;	

	if(cdev_add(&pci_mmap.cdev_struct,pci_mmap.drv_num,1)) {

                printk(KERN_ALERT "cdev_add failed '%s'\n",DEVICE_NAME);
                return 1;
        }

	device_create(class_desc, NULL, pci_mmap.drv_num, NULL, DEVICE_NAME);
	printk(KERN_INFO "PCI_MMAP_DRV INIT End\n");

	return 0;

}


static void __exit char_cleanup(void) 
{

	printk(KERN_INFO "Char Cleanup\n");
	cdev_del(&pci_mmap.cdev_struct);
	unregister_chrdev_region(pci_mmap.drv_num, 1);
	device_destroy(class_desc, pci_mmap.drv_num);
	class_destroy(class_desc);

}

module_init(char_init);
module_exit(char_cleanup);
