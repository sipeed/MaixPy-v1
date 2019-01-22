#ifndef _NT35310_H_
#define _NT35310_H_

#include <stdint.h>

/* clang-format off */

#define LCD_X_MAX	320
#define LCD_Y_MAX	480

#define BLACK		0x0000
#define NAVY		0x000F
#define DARKGREEN	0x03E0
#define DARKCYAN	0x03EF
#define MAROON		0x7800
#define PURPLE		0x780F
#define OLIVE		0x7BE0
#define LIGHTGREY	0xC618
#define DARKGREY	0x7BEF
#define BLUE		0x001F
#define GREEN		0x07E0
#define CYAN		0x07FF
#define RED		0xF800
#define MAGENTA		0xF81F
#define YELLOW		0xFFE0
#define WHITE		0xFFFF
#define ORANGE		0xFD20
#define GREENYELLOW	0xAFE5
#define PINK		0xF81F
#define USER_COLOR	0xAA55

#define NO_OPERATION		0x00
#define SOFTWARE_RESET		0x01
#define READ_ID			0x04
#define READ_STATUS		0x09
#define READ_POWER_MODE		0x0A
#define READ_MADCTL		0x0B
#define READ_PIXEL_FORMAT	0x0C
#define READ_IMAGE_FORMAT	0x0D
#define READ_SIGNAL_MODE	0x0E
#define READ_SELT_DIAG_RESULT	0x0F
#define SLEEP_ON		0x10
#define SLEEP_OFF		0x11
#define PARTIAL_DISPALY_ON	0x12
#define NORMAL_DISPALY_ON	0x13
#define INVERSION_DISPALY_OFF	0x20
#define INVERSION_DISPALY_ON	0x21
#define GAMMA_SET		0x26
#define DISPALY_OFF		0x28
#define DISPALY_ON		0x29
#define HORIZONTAL_ADDRESS_SET	0x2A
#define VERTICAL_ADDRESS_SET	0x2B
#define MEMORY_WRITE		0x2C
#define COLOR_SET		0x2D
#define MEMORY_READ		0x2E
#define PARTIAL_AREA		0x30
#define VERTICAL_SCROL_DEFINE	0x33
#define TEAR_EFFECT_LINE_OFF	0x34
#define TEAR_EFFECT_LINE_ON	0x35
#define MEMORY_ACCESS_CTL	0x36
#define VERTICAL_SCROL_S_ADD	0x37
#define IDLE_MODE_OFF		0x38
#define IDLE_MODE_ON		0x39
#define PIXEL_FORMAT_SET	0x3A
#define WRITE_MEMORY_CONTINUE	0x3C
#define READ_MEMORY_CONTINUE	0x3E
#define SET_TEAR_SCANLINE	0x44
#define GET_SCANLINE		0x45
#define WRITE_BRIGHTNESS	0x51
#define READ_BRIGHTNESS		0x52
#define WRITE_CTRL_DISPALY	0x53
#define READ_CTRL_DISPALY	0x54
#define WRITE_BRIGHTNESS_CTL	0x55
#define READ_BRIGHTNESS_CTL	0x56
#define WRITE_MIN_BRIGHTNESS	0x5E
#define READ_MIN_BRIGHTNESS	0x5F
#define READ_ID1		0xDA
#define READ_ID2		0xDB
#define READ_ID3		0xDC
#define RGB_IF_SIGNAL_CTL	0xB0
#define NORMAL_FRAME_CTL	0xB1
#define IDLE_FRAME_CTL		0xB2
#define PARTIAL_FRAME_CTL	0xB3
#define INVERSION_CTL		0xB4
#define BLANK_PORCH_CTL		0xB5
#define DISPALY_FUNCTION_CTL	0xB6
#define ENTRY_MODE_SET		0xB7
#define BACKLIGHT_CTL1		0xB8
#define BACKLIGHT_CTL2		0xB9
#define BACKLIGHT_CTL3		0xBA
#define BACKLIGHT_CTL4		0xBB
#define BACKLIGHT_CTL5		0xBC
#define BACKLIGHT_CTL7		0xBE
#define BACKLIGHT_CTL8		0xBF
#define POWER_CTL1		0xC0
#define POWER_CTL2		0xC1
#define VCOM_CTL1		0xC5
#define VCOM_CTL2		0xC7
#define NV_MEMORY_WRITE		0xD0
#define NV_MEMORY_PROTECT_KEY	0xD1
#define NV_MEMORY_STATUS_READ	0xD2
#define READ_ID4		0xD3
#define POSITIVE_GAMMA_CORRECT	0xE0
#define NEGATIVE_GAMMA_CORRECT	0xE1
#define DIGITAL_GAMMA_CTL1	0xE2
#define DIGITAL_GAMMA_CTL2	0xE3
#define INTERFACE_CTL		0xF6
/* clang-format on */

enum lcd_dir_t
{
	DIR_XY_RLUD = 0x00,
	DIR_YX_RLUD = 0x20,
	DIR_XY_LRUD = 0x40,
	DIR_YX_LRUD = 0x60,
	DIR_XY_RLDU = 0x80,
	DIR_YX_RLDU = 0xA0,
	DIR_XY_LRDU = 0xC0,
	DIR_YX_LRDU = 0xE0,
	DIR_XY_MASK = 0x20,
	DIR_MASK = 0xE0,
};

int lcd_busy(void);
void lcd_polling_enable(void);
void lcd_interrupt_enable(void);
void lcd_init(void);
void lcd_clear(uint16_t color);
void lcd_set_direction(enum lcd_dir_t dir);
void lcd_set_area(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color);
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color);
void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr);
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color);
void lcd_draw_rectangle_cpu(uint32_t *ptr, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color, uint16_t bg_color);
void lcd_draw_picture_slow(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr);

void tft_hard_init(void);
void tft_set_datawidth(uint8_t width);
void tft_write_command(uint8_t cmd);
void tft_write_byte_1(uint8_t data);
void tft_write_byte(uint8_t *data_buf, uint32_t length);
void tft_write_half(uint16_t *data_buf, uint32_t length);
void tft_write_word(uint32_t *data_buf, uint32_t length, uint32_t flag);
int tft_busy(void);

#endif
