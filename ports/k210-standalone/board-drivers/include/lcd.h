#ifndef _LCD_H_
#define _LCD_H_

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
void lcd_draw_point(uint16_t x, uint16_t y, uint16_t color);
void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color);
void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr);
void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color);
void lcd_draw_rectangle_cpu(uint32_t *ptr, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void lcd_ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color, uint16_t bg_color);
void lcd_draw_picture_slow(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr);

#endif
