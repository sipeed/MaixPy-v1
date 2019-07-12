<img width=205 src="assets/image/maixpy.png">

<br />
<br />


<div class="title_pic">
    <img src="assets/image/micropython.png"><img src="assets/image/icon_sipeed2.png"  height="60">
</div>

<br />

<a href="https://travis-ci.org/sipeed/MaixPy">
    <img src="https://travis-ci.org/sipeed/MaixPy.svg?branch=master" alt="Master branch build status" />
</a>
<a href="http://dl.sipeed.com/MAIX/MaixPy/release/master/">
    <img src="https://img.shields.io/badge/build-master-ff69b4.svg" alt="master build firmware" />
</a>
<a href="https://github.com/sipeed/MaixPy/releases">
    <img src="https://img.shields.io/github/release/sipeed/maixpy.svg" alt="Latest release version" />
</a>
<a href="https://github.com/sipeed/MaixPy/blob/master/LICENSE.md">
    <img src="https://img.shields.io/badge/license-Apache%20v2.0-orange.svg" alt="License" />
</a>

<br />

<a href="https://github.com/sipeed/MaixPy/issues?utf8=%E2%9C%93&q=is%3Aissue+label%3A%22good+first+issue%22">
    <img src="https://img.shields.io/github/issues/sipeed/maixpy/good%20first%20issue.svg" alt="Good first issues" />
</a>
<a href="https://github.com/sipeed/MaixPy/issues?q=is%3Aopen+is%3Aissue+label%3Abug">
    <img src="https://img.shields.io/github/issues/sipeed/maixpy/bug.svg" alt="Bug issues" />
</a>
<a href="https://github.com/sipeed/MaixPy/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement">
    <img src="https://img.shields.io/github/issues/sipeed/maixpy/enhancement.svg" alt="Enhancement issues" />
</a>



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

![](assets/image/maix_bit.png)

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

## Examples

[MaixPy_scripts](https://github.com/sipeed/MaixPy_scripts)

## Build From Source

See [build doc](build.md)

The historic version see [historic branch](https://github.com/sipeed/MaixPy/tree/historic) (No longer maintained, just keep commit history)

## License

See [LICENSE](LICENSE.md) file


## Other: As C SDK for C developers


In addition to the source code of the `MaixPy` project, since `MaixPy` exists as a component, it can be configured to not participate in compilation, so this repository can also be developed as `C SDK`. For the usage details, see [Building Documentation](build.md), which can be started by compiling and downloading `projects/hello_world`.

The compilation process is briefly as follows:

```
wget http://dl.cdn.sipeed.com/kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz
sudo tar -Jxvf kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz -C /opt
cd projects/hello_world
python3 project.py menuconfig
python3 project.py build
python3 project.py flash -B dan -b 1500000 -p /dev/ttyUSB0 -t
```


