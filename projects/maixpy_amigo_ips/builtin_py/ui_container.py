# This file is part of MaixUI
# Copyright (c) sipeed.com
#
# Licensed under the MIT license:
#   http://www.opensource.org/licenses/mit-license.php
#

import math
import os
import image
import gc

try:
  from core import agent, system
except ImportError:
  from lib.core import agent, system


class container:

  class demo:
    def load():
      pass
    def free():
      pass
    def event():
      pass

  current, temp, history = demo, None, None

  def forever():
    if container.temp != None:
      container.current, container.temp = container.temp, None
      container.current.load()
    if container.current != None:
      container.current.event()

  def reload(app):
    if container.current != None:
      tmp, container.current = container.current, None
      tmp.free()
      container.history, container.temp = tmp, app

  def latest():
    container.reload(container.history)

if __name__ == "__main__":
  from ui.ui_canvas import ui
  #from ui.ui_taskbar import taskbar

  def switch_Launcher(self):
    container.reload(Launcher)
    self.remove(switch_Launcher)

  def switch_Loading(self):
    container.reload(Loading)
    self.remove(switch_Loading)

  class Loading:
    h, w = 0, 0
    def load():
      Loading.h, Loading.w = 20, 20
      #system.remove(Loading.event)
      system.event(2000, switch_Launcher)
    def free():
      pass
    def event():
      pass
    @ui.warp_template(ui.blank_draw)
    @ui.warp_template(ui.grey_draw)
    @ui.warp_template(ui.bg_in_draw)
    def draw():
      ui.display()
    def event():
      if Loading.h < 240:
        Loading.h += 20
      if Loading.w < 240:
        Loading.w += 10
      ui.height, ui.weight = Loading.h, Loading.w
      Loading.draw()

  class Launcher:
    h, w = 0, 0
    def load():
      Launcher.h, Launcher.w = 20, 20
      system.event(2000, switch_Loading)
    def free():
      pass
    @ui.warp_template(ui.blank_draw)
    @ui.warp_template(ui.grey_draw)
    @ui.warp_template(ui.help_in_draw)
    @ui.warp_template(ui.anime_in_draw)
    def draw():
      ui.display()
    def event():
      if Launcher.h < 240:
        Launcher.h += 20
      if Launcher.w < 240:
        Launcher.w += 10
      ui.height, ui.weight = Launcher.h, Launcher.w
      Launcher.draw()

  # Application

  import time
  last = time.ticks_ms()
  container.reload(Loading)
  while True:
    container.forever()
    system.parallel_cycle()
    gc.collect()
    print('ram total : ' + str(gc.mem_free() / 1024) + ' kb')
