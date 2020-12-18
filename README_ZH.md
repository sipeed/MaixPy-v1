<img height=200 src="assets/image/maixpy.png">

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


[English](README.md)

<br />
<br />

**MaixPy, 让 AIOT 更简单～**

Maixpy 的目的是让 AIOT 编程更简单， 基于 [Micropython](http://www.micropython.org) 语法, 运行在一款有着便宜价格的高性能 AIOT 芯片 [K210](https://kendryte.com) 上.

利用 MaixPy 可以做很多事情,具体参考 [这里](https://maixpy.sipeed.com/zh/others/what_maix_do.html)

> K210 简介 : 
> * 拥有硬件加速的 AI 图像识别
> * 带硬件浮点运算的双核处理器
> * 8MB(6MB+2MB) 内存
> * 16MB 外置 Flash
> * 芯片 CPU 最高可达 800MHz 主频 (开发板支持最高主频具体看开发板介绍, 通常400MHz)
> * 麦克风阵列支持（8个麦克风）
> * 硬件 AES SHA256 支持
> * FPIOA (每个外设可以映射到任意引脚)
> * 外设: I2C, SPI, I2S, WDT, TIMER, RTC, UART, GPIO 等等


<img src="assets/image/maix_bit.png" height=500 alt="maix bit"/>

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

使用 AI 模型进行物体识别:
```python
import KPU as kpu
import sensor

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.set_windowing((224, 224))

model = kpu.load("/sd/mobilenet.kmodel")  # 加载模型
while(True):
    img = sensor.snapshot()               # 从摄像头采集照片
    out = kpu.forward(task, img)[:]       # 推理，获得 one-hot 输出
    print(max(out))                       # 打印最大概率的物体ID
```
> 具体的使用方法请阅读教程后尝试


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

## 使用在线编译工具定制固件

到[Maixhub.com](https://www.maixhub.com/compile.html)使用在线编译定制自己需要的功能

## Maixhub 模型平台

到 [Maixhub.com] 获取更多模型和训练自己的模型


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

