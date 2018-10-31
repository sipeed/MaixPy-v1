import machine

st7789=machine.st7789()
st7789.init()
ov2640=machine.ov2640()
ov2640.init()
buf=bytearray(320*240*2)
demo=machine.demo_face_detect()
