
# Makefile for maixpy
# ports/k210-freetos/Makefile
#
.PHONY: all clean compile
###############################################################################
# common.mk, include include.mk & platform.mk
sinclude $(COMMON_MK)
###############################################################################
# USER OPTION
LIB_NAME ?= maixpy
OUTPUT_DIR := build
OUTPUT_ELF := $(BIN_DIR)/$(LIB_NAME).elf
OUTPUT_BIN := $(BIN_DIR)/$(LIB_NAME).bin


###############################################################################
# USER CONFIG MAKE METHOD
sinclude ${INCLUDE_MK}
CFLAGS += $(INCLUDE_ALL)
CFLAGS += -DFFCONF_H=\"lib/oofatfs/ffconf.h\" 
sinclude $(COMMON_C_MK)

ALL_LIBS := $(wildcard $(BIN_DIR)/*.a)
$(info ${ALL_LIBS}...................)
# BIN_LDFLAGS += $(SRC_C_OUTPUT_OBJ) $(SRC_CXX_OUTPUT_OBJ)

all: include_mk compile out print_size	

print_size:
	$(SIZE) $(OUTPUT_ELF)

$(OUTPUT_ELF): $(SRC_C_OUTPUT_OBJ) $(SRC_C_OUTPUT_OBJ) $(ALL_LIBS)
	@mkdir -p $(BIN_DIR)
	@echo $(BIN_LDFLAGS)
	@$(CC) $(BIN_LDFLAGS) $(CFLAGS) -o $@

out:$(OUTPUT_ELF)
	$(OBJCOPY) --output-format=binary $(OUTPUT_ELF) $(OUTPUT_BIN)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	MAKE_J_NUMBER := $(shell cat /proc/cpuinfo | grep vendor_id | wc -l)
endif
ifeq ($(UNAME_S),Darwin)
	MAKE_J_NUMBER := $(shell sysctl -n hw.ncpu)
endif
subcompile:
	@mkdir -p $(BIN_DIR)
	$(call echo_info,"Making compile ...")
	$(foreach n,${SUBDIRS},make -j${MAKE_J_NUMBER} -C ${n} compile || exit "$$?";)

compile:subcompile

include_mk:
	$(call echo_info,"update include mk file ...")
	@echo " " > ${INCLUDE_MK}
	@echo ${SUBDIRS}
	mkdir -p ${INC_DIR}
	$(foreach n,${SUBDIRS},make -C ${n} include_mk; cat ${n}/include.mk >> ${INCLUDE_MK} ;)
	cp ${INCLUDE_MK} ${INC_DIR}/${LIB_NAME}.mk
	$(call echo_info,"update include mk ok!")
	
clean:
	$(call echo_info,"Cleaning "$(SUBDIRS)" ...")
	$(foreach n,${SUBDIRS},make -C ${n} clean;)
	rm -rf $(OUTPUT_DIR) $(BIN_DIR)
	rm -f $(INCLUDE_MK)
	find ./ -name "include\.mk" | xargs rm -f
	rm -f inc/*.mk







