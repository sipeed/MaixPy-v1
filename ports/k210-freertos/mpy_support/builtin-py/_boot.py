import gc
import uos
import os
import sys
import machine
from board import board_info
from fpioa_manager import *
from pye_mp import pye

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
from board import board_info
from fpioa_manager import *
import os, Maix, lcd
from Maix import FPIOA, GPIO

test_pin=16
fpioa = FPIOA()
fpioa.set_function(test_pin,FPIOA.GPIO7)
test_gpio=GPIO(GPIO.GPIO7,GPIO.IN)
lcd.init()
lcd.draw_string(100,120,"Welcome to MaixPy")
if test_gpio.value() == 0:
    print('PIN 16 pulled down, enter test mode')
    import sensor
    import image
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.run(1)
    lcd.freq(17000000)
    while True:
        img=sensor.snapshot()
        lcd.display(img)
'''
flash_ls = os.listdir()
if not "boot.py" in flash_ls:
    f = open("boot.py", "wb")
    f.write(boot_py)
    f.close()

# run boot.py
import boot


