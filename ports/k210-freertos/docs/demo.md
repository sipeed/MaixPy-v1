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

### Demo1 (Breathing light)

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

### Demo2 

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

### Attention

```
#freq:(0,], duty:[0,100], pin:[0,47], enable:[True/False] default True
```

## I2C


### Demo1

```
from machine import I2C

i2c = I2C(I2C.I2C0, freq=100000, scl=28, sda=29)
devices = i2c.scan()
print(devices)
```

### Demo2

```
import time
from machine import I2C

i2c = I2C(I2C.I2C0, freq=100000, scl=28, sda=29)
i2c.writeto(0x24,b'123')
i2c.readfrom(0x24,5)
```

### Demo3

```
i2c = I2C(I2C.I2C0, mode=I2C.MODE_MASTER, freq=400000, addr_size=7)
```

### Demo4: OLED(ssd1306 128x64)

```

import time
from machine import I2C

SSD1306_CMD  = 0
SSD1306_DATA = 1
SSD1306_ADDR = 0x3c

def oled_init(i2c):
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xAE, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x20, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x10, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xb0, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xc8, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x00, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x10, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x40, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x81, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xff, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xa1, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xa6, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xa8, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x3F, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xa4, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xd3, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x00, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xd5, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xf0, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xd9, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x22, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xda, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x12, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xdb, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x20, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x8d, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x14, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xaf, mem_size=8)



def oled_on(i2c):
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0X8D, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0X14, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0XAF, mem_size=8)

def oled_off(i2c):
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0X8D, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0X10, mem_size=8)
    i2c.writeto_mem(SSD1306_ADDR, 0x00, 0XAE, mem_size=8)

def oled_fill(i2c, data):
    for i in range(0,8):
        i2c.writeto_mem(SSD1306_ADDR, 0x00, 0xb0+i, mem_size=8)
        i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x10, mem_size=8)
        i2c.writeto_mem(SSD1306_ADDR, 0x00, 0x01, mem_size=8)
        for j in range(0,128):
            i2c.writeto_mem(SSD1306_ADDR, 0x40, data, mem_size=8)

i2c = I2C(I2C.I2C0, mode=I2C.MODE_MASTER, freq=400000, scl=28, sda=29, addr_size=7)

time.sleep(1)
oled_init(i2c)
oled_fill(i2c, 0xff)

```

### Demo5: Slave mode

```

from machine import I2C
import time

def on_receive(data):
    print(time.time(),"received data:",data)

def on_transmit():
    data = 0x88
    print(time.time(),"I will send:",data)
    return data

def on_event(event):
    if event == I2C.I2C_EV_START:
        print(time.time(),"received start event")
    elif event == I2C.I2C_EV_RESTART:
        print(time.time(),"received restart event")
    elif event == I2C.I2C_EV_STOP:
        print(time.time(),"received stop event")
    else:
        print(time.time(),"received unkown event")

i2c = I2C(I2C.I2C0, mode=I2C.MODE_SLAVE, scl=28, sda=29, addr = 0x24, addr_size=7, on_receive=on_receive, on_transmit=on_transmit, on_event=on_event)

```

## GPIO

### Demo1:GPIO OUTPUT test

```
import utime
from Maix import GPIO
fm.registered(board_info.LED_R,fm.fpioa.GPIO0)
led_r=GPIO(GPIO.GPIO0,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO0)
fm.registered(board_info.LED_R,fm.fpioa.GPIO1)
led_r=GPIO(GPIO.GPIO1,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO1)
fm.registered(board_info.LED_R,fm.fpioa.GPIO2)
led_r=GPIO(GPIO.GPIO2,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO2)
fm.registered(board_info.LED_R,fm.fpioa.GPIO3)
led_r=GPIO(GPIO.GPIO3,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO3)
fm.registered(board_info.LED_R,fm.fpioa.GPIO4)
led_r=GPIO(GPIO.GPIO4,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO4)
fm.registered(board_info.LED_R,fm.fpioa.GPIO5)
led_r=GPIO(GPIO.GPIO5,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO5)
fm.registered(board_info.LED_R,fm.fpioa.GPIO6)
led_r=GPIO(GPIO.GPIO6,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO6)
fm.registered(board_info.LED_R,fm.fpioa.GPIO7)
led_r=GPIO(GPIO.GPIO7,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO7)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS0)
led_r=GPIO(GPIO.GPIOHS0,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS0)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS1)
led_r=GPIO(GPIO.GPIOHS1,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS1)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS2)
led_r=GPIO(GPIO.GPIOHS2,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS2)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS3)
led_r=GPIO(GPIO.GPIOHS3,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS3)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS4)
led_r=GPIO(GPIO.GPIOHS4,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS4)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS5)
led_r=GPIO(GPIO.GPIOHS5,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS5)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS6)
led_r=GPIO(GPIO.GPIOHS6,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS6)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS7)
led_r=GPIO(GPIO.GPIOHS7,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS7)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS8)
led_r=GPIO(GPIO.GPIOHS8,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS8)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS9)
led_r=GPIO(GPIO.GPIOHS9,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS9)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS10)
led_r=GPIO(GPIO.GPIOHS10,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS10)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS11)
led_r=GPIO(GPIO.GPIOHS11,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS11)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS12)
led_r=GPIO(GPIO.GPIOHS12,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS12)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS13)
led_r=GPIO(GPIO.GPIOHS13,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS13)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS14)
led_r=GPIO(GPIO.GPIOHS14,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS14)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS15)
led_r=GPIO(GPIO.GPIOHS15,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS15)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS16)
led_r=GPIO(GPIO.GPIOHS16,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS16)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS17)
led_r=GPIO(GPIO.GPIOHS17,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS17)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS18)
led_r=GPIO(GPIO.GPIOHS18,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS18)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS19)
led_r=GPIO(GPIO.GPIOHS19,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS19)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS20)
led_r=GPIO(GPIO.GPIOHS20,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS20)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS21)
led_r=GPIO(GPIO.GPIOHS21,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS21)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS22)
led_r=GPIO(GPIO.GPIOHS22,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS22)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS23)
led_r=GPIO(GPIO.GPIOHS23,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS23)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS24)
led_r=GPIO(GPIO.GPIOHS24,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS24)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS25)
led_r=GPIO(GPIO.GPIOHS25,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS25)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS26)
led_r=GPIO(GPIO.GPIOHS26,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS26)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS27)
led_r=GPIO(GPIO.GPIOHS27,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS27)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS28)
led_r=GPIO(GPIO.GPIOHS28,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS28)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS29)
led_r=GPIO(GPIO.GPIOHS29,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS29)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS30)
led_r=GPIO(GPIO.GPIOHS30,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS30)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS31)
led_r=GPIO(GPIO.GPIOHS31,GPIO.OUT)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS31)

```

### Demo1:GPIO INPUT test
```
import utime
from Maix import GPIO
fm.registered(board_info.LED_R,fm.fpioa.GPIO0)
led_r=GPIO(GPIO.GPIO0,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO0)
fm.registered(board_info.LED_R,fm.fpioa.GPIO1)
led_r=GPIO(GPIO.GPIO1,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO1)
fm.registered(board_info.LED_R,fm.fpioa.GPIO2)
led_r=GPIO(GPIO.GPIO2,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO2)
fm.registered(board_info.LED_R,fm.fpioa.GPIO3)
led_r=GPIO(GPIO.GPIO3,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO3)
fm.registered(board_info.LED_R,fm.fpioa.GPIO4)
led_r=GPIO(GPIO.GPIO4,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO4)
fm.registered(board_info.LED_R,fm.fpioa.GPIO5)
led_r=GPIO(GPIO.GPIO5,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO5)
fm.registered(board_info.LED_R,fm.fpioa.GPIO6)
led_r=GPIO(GPIO.GPIO6,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO6)
fm.registered(board_info.LED_R,fm.fpioa.GPIO7)
led_r=GPIO(GPIO.GPIO7,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIO7)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS0)
led_r=GPIO(GPIO.GPIOHS0,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS0)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS1)
led_r=GPIO(GPIO.GPIOHS1,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS1)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS2)
led_r=GPIO(GPIO.GPIOHS2,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS2)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS3)
led_r=GPIO(GPIO.GPIOHS3,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS3)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS4)
led_r=GPIO(GPIO.GPIOHS4,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS4)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS5)
led_r=GPIO(GPIO.GPIOHS5,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS5)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS6)
led_r=GPIO(GPIO.GPIOHS6,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS6)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS7)
led_r=GPIO(GPIO.GPIOHS7,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS7)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS8)
led_r=GPIO(GPIO.GPIOHS8,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS8)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS9)
led_r=GPIO(GPIO.GPIOHS9,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS9)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS10)
led_r=GPIO(GPIO.GPIOHS10,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS10)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS11)
led_r=GPIO(GPIO.GPIOHS11,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS11)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS12)
led_r=GPIO(GPIO.GPIOHS12,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS12)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS13)
led_r=GPIO(GPIO.GPIOHS13,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS13)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS14)
led_r=GPIO(GPIO.GPIOHS14,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS14)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS15)
led_r=GPIO(GPIO.GPIOHS15,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS15)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS16)
led_r=GPIO(GPIO.GPIOHS16,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS16)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS17)
led_r=GPIO(GPIO.GPIOHS17,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS17)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS18)
led_r=GPIO(GPIO.GPIOHS18,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS18)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS19)
led_r=GPIO(GPIO.GPIOHS19,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS19)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS20)
led_r=GPIO(GPIO.GPIOHS20,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS20)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS21)
led_r=GPIO(GPIO.GPIOHS21,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS21)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS22)
led_r=GPIO(GPIO.GPIOHS22,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS22)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS23)
led_r=GPIO(GPIO.GPIOHS23,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS23)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS24)
led_r=GPIO(GPIO.GPIOHS24,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS24)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS25)
led_r=GPIO(GPIO.GPIOHS25,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS25)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS26)
led_r=GPIO(GPIO.GPIOHS26,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS26)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS27)
led_r=GPIO(GPIO.GPIOHS27,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS27)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS28)
led_r=GPIO(GPIO.GPIOHS28,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS28)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS29)
led_r=GPIO(GPIO.GPIOHS29,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS29)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS30)
led_r=GPIO(GPIO.GPIOHS30,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS30)
fm.registered(board_info.LED_R,fm.fpioa.GPIOHS31)
led_r=GPIO(GPIO.GPIOHS31,GPIO.IN)
utime.sleep_ms(500)
led_r.value()
fm.unregistered(board_info.LED_R,fm.fpioa.GPIOHS31)

```

### Demo1:GPIO IRQ test
```
import utime
from Maix import GPIO
def test_irq(GPIO,pin_num):
    print("key",pin_num,"\n")




fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS0)
key=GPIO(GPIO.GPIOHS0,GPIO.IN,GPIO.PULL_NONE)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7)

key.disirq()
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS1)
key=GPIO(GPIO.GPIOHS1,GPIO.IN,GPIO.PULL_NONE)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7)

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS1)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS2)
key=GPIO(GPIO.GPIOHS2,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS2)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS3)
key=GPIO(GPIO.GPIOHS3,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS3)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS4)
key=GPIO(GPIO.GPIOHS4,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS4)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS5)
key=GPIO(GPIO.GPIOHS5,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS5)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS6)
key=GPIO(GPIO.GPIOHS6,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS6)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS7)
key=GPIO(GPIO.GPIOHS7,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS7)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS8)
key=GPIO(GPIO.GPIOHS8,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS8)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS9)
key=GPIO(GPIO.GPIOHS9,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS9)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS10)
key=GPIO(GPIO.GPIOHS10,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS10)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS11)
key=GPIO(GPIO.GPIOHS11,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS11)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS12)
key=GPIO(GPIO.GPIOHS12,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS12)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS13)
key=GPIO(GPIO.GPIOHS13,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS13)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS14)
key=GPIO(GPIO.GPIOHS14,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS14)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS15)
key=GPIO(GPIO.GPIOHS15,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS15)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS16)
key=GPIO(GPIO.GPIOHS16,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS16)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS17)
key=GPIO(GPIO.GPIOHS17,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS17)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS18)
key=GPIO(GPIO.GPIOHS18,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS18)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS19)
key=GPIO(GPIO.GPIOHS19,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS19)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS20)
key=GPIO(GPIO.GPIOHS20,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS20)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS21)
key=GPIO(GPIO.GPIOHS21,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS21)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS22)
key=GPIO(GPIO.GPIOHS22,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS22)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS23)
key=GPIO(GPIO.GPIOHS23,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS23)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS24)
key=GPIO(GPIO.GPIOHS24,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS24)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS25)
key=GPIO(GPIO.GPIOHS25,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS25)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS26)
key=GPIO(GPIO.GPIOHS26,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS26)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS27)
key=GPIO(GPIO.GPIOHS27,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS27)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS28)
key=GPIO(GPIO.GPIOHS28,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS28)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS29)
key=GPIO(GPIO.GPIOHS29,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS29)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS30)
key=GPIO(GPIO.GPIOHS30,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS30)
fm.registered(board_info.BOOT_KEY,fm.fpioa.GPIOHS31)
key=GPIO(GPIO.GPIOHS31,GPIO.IN)
utime.sleep_ms(500)
key.value()
key.irq(test_irq,GPIO.IRQ_BOTH,GPIO.WAKEUP_NOT_SUPPORT,7) 

key.disirq()
fm.unregistered(board_info.BOOT_KEY,fm.fpioa.GPIOHS31)