import gc
import uos
import machine
import platform
import board
from fpioa_manager import *
import pwmc
board_info=board.board_info()
board.name="maixpy" # for rshell
try:
    from init import *
except:
    print("No init.py, Skipped")

