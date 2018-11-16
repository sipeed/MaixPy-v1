import uos
import os
import machine
import platform
import board
board_info=board.board_info()
pin_init=platform.pin_init()
pin_init.init()
test_gpio_pin_num=18
fpioa=machine.fpioa()
fpioa.set_function(test_gpio_pin_num,63)
test_pin=machine.GPIO(7,2,0)
lcd=machine.st7789()
lcd.init()
lcd.draw_string(116,121,"Welcome to MaixPy")
fpioa.set_function(board_info.LED_R,fpioa.GPIO0)
fpioa.set_function(board_info.LED_B,fpioa.GPIO1)
fpioa.set_function(board_info.LED_G,fpioa.GPIO2)
if test_pin.value() == 0:
    print('test')
    machine.test()
