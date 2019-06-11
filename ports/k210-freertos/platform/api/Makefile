#
# Makefile for maixpy
# ports/k210-freertos/platform/api/Makefile
#
.PHONY:all include_mk compile
##############################################################################
# common.mk, include include.mk & platform.mk
sinclude $(COMMON_MK)
##########################################################################
# USER OPTION
LIB_NAME ?= api
OUTPUT_DIR := build
OUTPUT_BIN := $(BIN_DIR)/$(LIB_NAME).a
INCLUDE := -I $(CUR_DIR) \
           -I $(CUR_DIR)/include \
		   -I $(CUR_DIR)/../../../../ \
		   -I $(CUR_DIR)/../../mpy_support/ \
		   -I $(CUR_DIR)/../../mpy_support/build \
		   -I $(CUR_DIR)/../../mpy_support/standard_lib/include


##############################################################################
# USER CONFIG MAKE METHOD
sinclude $(COMMON_C_MK)
sinclude $(INC_DIR)/sdk.mk
CFLAGS += $(INCLUDE)
CFLAGS += $(INCLUDE_ALL)

lib_mic.a:
	wget https://fdvad021asfd8q.oss-cn-hangzhou.aliyuncs.com/Sipeed_M1/software/lib_mic.a
	touch lib_mic.a

lib_sipeed_kpu: 
	@if [ ! -f lib_sipeed_kpu.a ]; then \
		echo "no lib_sipeed_kpu.a"; \
	fi;

all: lib_mic.a lib_sipeed_kpu compile
	$(call echo_info,"CUR_DIR "$(CUR_DIR)" ...")

$(OUTPUT_BIN):$(SRC_C_OBJ) 
	$(Q)$(AR) rcs -o $@ $(SRC_C_OUTPUT_OBJ) 

compile: lib_mic.a lib_sipeed_kpu $(OUTPUT_BIN)	

include_mk:
	echo "INCLUDE_ALL += \\" > $(INCLUDE_MK)
	echo $(INCLUDE)"\\" >> $(INCLUDE_MK)
	echo " " >> $(INCLUDE_MK)
	$(foreach n,${SUBDIRS},make -C ${n} include_mk; cat ${n}/include.mk >> ${INCLUDE_MK} ;)
	cp $(INCLUDE_MK) $(INC_DIR)/$(LIB_NAME).mk
	
clean:
	$(call echo_info,"Cleaning "$(SUBDIRS)" ...")
	$(foreach n,${SUBDIRS},make -C ${n} clean;)
	rm -rf $(OUTPUT_DIR)
