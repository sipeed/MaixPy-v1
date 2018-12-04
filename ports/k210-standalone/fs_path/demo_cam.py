import machine
st7789=machine.st7789()
st7789.init()
ov2640=machine.ov2640()
ov2640.init()
image=bytearray(320*240*2)
while(1):
    ov2640.get_image(image)
    st7789.draw_picture_default(image)
