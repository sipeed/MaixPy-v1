#!/bin/sh
make -C mkfs/mkspiffs/ clean && make -C mkfs/mkspiffs/ all
mkdir -p fs_path
mkdir -p output
mkfs/mkspiffs/mkspiffs -c fs_path/ output/fs.bin
