Docker for MaixPy ( K210 )
=============

## Get docker image

```
docker pull sipeed/k210_build
```

Use daocloud maybe faster if you are in China

```
docker pull daocloud.io/neucrack/k210_build
```

## Get source code

```
git clone https://github.com/sipeed/MaixPy.git
cd MaixPy
git submodule update --recursive --init
```

## Run Docker container

Just for compile:

```
docker run -it --name maixpy -v `pwd`:/maixpy sipeed/k210_build /bin/bash
```

Or you can connect your devices to container:

```
docker run -it --name maixpy --device /dev/ttyUSB0:/dev/ttyUSB0 -v `pwd`:/maixpy sipeed/k210_build /bin/bash
```

## Build firmware

```
cd /maixpy
cd projects/hello_world
python3 project.py distclean # if error occures, just ignore
python3 project.py build
```


## Burn firmware to board

* Use [kflash_gui](https://github.com/sipeed/kflash_gui)

* Or burn in docker container, but you must mount your device to container first,
```
python3 project.py flash -p /dev/ttyUSB0 -B dan -b 1500000 -S
```
more `flash` command for `project.py` see `python3 project.py --help`


