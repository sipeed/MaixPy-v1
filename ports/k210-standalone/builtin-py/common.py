import machine
class pin_init:
    fpioa=machine.fpioa()
    def init(self):
		self.fpioa.set_function(47,137)#FUNC_CMOS_PCLK
		self.fpioa.set_function(46,132)#FUNC_CMOS_XCLK
		self.fpioa.set_function(45,136)#FUNC_CMOS_HREF                             
		self.fpioa.set_function(44,134)#FUNC_CMOS_PWDN                             
		self.fpioa.set_function(43,135)#FUNC_CMOS_VSYN             
		self.fpioa.set_function(42,133)#FUNC_CMOS_RST               
		self.fpioa.set_function(41,146)#FUNC_SCCB_SCLK
		self.fpioa.set_function(40,147)#FUNC_SCCB_SDA
		self.fpioa.set_function(37, 25)#FUNC_GPIOHS1 init_rst
		self.fpioa.set_function(38, 26)#FUNC_GPIOHS2 tft_hard_init
		self.fpioa.set_function(36, 15)#FUNC_SPI0_SS3 tft_hard_init
		self.fpioa.set_function(39, 17)#FUNC_SPI0_SCLK tft_hard_init
		self.fpioa.set_function(12, 190)#FUNC_TIMER0_TOGGLE1
		self.fpioa.set_function(13, 191)#FUNC_TIMER0_TOGGLE2
		self.fpioa.set_function(14, 192)#FUNC_TIMER0_TOGGLE3
