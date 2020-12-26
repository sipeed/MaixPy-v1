<img height=230 src="assets/image/maixpy.png">

<br />

<div class="title_pic">
    <img src="assets/image/sipeed_logo.svg"  style="margin-right: 10px;" height=45> <img src="assets/image/micropython.png" height=50>
</div>

<br />
<br />

<a href="https://github.com/sipeed/MaixPy/actions">
    <img src="https://img.shields.io/github/workflow/status/Sipeed/MaixPy/compile%20test%20and%20publish?style=for-the-badge" alt="Master branch build status" />
</a>
<a href="http://dl.sipeed.com/MAIX/MaixPy/release/master/">
    <img src="https://img.shields.io/badge/download-master-ff69b4.svg?style=for-the-badge" alt="master build firmware" />
</a>
<a href="https://github.com/sipeed/MaixPy/releases">
    <img src="https://img.shields.io/github/release/sipeed/maixpy.svg?style=for-the-badge" alt="Latest release version" />
</a>
<a href="https://github.com/sipeed/MaixPy/blob/master/LICENSE.md">
    <img src="https://img.shields.io/badge/license-Apache%20v2.0-orange.svg?style=for-the-badge" alt="License" />
</a>

<br />

<a href="https://github.com/sipeed/MaixPy/issues?utf8=%E2%9C%93&q=is%3Aissue+label%3A%22good+first+issue%22">
    <img src="https://img.shields.io/github/issues/sipeed/maixpy/good%20first%20issue.svg?style=for-the-badge" alt="Good first issues" />
</a>
<a href="https://github.com/sipeed/MaixPy/issues?q=is%3Aopen+is%3Aissue+label%3Abug">
    <img src="https://img.shields.io/github/issues/sipeed/maixpy/bug.svg?style=for-the-badge" alt="Bug issues" />
</a>
<a href="https://github.com/sipeed/MaixPy/issues?q=is%3Aissue+is%3Aopen+label%3Aenhancement">
    <img src="https://img.shields.io/github/issues/sipeed/maixpy/enhancement.svg?style=for-the-badge" alt="Enhancement issues" />
</a>



<br/>
<br/>

[中文](README_ZH.md)

<br />
<br />

**Let's Sipeed up, Maximize AI's power!**

**MaixPy, makes AIOT easier!**

Maixpy is designed to make AIOT programming easier, based on the [Micropython](http://www.micropython.org) syntax, running on a very powerful embedded AIOT chip [K210](https://kendryte.com).

There are many things you can do with MaixPy, please refer to [here](https://maixpy.sipeed.com/en/others/what_maix_do.html)

> K210 brief: 
> * Image Recognition with hardware AI acceleration
> * Dual core with FPU
> * 8MB(6MB+2MB) RAM
> * 16MB external Flash
> * Max 800MHz CPU freq (see the dev board in detail, usually 400MHz)
> * Microphone array(8 mics)
> * Hardware AES SHA256
> * FPIOA (Periphrals can map to any pins)
> * Peripherals: I2C, SPI, I2S, WDT, TIMER, RTC, UART, GPIO etc.

<img src="assets/image/maix_bit.png" height=500 alt="maix bit"/>


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

Use AI model to recognize object:
```python
import KPU as kpu
import sensor

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.set_windowing((224, 224))

model = kpu.load("/sd/mobilenet.kmodel")  # load model
while(True):
    img = sensor.snapshot()               # take picture by camera
    out = kpu.forward(task, img)[:]       # inference, get one-hot output
    print(max(out))                       # print max probability object ID
```
> please read doc before run it

## Release

See [Releases page](https://github.com/sipeed/MaixPy/releases)

Get latest commit firmware: [master firmware](http://dl.sipeed.com/MAIX/MaixPy/release/master/)

Custom your firmware, see [build](#build-from-source) or use [online custom tool](#use-online-compilation-tools-to-customize-firmware)

## Documentation

Doc refer to [maixpy.sipeed.com](https://maixpy.sipeed.com)

## Examples

[MaixPy_scripts](https://github.com/sipeed/MaixPy_scripts)

## Build From Source

See [build doc](build.md)

The historic version see [historic branch](https://github.com/sipeed/MaixPy/tree/historic) (No longer maintained, just keep commit history)

## Use online compilation tools to customize firmware

Go to [maixhub.com](https://www.maixhub.com/compile.html) to use online compilation to customize the functions you need


## Model hub: Maixhub.com

Find more models on [Maixhub.com](https://maixhub.com)


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


