#
# Makefile for maixpy
# ports/k210-freetos/thirdpart/spiffs/Makefile
#
.PHONY:all include_mk compile
#############################################################################
# common.mk, include include.mk & platform.mk
sinclude $(COMMON_MK)

#############################################################################
# USER OPTION
LIB_NAME ?= spiffs
OUTPUT_DIR := build
OUTPUT_BIN := $(BIN_DIR)/$(LIB_NAME).a
INCLUDE := -I $(CUR_DIR) -I $(CUR_DIR)/spiffs/src -I $(CUR_DIR)/spiffs/src/default -I $(CUR_DIR)/spiffs-port/include 
SPIFFS_SRC_ROOT_DIR := spiffs
spiffs := $(SPIFFS_SRC_ROOT_DIR)
sinclude spiffs/files.mk
#######################################################################
# IMPORT PLATFORM OPTIONS
SRC_C += $(wildcard spiffs-port/*.c)
SRC_C += $(wildcard spiffs/src/*.c)
C_OBJ :=  $(SRC_C:.c=.o)
sinclude $(INC_DIR)/drivers.mk
CFLAGS += $(INCLUDE_ALL)

$(C_OBJ):%.o:%.c
	@mkdir -p $(dir $(OUTPUT_DIR)$@) 
	@$(CC)  -o $(OUTPUT_DIR)$@ -c $< $(CFLAGS)

all: compile

$(OUTPUT_BIN):$(C_OBJ)
	$(Q)$(AR) rcs -o $@ $(addprefix $(OUTPUT_DIR), $(C_OBJ))

compile: $(OUTPUT_BIN)

include_mk:
	echo "INCLUDE_ALL += \\" > $(INCLUDE_MK)
	echo $(INCLUDE)"\\" >> $(INCLUDE_MK)
	echo " " >> $(INCLUDE_MK)
	$(foreach n,${SUBDIRS},make -C ${n} include_mk; cat ${n}/include.mk >> ${INCLUDE_MK} ;)
	cp $(INCLUDE_MK) $(INC_DIR)/$(LIB_NAME).mk

clean:
	$(call echo_info,"Cleaning "$(SUBDIRS)" ...")
	rm -rf $(OUTPUT_DIR)

