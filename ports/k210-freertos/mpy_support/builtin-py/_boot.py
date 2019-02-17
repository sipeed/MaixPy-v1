import gc
import uos
import os
import sys
import machine
from board import board_info
from fpioa_manager import *
sys.path.append('flash')
board_info=board_info()

devices = os.listdir("/")
if "sd" in devices:
    os.chdir("/sd")
else:
    os.chdir("/flash")


