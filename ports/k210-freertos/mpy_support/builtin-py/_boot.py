import gc
import uos
import os
import sys
import machine
from board import board_info
from fpioa_manager import *
sys.path.append('flash')
board_info=board_info()
fm.registered(board_info.PIN11,fm.fpioa.UART1_TX)
fm.registered(board_info.PIN10,fm.fpioa.UART1_RX)
