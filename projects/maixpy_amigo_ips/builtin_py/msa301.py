
import struct

from micropython import const

class Struct:
    """
    Arbitrary structure register that is readable and writeable.
    Values are tuples that map to the values in the defined struct.  See struct
    module documentation for struct format string and its possible value types.
    :param int register_address: The register address to read the bit from
    :param type struct_format: The struct format string for this register.
    """

    def __init__(self, register_address, struct_format):
        self.format = struct_format
        self.buffer = bytearray(1 + struct.calcsize(self.format))
        self.buffer[0] = register_address

    def __get__(self, obj, objtype=None):
        with obj.i2c_device as i2c:
            i2c.write_then_readinto(self.buffer, self.buffer, out_end=1, in_start=1)
        return struct.unpack_from(self.format, memoryview(self.buffer)[1:])

    def __set__(self, obj, value):
        struct.pack_into(self.format, self.buffer, 1, *value)
        with obj.i2c_device as i2c:
            i2c.write(self.buffer)


class UnaryStruct:
    """
    Arbitrary single value structure register that is readable and writeable.
    Values map to the first value in the defined struct.  See struct
    module documentation for struct format string and its possible value types.
    :param int register_address: The register address to read the bit from
    :param type struct_format: The struct format string for this register.
    """

    def __init__(self, register_address, struct_format):
        self.format = struct_format
        self.address = register_address

    def __get__(self, obj, objtype=None):
        buf = bytearray(1 + struct.calcsize(self.format))
        buf[0] = self.address
        with obj.i2c_device as i2c:
            i2c.write_then_readinto(buf, buf, out_end=1, in_start=1)
        return struct.unpack_from(self.format, buf, 1)[0]

    def __set__(self, obj, value):
        buf = bytearray(1 + struct.calcsize(self.format))
        buf[0] = self.address
        struct.pack_into(self.format, buf, 1, value)
        with obj.i2c_device as i2c:
            i2c.write(buf)


class ROUnaryStruct(UnaryStruct):
    """
    Arbitrary single value structure register that is read-only.
    Values map to the first value in the defined struct.  See struct
    module documentation for struct format string and its possible value types.
    :param int register_address: The register address to read the bit from
    :param type struct_format: The struct format string for this register.
    """

    def __set__(self, obj, value):
        raise AttributeError()

class RWBit:
    """
    Single bit register that is readable and writeable.
    Values are `bool`
    :param int register_address: The register address to read the bit from
    :param type bit: The bit index within the byte at ``register_address``
    :param int register_width: The number of bytes in the register. Defaults to 1.
    :param bool lsb_first: Is the first byte we read from I2C the LSB? Defaults to true
    """

    def __init__(self, register_address, bit, register_width=1, lsb_first=True):
        self.bit_mask = 1 << (bit % 8)  # the bitmask *within* the byte!
        self.buffer = bytearray(1 + register_width)
        self.buffer[0] = register_address
        if lsb_first:
            self.byte = bit // 8 + 1  # the byte number within the buffer
        else:
            self.byte = register_width - (bit // 8)  # the byte number within the buffer

    def __get__(self, obj, objtype=None):
        with obj.i2c_device as i2c:
            i2c.write_then_readinto(self.buffer, self.buffer, out_end=1, in_start=1)
        return bool(self.buffer[self.byte] & self.bit_mask)

    def __set__(self, obj, value):
        with obj.i2c_device as i2c:
            i2c.write_then_readinto(self.buffer, self.buffer, out_end=1, in_start=1)
            if value:
                self.buffer[self.byte] |= self.bit_mask
            else:
                self.buffer[self.byte] &= ~self.bit_mask
            i2c.write(self.buffer)


class ROBit(RWBit):
    """Single bit register that is read only. Subclass of `RWBit`.
    Values are `bool`
    :param int register_address: The register address to read the bit from
    :param type bit: The bit index within the byte at ``register_address``
    :param int register_width: The number of bytes in the register. Defaults to 1.
    """

    def __set__(self, obj, value):
        raise AttributeError()


class RWBits:
    """
    Multibit register (less than a full byte) that is readable and writeable.
    This must be within a byte register.
    Values are `int` between 0 and 2 ** ``num_bits`` - 1.
    :param int num_bits: The number of bits in the field.
    :param int register_address: The register address to read the bit from
    :param type lowest_bit: The lowest bits index within the byte at ``register_address``
    :param int register_width: The number of bytes in the register. Defaults to 1.
    :param bool lsb_first: Is the first byte we read from I2C the LSB? Defaults to true
    :param bool signed: If True, the value is a "two's complement" signed value.
                        If False, it is unsigned.
    """

    def __init__(  # pylint: disable=too-many-arguments
        self,
        num_bits,
        register_address,
        lowest_bit,
        register_width=1,
        lsb_first=True,
        signed=False,
    ):
        self.bit_mask = ((1 << num_bits) - 1) << lowest_bit
        # print("bitmask: ",hex(self.bit_mask))
        if self.bit_mask >= 1 << (register_width * 8):
            raise ValueError("Cannot have more bits than register size")
        self.lowest_bit = lowest_bit
        self.buffer = bytearray(1 + register_width)
        self.buffer[0] = register_address
        self.lsb_first = lsb_first
        self.sign_bit = (1 << (num_bits - 1)) if signed else 0

    def __get__(self, obj, objtype=None):
        with obj.i2c_device as i2c:
            i2c.write_then_readinto(self.buffer, self.buffer, out_end=1, in_start=1)
        # read the number of bytes into a single variable
        reg = 0
        order = range(len(self.buffer) - 1, 0, -1)
        if not self.lsb_first:
            order = reversed(order)
        for i in order:
            reg = (reg << 8) | self.buffer[i]
        reg = (reg & self.bit_mask) >> self.lowest_bit
        # If the value is signed and negative, convert it
        if reg & self.sign_bit:
            reg -= 2 * self.sign_bit
        return reg

    def __set__(self, obj, value):
        value <<= self.lowest_bit  # shift the value over to the right spot
        with obj.i2c_device as i2c:
            i2c.write_then_readinto(self.buffer, self.buffer, out_end=1, in_start=1)
            reg = 0
            order = range(len(self.buffer) - 1, 0, -1)
            if not self.lsb_first:
                order = range(1, len(self.buffer))
            for i in order:
                reg = (reg << 8) | self.buffer[i]
            # print("old reg: ", hex(reg))
            reg &= ~self.bit_mask  # mask off the bits we're about to change
            reg |= value  # then or in our new value
            # print("new reg: ", hex(reg))
            for i in reversed(order):
                self.buffer[i] = reg & 0xFF
                reg >>= 8
            i2c.write(self.buffer)


class ROBits(RWBits):
    """
    Multibit register (less than a full byte) that is read-only. This must be
    within a byte register.
    Values are `int` between 0 and 2 ** ``num_bits`` - 1.
    :param int num_bits: The number of bits in the field.
    :param int register_address: The register address to read the bit from
    :param type lowest_bit: The lowest bits index within the byte at ``register_address``
    :param int register_width: The number of bytes in the register. Defaults to 1.
    """

    def __set__(self, obj, value):
        raise AttributeError()

class I2CDevice:
    """
    Represents a single I2C device and manages locking the bus and the device
    address.
    :param ~busio.I2C i2c: The I2C bus the device is on
    :param int device_address: The 7 bit device address
    :param bool probe: Probe for the device upon object creation, default is true
    .. note:: This class is **NOT** built into CircuitPython. See
      :ref:`here for install instructions <bus_device_installation>`.
    Example:
    .. code-block:: python
        import busio
        from board import *
        from adafruit_bus_device.i2c_device import I2CDevice
        with busio.I2C(SCL, SDA) as i2c:
            device = I2CDevice(i2c, 0x70)
            bytes_read = bytearray(4)
            with device:
                device.readinto(bytes_read)
            # A second transaction
            with device:
                device.write(bytes_read)
    """

    def __init__(self, i2c, device_address, probe=True):

        self.i2c = i2c
        self.device_address = device_address

        if probe:
            self.__probe_for_device()

    def readinto(self, buf, *, start=0, end=None):
        """
        Read into ``buf`` from the device. The number of bytes read will be the
        length of ``buf``.
        If ``start`` or ``end`` is provided, then the buffer will be sliced
        as if ``buf[start:end]``. This will not cause an allocation like
        ``buf[start:end]`` will so it saves memory.
        :param bytearray buffer: buffer to write into
        :param int start: Index to start writing at
        :param int end: Index to write up to but not include; if None, use ``len(buf)``
        """
        if end is None:
            end = len(buf)
        #self.i2c.readfrom_into(self.device_address, buf, start=start, end=end)
        self.i2c.readfrom_into(self.device_address, buf[start:end])
        # print('readinto', buf[start:end])

    def write(self, buf, *, start=0, end=None):
        """
        Write the bytes from ``buffer`` to the device, then transmit a stop
        bit.
        If ``start`` or ``end`` is provided, then the buffer will be sliced
        as if ``buffer[start:end]``. This will not cause an allocation like
        ``buffer[start:end]`` will so it saves memory.
        :param bytearray buffer: buffer containing the bytes to write
        :param int start: Index to start writing from
        :param int end: Index to read up to but not include; if None, use ``len(buf)``
        """
        if end is None:
            end = len(buf)
        #self.i2c.writeto(self.device_address, buf, start=start, end=end)
        self.i2c.writeto(self.device_address, buf[start:end])
        # print('write', buf[start:end])

    # pylint: disable-msg=too-many-arguments
    def write_then_readinto(
        self,
        out_buffer,
        in_buffer,
        *,
        out_start=0,
        out_end=None,
        in_start=0,
        in_end=None
    ):
        """
        Write the bytes from ``out_buffer`` to the device, then immediately
        reads into ``in_buffer`` from the device. The number of bytes read
        will be the length of ``in_buffer``.
        If ``out_start`` or ``out_end`` is provided, then the output buffer
        will be sliced as if ``out_buffer[out_start:out_end]``. This will
        not cause an allocation like ``buffer[out_start:out_end]`` will so
        it saves memory.
        If ``in_start`` or ``in_end`` is provided, then the input buffer
        will be sliced as if ``in_buffer[in_start:in_end]``. This will not
        cause an allocation like ``in_buffer[in_start:in_end]`` will so
        it saves memory.
        :param bytearray out_buffer: buffer containing the bytes to write
        :param bytearray in_buffer: buffer containing the bytes to read into
        :param int out_start: Index to start writing from
        :param int out_end: Index to read up to but not include; if None, use ``len(out_buffer)``
        :param int in_start: Index to start writing at
        :param int in_end: Index to write up to but not include; if None, use ``len(in_buffer)``
        """
        if out_end is None:
            out_end = len(out_buffer)
        if in_end is None:
            in_end = len(in_buffer)
        ## print('write_then_readinto', in_buffer[in_start:in_end], out_buffer[out_start:out_end])
        self.i2c.writeto(self.device_address, out_buffer[out_start:out_end])
        tmp = self.i2c.readfrom(self.device_address, in_end - in_start)
        in_buffer[in_start:in_end] = tmp
        # print('write_then_readinto', in_buffer)
        return
        self.i2c.writeto_then_readfrom(
            self.device_address,
            out_buffer,
            in_buffer,
            out_start=out_start,
            out_end=out_end,
            in_start=in_start,
            in_end=in_end,
        )

    # pylint: enable-msg=too-many-arguments

    def __enter__(self):
        #while not self.i2c.try_lock():
            #pass
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        #self.i2c.unlock()
        return False

    def __probe_for_device(self):
        """
        Try to read a byte from an address,
        if you get an OSError it means the device is not there
        or that the device does not support these means of probing
        """
        #while not self.i2c.try_lock():
            #pass
        #try:
            #self.i2c.writeto(self.device_address, b"")
        #except OSError:
            ## some OS's dont like writing an empty bytesting...
            ## Retry by reading a byte
            #try:
                #result = bytearray(1)
                #self.i2c.readfrom_into(self.device_address, result)
            #except OSError:
                #raise ValueError("No I2C device at address: %x" % self.device_address)
        #finally:
            #self.i2c.unlock()

_MSA301_I2CADDR_DEFAULT = 0x26
# _MSA301_I2CADDR_DEFAULT = const(0x26)

_MSA301_REG_PARTID = const(0x01)
_MSA301_REG_OUT_X_L = const(0x02)
_MSA301_REG_OUT_X_H = const(0x03)
_MSA301_REG_OUT_Y_L = const(0x04)
_MSA301_REG_OUT_Y_H = const(0x05)
_MSA301_REG_OUT_Z_L = const(0x06)
_MSA301_REG_OUT_Z_H = const(0x07)
_MSA301_REG_MOTIONINT = const(0x09)
_MSA301_REG_DATAINT = const(0x0A)
_MSA301_REG_RESRANGE = const(0x0F)
_MSA301_REG_ODR = const(0x10)
_MSA301_REG_POWERMODE = const(0x11)
_MSA301_REG_INTSET0 = const(0x16)
_MSA301_REG_INTSET1 = const(0x17)
_MSA301_REG_INTMAP0 = const(0x19)
_MSA301_REG_INTMAP1 = const(0x1A)
_MSA301_REG_TAPDUR = const(0x2A)
_MSA301_REG_TAPTH = const(0x2B)


_STANDARD_GRAVITY = 9.806


class Mode:  # pylint: disable=too-few-public-methods
    """An enum-like class representing the different modes that the MSA301 can
    use. The values can be referenced like ``Mode.NORMAL`` or ``Mode.SUSPEND``
    Possible values are
    - ``Mode.NORMAL``
    - ``Mode.LOW_POWER``
    - ``Mode.SUSPEND``
    """

    # pylint: disable=invalid-name
    NORMAL = 0b00
    LOWPOWER = 0b01
    SUSPEND = 0b010


class DataRate:  # pylint: disable=too-few-public-methods
    """An enum-like class representing the different data rates that the MSA301 can
    use. The values can be referenced like ``DataRate.RATE_1_HZ`` or ``DataRate.RATE_1000_HZ``
    Possible values are
    - ``DataRate.RATE_1_HZ``
    - ``DataRate.RATE_1_95_HZ``
    - ``DataRate.RATE_3_9_HZ``
    - ``DataRate.RATE_7_81_HZ``
    - ``DataRate.RATE_15_63_HZ``
    - ``DataRate.RATE_31_25_HZ``
    - ``DataRate.RATE_62_5_HZ``
    - ``DataRate.RATE_125_HZ``
    - ``DataRate.RATE_250_HZ``
    - ``DataRate.RATE_500_HZ``
    - ``DataRate.RATE_1000_HZ``
    """

    RATE_1_HZ = 0b0000  # 1 Hz
    RATE_1_95_HZ = 0b0001  # 1.95 Hz
    RATE_3_9_HZ = 0b0010  # 3.9 Hz
    RATE_7_81_HZ = 0b0011  # 7.81 Hz
    RATE_15_63_HZ = 0b0100  # 15.63 Hz
    RATE_31_25_HZ = 0b0101  # 31.25 Hz
    RATE_62_5_HZ = 0b0110  # 62.5 Hz
    RATE_125_HZ = 0b0111  # 125 Hz
    RATE_250_HZ = 0b1000  # 250 Hz
    RATE_500_HZ = 0b1001  # 500 Hz
    RATE_1000_HZ = 0b1010  # 1000 Hz


class BandWidth:  # pylint: disable=too-few-public-methods
    """An enum-like class representing the different bandwidths that the MSA301 can
    use. The values can be referenced like ``BandWidth.WIDTH_1_HZ`` or ``BandWidth.RATE_500_HZ``
    Possible values are
    - ``BandWidth.RATE_1_HZ``
    - ``BandWidth.RATE_1_95_HZ``
    - ``BandWidth.RATE_3_9_HZ``
    - ``BandWidth.RATE_7_81_HZ``
    - ``BandWidth.RATE_15_63_HZ``
    - ``BandWidth.RATE_31_25_HZ``
    - ``BandWidth.RATE_62_5_HZ``
    - ``BandWidth.RATE_125_HZ``
    - ``BandWidth.RATE_250_HZ``
    - ``BandWidth.RATE_500_HZ``
    - ``BandWidth.RATE_1000_HZ``
    """

    WIDTH_1_95_HZ = 0b0000  # 1.95 Hz
    WIDTH_3_9_HZ = 0b0011  # 3.9 Hz
    WIDTH_7_81_HZ = 0b0100  # 7.81 Hz
    WIDTH_15_63_HZ = 0b0101  # 15.63 Hz
    WIDTH_31_25_HZ = 0b0110  # 31.25 Hz
    WIDTH_62_5_HZ = 0b0111  # 62.5 Hz
    WIDTH_125_HZ = 0b1000  # 125 Hz
    WIDTH_250_HZ = 0b1001  # 250 Hz
    WIDTH_500_HZ = 0b1010  # 500 Hz


class Range:  # pylint: disable=too-few-public-methods
    """An enum-like class representing the different acceleration measurement ranges that the
    MSA301 can use. The values can be referenced like ``Range.RANGE_2_G`` or ``Range.RANGE_16_G``
    Possible values are
    - ``Range.RANGE_2_G``
    - ``Range.RANGE_4_G``
    - ``Range.RANGE_8_G``
    - ``Range.RANGE_16_G``
    """

    RANGE_2_G = 0b00  # +/- 2g (default value)
    RANGE_4_G = 0b01  # +/- 4g
    RANGE_8_G = 0b10  # +/- 8g
    RANGE_16_G = 0b11  # +/- 16g


class Resolution:  # pylint: disable=too-few-public-methods
    """An enum-like class representing the different measurement ranges that the MSA301 can
    use. The values can be referenced like ``Range.RANGE_2_G`` or ``Range.RANGE_16_G``
    Possible values are
    - ``Resolution.RESOLUTION_14_BIT``
    - ``Resolution.RESOLUTION_12_BIT``
    - ``Resolution.RESOLUTION_10_BIT``
    - ``Resolution.RESOLUTION_8_BIT``
    """

    RESOLUTION_14_BIT = 0b00
    RESOLUTION_12_BIT = 0b01
    RESOLUTION_10_BIT = 0b10
    RESOLUTION_8_BIT = 0b11


class TapDuration:  # pylint: disable=too-few-public-methods,too-many-instance-attributes
    """An enum-like class representing the options for the "double_tap_window" parameter of
    `enable_tap_detection`"""

    DURATION_50_MS = 0b000  # < 50 millis
    DURATION_100_MS = 0b001  # < 100 millis
    DURATION_150_MS = 0b010  # < 150 millis
    DURATION_200_MS = 0b011  # < 200 millis
    DURATION_250_MS = 0b100  # < 250 millis
    DURATION_375_MS = 0b101  # < 375 millis
    DURATION_500_MS = 0b110  # < 500 millis
    DURATION_700_MS = 0b111  # < 50 millis700 millis


class MSA301:  # pylint: disable=too-many-instance-attributes
    """Driver for the MSA301 Accelerometer.
        :param ~busio.I2C i2c_bus: The I2C bus the MSA is connected to.
    """

    _part_id = ROUnaryStruct(_MSA301_REG_PARTID, "<B")

    def __init__(self, i2c_bus):
        self.i2c_device = I2CDevice(i2c_bus, _MSA301_I2CADDR_DEFAULT)

        if self._part_id != 0x13:
            raise AttributeError("Cannot find a MSA301")

        self._disable_x = self._disable_y = self._disable_z = False
        self.power_mode = Mode.NORMAL
        self.data_rate = DataRate.RATE_500_HZ
        self.bandwidth = BandWidth.WIDTH_250_HZ
        self.range = Range.RANGE_4_G
        self.resolution = Resolution.RESOLUTION_14_BIT
        self._tap_count = 0

    _disable_x = RWBit(_MSA301_REG_ODR, 7)
    _disable_y = RWBit(_MSA301_REG_ODR, 6)
    _disable_z = RWBit(_MSA301_REG_ODR, 5)

    _xyz_raw = ROBits(48, _MSA301_REG_OUT_X_L, 0, 6)

    # tap INT enable and status
    _single_tap_int_en = RWBit(_MSA301_REG_INTSET0, 5)
    _double_tap_int_en = RWBit(_MSA301_REG_INTSET0, 4)
    _motion_int_status = ROUnaryStruct(_MSA301_REG_MOTIONINT, "B")

    # tap interrupt knobs
    _tap_quiet = RWBit(_MSA301_REG_TAPDUR, 7)
    _tap_shock = RWBit(_MSA301_REG_TAPDUR, 6)
    _tap_duration = RWBits(3, _MSA301_REG_TAPDUR, 0)
    _tap_threshold = RWBits(5, _MSA301_REG_TAPTH, 0)
    reg_tapdur = ROUnaryStruct(_MSA301_REG_TAPDUR, "B")

    # general settings knobs
    power_mode = RWBits(2, _MSA301_REG_POWERMODE, 6)
    bandwidth = RWBits(4, _MSA301_REG_POWERMODE, 1)
    data_rate = RWBits(4, _MSA301_REG_ODR, 0)
    range = RWBits(2, _MSA301_REG_RESRANGE, 0)
    resolution = RWBits(2, _MSA301_REG_RESRANGE, 2)

    @property
    def acceleration(self):
        """The x, y, z acceleration values returned in a 3-tuple and are in m / s ^ 2."""
        # read the 6 bytes of acceleration data
        # zh, zl, yh, yl, xh, xl
        raw_data = self._xyz_raw
        acc_bytes = bytearray()
        # shift out bytes, reversing the order
        for shift in range(6):
            bottom_byte = raw_data >> (8 * shift) & 0xFF
            acc_bytes.append(bottom_byte)

        # unpack three LE, signed shorts
        x, y, z = struct.unpack_from("<hhh", acc_bytes)

        current_range = self.range
        scale = 1.0
        if current_range == 3:
            scale = 512.0
        if current_range == 2:
            scale = 1024.0
        if current_range == 1:
            scale = 2048.0
        if current_range == 0:
            scale = 4096.0

        # shift down to the actual 14 bits and scale based on the range
        x_acc = ((x >> 2) / scale) * _STANDARD_GRAVITY
        y_acc = ((y >> 2) / scale) * _STANDARD_GRAVITY
        z_acc = ((z >> 2) / scale) * _STANDARD_GRAVITY

        return (x_acc, y_acc, z_acc)

    def enable_tap_detection(
        self,
        *,
        tap_count=1,
        threshold=25,
        long_initial_window=True,
        long_quiet_window=True,
        double_tap_window=TapDuration.DURATION_250_MS
    ):
        """
        Enables tap detection with configurable parameters.
        :param int tap_count: 1 to detect only single taps, or 2 to detect only double taps.\
        default is 1
        :param int threshold: A threshold for the tap detection.\
        The higher the value the less sensitive the detection. This changes based on the\
        accelerometer range. Default is 25.
        :param int long_initial_window: This sets the length of the window of time where a\
        spike in acceleration must occour in before being followed by a quiet period.\
        `True` (default) sets the value to 70ms, False to 50ms. Default is `True`
        :param int long_quiet_window: The length of the "quiet" period after an acceleration\
        spike where no more spikes can occour for a tap to be registered.\
        `True` (default) sets the value to 30ms, False to 20ms. Default is `True`.
        :param int double_tap_window: The length of time after an initial tap is registered\
        in which a second tap must be detected to count as a double tap. Setting a lower\
        value will require a faster double tap. The value must be a\
        ``TapDuration``. Default is ``TapDuration.DURATION_250_MS``.
        If you wish to set them yourself rather than using the defaults,
        you must use keyword arguments::
            msa.enable_tap_detection(tap_count=2,
                                     threshold=25,
                                     double_tap_window=TapDuration.DURATION_700_MS)
        """
        self._tap_shock = not long_initial_window
        self._tap_quiet = long_quiet_window
        self._tap_threshold = threshold
        self._tap_count = tap_count

        if double_tap_window > 7 or double_tap_window < 0:
            raise ValueError("double_tap_window must be a TapDuration")
        if tap_count == 1:
            self._single_tap_int_en = True
        elif tap_count == 2:
            self._tap_duration = double_tap_window
            self._double_tap_int_en = True
        else:
            raise ValueError("tap must be 1 for single tap, or 2 for double tap")

    @property
    def tapped(self):
        """`True` if a single or double tap was detected, depending on the value of the\
           ``tap_count`` argument passed to ``enable_tap_detection``"""
        if self._tap_count == 0:
            return False

        motion_int_status = self._motion_int_status

        if motion_int_status == 0:  # no interrupts triggered
            return False

        if self._tap_count == 1 and motion_int_status & 1 << 5:
            return True
        if self._tap_count == 2 and motion_int_status & 1 << 4:
            return True

        return False


if __name__ == "__main__":
    from machine import I2C
    i2c = I2C(I2C.I2C1, freq=100*1000, sda=31, scl=30) # cube
    #i2c = I2C(I2C.I2C1, freq=100*1000, sda=27, scl=24) # amigo
    dev_list = i2c.scan()
    print(_MSA301_I2CADDR_DEFAULT in dev_list)
    accel = MSA301(i2c)
    accel.enable_tap_detection()
    while True:
        #msa301.acceleration
        print("%f %f %f" % accel.acceleration)
        print(accel.tapped)
        time.sleep(0.1)
