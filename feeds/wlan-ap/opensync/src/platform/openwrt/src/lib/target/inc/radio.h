/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef _RADIO_H__
#define _RADIO_H__
#include "ovsdb_update.h"

struct timeout;
typedef void (*timeout_handler)(struct timeout *t);
struct rrm_neighbor {
	char *mac;
	char *ssid;
	char *ie;
};

struct timeout
{
	//struct list_head list;
	bool pending;

	timeout_handler cb;
	struct timeval time;
};

#define WLAN_IFUP_TIMEOUT 70 /* secs */
#define MAX_SSID 24 /* 8 ssids are supported per radio */

extern const struct target_radio_ops *radio_ops;
extern int reload_config;
extern struct blob_buf b;
extern struct uci_context *uci;

extern int radio_ubus_init(void);

extern int hapd_rrm_enable(char *name, int neighbor, int beacon);
extern int hapd_rrm_set_neighbors(char *name, struct rrm_neighbor *neigh, int count);

extern void radio_maverick(void *arg);
void set_config_apply_timeout(ovsdb_update_monitor_t *mon);
//extern void vif_init(void);

int nl80211_channel_get(char *name, unsigned int *chan);

#endif
