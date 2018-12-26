maixpy demo
======

## Timer

### Demo 1

```
from machine import Timer

def on_timer(timer,param):
    print("time up:",timer)
    print("param:",param)

tim = Timer(Timer.TIMER0, Timer.CHANNEL0, mode=Timer.MODE_ONE_SHOT, period=3000, callback=on_timer, param=on_timer)
print("period:",tim.period())
tim.start()
```

### Demo 2
```
import time
from machine import Timer

def on_timer(timer,param):
    print("time up:",timer)
    print("param:",param)

tim = Timer(Timer.TIMER0, Timer.CHANNEL0, mode=Timer.MODE_PERIODIC, period=1, unit=Timer.UNIT_S, callback=on_timer, param=on_timer, start=False, priority=1, div=0)
print("period:",tim.period())
tim.start()
time.sleep(5)
tim.stop()
time.sleep(5)
tim.restart()
time.sleep(5)
tim.stop()
del tim
```

### Attention

```
# period:[0,], unit:[Timer.UNIT_S/Timer.UNIT_MS/Timer.UNIT_US/Timer.UNIT_NS], mode:[Timer.MODE_PERIODIC/Timer.MODE_ONE_SHOT], start:[True/False] default True, priority:[1,7], div:clk_timer = clk_pll0/2^(div+1)([0,255] default 0)
#clk_timer*period(unit:s) should < 2^32 and >=1
```


## PWM

```
from machine import Timer,PWM
import time

tim = Timer(Timer.TIMER0, Timer.CHANNEL0, mode=Timer.MODE_PWM)
ch = PWM(tim, freq=500000, duty=50, pin=board_info.LED_G)
duty=0
dir = True
while True:
    if dir:
        duty += 10
    else:
        duty -= 10
    if duty>100:
        duty = 100
        dir = False
    elif duty<0:
        duty = 0
        dir = True
    time.sleep(0.05)
    ch.duty(duty)
```

```
import time
import machine

tim = machine.Timer(machine.Timer.TIMER0, machine.Timer.CHANNEL0, mode=machine.Timer.MODE_PWM)
ch0 = machine.PWM(tim, freq=3000000, duty=20, pin=board_info.LED_G, enable=False)
ch0.enable()
time.sleep(3)
ch0.freq(2000000)
print("freq:",ch0.freq())
ch0.duty(60)
time.sleep(3)
ch0.disable()
```

```
#freq:(0,], duty:[0,100], pin:[0,47], enable:[True/False] default True
```

