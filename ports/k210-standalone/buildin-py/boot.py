import gc
import platform
import uos
import os
import machine
import common

# init pin map
pin_init=common.pin_init()
pin_init.init()

# run usr init.py file
file_list = os.ls()
for i in range(len(file_list)):
    if file_list[i] == '/init.py':
        import init

