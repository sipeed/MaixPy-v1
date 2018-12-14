#
# Makefile for maixpy
# ports/k210-freetos/mpy_support/Makefile
#
.PHONY:all update_mk compile
CUR_DIR_ADDR := $(shell pwd)/
###############################################################################
# USER OPTION
LIB_NAME ?= api
OUTPUT_DIR := build/
CROSS_COMPILE ?= 
MK_VALUE :=
MK_VALUE +="INC += "$(CUR_DIR_ADDR)".""\n"
MK_VALUE +="INC += "$(CUR_DIR_ADDR)"include""\n"
MK_VALUE +="liba-sipeed_api += "$(CUR_DIR_ADDR)$(OUTPUT_DIR)"api.a""\n"
MK_VALUE +="output-file += "'$$(liba-sipeed_api)'"\r\n"
OUTPUT_STATIC_LIB := $(LIB_NAME).a
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
###############################################################################
# IMPORT MK FILE
# sinclude mk MICROPYTHON NEED OTHER MK,SO I INCLUDE THE MK WHERE IS IN ROOT DIR
sinclude ../mk
###############################################################################
# IMPORT PLATFORM OPTIONS
sinclude $(PLATFORM).mk
###############################################################################
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
define update_sub_mk
	if [ "$(dir $(1))" != "./" ] ; \
	then \
		$(MAKE) -C $(1) update_mk CROSS_COMPILE=$(CROSS_COMPILE) PLATFORM=$(PLATFORM); \
	fi;
endef
###############################################################################

GET_SUBDIRS1 := $(shell find . -maxdepth 1 -type d)
GET_SUBDIRS2 := $(basename $(patsubst ./%,%,$(GET_SUBDIRS1)))
SUBDIRS := $(GET_SUBDIRS2)

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


# AllDirs := $(shell ls -R | grep '^\./.*:$$' | awk '{gsub(":","");print}') .
FILE_MAKEFILE := $(foreach n,$(SUBDIRS) , $(wildcard $(n)/Makefile))

all: compile
	$(call mod_echo_func,"CUR_DIR "$(CUR_DIR)" ...")

$(OUTPUT_STATIC_LIB):$(CXX_OBJ) $(SRC_C_OBJ)
	$(Q)$(AR) rcs -o $(OUTPUT_DIR)$@ $(addprefix $(OUTPUT_DIR), $(SRC_C_OBJ) $(CXX_OBJ))

compile: $(OUTPUT_STATIC_LIB)
# $(foreach n,$(FILE_MAKEFILE),$(call sub_make,$(n)))

update_mk:
	$(foreach n,$(FILE_MAKEFILE),$(call update_sub_mk, $(dir $(n))))
	$(shell echo $(MK_VALUE)$(foreach nn,$(foreach n,$(SUBDIRS) , $(wildcard $(n)/mk)),"-include "$(CUR_DIR_ADDR)$(nn)"\n") > mk)
clean:
	rm -f mk
	rm -rf build
	$(call mod_echo_func,"Cleaning "$(SUBDIRS)" ...")
	$(foreach n,$(FILE_MAKEFILE),$(call sub_clean, $(dir $(n))))

