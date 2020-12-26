# -*- coding: UTF-8 -*-

from machine import I2C

# I2C1_SDA      28 CDATA    -> IO_31
# I2C1_SCL      1 CCLK      -> IO_30

# I2S_MCLK      2 MCLK      -> IO_19
# I2S_SCLK      7 SCLK      -> IO_35
# I2S_DOUT      8 ASDOUT    -> IO_34
# I2S_LRCK(WS)  9 LECK      -> IO_33
# I2S_DIN       10 DSDIN    -> IO_18


# ------------------------------------

class ES8374_I2S_CLOCK:

    _LCLK_DIV_MIN = -1
    _LCLK_DIV_128 = 0
    _LCLK_DIV_192 = 1
    _LCLK_DIV_256 = 2
    _LCLK_DIV_384 = 3
    _LCLK_DIV_512 = 4
    _LCLK_DIV_576 = 5
    _LCLK_DIV_768 = 6
    _LCLK_DIV_1024 = 7
    _LCLK_DIV_1152 = 8
    _LCLK_DIV_1408 = 9
    _LCLK_DIV_1536 = 1
    _LCLK_DIV_2112 = 1
    _LCLK_DIV_2304 = 1

    _LCLK_DIV_125 = 16
    _LCLK_DIV_136 = 17
    _LCLK_DIV_250 = 18
    _LCLK_DIV_272 = 19
    _LCLK_DIV_375 = 20
    _LCLK_DIV_500 = 21
    _LCLK_DIV_544 = 22
    _LCLK_DIV_750 = 23
    _LCLK_DIV_1000 = 24
    _LCLK_DIV_1088 = 25
    _LCLK_DIV_1496 = 26
    _LCLK_DIV_1500 = 27

    _MCLK_DIV_MIN = -1
    _MCLK_DIV_1 = 1
    _MCLK_DIV_2 = 2
    _MCLK_DIV_3 = 3
    _MCLK_DIV_4 = 4
    _MCLK_DIV_6 = 5
    _MCLK_DIV_8 = 6
    _MCLK_DIV_9 = 7
    _MCLK_DIV_11 = 8
    _MCLK_DIV_12 = 9
    _MCLK_DIV_16 = 10
    _MCLK_DIV_18 = 11
    _MCLK_DIV_22 = 12
    _MCLK_DIV_24 = 13
    _MCLK_DIV_33 = 14
    _MCLK_DIV_36 = 15
    _MCLK_DIV_44 = 16
    _MCLK_DIV_48 = 17
    _MCLK_DIV_66 = 18
    _MCLK_DIV_72 = 19
    _MCLK_DIV_5 = 20
    _MCLK_DIV_10 = 21
    _MCLK_DIV_15 = 22
    _MCLK_DIV_17 = 23
    _MCLK_DIV_20 = 24
    _MCLK_DIV_25 = 25
    _MCLK_DIV_30 = 26
    _MCLK_DIV_32 = 27
    _MCLK_DIV_34 = 28
    _MCLK_DIV_7 = 29
    _MCLK_DIV_13 = 30
    _MCLK_DIV_14 = 31

    def __init__(self):
        self.lclk_div = self._LCLK_DIV_256
        self.sclk_div = self._MCLK_DIV_4


class ES8374_CONFIG:

    # es8374_adc_input_t
    _ES8374_ADC_INPUT_LINE1      = 0
    _ES8374_ADC_INPUT_LINE2      = 1
    _ES8374_ADC_INPUT_ALL        = 2
    _ES8374_ADC_INPUT_DIFFERENCE = 3

    # es8374_dac_output_t
    _ES8374_DAC_OUTPUT_LINE1 = 0
    _ES8374_DAC_OUTPUT_LINE2 = 1
    _ES8374_DAC_OUTPUT_ALL   = 2

    # es8374_mode_t
    _ES8374_MODE_ENCODE  = 1
    _ES8374_MODE_DECODE  = 2
    _ES8374_MODE_BOTH    = 3
    _ES8374_MODE_LINE_IN = 4

    def __init__(self):
        self.adc_input      = self._ES8374_ADC_INPUT_LINE1
        self.dac_output     = self._ES8374_DAC_OUTPUT_LINE1
        self.es8374_mode    = self._ES8374_MODE_BOTH
        self.i2s_iface      = self.I2S_IFACE()

    class I2S_IFACE:

        # es8374_iface_mode_t
        _ES8374_MODE_SLAVE  = 0
        _ES8374_MODE_MASTER = 1

        # es8374_iface_format_t
        _ES8374_I2S_NORMAL  = 0
        _ES8374_I2S_LEFT    = 1
        _ES8374_I2S_RIGHT   = 2
        _ES8374_I2S_DSP     = 3

        # es8374_iface_samples_t
        _ES8374_08K_SAMPLES = 0
        _ES8374_11K_SAMPLES = 1
        _ES8374_16K_SAMPLES = 2
        _ES8374_22K_SAMPLES = 3
        _ES8374_24K_SAMPLES = 4
        _ES8374_32K_SAMPLES = 5
        _ES8374_44K_SAMPLES = 6
        _ES8374_48K_SAMPLES = 7

        # es8374_iface_bits_t
        _ES8374_BIT_LENGTH_16BITS   = 1
        _ES8374_BIT_LENGTH_24BITS   = 2
        _ES8374_BIT_LENGTH_32BITS   = 3


        def __init__(self):

            self.mode       = self._ES8374_MODE_SLAVE
            self.fmt        = self._ES8374_I2S_NORMAL
            self.samples    = self._ES8374_44K_SAMPLES
            self.bits       = self._ES8374_BIT_LENGTH_16BITS

#######################

class ES_MODULE:
    _ES_MODULE_MIN      = -1
    _ES_MODULE_ADC      = 0x01
    _ES_MODULE_DAC      = 0x02
    _ES_MODULE_ADC_DAC  = 0x03
    _ES_MODULE_LINE     = 0x04

class ES_BITS_LENGTH:
    _BIT_LENGTH_MIN      = -1
    _BIT_LENGTH_16BITS   = 0x03
    _BIT_LENGTH_18BITS   = 0x02
    _BIT_LENGTH_20BITS   = 0x01
    _BIT_LENGTH_24BITS   = 0x00
    _BIT_LENGTH_32BITS   = 0x04

class MIC_GAIN:
    _MIC_GAIN_MIN   = -1
    _MIC_GAIN_0DB   = 0
    _MIC_GAIN_3DB   = 3
    _MIC_GAIN_6DB   = 6
    _MIC_GAIN_9DB   = 9
    _MIC_GAIN_12DB  = 12
    _MIC_GAIN_15DB  = 15
    _MIC_GAIN_18DB  = 18
    _MIC_GAIN_21DB  = 21
    _MIC_GAIN_24DB  = 24

class D2SE_PGA:
    _D2SE_PGA_GAIN_MIN = -1
    _D2SE_PGA_GAIN_DIS = 0
    _D2SE_PGA_GAIN_EN = 1
    _D2SE_PGA_GAIN_MAX = 2

class BIT_LENGTH:
    _BIT_LENGTH_MIN         = -1
    _BIT_LENGTH_16BITS      = 0x03
    _BIT_LENGTH_18BITS      = 0x02
    _BIT_LENGTH_20BITS      = 0x01
    _BIT_LENGTH_24BITS      = 0x00
    _BIT_LENGTH_32BITS      = 0x04

class AUDIO_BIT_IFACE_LENGTH:
    _AUDIO_BIT_LENGTH_16BITS   = 1
    _AUDIO_BIT_LENGTH_24BITS   = 2
    _AUDIO_BIT_LENGTH_32BITS   = 3

class ES_CTRL:
    _ES8374_CTRL_STOP = 0x00
    _ES8374_CTRL_START = 0x01

# -----------
class ES8374:

    _ES8374_I2CADDR_DEFAULT = 0x10

    def __init__(self, i2c_bus=None, i2c_addr=_ES8374_I2CADDR_DEFAULT):
        if i2c_bus == None:
            self.i2c_bus = I2C(I2C.I2C1, freq=100*1000) # , sda=31, scl=30
        else:
            self.i2c_bus = i2c_bus

        self.i2c_addr = i2c_addr

        # dev_list = self.i2c_bus.scan()
        # # print("i2c devs_list:" + str(dev_list))

        self.clkdiv_cfg = ES8374_I2S_CLOCK()
        self.es8374_cfg = ES8374_CONFIG()
        # # print("self.es8374_cfg.es8374_mode:" + str(self.es8374_cfg.es8374_mode))
        self.stop(self.es8374_cfg.es8374_mode)
        self.init_reg(self.es8374_cfg.i2s_iface.mode,
            ((ES_BITS_LENGTH._BIT_LENGTH_16BITS << 4) | self.es8374_cfg.i2s_iface.fmt),
            self.clkdiv_cfg,
            self.es8374_cfg.dac_output,
            self.es8374_cfg.adc_input)

        self.setMICGain(MIC_GAIN._MIC_GAIN_18DB)
        self.setD2sePga(D2SE_PGA._D2SE_PGA_GAIN_EN)
        self.configI2SFormat(self.es8374_cfg.es8374_mode, self.es8374_cfg.i2s_iface.fmt)
        self.codecConfigI2S(self.es8374_cfg.es8374_mode, self.es8374_cfg.i2s_iface.fmt, self.es8374_cfg.i2s_iface.bits)

        # ---------------------
        # Play init

        # self.codecCtrlSate(self.es8374_cfg.es8374_mode, ES_CTRL._ES8374_CTRL_START)
        # # self.setVoiceVolume(30)
        # self.setVoiceVolume(100)
        # self._writeReg(0x1E, 0xA4)

        # # print("ES8374 |reg value: " + str(self._readREGAll()))
        # ---------------------


    def __deinit__(self):
        self._writeReg(0x00, 0x7F) # IC Rest and STOP

    # read reg value
    def _readReg(self, regAddr):
        self.i2c_bus.writeto(self.i2c_addr, bytes([regAddr]))
        return (self.i2c_bus.readfrom(self.i2c_addr, 1))[0]

    # write value to reg
    def _writeReg(self, regAddr, data):
        return self.i2c_bus.writeto_mem(self.i2c_addr, regAddr, data, mem_size=8)


    # read all reg value
    def _readREGAll(self):
        regValue = bytearray([])
        for i in range(0x6f):
            regValue.append(self._readReg(i))
        return regValue

    def stop(self, mode):
        if (mode == ES_MODULE._ES_MODULE_LINE):
            reg = self._readReg(0x1a) # disable lout
            reg |= 0x08
            self._writeReg(0x1a, reg)
            reg &= 0x9f
            self._writeReg(0x1a, reg)
            self._writeReg(0x1D, 0x12)# mute speaker
            self._writeReg(0x1E, 0x20)# disable class d
            reg = self._readReg(0x1c) #  disable spkmixer
            reg &= 0xbf
            self._writeReg(0x1c, reg)
            self._writeReg(0x1F, 0x00)# spk set

        if ((mode == ES_MODULE._ES_MODULE_DAC) or (mode == ES_MODULE._ES_MODULE_ADC_DAC)):
            self.setVoiceMute(True)
            reg = self._readReg(0x1a) # #disable lout
            reg |= 0x08
            self._writeReg(0x1a, reg)
            reg &= 0xdf
            self._writeReg(0x1a, reg)
            self._writeReg(0x1D, 0x12)# mute speaker
            self._writeReg(0x1E, 0x20)# disable class d
            reg = self._readReg(0x15) #  #power up dac
            reg |= 0x20
            self._writeReg(0x15, reg)

        if (mode == ES_MODULE._ES_MODULE_ADC or mode == ES_MODULE._ES_MODULE_ADC_DAC):
            reg = self._readReg(0x10) # #power up adc and input
            reg |= 0xc0
            self._writeReg(0x10, reg)
            reg = self._readReg(0x21) # #power up adc and input
            reg |= 0xc0
            self._writeReg(0x21, reg)

    def start(self, mode):
        if (mode == ES_MODULE._ES_MODULE_LINE):
            reg = self._readReg(0x1a) # set monomixer
            reg |= 0x60
            reg |= 0x20
            reg &= 0xf7
            self._writeReg(0x1a, reg)
            reg = self._readReg(0x1c) #  set spk mixer
            reg |= 0x40
            self._writeReg(0x1c, reg)
            self._writeReg(0x1D, 0x02) #  spk set
            self._writeReg(0x1F, 0x00) #  spk set
            self._writeReg(0x1E, 0xA0) #  spk on
        if (mode == ES_MODULE._ES_MODULE_ADC
            or mode == ES_MODULE._ES_MODULE_ADC_DAC
            or mode == ES_MODULE._ES_MODULE_LINE):
            reg = self._readReg(0x21) # power up adc and input
            reg &= 0x3f
            self._writeReg(0x21, reg)
            reg = self._readReg(0x10) # power up adc and input
            reg &= 0x3f
            self._writeReg(0x10, reg)

        if (mode == ES_MODULE._ES_MODULE_DAC
            or mode == ES_MODULE._ES_MODULE_ADC_DAC
            or mode == ES_MODULE._ES_MODULE_LINE):
            reg = self._readReg(0x1a) # disable lout
            reg |= 0x08
            self._writeReg(0x1a, reg)
            reg &= 0xdf
            self._writeReg(0x1a, reg)
            self._writeReg(0x1D, 0x12) #  mute speaker
            self._writeReg(0x1E, 0x20) #  disable class d
            reg = self._readReg(0x15)  # power up dac
            reg &= 0xdf
            self._writeReg(0x15, reg)
            reg = self._readReg(0x1a) # disable lout
            reg |= 0x20
            self._writeReg(0x1a, reg)
            reg &= 0xf7
            self._writeReg(0x1a, reg)
            self._writeReg(0x1D, 0x02) #  mute speaker
            self._writeReg(0x1E, 0xa0) #  disable class d

            self.setVoiceMute(False)

        # print("mode is :%d \r\n" % mode)

    def init_reg(self, ms_mode, fmt, cfg, out_channel, in_channel):

        self._writeReg(0x00, 0x3F)# IC Rst start
        self._writeReg(0x00, 0x03)# IC Rst stop
        self._writeReg(0x01, 0x7F)# IC clk on # M ORG 7F

        reg = self._readReg(0x0F)
        reg &= 0x7f
        reg |= (ms_mode << 7)
        self._writeReg(0x0f, reg)# CODEC IN I2S SLAVE MODE

        self._writeReg(0x6F, 0xA0)# pll set:mode enable
        self._writeReg(0x72, 0x41)# pll set:mode set
        #  self._writeReg(0x09,0x01)# pll set:reset on ,set start
        #  self._writeReg(0x0C,0x22)# pll set:k
        #  self._writeReg(0x0D,0x2E)# pll set:k
        #  self._writeReg(0x0E,0xC6)# pll set:k
        #  self._writeReg(0x0A,0x3A)# pll set:
        #  self._writeReg(0x0B,0x07)# pll set:n
        #  self._writeReg(0x09,0x41)# pll set:reset off ,set stop
        self._writeReg(0x09, 0x01)# pll set:reset on ,set start
        self._writeReg(0x0C, 0x08)# pll set:k
        self._writeReg(0x0D, 0x13)# pll set:k
        self._writeReg(0x0E, 0xE0)# pll set:k
        self._writeReg(0x0A, 0x8A)# pll set:
        self._writeReg(0x0B, 0x08)# pll set:n
        self._writeReg(0x09, 0x41)# pll set:reset off ,set stop

        self.i2sConfigClock(cfg)

        self._writeReg(0x24, 0x08)# adc set
        self._writeReg(0x36, 0x00)# dac set
        self._writeReg(0x12, 0x30)# timming set
        self._writeReg(0x13, 0x20)# timming set

        self.configI2SFormat(ES_MODULE._ES_MODULE_ADC, fmt)
        self.configI2SFormat(ES_MODULE._ES_MODULE_DAC, fmt)

        self._writeReg(0x21, 0x50)# adc set: SEL LIN1 CH+PGAGAIN=0DB
        self._writeReg(0x22, 0xFF)# adc set: PGA GAIN=0DB
        self._writeReg(0x21, 0x14)# adc set: SEL LIN1 CH+PGAGAIN=18DB
        self._writeReg(0x22, 0x55)# pga = +15db
        self._writeReg(0x08, 0x21)# set class d divider = 33, to avoid the high frequency tone on laudspeaker
        self._writeReg(0x00, 0x80)#  IC START

        self.setADCDACVolume(ES_MODULE._ES_MODULE_ADC, 0, 0) # 0dB
        self.setADCDACVolume(ES_MODULE._ES_MODULE_DAC, 0, 0) # 0dB

        self._writeReg(0x14, 0x8A)#  IC START
        self._writeReg(0x15, 0x40)#  IC START
        self._writeReg(0x1A, 0xA0)#  monoout set
        self._writeReg(0x1B, 0x19)#  monoout set
        self._writeReg(0x1C, 0x90)#  spk set
        self._writeReg(0x1D, 0x01)#  spk set
        self._writeReg(0x1F, 0x00)#  spk set
        self._writeReg(0x1E, 0x20)#  spk on
        self._writeReg(0x28, 0x70)#  alc set 0x70
        self._writeReg(0x26, 0x4E)#  alc set
        self._writeReg(0x27, 0x10)#  alc set
        self._writeReg(0x29, 0x00)#  alc set
        self._writeReg(0x2B, 0x00)#  alc set

        self._writeReg(0x25, 0x00)#  ADCVOLUME on
        self._writeReg(0x38, 0x00)#  DACVOLUME on
        self._writeReg(0x37, 0x30)#  dac set
        self._writeReg(0x6D, 0x60)# SEL:GPIO1=DMIC CLK OUT+SEL:GPIO2=PLL CLK OUT
        self._writeReg(0x71, 0x05)# for automute setting
        self._writeReg(0x73, 0x70)

        self.configDACOutput(out_channel)# 0x3c Enable DAC and Enable Lout/Rout/1/2
        self.configADCInput(in_channel)# 0x00 LINSEL & RINSEL, LIN1/RIN1 as ADC Input DSSEL,use one DS Reg11 DSR, LINPUT1-RINPUT1
        self.setVoiceVolume(0)
        self._writeReg(0x37, 0x00)#  dac set



    def i2sConfigClock(self, cfg):
        reg = self._readReg(0x0F) # power up adc and input
        reg &= 0xe0
        divratio = 0
        sclk_div_dir = {
            ES8374_I2S_CLOCK._MCLK_DIV_1  : 1, #
            ES8374_I2S_CLOCK._MCLK_DIV_2  : 2, # 2
            ES8374_I2S_CLOCK._MCLK_DIV_3  : 3, # 3
            ES8374_I2S_CLOCK._MCLK_DIV_4  : 4, # 4
            ES8374_I2S_CLOCK._MCLK_DIV_6  : 5, # 20
            ES8374_I2S_CLOCK._MCLK_DIV_8  : 6, # 5
            ES8374_I2S_CLOCK._MCLK_DIV_9  : 7, # 29
            ES8374_I2S_CLOCK._MCLK_DIV_11 : 8, # 6
            ES8374_I2S_CLOCK._MCLK_DIV_12 : 9, # 7
            ES8374_I2S_CLOCK._MCLK_DIV_16 : 10, # 21
            ES8374_I2S_CLOCK._MCLK_DIV_18 : 11, # 8
            ES8374_I2S_CLOCK._MCLK_DIV_22 : 12, # 9
            ES8374_I2S_CLOCK._MCLK_DIV_24 : 13, # 30
            ES8374_I2S_CLOCK._MCLK_DIV_33 : 14, # 31
            ES8374_I2S_CLOCK._MCLK_DIV_36 : 15, # 22
            ES8374_I2S_CLOCK._MCLK_DIV_44 : 16, # 10
            ES8374_I2S_CLOCK._MCLK_DIV_48 : 17, # 23
            ES8374_I2S_CLOCK._MCLK_DIV_66 : 18, # 11
            ES8374_I2S_CLOCK._MCLK_DIV_72 : 19, # 24
            ES8374_I2S_CLOCK._MCLK_DIV_5  : 20, # 12
            ES8374_I2S_CLOCK._MCLK_DIV_10 : 21, # 13
            ES8374_I2S_CLOCK._MCLK_DIV_15 : 22, # 25
            ES8374_I2S_CLOCK._MCLK_DIV_17 : 23, # 26
            ES8374_I2S_CLOCK._MCLK_DIV_20 : 24, # 27
            ES8374_I2S_CLOCK._MCLK_DIV_25 : 25, # 14
            ES8374_I2S_CLOCK._MCLK_DIV_30 : 26, # 28
            ES8374_I2S_CLOCK._MCLK_DIV_32 : 27, # 15
            ES8374_I2S_CLOCK._MCLK_DIV_34 : 28, # 16
            ES8374_I2S_CLOCK._MCLK_DIV_7  : 29, # 17
            ES8374_I2S_CLOCK._MCLK_DIV_13 : 30, # 18
            ES8374_I2S_CLOCK._MCLK_DIV_14 : 31, # 19
        }
        divratio = sclk_div_dir.get(cfg.sclk_div, None)

        reg |= divratio
        self._writeReg(0x0f, reg)

        lclk_div_dacratio_l_dir = {
            ES8374_I2S_CLOCK._LCLK_DIV_128   : (128  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_192   : (192  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_256   : (256  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_384   : (384  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_512   : (512  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_576   : (576  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_768   : (768  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_1024  : (1024 &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_1152  : (1152 &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_1408  : (1408 &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_1536  : (1536 &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_2112  : (2112 &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_2304  : (2304 &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_125   : (125  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_136   : (136  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_250   : (250  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_272   : (272  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_375   : (375  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_500   : (500  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_544   : (544  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_750   : (750  &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_1000  : (1000 &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_1088  : (1088 &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_1496  : (1496 &0xFF),
            ES8374_I2S_CLOCK._LCLK_DIV_1500  : (1500 &0xFF),
        }
        lclk_div_dacratio_h_dir = {
            ES8374_I2S_CLOCK._LCLK_DIV_128   : (128  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_192   : (192  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_256   : (256  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_384   : (384  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_512   : (512  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_576   : (576  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_768   : (768  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_1024  : (1024 >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_1152  : (1152 >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_1408  : (1408 >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_1536  : (1536 >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_2112  : (2112 >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_2304  : (2304 >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_125   : (125  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_136   : (136  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_250   : (250  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_272   : (272  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_375   : (375  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_500   : (500  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_544   : (544  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_750   : (750  >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_1000  : (1000 >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_1088  : (1088 >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_1496  : (1496 >>8),
            ES8374_I2S_CLOCK._LCLK_DIV_1500  : (1500 >>8),
        }
        dacratio_l = lclk_div_dacratio_l_dir.get(cfg.lclk_div, None)
        dacratio_h = lclk_div_dacratio_h_dir.get(cfg.lclk_div, None)

        self._writeReg(0x06, dacratio_h)# ADCFsMode,singel SPEED,RATIO=256
        self._writeReg(0x07, dacratio_l)# ADCFsMode,singel SPEED,RATIO=256

    def configI2SFormat(self, mode, fmt):
        fmt_tmp = ((fmt & 0xf0) >> 4)
        fmt_i2s =  fmt & 0x0f
        if (mode == ES_MODULE._ES_MODULE_ADC or mode == ES_MODULE._ES_MODULE_ADC_DAC):
            reg = self._readReg(0x10)
            reg &= 0xfc
            self._writeReg(0x10, (reg|fmt_i2s))
            self.setBitsPerSample(mode, fmt_tmp)

        if (mode == ES_MODULE._ES_MODULE_DAC or mode == ES_MODULE._ES_MODULE_ADC_DAC):
            reg = self._readReg(0x11)
            reg &= 0xfc
            self._writeReg(0x11, (reg|fmt_i2s))
            self.setBitsPerSample(mode, fmt_tmp)

    # set Bits Per Sample
    def setBitsPerSample(self, mode, bit_per_smaple):
        bits = bit_per_smaple & 0x0f

        if (mode == ES_MODULE._ES_MODULE_ADC or mode == ES_MODULE._ES_MODULE_ADC_DAC):
            reg = self._readReg(0x10)
            reg &= 0xe3
            self._writeReg(0x10, (reg| (bits << 2)))

        if (mode == ES_MODULE._ES_MODULE_DAC or mode == ES_MODULE._ES_MODULE_ADC_DAC):
            reg = self._readReg(0x11)
            reg &= 0xe3
            self._writeReg(0x11, (reg| (bits << 2)))

    # set ADC DAC Volume
    def setADCDACVolume(self, mode, volume, dot):
        if ((volume < -96) or (volume>0)):
            # print("Warning: volume < -96! or > 0!")
            if (volume < -96):
                volume = -96
            else:
                volume = 0

        dot = 1 if (dot >= 5) else 0
        volume = (-volume << 1) + dot
        if (mode == ES_MODULE._ES_MODULE_ADC or mode == ES_MODULE._ES_MODULE_ADC_DAC):
            self._writeReg(0x25, volume)
        if (mode == ES_MODULE._ES_MODULE_DAC or mode == ES_MODULE._ES_MODULE_ADC_DAC):
            self._writeReg(0x38, volume)

    def configDACOutput(self, output):
        reg = 0x1d
        self._writeReg(reg, 0x02)
        reg = self._readReg(0x1c) # set spk mixer
        reg |= 0x80
        self._writeReg(0x1c, reg)
        self._writeReg(0x1D, 0x02) # spk set
        self._writeReg(0x1F, 0x00) # spk set
        self._writeReg(0x1E, 0xA0) # spk on

    # NOTE: input not used
    def configADCInput(self, input):
        reg = 0x1d
        reg = self._readReg(0x21)
        reg = (reg & 0xcf) | 0x14
        self._writeReg( 0x21, reg)
        self._writeReg( 0x21, 0x24)

    # set Voice Volume
    def setVoiceVolume(self, volume):
        if volume < 0 :
            volume = 192
        elif volume > 96 :
            volume = 0
        else:
            volume = 192 - volume*2
        return self._writeReg(0x38, volume)

    # get Voice Volume
    def getVoiceVolume(self):
        volume = self._readReg(0x38)
        volume = (192-volume)/2
        if volume > 96:
            volume = 100
        return volume

    # codec Config I2S
    def codecConfigI2S(self, mode, interface_fmt, interface_bits):
        self.configI2SFormat(ES_MODULE._ES_MODULE_ADC, interface_fmt)
        if (interface_bits == AUDIO_BIT_IFACE_LENGTH._AUDIO_BIT_LENGTH_16BITS):
            tmp = BIT_LENGTH._BIT_LENGTH_16BITS
        elif (interface_bits == AUDIO_BIT_IFACE_LENGTH._AUDIO__BIT_LENGTH_24BITS):
            tmp = BIT_LENGTH._BIT_LENGTH_24BITS
        else:
            tmp = BIT_LENGTH._BIT_LENGTH_32BITS

        self.setBitsPerSample(ES_MODULE._ES_MODULE_ADC_DAC, tmp)

    def setVoiceMute(self, enable):
        reg = self._readReg(0x36)
        reg &= 0xdf
        reg = ( reg | (enable << 5))
        # print("SetVoiceMute[%s]:" % str(reg) )
        self._writeReg(0x36, reg)

    # get voice mute
    def getVoiceMute(self):
        value = self._readReg(0x36)
        return True if (value & 0x40) else False

    # set mic gain
    def setMICGain(self, gain):
        if ((gain>MIC_GAIN._MIC_GAIN_MIN) and (gain<MIC_GAIN._MIC_GAIN_24DB)):
            gain_n = 0
            gain_n = int(gain/3)
            reg = (gain_n | (gain_n<<4))
            # print("22H:0x%X" % reg)
            self._writeReg(0x22, reg) # MIC PGA
        else:
            pass
            # print("invalid micropython gain")

    def setD2sePga(self, gain):
        if ((gain>D2SE_PGA._D2SE_PGA_GAIN_MIN) and (gain<D2SE_PGA._D2SE_PGA_GAIN_MAX)):
            reg = self._readReg(0x21)
            reg &= 0xfb
            reg |= gain << 2
            self._writeReg(0x21, reg) # MIC PGA
        else:
            pass
            # print("invalid microphone gain!")

    def codecCtrlSate(self, mode, ctrl_state):
        mode_list = {
            ES8374_CONFIG._ES8374_MODE_ENCODE: ES_MODULE._ES_MODULE_ADC,
            ES8374_CONFIG._ES8374_MODE_LINE_IN: ES_MODULE._ES_MODULE_LINE,
            ES8374_CONFIG._ES8374_MODE_DECODE: ES_MODULE._ES_MODULE_DAC,
            ES8374_CONFIG._ES8374_MODE_BOTH: ES_MODULE._ES_MODULE_ADC_DAC,
        }
        es_mode = mode_list.get(mode, None)
        if (es_mode == None):
            es_mode = ES_MODULE._ES_MODULE_DAC
            # print("Codec mode not support, default is decode mode\r\n")

        if (ES_CTRL._ES8374_CTRL_STOP == ctrl_state):
            res = self.stop(es_mode)
        else:
            res = self.start(es_mode)
            # print("start default is decode mode:%d\r\n" % es_mode)



'''-----------------------------
1.测试 I2S 是否能够输出型号
2.测试 ES8374 是否初始化成功
3.测试 ES8374 是否配置正常

-----------------------------'''

if __name__ == "__main__":


    from fpioa_manager import *
    from Maix import GPIO, I2S, FFT

    import audio, time, image, lcd

    # cube
    #fm.register(19,fm.fpioa.I2S0_MCLK, force=True)
    #fm.register(35,fm.fpioa.I2S0_SCLK, force=True)
    #fm.register(33,fm.fpioa.I2S0_WS, force=True)
    #fm.register(34,fm.fpioa.I2S0_IN_D0, force=True)
    #fm.register(18,fm.fpioa.I2S0_OUT_D2, force=True)

    # amigo
    #fm.register(13,fm.fpioa.I2S0_MCLK, force=True)
    #fm.register(21,fm.fpioa.I2S0_SCLK, force=True)
    #fm.register(18,fm.fpioa.I2S0_WS, force=True)
    #fm.register(35,fm.fpioa.I2S0_IN_D0, force=True)
    #fm.register(34,fm.fpioa.I2S0_OUT_D2, force=True)

    #i2c = I2C(I2C.I2C1, freq=100*1000) # , sda=31, scl=30
    i2c = I2C(I2C.I2C3, freq=600*1000, sda=27, scl=24) # amigo

    #fm.register(30,fm.fpioa.I2C1_SCLK, force=True)
    #fm.register(31,fm.fpioa.I2C1_SDA, force=True)

    #fm.register(24,fm.fpioa.I2C1_SCLK, force=True)
    #fm.register(27,fm.fpioa.I2C1_SDA, force=True)

    while True:

        try:
            print(i2c.scan())

            time.sleep_ms(2000)

            dev = ES8374(i2c)

            dev.setVoiceVolume(100)

            dev.start(ES_MODULE._ES_MODULE_ADC_DAC)

            # init i2s(i2s0)
            i2s = I2S(I2S.DEVICE_0, pll2=262144000, mclk=31)

            # config i2s according to audio info # STANDARD_MODE LEFT_JUSTIFYING_MODE RIGHT_JUSTIFYING_MODE
            i2s.channel_config(I2S.CHANNEL_0, I2S.RECEIVER, resolution=I2S.RESOLUTION_16_BIT, cycles=I2S.SCLK_CYCLES_32, align_mode=I2S.STANDARD_MODE)

            fm.register(13,fm.fpioa.I2S0_MCLK, force=True)
            fm.register(21,fm.fpioa.I2S0_SCLK, force=True)
            fm.register(18,fm.fpioa.I2S0_WS, force=True)
            fm.register(35,fm.fpioa.I2S0_IN_D0, force=True)
            fm.register(34,fm.fpioa.I2S0_OUT_D2, force=True)

            i2s.set_sample_rate(22050)

            player = audio.Audio(path="/sd/record_4.wav", is_create=True, samplerate=22050)
            queue = []
            for i in range(200):
             tmp = i2s.record(1024)
             if len(queue) > 0:
                 print(time.ticks())
                 ret = player.record(queue[0])
                 queue.pop(0)
             i2s.wait_record()
             queue.append(tmp)
            player.finish()

            del i2s, player

            time.sleep_ms(2000)

            dev = ES8374(i2c)

            dev.setVoiceVolume(90)

            dev.start(ES_MODULE._ES_MODULE_ADC_DAC)

            # init i2s(i2s0)
            i2s = I2S(I2S.DEVICE_0, pll2=262144000, mclk=31)

            # config i2s according to audio info # STANDARD_MODE LEFT_JUSTIFYING_MODE RIGHT_JUSTIFYING_MODE
            i2s.channel_config(I2S.CHANNEL_2, I2S.TRANSMITTER, resolution=I2S.RESOLUTION_16_BIT, cycles=I2S.SCLK_CYCLES_32, align_mode=I2S.STANDARD_MODE)

            #fm.register(19,fm.fpioa.I2S0_MCLK, force=True)
            #fm.register(35,fm.fpioa.I2S0_SCLK, force=True)
            #fm.register(33,fm.fpioa.I2S0_WS, force=True)
            #fm.register(34,fm.fpioa.I2S0_IN_D0, force=True)
            #fm.register(18,fm.fpioa.I2S0_OUT_D2, force=True)

            fm.register(13,fm.fpioa.I2S0_MCLK, force=True)
            fm.register(21,fm.fpioa.I2S0_SCLK, force=True)
            fm.register(18,fm.fpioa.I2S0_WS, force=True)
            fm.register(35,fm.fpioa.I2S0_IN_D0, force=True)
            fm.register(34,fm.fpioa.I2S0_OUT_D2, force=True)

            for i in range(1):

                time.sleep_ms(10)

                # init audio
                #player = audio.Audio(path="/sd/res/sound/loop.wav")
                player = audio.Audio(path="/sd/record_4.wav")
                player.volume(100)

                # read audio info
                wav_info = player.play_process(i2s)
                # print("wav file head information: ", wav_info)
                i2s.set_sample_rate(wav_info[1])
                # print('loop to play audio')
                while True:
                    ret = player.play()
                    if ret == None:
                        # print("format error")
                        break
                    elif ret == 0:
                        break
                player.finish()

            del i2s, player

            #img = image.Image(size=(240, 240))
            #hist_width = 1 #changeable
            #x_shift = 0
            #for i in range(100):
                #temp = i2s.record(1024)
                #time.sleep_ms(10)
                #fft_res = FFT.run(temp.to_bytes(), 512)
                #fft_amp = FFT.amplitude(fft_res)
                ##print(len(fft_amp))
                ##print(fft_amp)
                #img = img.clear()
                #x_shift = 0
                #for i in range(512):
                    #hist_height = fft_amp[i]
                    #img = img.draw_rectangle((x_shift, 0, 1, hist_height), [255,255,255], 1, True)
                    ##print((x_shift, 0, 1, hist_height))
                    #x_shift = x_shift + 1
                #lcd.display(img)
                #fft_amp.clear()

            #del img, i2s
            #time.sleep_ms(2000)

        finally:
            pass


