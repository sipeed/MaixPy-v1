#!/bin/bash

config_file=config.conf
if [ ! -f $config_file ]; then
    echo "Please config toolchain path first in config.conf"
    echo "toolchain_path=/opt/kendryte-toolchain/bin" > config.conf
    exit 1
fi
build_config=`cat $config_file`
toolchain_setting=`echo ${build_config} |grep toolchain_path`
toolchain_path="/"`echo -e ${toolchain_setting#*/}`

if [ ! -d $toolchain_path ]; then
    echo "can not find toolchain, please set toolchain path in config.conf"
    echo "toolchain_path=/opt/kendryte-toolchain/bin" > config.conf
    exit 1
fi

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$toolchain_path
export CROSS_COMPILE=$toolchain_path/riscv64-unknown-elf-
export PLATFORM=k210

function help()
{
    echo "help:"
    echo "      ./build.sh "
    echo "      ./build.sh clean"
    echo ""
}

if [[ "$1x" == "cleanx" ]]; then
    make clean
    exit 0
fi

start_time=`date +%s`

MAKE_J_NUMBER=`cat /proc/cpuinfo | grep vendor_id | wc -l`
echo "=============================="
echo "CORE number: $MAKE_J_NUMBER"
echo "=============================="

make update_mk
make update_mk
# make -j$MAKE_J_NUMBER all
make all

end_time=`date +%s`
time_distance=`expr ${end_time} - ${start_time}`
date_time_now=$(date +%F\ \ %H:%M:%S)
echo -ne "\033[1;32m" #green
echo ====== Build Time: ${time_distance}s  complete at  ${date_time_now} ======= | tee -a ${LOG_FILE}
echo -e "\033[0m"




