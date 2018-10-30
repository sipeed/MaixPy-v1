#include <string.h>
#include "lcd.h"
#include "st7789.h"
#include "font.h"
// #include "common.h"
#include "sleep.h"
#include "gpiohs.h"
#include "fpioa.h"

//LCD的画笔颜色和背景色
uint16_t POINT_COLOR=0x0000;	//画笔颜色
uint16_t BACK_COLOR=0xFFFF;  //背景色

//管理LCD重要参数
//默认为竖屏
_lcd_dev lcddev;

//写寄存器函数
//regval:寄存器值
void LCD_WR_REG(uint8_t regval)
{
    tft_write_command(regval);
}

//写LCD数据
//data:要写入的值
void LCD_WR_DATA(uint8_t data)
{
    tft_write_byte(&data,1);
}

//写寄存器
//LCD_Reg:寄存器地址
//LCD_RegValue:要写入的数据
void LCD_WriteReg(uint8_t LCD_Reg, uint8_t LCD_RegValue)
{
	LCD_WR_REG(LCD_Reg);
	LCD_WR_DATA(LCD_RegValue);
}

//开始写GRAM
void LCD_WriteRAM_Prepare(void)
{
 	LCD_WR_REG(lcddev.wramcmd);
}

//LCD写GRAM
//RGB_Code:颜色值
void LCD_WriteRAM(uint16_t RGB_Code)
{
    uint8_t buf[2]={0};

    buf[0] = RGB_Code >> 8;
    buf[1] = RGB_Code & 0xff;
    tft_write_byte(buf,2);
}

//设置光标位置
//Xpos:横坐标
//Ypos:纵坐标
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{
	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(Xpos>>8);
	LCD_WR_DATA(Xpos&0XFF);

	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(Ypos>>8);
	LCD_WR_DATA(Ypos&0XFF);
}
//设置LCD的自动扫描方向
//注意:其他函数可能会受到此函数设置的影响(尤其是9341/6804这两个奇葩),
//所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
//dir:0~7,代表8个方向(具体定义见lcd.h)
//9320/9325/9328/4531/4535/1505/b505/8989/5408/9341等IC已经实际测试
void LCD_Scan_Dir(enum lcd_dir_t dir)
{
	uint16_t temp;

	LCD_WriteReg(0X36,dir);

	if(dir&0X20)
	{
		if(lcddev.width<lcddev.height)//交换X,Y
		{
			temp=lcddev.width;
			lcddev.width=lcddev.height;
			lcddev.height=temp;
		}
	}
	else
	{
		if(lcddev.width>lcddev.height)//交换X,Y
		{
			temp=lcddev.width;
			lcddev.width=lcddev.height;
			lcddev.height=temp;
		}
	}
	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(0);
    LCD_WR_DATA(0);
	LCD_WR_DATA((lcddev.width-1)>>8);
    LCD_WR_DATA((lcddev.width-1)&0XFF);
	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(0);
    LCD_WR_DATA(0);
	LCD_WR_DATA((lcddev.height-1)>>8);
    LCD_WR_DATA((lcddev.height-1)&0XFF);
}

//画点
//x,y:坐标
//POINT_COLOR:此点的颜色
void LCD_DrawPoint(uint16_t x,uint16_t y)
{
	LCD_SetCursor(x,y);		//设置光标位置
	LCD_WriteRAM_Prepare();	//开始写入GRAM
	LCD_WriteRAM(POINT_COLOR);
}

//设置LCD显示方向（6804不支持横屏显示）
//dir:0,竖屏；1,横屏
void LCD_Display_Dir(uint8_t dir)
{
	if(lcddev.dir==0)//竖屏
	{
		lcddev.dir=0;//竖屏
		lcddev.width=240;
		lcddev.height=320;
		lcddev.wramcmd=0x2C;
		lcddev.setxcmd=0x2a;
		lcddev.setycmd=0x2b;
	}else
	{
		lcddev.dir=1;//横屏
		lcddev.width=320;
		lcddev.height=240;
		lcddev.wramcmd=0x2c;
		lcddev.setxcmd=0x2a;
		lcddev.setycmd=0x2b;
	}
	LCD_Scan_Dir(DIR_YX_LRUD);	//默认扫描方向
}
//初始化lcd
//该初始化函数可以初始化各种ILI93XX液晶,但是其他函数是基于ILI9320的!!!
//在其他型号的驱动芯片上没有测试!
void lcd_init(void)
{
    tft_hard_init();

	//************* Start Initial Sequence **********//
	LCD_WR_REG(0x11);
	msleep(120); //Delay 120ms
	//--------------------------------ST7789S Frame rate setting----------------------------------//
	LCD_WR_REG(0xb2);
	LCD_WR_DATA(0x0c);
	LCD_WR_DATA(0x0c);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x33);
	LCD_WR_DATA(0x33);

	LCD_WR_REG(0xb7);
	LCD_WR_DATA(0x35);

	//---------------------------------ST7789S Power setting--------------------------------------//
	LCD_WR_REG(0xbb);
	LCD_WR_DATA(0x2b);

	LCD_WR_REG(0xc0);
	LCD_WR_DATA(0x2c);

	LCD_WR_REG(0xc2);
	LCD_WR_DATA(0x01);

	LCD_WR_REG(0xc3);
	LCD_WR_DATA(0x17);

	LCD_WR_REG(0xc4);
	LCD_WR_DATA(0x20);

	LCD_WR_REG(0xc6);
	LCD_WR_DATA(0x0f);

	LCD_WR_REG(0xca);
	LCD_WR_DATA(0x0f);

	LCD_WR_REG(0xc8);
	LCD_WR_DATA(0x08);

	LCD_WR_REG(0x55);
	LCD_WR_DATA(0x90);

	LCD_WR_REG(0xd0);
	LCD_WR_DATA(0xa4);
	LCD_WR_DATA(0xa1);

	LCD_WR_REG(0x3A);
	LCD_WR_DATA(0x05);

	LCD_WR_REG(0x36);
	LCD_WR_DATA(0x08);

	//--------------------------------ST7789S gamma setting---------------------------------------//
	LCD_WR_REG(0xe0);
	LCD_WR_DATA(0xF0);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x0A);
	LCD_WR_DATA(0x10);
	LCD_WR_DATA(0x12);

	LCD_WR_DATA(0x1b);
	LCD_WR_DATA(0x39);
	LCD_WR_DATA(0x44);
	LCD_WR_DATA(0x47);
	LCD_WR_DATA(0x28);

	LCD_WR_DATA(0x12);
	LCD_WR_DATA(0x10);
	LCD_WR_DATA(0x16);
	LCD_WR_DATA(0x1b);

	LCD_WR_REG(0xe1);
	LCD_WR_DATA(0xf0);
	LCD_WR_DATA(0x00);
	LCD_WR_DATA(0x0a);
	LCD_WR_DATA(0x10);
	LCD_WR_DATA(0x11);

	LCD_WR_DATA(0x1a);
	LCD_WR_DATA(0x3b);
	LCD_WR_DATA(0x34);
	LCD_WR_DATA(0x4e);
	LCD_WR_DATA(0x3a);

	LCD_WR_DATA(0x17);
	LCD_WR_DATA(0x16);
	LCD_WR_DATA(0x21);
	LCD_WR_DATA(0x22);

	LCD_WR_REG(0x29);
	LCD_WR_REG(0x2c);

	LCD_Display_Dir(1);
	lcd_clear(BLACK);
}

//画线
//x1,y1:起点坐标
//x2,y2:终点坐标
void LCD_DrawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	uint16_t t;
	int xerr=0,yerr=0,delta_x,delta_y,distance;
	int incx,incy,uRow,uCol;
	delta_x=x2-x1; //计算坐标增量
	delta_y=y2-y1;
	uRow=x1;
	uCol=y1;
	if(delta_x>0)incx=1; //设置单步方向
	else if(delta_x==0)incx=0;//垂直线
	else {incx=-1;delta_x=-delta_x;}
	if(delta_y>0)incy=1;
	else if(delta_y==0)incy=0;//水平线
	else{incy=-1;delta_y=-delta_y;}
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
	else distance=delta_y;
	for(t=0;t<=distance+1;t++ )//画线输出
	{
		LCD_DrawPoint(uRow,uCol);//画点
		xerr+=delta_x ;
		yerr+=delta_y ;
		if(xerr>distance)
		{
			xerr-=distance;
			uRow+=incx;
		}
		if(yerr>distance)
		{
			yerr-=distance;
			uCol+=incy;
		}
	}
}

//画矩形
//(x1,y1),(x2,y2):矩形的对角坐标
void LCD_DrawRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	LCD_DrawLine(x1,y1,x2,y1+5);
	LCD_DrawLine(x1,y1,x1,y2+5);
	LCD_DrawLine(x1,y2,x2,y2+5);
	LCD_DrawLine(x2,y1,x2,y2+5);
}

//设置窗口
void LCD_Set_Window(uint16_t sx,uint16_t sy,uint16_t width,uint16_t height)
{
	width=sx+width-1;
	height=sy+height-1;

	LCD_WR_REG(lcddev.setxcmd);
	LCD_WR_DATA(sx>>8);
	LCD_WR_DATA(sx&0XFF);
	LCD_WR_DATA(width>>8);
	LCD_WR_DATA(width&0XFF);

	LCD_WR_REG(lcddev.setycmd);
	LCD_WR_DATA(sy>>8);
	LCD_WR_DATA(sy&0XFF);
	LCD_WR_DATA(height>>8);
	LCD_WR_DATA(height&0XFF);
	LCD_WR_REG(lcddev.wramcmd);
}

void lcd_draw_char(uint16_t x, uint16_t y, char c, uint16_t color)
{
	uint8_t i, j, data;
	uint16_t tmp;

	tmp = POINT_COLOR;
	POINT_COLOR = color;

	for (i = 0; i < 16; i++) {
		data = ascii0816[c * 16 + i];
		for (j = 0; j < 8; j++) {
			if (data & 0x80)
				LCD_DrawPoint(x + j, y);
			data <<= 1;
		}
		y++;
	}
	POINT_COLOR = tmp;
}

void lcd_draw_string(uint16_t x, uint16_t y, char *str, uint16_t color)
{
	while (*str) {
		lcd_draw_char(x, y, *str, color);
		str++;
		x += 8;
	}
}

void ram_draw_string(char *str, uint32_t *ptr, uint16_t font_color, uint16_t bg_color)
{
	uint8_t i, j, data, *pdata;
	uint16_t width;
	uint32_t *pixel;

	width = 4 * strlen(str);
	while (*str) {
		pdata = (uint8_t *)&ascii0816[(*str) * 16];
		for (i = 0; i < 16; i++) {
			data = *pdata++;
			pixel = ptr + i * width;
			for (j = 0; j < 4; j++) {
				switch (data >> 6) {
				case 0:
					*pixel = ((uint32_t)bg_color << 16) | bg_color; break;
				case 1:
					*pixel = ((uint32_t)bg_color << 16) | font_color; break;
				case 2:
					*pixel = ((uint32_t)font_color << 16) | bg_color; break;
				case 3:
					*pixel = ((uint32_t)font_color << 16) | font_color; break;
				default:
					*pixel = 0; break;
				}
				data <<= 2;
				pixel++;
			}
		}
		str++;
		ptr += 4;
	}
}

void lcd_draw_rectangle_cpu(uint32_t *ptr, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
	uint32_t *addr1, *addr2;
	uint16_t i;
	uint32_t data = ((uint32_t)color << 16) | (uint32_t)color;

	if (x1 == 319)
		x1 = 318;
	if (x2 == 0)
		x2 = 1;
	if (y1 == 239)
		y1 = 238;
	if (y2 == 0)
		y2 = 1;

	addr1 = ptr + (320 * y1 + x1) / 2;
	addr2 = ptr + (320 * (y2 - 1) + x1) / 2;
	for (i = x1; i < x2; i += 2) {
		*addr1 = data;
		*(addr1 + 160) = data;
		*addr2 = data;
		*(addr2 + 160) = data;
		addr1++;
		addr2++;
	}
	addr1 = ptr + (320 * y1 + x1) / 2;
	addr2 = ptr + (320 * y1 + x2 - 1) / 2;
	for (i = y1; i < y2; i++) {
		*addr1 = data;
		*addr2 = data;
		addr1 += 160;
		addr2 += 160;
	}
}

void lcd_draw_rectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t width, uint16_t color)
{
	uint16_t colortemp=POINT_COLOR;

	POINT_COLOR = RED;
	LCD_DrawRectangle(x1,y1,x2,y2);
	POINT_COLOR = colortemp;
}

void lcd_draw_picture(uint16_t x1, uint16_t y1, uint16_t width, uint16_t height, uint32_t *ptr)
{
#if 1
	LCD_Set_Window(x1, y1, x1 + width, y1 + height);
    LCD_SetCursor(0x00,0x0000);	//设置光标位置
	LCD_WriteRAM_Prepare();     //开始写入GRAM
	tft_set_datawidth(32);
	tft_write_word(ptr, width * height / 2, 0);
	tft_set_datawidth(8);
#else
	uint32_t index;
	uint8_t dat[4];

	LCD_Set_Window(x1, y1, x1 + width, y1 + height);
    LCD_SetCursor(0x00,0x0000);	//设置光标位置
	LCD_WriteRAM_Prepare();     //开始写入GRAM

	for(index=0;index<width*height/2;index++)
	{
		dat[0]=*ptr>>24&0xff;
		dat[1]=*ptr>>16&0xff;
		dat[2]=*ptr>>8&0xff;
		dat[3]=*ptr&0xff;
		tft_write_byte(&dat,4);
		ptr++;
	}
	// msleep(10);
#endif
}

//清屏函数
//color:要清屏的填充色
void lcd_clear(uint16_t color)
{
    uint8_t data[2]={0};
	uint32_t index=0;
	uint32_t totalpoint=lcddev.width;

	totalpoint*=lcddev.height; 	//得到总点数
    LCD_SetCursor(0x00,0x0000);	//设置光标位置
	LCD_WriteRAM_Prepare();     //开始写入GRAM

    data[0] = color >> 8;
    data[1] = color & 0xff;

	for(index=0;index<totalpoint;index++)
	{
        tft_write_byte(&data,2);
	}
}