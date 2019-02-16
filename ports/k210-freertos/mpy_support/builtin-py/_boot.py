import gc
import uos
import os
import sys
import machine
from board import board_info
from fpioa_manager import *
sys.path.append('/sd')
board_info=board_info()
sd_ls = os.listdir('/sd')
for i in range(len(sd_ls)):
	print(sd_ls[i])
    if sd_ls[i] == 'boot.py':
        import boot
flash_ls = os.listdir('/flash')
for i in range(len(flash_ls)):
	print(flash_ls[i])
    if flash_ls[i] == 'boot.py':
        import boot
