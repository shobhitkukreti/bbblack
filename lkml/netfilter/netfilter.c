/* Linux Kernel NetFilter */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/sysfs.h>
#include <linux/fs.h>

/* netfilter function hooks */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DeathFire");
MODULE_DESCRIPTION("Netfilter IP Packets");

static unsigned int ret = NF_ACCEPT;

typedef struct nf_hook_ops network_hook;
/* RX Packet Hooks, TX Packet Hook */
static network_hook inp_hook, outp_hook;

/* kobj for directory */
static struct kobject *dkobj;

static ssize_t nf_show(struct kobject *kobj, struct kobj_attribute *attr,
                      char *buf)
{
        return sprintf(buf, "0: Allow Comm, 1: Disable Comm\n");
}

static ssize_t nf_store(struct kobject *kobj, struct kobj_attribute *attr,
                      char *buf, size_t count)
{
	int rt =0;
        sscanf(buf, "%u", &rt);
	switch(rt) {
	case 0: ret = NF_ACCEPT; break;
	case 1: ret = NF_DROP; break;
	}
        return count;
}

/* Attribute for files with permissions and show/store functions */
static struct kobj_attribute nf_attribute =__ATTR(nf_info, 0660, nf_show,
                                                   nf_store);

/* RX and TX Hooks Into the packet path */
unsigned int rx_hook_func(void *priv, struct sk_buff *skb, const struct nf_hook_state *state){

	printk(KERN_INFO "Incoming Packet on Inf: %s\n", state->in->name);
	return ret;
}

unsigned int tx_hook_func(void *priv, struct sk_buff *skb, const struct nf_hook_state *state){

	printk(KERN_INFO "Outgoing Packet on Inf: %s\n", state->out->name);
	return ret;
}

static int __init nf_module(void){

	// second arg NULL specifies parent directory is sys"
	dkobj = kobject_create_and_add("netfilter-ex", NULL);
	if(!dkobj){
		pr_err("Cannot Create dkobj\n");
	return -ENOMEM;
	}

	if(sysfs_create_file(dkobj, &nf_attribute.attr))
	{
		pr_err("Sysfile not created\n");
		kobject_put(dkobj);	
	} 

	inp_hook.hook 		= rx_hook_func;
	inp_hook.pf		= PF_INET;
	inp_hook.hooknum	= 0; //NF_IP_PRE_ROUTING;		
	inp_hook.priority	= NF_IP_PRI_FIRST;
	outp_hook.hook 		= tx_hook_func;
	outp_hook.pf		= PF_INET;
	outp_hook.hooknum	= 4; //NF_IP_POST_ROUTING;		
	outp_hook.priority	= NF_IP_PRI_FIRST;

	nf_register_hook(&inp_hook);
	nf_register_hook(&outp_hook);
	return 0;

}

static void __exit remove_hooks(void)
{
	kobject_put(dkobj);
	nf_unregister_hook(&inp_hook);
	nf_unregister_hook(&outp_hook);

}

module_init(nf_module);
module_exit(remove_hooks);
