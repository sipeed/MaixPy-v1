#
# Makefile for maixpy
# ports/k210-freertos/platform/sdk/Makefile
#
.PHONY:all include_mk compile
###############################################################################
# common.mk, include include.mk & platform.mk
sinclude $(COMMON_MK)
###################################
# USER OPTION
LIB_NAME ?= sdk
OUTPUT_DIR :=build
SDK_GIT_ADDR := https://github.com/sipeed/kendryte-standalone-sdk.git
SDK_GIT_BRANCH := Maixpy-dev
SDK_DIR_NAME :=kendryte-standalone-sdk
SDK_DIR := $(CUR_DIR)/$(SDK_DIR_NAME)
INCLUDE := -I "$(SDK_DIR)/lib/bsp/include/" -I "$(SDK_DIR)/lib/drivers/include/" -I "$(SDK_DIR)/lib/bsp/include/" -I "$(SDK_DIR)/lib/bsp/include/" -I "$(SDK_DIR)/lib/freertos/conf/" -I "$(SDK_DIR)/lib/freertos/include/" -I "$(SDK_DIR)/lib/freertos/portable/" -I "$(SDK_DIR)/lib/utils/include/" 

##############################################################################
# USER CONFIG MAKE METHOD
SUBDIRS :=
all: build_env compile update_mk

# cd $(SDK_DIR_NAME) && patch -p1 -u < ../freeRTOS.patch && cd -;
build_env:
	if [ ! -d $(SDK_DIR) ] ; \
	then \
	     git clone -b $(SDK_GIT_BRANCH) $(SDK_GIT_ADDR);\
	fi;

MAKE_J_NUMBER=$(shell cat /proc/cpuinfo | grep vendor_id | wc -l)
compile: build_env
	-mkdir -p build
	cd build && cmake ../$(SDK_DIR_NAME)/ -DPROJ=hello_world -DTOOLCHAIN=$(patsubst %/,%,$(dir $(CROSS_COMPILE))) && make -j${MAKE_J_NUMBER}
	cp ${CUR_DIR}/${OUTPUT_DIR}/lib/libkendryte.a ${BIN_DIR}/

include_mk: build_env
	$(call echo_info,"update include mk file ...")
	echo "INCLUDE_ALL += \\" > $(INCLUDE_MK)
	echo $(INCLUDE)"\\" >> $(INCLUDE_MK)
	echo " " >> $(INCLUDE_MK)
	$(foreach n,${SUBDIRS},make -C ${n} include_mk; cat ${n}/include.mk >> ${INCLUDE_MK} ;)
	cp $(INCLUDE_MK) $(INC_DIR)/$(LIB_NAME).mk

clean:
	rm -rf build
	
	
