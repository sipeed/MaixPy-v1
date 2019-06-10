#
# Makefile for maixpy
# ports/k210-freetos/mpy_support/Makefile
#
.PHONY:all include_mk compile 
##############################################################################
# common.mk, include include.mk & platform.mk
sinclude $(COMMON_MK)
sinclude $(MAIXPY_MODULES_CONFIG_MK)
CFLAGS += -DFFCONF_H=\"lib/oofatfs/ffconf.h\"
include ../../../py/mkenv.mk
QSTR_DEFS = qstrdefsport.h
include ../../../py/py.mk
CFLAGS += -I build/
######################################################################
# USER OPTIONS
LIB_NAME ?= mpy_support
OUTPUT_DIR := build
OUTPUT_BIN := $(BIN_DIR)/$(LIB_NAME).a
FROZEN_MPY_DIR ?= builtin-py
SRC_C := 
AllDirs :=

INCLUDE := -I $(CUR_DIR) \
			-I $(CUR_DIR)/Maix/include \
			-I $(CUR_DIR)/network/include/ \
			-I $(CUR_DIR)/standard_lib/include/ \
			-I $(CUR_DIR)/video/include/ \
			-I $(CUR_DIR)/video/avi/include/ \
			-I $(CUR_DIR)/../../../  \
			-I $(CUR_DIR)/omv/include/ \
			-I $(CUR_DIR)/omv/py/include/ \
			-I $(CUR_DIR)/omv/boards/MAIX/ \
			-I $(CUR_DIR)/omv/img/include/ \
			-I $(CUR_DIR)/../../../lib/oofatfs/ \
			-I $(CUR_DIR)/../../../py/ \
			-I $(CUR_DIR)/../../../lib/mp-readline/ \
			-I $(CUR_DIR)/$(OUTPUT_DIR) \
			-I $(CUR_DIR)/audio/include/ \
			-I $(CUR_DIR)/audio/wav/include/ \
			-I $(CUR_DIR)/../../platform/utils/include/ \




######################################################################
# MPY OPTIONS
sinclude $(INC_DIR)/sdk.mk
MPY_EXT_SRC_C := \
		lib/utils/stdout_helpers.c \
		lib/utils/interrupt_char.c \
		lib/utils/pyexec.c \
		lib/libc/string0.c \
		lib/mp-readline/readline.c \
		lib/netutils/netutils.c \
		lib/timeutils/timeutils.c \
		lib/utils/sys_stdio_mphal.c \
		lib/oofatfs/ff.c \
		lib/oofatfs/option/ccsbcs.c
AllDirs += $(shell ls -R | grep '^\./.*:$$' | grep -v "lvgl" |grep -v "nes" | grep -v "video" | grep -v "touchscreen" |grep -v "./build" | awk '{gsub(":","");print}') .

###### LVGL #######
ifeq (${CFG_MAIXPY_LVGL_SUPPORT},y)

LVGL_DIR := $(CUR_DIR)/lvgl
LVGL_DIR_REL := ./lvgl

LVGL_BINDING_DIR := $(LVGL_DIR)/lv_bindings
LVGL_CORE_DIR := $(LVGL_BINDING_DIR)/lvgl
LVGL_CORE_DIR_REL := ./lvgl/lv_bindings/lvgl
LVGL_BUILD_DIR := $(OUTPUT_DIR)/lvgl
ALL_LVGL_SRC := \
			${wildcard ${LVGL_DIR_REL}/*.c} \
			${wildcard ${LVGL_CORE_DIR_REL}/*.c} \
			${wildcard ${LVGL_CORE_DIR_REL}/lv_core/*.c} \
			${wildcard ${LVGL_CORE_DIR_REL}/lv_draw/*.c} \
			${wildcard ${LVGL_CORE_DIR_REL}/lv_fonts/*.c} \
			${wildcard ${LVGL_CORE_DIR_REL}/lv_hal/*.c} \
			${wildcard ${LVGL_CORE_DIR_REL}/lv_misc/*.c} \
			${wildcard ${LVGL_CORE_DIR_REL}/lv_objx/*.c} \
			${wildcard ${LVGL_CORE_DIR_REL}/lv_themes/*.c} 

LVGL_INCLUDE := \
			-I $(LVGL_DIR)/include \
			-I $(LVGL_BINDING_DIR)/include \
			-I $(LVGL_CORE_DIR) \
			-I $(LVGL_CORE_DIR)/lv_core \
			-I $(LVGL_CORE_DIR)/lv_draw \
			-I $(LVGL_CORE_DIR)/lv_fonts \
			-I $(LVGL_CORE_DIR)/lv_hal \
			-I $(LVGL_CORE_DIR)/lv_misc \
			-I $(LVGL_CORE_DIR)/lv_objx \
			-I $(LVGL_CORE_DIR)/lv_themes  

LVGL_MPY = $(LVGL_BUILD_DIR)/lv_mpy.c
LVGL_PP = $(LVGL_BUILD_DIR)/lvgl.pp.c
INCLUDE += $(LVGL_INCLUDE) 
LV_CFLAGS += -DLV_CONF_INCLUDE_SIMPLE -DMAIXPY_LVGL_SUPPORT=1
CFLAGS_MOD += $(LV_CFLAGS)

$(LVGL_MPY): $(LVGL_BINDING_DIR)/gen/gen_mpy.py 
	$(ECHO) "LVGL-GEN $@"
	$(Q)mkdir -p $(dir $@)
	$(Q)$(CPP) $(LV_CFLAGS) $(INCLUDE) -I $(LVGL_BINDING_DIR)/pycparser/utils/fake_libc_include $(LVGL_CORE_DIR)/lvgl.h > $(LVGL_PP)
	$(Q)$(PYTHON) $(LVGL_BINDING_DIR)/gen/gen_mpy.py -X anim -X group -X task -E $(LVGL_PP) $(LVGL_CORE_DIR)/lvgl.h > $@

SRC_C += $(LVGL_MPY)
SRC_C += $(ALL_LVGL_SRC)

endif
###### LVGL END #######


###### OMV #######
ifeq (${CFG_MAIXPY_OMV_MINIMUM},y)
CFLAGS_MOD += -DMAIXPY_OMV_MINIMUM_FUNCTION=1
endif
###### OMV END ####

###### VIDEO #######
ifeq (${CFG_MAIXPY_VIDEO_SUPPORT},y)
CFLAGS_MOD += -DMAIXPY_VIDEO_SUPPORT=1
VIDEO_DIR_REL := ./video
VIDEO_DIR := $(CUR_DIR)/video
SRC_VIDEO := \
			${wildcard ${VIDEO_DIR_REL}/*.c} \
			${wildcard ${VIDEO_DIR_REL}/avi/*.c} 

VIDEO_INCLUDE := \
			-I $(VIDEO_DIR)/include \
			-I $(VIDEO_DIR)/avi/include 

INCLUDE += $(VIDEO_INCLUDE)
SRC_C += $(SRC_VIDEO)
endif
###### VIDEO END ####

###### M5Stick #######
ifeq (${CFG_MAIXPY_M5STICK},y)
CFLAGS_MOD += -DMAIXPY_M5STICK=1
endif
######################

###### NES #######
ifeq (${CFG_MAIXPY_NES_SUPPORT},y)
CFLAGS_MOD += -DMAIXPY_NES_EMULATOR_SUPPORT=1
NES_DIR_REL := ./nes
NES_DIR := $(CUR_DIR)/nes
SRC_NES := \
			${wildcard ${NES_DIR_REL}/*.c} 

NES_INCLUDE := \
			-I $(NES_DIR)/include 

INCLUDE += $(NES_INCLUDE)
SRC_C += $(SRC_NES)
endif
###### NES END ####

##### TOUCHSCREEN #####
ifeq (${CFG_MAIXPY_TOUCH_SCREEN_SUPPORT},y)
TOUCHSCREEN_ROOT_DIR := $(CUR_DIR)/touchscreen
TOUCHSCREEN_ROOT_DIR_REL := ./touchscreen
CFLAGS_MOD += -DMAIXPY_TOUCH_SCREEN_SUPPORT=1
TOUCHSCREEN_INLCUDE := -I $(TOUCHSCREEN_ROOT_DIR)/include

ifeq (${CFG_MAIXPY_TOUCH_SCREEN_DRIVER},ns2009)
TOUCHSCREEN_DRIVER_DIR_REL := ./touchscreen/ns2009
TOUCHSCREEN_DRIVER_DIR := $(CUR_DIR)/touchscreen/ns2009
TOUCHSCREEN_INLCUDE += \
					-I $(TOUCHSCREEN_DRIVER_DIR)/include 

endif
SRC_TOUCHSCREEN := \
			${wildcard ${TOUCHSCREEN_ROOT_DIR_REL}/*.c} \
			${wildcard ${TOUCHSCREEN_DRIVER_DIR_REL}/*.c}

INCLUDE += $(TOUCHSCREEN_INLCUDE)
SRC_C += $(SRC_TOUCHSCREEN)
endif
#######################



#$(BUILD)/_frozen_mpy.c:  $(BUILD)/genhdr/qstrdefs.generated.h
#	$(ECHO) "MISC freezing bytecode"
#	$(ECHO) $<
#	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=mpz $< frozentest.mpy > $@


######################################################################
# COMPILE CUR SRC FILE
sinclude $(INC_DIR)/sdk.mk
sinclude $(INC_DIR)/spiffs.mk
sinclude $(INC_DIR)/platform.mk
CFLAGS+=$(INCLUDE) $(INCLUDE_ALL)
TEMP_DIRS := $(subst build,,$(AllDirs))
SRC_C += $(foreach n,$(TEMP_DIRS) , $(wildcard $(n)/*.c))
SRC_CPP := $(foreach n,$(TEMP_DIRS) , $(wildcard $(n)/*.cpp))
SRC_C += $(MPY_EXT_SRC_C) 
SRC_QSTR += $(SRC_C)
SRC_C_ALL := $(SRC_C)
SRC_C_ALL += $(BUILD)/frozen_mpy.c 
C_OBJ := $(addprefix $(OUTPUT_DIR)/, $(SRC_C_ALL:.c=.o))
OBJ := $(C_OBJ) $(PY_O) 
# USE MPY COMPILE SRICPT COMPILE SRC FILE
CXX_OBJ := $(addprefix $(OUTPUT_DIR)/, $(SRC_CPP:.cpp=.o))
CFLAGS += $(CFLAGS_MOD)
# List of sources for qstr extraction
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

$(CXX_OBJ):%.o:%.cpp
	$(Q)$(CXX)  -o $(OUTPUT_DIR)/$@ -c $< -I$(INC) $(CXXFLAGS) -lstdc++

MAKE_J_NUMBER=$(shell cat /proc/cpuinfo | grep vendor_id | wc -l)
# $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstrdefs.generated.h $(BUILD)/_frozen_mpy.c
compile: $(BUILD)/genhdr/qstrdefs.generated.h mkbuild $(OUTPUT_BIN)
	$(call echo_info,"Making compile ...")
	$(foreach n,${SUBDIRS},make -j${MAKE_J_NUMBER} -C ${n} compile || exit "$$?";)

$(OUTPUT_BIN): $(OBJ) $(CXX_OBJ) 
	@$(AR) rcs -o $@ $(CXX_OBJ) $(OBJ) $(LIBS) $(LDFLAGS)

mkbuild:
	@mkdir -p $(BUILD)

all: compile
	
include_mk:
	echo "INCLUDE_ALL += \\" > $(INCLUDE_MK)
	echo $(INCLUDE)"\\" >> $(INCLUDE_MK)
	echo " " >> $(INCLUDE_MK)
	$(foreach n,${SUBDIRS},make -C ${n} include_mk; cat ${n}/include.mk >> ${INCLUDE_MK} ;)
	cp $(INCLUDE_MK) $(INC_DIR)/$(LIB_NAME).mk

clean_all: clean
	$(call mod_echo_func,"Cleaning "$(SUBDIRS)" ...")
	$(foreach n,$(SUBDIRS),make -C ${n} clean;))

include $(TOP)/py/mkrules.mk
