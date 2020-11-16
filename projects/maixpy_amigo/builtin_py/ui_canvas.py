# This file is part of MaixUI
# Copyright (c) sipeed.com
#
# Licensed under the MIT license:
#   http://www.opensource.org/licenses/mit-license.php
#

import image, lcd, math, gc, random, os

# gc.collect()

lcd.init(freq=15000000)

def print_mem_free():
    print('ram total : ' + str(gc.mem_free() / 1024) + ' kb')


class ui:

    alpha, img, anime, bak = 0, None, None, None

    bg_path = os.getcwd() + "/res/images/bg.jpg"
    logo_path = os.getcwd() + "/res/images/logo.jpg"

    height, weight = lcd.height(), lcd.width()
    enable = True

    def warp_template(func):
        def tmp_warp(warp=None):
            if warp:
              return lambda *args: [func(), warp()]
        return tmp_warp

    def blank_draw():
        if ui.enable:
            ui.canvas = image.Image(size=(ui.height, ui.weight)
                                )  # 10ms # 168.75kb (112kb)

    def grey_draw():
        if ui.enable:
            ui.canvas.draw_rectangle((0, 0, ui.height, ui.weight),
                                 fill=True, color=(75, 75, 75))

    def bg_in_draw():
        if ui.enable:
            #ui.canvas.draw_rectangle((0, 0, ui.height, ui.weight),
                                    #fill=True, color=(75, 75, 75))
            #if ui.bak == None:
            #ui.bak.draw_rectangle((60,30,120,150), fill=True, color=(250, 0, 0))
            #ui.bak.draw_string(70, 40, "o", color=(255, 255, 255), scale=2)
            #ui.bak.draw_string(80, 10, "s", color=(255, 255, 255), scale=15)
            ui.canvas.draw_circle(121, 111, int(50),
                                color=(64, 64, 64), thickness=3)  # 10ms
            ui.canvas.draw_circle(120, 110, int(50),
                                color=(250, 0, 0))  # 10ms
            ui.canvas.draw_circle(120, 110, int(50), fill=True,
                                color=(250, 0, 0))  # 10ms

            sipeed = b'\x40\xA3\x47\x0F\x18\x38\x18\x0F\x07\x03\x00\x00\x00\x0F\x0F\x0F\x00\xFC\xFC\xFC\x00\x00\x00\xF0\xF8\xFC\x06\x07\x06\xFC\xF8\xF0'

            ui.canvas.draw_font(88, 80, 16, 16, sipeed, scale=4, color=(64,64,64))

            ui.canvas.draw_font(86, 78, 16, 16, sipeed, scale=4, color=(255,255,255))


            # ui.canvas = ui.canvas # 15ms
            #ui.canvas = ui.bak.copy() # 10ms 282kb

    def bg_draw():
        if ui.enable:
            if ui.bak == None:
                ui.bak = image.Image(ui.bg_path)  # 90ms
            ui.canvas.draw_image(ui.bak, 0, 0)  # 20ms

    def help_in_draw():
        if ui.enable:
            ui.canvas.draw_string(30, 6, "<", (255, 0, 0), scale=2)
            ui.canvas.draw_string(60, 6, "ENTER/HOME", (255, 0, 0), scale=2)
            ui.canvas.draw_string(200, 6, ">", (255, 0, 0), scale=2)
            ui.canvas.draw_string(10, ui.height - 30,
                                "RESET", (255, 0, 0), scale=2)
            ui.canvas.draw_string(178, ui.height - 30,
                                "POWER", (255, 0, 0), scale=2)

    def anime_draw(alpha=None):
        if ui.enable:
            if alpha == None:
                alpha = math.cos(math.pi * ui.alpha / 32) * 80 + 170
                ui.alpha = (ui.alpha + 1) % 64
            if ui.anime == None:
                ui.anime = image.Image(ui.logo_path)  # 90ms
            ui.canvas.draw_image(ui.anime, 70, 70, alpha=int(alpha))  # 15ms

    def anime_in_draw(alpha=None):
        if ui.enable:
            if alpha == None:
                alpha = math.cos(math.pi * ui.alpha / 100) * 200
                ui.alpha = (ui.alpha + 1) % 200
            r, g, b = random.randint(120, 255), random.randint(
                120, 255), random.randint(120, 255)
            ui.canvas.draw_circle(0, 0, int(alpha), color=(
                r, g, b), thickness=(r % 5))  # 10ms
            ui.canvas.draw_circle(0, 0, 200 - int(alpha),
                                color=(r, g, b), thickness=(g % 5))  # 10ms

            ui.canvas.draw_circle(240, 0, int(alpha), color=(
                r, g, b), thickness=(b % 5))  # 10ms
            ui.canvas.draw_circle(240, 0, 200 - int(alpha),
                                color=(r, g, b), thickness=(r % 5))  # 10ms

            ui.canvas.draw_circle(0, 240, int(alpha), color=(
                r, g, b), thickness=(g % 5))  # 10ms
            ui.canvas.draw_circle(0, 240, 200 - int(alpha),
                                color=(r, g, b), thickness=(b % 5))  # 10ms

            ui.canvas.draw_circle(240, 240, int(alpha), color=(
                r, g, b), thickness=(r % 5))  # 10ms
            ui.canvas.draw_circle(240, 240, 200 - int(alpha),
                                color=(r, g, b), thickness=(g % 5))  # 10ms

    def display():  # 10ms
        try:
            if ui.canvas != None:
                lcd.display(ui.canvas)
        finally:
            try:
                if ui.canvas != None:
                    tmp = ui.canvas
                    ui.canvas = None
                    del tmp
            except Exception as e:
                pass
                # gc.collect()

if __name__ == "__main__":
    # ui.height, ui.weight = 480, 320 # amigo
    #@ui.warp_template(ui.blank_draw)
    #@ui.warp_template(ui.bg_draw)
    #@ui.warp_template(ui.anime_draw)
    # 50ms
    @ui.warp_template(ui.blank_draw)
    #@ui.warp_template(ui.grey_draw)
    @ui.warp_template(ui.bg_in_draw)
    #@ui.warp_template(ui.anime_in_draw)
    #@ui.warp_template(ui.help_in_draw)
    # 20ms
    def test_launcher_draw():
        #ui.bg_draw()
        ui.display()

    import time
    last = time.ticks_ms()
    while True:
        try:
            print(time.ticks_ms() - last)
            last = time.ticks_ms()
            # gc.collect()
            test_launcher_draw()
        except Exception as e:
            print(e)
