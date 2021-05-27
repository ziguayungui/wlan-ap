#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=quectel-ltecm
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Package/quectel-ltecm
	SECTION:=LTE
	CATEGORY:=LTE
	DEPENDS:= +libpthread
	TITLE:=QUECTEL LTE Connection Manager
endef

define Package/quectel-ltecm/description
	The utility is used to setup and manage LTE link.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/quectel-ltecm/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/quectel-CM $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_DATA) ./files/quectelcm.hotplug $(1)/etc/hotplug.d/iface/60-quectelcm
#	$(INSTALL_DIR) $(1)/etc/init.d
#	$(INSTALL_BIN) ./files/acc.init $(1)/etc/init.d/acc
endef

$(eval $(call BuildPackage,quectel-ltecm))
