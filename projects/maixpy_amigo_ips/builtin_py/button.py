# This file is part of MaixUI
# Copyright (c) sipeed.com
#
# Licensed under the MIT license:
#   http://www.opensource.org/licenses/mit-license.php
#

import sys, time

from fpioa_manager import fm
from Maix import FPIOA, GPIO

class button_io:

    home_button = None
    next_button = None
    back_button = None

    def config(ENTER=10, BACK=11, NEXT=16):
        fm.register(ENTER, fm.fpioa.GPIOHS0 + ENTER, force=True)
        fm.register(NEXT, fm.fpioa.GPIOHS0 + NEXT, force=True)
        fm.register(BACK, fm.fpioa.GPIOHS0 + BACK, force=True)
                
        button_io.home_button = GPIO(GPIO.GPIOHS0 + ENTER, GPIO.IN, GPIO.PULL_UP)
        button_io.next_button = GPIO(GPIO.GPIOHS0 + NEXT, GPIO.IN, GPIO.PULL_UP)
        button_io.back_button = GPIO(GPIO.GPIOHS0 + BACK, GPIO.IN, GPIO.PULL_UP)


Match = [[(1, 0), (1, 0)], [(2, 1), None]] # 0 1 1 2 0
#Match = [[None, (1, 0)], [(2, 1), None]]  # 0 1 0 0 2

class sipeed_button:

    def __init__(self):
        self.home_last, self.next_last, self.back_last = 1, 1, 1
        self.last_time, self.bak_time = 0, 0
        self.cache = {
            'home': 0,
            'next': 0,
            'back': 0,
        }
        self.pause_time = 0
        self.enable = True
        self.Limit = 1000  # 1s

    def home(self):
        if self.enable:
            tmp, self.cache['home'] = self.cache['home'], 0
            return tmp
        return 0

    def next(self):
        if self.enable:
            tmp, self.cache['next'] = self.cache['next'], 0
            return tmp
        return 0

    def back(self):
        if self.enable:
            tmp, self.cache['back'] = self.cache['back'], 0
            return tmp
        return 0

    def interval(self):
        if self.enable:
            tmp, self.last_time = self.last_time, 0
            return tmp

    def event(self):

        if self.enable:

            self.bak_time = time.ticks_ms()

            home_value = button_io.home_button.value()

            tmp = Match[home_value][self.home_last]
            if tmp:
                self.last_time = time.ticks_ms() - self.bak_time
                self.cache['home'], self.home_last = tmp

            tmp = Match[button_io.back_button.value()][self.back_last]
            if tmp:
                self.last_time = time.ticks_ms() - self.bak_time
                self.cache['back'], self.back_last = tmp

            tmp = Match[button_io.next_button.value()][self.next_last]
            if tmp:
                self.last_time = time.ticks_ms() - self.bak_time
                self.cache['next'], self.next_last = tmp

    def expand_event(self):

        if self.enable:

            tmp = button_io.home_button.value()
            if tmp == 0 and self.home_last == 1 and self.pause_time < time.ticks_ms():
                self.cache['home'], self.home_last, self.bak_time = 1, 0, time.ticks_ms()

            # (monkey patch) long press home 0 - 1 - 2 - 0 - 1 - 2 - 0
            if self.home_last == 0 and self.bak_time != 0 and time.ticks_ms() - self.bak_time > self.Limit:
                #print(tmp, self.home_last, self.pause_time, time.ticks_ms())
                self.cache['home'], self.home_last, self.last_time = 2, 1, time.ticks_ms(
                ) - self.bak_time
                self.pause_time = time.ticks_ms() + self.Limit

            if tmp == 1 and self.home_last == 0 and self.pause_time < time.ticks_ms():
                self.cache['home'], self.home_last, self.last_time = 2, 1, time.ticks_ms(
                ) - self.bak_time

            tmp = button_io.back_button.value()

            if tmp == 0 and self.back_last == 1:
                self.cache['back'], self.back_last, self.bak_time = 1, 0, time.ticks_ms()

            if self.bak_time != 0 and time.ticks_ms() - self.bak_time > self.Limit:
                tmp = not tmp

            if tmp == 1 and self.back_last == 0:
                self.cache['back'], self.back_last, self.last_time = 2, 1, time.ticks_ms(
                ) - self.bak_time

            tmp = button_io.next_button.value()

            if tmp == 0 and self.next_last == 1:
                self.cache['next'], self.next_last, self.bak_time = 1, 0, time.ticks_ms()

            if self.bak_time != 0 and time.ticks_ms() - self.bak_time > self.Limit:
                tmp = not tmp

            if tmp == 1 and self.next_last == 0:
                self.cache['next'], self.next_last, self.last_time = 2, 1, time.ticks_ms(
                ) - self.bak_time

class ttgo_button:
    PIR = 16

    def __init__(self):
        self.home_last = 1
        self.cache = {
            'home': 0,
        }

        fm.register(ttgo_button.PIR, fm.fpioa.GPIOHS16, force=True)
        self.home_button = GPIO(GPIO.GPIOHS16, GPIO.IN, GPIO.PULL_UP)

    def home(self):
        tmp, self.cache['home'] = self.cache['home'], 0
        return tmp

    def event(self):

        tmp = Match[self.home_button.value()][self.home_last]
        if tmp:
            self.cache['home'], self.home_last = tmp


if __name__ == "__main__":

    #tmp = ttgo_button()
    #while True:
    #tmp.event()
    #time.sleep_ms(200)
    #print(tmp.home())

    print(os.listdir())
    tmp = sipeed_button()
    # button_io.config(10, 11, 16) # cube
    button_io.config(23, 31, 20) # amigo
    print(os.listdir())
    while True:
        time.sleep_ms(200)
        #tmp.event()
        #print(tmp.home(), tmp.back(), tmp.next())
        tmp.expand_event()
        print(tmp.home(), tmp.back(), tmp.next(), tmp.interval())
        #print(os.listdir())
