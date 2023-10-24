import gc, lcd, json
try:
    with open('/flash/config.json', "rb") as f:
        cfg = json.loads(f.read())
        if cfg["type"]=="amigo_ips":
            lcd.init()
        del cfg
    gc.collect()
except Exception as e:
    pass
finally:
    gc.collect()