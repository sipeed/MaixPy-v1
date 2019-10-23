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

import gc, uos, sys
import machine
from board import board_info
from fpioa_manager import fm
from pye_mp import pye
from Maix import FPIOA, GPIO

banner = '''
 __  __              _____  __   __  _____   __     __
|  \/  |     /\     |_   _| \ \ / / |  __ \  \ \   / /
| \  / |    /  \      | |    \ V /  | |__) |  \ \_/ /
| |\/| |   / /\ \     | |     > <   |  ___/    \   /
| |  | |  / ____ \   _| |_   / . \  | |         | |
|_|  |_| /_/    \_\ |_____| /_/ \_\ |_|         |_|

M5StickV by M5Stack : https://m5stack.com/
M5StickV Wiki       : https://docs.m5stack.com
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
import lcd
import image
import time
import uos
import sys
from pmu import axp192

pmu = axp192()
pmu.enablePMICSleepMode(True)

lcd.init()
lcd.rotation(2) #Rotate the lcd 180deg

try:
    img = image.Image("/flash/startup.jpg") 
    lcd.display(img) 
except:
    lcd.draw_string(lcd.width()//2-100,lcd.height()//2-4, "Error: Cannot find start.jpg", lcd.WHITE, lcd.RED)   

from Maix import I2S, GPIO
import audio
from Maix import GPIO
from fpioa_manager import *

fm.register(board_info.SPK_SD, fm.fpioa.GPIO0)
spk_sd=GPIO(GPIO.GPIO0, GPIO.OUT)
spk_sd.value(1) #Enable the SPK output

fm.register(board_info.SPK_DIN,fm.fpioa.I2S0_OUT_D1)
fm.register(board_info.SPK_BCLK,fm.fpioa.I2S0_SCLK)
fm.register(board_info.SPK_LRCLK,fm.fpioa.I2S0_WS)

wav_dev = I2S(I2S.DEVICE_0)

try:
    player = audio.Audio(path = "/flash/ding.wav")
    player.volume(100)
    wav_info = player.play_process(wav_dev)
    wav_dev.channel_config(wav_dev.CHANNEL_1, I2S.TRANSMITTER,resolution = I2S.RESOLUTION_16_BIT, align_mode = I2S.STANDARD_MODE)
    wav_dev.set_sample_rate(wav_info[1])
    while True:
        ret = player.play()
        if ret == None:
            break
        elif ret==0:
            break
    player.finish()
except:
    pass

fm.register(board_info.BUTTON_A, fm.fpioa.GPIO1)
but_a=GPIO(GPIO.GPIO1, GPIO.IN, GPIO.PULL_UP) #PULL_UP is required here!

if but_a.value() == 0: #If dont want to run the demo
    sys.exit() 

fm.register(board_info.BUTTON_B, fm.fpioa.GPIO2)
but_b = GPIO(GPIO.GPIO2, GPIO.IN, GPIO.PULL_UP) #PULL_UP is required here!

fm.register(board_info.LED_W, fm.fpioa.GPIO3)
led_w = GPIO(GPIO.GPIO3, GPIO.OUT)
led_w.value(1) #RGBW LEDs are Active Low

fm.register(board_info.LED_R, fm.fpioa.GPIO4)
led_r = GPIO(GPIO.GPIO4, GPIO.OUT)
led_r.value(1) #RGBW LEDs are Active Low

fm.register(board_info.LED_G, fm.fpioa.GPIO5)
led_g = GPIO(GPIO.GPIO5, GPIO.OUT)
led_g.value(1) #RGBW LEDs are Active Low

fm.register(board_info.LED_B, fm.fpioa.GPIO6)
led_b = GPIO(GPIO.GPIO6, GPIO.OUT)
led_b.value(1) #RGBW LEDs are Active Low


time.sleep(0.5) # Delay for few seconds to see the start-up screen :p            

import sensor
import KPU as kpu

err_counter = 0

while 1:
    try:
        sensor.reset() #Reset sensor may failed, let's try some times
        break
    except:
        err_counter = err_counter + 1
        if err_counter == 20:
            lcd.draw_string(lcd.width()//2-100,lcd.height()//2-4, "Error: Sensor Init Failed", lcd.WHITE, lcd.RED)   
        time.sleep(0.1)
        continue

sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA) #QVGA=320x240
sensor.run(1)

task = kpu.load(0x300000) # Load Model File from Flash
anchor = (1.889, 2.5245, 2.9465, 3.94056, 3.99987, 5.3658, 5.155437, 6.92275, 6.718375, 9.01025)
# Anchor data is for bbox, extracted from the training sets.
kpu.init_yolo2(task, 0.5, 0.3, 5, anchor)

but_stu = 1

try:
    while(True):
        img = sensor.snapshot() # Take an image from sensor
        bbox = kpu.run_yolo2(task, img) # Run the detection routine
        if bbox:
            for i in bbox:
                print(i)
                img.draw_rectangle(i.rect())
        lcd.display(img)

        if but_a.value() == 0 and but_stu == 1:
            if led_w.value() == 1:        
                led_w.value(0)
            else:
                led_w.value(1)
            but_stu = 0
        if but_a.value() == 1 and but_stu == 0:
            but_stu = 1
            
except KeyboardInterrupt:
    a = kpu.deinit(task)
    sys.exit()
'''

f = open("/flash/boot.py", "wb")
f.write(boot_py)
f.close()

print(banner)
with open("/flash/boot.py") as f:
    exec(f.read())

