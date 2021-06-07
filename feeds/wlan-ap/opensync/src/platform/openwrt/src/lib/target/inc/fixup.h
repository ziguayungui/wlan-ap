/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef _FIXUP_H__
#define _FIXUP_H__

struct vif_fixup {
        struct avl_node avl;
        char name[IF_NAMESIZE];
        bool has_radius_nasid;
	int conf_ft_psk;
};

struct vif_fixup * vif_fixup_find(const char *name);
void vif_fixup_del(char *ifname);
void vif_fixup_add(char *ifname);


struct radio_fixup {
        struct avl_node avl;
        char name[IF_NAMESIZE];
	int tx_power, conf_tx_power;
	int tx_chainmask, conf_tx_chainmask;
	int rx_chainmask, conf_rx_chainmask;
	int bcn_int, conf_bcn_int;
	char hw_mode[128+1], conf_hw_mode[128+1];
	int conf_max_clients, max_clients;
};

struct radio_fixup * radio_fixup_find(const char *ifname);

#endif
