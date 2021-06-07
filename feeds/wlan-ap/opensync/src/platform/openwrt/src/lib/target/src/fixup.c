/* SPDX-License-Identifier: BSD-3-Clause */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>

#include "log.h"
#include "const.h"
#include "target.h"

#include <libubox/avl-cmp.h>
#include <libubox/avl.h>
#include <libubox/vlist.h>
#include <net/if.h>

#include "fixup.h"

/*
 * VIF Fixup
 */

static struct avl_tree vif_fixup_tree = AVL_TREE_INIT(vif_fixup_tree, avl_strcmp, false, NULL);

struct vif_fixup * vif_fixup_find(const char *ifname)
{
        struct vif_fixup *vif = avl_find_element(&vif_fixup_tree, ifname, vif, avl);
        if (vif)
                return vif;

	/* Not found, add */
        vif = malloc(sizeof(*vif));
        if (!vif)
                return NULL;

        memset(vif, 0, sizeof(*vif));
        strncpy(vif->name, ifname, IF_NAMESIZE);
        vif->avl.key = vif->name;
        avl_insert(&vif_fixup_tree, &vif->avl);
        return vif;
}

void vif_fixup_del(char *ifname)
{
        struct vif_fixup *vif;

        vif = avl_find_element(&vif_fixup_tree, ifname, vif, avl);
        if (vif) {
        	avl_delete(&vif_fixup_tree, &vif->avl);
        	free(vif);
	}
}

bool target_vif_fixup(struct schema_Wifi_VIF_Config *vconf, struct schema_Wifi_VIF_State *vstate)
{
	LOGI("target_vif_fixup called %s", vconf->if_name);
        return true;
}

/*
 * Radio Fixup
 */

static struct avl_tree radio_fixup_tree = AVL_TREE_INIT(radio_fixup_tree, avl_strcmp, false, NULL);

struct radio_fixup * radio_fixup_find(const char *ifname)
{
        struct radio_fixup *radio = avl_find_element(&radio_fixup_tree, ifname, radio, avl);
	if (radio)
        	return radio;

	/* Not found, Add */
        radio = malloc(sizeof(*radio));
        if (!radio)
                return NULL;

        memset(radio, 0, sizeof(*radio));
        strncpy(radio->name, ifname, IF_NAMESIZE);
        radio->avl.key = radio->name;
        avl_insert(&radio_fixup_tree, &radio->avl);
	return radio;
}

bool target_radio_fixup(struct schema_Wifi_Radio_Config *rconf, struct schema_Wifi_Radio_State *rstate)
{
        struct radio_fixup *rfixup = radio_fixup_find(rconf->if_name);
	char *val;

	LOGI("target_radio_fixup called %s", rconf->if_name);
	if (!rfixup)	return false;
	LOGI("fixup tx_power %d %d %d", rconf->tx_power, rfixup->conf_tx_power, rfixup->tx_power);
        LOGI("fixup tx_chainmask %d %d %d", rconf->tx_chainmask, rfixup->conf_tx_chainmask, rfixup->tx_chainmask);
        LOGI("fixup rx_chainmask %d %d %d", rconf->rx_chainmask, rfixup->conf_rx_chainmask, rfixup->rx_chainmask);
        LOGI("fixup bcn_int %d %d %d", rconf->bcn_int, rfixup->bcn_int, rfixup->bcn_int);
	if (rconf->tx_power == rfixup->conf_tx_power)         rconf->tx_power = rfixup->tx_power;
	if (rconf->tx_chainmask == rfixup->conf_tx_chainmask) rconf->tx_chainmask = rfixup->tx_chainmask;
	if (rconf->rx_chainmask == rfixup->conf_rx_chainmask) rconf->rx_chainmask = rfixup->rx_chainmask;
	if (rconf->bcn_int == rfixup->conf_bcn_int)           rconf->bcn_int = rfixup->bcn_int;
	if ((!strcmp(rconf->hw_mode, "auto")) && (rfixup->hw_mode[0])) {
		strncpy(rconf->hw_mode, rfixup->hw_mode, sizeof(rfixup->hw_mode));
	}
	val = SCHEMA_KEY_VAL(rconf->custom_options, SCHEMA_CONSTS_MAX_CLIENTS);
	if ((val) && (strtol(val, NULL, 10) == rfixup->conf_max_clients)) {
		snprintf(val, 5, "%d", rfixup->max_clients);
	}
	
	return true;
}


