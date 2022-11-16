#
#
# Note: This Makefile is for 2.6.X and 3.X kernel, at present.
#
# V0.0.0.6 Ver.Make
SHELL := /bin/bash
# QNAP Patch
KERNEL_SRC := $(KSRC)

PWD:=$(shell pwd)

SUPPORT_VANIR=n
SUPPORT_ODIN=n
SUPPORT_THOR=n
SUPPORT_MAGNI=n
SUPPORT_ATHENA=y
SUPPORT_SP2=n

RAID_MODULE=n
RAID6_MODULE=n
CACHE_MODULE=n


##
# default to build for the running kernel
ifeq ("x", "x$(KERNEL_SRC)")
	KERNEL_SRC=/lib/modules/$(shell uname -r)/build

  ifeq ("powerpc", "$(ARCH)")
   	KERNEL_SRC=/root/katmai_29/lib/modules/2.6.29/build
  endif
 
  ifeq ("arm", "$(ARCH)")
	KERNEL_SRC=/root/armadaxp/linux-2.6.35.9
  endif
endif

# use KERNEL_SRC if not called by our driver disk maker
ifeq ("x", "x$(KERNEL_SOURCE_DIR)")
        KERNEL_SRC_DIR=$(KERNEL_SRC)
else
	KERNEL_SRC_DIR=$(KERNEL_SOURCE_DIR)
endif

ifeq ("x", "x$(KERNEL_BUILD_DIR)")
	KERNEL_BLD_DIR=$(KERNEL_SRC_DIR)
else
        KERNEL_BLD_DIR=$(KERNEL_BUILD_DIR)
endif

#ifneq ($(KERNELRELEASE),)
#	include $(CURDIR)/mv_conf.mk
#else
#	include mv_conf.mk
#endif

	LIB_TYPE=_nonraid_
# KERNEL_VER := $(shell cat $(KERNEL_BLD_SRC)/include/linux/version.h | grep UTS_RELEASE | cut -c22-24 | head -n 1)
ifneq ("arm", "$(ARCH)")
# we may build for 32bit kernel on a 64bit system
CONFIG_64BIT=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_64BIT | awk -F= '{print $$2}')
CONFIG_REGPARM=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_REGPARM | awk -F= '{print $$2}')
CONFIG_SUSE_KERNEL=$(shell [ -f $(KERNEL_BLD_DIR)/.config ] && cat $(KERNEL_BLD_DIR)/.config | grep -m 1 CONFIG_SUSE_KERNEL | awk -F= '{print $$2}')
endif

ifeq ($(ARCH), )
ifeq ($(strip $(CONFIG_64BIT)),y)
	ARCH_TYPE=x86_64
		MV_LIB_NAME=libmv$(LIB_TYPE)64
else
	ARCH_TYPE=i386
	ifeq ($(strip $(CONFIG_REGPARM)),y)
		MV_LIB_NAME=libmv$(LIB_TYPE)32
	else
		MV_LIB_NAME=libmv$(LIB_TYPE)32_noregparm
	endif
endif
else
	ARCH_TYPE=$(ARCH)
endif

ifeq ("powerpc", "$(ARCH)")
	MV_CC     =  $(CROSS_COMPILE)gcc
	MV_LD     =  $(CROSS_COMPILE)ld
else

ifeq ("arm", "$(ARCH)")
	MV_CC	  =  $(CROSS_COMPILE)gcc
	MV_LD	  =  $(CROSS_COMPILE)ld
else
# QNAP Patch
	MV_CC     =  $(CROSS_COMPILE)gcc
	MV_LD     =  $(CROSS_COMPILE)ld
####
endif
endif

export LD_LIBRARY_PATH

core_com_objs := core/api/core_api.o  core/api/core_api_exp.o  core/api/core_api_bbu.o core/api/core_api_flash.o core/api/core_api_led.o core/api/core_api_pd.o core/api/core_api_test.o \
		core/core/core_alarm.o core/core/core_device.o  core/core/core_err_inj.o	 \
	   core/core/core_error.o \
	   core/core/core_event.o core/core/core_large_req.o \
	   core/core/core_manager.o core/core/core_multi_path.o \
	   core/core/core_pd_map.o \
	   core/core/core_protocol.o \
	   core/core/core_resource.o \
	   core/core/core_util.o  \
	   core/pl/core_bbu.o  \
	   core/pl/core_gpio.o \
	   core/pl/core_i2c.o \
	   core/pl/core_rmw_flash.o \
	   core/pl/core_eeprom.o \
	   core/pm/core_pm.o \
	   core/sas_base/sas_error.o core/sas_base/sas_handler.o \
	   core/sas_base/sas_util.o core/sas_base/sas_init.o \
	   core/sas_smp/core_expander.o \
	   core/sata_base/core_sata.o core/sata_base/sata_error.o \
	   core/scsi/core_sat.o \
	   core/scsi/core_ses.o



athena_objs_$(SUPPORT_ATHENA)   := product/athena/core_spi.o   \
			       product/athena/core_hal.o \
			       product/athena/core_init.o
sp2_objs_$(SUPPORT_SP2)   := product/sp2/core_spi.o   \
			       product/sp2/core_hal.o \
			       product/sp2/core_init.o
CORE_OBJS       := $(core_com_objs) $(athena_objs_y) $(sp2_objs_y)


COMM_OBJS       := lib/common/com_util.o lib/common/com_u64.o      \
		   lib/common/com_scsi.o lib/common/com_tag.o      \
		   lib/common/com_sgd.o  lib/common/com_nvram.o    \
		   linux/hba_priv.o	hba_sm/cc.o hba_sm/hba_api.o

OSDEP_OBJS      := linux/linux_main.o    linux/hba_exp.o          \
		   linux/hba_mod.o       linux/hba_timer.o        \
		   linux/mv_os.o   linux/linux_iface.o

# for partial source tree building
include_dir := include  include/generic  include/icommon

header_dirs     :=   $(include_dir)

header_file_$(SUPPORT_ATHENA) :=mv_config_athena.h
header_file_$(SUPPORT_SP2) :=mv_config_sp2.h

LIBMV_OBJS	:=  $(COMM_OBJS) $(CORE_OBJS)

HBA_OBJS        := $(OSDEP_OBJS) $(LIBMV_OBJS)

ifneq ("arm", "$(ARCH)")
INCLUDE_DIR     = -I$(KERNEL_BLD_DIR)/include                          \
		  -I$(KERNEL_BLD_DIR)/include/scsi                     \
		  -I$(KERNEL_BLD_DIR)/drivers/scsi                     \
	 	  -I$(KERNEL_SRC_DIR)/include                          \
	 	  -I$(KERNEL_SRC_DIR)/include/scsi                     \
	 	  -I$(KERNEL_SRC_DIR)/drivers/scsi
endif


ifneq ($(KERNELRELEASE),)

ifeq ($(SUPPORT_ATHENA),y)
obj-m         :=  mv14xx.o
mv14xx-objs   :=   $(HBA_OBJS)
endif

ifeq ($(SUPPORT_SP2),y)
obj-m         :=  mv_sp2.o
mv_sp2-objs   :=   $(HBA_OBJS)
endif

clean-files   +=   Modules.symvers
clean-files   +=   Module.symvers modules.order
EXTRA_CFLAGS  :=   -I$(src)/include   -I$(src)/core  \
		   -I$(src)/include   -I$(src)/core/api  \
		   -I$(src)/include   -I$(src)/core/core  \
		   -I$(src)/include   -I$(src)/core/pl  \
		   -I$(src)/include   -I$(src)/core/pm  \
		   -I$(src)/include   -I$(src)/core/sas_base  \
		   -I$(src)/include   -I$(src)/core/sas_smp  \
		   -I$(src)/include   -I$(src)/core/sata_base  \
		   -I$(src)/include   -I$(src)/core/scsi  \
		   -I$(src)/.               \
		   -I$(src)/linux  -I$(src)/hba_sm -I$(src)/math            \
                   -I$(src)/include/generic \
		   -I$(src)/include/icommon

ifeq ($(SUPPORT_ATHENA),y)
EXTRA_CFLAGS += -I$(src)/product/athena
endif

ifeq ($(SUPPORT_SP2),y)
EXTRA_CFLAGS += -I$(src)/product/sp2
endif

EXTRA_CFLAGS  +=  -D__MV_LINUX__ $(INCLUDE_DIR) -std=gnu99 #-D__MV_DEBUG__

ifeq ($(CONFIG_64BIT), y)
EXTRA_CFLAGS  +=   -D_64_SYS_
else
EXTRA_CFLAGS  +=   -D_32_LEGACY_
endif

EXTRA_CFLAGS  +=   -D_64_BIT_COMPILER

EXTRA_CFLAGS  +=   -include $(src)/mv_config.h

ifeq ($(SUPPORT_ATHENA), y)
EXTRA_CFLAGS  +=   -DPRODUCTNAME_ATHENA
endif

ifeq ($(SUPPORT_SP2), y)
EXTRA_CFLAGS  +=   -DPRODUCTNAME_SP2
endif

ifeq ($(CONFIG_SUSE_KERNEL), y)
EXTRA_CFLAGS  += -DIS_OPENSUSE_SLED_SLES=1
else
EXTRA_CFLAGS	+= -DIS_OPENSUSE_SLED_SLES=0
endif

EXTRA_CFLAGS  +=  -Wno-unused-variable
EXTRA_CFLAGS  += -D__LEGACY_OSSW__=1 -D_OS_LINUX

else

# Why use SUBDIRS? for backward compatibility
all:
	$(MAKE) ARCH=$(ARCH_TYPE) CC=$(MV_CC) LD=$(MV_LD) CROSS_COMPILE=$(CROSS_COMPILE) V=$(V) -C $(KERNEL_BLD_DIR) M=$(CURDIR) modules
#QNAP Patch
export INSTALL_MOD_DIR = $(ROOTFS1)/misc
install:
	@ $(MAKE) -C $(KERNEL_BLD_DIR) M=$(shell pwd) modules_install
##
lib: all
	@mkdir -p mv_lib
	@$(LD) -S -r -o $(MV_LIB_NAME).o $(LIBMV_OBJS)
	@strip --strip-debug $(MV_LIB_NAME).o
	@cp -rf $(MV_LIB_NAME).o ./mv_lib/$(MV_LIB_NAME).obj

partial:
	@echo
	@echo -n "Creating partial source tree ... "
	@rm -rf partial
	@mkdir -p partial partial/lib partial/include
	@cp Makefile.partial partial/Makefile
	@cp patch.kbuild partial/
	@cp patch.sh partial/
	@cp Makefile.kbuild.partial partial/Makefile.kbuild
	@cp -uf ./mv_lib/* ./partial/lib/
	@cp -uf ./readme ./partial/readme
	@cp -uf linux/*.[ch] ./partial
	@cp -uf hba_sm/cc.h ./partial
	@cp -uf hba_sm/hba_api.h ./partial
	@cp -uf hba_sm/mv_hba.h ./partial
	@rm -rf ./partial/hba_priv.c
	@cp -uf include/com_list.c ./partial/include
	@cp -uf mv_conf.mk ./partial
	@cp -uf mv_config.h ./partial
	@cp -uf mv_cache.h ./partial
	@cp -uf mv_include.h ./partial/include
	@cp -uf mv_linux.h ./partial
	@if [ "x${SUPPORT_ATHENA}" == "xy" ]; then cp product/athena/mv_product_athena.h ./partial;cp -uf mv_config_athena.h ./partial;fi
	@
	@for i in $(header_dirs); do cp -uf $$i/*.h  ./partial/include; done
	@if [ "x$$?" == "x0" ];then echo done.;else echo failed.;fi
	@echo

kbuild:
	-@cat patch.kbuild | patch -p1 -N -d $(KERNEL_SRC_DIR)/drivers/scsi
	@sh patch.sh $(KERNEL_SRC_DIR)/drivers/scsi a
	@mkdir -p $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@cp -r * $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@cp -f Makefile.kbuild $(KERNEL_SRC_DIR)/drivers/scsi/mv/Makefile
	@echo done.

ukbuild:
	-@cat patch.kbuild | patch -p1 -N -R -d $(KERNEL_SRC_DIR)/drivers/scsi
	@sh patch.sh $(KERNEL_SRC_DIR)/drivers/scsi
	@rm -rf $(KERNEL_SRC_DIR)/drivers/scsi/mv
	@echo Patch has been successfully rewinded.

nonraid:
	@echo -n "Creating non-RAID source tree ... "
	@rm -rf thor odin vanir norse magni
	@mkdir norse
	@mkdir -p norse/linux norse/hba_sm norse/lib/common norse/core norse/product
	@mkdir -p norse/core/api norse/core/core norse/core/pl norse/core/pm
	@mkdir -p norse/core/sas_base norse/core/sas_smp norse/core/sata_base norse/core/scsi
	@mkdir -p norse/include/icommon norse/include/generic
	@
	@cp -uf linux/*.[ch] norse/linux
	@cp -uf hba_sm/*.[ch] norse/hba_sm
	@cp -uf lib/common/*.[ch] norse/lib/common
	@cp -uf include/*.[ch] norse/include/
	@cp -uf include/icommon/*.[ch] norse/include/icommon
	@cp -uf include/generic/*.[ch] norse/include/generic
	@cp -uf patch.* norse
	@cp -uf readme* norse
	@cp -uf mv_cache.h norse
	@cp -uf mv_linux.h norse
	@cp -uf Makefile.kbuild norse
	@cp -uf Makefile.nonraid norse/Makefile
	@cp -uf mv_config.h norse
	@cp -uf mv_include.h norse
	@
	@if [ "x${SUPPORT_ATHENA}" == "xy" ]; then cp core/api/*.[ch] norse/core/api; cp core/core/*.[ch] norse/core/core; rm -rf norse/core/core/core_xor.[ch]; cp core/pl/*.[ch] norse/core/pl; rm -rf norse/core/pl/core_swxor.c; cp core/pm/*.[ch] norse/core/pm; cp core/sas_base/*.[ch] norse/core/sas_base; cp core/sas_smp/*.[ch] norse/core/sas_smp; cp core/sata_base/*.[ch] norse/core/sata_base; cp core/scsi/*.[ch] norse/core/scsi; mkdir norse/product/athena; cp product/athena/*.[ch] norse/product/athena; rm -rf norse/product/athena/pl_xor.[ch]; cp mv_config_athena.h norse;fi
	@if [ "x${SUPPORT_ATHENA}" == "xy" ]; then echo "SUPPORT_ATHENA=y" > norse/mv_conf.mk;mv norse athena;fi
	@echo "done"

clean:
	$(MAKE) ARCH=$(ARCH_TYPE) CC=$(MV_CC) LD=$(MV_LD) CROSS_COMPILE=$(CROSS_COMPILE) V=$(V) -C $(KERNEL_BLD_DIR) M=$(CURDIR) clean
	@rm -rf partial
	@rm -rf mv_lib
	@rm -rf *.o *.obj
endif
