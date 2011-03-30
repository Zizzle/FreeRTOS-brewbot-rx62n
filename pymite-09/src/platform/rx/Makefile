# PyMite Configuration
PLATFORM := $(notdir $(CURDIR))
PM_LIB_ROOT = pmvm_$(PLATFORM)
PM_LIB_FN = lib$(PM_LIB_ROOT).a
PM_LIB_PATH = ../../vm/$(PM_LIB_FN)
PM_USR_SOURCES = main.py
PM_HEAP_SIZE = 0x2000
PMIMGCREATOR := ../../tools/pmImgCreator.py
PMGENPMFEATURES := ../../tools/pmGenPmFeatures.py
IPM = true
DEBUG = true

SHELL = sh
CC = rx-elf-gcc
OBJCOPY = rx-elf-objcopy
OBJDUMP = rx-elf-objdump
SIZE = rx-elf-size
AR = rx-elf-ar
NM = rx-elf-nm


TARGET = main
SOURCES = $(TARGET).c plat.c $(TARGET)_nat.c $(TARGET)_img.c plat_io_stubs.c
OBJS = $(SOURCES:.c=.o)

ifeq ($(DEBUG),true)
	CDEFS += -g -ggdb -D__DEBUG__=1
endif
CINCS = -I$(abspath .)
CFLAGS = -Os -Wall -gstabs -fno-strict-aliasing -Wstrict-prototypes \
         -Wdeclaration-after-statement -Werror -I../../vm $(CDEFS) $(CINCS)

export CC OBJCOPY NM CFLAGS ALL_CFLAGS AR IPM PM_LIB_FN


.PHONY: all clean

all : pmfeatures.h $(TARGET).out

$(PM_LIB_PATH) : ../../vm/*.c ../../vm/*.h
	make -C ../../vm

$(TARGET).out : $(OBJS) $(PM_LIB_PATH)
	$(CC) -o $@ $(OBJS) $(PM_LIB_PATH) -lm

pmfeatures.h : pmfeatures.py $(PMGENPMFEATURES)
	$(PMGENPMFEATURES) pmfeatures.py > $@

# Generate native code and module images from the python source
$(TARGET)_nat.c $(TARGET)_img.c: $(PM_USR_SOURCES) pmfeatures.py
	$(PMIMGCREATOR) -f pmfeatures.py -c -u -o $(TARGET)_img.c --native-file=$(TARGET)_nat.c $(PM_USR_SOURCES)

clean :
	$(MAKE) -C ../../vm clean
	$(RM) $(TARGET).out $(OBJS) $(TARGET)_img.* $(TARGET)_nat.* pmfeatures.h

# Compile: create object files from C source files.
.c.o:
	$(CC) -c $(CFLAGS) $< -o $@ 


# Compile: create assembler files from C source files.
.c.s:
	$(CC) -S $(CFLAGS) $< -o $@


# Assemble: create object files from assembler source files.
.S.o:
	$(CC) -c $(ASFLAGS) $< -o $@
