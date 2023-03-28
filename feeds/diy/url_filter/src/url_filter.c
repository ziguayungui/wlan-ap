#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netfilter.h>
#include <net/netfilter/nf_conntrack.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <linux/types.h>

char *black_url_list[]={"www.baidu.com", NULL};

int parse_http_proto(char *data, int len, char *url, int url_len)
{
    int i = 0;
    int start = 0;
    for (i=0; i< len; i++)
    {
        if  (data[i] == 0x0d && data[i+1] == 0x0a)
        {
            if (0 == memcmp(&data[start], "Host:", 5))
            {
                char *host_pos = data + start + 6;
                int host_len = i - start -6;
                if (host_len > url_len - 1)
                    memcpy(url, host_pos, host_len - 1);
                else
                    memcpy(url, host_pos, host_len);
                return 0;
            }

            if (data[i+2] == 0x0d && data[i+3]==0x0a)
            {
                break;
            }
            start = i+2;
        }
    }
    return -1;
}

static unsigned int url_filter_hook(void *priv, struct sk_buff *skb, const struct nf_hook_state *state)
{
    struct nf_conn *ct = (struct nf_conn *)skb->_nfct;
    struct udphdr *udph = NULL;
    struct tcphdr *tcph = NULL;
    struct iphdr *iph = NULL;
    struct ethhdr *ethhdr = NULL;
    u_int16_t sport = 0;
    u_int16_t dport = 0;
    u_int32_t i = 0;
    char url[64] = {0};
    int ret = -1;

    if(ct == NULL || !nf_ct_is_confirmed(ct))
        return NF_ACCEPT;
    iph = ip_hdr(skb);
    if (!iph)
        return NF_ACCEPT;
    
    if (IPPROTO_TCP != iph->protocol)
        return NF_ACCEPT;
    
    tcph = tcp_hdr(skb);
    char *data = skb->data + iph->ihl*4 + tcph->doff*4;
    int len = ntohs(iph->tot_len) - iph->ihl*4 - tcph->doff *4;
    sport = htons(tcph->source);
    dport = htons(tcph->dest);

    if (80 == dport && len > 0)
        ret = parse_http_proto(data, len, url, sizeof(url));
    
    if(0 == ret)
    {
        for (i=0; NULL != black_url_list[i]; i++)
        {
            printk("Drop packet %pI4:%d--->%pI4:%d url=%s\n", &iph->saddr, sport, &iph->daddr, dport, black_url_list[i]);
            return NF_DROP;
        }
    }    
    return NF_ACCEPT;
}

static struct nf_hook_ops hook_ops[] __read_mostly = {
    {
        .hook = url_filter_hook,
        .pf = PF_INET,
        .hooknum = NF_INET_FORWARD,
        .priority = NF_IP_PRI_FIRST + 1,
    }
};

static int __init url_filter_init(void)
{
	int ret = 0;
    nf_register_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
    printk("#init url filter.....ok\n");
	return ret;
}
module_init(url_filter_init);

static void __exit url_filter_exit(void)
{
    nf_unregister_net_hooks(&init_net, hook_ops, ARRAY_SIZE(hook_ops));
	printk("#exit url filter.....ok\n");
}
module_exit(url_filter_exit);
MODULE_LICENSE("GPL");

