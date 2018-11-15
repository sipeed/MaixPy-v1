import gc
import uos
import os
import machine
import platform
import app
import board
import fpioa_manager
board_info=board.board_info()
fm=fpioa_manager.fpioa_manager()
file_list = os.ls()
for i in range(len(file_list)):
    if file_list[i] == '/init.py':
        import init

