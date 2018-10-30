UART                    ok
GPIO                    ok
GPIOHS(上下沿触发)      pause
PWM                     test
SYS
----------------------
I2C
SPI
I2S
RTC

DVP
WDT

TIMER
PLIC
FFT
----------------------

SHA256
aes

FPIOA(首先包含在其它模块的初始化中，后面再考虑单独出来)
DMAC (包含在其他模块内)



应用
文件系统（SPIFFS）
upip
设备抽象
 spi flash操作
 tf 操作
 按键
 RGB LED
 WS2812
 MCU LCD
 MIC
 Speaker
 camera
基础网络
mqtt
