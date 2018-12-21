MAIXPY
======


## Compile

1. Edit `config.conf `, set `toolchain_path`

```
echo "toolchain_path=/opt/kendryte-toolchain/bin" > config.conf
```

2. Build

```
./build.sh clean
./build.sh
```

Then we can get bin file(s) in `output` folder

## Burn(/Flash)

1. Get Burn tool 

```
./flash.sh install
```

2. change settings

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


