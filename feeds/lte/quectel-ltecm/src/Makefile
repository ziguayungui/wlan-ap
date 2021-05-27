#ifneq ($(CROSS_COMPILE),)
#CROSS-COMPILE:=$(CROSS_COMPILE)
#endif
#CROSS-COMPILE:=/workspace/buildroot/buildroot-qemu_mips_malta_defconfig/output/host/usr/bin/mips-buildroot-linux-uclibc-
#CROSS-COMPILE:=/workspace/buildroot/buildroot-qemu_arm_vexpress_defconfig/output/host/usr/bin/arm-buildroot-linux-uclibcgnueabi-
#CROSS-COMPILE:=/home/jianan/ignite/acn-devices3.metrolinq-qca/staging_dir/toolchain-arm_cortex-a7_gcc-4.8-linaro_uClibc-1.0.14_eabi/bin/arm-openwrt-linux-uclibcgnueabi-
#CC:=$(CROSS-COMPILE)gcc
#LD:=$(CROSS-COMPILE)ld

LDLIBS += -lpthread -ldl

all: release

release: clean
	$(CC) -Wall -s QmiWwanCM.c GobiNetCM.c main.c MPQMUX.c QMIThread.c util.c udhcpc.c -o quectel-CM ${LDLIBS}

debug: clean
	$(CC) -Wall -g QmiWwanCM.c GobiNetCM.c main.c MPQMUX.c QMIThread.c util.c udhcpc.c -o quectel-CM ${LDLIBS}

clean:
	rm -rf quectel-CM *~

