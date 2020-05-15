import os, sys, time

sys.path.append('')
sys.path.append('.')

# chdir to "/sd" or "/flash"
devices = os.listdir("/")
if "sd" in devices:
    os.chdir("/sd")
    sys.path.append('/sd')
else:
    os.chdir("/flash")
sys.path.append('/flash')

print("[MaixPy] init end") # for IDE
for i in range(200):
    time.sleep_ms(1) # wait for key interrupt(for maixpy ide)

# check IDE mode
ide_mode_conf = "/flash/ide_mode.conf"
ide = True
try:
    f = open(ide_mode_conf)
    f.close()
except Exception:
    ide = False

if ide:
    os.remove(ide_mode_conf)
    from machine import UART
    import lcd
    lcd.init(color=lcd.PINK)
    repl = UART.repl_uart()
    repl.init(1500000, 8, None, 1, read_buf_len=2048, ide=True, from_ide=False)
    sys.exit()    

import gc
import machine
from board import board_info
from fpioa_manager import fm
from Maix import FPIOA, GPIO
from machine import I2C
import axp202


i2c = I2C(I2C.I2C0, freq=400000, scl=30, sda=31)
p = None
try:
    p = axp202.PMU(i2c,0x35)
except:
    p = axp202.PMU(i2c,0x34)
else:
    p.setShutdownTime(axp202.AXP202_SHUTDOWN_TIME_4S)
    p.setLDO2Voltage(1800)
    p.enablePower(axp202.AXP192_LDO2)
    p.enablePower(6);
    p.setLDO3Mode(1)

banner = '''
 __  __              _____  __   __  _____   __     __
|  \/  |     /\     |_   _| \ \ / / |  __ \  \ \   / /
| \  / |    /  \      | |    \ V /  | |__) |  \ \_/ /
| |\/| |   / /\ \     | |     > <   |  ___/    \   /
| |  | |  / ____ \   _| |_   / . \  | |         | |
|_|  |_| /_/    \_\ |_____| /_/ \_\ |_|         |_|

Co-op by Sipeed     : https://www.sipeed.com
'''

dirList = os.listdir()

if "boot.py" in dirList:
    print(banner)
    with open("boot.py") as f:
        exec(f.read())
    sys.exit()
else:
    if "boot.py" in os.listdir("/flash"):
        print(banner)
        with open("/flash/boot.py") as f:
            exec(f.read())
        sys.exit()

# detect boot.py
boot_py = '''
from fpioa_manager import *
import os, Maix, lcd, image
from Maix import FPIOA, GPIO

test_pin=16
fpioa = FPIOA()
fpioa.set_function(test_pin,FPIOA.GPIO7)
test_gpio=GPIO(GPIO.GPIO7,GPIO.IN)
lcd.init(freq=15000000,color=(255,0,0))
fm.register(board_info.PIN17,fm.fpioa.GPIO0)
led=GPIO(GPIO.GPIO0,GPIO.OUT)
led.value(1)
lcd.rotation(1)
lcd.clear((255,0,0))
lcd.draw_string(lcd.width()//2-68,lcd.height()//2-4, "Welcome to MaixPy", lcd.WHITE, lcd.RED)
if test_gpio.value() == 0:
    lcd.rotation(2)
    print('PIN 16 pulled down, enter test mode')
    import sensor
    import image
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.run(1)
    lcd.freq(16000000)
    while True:
        img=sensor.snapshot()
        lcd.display(img)
'''

f = open("/flash/boot.py", "wb")
f.write(boot_py)
f.close()

print(banner)
with open("/flash/boot.py") as f:
    exec(f.read())

