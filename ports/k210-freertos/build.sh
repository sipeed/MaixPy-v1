#!/bin/bash

########################## SETTINGS ######################
toolchain_path=/media/neucrack/data/main/work/sipeed/k210/tools/toolchain/kendryte-toolchain/bin
##########################################################

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




