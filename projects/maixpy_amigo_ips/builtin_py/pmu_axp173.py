# This file is part of MaixUI
# Copyright (c) sipeed.com
#
# Licensed under the MIT license:
#   http://www.opensource.org/licenses/mit-license.php
#

# -*- coding: UTF-8 -*-
# Untitled - By: Echo - 周五 4月 2 2020
# start of pmu_axp173.py

from machine import I2C, Timer
import lcd
import time

AXP173_ADDR = 0x34

class AXP173:

    '''
    PMU AXP173
    I2C Addr: 0x34
    '''

    class PMUError(Exception):
        pass

    class OutOfRange(PMUError):
        pass

    _chargingCurrent_100mA = 0
    _chargingCurrent_190mA = 1
    _chargingCurrent_280mA = 2
    _chargingCurrent_360mA = 3
    _chargingCurrent_450mA = 4
    _chargingCurrent_550mA = 5
    _chargingCurrent_630mA = 6
    _chargingCurrent_700mA = 7
    _chargingCurrent_780mA = 8
    _chargingCurrent_880mA = 9
    _chargingCurrent_960mA = 10
    _chargingCurrent_1000mA = 11
    _chargingCurrent_1080mA = 12
    _chargingCurrent_1160mA = 13
    _chargingCurrent_1240mA = 14
    _chargingCurrent_1320mA = 15

    _targevoltage_4100mV = 0
    _targevoltage_4150mV = 1
    _targevoltage_4200mV = 2
    _targevoltage_4360mV = 3

    def __init__(self, i2c_dev=None, i2c_addr=AXP173_ADDR):
        from machine import I2C
        if i2c_dev is None:
            try:
                self.i2cDev = I2C(I2C.I2C1, freq=100*1000, scl=30, sda=31)
                time.sleep(0.5)
            except Exception:
                raise PMUError("Unable to init I2C1 as Master")
        else:
            self.i2cDev = i2c_dev

        self.axp173Addr = i2c_addr
        scan_list = self.i2cDev.scan()
        self.__preButPressed__ = -1
        self.onPressedListener = None
        self.onLongPressedListener = None
        self.system_periodic_task = None

        if self.axp173Addr not in scan_list:
            raise Exception("Error: Unable connect pmu_axp173!")
        # enable timer by default
        # self.enablePMICSleepMode(True)

    def set_on_pressed_listener(self, listener):
        self.onPressedListener = listener

    def set_on_long_pressed_listener(self, listener):
        self.onLongPressedListener = listener

    def set_system_periodic_task(self, task):
        self.system_periodic_task = task

    def __chkPwrKeyWaitForSleep__(self, timer):
        if self.system_periodic_task:
            self.system_periodic_task(self)
        self.i2cDev.writeto(self.axp173Addr, bytes([0x46]))
        pek_stu = (self.i2cDev.readfrom(self.axp173Addr, 1))[0]
        self.i2cDev.writeto_mem(self.axp173Addr, 0x46,
                                0xFF, mem_size=8)  # Clear IRQ

        # Prevent loop in restart, wait for release
        if self.__preButPressed__ == -1 and ((pek_stu & (0x01 << 1)) or (pek_stu & 0x01)):
            print("return")
            return

        if self.__preButPressed__ == -1 and ((pek_stu & (0x01 << 1)) == False and (pek_stu & 0x01) == False):
            self.__preButPressed__ = 0
            #print("self.__preButPressed__ == 0")

        if pek_stu & 0x01:
            print("before enter sleep")
            if self.onLongPressedListener:
                self.onLongPressedListener(self)
            print("after enter sleep is never called")

        if pek_stu & (0x01 << 1):
            if self.onPressedListener:
                self.onPressedListener(self)

    def __write_reg(self, reg_address, value):
        self.i2cDev.writeto_mem(
            self.axp173Addr, reg_address, value, mem_size=8)

    def __read_reg(self, reg_address):
        self.i2cDev.writeto(self.axp173Addr, bytes([reg_address]))
        return (self.i2cDev.readfrom(self.axp173Addr, 1))[0]

    def __is_bit_set(self, byte_data, bit_index):
        return byte_data & (1 << bit_index) != 0

    def enable_adc(self, enable):
        if enable:
            self.__write_reg(0x82, 0xFF)
        else:
            self.__write_reg(0x82, 0x00)

    def enable_coulomb_counter(self, enable):
        if enable:
            self.__write_reg(0xB8, 0x80)
        else:
            self.__write_reg(0xB8, 0x00)

    def stop_coulomb_counter(self):
        self.__write_reg(0xB8, 0xC0)

    def clear_coulomb_counter(self):
        self.__write_reg(0xB8, 0xA0)

    def writeREG(self, regaddr, value):
        self.__write_reg(regaddr, value)

    def readREG(self, regaddr):
        self.__read_reg(regaddr)

    def __get_coulomb_charge_data(self):
        CoulombCounter_LSB = self.__read_reg(0xB0)
        CoulombCounter_B1 = self.__read_reg(0xB1)
        CoulombCounter_B2 = self.__read_reg(0xB2)
        CoulombCounter_MSB = self.__read_reg(0xB3)

        return ((CoulombCounter_LSB << 24) + (CoulombCounter_B1 << 16) +
                (CoulombCounter_B2 << 8) + CoulombCounter_MSB)

    def __get_coulomb_discharge_data(self):
        CoulombCounter_LSB = self.__read_reg(0xB4)
        CoulombCounter_B1 = self.__read_reg(0xB5)
        CoulombCounter_B2 = self.__read_reg(0xB6)
        CoulombCounter_MSB = self.__read_reg(0xB7)

        return ((CoulombCounter_LSB << 24) + (CoulombCounter_B1 << 16) +
                (CoulombCounter_B2 << 8) + CoulombCounter_MSB)

    def get_coulomb_counter_data(self):
        return 65536 * 0.5 * (self.__get_coulomb_charge_data() -
                              self.__get_coulomb_discharge_data) / 3600.0 / 25.0

    def getPowerWorkMode(self):
        mode = self.__read_reg(0x01)
        #print("Work mode: " + hex(mode))
        return mode

    def is_charging(self):
        mode = self.getPowerWorkMode()
        if (self.__is_bit_set(mode, 6)):
            return True
        else:
            return False
        #return True if () else False

    def getVbatVoltage(self):
        Vbat_LSB = self.__read_reg(0x78)
        Vbat_MSB = self.__read_reg(0x79)

        return ((Vbat_LSB << 4) + Vbat_MSB) * 1.1  # AXP173-DS PG26 1.1mV/div

    def is_usb_plugged_in(self):
        power_data = self.__read_reg(0x00)
        return self.__is_bit_set(power_data, 6) and self.__is_bit_set(power_data, 7)

    def getUSBVoltage(self):
        Vin_LSB = self.__read_reg(0x56)
        Vin_MSB = self.__read_reg(0x57)

        return ((Vin_LSB << 4) + Vin_MSB) * 1.7  # AXP173-DS PG26 1.7mV/div

    def getUSBInputCurrent(self):
        Iin_LSB = self.__read_reg(0x58)
        Iin_MSB = self.__read_reg(0x59)

        return ((Iin_LSB << 4) + Iin_MSB) * 0.625  # AXP173-DS PG26 0.625mA/div

    def getConnextVoltage(self):
        Vcnx_LSB = self.__read_reg(0x5A)
        Vcnx_MSB = self.__read_reg(0x5B)

        return ((Vcnx_LSB << 4) + Vcnx_MSB) * 1.7  # AXP173-DS PG26 1.7mV/div

    # VBUS ADC
    def getConnextInputCurrent(self):
        IinCnx_LSB = self.__read_reg(0x5C)
        IinCnx_MSB = self.__read_reg(0x5D)

        # AXP173-DS PG26 0.625mA/div
        current = ((IinCnx_LSB << 4) + IinCnx_MSB) * 0.625
        # print("current data:" + hex(((IinCnx_LSB << 4) + IinCnx_MSB)))
        # print("current:" + str(current))
        return current
        #return ((IinCnx_LSB << 4) + IinCnx_MSB) * 0.625

    def getBatteryChargeCurrent(self):
        Ichg_LSB = self.__read_reg(0x7A)
        Ichg_MSB = self.__read_reg(0x7B)

        return ((Ichg_LSB << 5) + Ichg_MSB) * 0.5  # AXP173-DS PG27 0.5mA/div

    def getBatteryDischargeCurrent(self):
        Idcg_LSB = self.__read_reg(0x7C)
        Idcg_MSB = self.__read_reg(0x7D)

        return ((Idcg_LSB << 5) + Idcg_MSB) * 0.5  # AXP173-DS PG27 0.5mA/div

    def getBatteryInstantWatts(self):
        Iinswat_LSB = self.__read_reg(0x70)
        Iinswat_B2 = self.__read_reg(0x71)
        Iinswat_MSB = self.__read_reg(0x72)

        # AXP173-DS PG32 0.5mA*1.1mV/1000/mW
        return ((Iinswat_LSB << 16) + (Iinswat_B2 << 8) + Iinswat_MSB) * 1.1 * 0.5 / 1000

    def getTemperature(self):
        Temp_LSB = self.__read_reg(0x5E)
        Temp_MSB = self.__read_reg(0x5F)

        # AXP173-DS PG26 0.1degC/div -144.7degC Biased
        return (((Temp_LSB << 4) + Temp_MSB) * 0.1) - 144.7

    def setK210Vcore(self, vol):
        if vol > 1.05 or vol < 0.8:
            raise OutOfRange("Voltage is invaild for K210")
        DCDC2Steps = int((vol - 0.7) * 1000 / 25)
        self.__write_reg(0x23, DCDC2Steps)

    def setScreenBrightness(self, brightness):
        if brightness > 15 or brightness < 0:
            raise OutOfRange(
                "Range for brightness is from 0 to 15, but min 7 is the screen visible value")
        self.__write_reg(0x91, (int(brightness) & 0x0f) << 4)

    def getKeyStatus(self):  # -1: NoPress, 1: ShortPress, 2:LongPress
        but_stu = self.__read_reg(0x46)
        if (but_stu & (0x1 << 1)):
            return 1
        else:
            if (but_stu & (0x1 << 0)):
                return 2
            else:
                return -1

    def exten_output_enable(self, enable=True):
        enten_set = self.__read_reg(0x10)
        if enable == True:
            enten_set = enten_set | 0x04
        else:
            enten_set = enten_set & 0xFC
        return self.__write_reg(0x10, enten_set)

    def setEnterChargingControl(self, enable, volatge=_targevoltage_4200mV, current=_chargingCurrent_190mA):
        if enable == False:
            #1100_1000
            #print("DisenableChargingContorl")
            self.__write_reg(0x33, 0xC8)
        else:
            #print("volatge" + hex((volatge<<4)))
            #print("current" + hex(current))
            power_mode = ((enable << 7) + (volatge << 5) +
                          (current))  # 1100_0001
            # print("setChargingContorl:" + hex(power_mode))
            self.__write_reg(0x33, power_mode)  # Turn off other power source

    def getChargingControl(self):
        return self.__read_reg(0x33)

    def setEnterSleepMode(self):
        self.__write_reg(0x31, 0x0F)  # Enable Sleep Mode
        self.__write_reg(0x91, 0x00)  # Turn off GPIO0/LDO0
        self.__write_reg(0x12, 0x00)  # Turn off other power source

    def enablePMICSleepMode(self, enable):
        if enable:
            # self.__write_reg(0x36, 0x27)  # Turnoff PEK Overtime Shutdown
            self.__write_reg(0x46, 0xFF)  # Clear the interrupts
            self.butChkTimer = Timer(Timer.TIMER2, Timer.CHANNEL0, mode=Timer.MODE_PERIODIC,
                                     period=500, callback=self.__chkPwrKeyWaitForSleep__)
        else:
            #self.__write_reg(0x36, 0x6C)  # Set to default
            try:
                self.butChkTimer.stop()
                del self.butChkTimer
            except Exception:
                pass

# end of pmu_axp173.py


if __name__ == "__main__":
    import time
    # tmp = I2C(I2C.I2C1, freq=100*1000, scl=30, sda=31)
    tmp = I2C(I2C.I2C3, freq=100*1000, scl=24, sda=27)
    axp173 = AXP173(tmp)
    axp173.enable_adc(True)
    # 默认充电限制在 4.2V, 190mA 档位
    axp173.setEnterChargingControl(True)
    axp173.exten_output_enable()

    from ui_canvas import ui

    ui.height, ui.weight = int(lcd.width()), int(lcd.height() / 2)
    @ui.warp_template(ui.blank_draw)
    def test_pmu_axp173_draw():
        global axp173
        tmp = []

        work_mode = axp173.getPowerWorkMode()
        tmp.append("WorkMode:" + hex(work_mode))

        # 检测 电池电压
        vbat_voltage = axp173.getVbatVoltage()
        tmp.append("vbat_voltage: {0} V".format(vbat_voltage))

        # 检测 电池充电电流
        BatteryChargeCurrent = axp173.getBatteryChargeCurrent()
        tmp.append("BatChargeCurrent: {0:>4.1f}mA".format(
            BatteryChargeCurrent))

        # 检测 USB-ACIN 电压
        usb_voltage = axp173.getUSBVoltage()
        tmp.append("usb_voltage: {0:>4}mV".format(usb_voltage))

        # 检测 USB-ACIN 电流
        USBInputCurrent = axp173.getUSBInputCurrent()
        tmp.append("USBInputCurrent: {0:>4.1f}mA".format(USBInputCurrent))

        ### 检测 VBUS 电压
        #usb_voltage = axp173.getConnextVoltage()
        #print("6 VUBS_voltage: " + str(usb_voltage))

        ### 检测 VBUS 电流
        #USBInputCurrent = axp173.getConnextInputCurrent()
        #print("7 VUBSInputCurrent: " + str(USBInputCurrent) + "mA")

        getChargingControl = axp173.getChargingControl()
        tmp.append("ChargingControl: {}".format(hex(getChargingControl)))

        # 检测 是否正在充电
        if axp173.is_charging() == True:
            tmp.append("Charging....")
        else:
            tmp.append("Not Charging")
        tmp.append(axp173.is_charging())

        # 检测 USB 是否连接
        if axp173.is_usb_plugged_in() == 1:
            tmp.append("USB plugged ....")

        else:
            tmp.append("USB is not plugged in")


        for i in range(len(tmp)):
            print(tmp[i])
            ui.canvas.draw_string(
                20, 20 + 20*i, "{0}".format(str(tmp[i])), mono_space=1)

        ui.display()

    import time
    last = time.ticks_ms()
    while True:
        try:
            time.sleep_ms(1000)
            print(time.ticks_ms() - last)
            last = time.ticks_ms()
            test_pmu_axp173_draw()
        except Exception as e:
            # gc.collect()
            print(e)
