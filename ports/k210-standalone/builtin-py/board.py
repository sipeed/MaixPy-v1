class board_info:
    def __init__(self):
        self.pin_num = 48;
        self.WIFI_TX = 6;
        self.WIFI_RX = 7;
        self.WIFI_EN = 8;
        self.PIN9 = 9;
        self.PIN10 = 10;
        self.PIN11 = 11;
        self.LED_B = 12;
        self.LED_G = 13
        self.LED_R = 14
        self.PIN15 = 15
        self.BOOT_KEY = 16
        self.PIN17 = 17
        self.MIC_ARRAY_BCK = 18
        self.MIC_ARRAY_WS = 19
        self.MIC_ARRAY_DATA3 = 20
        self.MIC_ARRAY_DATA2 = 21
        self.MIC_ARRAY_DATA1 = 22
        self.MIC_ARRAY_DATA0 = 23
        self.MIC_ARRAY_LED = 24
        self.SPI0_CS1 = 25
        self.SPI0_MISO = 26
        self.SPI0_CLK = 27
        self.SPI0_MOSI = 28
        self.SPI0_CS0 = 29
        self.MIC0_WS = 30
        self.MIC0_DATA = 31
        self.MIC0_BCK = 32
        self.I2S_WS = 33
        self.I2S_DA = 34
        self.I2S_BCK = 35
        self.LCD_CS = 36
        self.LCD_RST = 37	
        self.LCD_DC = 38
        self.LCD_WR = 39
        self.DVP_SDA = 40
        self.DVP_SCL = 41
        self.DVP_RST = 42
        self.DVP_VSYNC = 43
        self.DVP_PWDN = 44
        self.DVP_HSYNC = 45
        self.DVP_XCLK = 46
        self.DVP_PCLK = 47
        self.pin_name=['WIFI_TX ','WIFI_RX ','WIFI_EN ','PIN9','PIN10','PIN11','LED_B','LED_G','LED_R','PIN15','BOOT_KEY','PIN17','MIC_ARRAY_BCK','MIC_ARRAY_WS ','MIC_ARRAY_DATA3','MIC_ARRAY_DATA2','MIC_ARRAY_DATA1','MIC_ARRAY_DATA0','MIC_ARRAY_LED','SPI0_CS1','SPI0_MISO','SPI0_CLK ','SPI0_MOSI','SPI0_CS0','MIC0_WS','MIC0_DATA','MIC0_BCK','I2S_WS','I2S_DA','I2S_BCK','LCD_CS','LCD_RST','LCD_DC','LCD_WR ','DVP_SDA','DVP_SCL','DVP_RST','DVP_VSYNC','DVP_PWDN','DVP_HSYNC','DVP_XCLK','DVP_PCLK']
    def pin_map(self,Pin = None):
        if Pin == None:
            print("+---Pin----+----Function----------+\n\
|   6      |     WIFI_TX          |\n\
+——————————|——————————————————————+\n\
|   7      |     WIFI_RX          |\n\
+——————————|——————————————————————+\n\
|   8      |     WIFI_EN          |\n\
+——————————|——————————————————————+\n\
|   9      |     Pin9             |\n\
+——————————|——————————————————————+\n\
|   10     |     Pin10            |\n\
+——————————|——————————————————————+\n\
|   11     |     Pin11            |\n\
+——————————|——————————————————————+\n\
|   12     |     LED_B            |\n\
+——————————|——————————————————————+\n\
|   13     |     LED_G            |\n\
+——————————|——————————————————————+\n\
|   14     |     LED_R            |\n\
+——————————|——————————————————————+\n\
|   15     |     Pin15            |\n\
+——————————|——————————————————————+\n\
|   16     |     BOOT_KEY         |\n\
+——————————|——————————————————————+\n\
|   17     |     Pin17            |\n\
+——————————|——————————————————————+\n\
|   18     |     MIC_ARRAY_BCK    |\n\
+——————————|——————————————————————+\n\
|   19     |     MIC_ARRAY_WS     |\n\
+——————————|——————————————————————+\n\
|   20     |     MIC_ARRAY_DATA3  |\n\
+——————————|——————————————————————+\n\
|   21     |     MIC_ARRAY_DATA2  |\n\
+——————————|——————————————————————+\n\
|   22     |     MIC_ARRAY_DATA1  |\n\
+——————————|——————————————————————+\n\
|   23     |     MIC_ARRAY_DATA0  |\n\
+——————————|——————————————————————+\n\
|   24     |     MIC_ARRAY_LED    |\n\
+——————————|——————————————————————+\n\
|   25     |     SPI0_CS1         |\n\
+——————————|——————————————————————+\n\
|   26     |     SPI0_MISO        |\n\
+——————————|——————————————————————+\n\
|   27     |     SPI0_CLK         |\n\
+——————————|——————————————————————+\n\
|   28     |     SPI0_MOSI        |\n\
+——————————|——————————————————————+\n\
|   29     |     SPI0_CS0         |\n\
+——————————|——————————————————————+\n\
|   30     |     MIC0_WS          |\n\
+——————————|——————————————————————+\n\
|   31     |     MIC0_DATA        |\n\
+——————————|——————————————————————+\n\
|   32     |     MIC0_BCK         |\n\
+——————————|——————————————————————+\n\
|   33     |     I2S_WS           |\n\
+——————————|——————————————————————+\n\
|   34     |     I2S_DA           |\n\
+——————————|——————————————————————+\n\
|   35     |     I2S_BCK          |\n\
+——————————|——————————————————————+\n\
|   36     |     LCD_CS           |\n\
+——————————|——————————————————————+\n\
|   37     |     LCD_RST          |\n\
+——————————|——————————————————————+\n\
|   38     |     LCD_DC           |\n\
+——————————|——————————————————————+\n\
|   39     |     LCD_WR           |\n\
+——————————|——————————————————————+\n\
|   40     |     DVP_SDA          |\n\
+——————————|——————————————————————+\n\
|   41     |     DVP_SCL          |\n\
+——————————|——————————————————————+\n\
|   42     |     DVP_RST          |\n\
+——————————|——————————————————————+\n\
|   43     |     DVP_VSYNC        |\n\
+——————————|——————————————————————+\n\
|   44     |     DVP_PWDN         |\n\
+——————————|——————————————————————+\n\
|   45     |     DVP_HSYNC        |\n\
+——————————|——————————————————————+\n\
|   46     |     DVP_XCLK         |\n\
+——————————|——————————————————————+\n\
|   47     |     DVP_PCLK         |\n\
+——————————|——————————————————————+")
        elif Pin < 6 or Pin > 47:
            print("Pin num must greater than 6 or less than 47")
        else:
            print("Pin",Pin,"-->",self.pin_name[Pin-6])   
