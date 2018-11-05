import gc
import platform
import uos
import os
import machine
import common
import app
pin_init=common.pin_init()
pin_init.init()
#ov=machine.ov2640()
#lcd=machine.st7789()
#ov.init()
#lcd.init()
#buf=bytearray(320*240*2)
#test=machine.test()
#test=test.init()
test_gpio_pin_num=15
fpioa=machine.fpioa()
fpioa.set_function(test_gpio_pin_num,63)
test_pin=machine.pin(7,2,0)
if test_pin.value() == 0:
    print('test')
    machine.test()
    #while(1):
    #    ov.get_image(buf)
    #    lcd.draw_picture_default(buf)
    #    test.gpio()

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
