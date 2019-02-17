import gc
import uos
import os
import sys
import machine
from board import board_info
from fpioa_manager import *

board_info=board_info()
sys.path.append('.')

# chdir to "/sd" or "/flash"
devices = os.listdir("/")
if "sd" in devices:
    os.chdir("/sd")
    sys.path.append('/sd')
else:
    os.chdir("/flash")
sys.path.append('/flash')

# detect boot.py
boot_py = '''
import os, Maix, lcd
from Maix import FPIOA, GPIO

test_pin=15
fpioa = FPIOA()
fpioa.set_function(test_pin,FPIOA.GPIO7)
test_gpio=GPIO(GPIO.GPIO7,GPIO.IN)
lcd.init()
lcd.draw_string(100,120,"Welcome to MaixPy")
if test_gpio.value() == 0:
    print('PIN 15 pulled down, enter test mode')

'''
flash_ls = os.listdir()
if not "boot.py" in flash_ls:
    f = open("boot.py", "wb")
    f.write(boot_py)
    f.close()

# run boot.py
import boot


