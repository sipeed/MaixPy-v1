MaixPy
=======================

</br>

<div class="title_pic">
    <img src="ports/k210-freertos/docs/assets/micropython.png"><img src="ports/k210-freertos/docs/assets/icon_sipeed2.png"  height="60">
</div>

</br>
</br>

**Let's Sipeed up, Maximize AI's power!**

**MaixPy, makes AIOT easier!**

Maixpy is designed to make AIOT programming easier, based on the [Micropython]((http://www.micropython.org)) syntax, running on a very powerful embedded AIOT chip [K210](https://kendryte.com).

## Simple code

Find I2C devices:

```python
from machine import I2C

i2c = I2C(I2C.I2C0, freq=100000, scl=28, sda=29)
devices = i2c.scan()
print(devices)
```

Take picture:

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

## Documentation

Refer to [maixpy.sipeed.com](https://maixpy.sipeed.com)


## Build From Source

See [build doc](ports/k210-freertos/README.md)

## License

See [LICENSE](LICENSE) file

