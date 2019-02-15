#!/bin/bash
export PROJECT_DIR=/media/neucrack/data/main/work/sipeed/k210/code/MaixPy_Internal/ports/k210-freertos
export TOOLCHAIN_PATH=/media/neucrack/data/main/work/sipeed/k210/tools/toolchain/kendryte-toolchain
export PLATFORM=k210
export BIN_DIR=`pwd`/output
export INC_DIR=`pwd`/inc
export SHELL=/bin/bash

# don't edit
export PLATFORM_MK=`pwd`/${PLATFORM}.mk
export INCLUDE_MK=include.mk
export COMMON_MK=`pwd`/common.mk
export COMMON_C_MK=`pwd`/common_c.mk
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${TOOLCHAIN_PATH}/bin
export CROSS_COMPILE=${TOOLCHAIN_PATH}/bin/riscv64-unknown-elf-
export TOOLCHAIN_LIB_DIR=${TOOLCHAIN_PATH}/lib/gcc/riscv64-unknown-elf/8.2.0

echo "config ok!"
