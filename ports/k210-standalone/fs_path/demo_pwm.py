import machine
import board
board_info=board.board_info()
flag=0
duty = 0
def func(timer):
        global duty
        global flag
        if(flag == 0):
            duty = duty + 1
            if(duty > 100):
                flag = 1
        if(flag == 1):
            duty = duty - 1
            if(duty < 1):
                flag=0
        LED_B.duty(duty)
        LED_G.duty(100-duty)
        LED_R.duty(100-duty/2)

fpioa=machine.fpioa()
fpioa.set_function(board_info.LED_B, fpioa.TIMER1_TOGGLE1)
fpioa.set_function(board_info.LED_G, fpioa.TIMER1_TOGGLE2)
fpioa.set_function(board_info.LED_R, fpioa.TIMER1_TOGGLE3)
LED_B=machine.pwm(1,0,2000000,90,12)
LED_G=machine.pwm(1,1,2000000,90,13)
LED_R=machine.pwm(1,2,2000000,90,14)
timer=machine.timer(0,0)
timer.init(freq=100,period=0,div=0,callback=func)
