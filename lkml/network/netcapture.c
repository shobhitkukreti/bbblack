#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/skbuff.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("S.K.");
MODULE_DESCRIPTION("Capture Network Packets on Interface");

struct packet_type network_packet;

int packet_rcv(struct sk_buff *skb, struct net_device *dev,
	struct packet_type *pt, struct net_device *orig_dev)
{
	pr_info("@DeathFire : New packet captured.\n");

/* linux/if_packet.h : Packet types */
 // #define PACKET_HOST 0 /* To us */
 // #define PACKET_BROADCAST 1 /* To all */
 // #define PACKET_MULTICAST 2 /* To group */
 // #define PACKET_OTHERHOST 3 /* To someone else */
 // #define PACKET_OUTGOING 4 /* Outgoing of any type */
 // #define PACKET_LOOPBACK 5 /* MC/BRD frame looped back */
 // #define PACKET_USER 6 /* To user space */
 // #define PACKET_KERNEL 7 /* To kernel space */
 /* Unused, PACKET_FASTROUTE and PACKET_LOOPBACK are invisible to user space */
 // #define PACKET_FASTROUTE 6 /* Fastrouted frame */



switch (skb->pkt_type) {
case PACKET_HOST:
	pr_info("@DeathFire PACKET to us\n ");
	break;
case PACKET_BROADCAST:
	pr_info("@DeathFire : PACKET to all\n");
	 break;
case PACKET_MULTICAST:
	pr_info("@DeathFire : PACKET to group\n");
	break;
case PACKET_OTHERHOST:
	pr_info("@DeathFire : PACKET to someone else\n");
	break;
case PACKET_OUTGOING:
	pr_info("@DeathFire : PACKET outgoing\n");
	break;
case PACKET_LOOPBACK:
	pr_info("@DeathFire : PACKET LOOPBACK\n");
	break;
case PACKET_FASTROUTE:
	pr_info("@DeathFire: PACKET FASTROUTE\n");
	break;
}

pr_info("Dev: %s ; 0x%.4X ; 0x%.4X\n", skb->dev->name,
		  ntohs(skb->protocol), ip_hdr(skb)->protocol);
pr_info("@SK : VLAN ID :%u\n", ntohs((skb->vlan_tci))&0XFFF);

	kfree_skb(skb);
	return 0;
}

static int __init net_init(void)
{
 /* See the <linux/if_ether.h>
 When protocol is set to htons(ETH_P_ALL), then all protocols are received.
 All incoming packets of that protocol type will be passed to the packet
 socket before they are passed to the protocols implemented in the kernel. */
 /* Few examples */
 //ETH_P_LOOP 0x0060 /* Ethernet Loopback packet */
 //ETH_P_IP 0x0800 /* Internet Protocol packet */
 //ETH_P_ARP 0x0806 /* Address Resolution packet */
 //ETH_P_LOOPBACK 0x9000 /* Ethernet loopback packet, per IEEE 802.3 */
 //ETH_P_ALL 0x0003 /* Every packet (be careful!!!) */
 //ETH_P_802_2 0x0004 /* 802.2 frames */
 //ETH_P_SNAP 0x0005 /* Internal only */

	network_packet.type = htons(ETH_P_IP);

/* NULL is a wildcard */
 //network_packet.dev = NULL;
	network_packet.dev = dev_get_by_name(&init_net,
			"enx0050b6216fee");
	network_packet.func = packet_rcv;

/* Packet sockets are used to receive or send raw packets at the device
 driver (OSI Layer 2) level. They allow the user to implement
 protocol modules in user space on top of the physical layer. */

/* Add a protocol handler to the networking stack.
 The passed packet_type is linked into kernel lists and may not be freed until
  it has been removed from the kernel lists. */
	dev_add_pack(&network_packet);

	pr_info("Module insertion successfully!\n");
	return 0; // Non-zero return means that the module couldn't be loaded.
}

static void __exit net_cleanup(void)
{
	dev_remove_pack(&network_packet);
	pr_info("Cleaning up module..\n");
}

module_init(net_init);
module_exit(net_cleanup);
