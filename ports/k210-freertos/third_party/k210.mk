SHELL:=/bin/bash

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
	-O2 \
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

do_mk:
	$(shell touch ./k210_env)
	$(shell echo "export LD_LIBRARY_PATH=$$""LD_LIBRARY_PATH:"$(dir $(CROSS_COMPILE)) > ./k210_env)
	$(call mod_echo_func,"source k210_env"" ...")
	$(call mod_echo_func,"k210_env "$(shell cat ./k210_env)" ...")
	$(shell source ./k210_env)
	source ./k210_env
	

