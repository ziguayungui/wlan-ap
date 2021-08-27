/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef _VIF_H__
#define _VIF_H__

#include <uci.h>
#include <uci_blob.h>

#define OVSDB_SECURITY_KEY                  "key"
#define OVSDB_SECURITY_OFTAG                "oftag"
#define OVSDB_SECURITY_MODE                 "mode"
#define OVSDB_SECURITY_MODE_WEP64           "64"
#define OVSDB_SECURITY_MODE_WEP128          "128"
#define OVSDB_SECURITY_MODE_WPA1            "1"
#define OVSDB_SECURITY_MODE_WPA2            "2"
#define OVSDB_SECURITY_MODE_WPA3            "3"
#define OVSDB_SECURITY_MODE_MIXED           "mixed"
#define OVSDB_SECURITY_ENCRYPTION           "encryption"
#define OVSDB_SECURITY_ENCRYPTION_OPEN      "OPEN"
#define OVSDB_SECURITY_ENCRYPTION_WEP       "WEP"
#define OVSDB_SECURITY_ENCRYPTION_WPA_PSK   "WPA-PSK"
#define OVSDB_SECURITY_ENCRYPTION_WPA_SAE   "WPA-SAE"
#define OVSDB_SECURITY_ENCRYPTION_WPA_EAP   "WPA-EAP"
#define OVSDB_SECURITY_ENCRYPTION_WPA3_EAP  "WPA3-EAP"
#define OVSDB_SECURITY_ENCRYPTION_WPA3_EAP_192  "WPA3-EAP-192"
#define OVSDB_SECURITY_RADIUS_SERVER_IP     "radius_server_ip"
#define OVSDB_SECURITY_RADIUS_SERVER_PORT   "radius_server_port"
#define OVSDB_SECURITY_RADIUS_SERVER_SECRET "radius_server_secret"
#define OVSDB_SECURITY_RADIUS_ACCT_IP       "radius_acct_ip"
#define OVSDB_SECURITY_RADIUS_ACCT_PORT     "radius_acct_port"
#define OVSDB_SECURITY_RADIUS_ACCT_SECRET   "radius_acct_secret"
#define OVSDB_SECURITY_RADIUS_ACCT_INTERVAL "radius_acct_interval"

#define SCHEMA_CONSTS_RADIUS_NAS_ID         "radius_nas_id"
#define SCHEMA_CONSTS_RADIUS_OPER_NAME      "radius_oper_name"
#define SCHEMA_CONSTS_RADIUS_NAS_IP         "radius_nas_ip"

#define SCHEMA_VIF_CUSTOM_OPT_SZ            20
#define SCHEMA_VIF_CUSTOM_OPTS_MAX          15

enum {
	WIF_ATTR_DEVICE,
	WIF_ATTR_IFNAME,
	WIF_ATTR_INDEX,
	WIF_ATTR_MODE,
	WIF_ATTR_SSID,
	WIF_ATTR_BSSID,
	WIF_ATTR_CHANNEL,
	WIF_ATTR_ENCRYPTION,
	WIF_ATTR_KEY,
	WIF_ATTR_DISABLED,
	WIF_ATTR_HIDDEN,
	WIF_ATTR_ISOLATE,
	WIF_ATTR_NETWORK,
	WIF_ATTR_AUTH_SERVER,
	WIF_ATTR_AUTH_PORT,
	WIF_ATTR_AUTH_SECRET,
	WIF_ATTR_ACCT_SERVER,
	WIF_ATTR_ACCT_PORT,
	WIF_ATTR_ACCT_SECRET,
	WIF_ATTR_ACCT_INTERVAL,
	WIF_ATTR_REQ_CUI,
	WIF_ATTR_IEEE80211R,
	WIF_ATTR_IEEE80211W,
	WIF_ATTR_MOBILITY_DOMAIN,
	WIF_ATTR_FT_OVER_DS,
	WIF_ATTR_FT_PSK_LOCAL,
	WIF_ATTR_UAPSD,
	WIF_ATTR_VLAN_ID,
	WIF_ATTR_VID,
	WIF_ATTR_MACLIST,
	WIF_ATTR_MACFILTER,
	WIF_ATTR_RATELIMIT,
	WIF_ATTR_URATE,
	WIF_ATTR_DRATE,
	WIF_ATTR_CURATE,
	WIF_ATTR_CDRATE,
	WIF_ATTR_IEEE80211V,
	WIF_ATTR_BSS_TRANSITION,
	WIF_ATTR_DISABLE_EAP_RETRY,
	WIF_ATTR_IEEE80211K,
	WIF_ATTR_RTS_THRESHOLD,
	WIF_ATTR_DTIM_PERIOD,
	WIF_ATTR_INTERWORKING,
	WIF_ATTR_HS20,
	WIF_ATTR_HESSID,
	WIF_ATTR_ROAMING_CONSORTIUM,
	WIF_ATTR_VENUE_NAME,
	WIF_ATTR_VENUE_GROUP,
	WIF_ATTR_VENUE_TYPE,
	WIF_ATTR_VENUE_URL,
	WIF_ATTR_NETWORK_AUTH_TYPE,
	WIF_ATTR_IPADDR_TYPE_AVAILABILITY,
	WIF_ATTR_CONNECTION_CAPABILITY,
	WIF_ATTR_DOMAIN_NAME,
	WIF_ATTR_MCC_MNC,
	WIF_ATTR_NAI_REALM,
	WIF_ATTR_GAS_ADDR3,
	WIF_ATTR_QOS_MAP_SET,
	WIF_ATTR_OSEN,
	WIF_ATTR_ACCESS_NETWORK_TYPE,
	WIF_ATTR_INTERNET,
	WIF_ATTR_ESR,
	WIF_ATTR_ASRA,
	WIF_ATTR_UESA,
	WIF_ATTR_DISABLE_DGAF,
	WIF_ATTR_WAN_METRICS,
	WIF_ATTR_ANQP_DOMAIN_ID,
	WIF_ATTR_DEAUTH_REQUEST_TIMEOUT,
	WIF_ATTR_OPER_FRIENDLY_NAME,
	WIF_ATTR_OPERATING_CLASS,
	WIF_ATTR_OPER_ICON,
	WIF_ATTR_PROBE_ACCEPT_RATE,
	WIF_ATTR_CLIENT_CONNECT_THRESHOLD,
	WIF_ATTR_CLIENT_DISCONNECT_THRESHOLD,
	WIF_ATTR_BEACON_RATE,
	WIF_ATTR_MCAST_RATE,
	WIF_ATTR_RADIUS_NAS_ID_ATTR,
	WIF_ATTR_RADIUS_NAS_IP_ATTR,
	WIF_ATTR_RADIUS_AUTH_REQ_ATTR,
	WIF_ATTR_RADIUS_ACCT_REQ_ATTR,
	WIF_ATTR_MESH_ID,
	WIF_ATTR_MESH_FWDING,
	WIF_ATTR_MESH_MCAST_RATE,
	WIF_ATTR_DYNAMIC_VLAN,
	WIF_ATTR_DVLAN_FILE,
	WIF_ATTR_DVLAN_NAMING,
	WIF_ATTR_DVLAN_BRIDGE,
	WIF_ATTR_MIN_HW_MODE,
	WIF_ATTR_11R_R0KH,
	WIF_ATTR_11R_R1KH,
	WIF_ATTR_RADPROXY,
	WIF_ATTR_PROXY_ARP,
	WIF_ATTR_MCAST_TO_UCAST,
	WIF_ATTR_AUTH_CACHE,
	WIF_ATTR_WDS,
	__WIF_ATTR_MAX,
};
extern const struct blobmsg_policy wifi_iface_policy[__WIF_ATTR_MAX];
extern const struct uci_blob_param_list wifi_iface_param;

struct vif_crypto {
        char *uci;
        char *encryption;
        char *mode;
        int enterprise;
};

struct vif_crypto *vif_get_vif_crypto_map(struct blob_attr **tb);
bool vif_get_security(struct schema_Wifi_VIF_State *vstate, char *mode, char *encryption, char *radiusServerIP, char *password, char *port);
extern bool vif_state_update(struct uci_section *s, struct schema_Wifi_VIF_Config *vconf);
void vif_hs20_update(struct schema_Hotspot20_Config *hs2conf);
void vif_hs20_osu_update(struct schema_Hotspot20_OSU_Providers *hs2osuconf);
void vif_hs20_icon_update(struct schema_Hotspot20_Icon_Config *hs2iconconf);
void vif_section_del(char *section_name);
void vif_check_radius_proxy(void);
int vif_config_security_set(struct blob_buf *b, const struct schema_Wifi_VIF_Config *vconf);
void vif_config_custom_opt_set(struct blob_buf *b, struct blob_buf *del, const struct schema_Wifi_VIF_Config *vconf);
int vif_test_wds_sta_config( void );
int wds_vif_config_set(const struct schema_Wifi_Radio_Config *rconf, 
	const struct schema_Wifi_VIF_Config *vconf, const struct schema_Wifi_VIF_Config_flags *changed);
int vif_wds_init(void);
#endif
