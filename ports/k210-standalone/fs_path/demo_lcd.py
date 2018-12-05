import machine
import os
lcd=machine.st7789()
lcd.init()
image=bytearray(101*101*2)
os.read('/logo.bin',0,0,image)
lcd.draw_picture(0,0,101,101,image)
