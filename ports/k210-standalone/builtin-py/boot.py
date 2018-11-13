import gc
import platform
import uos
import os
import machine
import common
import app
file_list = os.ls()
for i in range(len(file_list)):
    if file_list[i] == '/init.py':
        import init

