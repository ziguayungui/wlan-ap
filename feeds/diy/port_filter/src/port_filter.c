#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <linux/types.h>

static unsigned int port_filter_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
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
    
    switch(iph->protocol)
    {
        case IPPROTO_UDP:
            udph=udp_hdr(skb);
            dport=htons(udph->dest);
            sport=htons(udph->source);
            break;
        case IPPROTO_TCP:
            tcph=tcp_hdr(skb);
            dport=htons(tcph->dest);
            sport=htons(tcph->source);
            break;
        default:
            return NF_ACCEPT;
    }

    printk("flow %pI4:%d -----> %pI4:%d\n", &iph->saddr, sport, &iph->daddr, dport);
    if(IPPROTO_TCP == iph->protocol && (443 == dport || 80 == dport))
    {
        printk("Drop tcp 80 or 443 packet %pI4:%d--->%pI4:%d\n", &iph->saddr, sport, &iph->daddr, dport);
        return NF_DROP;
    }
    
    if(IPPROTO_UDP == iph->protocol && (53 == dport))
    {
        printk("Drop dns packet %pI4:%d--->%pI4:%d\n", &iph->saddr, sport, &iph->daddr, dport);
        return NF_DROP;
    }
    return NF_ACCEPT;
}

static struct nf_hook_ops hook_ops[] __read_mostly = {
    {
        .hook = port_filter_hook,
        .pf = PF_INET,
        .hooknum = NF_INET_FORWARD,
        .priority = NF_IP_PRI_FIRST + 1,
    }
};

static int __init port_filter_init(void)
{
	int ret = 0;
    nf_register_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
    printk("#init port filter.....ok\n");
	return ret;
}
module_init(port_filter_init);

static void __exit port_filter_exit(void)
{
    nf_unregister_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
	printk("#exit port filter.....ok\n");
}
module_exit(port_filter_exit);
MODULE_LICENSE("GPL");

