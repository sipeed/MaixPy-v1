import uos
import os
import machine
import common
pin_init=common.pin_init()
pin_init.init()
test_gpio_pin_num=15
fpioa=machine.fpioa()
fpioa.set_function(test_gpio_pin_num,63)
test_pin=machine.pin(7,2,0)
if test_pin.value() == 0:
    print('test')
    machine.test()
                            
