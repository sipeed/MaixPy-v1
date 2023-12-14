"""This module provides methods to interact with the AXP173 Power Management Unit (PMU)"""

from machine import I2C


AXP173_ADDRESS = 0x34
AXP173_POWER_OUTPUT_CONTROL = 0x12
AXP173_DC_DC3_VOLTAGE_REG = 0x27
AXP173_LDO_2_3_VOLTAGE_REG = 0x28
AXP173_SHUTDOWN_VOLT_SETTINGS = 0x31
AXP173_SHUTDOWN_SETTINGS = 0x32
AXP173_PEK_SETTINGS_REG = 0x36
AXP173_IRQ_ENABLE_3 = 0x42
AXP173_IRQ_STATS_3 = 0x46
AXP173_ADC_REG = 0x82
AXP173_IRQ_CLEAR = 0xFF


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

        # Set proper LDO voltages for camera
        self.__write_register(AXP173_SHUTDOWN_SETTINGS, 0x60)
        self.__write_register(AXP173_LDO_2_3_VOLTAGE_REG, 0x0C)
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

    def get_battery_voltage(self):
        """Returns battery voltage"""

        return self.__get_voltage(0x78, 0x79, 1.1)  # AXP173-DS PG26 1.1mV/div

    def get_usb_voltage(self):
        """Returns USB voltage"""

        return self.__get_voltage(0x56, 0x57, 1.7)  # AXP173-DS PG26 1.7mV/div

    def __get_voltage(self, lsb_reg, msb_reg, divisor):
        lsb = self.__read_register(lsb_reg)
        msb = self.__read_register(msb_reg)
        return ((lsb << 4) + msb) * divisor

    def enter_sleep_mode(self):
        """Set the device to enter sleep mode."""

        self.__write_register(AXP173_LDO_2_3_VOLTAGE_REG, 0xCF)
        self.__write_register(AXP173_IRQ_STATS_3, AXP173_IRQ_CLEAR)  # Clear interrupts
        self.__write_register(AXP173_SHUTDOWN_VOLT_SETTINGS, 0x0F)  # Enable Sleep Mode
        self.__write_register(
            AXP173_POWER_OUTPUT_CONTROL, 0x00
        )  # Turn off other power sources


    # Uncomment code below to use Coulomb counter or other specific features
    
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
