## What is config.json

differences in various hardware IO definitions, such as the difference in LCD / Sensor / SdCard resources in the system.

> MaixPy 发展的过程中出现了各类硬件 IO 定义差异较大的地方，例如系统中 LCD / Sensor / SdCard 资源的差异。

Therefore, MaixPy has added config.json to adapt to different hardware parameters. It can remap the IO resources used at the bottom. For the versions provided by Sipeed, the corresponding config.json is stored in this directory for your reference and use.

> 所以 MaixPy 加入了 config.json 来适配不同的硬件参数，它可以重映射底层使用的 IO 资源， 对于 Sipeed 提供的版型均在该目录下存放相应的 config.json 供你参考和使用。

## How to use ?

This requires you to upload the config.json file to the flash. You can use MaixPy IDE to send the file to the flash, or use [ampy](https://github.com/scientifichackers/ampy) / [mpfshell-lite](https://github.com/junhuanchen/mpfshell-lite) to upload the file.

> 这需要你上传 config.json 文件到 flash 当中，你可以使用 MaixPy IDE 发送文件到 flash 当中，也可以使用 [ampy](https://github.com/scientifichackers/ampy) / [mpfshell-lite](https://github.com/junhuanchen/mpfshell-lite) 进行文件的上传。

The configuration file config.json has the following templates:

> 这个配置文件 config.json 有如下模板：

```json
{
    "config_name": "template",
    "lcd": {
        "rst" : 37,
        "dcx" : 38,
        "ss" : 36,
        "clk" : 39,
        "height": 240,
        "width": 320,
        "invert": 0,
        "offset_x1": 0,
        "offset_y1": 0,
        "offset_x2": 0,
        "offset_y2": 0,
        "dir": 96
    },
    "freq_cpu": 416000000,
    "freq_pll1": 400000000,
    "kpu_div": 1,
    "sensor": {
        "cmos_pclk":40,
        "cmos_xclk":41,
        "cmos_href":42,
        "cmos_pwdn":43,
        "cmos_vsync":44,
        "cmos_rst":45,
        "reg_width":16,
        "i2c_num":2,
        "pin_clk":46,
        "pin_sda":47
    },
    "sdcard":{
        "sclk":29,
        "mosi":30,
        "miso":31,
        "cs":29,
        "cs_gpio":32
    }
}
```

You can decide whether to fill in the configuration items according to your own hardware IO situation. This is all the parameters supported by the system up to now (September 18, 2020).

> 可以根据自己的硬件 IO 情况来决定是否要填入配置项，这是截至目前系统内部支持的所有参数（2020年9月18日）。

## How to access it in MaixPy

This is actually a Maix.config module, get_value('value_name', default_value).

```shell
>>> import Maix
>>> Maix.config.get_value('kpu_div', None)
1
>>> Maix.config.get_value('sdcard', {})
{'cs': 26, 'mosi': 10, 'sclk': 11, 'miso': 6}
>>> 
```

So did you learn?

## What use ?

The purpose at the beginning of the design is to unify MaixPy's hardware IO resources. The firmware can solve the differences of different hardware through configuration items, and it can also unify the hardware resources of board_info used by the basic sample code.

> 在设计之初的目的就是为了统一 MaixPy 的硬件 IO 资源，固件可以通过配置项来解决不同硬件的差异，还可以统一基础示例代码所使用 board_info 的硬件资源。
