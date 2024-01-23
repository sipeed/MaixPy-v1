"""This module provides methods to interact with the AXP173 Power Management Unit (PMU)"""

from machine import I2C

# registers
AXP173_POWER_STATUS = 0x00
AXP173_CHARGE_STATUS = 0x01
# AXP173_OTG_VBUS_STATUS = 0x04

AXP173_EXTEN_DC2_SW = 0x10  # EXTEN & DC-DC2 switch control register
AXP173_DC1_LDO234_SW = 0x12  # DC-DC1/LDO4 & LDO2/3 switch control register

# Output voltage settings
AXP173_DC2_VOLT = 0x23  # DC-DC2 output voltage setting
AXP173_DC2_VOLT_SLOPE = 0x25  # DC-DC2 dynamic voltage adjustment parameter setting

AXP173_DC1_VOLT = 0x26  # DC-DC1 output voltage setting
AXP173_LDO4_VOLT = 0x27  # LDO4 output voltage setting
AXP173_LDO23_VOLT = 0x28  # LDO2/3 output voltage setting

AXP173_VBUS_TO_IPSOUT = 0x30  # VBUS-IPSOUT path setting register
AXP173_SHUTDOWN_VOLT = 0x31  # VOFF shutdown voltage setting register
AXP173_SHUTDOWN_BAT_CHGLED = 0x32  # Shutdown, battery detection, CHGLED control register

AXP173_CHARGE_CONTROL_1 = 0x33  # Charging control register 1
AXP173_CHARGE_CONTROL_2 = 0x34  # Charging control register 2

AXP173_PEK = 0x36  # PEK parameter setting register
# AXP173_DCDC_FREQUENCY = 0x37  # DCDC converter operating frequency setting register
# AXP173_BAT_CHG_L_TEMP = 0x38  # Battery charging low temperature alarm setting register
# AXP173_BAT_CHG_H_TEMP = 0x39  # Battery charging high temperature alarm setting register

# AXP173_APS_LOW_POWER1 = 0x3a  # APS low power Level1 setting register
# AXP173_APS_LOW_POWER2 = 0x3b  # APS low power Level2 setting register
# AXP173_BAT_DISCHG_L_TEMP = 0x3c  # Battery discharge low temperature alarm setting register
# AXP173_BAT_DISCHG_H_TEMP = 0x3d  # Battery discharge high temperature alarm setting register

AXP173_DCDC_MODE = 0x80  # DCDC operating mode setting register
AXP173_ADC_ENABLE_1 = 0x82  # ADC enable setting register 1
AXP173_ADC_ENABLE_2 = 0x83  # ADC enable setting register 2
AXP173_ADC_RATE_TS_PIN = 0x84  # ADC sampling rate setting, TS pin control register

# AXP173_TIMER_CONTROL = 0x8a  # Timer control register
# AXP173_VBUS_MONITOR = 0x8b  # VBUS monitoring setting register
# AXP173_TEMP_SHUTDOWN_CONTROL = 0x8f  # Over-temperature shutdown control register

# Interrupt control registers
AXP173_IRQ_EN_CONTROL_1 = 0x40  # IRQ enable control register 1
AXP173_IRQ_EN_CONTROL_2 = 0x41  # IRQ enable control register 2
AXP173_IRQ_EN_CONTROL_3 = 0x42  # IRQ enable control register 3
AXP173_IRQ_EN_CONTROL_4 = 0x43  # IRQ enable control register 4

AXP173_IRQ_STATUS_1 = 0x44  # IRQ status register 1
AXP173_IRQ_STATUS_2 = 0x45  # IRQ status register 2
AXP173_IRQ_STATUS_3 = 0x46  # IRQ status register 3
AXP173_IRQ_STATUS_4 = 0x47  # IRQ status register 4

# ADC data registers
AXP173_ACIN_VOLTAGE = 0x56  # ACIN voltage ADC data high 8 bits, low 4 bits at (0x57)
AXP173_ACIN_CURRENT = 0x58  # ACIN current ADC data high 8 bits, low 4 bits at (0x59)
AXP173_VBUS_VOLTAGE = 0x5a  # VBUS voltage ADC data high 8 bits, low 4 bits at (0x5b)
AXP173_VBUS_CURRENT = 0x5c  # VBUS current ADC data high 8 bits, low 4 bits at (0x5d)

# Temperature related
AXP173_TEMP = 0x5e  # AXP173 internal temperature monitoring ADC data high 8 bits, low 4 bits at (0x5f)
AXP173_TS_INPUT = 0x62  # TS input ADC data high 8 bits, default monitors battery temperature, low 4 bits at (0x63)

AXP173_BAT_POWER = 0x70  # Battery instantaneous power high 8 bits, middle 8 bits (0x71), high 8 bits (0x72)
AXP173_BAT_VOLTAGE = 0x78  # Battery voltage high 8 bits, low 4 bits (0x79)
AXP173_CHARGE_CURRENT = 0x7a  # Battery charging current high 8 bits, low 5 bits (0x7b)
AXP173_DISCHARGE_CURRENT = 0x7c  # Battery discharging current high 8 bits, low 5 bits (0x7d)
AXP173_APS_VOLTAGE = 0x7e  # APS voltage high 8 bits, low 4 bits (0x7f)
AXP173_CHARGE_COULOMB = 0xb0  # Battery charge coulomb counter register 3,2(0xb1),1(0xb2),0(0xb3)
AXP173_DISCHARGE_COULOMB = 0xb4  # Battery discharge coulomb counter register 3,2(0xb5),1(0xb6),0(0xb7)
AXP173_COULOMB_COUNTER_CONTROL = 0xb8  # Coulomb counter control register

# Computed ADC
AXP173_COULOMB_COUNTER = 0xff


AXP173_ADDRESS = 0x34
AXP173_POWER_OUTPUT_CONTROL = 0x12
AXP173_DC_DC3_VOLTAGE_REG = 0x27
AXP173_LDO_2_3_VOLTAGE_REG = 0x28
AXP173_SHUTDOWN_VOLT_SETTINGS = 0x31
AXP173_SHUTDOWN_SETTINGS = 0x32
AXP173_CHARGE_CONTROL_1 = 0x33
AXP173_PEK_SETTINGS_REG = 0x36
AXP173_IRQ_ENABLE_3 = 0x42
AXP173_IRQ_STATS_3 = 0x46
AXP173_ADC_REG = 0x82
AXP173_IRQ_CLEAR = 0xFF

TARGET_VOLTAGE_42 = 2 # 4.2V
CHARGING_CURRENT_190 = 1 # 190mA


class PMUError(Exception):
    """PMU Error"""


class NotFoundError(PMUError):
    """PMU Not Found"""


class OutOfRange(PMUError):
    """Out of Range"""


class PMUController:
    """Control for AXP173 PMU."""

    def __init__(self, i2c_device=None):
        if i2c_device is None:
            try:
                self.i2c_device = I2C(I2C.I2C0, freq=400000, scl=24, sda=27)
            except:
                raise PMUError("Unable to initialize I2C0 as Master")
        else:
            self.i2c_device = i2c_device

        power_mode = 1 << 7  # enable
        power_mode += TARGET_VOLTAGE_42 << 5  # 4.2V
        power_mode += CHARGING_CURRENT_190 << 0  # 190mA
        self.__write_register(AXP173_CHARGE_CONTROL_1, power_mode)

        # Set DC-DC1 to 3.3V
        self.__write_register(AXP173_DC1_VOLT, 0x68)
        # Set DC-DC2 to defaults - But it is not used
        self.__write_register(AXP173_DC2_VOLT, 0x16)
        self.__write_register(AXP173_DC2_VOLT_SLOPE, 0x00)
        # Set LDO2 and 3
        self.__write_register(AXP173_LDO_2_3_VOLTAGE_REG, 0x0C)
        # Set LDO4 to defaults: 2.5V
        self.__write_register(AXP173_LDO4_VOLT, 0x48)
        # Output power control register - Enable DC-DC1, LDO4, LDO3, EXTEN, 
        self.__write_register(AXP173_POWER_OUTPUT_CONTROL, 0b01001011)
        # Set VBUS-IPSOUT default path
        self.__write_register(AXP173_VBUS_TO_IPSOUT,0x60)

        self.__write_register(AXP173_SHUTDOWN_SETTINGS, 0x40)
        self.__write_register(AXP173_PEK_SETTINGS_REG, 0x5D)

        device_list = self.i2c_device.scan()
        if AXP173_ADDRESS not in device_list:
            raise NotFoundError("PMU not found")

    def __write_register(self, register_address, value):
        self.i2c_device.writeto_mem(AXP173_ADDRESS, register_address, value)

    def __read_register(self, register_address):
        self.i2c_device.writeto(AXP173_ADDRESS, bytes([register_address]))
        return (self.i2c_device.readfrom(AXP173_ADDRESS, 1))[0]

    def enable_adcs(self, enable):
        """Enable or disable ADCs."""

        if enable:
            self.__write_register(AXP173_ADC_REG, 0xFF)
        else:
            self.__write_register(AXP173_ADC_REG, 0x00)

    def usb_connected(self):
        """Returns True if AC IN available, False otherwise"""
        return True if self.__read_register(AXP173_POWER_STATUS) & 0x40 else False
    
    def charging(self):
        """Returns True if charging, False otherwise"""
        return True if self.__read_register(AXP173_CHARGE_STATUS) & 0x40 else False
    
    def get_battery_voltage(self):
        """Returns battery voltage"""

        return self.__get_voltage(0x78, 0x79, 1.1)  # AXP173-DS PG26 1.1mV/div

    def __get_voltage(self, msb_reg, lsb_reg, divisor):
        msb = self.__read_register(msb_reg)
        lsb = self.__read_register(lsb_reg)
        return ((msb << 4) + lsb) * divisor

    def enter_sleep_mode(self):
        """Actually it won't sleep, it will shutdown instead."""
        self.__write_register(AXP173_SHUTDOWN_SETTINGS, 0xC0) # Shuts down all outputs


    # Uncomment code below to use Coulomb counter or other specific features
        

    # def get_usb_voltage(self):
    #     """Returns USB voltage"""
    #     return self.__get_voltage(0x56, 0x57, 1.7)  # AXP173-DS PG26 1.7mV/div
    
    # def enable_coulomb_counter(self, enable):
    #     """Enable or disable the Coulomb counter."""
    #     self.__write_register(0xB8, 0x80 if enable else 0x00)

    # def stop_coulomb_counter(self):
    #     """Stop the Coulomb counter."""
    #     self.__write_register(0xB8, 0xC0)

    # def clear_coulomb_counter(self):
    #     """Clear the Coulomb counter."""
    #     self.__write_register(0xB8, 0xA0)

    # def __get_coulomb_charge_data(self):
    #     return self.__assemble_coulomb_data(0xB0, 0xB1, 0xB2, 0xB3)

    # def __get_coulomb_discharge_data(self):
    #     return self.__assemble_coulomb_data(0xB4, 0xB5, 0xB6, 0xB7)

    # def __assemble_coulomb_data(self, lsb_reg, b1_reg, b2_reg, msb_reg):
    #     lsb = self.__read_register(lsb_reg)
    #     b1 = self.__read_register(b1_reg)
    #     b2 = self.__read_register(b2_reg)
    #     msb = self.__read_register(msb_reg)
    #     return (lsb << 24) + (b1 << 16) + (b2 << 8) + msb

    # def get_coulomb_counter_data(self):
    #     charge_data = self.__get_coulomb_charge_data()
    #     discharge_data = self.__get_coulomb_discharge_data()
    #     return 65536 * 0.5 * (charge_data - discharge_data) / 3600.0 / 25.0

    # def set_k210_vcore(self, voltage):
    #     """Set the voltage for K210's core."""

    #     if voltage > 1.05 or voltage < 0.8:
    #         raise OutOfRange("Voltage is invalid for K210")
    #     dcdc2_steps = int((voltage - 0.7) * 1000 / 25)
    #     self.__write_register(0x23, dcdc2_steps)

    # def get_key_status(self):  # -1: NoPress, 1: ShortPress, 2:LongPress
    #     """Get the status of the power key."""

    #     button_status = self.__read_register(AXP173_IRQ_STATS_3)
    #     if button_status & (0x1 << 1):
    #         return 1
    #     if button_status & (0x1 << 0):
    #         return 2
    #     return -1
