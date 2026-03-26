PS2SDK    ?= /root/ps2dev/ps2sdk
PS2SDKSRC ?= /root/ps2sdk-src

export PS2SDK
export PS2SDKSRC

IOP_BIN  = libsdpcm.irx
IOP_OBJS = libsdpcm_irx.o imports.o exports.o

IOP_CFLAGS += -G0 -O2 -Wall -Wno-unused-function -Wno-unused-variable

IOP_INCS += \
	-I$(PS2SDK)/iop/include \
	-I$(PS2SDK)/common/include

IOP_IMPORT_INCS += \
	system/loadcore \
	system/threadman \
	system/stdio \
	system/sifcmd \
	system/sifrpc

IOP_INCS += \
        -I$(PS2SDKSRC)/iop/sound/audsrv/src

IOP_INCS += \
        -I$(PS2SDKSRC)/iop/sound/audsrv/src

include $(PS2SDKSRC)/Defs.make
include $(PS2SDKSRC)/iop/Rules.bin.make
include $(PS2SDKSRC)/iop/Rules.make
include $(PS2SDKSRC)/iop/Rules.release
