#
# Common Makefile part for maixpy
# ports/k210-freertos/common.mk
#
###############################################################################
# basic & header & platform options
SHELL := /bin/bash
CUR_DIR := $(shell pwd)
sinclude $(INCLUDE_MK)	
sinclude $(PLATFORM_MK)
ECHO:=echo -e
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
	@${ECHO} ""
	@${ECHO} ${BLUE}${LIB_NAME}$1${NC};
endef
define echo_note
	@${ECHO} ""
	@${ECHO} ${BLUE}$1${NC};
endef
define echo_warn
	@${ECHO} ""
	@${ECHO} ${BLUE}$1 ${NC};
endef
define echo_err
	@${ECHO} ""
	@${ECHO} ${BLUE}$1 ${NC};
endef

###############################################################################
