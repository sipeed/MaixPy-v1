#
# Makefile for maixpy
# ports/k210-freetos/mpy_support/Makefile
#
.PHONY:all include_mk compile 
##############################################################################
# common.mk, include include.mk & platform.mk
sinclude $(COMMON_MK)
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
			-I $(CUR_DIR)/nes/include/ \



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
AllDirs := $(shell ls -R | grep '^\./.*:$$' | awk '{gsub(":","");print}') .

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
SRC_C := $(foreach n,$(TEMP_DIRS) , $(wildcard $(n)/*.c))
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

# $(HEADER_BUILD)/qstr.i.last $(HEADER_BUILD)/qstrdefs.generated.h $(BUILD)/_frozen_mpy.c
compile: $(BUILD)/genhdr/qstrdefs.generated.h mkbuild $(OUTPUT_BIN)
	$(call echo_info,"Making compile ...")
	$(foreach n,${SUBDIRS},make -C ${n} compile || exit "$$?";)
	
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
