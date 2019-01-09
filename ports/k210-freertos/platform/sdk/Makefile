#
.PHONY:all update_mk build_env
###################################
# USER OPTION
LIB_NAME ?= sdk
CROSS_COMPILE ?=
OUTPUT_DIR :=build/
####################################
# Optional options
PLATFORM ?=
####################################
SDK_GIT_ADDR := https://github.com/sipeed/kendryte-standalone-sdk.git
SDK_GIT_BRANCH := Maixpy-dev
CUR_DIR_ADDR := $(shell pwd)/
SDK_DIR_NAME :=kendryte-standalone-sdk
SDK_DIR_ADDR := $(CUR_DIR_ADDR)$(SDK_DIR_NAME)/

MK_VALUE :=
MK_VALUE +="INC += "$(SDK_DIR_ADDR)"lib/bsp/include/""\r\n"
MK_VALUE +="INC += "$(SDK_DIR_ADDR)"lib/drivers/include/""\r\n"
MK_VALUE +="INC += "$(SDK_DIR_ADDR)"lib/freertos/conf/""\r\n"
MK_VALUE +="INC += "$(SDK_DIR_ADDR)"lib/freertos/include/""\r\n"
MK_VALUE +="INC += "$(SDK_DIR_ADDR)"lib/freertos/portable/""\r\n"
MK_VALUE +="INC += "$(SDK_DIR_ADDR)"lib/utils/include/""\r\n"
MK_VALUE +="liba-sdk := "$(CUR_DIR_ADDR)$(OUTPUT_DIR)"lib/libkendryte.a""\r\n"
MK_VALUE +="output-file += "'$$(liba-sdk)'"\r\n"
MK_VALUE +="platform-lds += "$(CUR_DIR_ADDR)$(SDK_DIR_NAME)"/lds/kendryte.ld""\r\n"
####################################

define mod_echo_func
	@echo "\033[;39m"$(1)"\033[;39m";
endef

all: build_env compile update_mk

# cd $(SDK_DIR_NAME) && patch -p1 -u < ../freeRTOS.patch && cd -;
build_env:
	if [ ! -d $(SDK_DIR_NAME) ] ; \
	then \
	     git clone -b $(SDK_GIT_BRANCH) $(SDK_GIT_ADDR);\
	fi;

compile:
	-mkdir build
	MAKE_J_NUMBER=`cat /proc/cpuinfo | grep vendor_id | wc -l`
	cd build && cmake ../$(SDK_DIR_NAME)/ -DPROJ=hello_world -DTOOLCHAIN=$(subst bin/,bin,$(dir $(CROSS_COMPILE))) && make -j${MAKE_J_NUMBER}

update_mk: build_env
	$(call mod_echo_func,"update mk file"" ...")
	$(shell echo $(MK_VALUE) > mk)

clean:
	rm -rf build
	rm -f mk
	
	
