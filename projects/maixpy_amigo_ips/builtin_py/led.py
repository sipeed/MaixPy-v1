
from fpioa_manager import *
from Maix import FPIOA, GPIO

class sipeed_led:

    r, g, b, w = None, None, None, None

    def init(r = 13, g = 12, b = 14, w = 32):
        fm.register(r, fm.fpioa.GPIOHS13, force=True)
        fm.register(g, fm.fpioa.GPIOHS12, force=True)
        fm.register(b, fm.fpioa.GPIOHS14, force=True)
        fm.register(w, fm.fpioa.GPIOHS3, force=True)

        sipeed_led.r = GPIO(GPIO.GPIOHS13, GPIO.OUT, value=1)
        sipeed_led.g = GPIO(GPIO.GPIOHS12, GPIO.OUT, value=1)
        sipeed_led.b = GPIO(GPIO.GPIOHS14, GPIO.OUT, value=1)
        sipeed_led.w = GPIO(GPIO.GPIOHS3, GPIO.OUT, value=1)

    def unit_test():
        import time
        sipeed_led.r.value(0)
        time.sleep(1)
        sipeed_led.r.value(1)
        sipeed_led.g.value(0)
        time.sleep(1)
        sipeed_led.g.value(1)
        sipeed_led.b.value(0)
        time.sleep(1)
        sipeed_led.b.value(1)
        sipeed_led.w.value(0)
        time.sleep(1)
        sipeed_led.w.value(1)

if __name__ == "__main__":
    sipeed_led.init(13, 12, 14, 32) # cube
    # sipeed_led.init(14, 15, 17, 32) # amigo

    while True:
        sipeed_led.unit_test()

