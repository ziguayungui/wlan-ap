#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <linux/types.h>

static unsigned int icmp_filter_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct nf_conn *ct = (struct nf_conn *)skb->_nfct;
    struct udphdr *udph = NULL;
    struct tcphdr *tcph = NULL;
    struct iphdr *iph = NULL;

    u_int16_t sport = 0;
    u_int16_t dport = 0;

    if(ct == NULL || !nf_ct_is_confirmed(ct))
        return NF_ACCEPT;
    iph = ip_hdr(skb);
    if (!iph)
        return NF_ACCEPT;
    
    if(IPPROTO_ICMP == iph->protocol)
    {
        printk("Drop icmp packet %pI4--->%pI4\n", &iph->saddr, &iph->daddr);
        return NF_DROP;
    }
    
    return NF_ACCEPT;
}

static struct nf_hook_ops hook_ops[] __read_mostly = {
    {
        .hook = icmp_filter_hook,
        .pf = PF_INET,
        .hooknum = NF_INET_LOCAL_IN,
        .priority = NF_IP_PRI_FIRST + 1,
    }
};

static int __init icmp_filter_init(void)
{
	int ret = 0;
    nf_register_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
    printk("#init icmp filter.....ok\n");
	return ret;
}
module_init(icmp_filter_init);

static void __exit icmp_filter_exit(void)
{
    nf_unregister_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
	printk("#exit icmp filter.....ok\n");
}
module_exit(icmp_filter_exit);
MODULE_LICENSE("GPL");

