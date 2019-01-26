import gc
import uos
import machine
import platform
import board
from fpioa_manager import *
import pwmc
board_info=board.board_info()
if '/init.py' in uos.listdir():
    from init import *
