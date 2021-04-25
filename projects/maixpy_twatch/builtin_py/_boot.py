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
try:
    import os, Maix, gc, lcd, image
    from Maix import FPIOA, GPIO
    gc.collect()
    lcd.init(freq=15000000)
    fm.register(17,fm.fpioa.GPIO0)
    led=GPIO(GPIO.GPIO0,GPIO.OUT)
    led.value(1)
    loading = image.Image(size=(lcd.width(), lcd.height()))
    loading.draw_rectangle((0, 0, lcd.width(), lcd.height()), fill=True, color=(255, 0, 0))
    info = "Welcome to MaixPy"
    loading.draw_string(int(lcd.width()//2 - len(info) * 5), (lcd.height())//4, info, color=(255, 255, 255), scale=2, mono_space=0)
    v = sys.implementation.version
    vers = 'V{}.{}.{} : maixpy.sipeed.com'.format(v[0],v[1],v[2])
    loading.draw_string(int(lcd.width()//2 - len(info) * 6), (lcd.height())//3 + 20, vers, color=(255, 255, 255), scale=1, mono_space=1)
    lcd.display(loading)
    del loading, v, info, vers
    gc.collect()
finally:
    gc.collect()
'''

f = open("/flash/boot.py", "wb")
f.write(boot_py)
f.close()

print(banner)
with open("/flash/boot.py") as f:
    exec(f.read())


import json

config = {
    "type": "twatch",
    "lcd": {
        "height": 240,
        "width": 240,
        "invert": 1,
        "dir": 96,
        "lcd_type": 0
    },
    "board_info": {
        'BOOT_KEY': 16,
        'LED_R': 12,
        'LED_G': 13,
        'LED_B': 14,
        'WIFI_TX': 6,
        'WIFI_RX': 7,
        'WIFI_EN': 8,
        'I2S0_MCLK': 13,
        'I2S0_SCLK': 21,
        'I2S0_WS': 18,
        'I2S0_IN_D0': 35,
        'I2S0_OUT_D2': 34,
        'SPI0_MISO': 26,
        'SPI0_CLK': 27,
        'SPI0_MOSI': 28,
        'SPI0_CS0': 29,
        'MIC0_WS': 30,
        'MIC0_DATA': 31,
        'MIC0_BCK': 32,
        'I2S_WS': 33,
        'I2S_DA': 34,
        'I2S_BCK': 35
    }
}

cfg = json.dumps(config)
print(cfg)

try:
    with open('/flash/config.json', 'rb') as f:
        tmp = json.loads(f.read())
    # print(tmp)
    if tmp["type"] != config["type"]:
        raise Exception('config.json no exist')
except Exception as e:
    with open('/flash/config.json', "w") as f:
        f.write(cfg)
    import machine
    machine.reset()
