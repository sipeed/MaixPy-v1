import gc
import uos
import os
import machine
import platform
import app
import board
board_info=board.board_info()
file_list = os.ls()
for i in range(len(file_list)):
    if file_list[i] == '/init.py':
        import init

