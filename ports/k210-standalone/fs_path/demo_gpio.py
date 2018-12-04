import machine
import board
from fpioa_manager import *
board_info=board.board_info()
fm.fpioa.set_function(board_info.LED_G,fm.fpioa.GPIO0)
gpio=machine.GPIO(machine.GPIO.GPIO0,machine.GPIO.DM_OUTPUT,machine.GPIO.LOW_LEVEL)
gpio.value(gpio.LOW_LEVEL)
