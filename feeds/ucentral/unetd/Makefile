#
# Copyright (C) 2022 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=unetd
PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL=https://github.com/nbd168/unetd
PKG_SOURCE_DATE:=2022-06-15
PKG_SOURCE_VERSION:=d7df17009e7fe0828a38ce9f6ad107d75c04f5f4
PKG_MIRROR_HASH:=c966f29cd553967b2a6a1c6dc32549129a1ae9303b1d2d8cb294f8bbd7f21768

PKG_LICENSE:=GPL-2.0
PKG_MAINTAINER:=Felix Fietkau <nbd@nbd.name>

PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/unetd
  SECTION:=utils
  CATEGORY:=Base system
  TITLE:=Wireguard network configuration service
  DEPENDS:=+libubox +libubus +libblobmsg-json +libnl-tiny +TARGET_ipq807x:kmod-wireguard-backport +!TARGET_ipq807x:kmod-wireguard +wireguard-tools
endef

TARGET_CFLAGS += \
	-I$(STAGING_DIR)/usr/include/libnl-tiny \
	-I$(STAGING_DIR)/usr/include

CMAKE_OPTIONS += \
	-DLIBNL_LIBS=-lnl-tiny

define Package/unetd/install
	$(INSTALL_DIR) \
		$(1)/etc/init.d \
		$(1)/lib/netifd/proto \
		$(1)/usr/sbin
	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/sbin/unetd \
		$(1)/usr/sbin/
	$(INSTALL_BIN) ./files/unetd.init $(1)/etc/init.d/unetd
	$(INSTALL_BIN) ./files/unetd.sh $(1)/lib/netifd/proto
endef

$(eval $(call BuildPackage,unetd))
