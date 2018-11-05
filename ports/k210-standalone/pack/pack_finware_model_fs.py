#!/usr/bin/env python3
# -*- coding: utf-8 -*-
global model_bin_buf
global model_bin_size
model_bin_size = 0
bin_path='./bin_file/'
output_bin_file = open('sipeedm1.bin', 'wb+')

firmware_file = open('micropython.bin', 'rb')
firmware_bin_buf = firmware_file.read()
firmware_bin_size = len(firmware_bin_buf)
print("firmware_bin_size %d\n",firmware_bin_size)

output_bin_file.write(firmware_bin_buf)

cfg_offset = int("0x00080000", base = 16)
cfg_file = open(bin_path+'cfg.bin', 'rb')
cfg_bin_buf = cfg_file.read()
cfg_bin_size = len(cfg_bin_buf)
print("cfg_bin_size %d\n",cfg_bin_size)
print("cfg_offset %d\n",cfg_offset)
output_bin_file.seek(cfg_offset, 0)
output_bin_file.write(cfg_bin_buf)

model_offset = int("0x00090000", base = 16)
model_file = open(bin_path+'model.bin', 'rb')
model_bin_buf = model_file.read()
model_bin_size = len(model_bin_buf)
print("model_bin_size %d\n",model_bin_size)
print("model_offset %d\n",model_offset)
output_bin_file.seek(model_offset, 0)
output_bin_file.write(model_bin_buf)


fs_offset = int("0x00200000", base = 16)
fs_file = open(bin_path+'fs.bin', 'rb')
fs_bin_buf = fs_file.read()
fs_bin_size = len(fs_bin_buf)
print("fs_bin_size %d\n",fs_bin_size)
print("fs_offset %d\n",fs_offset)
output_bin_file.seek(fs_offset, 0)
output_bin_file.write(fs_bin_buf)

output_bin_file.close()
