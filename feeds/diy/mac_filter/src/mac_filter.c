#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <linux/types.h>

static unsigned int mac_filter_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct nf_conn *ct = (struct nf_conn *)skb->_nfct;
    struct udphdr *udph = NULL;
    struct tcphdr *tcph = NULL;
    struct iphdr *iph = NULL;
    struct ethhdr *ethhdr = NULL;

    u_int16_t sport = 0;
    u_int16_t dport = 0;
    char *black_mac_list[]={"0A:00:27:00:00:16"};
    u_int32_t i = 0;
    u_int8_t smac_buf[32]={0};
    u_int8_t dmac_buf[32]={0};

    if(ct == NULL || !nf_ct_is_confirmed(ct))
        return NF_ACCEPT;
    iph = ip_hdr(skb);
    if (!iph)
        return NF_ACCEPT;
    ethhdr = eth_hdr(skb);
    if (!ethhdr)
        return NF_ACCEPT;
    if (!skb->dev)
        return NF_ACCEPT;
    if(IPPROTO_ICMP == iph->protocol)
    {
        for (i=0; black_mac_list[i] != NULL; i++)
        {
            sprintf(smac_buf,"%02X:%02X:%02X:%02X:%02X:%02X",
                ethhdr->h_source[0],ethhdr->h_source[1],ethhdr->h_source[2],ethhdr->h_source[3],ethhdr->h_source[4],ethhdr->h_source[5]);
            sprintf(dmac_buf,"%02X:%02X:%02X:%02X:%02X:%02X",
                ethhdr->h_dest[0],ethhdr->h_dest[1],ethhdr->h_dest[2],ethhdr->h_dest[3],ethhdr->h_dest[4],ethhdr->h_dest[5]);

            if (0==strcmp(black_mac_list[i], smac_buf) ||
                0==strcmp(black_mac_list[i], dmac_buf))
            {
                printk("Drop mac packet %s--->%s\n", smac_buf, dmac_buf);
                return NF_DROP;
            }
        }
    }
    
    return NF_ACCEPT;
}

static struct nf_hook_ops hook_ops[] __read_mostly = {
    {
        .hook = mac_filter_hook,
        .pf = PF_INET,
        .hooknum = NF_INET_LOCAL_IN,
        .priority = NF_IP_PRI_FIRST + 1,
    }
};

static int __init mac_filter_init(void)
{
	int ret = 0;
    nf_register_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
    printk("#init mac filter.....ok\n");
	return ret;
}
module_init(mac_filter_init);

static void __exit mac_filter_exit(void)
{
    nf_unregister_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
	printk("#exit mac filter.....ok\n");
}
module_exit(mac_filter_exit);
MODULE_LICENSE("GPL");

