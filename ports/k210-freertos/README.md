MAIXPY
======


## Compile

1. Compile MicroPython cross compiler

Build in root path of MaixPy
```
cd MaixPy
make -C mpy-cross
```

2. Get toolchain and Edit `config.conf `, set `toolchain_path`


* Downlaod toolchain from [kendryte](https://kendryte.com/downloads/) (toolchain V8.2)

* Decompress and copy to somewhere `/opt/kendryte-toolchain` for example , then set path

```
cd ports/k210-freertos
echo "toolchain_path=/opt/kendryte-toolchain" > config.conf
```

* Install dependence

```
sudo apt install cmake
```

1. Build

Build port
```
chmod +x build.sh
./build.sh
```

Then we can get bin file(s) in `output` folder

## Burn(/Flash)

1. Get Burn tool 

```
./flash.sh install
```

2. Change default settings

Edit `flash.sh`

```
############### SETTINGS #############
baud=2000000
device=/dev/ttyUSB0
#######################################
```

3. Burn/Flash to board

```
./flash.sh
```

If Flash fail, try again or Decrease baud rate


more parameters use
```
./flash.sh help
```

## For contribute developer

* tag name must start with `v`, e.g. `v1.2.3`, or we can not get version in `MaixPy` by command `sys.implementation.version`



