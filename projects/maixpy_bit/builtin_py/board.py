try:
    import ujson as json
except ImportError:
    import json

config = json.loads("""
{
    "type":"bit",
    "lcd": {
        "height": 240,
        "width": 320,
        "invert": 0,
        "lcd_type": 0
    },
    "sdcard": {
        "sclk": 27,
        "mosi": 28,
        "miso": 26,
        "cs": 29
    },
    "board_info": {
      "BOOT_KEY": 16,
      "LED_R": 13,
      "LED_G": 12,
      "LED_B": 14,
      "MIC0_WS": 19,
      "MIC0_DATA": 20,
      "MIC0_BCK": 18
    },
    "krux": {
        "pins":{
            "BUTTON_A": 22,
            "BUTTON_B": 21,
            "BUTTON_C": 16
        },
        "display": {
            "touch": false,
            "font": [8, 16],
            "orientation":[1, 0]
        },
        "sensor": {
            "flipped": true,
            "lenses": true
        }
    }
}
""")
