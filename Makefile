all : freertos.elf

CFLAGS = \
	-I brewbot \
	-I brewbot/drivers \
	-I brewbot/include \
	-I brewbot/Common/include \
	-I brewbot/network-apps \
	-I brewbot/network-apps/webserver \
	-I brewbot/Common/ethernet/FreeTCPIP \
	-I Source/include \
	-I Source/portable/GCC/RX600 \
	$(END)


CFILES_ENET = \
	brewbot/GNU-Files/start.asm \
	brewbot/GNU-Files/hwinit.c \
	brewbot/GNU-Files/inthandler.c \
	brewbot/Renesas-Files/hwsetup.c \
	Source/list.c \
	Source/portable/GCC/RX600/port.c \
	Source/portable/MemMang/heap_2.c \
	Source/queue.c \
	Source/tasks.c \
	brewbot/Common/ethernet/FreeTCPIP/apps/httpd/http-strings.c \
	brewbot/Common/ethernet/FreeTCPIP/apps/httpd/httpd-fs.c \
	brewbot/Common/ethernet/FreeTCPIP/apps/httpd/httpd.c \
	brewbot/Common/ethernet/FreeTCPIP/psock.c \
	brewbot/Common/ethernet/FreeTCPIP/timer.c \
	brewbot/Common/ethernet/FreeTCPIP/uip.c \
	brewbot/Common/ethernet/FreeTCPIP/uip_arp.c \
	brewbot/network-apps/webserver/EMAC.c \
	brewbot/network-apps/webserver/httpd-cgi.c \
	brewbot/network-apps/webserver/phy.c \
	brewbot/network-apps/memb.c \
	brewbot/network-apps/ftpd.c \
	brewbot/network-apps/telnetd.c \
	brewbot/network-apps/shell.c \
	brewbot/network-apps/uIP_Task.c \
	brewbot/drivers/ds1820.c \
	brewbot/drivers/spi.c \
	brewbot/drivers/p5q.c \
	brewbot/drivers/lcd.c \
	brewbot/drivers/font_x5x7.c \
	brewbot/drivers/vects.c \
	brewbot/fatfs/ff.c \
	brewbot/fatfs/diskio.c \
	brewbot/hop_droppers.c \
	brewbot/crane.c \
	brewbot/menu.c \
	brewbot/fill.c \
	brewbot/diagnostics.c \
	brewbot/heat.c \
	brewbot/brew.c \
	brewbot/buttons.c \
	brewbot/level_probes.c \
	brewbot/brew_task.c \
	brewbot/settings.c \
	brewbot/main-full.c

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
