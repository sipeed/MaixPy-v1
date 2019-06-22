Changelog for Kendryte K210
======

## 0.1.0

Kendryte K210 first SDK with FreeRTOS, have fun. 

## 0.2.0

- Major changes
  - Rework trap handling 
  - New functions to enable spi0 and dvp pin 
  - New functions to select IO power mode
- Breaking changes
  - Modify struct enum union format
- Non-breaking bug fixes
  - Fix spi lcd unwork issues
  - Fix dual core startup issues
  - Use "__global_pointer$" instead of "_gp"
  
## 0.3.0

- Major change
  - Modify AES、FFT、SHA、I2C、SPI、WDT、SPI driver
- Breaking changes
  - Modify struct enum union format
- Non-breaking bug fixes
  - Fix out of memory issues
  - Fix lcd unused issues

## 0.4.1

- Major change
  - Add dma support for aes driver
  - Add uarths driver
  - Add dma interrupt handler

- Non-breaking bug fixes
  - Fix the procedure of setting pll
  - Fix wdt interrupt bug
  - Fix serveral bugs in i2s drivers
  
## 0.5.0
  
- Major change
  - Add KPU driver
  - Find toolchain automatically

- Non-breaking bug fixes
  - Fix aes gcm bug
  - Fix dmac interrupt bug
  - Fix i2s  transfer bug

## 0.5.1

- Major changes
  - Add i2c slave driver
  
- Non-breaking bug fixes
  - Fix pll init issues
  - Fix spi receive mode issues
  - Fix redefine function does not report error issues
  - Reduce stack size
  
## 0.5.2
- Major change
  - Add KPU driver for latest model compiler
  - Automatic set PROJ if user not set it
  - Update timer driver to support better interrupt
  - Add uart dma and interrupt function
- Non-breaking bug fixes
  - Fix rtc issues
  - Fix sccb issues

- Breaking change
  - Fix timer interrupt lost problem
  - Add new timer interrupt API

## 0.5.3
- Major change
  - Modify KPU driver for latest model compiler
  - Add freertos
  - Add new gpiohs and wdt interrupt function
  - Add dvp xclk setting
  - Add sysctl reset status

- Non-breaking bug fixes
  - Fix i2c issues
  - Fix spi issues

- Breaking change
  - Fix uarths stopbit problem
  - Fix core1 stack problem
  - Fix core1 interrupt problem
  
## 0.5.4
- Major change
  - Modify KPU driver for NNCASE
  - Add APU driver
  - Add support for new toolchain
  - UART use shadow regs
  - Add spi slave driver
  - Add travis CI script

- Non-breaking bug fixes
  - Fix float issues

- Breaking change
  - Fix bus reset problem
  
## 0.5.5
- Major change
  - Add SPI I2C I2S UART DMA callback
  - Add malloc lock
  - Update WIN32 cmake program auto-set
  - Upadte KPU driver for lastest NNCASE

- Non-breaking bug fixes
  - Fix double issues
  - Fix GPIO issues

- Breaking change
  - Fix device reset problem
