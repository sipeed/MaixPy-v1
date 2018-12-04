import machine
timer=machine.timer(machine.timer.TIMER0,machine.timer.CHANNEL0)
def func(timer):
    print("Hello world")
timer.init(freq=10,period=0,div=0,callback=func)

