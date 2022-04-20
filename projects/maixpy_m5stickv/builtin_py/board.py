try:
    import ujson as json
except ImportError:
    import json

config = json.loads("""
{
    "type": "m5stickv",
    "lcd": {
        "height": 135,
        "width": 240,
        "invert": 0,
        "dir": 40,
        "lcd_type": 3
    },
    "sdcard": {
        "sclk": 30,
        "mosi": 33,
        "miso": 31,
        "cs": 32
    },
    "board_info": {
        "ISP_RX": 4,
        "ISP_TX": 5,
        "WIFI_TX": 39,
        "WIFI_RX": 38,
        "CONNEXT_A": 35,
        "CONNEXT_B": 34,
        "MPU_SDA": 29,
        "MPU_SCL": 28,
        "MPU_INT": 23,
        "SPK_LRCLK": 14,
        "SPK_BCLK": 15,
        "SPK_DIN": 17,
        "SPK_SD": 25,
        "MIC_LRCLK": 10,
        "MIC_DAT": 12,
        "MIC_CLK": 13,
        "LED_W": 7,
        "LED_R": 6,
        "LED_G": 9,
        "LED_B": 8,
        "BUTTON_A": 36,
        "BUTTON_B": 37
    },
    "krux": {
        "pins":{
            "BUTTON_A": 36,
            "BUTTON_B": 37,
            "LED_W": 7,
            "UART2_TX": 35,
            "UART2_RX": 34,
            "I2C_SCL": 28,
            "I2C_SDA": 29
        },
        "display": {
            "touch": false,
            "font": [8, 14],
            "orientation":[1, 2],
            "qr_colors":[16904, 61307]
        },
        "sensor": {
            "flipped": false,
            "lenses": false
        }
    }
}
""")
