## MaixPy MaixCube

- require kendryte toolchain

### Configure project

~~~
$ python3 project.py menuconfig
~~~

### Build

~~~
$ python3 project.py build
~~~

### Flash to board

~~~
$ kflash -p /dev/ttyUSB0 -b 1500000 -B goE maixpy.bin
~~~

### Config Cube Devices
~~~
$ curl -LO https://raw.githubusercontent.com/sipeed/MaixPy/f9bb0bb5e807846e6b6e397b9ced2963c5472fab/components/boards/config/cube.config.json
$ ampy -p /dev/ttyUSB0 put ./cube.config.json /flash/config.json
~~~
