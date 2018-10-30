import gc
import platform
import uos
import os
import machine
import common
import app

# init pin map
pin_init=common.pin_init()
pin_init.init()

# run usr init.py file
file_list = os.ls()
for i in range(len(file_list)):
    if file_list[i] == '/init.py':
        import init
# machine.ov2640()
# init()
# machine.st7789()
# init()
# o=machine.demo_face_detect()
# o.init()
# =bytearray(320*240*2)