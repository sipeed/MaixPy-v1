# This file is part of MaixUI
# Copyright (c) sipeed.com
#
# Licensed under the MIT license:
#   http://www.opensource.org/licenses/mit-license.php
#

import time

FT_DEVIDE_MODE = 0x00
FT_ID_G_MODE = 0xA4
FT_ID_G_THGROUP = 0x80
FT_ID_G_PERIODACTIVE = 0x88

FT6X36_ADDR = 0x38

class TouchLow:
  i2c3 = None
  addr = 0x0

  def config(i2c3, addr=FT6X36_ADDR):
    TouchLow.i2c3 = i2c3
    TouchLow.addr = addr

  def write_reg(reg_addr, buf):
    TouchLow.i2c3.writeto_mem(TouchLow.addr, reg_addr, buf, mem_size=8)

  def read_reg(reg_addr, buf_len):
    return TouchLow.i2c3.readfrom_mem(TouchLow.addr, reg_addr, buf_len, mem_size=8)

  def config_ft6x36():
    TouchLow.write_reg(FT_DEVIDE_MODE, 0); # 进入正常操作模式
    TouchLow.write_reg(FT_ID_G_THGROUP, 12); # 设置触摸有效值，触摸有效值，12，越小越灵敏
    TouchLow.write_reg(FT_DEVIDE_MODE, 14); # 激活周期，不能小于12，最大14

  def get_point():
    if TouchLow.i2c3 != None:
      #data = self.read_reg(0x01, 1)
      #print("get_gesture:" + str(data))
      data = TouchLow.read_reg(0x02, 1)
      #print("get_points:" + str(data))
      if (data != None and data[0] == 0x1):
          data_buf = TouchLow.read_reg(0x03, 4)
          y = ((data_buf[0] & 0x0f) << 8) | (data_buf[1])
          x = ((data_buf[2] & 0x0f) << 8) | (data_buf[3])
          #print("1 point[{}:{}]".format(x,y))
          if ((data_buf[0] & 0xc0) == 0x80):
              #print("2 point[({},{}):({},{})]".format(
                  #x, y,  self.width - x, self.height - y))
              return (x, y)
    return None

class Touch:

  idle, press, release = 0, 1, 2

  def __init__(self, w, h, cycle=1000):
    self.cycle = cycle
    self.last_time = 0
    self.points = [(0, 0, 0), (0, 0, 0)]
    self.state = Touch.idle
    self.width, self.height = w, h

  def event(self):
    tmp = TouchLow.get_point()
    if tmp != None:
      x, y = tmp
      self.last_time = time.ticks_ms()
      if self.state != Touch.press:
          self.state = Touch.press
          self.points[0] = (x, y, time.ticks_ms())
      self.points[1] = (x, y, time.ticks_ms())

    # timeout return ilde.
    if time.ticks_ms() > self.last_time + self.cycle:
        if self.state == Touch.release:
            self.state = Touch.idle
            self.points = [(0, 0, 0), (0, 0, 0)]
            return
        if self.state == Touch.press:
            self.state = Touch.release
            return

if __name__ == "__main__":

  import lcd
  from machine import I2C

  i2c = I2C(I2C.I2C3, freq=1000*1000, scl=24, sda=27) # amigo
  devices = i2c.scan()
  print(devices)
  TouchLow.config(i2c)
  tmp = Touch(480, 320, 200)
  while 1:
    #tmp.get_point()
    tmp.event()
    print(tmp.state, tmp.points)
    #time.sleep_ms(200)
