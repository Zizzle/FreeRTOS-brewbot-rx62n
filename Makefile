all : freertos.elf

CFLAGS = \
	-I brewbot/Common/include \
	-I brewbot/webserver \
	-I brewbot \
	-I brewbot/include \
	-I brewbot/Common/ethernet/FreeTCPIP \
	-I Source/include \
	-I Source/portable/GCC/RX600 \
	$(END)


CFILES_ENET = \
	brewbot/GNU-Files/start.asm \
	brewbot/Common/ethernet/FreeTCPIP/apps/httpd/http-strings.c \
	brewbot/Common/ethernet/FreeTCPIP/apps/httpd/httpd-fs.c \
	brewbot/Common/ethernet/FreeTCPIP/apps/httpd/httpd.c \
	brewbot/Common/ethernet/FreeTCPIP/psock.c \
	brewbot/Common/ethernet/FreeTCPIP/timer.c \
	brewbot/Common/ethernet/FreeTCPIP/uip.c \
	brewbot/Common/ethernet/FreeTCPIP/uip_arp.c \
	brewbot/GNU-Files/hwinit.c \
	brewbot/GNU-Files/inthandler.c \
	brewbot/hop_droppers.c \
	brewbot/ParTest.c \
	brewbot/Renesas-Files/hwsetup.c \
	brewbot/main-full.c \
	brewbot/uIP_Task.c \
	brewbot/vects.c \
	brewbot/webserver/EMAC.c \
	brewbot/webserver/httpd-cgi.c \
	brewbot/webserver/phy.c \
	brewbot/lcd.c \
	brewbot/font_x5x7.c \
	Source/list.c \
	Source/portable/GCC/RX600/port.c \
	Source/portable/MemMang/heap_2.c \
	Source/queue.c \
	Source/tasks.c \
	brewbot/crane-x.c \
	brewbot/ds1820.c

CFILES = \
	brewbot/GNU-Files/start.asm \
	brewbot/main-blinky.c \
	brewbot/ParTest.c \
	brewbot/vects.c \
	Source/list.c \
	Source/queue.c \
	Source/tasks.c \
	Source/portable/MemMang/heap_2.c \
	Source/portable/GCC/RX600/port.c \
	brewbot/GNU-Files/hwinit.c \
	brewbot/GNU-Files/inthandler.c \
	brewbot/Renesas-Files/hwsetup.c \
	$(END)

OFILES := $(addsuffix .o,$(basename $(CFILES_ENET)))

freertos.elf : $(OFILES)
	rx-elf-gcc -nostartfiles $(OFILES) -o freertos.elf -T RTOSDemo_Blinky_auto.gsi
	rx-elf-size freertos.elf

%.o : %.c
	rx-elf-gcc -Wall -c $(CFLAGS) -Os $< -o $@

%.o : %.S
	rx-elf-gcc -x assembler-with-cpp -c $(CFLAGS) -O2 $< -o $@

%.o : %.asm
	rx-elf-gcc -x assembler-with-cpp -c $(CFLAGS) -O2 $< -o $@

flash : freertos.elf
	sudo rxusb -v freertos.elf

clean :
	rm -f $(OFILES) freertos.elf
