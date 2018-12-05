import machine
from fpioa_manager import *
global fm
class PT:
	def __init__(self,P_x = -0.07,P_y = -0.05,x_init_ang = 90,y_init_ang = 90,limited_val = 15,pr_en = 0):
		global fm
		if x_init_ang > 160 or x_init_ang<30:
			print("x_init_ang value error")
			return None
		if y_init_ang > 160 or y_init_ang<30:
			print("y_init_ang value error")
			return None
		if P_x > 0 :
			print("x axis P_value(P_x) must <0")
			return None
		if P_y > 0 :
			print("y axis P_value(P_y) must <0")
			return None
		self.Px = P_x
		self.Py = P_y
		self.x_angle = x_init_ang
		self.y_angle = y_init_ang
		self.print_en = pr_en
		self.limite_value = limited_val
		self.cam=machine.ov2640();
		self.lcd=machine.st7789();
		self.demo=machine.face_detect();
		self.demo.init();
		self.cam.init();
		self.lcd.init();
		fm.registered(34,fm.fpioa.TIMER1_TOGGLE1);
		fm.registered(35,fm.fpioa.TIMER1_TOGGLE2);
		self.y_axis=machine.pwm(machine.pwm.TIMER1,machine.pwm.CHANEEL0,50,7.5,35);
		self.x_axis=machine.pwm(machine.pwm.TIMER1,machine.pwm.CHANEEL1,50,7.5,34);
		self.x_axis.duty(7.5)
		self.y_axis.duty(7.5)
	
	def set_P(self,P_x = 0,P_y = 0):
		if P_y >= 0 :
			print("y axis P_value(P_y) must <0")
			return False
		if P_x >= 0 :
			print("x axis P_value(P_x) must <0")
			return False
		self.Px = P_x
		self.Py = P_y
	def run(self):
		ex = 0.0
		ey = 0.0
		ux_angle = 0.0
		uy_angle = 0.0
		x = 0.0
		y = 0.0
		x1 = 0
		x2 = 0
		y1 = 0
		y2 = 0
		image = bytearray(320*240*2);
		while(1):
			self.cam.get_image(image);
			data = self.demo.process_image(image);
			self.lcd.draw_picture_default(image);
			x1=data[0];
			x2=data[1];
			y1=data[2];
			y2=data[3];
			
			if x1 > 320 or x2 > 320 or y1 > 240 or y2 > 240 :
				continue
			x =(x2 - x1)/2+x1;
			y =(y2 - y1)/2+y1;
			   
			ex = (160 - x);
			ey = (120 - y);
				
			ux_angle = ex  * self.Px
			if abs(ex) < 5 : ux_angle = 0
			if ux_angle >= 15 : ux_angle = 15
			if ux_angle <= -15 : ux_angle = -15
			
			uy_angle = ey  * self.Py	
			if abs(ey) < 5 : ux_angle = 0
			if uy_angle >= self.limite_value : uy_angle = self.limite_value
			if uy_angle <= -self.limite_value : uy_angle = -self.limite_value

			self.x_angle = ux_angle + self.x_angle
			self.y_angle = uy_angle + self.y_angle
		        if self.x_angle * 0.055 >= 9.8 :
                            self.x_angle = 180 
                            ux_angle = 0
                        if self.x_angle * 0.055 <= 0.5 :
                            self.x_angle = 0
                            ux_angle = 0 
			x_angle_duty =  (self.x_angle) * 0.055
			y_angle_duty =  (self.y_angle) * 0.055
		
			if self.print_en == 1 :
				print("x = ",x)
				print("y = ",y)
				print("ux_angle = ",ux_angle)
				print("uy_angle = ",uy_angle)
				print("x_angle = ",self.x_angle)
				print("y_angle = ",self.y_angle)	
			self.x_axis.duty(2.5+x_angle_duty)
			self.y_axis.duty(2.5+y_angle_duty)
