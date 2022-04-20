try:
    import ujson as json
except ImportError:
    import json

config = json.loads("""
{
    "type": "twatch",
    "lcd": {
        "height": 240,
        "width": 240,
        "invert": 1,
        "dir": 96,
        "lcd_type": 0
    },
    "board_info": {
        "BOOT_KEY": 16,
        "LED_R": 12,
        "LED_G": 13,
        "LED_B": 14,
        "WIFI_TX": 6,
        "WIFI_RX": 7,
        "WIFI_EN": 8,
        "I2S0_MCLK": 13,
        "I2S0_SCLK": 21,
        "I2S0_WS": 18,
        "I2S0_IN_D0": 35,
        "I2S0_OUT_D2": 34,
        "SPI0_MISO": 26,
        "SPI0_CLK": 27,
        "SPI0_MOSI": 28,
        "SPI0_CS0": 29,
        "MIC0_WS": 30,
        "MIC0_DATA": 31,
        "MIC0_BCK": 32,
        "I2S_WS": 33,
        "I2S_DA": 34,
        "I2S_BCK": 35
    },
    "krux": {
        "pins":{},
        "display": {},
        "sensor": {}
    }
}
""")
