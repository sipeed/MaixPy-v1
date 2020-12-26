# This file is part of MaixUI
# Copyright (c) sipeed.com
#
# Licensed under the MIT license:
#   http://www.opensource.org/licenses/mit-license.php
#

import time
import gc
import math
import random

try:
    from ui_canvas import ui
    from button import sipeed_button, button_io
    import camera
    from ai_track import find_color
    from ai_classify import HowMany, MaybeIs, MoblieNet
    from ai_face import FaceReco, FaceDetect
    from ai_ocr import Minist
    from led import sipeed_led
except ImportError:
    from ui.ui_canvas import ui
    from driver.button import sipeed_button, button_io
    import driver.camera as camera
    from driver.ai_track import find_color
    from driver.ai_classify import HowMany, MaybeIs, MoblieNet
    from driver.ai_face import FaceReco, FaceDetect
    from driver.ai_ocr import Minist
    from driver.led import sipeed_led

import KPU as kpu
from Maix import utils


class ai_sample():

    is_load = False

    def load():
        if ai_sample.is_load == False:
            #print(ai_sample.load)
            ai_sample.is_load = True

    def work(img):
        #print(ai_sample.work)
        # img.draw_string(20, 2, 'sample free %d kb' %
        #                 (utils.heap_free() / 1024), (127, 255, 255), scale=2)
        return img

    def free():
        if ai_sample.is_load:
            #print(ai_sample.free)
            ai_sample.is_load = False


class ai_camera():

    index, model, models = 0, ai_sample, [ai_sample, FaceDetect, FaceReco, find_color, HowMany, MaybeIs, MoblieNet, Minist]
    btn, replace = sipeed_button(), False
    backlight = 1

    def reload():
        if ai_camera.model:
            ai_camera.model.free()
            ai_camera.model = ai_camera.models[ai_camera.index]
            ai_camera.model.load()

    def back():
        ai_camera.index -= 1
        ai_camera.replace = True

    def next():
        ai_camera.index += 1
        ai_camera.replace = True

    def jump(index):
        ai_camera.index = index
        ai_camera.replace = True

    def exit():
        sipeed_led.w.value(1)

    def ai_draw():
        ai_camera.btn.event()

        if ai_camera.btn.home() == 2:
            ai_camera.backlight = not ai_camera.backlight
            sipeed_led.w.value(ai_camera.backlight)

        if ai_camera.btn.back() == 2:
            ai_camera.back()
        elif ai_camera.btn.next() == 2:
            ai_camera.next()

        ai_camera.index = ai_camera.index % len(ai_camera.models)

        if ai_camera.replace:
            ai_camera.reload()
            ai_camera.replace = False

        tmp = camera.obj.get_image()
        if ai_camera.model and ai_camera.model.is_load:
            ai_camera.model.work(tmp)

        ui.canvas.draw_image(tmp, 0, 0)


ai_camera.reload()

if __name__ == "__main__":

    ui.height, ui.weight = 480, 320
    button_io.config(23, 31, 20) # amigo
    def test_ai_camera():

        @ui.warp_template(ui.blank_draw)
        @ui.warp_template(ai_camera.ai_draw)
        def app_main():
            ui.display()

        import time
        last = time.ticks_ms()
        while True:
            kpu.memtest()
            try:
                print(time.ticks_ms() - last)
                last = time.ticks_ms()
                app_main()
            except Exception as e:
                # gc.collect()
                print(e)
            kpu.memtest()

    test_ai_camera()
