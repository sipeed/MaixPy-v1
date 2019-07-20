class Board_Info:
    def __init__(self):
        self.pin_num = 48
        self.ISP_RX = 4
        self.ISP_TX = 5
        self.WIFI_TX = 39
        self.WIFI_RX = 38
        self.CONNEXT_A=35
        self.CONNEXT_B=34
        self.MPU_SDA=29
        self.MPU_SCL=28
        self.MPU_INT=23
        self.SPK_LRCLK=14
        self.SPK_BCLK=15
        self.SPK_DIN=17
        self.SPK_SD=25
        self.MIC_LRCLK=10
        self.MIC_DAT=12
        self.MIC_CLK=13
        self.LED_W=7
        self.LED_R=6
        self.LED_G=9
        self.LED_B=8
        self.BUTTON_A=36
        self.BUTTON_B=37
        self.pin_name=['ISP_RX','ISP_TX','WIFI_TX','WIFI_RX','CONNEXT_A','CONNEXT_B','MPU_SDA','MPU_SCL','MPU_INT','SPK_LRCLK','SPK_BCLK','SPK_DIN','SPK_SD','MIC_LRCLK','MIC_DAT','MIC_CLK','LED_W','LED_R','LED_G','LED_B','BUTTON_A','BUTTON_B']

    def pin_map(self,Pin = None):
        num_len = 10
        str_len = 23
        if Pin == None :
            num_sum_length = num_len
            str_sum_length = str_len
            Pin_str_obj = "Pin"
            Pin_str_obj_length = len(Pin_str_obj)
            Pin_str_obj_front = 3
            Pin_str_obj_rear = num_sum_length - Pin_str_obj_front - Pin_str_obj_front
            fun_str_obj = "Function"
            fun_str_obj_length = len(fun_str_obj)
            fun_str_obj_front = 5
            fun_str_obj_rear = str_sum_length - fun_str_obj_front - fun_str_obj_length
            print("|%s%s%s|%s%s%s|"%(str(Pin_str_obj_front * '-'),Pin_str_obj,str(Pin_str_obj_rear * '-'),str(fun_str_obj_front * '-'),fun_str_obj,str(fun_str_obj_rear*'-')))
            for i in range(0,len(self.pin_name)):
                num = str(i)
                num_length = len(num)
                num_front = 3
                num_rear = num_sum_length - num_front - num_length
                str_length = len(self.pin_name[i])
                str_front = 5
                str_rear = str_sum_length - str_front - str_length
                print("|%s%d%s|%s%s%s|"%(str(num_front * ' '),i,str(num_rear * ' '),str(str_front * ' '),self.pin_name[i],str(str_rear*' ')))
                print("+%s|%s+"%(str(num_sum_length*'-'),str(str_sum_length*'-')))
        elif isinstance(Pin,int) and Pin < 0 or Pin > 47:
            print("Pin num must in range[0,47]")
            return False
        elif isinstance(Pin,int):
            Pin_sum_length = num_len
            string_sum_length = str_len
            pin_str_obj = "Pin"
            pin_str_obj_length = len(pin_str_obj)
            pin_str_obj_front = 3
            pin_str_obj_rear = Pin_sum_length - pin_str_obj_front - pin_str_obj_front
            Fun_str_obj = "Function"
            Fun_str_obj_length = len(Fun_str_obj)
            Fun_str_obj_front = 5
            Fun_str_obj_rear = string_sum_length - Fun_str_obj_front - Fun_str_obj_length
            print("|%s%s%s|%s%s%s|"%(str(pin_str_obj_front * '-'),pin_str_obj,str(pin_str_obj_rear * '-'),str(Fun_str_obj_front * '-'),Fun_str_obj,str(Fun_str_obj_rear*'-')))
            Pin_str = str(Pin)
            Pin_length = len(Pin_str)
            Pin_front = 3
            Pin_rear = Pin_sum_length - Pin_front - Pin_length
            string_length = len(self.pin_name[Pin])
            string_front = 5
            string_rear = string_sum_length - string_front - string_length
            print("|%s%d%s|%s%s%s|"%(str(Pin_front * ' '),Pin,str(Pin_rear * ' '),str(string_front * ' '),self.pin_name[Pin],str(string_rear*' ')))
            print("+%s|%s+"%(str(Pin_sum_length*'-'),str(string_sum_length*'-')))
        else:
            print("Unknow error")
            return False
global board_info
board_info=Board_Info()
