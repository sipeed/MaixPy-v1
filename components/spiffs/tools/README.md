spiffs tools
========

## Generate spiffs image from files

* Create `fs` dir, and put files in this forlder

* Genrate by execute script `gen_spiffs.sh`

```
chmod +x gen_spiffs.sh
python gen_spiffs_image.py ../../../projects/maixpy_k210/config_defaults.mk
```

the config file from project folder, you can use `config_defaults.mk` if you compiled maixpy with default config, or you must use `build/config/global_config.mk`

* Then the image file(`maixpy_spiffs.img`) will be in `fs_image` folder

* Burn it to flash at correct address with [kflash_gui](https://github.com/sipeed/kflash_gui) or `kflash.py`, this adress is assigned in the config file(`config_defaults.mk` or `build/config/global_config.mk`)( variable `CONFIG_SPIFFS_START_ADDR`)


