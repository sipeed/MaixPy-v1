import board
import machine
class fpioa_manager:
    def __init__(self):
        self.board_dict={}
        self.fpioa_dict={}
        self.fpioa = machine.fpioa()
        self.board_info = board.board_info()
    def registered(self,pin,function):
        ret = self.find_dict(pin,function)
        if ret == 1:
            self.board_dict[pin] = function
            self.fpioa_dict[function] = pin
            self.fpioa.set_function(pin,function)
            print("set successed")
            return 1
        else:
            return 0
    def unregistered(self,pin,function):
        ret = self.find_dict(pin,function)
        if ret == 0:
            ret_func=self.board_dict.pop(pin)
            ret_pin=self.fpioa_dict.pop(function)
            return 1
        else:
            return 0
    def find_dict(self,pin,function):
        func_ret = self.__find_board_dict(pin)
        pin_ret = self.__find_fpioa_dict(function)
        if func_ret == None and pin_ret == None:
            return 1
        else:
            return 0
    def __find_board_dict(self,pin):
        function = self.board_dict.get(pin)
        if function == None:
            return None
        else :
            self.fpioa.help(function)
            return self.board_dict[pin]
    def __find_fpioa_dict(self,function):
        pin = self.fpioa_dict.get(function)
        if pin == None:
            return None
        else:
            self.board_info.pin_map(pin)
            return self.fpioa_dict[function]

global fm
fm=fpioa_manager()
