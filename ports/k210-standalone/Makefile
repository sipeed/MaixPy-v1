.PHONY: clean_all build packfs pack patch_py recover_py
include ../../py/mkenv.mk
CROSS_COMPILE ?= 
LIB_PATH = $(dir $(CROSS_COMPILE))
SDK_PATH ?= kendryte-standalone-sdk
MPY_MOD_PATH ?= mpy-mod
FS_PORT_PATH ?= spiffs-port
BOARD_DRIVERS_PATH ?= board-drivers

# qstr definitions (must come before including py.mk)
QSTR_DEFS = qstrdefsport.h

FROZEN_MPY_DIR ?= builtin-py

# include py core make definitionsvers/uarths.c
include $(TOP)/py/py.mk

# mpy header
INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -Iconfig
INC += -Iinclude
INC += -I$(TOP)/lib/timeutils

# mpy mod header
INC += -I$(MPY_MOD_PATH)/machine/include
INC += -I$(MPY_MOD_PATH)/uos/include

# board drivers header
INC += -I$(BOARD_DRIVERS_PATH)/include

# spiffs ports header
INC += -Ispiffs-port/include

# sdk header
INC += -I$(SDK_PATH)/lib/bsp/include
INC += -I$(SDK_PATH)/lib/drivers/include
INC += -I$(SDK_PATH)/lib/utils/include
INC += -I$(SDK_PATH)/lib/math

# spiffs header
INC += -Ispiffs/src
INC += -Ispiffs/src/default

# compile option
C_DEFINES ?=
C_DEFINES += -DCONFIG_LOG_COLORS \
			 -DCONFIG_LOG_ENABLE \
			 -DCONFIG_LOG_LEVEL=LOG_VERBOSE \
			 -DDEBUG=1 \
			 -DFPGA_PLL \
			 -DLOG_KERNEL \
			 -D__riscv64

ARCH_FLAGS = -march=rv64imafdc -mabi=lp64d

BOTH_FLAGS = -mcmodel=medany \
			 -fno-common \
			 -ffunction-sections \
			 -fdata-sections \
			 -fstrict-volatile-bitfields \
			 -fno-zero-initialized-in-bss \
			 -Os \
			 -ggdb \
			 -Wall \
			 -Werror=all \
			 -Wno-error=unused-function \
			 -Wno-error=unused-but-set-variable \
			 -Wno-error=unused-variable \
			 -Wno-error=deprecated-declarations \
			 -Wextra \
			 -Werror=frame-larger-than=65536 \
			 -Wno-unused-parameter \
			 -Wno-sign-compare \
			 -Wno-error=missing-braces \
			 -Wno-error=return-type \
			 -Wno-error=pointer-sign \
			 -Wno-missing-braces \
			 -Wno-pointer-to-int-cast \
			 -Wno-strict-aliasing \
			 -Wno-implicit-fallthrough \
			 -Linclude/

CFLAGS_RV64_K210 = -lm \
				   -mfdiv \
				   -mdiv \
				   -std=gnu11 \
				   -Wno-old-style-declaration \
				   -g \
				   $(C_DEFINES) \
				   $(BOTH_FLAGS)

CFLAGS = $(INC) \
		 -std=gnu11 \
		 $(CFLAGS_RV64_K210) \
		 -g -Wno-error=unused-label \
		 -Wno-error=unused-const-variable= \
		 -Wno-error=format= \
		 -Wno-error=parentheses


CFLAGS_MOD += $(C_DEFINES)
CFLAGS += $(CFLAGS_MOD)

LDPPFLAGS += \
	-DDUSE_BINUTILS_2_19=1
LDFLAGS += --gc-sections

LDFLAGS = -lm $(BOTH_FLAGS) -T $(SDK_PATH)/lds/kendryte.ld \
        -nostartfiles \
        -static \
        -Wl,--gc-sections \
        -Wl,-static \
        -Wl,--start-group \
        -Wl,--whole-archive \
        -Wl,--no-whole-archive \
        -Wl,--end-group \
        -Wl,-EL

LIBS =

MODULES_SRC_C= \
		$(MPY_MOD_PATH)/uos/moduos.c \
		$(MPY_MOD_PATH)/machine/modmachine.c \
		$(MPY_MOD_PATH)/machine/machine_uarths.c \
		$(MPY_MOD_PATH)/machine/machine_uart.c \
		$(MPY_MOD_PATH)/machine/machine_pin.c \
		$(MPY_MOD_PATH)/machine/machine_pwm.c \
		$(MPY_MOD_PATH)/machine/machine_timer.c \
		$(MPY_MOD_PATH)/machine/machine_st7789.c \
		$(MPY_MOD_PATH)/machine/machine_ov2640.c \
		$(MPY_MOD_PATH)/machine/machine_burner.c \
		$(MPY_MOD_PATH)/machine/machine_face_detect.c \
		$(MPY_MOD_PATH)/machine/machine_spiflash.c \
		$(MPY_MOD_PATH)/machine/machine_zmodem.c \
		$(MPY_MOD_PATH)/machine/machine_fpioa.c \
		$(MPY_MOD_PATH)/machine/machine_ws2812.c \
		$(MPY_MOD_PATH)/machine/machine_test.c \
		$(MPY_MOD_PATH)/machine/machine_devmem.c\
		$(MPY_MOD_PATH)/machine/machine_esp8285.c\
		$(MPY_MOD_PATH)/socket/modsocket.c


LIB_SRC_C ?=
MICROPY_FATFS=1
ifeq ($(MICROPY_FATFS), 1)
LIB_SRC_C += \
lib/oofatfs/ff.c \
lib/oofatfs/option/unicode.c
endif

FS_SRC_C = \
		$(FS_PORT_PATH)/spiffs-port.c \
		spiffs/src/spiffs_cache.c \
		spiffs/src/spiffs_check.c \
		spiffs/src/spiffs_gc.c \
		spiffs/src/spiffs_hydrogen.c \
		spiffs/src/spiffs_nucleus.c \
		file_io.c

BOARD_DRIVERS = $(BOARD_DRIVERS_PATH)/lcd.c \
				$(BOARD_DRIVERS_PATH)/st7789.c \
				$(BOARD_DRIVERS_PATH)/ov2640.c \
				$(BOARD_DRIVERS_PATH)/w25qxx.c \
				$(BOARD_DRIVERS_PATH)/face_detect.c \
				$(BOARD_DRIVERS_PATH)/region_layer.c \
				$(BOARD_DRIVERS_PATH)/uart_core.c \
				$(BOARD_DRIVERS_PATH)/ws2812b.c \
				$(BOARD_DRIVERS_PATH)/esp8285.c

MPY_LIB_SRC = lib/timeutils/timeutils.c \
			  lib/utils/sys_stdio_mphal.c

SRC_C = \
		$(MODULES_SRC_C)\
		main.c \
		mphalport.c \
		modutime.c \
		$(SDK_PATH)/lib/bsp/entry.c \
		$(SDK_PATH)/lib/bsp/entry_user.c \
		$(SDK_PATH)/lib/bsp/interrupt.c \
		$(SDK_PATH)/lib/bsp/printf.c \
		$(SDK_PATH)/lib/bsp/sleep.c \
		$(SDK_PATH)/lib/bsp/syscalls.c \
		$(SDK_PATH)/lib/drivers/uarths.c \
		$(SDK_PATH)/lib/math/fastexp.c \
		$(SDK_PATH)/lib/drivers/aes.c \
		$(SDK_PATH)/lib/drivers/clint.c \
		$(SDK_PATH)/lib/drivers/common.c \
		$(SDK_PATH)/lib/drivers/dmac.c \
		$(SDK_PATH)/lib/drivers/dvp.c \
		$(SDK_PATH)/lib/drivers/fft.c \
		$(SDK_PATH)/lib/drivers/fpioa.c \
		$(SDK_PATH)/lib/drivers/gpio.c \
		$(SDK_PATH)/lib/drivers/gpiohs.c \
		$(SDK_PATH)/lib/drivers/i2c.c \
		$(SDK_PATH)/lib/drivers/i2s.c \
		$(SDK_PATH)/lib/drivers/plic.c \
		$(SDK_PATH)/lib/drivers/pwm.c \
		$(SDK_PATH)/lib/drivers/rtc.c \
		$(SDK_PATH)/lib/drivers/sha256.c \
		$(SDK_PATH)/lib/drivers/spi.c \
		$(SDK_PATH)/lib/drivers/sysclock.c \
		$(SDK_PATH)/lib/drivers/sysctl.c \
		$(SDK_PATH)/lib/drivers/timer.c \
		$(SDK_PATH)/lib/drivers/uart.c \
		$(SDK_PATH)/lib/drivers/wdt.c \
		$(BOARD_DRIVERS) \
		$(MPY_LIB_SRC) \
		$(FS_SRC_C)



PY_SRC_C = \
        help.c \
        lib/utils/stdout_helpers.c \
		lib/utils/interrupt_char.c \
		lib/utils/pyexec.c \
		lib/libc/string0.c \
		lib/mp-readline/readline.c \
		lib/netutils/netutils.c
	
ASM_SRCS = $(SDK_PATH)/lib/bsp/crt.S

# List of sources for qstr extraction
SRC_QSTR += $(SRC_C)  
# Append any auto-generated sources that are needed by sources listed in SRC_QSTR
SRC_QSTR_AUTO_DEPS +=

OBJ = $(PY_O) $(addprefix $(BUILD)/, $(SRC_C:.c=.o)) $(addprefix $(BUILD)/, $(PY_SRC_C:.c=.o)) $(addprefix $(BUILD)/, $(ASM_SRCS:.S=.o))

$(BUILD)/_frozen_mpy.c: frozentest.mpy $(BUILD)/genhdr/qstrdefs.generated.h
	$(ECHO) "MISC freezing bytecode"
	$(Q)$(TOP)/tools/mpy-tool.py -f -q $(BUILD)/genhdr/qstrdefs.preprocessed.h -mlongint-impl=none $< > $@

all:export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(LIB_PATH)
all: 
	make micropython.bin 

micropython: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC)  -o $@ $(OBJ) $(LIBS) $(LDFLAGS) $(CFLAGS)
	$(Q)$(SIZE) $@

micropython.bin: micropython
	$(OBJCOPY) --output-format=binary micropython micropython.bin
	cp micropython.bin bin_file

clean_all:  clean
			rm -rf micropython micropython.bin
			make -C ../../mpy-cross clean
			rm -rf ./bin_file/micropython.bin
			rm -rf ./bin_file/fs.bin
			rm -rf ./bin_file/firmware.bin
			rm -rf ./sipeedm1.kfpkg
build:
	rm -rf kendryte-standalone-sdk kendryte-standalone-sdk-0.4.1 v0.4.1.zip
	wget https://github.com/kendryte/kendryte-standalone-sdk/archive/v0.4.1.zip
	unzip v0.4.1.zip
	rm -rf v0.4.1.zip
	mv kendryte-standalone-sdk-0.4.1 kendryte-standalone-sdk
	cd kendryte-standalone-sdk && patch -p1 -u < ../sdk.patch && cd -

packfs:
	rm -rf fs.bin
	make -C pack/mkfs/mkspiffs/ clean && make -C pack/mkfs/mkspiffs/ all
	pack/mkfs/mkspiffs/mkspiffs -c fs_path/ fs.bin
	mv fs.bin bin_file

pack_bin:   packfs	
	pack/pack.sh

pack_kfpkg: packfs
	pack/pack_zip.sh

include $(TOP)/py/mkrules.mk


