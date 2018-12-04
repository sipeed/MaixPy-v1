import machine
import board
from fpioa_manager import *
class pwm:
    def __init__(self,pwm_timer=None,pwm_channel=None,freq=0,duty=0,pin=None):
        global fm
        if pwm_timer == None or pwm_channel == None:
            print("Please enter timer and channel")
            return None
        if fm == None:
            print("pwm need a fpioa manager to manage ther fpioa")
            return None
        if pin == None:
            print("pwm need a output pin")
            return None
        if freq <= 0 :
            self.__freq = 2000000
            print("use default frequency 2000000")
        else:
            self.__freq = freq
        if duty > 100 and duty < 0:
            self.__duty=30
            print("duty out range,set duty = 50")
        else:
            self.__duty = duty
        self.__timer = pwm_timer
        self.__channel = pwm_channel
        self.__out_pin = pin
        print("timer:",self.__timer)
        print("channel:",self.__channel)
        print("pin:",self.__out_pin)
        fm.registered(self.__out_pin,fm.fpioa.TIMER0_TOGGLE1+self.__timer*4+self.__channel)
        print("use timer_channel = ",fm.fpioa.TIMER0_TOGGLE1+self.__timer+self.__channel)
        print("use timer = ",self.__timer)
        print("use channel = ",self.__channel)
        self.__pwm=machine.pwm(self.__timer,self.__channel,self.__freq,self.__duty,self.__out_pin)
    def duty(self,duty):
        self.__duty=duty
        self.__pwm.duty(duty)
    def freq(self,freq):
        self.__freq=freq
        self.__pwm.freq(freq)
    def set_fpioa(self,out_pin):
        global fm
        self.__out_pin = out_pin
        fm.registered(out_pin,fm.fpioa.TIMER0_TOGGLE1+self.__timer+self.__channel)

