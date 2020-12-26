
# This file is part of MaixUI
# Copyright (c) sipeed.com
#
# Licensed under the MIT license:
#   http://www.opensource.org/licenses/mit-license.php
#

class agent:

  def __init__(self):
    self.msg = []
    self.arg = {}

  def get_ms(self):
    import time
    # return time.time() * 1000
    return time.ticks_ms()

  def event(self, cycle, func, args=None):
    # arrived, cycle, function
    tmp = (self.get_ms() + cycle, cycle, func, args)
    #print('self.event', tmp)
    self.msg.append(tmp)

  def remove(self, func):
    #print(self.remove)
    for pos in range(len(self.msg)): # maybe use map
      #print(self.msg[pos][2], func)
      if self.msg[pos][2] == func:
          self.msg.remove(self.msg[pos])
          break
    #print(self.msg)

  def call(self, obj, pos=0):
    #print(self.call, pos, obj)
    self.msg.pop(pos)
    self.event(obj[1], obj[2], obj[3])
    obj[2](obj[3]) if obj[3] else obj[2]() # exec function

  def cycle(self):
    if (len(self.msg)):
      obj = self.msg[0]
      if (self.get_ms() >= obj[0]):
        self.call(obj, 0)

  def parallel_cycle(self):
    for pos in range(len(self.msg)): # maybe use map
      obj = self.msg[pos]
      if (self.get_ms() >= obj[0]):
        self.call(obj, pos)
        break

  def unit_test(self):

    class tmp:
        def test_0(self):
          print('test_0')

        def test_1():
          print('test_1')

        def test_2(self):
          print('test_2')
          self.remove(tmp.test_1)
          self.event(1000, tmp.test_1)
          self.remove(tmp.test_2)

    import time
    self.event(200, tmp.test_0, self)
    self.event(10, tmp.test_1)
    self.event(2000, tmp.test_2, self)
    while True:
      self.parallel_cycle()
      time.sleep(0.1)

system = agent()

if __name__ == "__main__":
  #agent().unit_test()
  system.unit_test()
