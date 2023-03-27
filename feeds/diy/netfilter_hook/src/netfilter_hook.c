#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <linux/types.h>

static unsigned int forward_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
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
    
    printk("1--------------forward %pI4 -----> %pI4\n", &iph->saddr, &iph->daddr);
    return NF_ACCEPT;
}

static unsigned int forward_hook2(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
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
    
    printk("2--------------forward %pI4 -----> %pI4\n", &iph->saddr, &iph->daddr);
    return NF_ACCEPT;
}

static unsigned int local_in_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
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
    
    printk("3--------------local_in %pI4 -----> %pI4\n", &iph->saddr, &iph->daddr);
    return NF_ACCEPT;
}

static struct nf_hook_ops hook_ops[] __read_mostly = {
    {
        .hook = forward_hook,
        .pf = PF_INET,
        .hooknum = NF_INET_FORWARD,
        .priority = NF_IP_PRI_FIRST + 1,
    },
    {
        .hook = forward_hook2,
        .pf = PF_INET,
        .hooknum = NF_INET_FORWARD,
        .priority = NF_IP_PRI_FIRST + 2,
    },
    {
        .hook = local_in_hook,
        .pf = PF_INET,
        .hooknum = NF_INET_LOCAL_IN,
        .priority = NF_IP_PRI_FIRST + 1,
    }
};


static int __init netfilter_hook_init(void)
{
	int ret = 0;
    nf_register_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
    printk("#init netfilter hook.....ok\n");
	return ret;
}
module_init(netfilter_hook_init);

static void __exit netfilter_hook_exit(void)
{
	printk("#exit netfilter hook.....ok\n");
}
module_exit(netfilter_hook_exit);
MODULE_LICENSE("GPL");

