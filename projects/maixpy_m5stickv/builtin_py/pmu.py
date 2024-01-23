"""This module provides methods to interact with the AXP192 Power Management Unit (PMU)"""

from machine import I2C, Timer

AXP192_POWER_STATUS = 0x00
AXP192_CHARGE_STATUS = 0x01
AXP192_ADDRESS = 0x34
AXP192_POWER_OUTPUT_CONTROL = 0x12
AXP192_SHUTDOWN_VOLT_SETTINGS = 0x31
AXP192_SHUTDOWN_SETTINGS = 0x32
AXP192_PEK_SETTINGS_REG = 0x36
AXP192_IRQ_STATS_3 = 0x46
AXP192_ADC_REG = 0x82
AXP192_GPIO_0_VOLTAGE_SETTING = 0x91

PRESSED = 0
RELEASED = 1


class PMUError(Exception):
    """Base class for PMU errors."""


class NotFoundError(PMUError):
    """Raised when a device is not found."""


class OutOfRange(PMUError):
    """Raised when a value is out of range."""

__pmu_i2c_dev__ = None
__pre_but_pressed__ = -1

def __ckeck_power_key__(timer):
    global __pre_but_pressed__  # <- Do not do this :(

    __pmu_i2c_dev__.writeto(AXP192_ADDRESS, bytes([AXP192_IRQ_STATS_3]))
    pek_stu = (__pmu_i2c_dev__.readfrom(AXP192_ADDRESS, 1))[0]
    __pmu_i2c_dev__.writeto_mem(
        AXP192_ADDRESS, AXP192_IRQ_STATS_3, 0xFF, mem_size=8
    )  # Clear IRQ

    # Prevent loop in restart, wait for release
    if __pre_but_pressed__ == -1 and ((pek_stu & (0x01 << 1)) or (pek_stu & 0x01)):
        return

    if __pre_but_pressed__ == -1 and (
        (pek_stu & (0x01 << 1)) is False and (pek_stu & 0x01) is False
    ):
        __pre_but_pressed__ = RELEASED

    if pek_stu & 0x01:
        __pmu_i2c_dev__.writeto_mem(
            AXP192_ADDRESS, AXP192_SHUTDOWN_VOLT_SETTINGS, 0x0F, mem_size=8
        )  # Enable Sleep Mode
        __pmu_i2c_dev__.writeto_mem(
            AXP192_ADDRESS, AXP192_GPIO_0_VOLTAGE_SETTING, 0x00, mem_size=8
        )  # Turn off GPIO0/LDO0
        __pmu_i2c_dev__.writeto_mem(
            AXP192_ADDRESS, AXP192_POWER_OUTPUT_CONTROL, 0x00, mem_size=8
        )  # Turn off other power source

    if pek_stu & (0x01 << 1):
        __pre_but_pressed__ = PRESSED
    else:
        __pre_but_pressed__ = RELEASED


class PMUController:
    """Control for AXP192 PMU."""

    def __init__(self, i2c_device=None):
        self.button_check_timer = None
        if i2c_device is None:
            try:
                self.i2c_device = I2C(I2C.I2C0, freq=400000, scl=24, sda=27)
            except:
                raise PMUError("Unable to initialize I2C0 as Master")
        else:
            self.i2c_device = i2c_device

        global __pmu_i2c_dev__, __pre_but_pressed__
        __pmu_i2c_dev__ = self.i2c_device
        __pre_but_pressed__ = -1

        device_list = self.i2c_device.scan()
        if AXP192_ADDRESS not in device_list:
            raise NotFoundError("PMU not found")

    def __write_register(self, register_address, value):
        self.i2c_device.writeto_mem(AXP192_ADDRESS, register_address, value)

    def __read_register(self, register_address):
        self.i2c_device.writeto(AXP192_ADDRESS, bytes([register_address]))
        return (self.i2c_device.readfrom(AXP192_ADDRESS, 1))[0]

    def enable_adcs(self, enable):
        """Enable or disable ADCs."""

        if enable:
            self.__write_register(AXP192_ADC_REG, 0xFF)
        else:
            self.__write_register(AXP192_ADC_REG, 0x00)

    def usb_connected(self):
        """Returns True if AC IN available, False otherwise"""
        return True if self.__read_register(AXP192_POWER_STATUS) & 0x40 else False
    
    def charging(self):
        """Returns True if charging, False otherwise"""
        return True if self.__read_register(AXP192_CHARGE_STATUS) & 0x40 else False

    def get_battery_voltage(self):
        """Returns battery voltage"""

        return self.__get_voltage(0x78, 0x79, 1.1)  # AXP173-DS PG26 1.1mV/div

    def __get_voltage(self, msb_reg, lsb_reg, divisor):
        msb = self.__read_register(msb_reg)
        lsb = self.__read_register(lsb_reg)
        return ((msb << 4) + lsb) * divisor

    def set_screen_brightness(self, brightness):
        """Sets the screen brightness by modifying the backlight voltage"""
        if brightness > 15 or brightness < 0:
            raise OutOfRange("Range for brightness is from 0 to 15")
        self.__write_register(
            AXP192_GPIO_0_VOLTAGE_SETTING, (int(brightness) & 0x0F) << 4
        )

    def enter_sleep_mode(self):
        """Set the device to enter sleep mode."""

        self.__write_register(AXP192_PEK_SETTINGS_REG, 0x6C)  # Set to default
        try:
            self.button_check_timer.stop()
            del self.button_check_timer
        except:
            pass
        self.__write_register(AXP192_SHUTDOWN_VOLT_SETTINGS, 0x0F)  # Enable Sleep Mode
        self.__write_register(
            AXP192_GPIO_0_VOLTAGE_SETTING, 0x00
        )  # Turn off GPIO0/LDO0
        self.__write_register(
            AXP192_POWER_OUTPUT_CONTROL, 0x00
        )  # Turn off other power source

    def enable_pek_button_monitor(self):
        """Enable button polling through a timed task"""
        self.__write_register(
            AXP192_PEK_SETTINGS_REG, 0x27
        )  # Turnoff PEK Overtime Shutdown
        self.__write_register(AXP192_IRQ_STATS_3, 0xFF)  # Clear the interrupts

        self.button_check_timer = Timer(
            Timer.TIMER2,
            Timer.CHANNEL0,
            mode=Timer.MODE_PERIODIC,
            period=100,
            callback=__ckeck_power_key__,
        )

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

class PMU_Button:
    """Class to mimic Krux GPIO button with interrupt events"""

    def __init__(self):
        self.state = RELEASED

    def value(self):
        """Returns PMU button value"""
        return __pre_but_pressed__

    def event(self):
        """Converts polling in events"""
        if self.state == RELEASED:
            if __pre_but_pressed__ == PRESSED:
                self.state = PRESSED
                return True
        self.state = __pre_but_pressed__
        return False
