#
# Makefile for maixpy
# ports/k210-freetos/Makefile
#
.PHONY: all clean compile do_mk update_all_platform_mk
CUR_DIR_ADDR := $(shell pwd)/
###############################################################################
# USER OPTION
LIB_NAME ?= Maixpy
CROSS_COMPILE ?= 
OUTPUT_DIR := build/
MK_VALUE :="INC += "".""\n"
OUTPUT_BIN := Maixpy.bin
CC := $(CROSS_COMPILE)gcc
CXX ?= $(CROSS_COMPILE)c++
AR ?= $(CROSS_COMPILE)AR
OBJCOPY ?= $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size
CRT_DIR_ADDR ?=$(dir $(CROSS_COMPILE))/../lib/gcc/riscv64-unknown-elf/8.2.0/
###############################################################################
# Optional options
PLATFORM ?=
###############################################################################
# COMPILE OPTIONS
CFLAGS :=
LDFLAGS :=
CXXFLAGS :=
INC :=
output-file :=
platform-lds :=
###############################################################################
sinclude mk
sinclude $(PLATFORM).mk

define sub_clean
	if [ "$(1)" != " mpy_support/" ] ; \
	then \
	$(MAKE) -C $(1) clean CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM=$(PLATFORM); \
	else \
	$(MAKE) -C $(1) clean_all CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM=$(PLATFORM); \
	fi;
endef

define sub_make
	if [ "$(1)" != " mpy_support/" ] ; \
	then \
		$(MAKE) -C $(1) all CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM=$(PLATFORM); \
	fi;
endef

define mod_echo_func
	@echo "\033[;31m"$(1)"\033[;31m";
endef

define update_sub_platform_mk
	if [ "$(dir $(1))" != "./" ] ; \
	then \
		cat $(PLATFORM).mk>$(1); \
	fi;
endef

define update_sub_mk
	if [ "$(dir $(1))" != "./" ] ; \
	then \
		$(MAKE) -C $(1) update_mk CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM=$(PLATFORM); \
	fi;
endef

GET_SUBDIRS1 := $(shell find . -maxdepth 1 -type d)
GET_SUBDIRS2 := $(basename $(patsubst ./%,%,$(GET_SUBDIRS1)))
SUBDIRS := $(GET_SUBDIRS2)

AllDirs := $(shell ls -R | grep '^\./.*:$$' | awk '{gsub(":","");print}') .
PLATFORM_MK := $(foreach n,$(AllDirs) , $(wildcard $(n)/$(PLATFORM).mk))
FILE_MAKEFILE := $(foreach n,$(SUBDIRS) , $(wildcard $(n)/Makefile))

all: do_mk update_all_platform_mk sub_compile compile $(OUTPUT_BIN) print_bin_size

SRC_C := $(wildcard *.c)
SRC_CPP := $(wildcard *.cpp)
SRC_C_OBJ := $(SRC_C:.c=.o)
CXX_OBJ := $(SRC_CPP:.cpp=.o)
INCLUDE_FLAGS := $(addprefix -I, $(INC))
CFLAGS += $(INCLUDE_FLAGS)
$(CXX_OBJ):%.o:%.cpp
	@mkdir -p $(dir $(OUTPUT_DIR))
	@$(CXX)  -o $(OUTPUT_DIR)/$@ -c $< $(INCLUDE_FLAGS) $(CXXFLAGS) -lstdc++
$(SRC_C_OBJ):%.o:%.c
	@mkdir -p $(dir $(OUTPUT_DIR))
	@$(CC)  -o $(OUTPUT_DIR)/$@ -c $< $(INCLUDE_FLAGS) $(CFLAGS) -lstdc++

SRC_C_OUTPUT_OBJ := $(addprefix $(OUTPUT_DIR), $(SRC_C_OBJ))
SRC_CXX_OUTPUT_OBJ := $(addprefix $(OUTPUT_DIR), $(CXX_OBJ))

CRT_FALGS := $(CRT_DIR_ADDR)crti.o 
CRT_FALGS += $(CRT_DIR_ADDR)crtbegin.o $(SRC_C_OUTPUT_OBJ) $(SRC_CXX_OUTPUT_OBJ)
CRT_FALGS += $(CRT_DIR_ADDR)crtend.o
CRT_FALGS += $(CRT_DIR_ADDR)crtn.o

BIN_LDFLAGS := \
		-nostartfiles \
		-static \
		-Wl,--gc-sections \
		-Wl,-static \
		-Wl,--start-group \
		-Wl,--whole-archive \
		-Wl,--no-whole-archive \
		-Wl,--end-group \
		-Wl,-EL \
		-T \
		$(platform-lds) \
		$(CRT_FALGS) \
		-Wl,--start-group \
		-lm \
		$(SRC_CXX_OUTPUT_OBJ) \
		$(output-file) \
		$(liba-libfreertos) \
		-latomic \
		$(liba-bsp) \
		-lc \
		-lstdc++ \
		$(CXX_OBJ) \
		-Wl,--end-group \
		$(liba-fatfs) \
		$(liba-hal) \
		-lm \
		-lstdc++ \
		-lm

$(LIB_NAME): $(output-file) $(SRC_C_OUTPUT_OBJ) $(SRC_C_OUTPUT_OBJ)
	@$(CC) $(CFLAGS) $(BIN_LDFLAGS) -o $@

print_bin_size:
	$(SIZE) $(LIB_NAME)

$(OUTPUT_BIN):$(LIB_NAME)
	$(OBJCOPY) --output-format=binary $(LIB_NAME) $@

compile:$(SRC_C_OBJ) $(CXX_OBJ)

sub_compile:
	$(call mod_echo_func,"Compiling "$(SUBDIRS)" ...")
	$(foreach n,$(FILE_MAKEFILE),$(call sub_make, $(dir $(n))))
	$(call mod_echo_func,"compile "$(SUBDIRS)" .")
	$(MAKE) -C mpy_support all CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM=$(PLATFORM)

update_mk:
	$(call mod_echo_func,"update mk file"" ...")
	$(foreach n,$(FILE_MAKEFILE),$(call update_sub_mk, $(dir $(n))))
	$(shell echo $(MK_VALUE)$(foreach nn,$(foreach n,$(SUBDIRS) , $(wildcard $(n)/mk)),"-include "$(CUR_DIR_ADDR)$(nn)"\n") > mk)

update_all_platform_mk:
	$(call mod_echo_func,"PLATFORM_MK"$(PLATFORM_MK)" ...")
	if [ -f $(PLATFORM).mk ] ; \
	then \
	     $(foreach n,$(PLATFORM_MK),$(call update_sub_platform_mk, $(n))) \
	fi;
	
clean:
	$(call mod_echo_func,"Cleaning "$(SUBDIRS)" ...")
	$(foreach n,$(FILE_MAKEFILE),$(call sub_clean, $(dir $(n))))
	rm -f mk
	rm -rf build
	rm -rf $(LIB_NAME) $(OUTPUT_BIN)


