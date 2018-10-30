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
ov=machine.ov2640()
ov.init()
st=machine.st7789()
st.init()
demo=machine.demo_face_detect()
demo.init()
buf=bytearray(320*240*2)