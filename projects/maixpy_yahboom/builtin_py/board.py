try:
    import ujson as json
except ImportError:
    import json

config = json.loads("""
{
    "type": "yahboom",
    "lcd": {
        "dcx": 31,
        "ss": 30,
        "rst": 23,
        "clk": 28,
        "height": 240,
        "width": 320,
        "invert": 1,
        "dir": 96,
        "lcd_type": 0
    },
    "sdcard": {
        "sclk": 32,
        "mosi": 35,
        "miso": 33,
        "cs": 34
    },
    "board_info": {
        "BOOT_KEY": 16,
        "I2C_SDA": 25,
        "I2C_SCL": 24,
        "SPI_SCLK": 32,
        "SPI_MOSI": 35,
        "SPI_MISO": 33,
        "SPI_CS": 34
    },
    "krux": {
        "pins": {
            "BUTTON_B": 16,
            "TOUCH_IRQ": 22,
            "I2C_SDA": 25,
            "I2C_SCL": 24
        },
        "display": {
            "touch": true,
            "font": [8, 16],
            "inverted_coordinates": false,
            "qr_colors": [0, 6342]
        }
    }
}
""")
