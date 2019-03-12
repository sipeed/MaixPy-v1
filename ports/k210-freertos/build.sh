#!/bin/bash
echo $0

set -e

source config.sh

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
elif [[ "$1x" == "distcleanx" ]]; then
    make clean
    rm mpy_support/lextab.py mpy_support/yacctab.py
    rm platform/sdk/kendryte-standalone-sdk -rf
    exit 0
elif [[ "$1x" != "x" ]]; then
    help
    exit 0
fi
start_time=`date +%s`

MAKE_J_NUMBER=`cat /proc/cpuinfo | grep vendor_id | wc -l`
echo "=============================="
echo "CORE number: $MAKE_J_NUMBER"
echo "=============================="

CFG_MODE_DEFAULT=`wc -l modules_conf_default.mk |awk '{print $1}'`
CFG_MODE=`wc -l modules_conf.mk |awk '{print $1}'`

if [[ ! -f modules_conf.mk || ${CFG_MODE_DEFAULT} != ${CFG_MODE} ]]; then
    cp modules_conf.mk modules_conf_bak.mk
    cp modules_conf_default.mk modules_conf.mk
fi

# make -j$MAKE_J_NUMBER all
make include_mk
make -j$MAKE_J_NUMBER compile
make out
echo "--------------------------------------------------------------"
make print_size

end_time=`date +%s`
time_distance=`expr ${end_time} - ${start_time}`
date_time_now=$(date +%F\ \ %H:%M:%S)
echo -ne "\033[1;32m" #green
echo ====== Build Time: ${time_distance}s  complete at  ${date_time_now} =======
echo -e "\033[0m"




