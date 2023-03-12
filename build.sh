#!/bin/bash

set -ex
ROOT_PATH=${PWD}
BUILD_DIR=${ROOT_PATH}/openwrt
TARGET=${1}

if [ -z "$1" ]; then
	echo "Error: please specify TARGET"
	exit 1
fi

if [ ! "$(ls -A $BUILD_DIR)" ]; then
	python3 setup.py --setup || exit 1
    
else
	python3 setup.py --rebase
	echo "### OpenWrt repo already setup"
fi

sed -i "s= [a-z]*:.*/project= https://gitee.com/wlan-ap=g" openwrt/feeds.conf.default || true
sed -i "s= [a-z]*:.*/feed= https://gitee.com/wlan-ap=g" openwrt/feeds.conf.default || true

cd ${BUILD_DIR}
./scripts/gen_config.py ${TARGET} || exit 1
cd -

echo "### Building image ..."
cd $BUILD_DIR
make -j$(nproc) V=s
