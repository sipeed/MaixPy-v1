#!/bin/sh
make -C mkfs/mkspiffs/ clean && make -C mkfs/mkspiffs/ all
mkdir -p fs
mkdir -p output
mkfs/mkspiffs/mkspiffs -c fs/ output/fs.bin
