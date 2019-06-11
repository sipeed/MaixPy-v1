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
    rm platform/api/lib_mic.a -f
    exit 0
elif [[ "$1x" != "x" ]]; then
    help
    exit 0
fi
start_time=`date +%s`

UNAME_S=`uname -s`
if [ "$UNAME_S" == "Linux" ]
then
	MAKE_J_NUMBER=`cat /proc/cpuinfo | grep vendor_id | wc -l`
fi
if [ "$UNAME_S" == "Darwin" ]
then
	MAKE_J_NUMBER=`sysctl -n hw.ncpu`
fi
echo "=============================="
echo "CORE number: $MAKE_J_NUMBER"
echo "=============================="


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




