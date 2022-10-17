include $(TOPDIR)/rules.mk

PKG_NAME:=ucentral-tools
PKG_RELEASE:=1

PKG_SOURCE_URL=https://github.com/blogic/ucentral-tools.git
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2021-01-28
PKG_SOURCE_VERSION:=f538ff28c7a01b05fd2f5e987fe6b2a1d7532527

PKG_MAINTAINER:=John Crispin <john@phrozen.org>
PKG_LICENSE:=BSD-3-Clause

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/ucentral-tools
  SECTION:=ucentral
  CATEGORY:=uCentral
  TITLE:=ucentral tools collection
  DEPENDS:=+libubox +libcurl +libradcli +libopenssl
endef

define Package/ucentral-tools/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/{dhcpdiscover,dnsprobe,radiusprobe,ip-collide} $(1)/usr/sbin/
	$(CP) ./files/* $(1)
endef

$(eval $(call BuildPackage,ucentral-tools))
