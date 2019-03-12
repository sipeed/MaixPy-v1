#
# Common Makefile part for maixpy
# ports/k210-freertos/common.mk
#
###############################################################################
# basic & header & platform options
CUR_DIR := $(shell pwd)
sinclude $(INCLUDE_MK)	
sinclude $(PLATFORM_MK)
SUBDIRS := $(shell find . -maxdepth 1 -type d)
SUBDIRS := $(basename $(patsubst ./%,%,$(SUBDIRS)))
FILE_MAKEFILE := $(foreach n,$(SUBDIRS) , $(wildcard $(n)/Makefile))
SUBDIRS := $(foreach n,$(FILE_MAKEFILE), $(dir $(n)) )
###############################################################################
# Terminal /Color 
RED='\e[1;31m'
GREEN='\e[1;32m'
YELLOW='\e[1;33m' 
BLUE='\e[1;34m' 
NC='\e[0m'
define echo_info
	@echo -e "${BLUE}${LIB_NAME} "$1"${NC}";
endef
define echo_note
	@echo -e "${BLUE}"$1"${NC}";
endef
define echo_warn
	@echo -e "${BLUE}"$1"${NC}";
endef
define echo_err
	@echo -e "${BLUE}"$1"${NC}";
endef

###############################################################################
