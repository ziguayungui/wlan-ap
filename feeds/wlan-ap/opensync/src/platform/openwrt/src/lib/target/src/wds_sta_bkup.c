/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

#include <uci.h>
#include <uci_blob.h>
#include <curl/curl.h>

#include "log.h"
#include "const.h"
#include "target.h"
#include "evsched.h"
#include "radio.h"
#include "vif.h"
#include "vlan.h"
#include "nl80211.h"
#include "utils.h"
#include "phy.h"
#include "captive.h"
#include "ovsdb_table.h"
#include "ovsdb_sync.h"
#include "rrm_config.h"
#include "fixup.h"
#include <libubox/blobmsg_json.h>

#define MODULE_ID LOG_MODULE_ID_VIF
#define UCI_BUFFER_SIZE 80

extern ovsdb_table_t table_Wifi_VIF_Config;
extern ovsdb_table_t table_Wifi_Radio_Config;

extern struct blob_buf b;
extern struct blob_buf del;
static ovsdb_table_t table_Manager;
static struct blob_buf bBkup= { };

#define vif_security_append(vif, index, key, value) \
	STRSCPY(vif->security_keys[index], key); \
	STRSCPY(vif->security[index], value); \
	index = index + 1; \
	vif->security_len = index; 

#define WDS_FILENAME "/usr/opensync/certs/wds-sta-config.json"
typedef enum {
	eWDS_BKHL_INVALID = -1,
	eWDS_BKHL_AVAIL   = 0,
	eWDS_BKHL_ACTIVE  = 1,
} eWdsBkhlState;

static eWdsBkhlState wds_bkhl_check = 1;
static bool cloud_connected = false;
static struct timespec last_test_time = {};
 
static eWdsBkhlState vif_backup_wds_sta_config(struct blob_buf *b)
{ 
	FILE *fd;
	char *jsonTxt;

	fd = fopen(WDS_FILENAME, "w");
	if (fd) {
		jsonTxt = blobmsg_format_json(b->head, true);
		fprintf(fd, "%s", jsonTxt);
		fclose(fd);
		free(jsonTxt);
	} else {
                LOGI("%s: Failed to back-up WDS-STA config.", __FUNCTION__);
		return eWDS_BKHL_INVALID;
	}
	return eWDS_BKHL_AVAIL;
}

static eWdsBkhlState vif_load_wds_sta_config(struct blob_buf *b)
{
	FILE *fd;
	struct stat st;
	char *jsonTxt;

        fd = fopen(WDS_FILENAME, "r");
        if (fd) {
	        stat(WDS_FILENAME, &st);
		jsonTxt = malloc(st.st_size+1);
		if (!jsonTxt)	{
			fclose(fd);
			LOGI("%s: Failed to restore WDS-STA config (malloc).", __FUNCTION__);
			return -1;
		}
		fscanf(fd,"%s", jsonTxt);
		blob_buf_init(b, 0);
                blobmsg_add_json_from_string(b, jsonTxt);
                fclose(fd);
                free(jsonTxt);
        } else {
                LOGI("%s: Failed to restore WDS-STA config (file).", __FUNCTION__);
		return eWDS_BKHL_INVALID;
        }
        return eWDS_BKHL_ACTIVE;
}

static eWdsBkhlState vif_apply_wds_sta_config( void )
{
	struct blob_attr *tb[__WIF_ATTR_MAX] = { };
	struct schema_Wifi_VIF_Config conf, *vconf;
	struct vif_crypto *vc = NULL;
	int index = 0;
	json_t *where, *where_parent;
	char *ifName, phyName[12];
	int val=0, radio=0;

	if (vif_load_wds_sta_config(&bBkup) != eWDS_BKHL_ACTIVE) {
		return eWDS_BKHL_INVALID;
	}
	blobmsg_parse(wifi_iface_policy, __WIF_ATTR_MAX, tb, blob_data(bBkup.head), blob_len(bBkup.head));
	if (!tb[WIF_ATTR_DEVICE] || !tb[WIF_ATTR_IFNAME] || !tb[WIF_ATTR_SSID]) {
		LOGI("Invalid WDS backhaul back-up file");
		return eWDS_BKHL_INVALID;
	}

	memset(&conf, 0, sizeof(conf));
	ifName = blobmsg_get_string(tb[WIF_ATTR_IFNAME]);
	sscanf(ifName,"wlan%d_%d",&radio,&val);
	sprintf(phyName,"radio%d",radio);

        if ((where = ovsdb_where_simple(SCHEMA_COLUMN(Wifi_VIF_Config, if_name), ifName))) {
		/* Found existing SSID with same if_name, remove it. */
		where_parent = ovsdb_where_simple(SCHEMA_COLUMN(Wifi_Radio_Config, if_name), phyName);
		ovsdb_table_delete_where_with_parent(&table_Wifi_VIF_Config, where, 
			 SCHEMA_TABLE(Wifi_Radio_Config), where_parent, SCHEMA_COLUMN(Wifi_Radio_Config, vif_configs));
		LOGI("DEL conflicting if_name to WDS backhaul ");
	}

	LOGI("ADD saved WDS backhaul on %s/%s", phyName, ifName);
	memset(&conf, 0, sizeof(conf));
	schema_Wifi_VIF_Config_mark_all_present(&conf);
	conf._partial_update = true;
	vconf = &conf;

	SCHEMA_SET_STR(conf.if_name, blobmsg_get_string(tb[WIF_ATTR_IFNAME]));
	SCHEMA_SET_STR(conf.ssid,    blobmsg_get_string(tb[WIF_ATTR_SSID]));
	SCHEMA_SET_STR(conf.mode,    "wds-sta");
	SCHEMA_SET_INT(conf.enabled, 1);

	if (tb[WIF_ATTR_IEEE80211V] && blobmsg_get_bool(tb[WIF_ATTR_IEEE80211V]))
		SCHEMA_SET_INT(conf.btm, 1);
	else
		SCHEMA_SET_INT(conf.btm, 0);

	if (tb[WIF_ATTR_ISOLATE] && blobmsg_get_bool(tb[WIF_ATTR_ISOLATE]))
		SCHEMA_SET_INT(conf.ap_bridge, 0);
	else
		SCHEMA_SET_INT(conf.ap_bridge, 1);

	SCHEMA_SET_INT(conf.uapsd_enable, true);

	if (tb[WIF_ATTR_NETWORK]) 
		SCHEMA_SET_STR(conf.bridge, blobmsg_get_string(tb[WIF_ATTR_NETWORK]));
	else
		SCHEMA_SET_STR(conf.bridge, "wan");

	if (tb[WIF_ATTR_VLAN_ID])
		SCHEMA_SET_INT(conf.vlan_id, blobmsg_get_u32(tb[WIF_ATTR_VLAN_ID]));
	else
		SCHEMA_SET_INT(conf.vlan_id, 1);

       	vc = vif_get_vif_crypto_map(tb);
	if (!vc) {
		vif_security_append(vconf, index, OVSDB_SECURITY_ENCRYPTION, OVSDB_SECURITY_ENCRYPTION_OPEN);
	} else if (vc->enterprise) {
		vif_security_append(vconf, index, OVSDB_SECURITY_ENCRYPTION, vc->encryption);
		vif_security_append(vconf, index, OVSDB_SECURITY_MODE, vc->mode);
		vif_security_append(vconf, index, OVSDB_SECURITY_RADIUS_SERVER_IP,
				  blobmsg_get_string(tb[WIF_ATTR_AUTH_SERVER]));
		vif_security_append(vconf, index, OVSDB_SECURITY_RADIUS_SERVER_PORT,
				  blobmsg_get_string(tb[WIF_ATTR_AUTH_PORT]));
		vif_security_append(vconf, index, OVSDB_SECURITY_RADIUS_SERVER_SECRET,
				  blobmsg_get_string(tb[WIF_ATTR_AUTH_SECRET]));

		if (tb[WIF_ATTR_ACCT_SERVER] && tb[WIF_ATTR_ACCT_PORT] && tb[WIF_ATTR_ACCT_SECRET])
		{
			vif_security_append(vconf, index, OVSDB_SECURITY_RADIUS_ACCT_IP,
				blobmsg_get_string(tb[WIF_ATTR_ACCT_SERVER]));
			vif_security_append(vconf, index, OVSDB_SECURITY_RADIUS_ACCT_PORT,
				blobmsg_get_string(tb[WIF_ATTR_ACCT_PORT]));
			vif_security_append(vconf, index, OVSDB_SECURITY_RADIUS_ACCT_SECRET,
				blobmsg_get_string(tb[WIF_ATTR_ACCT_SECRET]));
		}

		if (tb[WIF_ATTR_ACCT_INTERVAL])
		{
			char interval[5];
			sprintf(interval, "%d", blobmsg_get_u32(tb[WIF_ATTR_ACCT_INTERVAL]));
			vif_security_append(vconf, index, OVSDB_SECURITY_RADIUS_ACCT_INTERVAL, interval);
		}
	} else if (tb[WIF_ATTR_KEY]) {
		vif_security_append(vconf, index, OVSDB_SECURITY_ENCRYPTION, vc->encryption);
		vif_security_append(vconf, index, OVSDB_SECURITY_MODE, vc->mode);
		vif_security_append(vconf, index, OVSDB_SECURITY_KEY,
				  blobmsg_get_string(tb[WIF_ATTR_KEY]));
	} else {
       		vif_security_append(vconf, index, OVSDB_SECURITY_ENCRYPTION, OVSDB_SECURITY_ENCRYPTION_OPEN);
	}

	SCHEMA_SET_STR(conf.min_hw_mode, "11n");
	SCHEMA_SET_INT(conf.rrm, 1);
	SCHEMA_SET_INT(conf.ft_psk, 0);
	SCHEMA_SET_INT(conf.group_rekey, 0);
	strscpy(conf.mac_list_type, "none", sizeof(conf.mac_list_type));
	conf.mac_list_len = 0;
 
	radio_ops->op_vconf(&conf, blobmsg_get_string(tb[WIF_ATTR_DEVICE]));
	return eWDS_BKHL_ACTIVE;
}

int vif_test_wds_sta_config( void )
{
	struct timespec current_time;

	if (cloud_connected == true)		return 0;
	if (wds_bkhl_check != eWDS_BKHL_AVAIL)	return 0;

	clock_gettime(CLOCK_MONOTONIC, &current_time);
	if ((current_time.tv_sec - last_test_time.tv_sec) > 60)
	{
		last_test_time = current_time;
		/* Manager reports no connection to the cloud.
		   Apply the saved wds-sta config to attempt recovery */
		LOGI("Cloud disconnected: ADD saved WDS backhaul");
               	wds_bkhl_check = vif_apply_wds_sta_config();
		sync();
		system("/sbin/reload_config");
	}
	return 0;
}


int wds_vif_config_set(const struct schema_Wifi_Radio_Config *rconf,
                        const struct schema_Wifi_VIF_Config *vconf,
                        const struct schema_Wifi_VIF_Config_flags *changed)
{
        int vid = 0;

        blob_buf_init(&b, 0);
        blob_buf_init(&del,0);
        blobmsg_add_string(&b, "ifname", vconf->if_name);
        blobmsg_add_string(&b, "device", rconf->if_name);
        if (!strcmp(vconf->mode, "wds-ap")) {
                blobmsg_add_string(&b, "mode", "ap");
                blobmsg_add_bool(&b, "wds", 1);
        } else if (!strcmp(vconf->mode, "wds-sta")) {
                blobmsg_add_string(&b, "mode", "sta");
                blobmsg_add_bool(&b, "wds", 1);
        } else {
                return -1;
        }

        blobmsg_add_bool(&b, "disabled", vconf->enabled ? 0 : 1);
        blobmsg_add_string(&b, "ssid", vconf->ssid);
        if (!strcmp(vconf->ssid_broadcast, "disabled"))
                blobmsg_add_bool(&b, "hidden", 1);
        else
                blobmsg_add_bool(&b, "hidden", 0);
        if (vconf->ap_bridge)
                blobmsg_add_bool(&b, "isolate", 0);
        else
                blobmsg_add_bool(&b, "isolate", 1);
        if (vconf->uapsd_enable)
                blobmsg_add_bool(&b, "uapsd", 1);
        else
                blobmsg_add_bool(&b, "uapsd", 0);
        blobmsg_add_string(&b, "min_hw_mode", vconf->min_hw_mode);

       if (vconf->ft_mobility_domain) {
                blobmsg_add_bool(&b, "ieee80211r", 1);
                blobmsg_add_hex16(&b, "mobility_domain", vconf->ft_mobility_domain);
                blobmsg_add_bool(&b, "ft_over_ds", 0);
                blobmsg_add_bool(&b, "reassociation_deadline", 1);
        } else {
                blobmsg_add_bool(&b, "ieee80211r", 0);
        }
        if (vconf->btm) {
                blobmsg_add_bool(&b, "ieee80211v", 1);
                blobmsg_add_bool(&b, "bss_transition", 1);
        } else {
                blobmsg_add_bool(&b, "ieee80211v", 0);
                blobmsg_add_bool(&b, "bss_transition", 0);
        }
        blobmsg_add_string(&b, "network", vconf->bridge);

        if (changed->vlan_id && strncmp(vconf->bridge, "gre", strlen("gre"))) {
                blobmsg_add_u32(&b, "vlan_id", vconf->vlan_id);
                if (vconf->vlan_id > 2)
                        vid = vconf->vlan_id;
                blobmsg_add_u32(&b, "vid", vid);
        }

        if (changed->mac_list_type) {
                struct blob_attr *a;
                int i;
                if (!strcmp(vconf->mac_list_type, "whitelist"))
                        blobmsg_add_string(&b, "macfilter", "allow");
                else if (!strcmp(vconf->mac_list_type,"blacklist"))
                        blobmsg_add_string(&b, "macfilter", "deny");
                else
                        blobmsg_add_string(&b, "macfilter", "disable");

                a = blobmsg_open_array(&b, "maclist");
                for (i = 0; i < vconf->mac_list_len; i++)
                        blobmsg_add_string(&b, NULL, (char*)vconf->mac_list[i]);
                blobmsg_close_array(&b, a);
        }

        blobmsg_add_bool(&b, "wpa_disable_eapol_key_retries", 1);
        blobmsg_add_u32(&b, "channel", rconf->channel);

        if (vif_config_security_set(&b, vconf))
                return -1;

        if (changed->custom_options)
                vif_config_custom_opt_set(&b, &del, vconf);

        rrm_config_vif(&b, &del, rconf->freq_band, vconf->if_name);

        blob_to_uci_section(uci, "wireless", vconf->if_name, "wifi-iface",
                            b.head, &wifi_iface_param, del.head);

        if (vid)
                vlan_add((char *)vconf->if_name, vid, !strcmp(vconf->bridge, "wan"));
        else
                vlan_del((char *)vconf->if_name);

        uci_commit_all(uci);

        if (!strcmp(vconf->mode, "wds-sta"))
                wds_bkhl_check = vif_backup_wds_sta_config(&b);
        return 0;
}


static void callback_Manager(ovsdb_update_monitor_t *mon,
			     struct schema_Manager *old,
			     struct schema_Manager *conf)
{
	if (mon->mon_type == OVSDB_UPDATE_DEL)
		return;

	cloud_connected = conf->is_connected;
	LOGI("Cloud MOD state: %s", cloud_connected ? "connect" : "disconnect");
	clock_gettime(CLOCK_MONOTONIC, &last_test_time);
}

int vif_wds_init( void )
{
        wds_bkhl_check = eWDS_BKHL_AVAIL;
        clock_gettime(CLOCK_MONOTONIC, &last_test_time);
	OVSDB_TABLE_INIT_NO_KEY(Manager);
	OVSDB_TABLE_MONITOR(Manager, false);
	return 0;
}
