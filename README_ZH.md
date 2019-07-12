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

[English](README.md)

<br />
<br />

**MaixPy, 让 AIOT 更简单～**

Maixpy 的目的是让 AIOT 编程更简单， 基于 [Micropython](http://www.micropython.org) 语法, 运行在一款有着便宜价格的高性能芯片 [K210](https://kendryte.com) 上.
> K210 简介 : 
> * 拥有硬件加速的图像识别
> * 带硬件浮点运算的双核处理器
> * 8MB(6MB+2MB) 内存
> * 16MB 外置 Flash
> * 芯片 CPU 最高可达 800MHz 主频 (开发板支持最高主频具体看开发板介绍)
> * 麦克风阵列支持（8个麦克风）
> * 硬件 AES SHA256 支持
> * FPIOA (每个外设可以映射到任意引脚)
> * 外设: I2C, SPI, I2S, WDT, TIMER, RTC, UART, GPIO 等等

![](assets/image/maix_bit.png)

## 简单易懂的代码

寻找 I2C 设备:

```python
from machine import I2C

i2c = I2C(I2C.I2C0, freq=100000, scl=28, sda=29)
devices = i2c.scan()
print(devices)
```

拍照:

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

## 固件发布

发布版本固件： [固件发布页面](https://github.com/sipeed/MaixPy/releases)

最新提交（开发中）的固件: [master 分支的固件](http://dl.sipeed.com/MAIX/MaixPy/release/master/)

## 文档

查看 [maixpy.sipeed.com](https://maixpy.sipeed.com)

## 例示代码

[MaixPy_scripts](https://github.com/sipeed/MaixPy_scripts)

## 从源码构建自己的固件

参考 [构建文档](build.md)

旧的构建版本请见 [historic 分支](https://github.com/sipeed/MaixPy/tree/historic) （不再维护，仅仅为了保留提交记录）

## 开源协议

查看 [LICENSE](LICENSE.md) 文件

## 其它： 使用本仓库作为 `SDK` 用 `C` 语言开发

本仓库除了作为 `MaixPy` 工程的源码存在以外， 由于`MaixPy`作为一个组件存在， 可以配置为不参与编译， 所以也可以作为 `C SDK` 来进行开发， 使用方法见 [构建文档](build.md), 可以从编译下载`projects/hello_world`开始

大致上编译下载过程如下：

```
wget http://dl.cdn.sipeed.com/kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz
sudo tar -Jxvf kendryte-toolchain-ubuntu-amd64-8.2.0-20190409.tar.xz -C /opt
cd projects/hello_world
python3 project.py menuconfig
python3 project.py build
python3 project.py flash -B dan -b 1500000 -p /dev/ttyUSB0 -t
```

