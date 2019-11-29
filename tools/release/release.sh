#!/bin/bash

set -e

#
dirty=`git describe --long --tag --dirty --always | awk -F "-" '{print $4}'`
version_full=`git describe --long --tag --dirty --always`
version=`echo $version_full | awk -F "-" '{print $1}'`
version_dev=`echo $version_full | awk -F "-" '{print $2}'`
version_git_rev=`echo $version_full | awk -F "-" '{print $3}'`
if [[ "x$version_dev" != "x" ]]; then
    version=${version}_${version_dev}_${version_git_rev}
fi
if [[ "x$dirty" == "xdirty" ]]; then
    echo -e "\033[33m [WARNING] coding is dirty!!, please commit all code firstly \033[0m"
    git status
fi
echo $version_full
echo $version

release_dir=`pwd`/bin/maixpy_$version
rm -rf $release_dir $release_dir/elf
mkdir -p $release_dir
mkdir -p $release_dir/elf

#
cd ../../projects

#
cd maixpy_k210
echo "-------------------"
echo "build project maixpy_k210"
echo "-------------------"
python project.py distclean
python project.py build
cp build/maixpy.bin $release_dir/maixpy_$version.bin
cp build/maixpy.elf $release_dir/elf/maixpy_$version.elf
cd ..

# maixpy_k210 with lvgl
cd maixpy_k210
echo "-------------------"
echo "build project maixpy_k210_with_lvgl"
echo "-------------------"
python project.py distclean
python project.py build --config_file "config_with_lvgl.mk"
cp build/maixpy.bin $release_dir/maixpy_${version}_with_lvgl.bin
cp build/maixpy.elf $release_dir/elf/maixpy_${version}_with_lvgl.elf
cd ..

#
cd maixpy_k210_minimum
echo "-------------------"
echo "build project maixpy_k210_minimum"
echo "-------------------"
python project.py distclean
python project.py build
cp build/maixpy.bin $release_dir/maixpy_${version}_minimum.bin
cp build/maixpy.elf $release_dir/elf/maixpy_${version}_minimum.elf
cd ..

# minimum with IDE support
cd maixpy_k210_minimum
echo "-------------------"
echo "build project maixpy_k210_minimum"
echo "-------------------"
python project.py distclean
python project.py build --config_file "config_with_ide_support.mk"
cp build/maixpy.bin $release_dir/maixpy_${version}_minimum_with_ide_support.bin
cp build/maixpy.elf $release_dir/elf/maixpy_${version}_minimum_with_ide_support.elf
cd ..

#
cd maixpy_m5stickv
echo "-------------------"
echo "build project maixpy_m5stickv"
echo "-------------------"
python project.py distclean
python project.py build
cp build/maixpy.bin $release_dir/maixpy_${version}_m5stickv.bin
cp build/maixpy.elf $release_dir/elf/maixpy_${version}_m5stickv.elf
cd ..

cd $release_dir
7z a elf_maixpy_${version}.7z elf/*
rm -rf elf

ls -al

