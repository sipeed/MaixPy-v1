from machine import I2C, Timer
import machine

class PMUError(Exception):
    pass
 
class NotFoundError(PMUError):
    pass

class OutOfRange(PMUError):
    pass

def __chkPwrKeyWaitForSleep__(timer):
    global __pmuI2CDEV__, __preButPressed__ #<- Do not do this :(

    __pmuI2CDEV__.writeto(52, bytes([0x46]))
    pek_stu = (__pmuI2CDEV__.readfrom(52, 1))[0]
    __pmuI2CDEV__.writeto_mem(52, 0x46, 0xFF, mem_size=8) #Clear IRQ

    #Prevent loop in restart, wait for release
    if __preButPressed__ == -1 and ((pek_stu & (0x01 << 1)) or (pek_stu & 0x01)):
        return
    
    if __preButPressed__ == -1 and  ((pek_stu & (0x01 << 1)) == False and (pek_stu & 0x01) == False):
        __preButPressed__ = 0

    if (pek_stu & 0x01):
        __pmuI2CDEV__.writeto_mem(52, 0x31, 0x0F, mem_size=8)  #Enable Sleep Mode
        __pmuI2CDEV__.writeto_mem(52, 0x91, 0x00, mem_size=8)  #Turn off GPIO0/LDO0
        __pmuI2CDEV__.writeto_mem(52, 0x12, 0x00, mem_size=8)  #Turn off other power source
    
    if (pek_stu & (0x01 << 1)):
        machine.reset()

class axp192:
    def __init__(self, i2cDev=None):
        if i2cDev == None:
            try:
                self.i2cDev = I2C(I2C.I2C0, freq=400000, scl=28, sda=29)
            except:
                raise PMUError("Unable to init I2C0 as Master")
        else:
            self.i2cDev = i2cDev
        
        self.axp192Addr = 52

        global __pmuI2CDEV__, __preButPressed__
        __pmuI2CDEV__ = self.i2cDev
        __preButPressed__ = -1
        
        scanList = self.i2cDev.scan()
        if self.axp192Addr not in scanList:
            raise NotFoundError
        
    def __writeReg(self, regAddr, value):
        self.i2cDev.writeto_mem(self.axp192Addr, regAddr, value, mem_size=8)

    def __readReg(self, regAddr):
        self.i2cDev.writeto(self.axp192Addr, bytes([regAddr]))
        return (self.i2cDev.readfrom(self.axp192Addr, 1))[0]

    def enableADCs(self, enable):
        if enable == True:
            self.__writeReg(0x82, 0xFF)
        else:
            self.__writeReg(0x82, 0x00)

    def enableCoulombCounter(self, enable):
        if enable == True:
            self.__writeReg(0xB8, 0x80)
        else:
            self.__writeReg(0xB8, 0x00)
    
    def stopCoulombCounter(self):
        self.__writeReg(0xB8, 0xC0)
    
    def clearCoulombCounter(self):
        self.__writeReg(0xB8, 0xA0)
    
    def __getCoulombChargeData(self):
        CoulombCounter_LSB = self.__readReg(0xB0)
        CoulombCounter_B1 = self.__readReg(0xB1)
        CoulombCounter_B2 = self.__readReg(0xB2)
        CoulombCounter_MSB = self.__readReg(0xB3)

        return ((CoulombCounter_LSB << 24) + (CoulombCounter_B1 << 16) + \
            (CoulombCounter_B2 << 8) + CoulombCounter_MSB)
    
    def __getCoulombDischargeData(self):
        CoulombCounter_LSB = self.__readReg(0xB4)
        CoulombCounter_B1 = self.__readReg(0xB5)
        CoulombCounter_B2 = self.__readReg(0xB6)
        CoulombCounter_MSB = self.__readReg(0xB7)

        return ((CoulombCounter_LSB << 24) + (CoulombCounter_B1 << 16) + \
            (CoulombCounter_B2 << 8) + CoulombCounter_MSB)
    
    def getCoulombCounterData(self):
        return 65536 * 0.5 * (self.__getCoulombChargeData() -\
             self.__getCoulombDischargeData) / 3600.0 / 25.0
    
    def getVbatVoltage(self):
        Vbat_LSB = self.__readReg(0x78)
        Vbat_MSB = self.__readReg(0x79)

        return ((Vbat_LSB << 4) + Vbat_MSB) * 1.1 #AXP192-DS PG26 1.1mV/div
        
    def getUSBVoltage(self):
        Vin_LSB = self.__readReg(0x56)
        Vin_MSB = self.__readReg(0x57)

        return ((Vin_LSB << 4) + Vin_MSB) * 1.7 #AXP192-DS PG26 1.7mV/div
    
    def getUSBInputCurrent(self):
        Iin_LSB = self.__readReg(0x58)
        Iin_MSB = self.__readReg(0x59)

        return ((Iin_LSB << 4) + Iin_MSB) * 0.625 #AXP192-DS PG26 0.625mA/div
    
    def getConnextVoltage(self):
        Vcnx_LSB = self.__readReg(0x5A)
        Vcnx_MSB = self.__readReg(0x5B)

        return ((Vcnx_LSB << 4) + Vcnx_MSB) * 1.7 #AXP192-DS PG26 1.7mV/div
    
    def getConnextInputCurrent(self):
        IinCnx_LSB = self.__readReg(0x5C)
        IinCnx_MSB = self.__readReg(0x5D)

        return ((IinCnx_LSB << 4) + IinCnx_MSB) * 0.625 #AXP192-DS PG26 0.625mA/div

    def getBatteryChargeCurrent(self):
        Ichg_LSB = self.__readReg(0x7A)
        Ichg_MSB = self.__readReg(0x7B)

        return ((Ichg_LSB << 5) + Ichg_MSB) * 0.5 #AXP192-DS PG27 0.5mA/div

    def getBatteryDischargeCurrent(self):
        Idcg_LSB = self.__readReg(0x7C)
        Idcg_MSB = self.__readReg(0x7D)

        return ((Idcg_LSB << 5) + Idcg_MSB) * 0.5 #AXP192-DS PG27 0.5mA/div

    def getBatteryInstantWatts(self):
        Iinswat_LSB = self.__readReg(0x70)
        Iinswat_B2 = self.__readReg(0x71)
        Iinswat_MSB = self.__readReg(0x72)

        #AXP192-DS PG32 0.5mA*1.1mV/1000/mW
        return ((Iinswat_LSB << 16) + (Iinswat_B2 << 8) + Iinswat_MSB) * 1.1 * 0.5 / 1000 
 
    def getTemperature(self):
        Temp_LSB = self.__readReg(0x5E)
        Temp_MSB = self.__readReg(0x5F)

        #AXP192-DS PG26 0.1degC/div -144.7degC Biased
        return (((Temp_LSB << 4) + Temp_MSB) * 0.1) - 144.7 
    
    def setK210Vcore(self, vol):
        if vol > 1.05 or vol < 0.8:
            raise OutOfRange("Voltage is invaild for K210")
        DCDC2Steps = int((vol - 0.7) * 1000 / 25)
        self.__writeReg(0x23, DCDC2Steps)
    
    def setScreenBrightness(self, brightness):
        if brightness > 15 or brightness < 0:
            raise OutOfRange("Range for brightness is from 0 to 15")
        self.__writeReg(0x91, (int(brightness) & 0x0f) << 4)

    def getKeyStuatus(self): # -1: NoPress, 1: ShortPress, 2:LongPress
        but_stu = self.__readReg(0x46)
        if (but_stu & (0x1 << 1)):
            return 1
        else:
            if (but_stu & (0x1 << 0)):
                return 2
            else:
                return -1
    
    def setEnterSleepMode(self):
        self.__writeReg(0x31, 0x0F)  #Enable Sleep Mode
        self.__writeReg(0x91, 0x00)  #Turn off GPIO0/LDO0
        self.__writeReg(0x12, 0x00)  #Turn off other power source

    def enablePMICSleepMode(self, enable):
        if enable == True:
            self.__writeReg(0x36, 0x27) #Turnoff PEK Overtime Shutdown
            self.__writeReg(0x46, 0xFF) #Clear the interrupts

            self.butChkTimer = Timer(Timer.TIMER2, Timer.CHANNEL0, mode=Timer.MODE_PERIODIC, period=500, callback=__chkPwrKeyWaitForSleep__)
        else:
            self.__writeReg(0x36, 0x6C) #Set to default
            try:
                self.butChkTimer.stop()
                del self.butChkTimer
            except:
                pass
            
