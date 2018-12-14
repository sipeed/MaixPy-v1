#
# Makefile for maixpy
# ports/k210-freetos/mpy_support/Makefile
#
.PHONY:all update_mk compile
CUR_DIR_ADDR := $(shell pwd)/
###############################################################################
# USER OPTION
LIB_NAME ?= spiffs
CROSS_COMPILE ?= 
OUTPUT_DIR := build/
MK_VALUE :="INC += "$(CUR_DIR_ADDR)"\n"
MK_VALUE +="INC += "$(CUR_DIR_ADDR)"spiffs/src""\n"
MK_VALUE +="INC += "$(CUR_DIR_ADDR)"spiffs/src/default""\n"
MK_VALUE +="INC += "$(CUR_DIR_ADDR)"spiffs-port/include""\n"
MK_VALUE +="liba-spiffs += "$(CUR_DIR_ADDR)$(OUTPUT_DIR)"spiffs.a""\n"
MK_VALUE +="output-file += "'$$(liba-spiffs)'"\r\n"
OUTPUT_STATIC_LIB := $(LIB_NAME).a
SPIFFS_SRC_ROOT_DIR := spiffs
spiffs := $(SPIFFS_SRC_ROOT_DIR)
CXX := $(CROSS_COMPILE)c++
CC := $(CROSS_COMPILE)gcc
AR := $(CROSS_COMPILE)ar
###############################################################################
# Optional options
PLATFORM ?=
###############################################################################
# COMPILE OPTIONS
CFLAGS :=
LDFLAGS :=
CXXFLAGS :=
INC :=
SRC_C :=
C_OBJ :=
CPATH :=
###############################################################################
# IMPORT MK FILE
sinclude spiffs/files.mk
sinclude mk
###############################################################################
# IMPORT PLATFORM OPTIONS
sinclude $(PLATFORM).mk
###############################################################################
# SOME FUNCTIONS
define mod_echo_func
	@echo "\033[;34m"$(1)"\033[;34m";
endef
define sub_clean
	$(MAKE) -C $(1) clean CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM=$(PLATFORM);
endef
define sub_make
	$(call mod_echo_func,$(dir $(1)))
	$(MAKE) -C $(basename $(patsubst %/,%,$(patsubst ./%,%,$(dir $(1))))) all CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM=$(PLATFORM)
endef

SRC_C += $(addprefix $(CPATH)/, $(CFILES))
SRC_C += spiffs-port/spiffs-port.c
C_OBJ :=  $(SRC_C:.c=.o)

INCLUDE_FLAGS := $(addprefix -I, $(INC))

CFLAGS += $(INCLUDE_FLAGS)

$(C_OBJ):%.o:%.c
	@mkdir -p $(dir $(OUTPUT_DIR)$@) 
	@$(CC)  -o $(OUTPUT_DIR)$@ -c $< $(CFLAGS)

all: compile

$(OUTPUT_STATIC_LIB):$(C_OBJ)
	$(Q)$(AR) rcs -o $(OUTPUT_DIR)$@ $(addprefix $(OUTPUT_DIR), $(C_OBJ))

compile: $(OUTPUT_STATIC_LIB)
	$(call mod_echo_func,"AllDirs "$(AllDirs)" ...")
# $(foreach n,$(FILE_MAKEFILE),$(call sub_make,$(n)))

update_mk:
	$(shell echo $(MK_VALUE)$(foreach nn,$(foreach n,$(SUBDIRS) , $(wildcard $(n)/mk)),"-include "$(CUR_DIR_ADDR)$(nn)"\n") > mk)

clean:
	rm -f mk
	rm -rf build

