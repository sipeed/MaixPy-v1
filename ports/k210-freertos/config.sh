#!/bin/bash

config_file=config.conf
if [ ! -f $config_file ]; then
    echo "Please config toolchain path first in config.conf"
    echo -e "toolchain_path=/opt/kendryte-toolchain\nbaud=2000000\ndevice=/dev/ttyUSB0\nBoard=dan\n" > config.conf
    exit 1
fi
build_config=`cat $config_file`
toolchain_setting=`echo "${build_config}" |grep toolchain_path`
toolchain_path="/"`echo -e ${toolchain_setting#*/}`

if [ ! -d $toolchain_path ]; then
    echo "can not find toolchain, please set toolchain path in config.conf"
    echo -e "toolchain_path=/opt/kendryte-toolchain\nbaud=2000000\ndevice=/dev/ttyUSB0\nBoard=dan\n" > config.conf
    exit 1
fi

cwd=`pwd`
export PROJECT_DIR=$cwd
export TOOLCHAIN_PATH=${toolchain_path}
export PLATFORM=k210
export BIN_DIR=`pwd`/output
export INC_DIR=`pwd`/inc

export SHELL=/bin/bash
export PLATFORM_MK=`pwd`/${PLATFORM}.mk
export INCLUDE_MK=include.mk
export COMMON_MK=`pwd`/common.mk
export COMMON_C_MK=`pwd`/common_c.mk
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${TOOLCHAIN_PATH}/bin
export CROSS_COMPILE=${TOOLCHAIN_PATH}/bin/riscv64-unknown-elf-
export TOOLCHAIN_LIB_DIR=${TOOLCHAIN_PATH}/lib/gcc/riscv64-unknown-elf/8.2.0

echo "=============================="
echo "toolchain path: $TOOLCHAIN_PATH"
echo "k210 port path: $PROJECT_DIR"
echo "=============================="

