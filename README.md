<img width=205 src="ports/k210-freertos/docs/assets/maixpy.png">
=======================



<div class="title_pic">
    <img src="ports/k210-freertos/docs/assets/micropython.png"><img src="ports/k210-freertos/docs/assets/icon_sipeed2.png"  height="60">
</div>

<br />


[![Build Status](https://travis-ci.org/sipeed/MaixPy.svg?branch=master)](https://travis-ci.org/sipeed/MaixPy)

<br />

[中文](README_ZH.md)

<br />
<br />

**Let's Sipeed up, Maximize AI's power!**

**MaixPy, makes AIOT easier!**

Maixpy is designed to make AIOT programming easier, based on the [Micropython](http://www.micropython.org) syntax, running on a very powerful embedded AIOT chip [K210](https://kendryte.com).
> K210 brief: 
> * Image Recognition with hardware acceleration
> * Dual core with FPU
> * 8MB(6MB+2MB) RAM
> * 16MB external Flash
> * Max 800MHz CPU freq (see the dev board in detail)
> * Microphone array(8 mics)
> * Hardware AES SHA256
> * FPIOA (Periphrals can map to any pins)
> * Peripherals: I2C, SPI, I2S, WDT, TIMER, RTC, UART, GPIO etc.

![](ports/k210-freertos/docs/assets/maix_bit.png)

## Simple code

Find I2C devices:

```python
from machine import I2C

i2c = I2C(I2C.I2C0, freq=100000, scl=28, sda=29)
devices = i2c.scan()
print(devices)
```

Take picture:

```python
import sensor
import image
import lcd

lcd.init()
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.run(1)
while True:
    img=sensor.snapshot()
    lcd.display(img)
```

## Release

See [Releases page](https://github.com/sipeed/MaixPy/releases)

Get latest commit firmware: [master firmware](http://dl.sipeed.com/MAIX/MaixPy/release/master/)

## Documentation

Doc refer to [maixpy.sipeed.com](https://maixpy.sipeed.com)


## Build From Source

See [build doc](ports/k210-freertos/README.md)

## License

See [LICENSE](LICENSE.md) file

