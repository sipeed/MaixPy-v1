#!/bin/bash
bsize='1M'
bcount=12
cd bin_file
python3 ../pack/pre_process_mpy.py
dd if=/dev/zero of=firmware.bin bs=$bsize count=$bcount
dd if=micropython_sha256.bin of=firmware.bin conv=notrunc
cfgseek=$[0x80000]
echo  readl cfgseek is $[$cfgseek]
dd if=cfg.bin of=firmware.bin bs=1 seek=$[$cfgseek] conv=notrunc
modseek=$[0x90000]
echo  real modseek is $[$modseek-5]
dd if=model.bin of=firmware.bin bs=1 seek=$[$modseek] conv=notrunc
fsseek=$[0x600000]
echo  real fsseek is $[$fsseek-5]
dd if=fs.bin of=firmware.bin bs=1 seek=$[$fsseek] conv=notrunc
rm -rf micropython_sha256.bin
