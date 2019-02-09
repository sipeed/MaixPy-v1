#!/bin/bash
export PROJECT_DIR=/home/zepan/develop/MAIX/MaixPy/ports/k210-freertos
export TOOLCHAIN_PATH=/opt/riscv-toolchain
export PLATFORM=k210
export BIN_DIR=`pwd`/output
export INC_DIR=`pwd`/inc

# don't edit
export PLATFORM_MK=`pwd`/${PLATFORM}.mk
export INCLUDE_MK=include.mk
export COMMON_MK=`pwd`/common.mk
export COMMON_C_MK=`pwd`/common_c.mk
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${TOOLCHAIN_PATH}/bin
export CROSS_COMPILE=${TOOLCHAIN_PATH}/bin/riscv64-unknown-elf-
export TOOLCHAIN_LIB_DIR=${TOOLCHAIN_PATH}/lib/gcc/riscv64-unknown-elf/8.2.0

echo "config ok!"
