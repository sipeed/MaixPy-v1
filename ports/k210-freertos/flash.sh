#!/bin/bash

############### SETTINGS #############
baud=2000000
device=/dev/ttyUSB0
#######################################

monitor=false
kflash_py=tools/kflash.py/kflash.py

start_time=`date +%s`

if [[ "$1x" != "x" ]] && [[ "$1x" != "mx" ]] && [[ "$1x" != "monitorx" ]] && [[ "$1x" != "tx" ]]; then
    device=$1
fi
if [[ "${@: -1}x" == "mx" ]] || [[ "${@: -1}x" == "monitorx" ]] || [[ "${@: -1}x" == "tx" ]]; then
    monitor=true
fi

function help()
{
    echo ""
    echo "help:"
    echo "      ./flash.sh install:   download flash script"
    echo "      ./flash.sh uninstall: remove flash script"
    echo "      ./flash.sh 'device(default /dev/ttyUSB0)' 'monitor/m/t'"
    echo ""
    echo "  e.g."
    echo "      ./flash.sh "
    echo "      ./flash.sh /dev/ttyUSB0 m"
    echo "      ./flash.sh m"
    echo ""
}

if [[ "x$1" == "xhelp" ]]; then
    help
    exit 0
elif [[ "x$1" == "xinstall" ]]; then
    git clone https://github.com/sipeed/kflash.py.git tools/kflash.py
    exit 0
elif [[ "x$1" == "xuninstall" ]]; then
    rm -rf tools/kflash.py
    echo "uninstall ok"
    exit 0
fi

if [[ ! -f $kflash_py ]]; then
    git clone https://github.com/sipeed/kflash.py.git tools/kflash.py
fi

cwd=`pwd`
bin_file_path="$cwd/output/maixpy.bin"

if [[ -f $bin_file_path ]]; then
    if [[ $monitor == true ]]; then
        python3 $kflash_py -b $baud -p $device -B dan -t $bin_file_path
    else
        python3 $kflash_py -b $baud -p $device -B dan $bin_file_path
    fi
else
    echo "bin file not exist!!!!"
    echo "burn fail"
fi

end_time=`date +%s`
time_distance=`expr ${end_time} - ${start_time}`
date_time_now=$(date +%F\ \ %H:%M:%S)
echo -ne "\033[1;32m" #green
echo ====== Flash Time: ${time_distance}s  complete at  ${date_time_now} =======
echo -e "\033[0m"


