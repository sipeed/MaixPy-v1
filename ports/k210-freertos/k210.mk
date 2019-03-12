# SHELL:=/bin/bash
CC = $(CROSS_COMPILE)gcc
CPP = $(CC) -E
CXX = $(CROSS_COMPILE)c++
AR = $(CROSS_COMPILE)ar
OBJCOPY = $(CROSS_COMPILE)objcopy
SIZE = $(CROSS_COMPILE)size

CFLAGS = \
	-DCONFIG_LOG_COLORS \
	-DCONFIG_LOG_ENABLE \
	-DCONFIG_LOG_LEVEL=LOG_INFO \
	-DDEBUG=1 \
	-DLOG_KERNEL \
	-D__riscv64 \
	-mcmodel=medany \
	-march=rv64imafdc \
	-fno-common \
	-ffunction-sections \
	-fdata-sections \
	-fstrict-volatile-bitfields \
	-fno-zero-initialized-in-bss \
	-Os \
	-ffloat-store \
	-ggdb \
	-std=gnu11 \
	-Wall \
	-Wno-error=unused-function \
	-Wno-error=unused-but-set-variable \
	-Wno-error=unused-variable \
	-Wno-error=deprecated-declarations \
	-Wno-error=maybe-uninitialized \
	-Wextra \
	-Werror=frame-larger-than=65536 \
	-Wno-unused-parameter \
	-Wno-unused-function \
	-Wno-implicit-fallthrough \
	-Wno-sign-compare \
	-Wno-error=missing-braces \
	-Wno-old-style-declaration \
	-g \
	-Wno-error=format= \
	-Wno-error=pointer-sign

CXXFLAGS := \
	-DCONFIG_LOG_COLORS \
	-DCONFIG_LOG_ENABLE \
	-DCONFIG_LOG_LEVEL=LOG_INFO \
	-DDEBUG=1 \
	-DLOG_KERNEL \
	-D__riscv64 \
	-mcmodel=medany \
	-march=rv64imafdc \
	-fno-common \
	-ffunction-sections \
	-fdata-sections \
	-fstrict-volatile-bitfields \
	-fno-zero-initialized-in-bss \
	-O2 \
	-ggdb \
	-std=gnu++17 \
	-Wall \
	-Wno-error=unused-function \
	-Wno-error=unused-but-set-variable \
	-Wno-error=unused-variable \
	-Wno-error=deprecated-declarations \
	-Wno-error=maybe-uninitialized \
	-Wextra \
	-Werror=frame-larger-than=65536 \
	-Wno-unused-parameter \
	-Wno-unused-function \
	-Wno-implicit-fallthrough \
	-Wno-sign-compare \
	-Wno-error=missing-braces \
	-g \
	-Wno-error=format= \
	-Wno-error=pointer-sign

platform-lds:=$(PROJECT_DIR)/platform/sdk/kendryte-standalone-sdk/lds/kendryte.ld
toolchain-obj:=$(wildcard $(TOOLCHAIN_LIB_DIR)/*.o)
lib-file:=$(wildcard $(BIN_DIR)/*.a)
# mainobj:=$(wildcard $(PROJECT_DIR)/build/main.o)
mainobj:=$(PROJECT_DIR)/build/main.o
BIN_LDFLAGS := \
                -static \
                -Wl,-static \
		-nostartfiles\
		-Wl,--gc-sections \
		-Wl,-EL \
		-T $(platform-lds) \
		$(mainobj) \
		$(BIN_DIR)/mpy_support.a \
		$(BIN_DIR)/spiffs.a \
		$(BIN_DIR)/drivers.a \
		$(BIN_DIR)/api.a \
		$(BIN_DIR)/libkendryte.a \
		-lm -latomic -lc -lstdc++ \
		-Wl,--start-group \
		-lc \
		-lstdc++ \
		$(BIN_DIR)/libkendryte.a \
		-Wl,--end-group \



