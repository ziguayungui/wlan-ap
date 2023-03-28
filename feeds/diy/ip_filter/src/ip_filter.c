#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <linux/types.h>

static unsigned int ip_filter_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct nf_conn *ct = (struct nf_conn *)skb->_nfct;
    struct udphdr *udph = NULL;
    struct tcphdr *tcph = NULL;
    struct iphdr *iph = NULL;

    u_int16_t sport = 0;
    u_int16_t dport = 0;
    char *black_ip_list[]={"192.168.56.1"};
    u_int32_t i = 0;
    char src_ip[32] = {0}; 
    char dst_ip[32] = {0}; 

    if(ct == NULL || !nf_ct_is_confirmed(ct))
        return NF_ACCEPT;
    iph = ip_hdr(skb);
    if (!iph)
        return NF_ACCEPT;
    
    if(IPPROTO_ICMP == iph->protocol)
    {
        for (i=0; black_ip_list[i] != NULL; i++)
        {
            sprintf(src_ip, "%pI4", &iph->saddr); 
            sprintf(dst_ip, "%pI4", &iph->daddr);
            if (0==strcmp(black_ip_list[i], src_ip) ||
                0==strcmp(black_ip_list[i], dst_ip))
            {
                printk("Drop ip packet %pI4--->%pI4\n", &iph->saddr, &iph->daddr);
                return NF_DROP;
            }
        }
    }
    
    return NF_ACCEPT;
}

static struct nf_hook_ops hook_ops[] __read_mostly = {
    {
        .hook = ip_filter_hook,
        .pf = PF_INET,
        .hooknum = NF_INET_LOCAL_IN,
        .priority = NF_IP_PRI_FIRST + 1,
    }
};

static int __init ip_filter_init(void)
{
	int ret = 0;
    nf_register_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
    printk("#init ip filter.....ok\n");
	return ret;
}
module_init(ip_filter_init);

static void __exit ip_filter_exit(void)
{
    nf_unregister_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
	printk("#exit ip filter.....ok\n");
}
module_exit(ip_filter_exit);
MODULE_LICENSE("GPL");

