'''
MIT License

Copyright (c) 2019 lewis he

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

axp20x.py - MicroPython library for X-Power AXP202 chip.
Created by Lewis he on June 24, 2019.
github:https://github.com/lewisxhe/AXP202X_Libraries
'''


import micropython
from ustruct import unpack
from axp_constants import *
from machine import I2C

default_pin_scl = 22
default_pin_sda = 21
default_pin_intr = 35
default_chip_type = AXP202_CHIP_ID
default_dev_address = AXP202_SLAVE_ADDRESS


class PMU(object):
    def __init__(self, i2c=None, address=None, chip=None):
        self.bus = i2c
        self.chip = chip if chip is not None else default_chip_type
        self.address = address if address else default_dev_address

        self.buffer = bytearray(16)
        self.bytebuf = memoryview(self.buffer[0:1])
        self.wordbuf = memoryview(self.buffer[0:2])
        self.irqbuf = memoryview(self.buffer[0:5])

        self.init_device()

    def init_i2c(self):
        self.bus = I2C(scl=self.pin_scl,
                       sda=self.pin_sda)

    def init_pins(self):
        self.pin_sda = Pin(self.sda)
        self.pin_scl = Pin(self.scl)
        self.pin_intr = Pin(self.intr, mode=Pin.IN)

    def write_byte(self, reg, val):
        self.bus.writeto_mem(self.address, reg, val, mem_size=8)

    def read_byte(self, reg):
        self.bus.writeto(self.address, bytes([reg]))
        return (self.bus.readfrom(self.address, 1))[0]

    def read_word(self, reg):
        self.bus.readfrom_mem_into(self.address, reg, self.wordbuf)
        return unpack('>H', self.wordbuf)[0]

    def read_word2(self, reg):
        self.bus.readfrom_mem_into(self.address, reg, self.wordbuf)
        return unpack('>h', self.wordbuf)[0]

    def init_device(self):
        self.chip = self.read_byte(AXP202_IC_TYPE)
        if(self.chip == AXP202_CHIP_ID):
            pass
        elif(self.chip == AXP192_CHIP_ID):
            raise Exception("No Support AXP192!")
        else:
            raise Exception("Invalid Chip ID!")

    def enablePower(self, ch):
        data = self.read_byte(AXP202_LDO234_DC23_CTL)
        data = data | (1 << ch)
        self.write_byte(AXP202_LDO234_DC23_CTL, data)

    def disablePower(self, ch):
        data = self.read_byte(AXP202_LDO234_DC23_CTL)
        data = data & (~(1 << ch))
        self.write_byte(AXP202_LDO234_DC23_CTL, data)

    def __BIT_MASK(self, mask):
        return 1 << mask

    def __get_h8_l5(self, regh8, regl5):
        hv = self.read_byte(regh8)
        lv = self.read_byte(regl5)
        return (hv << 5) | (lv & 0x1F)

    def __get_h8_l4(self, regh8, regl5):
        hv = self.read_byte(regh8)
        lv = self.read_byte(regl5)
        return (hv << 4) | (lv & 0xF)

    def isChargeing(self):
        data = self.read_byte(AXP202_MODE_CHGSTATUS)
        return data & self.__BIT_MASK(6)

    def isBatteryConnect(self):
        data = self.read_byte(AXP202_MODE_CHGSTATUS)
        return data & self.__BIT_MASK(5)

    def getAcinCurrent(self):
        data = self.__get_h8_l4(AXP202_ACIN_CUR_H8, AXP202_ACIN_CUR_L4)
        return data * AXP202_ACIN_CUR_STEP

    def getAcinVoltage(self):
        data = self.__get_h8_l4(AXP202_ACIN_VOL_H8, AXP202_ACIN_VOL_L4)
        return data * AXP202_ACIN_VOLTAGE_STEP

    def getVbusVoltage(self):
        data = self.__get_h8_l4(AXP202_VBUS_VOL_H8, AXP202_VBUS_VOL_L4)
        return data * AXP202_VBUS_VOLTAGE_STEP

    def getVbusCurrent(self):
        data = self.__get_h8_l4(AXP202_VBUS_CUR_H8, AXP202_VBUS_CUR_L4)
        return data * AXP202_VBUS_CUR_STEP

    def getTemp(self):
        hv = self.read_byte(AXP202_INTERNAL_TEMP_H8)
        lv = self.read_byte(AXP202_INTERNAL_TEMP_L4)
        data = (hv << 8) | (lv & 0xF)
        return data / 1000

    def getTSTemp(self):
        data = self.__get_h8_l4(AXP202_TS_IN_H8, AXP202_TS_IN_L4)
        return data * AXP202_TS_PIN_OUT_STEP

    def getGPIO0Voltage(self):
        data = self.__get_h8_l4(AXP202_GPIO0_VOL_ADC_H8,
                                AXP202_GPIO0_VOL_ADC_L4)
        return data * AXP202_GPIO0_STEP

    def getGPIO1Voltage(self):
        data = self.__get_h8_l4(AXP202_GPIO1_VOL_ADC_H8,
                                AXP202_GPIO1_VOL_ADC_L4)
        return data * AXP202_GPIO1_STEP

    def getBattInpower(self):
        h8 = self.read_byte(AXP202_BAT_POWERH8)
        m8 = self.read_byte(AXP202_BAT_POWERM8)
        l8 = self.read_byte(AXP202_BAT_POWERL8)
        data = (h8 << 16) | (m8 << 8) | l8
        return 2 * data * 1.1 * 0.5 / 1000

    def getBattVoltage(self):
        data = self.__get_h8_l4(AXP202_BAT_AVERVOL_H8, AXP202_BAT_AVERVOL_L4)
        return data * AXP202_BATT_VOLTAGE_STEP

    def getBattChargeCurrent(self):
        data = 0
        if(self.chip == AXP202_CHIP_ID):
            data = self.__get_h8_l4(
                AXP202_BAT_AVERCHGCUR_H8, AXP202_BAT_AVERCHGCUR_L4) * AXP202_BATT_CHARGE_CUR_STEP
        elif (self.chip == AXP192_CHIP_ID):
            data = self.__get_h8_l5(
                AXP202_BAT_AVERCHGCUR_H8, AXP202_BAT_AVERCHGCUR_L4) * AXP202_BATT_CHARGE_CUR_STEP
        return data

    def getBattDischargeCurrent(self):
        data = self.__get_h8_l4(
            AXP202_BAT_AVERDISCHGCUR_H8, AXP202_BAT_AVERDISCHGCUR_L5) * AXP202_BATT_DISCHARGE_CUR_STEP
        return data

    def getSysIPSOUTVoltage(self):
        hv = self.read_byte(AXP202_APS_AVERVOL_H8)
        lv = self.read_byte(AXP202_APS_AVERVOL_L4)
        data = (hv << 4) | (lv & 0xF)
        return data

    def enableADC(self, ch, val):
        if(ch == 1):
            data = self.read_byte(AXP202_ADC_EN1)
            data = data | (1 << val)
            self.write_byte(AXP202_ADC_EN1, data)
        elif(ch == 2):
            data = self.read_byte(AXP202_ADC_EN2)
            data = data | (1 << val)
            self.write_byte(AXP202_ADC_EN1, data)
        else:
            return

    def disableADC(self, ch, val):
        if(ch == 1):
            data = self.read_byte(AXP202_ADC_EN1)
            data = data & (~(1 << val))
            self.write_byte(AXP202_ADC_EN1, data)
        elif(ch == 2):
            data = self.read_byte(AXP202_ADC_EN2)
            data = data & (~(1 << val))
            self.write_byte(AXP202_ADC_EN1, data)
        else:
            return

    def enableIRQ(self, val):
        if(val & 0xFF):
            data = self.read_byte(AXP202_INTEN1)
            data = data | (val & 0xFF)
            self.write_byte(AXP202_INTEN1, data)

        if(val & 0xFF00):
            data = self.read_byte(AXP202_INTEN2)
            data = data | (val >> 8)
            self.write_byte(AXP202_INTEN2, data)

        if(val & 0xFF0000):
            data = self.read_byte(AXP202_INTEN3)
            data = data | (val >> 16)
            self.write_byte(AXP202_INTEN3, data)

        if(val & 0xFF000000):
            data = self.read_byte(AXP202_INTEN4)
            data = data | (val >> 24)
            self.write_byte(AXP202_INTEN4, data)

    def disableIRQ(self, val):
        if(val & 0xFF):
            data = self.read_byte(AXP202_INTEN1)
            data = data & (~(val & 0xFF))
            self.write_byte(AXP202_INTEN1, data)

        if(val & 0xFF00):
            data = self.read_byte(AXP202_INTEN2)
            data = data & (~(val >> 8))
            self.write_byte(AXP202_INTEN2, data)

        if(val & 0xFF0000):
            data = self.read_byte(AXP202_INTEN3)
            data = data & (~(val >> 16))
            self.write_byte(AXP202_INTEN3, data)

        if(val & 0xFF000000):
            data = self.read_byte(AXP202_INTEN4)
            data = data & (~(val >> 24))
            self.write_byte(AXP202_INTEN4, data)
        pass

    def readIRQ(self):
        if(self.chip == AXP202_CHIP_ID):
            for i in range(5):
                self.irqbuf[i] = self.read_byte(AXP202_INTSTS1 + i)
        elif(self.chip == AXP192_CHIP_ID):
            for i in range(4):
                self.irqbuf[i] = self.read_byte(AXP192_INTSTS1 + i)

            self.irqbuf[4] = self.read_byte(AXP192_INTSTS5)

    def clearIRQ(self):
        if(self.chip == AXP202_CHIP_ID):
            for i in range(5):
                self.write_byte(AXP202_INTSTS1 + i, 0xFF)
                self.irqbuf[i] = 0
        elif(self.chip == AXP192_CHIP_ID):
            for i in range(4):
                self.write_byte(AXP192_INTSTS1 + i, 0xFF)
            self.write_byte(AXP192_INTSTS5, 0xFF)

    def isVBUSPlug(self):
        data = self.read_byte(AXP202_STATUS)
        return data & self.__BIT_MASK(5)

    # Only can set axp192
    def setDC1Voltage(self, mv):
        if(self.chip != AXP192_CHIP_ID):
            return
        if(mv < 700):
            mv = 700
        elif(mv > 3500):
            mv = 3500
        val = (mv - 700) / 25
        self.write_byte(AXP192_DC1_VLOTAGE, int(val))

    def setDC2Voltage(self, mv):
        if(mv < 700):
            mv = 700
        elif(mv > 3500):
            mv = 3500
        val = (mv - 700) / 25
        self.write_byte(AXP202_DC2OUT_VOL, int(val))

    def setDC3Voltage(self, mv):
        if(mv < 700):
            mv = 700
        elif(mv > 3500):
            mv = 3500
        val = (mv - 700) / 25
        self.write_byte(AXP202_DC3OUT_VOL, int(val))

    def setLDO2Voltage(self, mv):
        if(mv < 1800):
            mv = 1800
        elif(mv > 3300):
            mv = 3300
        val = (mv - 1800) / 100
        # self.write_byte(AXP202_LDO24OUT_VOL, int(val))
        prev = self.read_byte(AXP202_LDO24OUT_VOL)
        prev &= 0x0F
        prev = prev | (int(val) << 4)
        self.write_byte(AXP202_LDO24OUT_VOL, (prev))

    def setLDO3Voltage(self, mv):
        if(mv < 700):
            mv = 700
        elif(mv > 2275):
            mv = 2275
        val = (mv - 700) / 25
        prev = self.read_byte(AXP202_LDO3OUT_VOL)
        prev &= 0x80
        prev = prev | int(val)
        self.write_byte(AXP202_LDO3OUT_VOL, (prev))
        self.write_byte(AXP202_LDO3OUT_VOL, int(val))

    def setLDO4Voltage(self, arg):
        data = self.read_byte(AXP202_LDO24OUT_VOL)
        data = data & 0xF0
        data = data | arg
        self.write_byte(AXP202_LDO24OUT_VOL, data)

    def setLDO3Mode(self, mode):
        if(mode > AXP202_LDO3_DCIN_MODE):
            return
        data = self.read_byte(AXP202_LDO3OUT_VOL)
        if(mode):
            data = data | self.__BIT_MASK(7)
        else:
            data = data & (~self.__BIT_MASK(7))
        self.write_byte(AXP202_LDO3OUT_VOL, data)

    def setStartupTime(self, val):
        startupParams = (
            0b00000000,
            0b01000000,
            0b10000000,
            0b11000000)
        if(val > AXP202_STARTUP_TIME_2S):
            return
        data = self.read_byte(AXP202_POK_SET)
        data = data & (~startupParams[3])
        data = data | startupParams[val]
        self.write_byte(AXP202_POK_SET, data)

    def setlongPressTime(self, val):
        longPressParams = (
            0b00000000,
            0b00010000,
            0b00100000,
            0b00110000)
        if(val > AXP202_LONGPRESS_TIME_2S5):
            return
        data = self.read_byte(AXP202_POK_SET)
        data = data & (~longPressParams[3])
        data = data | longPressParams[val]
        self.write_byte(AXP202_POK_SET, data)

    def setShutdownTime(self, val):
        shutdownParams = (
            0b00000000,
            0b00000001,
            0b00000010,
            0b00000011)
        if(val > AXP202_SHUTDOWN_TIME_10S):
            return
        data = self.read_byte(AXP202_POK_SET)
        data = data & (~shutdownParams[3])
        data = data | shutdownParams[val]
        self.write_byte(AXP202_POK_SET, data)

    def setTimeOutShutdown(self, en):
        data = self.read_byte(AXP202_POK_SET)
        if(en):
            data = data | self.__BIT_MASK(3)
        else:
            data = data | (~self.__BIT_MASK(3))
        self.write_byte(AXP202_POK_SET, data)

    def shutdown(self):
        data = self.read_byte(AXP202_OFF_CTL)
        data = data | self.__BIT_MASK(7)
        self.write_byte(AXP202_OFF_CTL, data)

    def getSettingChargeCurrent(self):
        data = self.read_byte(AXP202_CHARGE1)
        data = data & 0b00000111
        curr = 300 + data * 100
        return curr

    def isChargeingEnable(self):
        data = self.read_byte(AXP202_CHARGE1)
        if(data & self.__BIT_MASK(7)):
            return True
        return False

    def enableChargeing(self):
        data = self.read_byte(AXP202_CHARGE1)
        data = data | self.__BIT_MASK(7)
        self.write_byte(AXP202_CHARGE1, data)

    def setChargingTargetVoltage(self, val):
        targetVolParams = (
            0b00000000,
            0b00100000,
            0b01000000,
            0b01100000)
        if(val > AXP202_TARGET_VOL_4_36V):
            return
        data = self.read_byte(AXP202_CHARGE1)
        data = data & (~targetVolParams[3])
        data = data | targetVolParams[val]
        self.write_byte(AXP202_CHARGE1, data)

    def getBattPercentage(self):
        data = self.read_byte(AXP202_BATT_PERCENTAGE)
        mask = data & self.__BIT_MASK(7)
        if(mask):
            return 0
        return data & (~self.__BIT_MASK(7))

    def setChgLEDMode(self, mode):
        data = self.read_byte(AXP202_OFF_CTL)
        data |= self.__BIT_MASK(3)
        if(mode == AXP20X_LED_OFF):
            data = data & 0b11001111
        elif(mode == AXP20X_LED_BLINK_1HZ):
            data = data & 0b11001111
            data = data | 0b00010000
        elif(mode == AXP20X_LED_BLINK_4HZ):
            data = data & 0b11001111
            data = data | 0b00100000
        elif(mode == AXP20X_LED_LOW_LEVEL):
            data = data & 0b11001111
            data = data | 0b00110000
        self.write_byte(AXP202_OFF_CTL, data)
