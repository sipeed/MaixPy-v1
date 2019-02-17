#
# Makefile for maixpy
# ports/k210-freetos/third_party/Makefile
#
.PHONY:all include_mk compile
########################################################################
# common.mk, include include.mk & platform.mk
sinclude $(COMMON_MK)
###################################
# USER OPTION
LIB_NAME ?= third_party

####################################
all: compile

compile:
	$(call echo_info,"Making compile ...")
	$(foreach n,${SUBDIRS},make -C ${n} compile || exit "$$?";)

include_mk:
	echo "INCLUDE_ALL += \\" > $(INCLUDE_MK)
	echo $(INCLUDE)"\\" >> $(INCLUDE_MK)
	echo " " >> $(INCLUDE_MK)
	$(foreach n,${SUBDIRS},make -C ${n} include_mk; cat ${n}/include.mk >> ${INCLUDE_MK} ;)
	cp $(INCLUDE_MK) $(INC_DIR)/$(LIB_NAME).mk

clean:
	$(call mod_echo_func,"Cleaning "$(SUBDIRS)" ...")
	$(foreach n,${SUBDIRS},make -C ${n} clean;)

