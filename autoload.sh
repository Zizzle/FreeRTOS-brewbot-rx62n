#!/bin/bash

unset DISPLAY

function wait_for_plugin()
{
    MLINES=`wc -l < /var/log/messages` 

    while true; do
        tail -n +$MLINES /var/log/messages | grep "New USB device found, idVendor=045b, idProduct=0025"
        if [ "$?" = "0" ]; then
                        break
        fi
	sleep 1
    done
}

while true; do
    wait_for_plugin
    play /usr/share/sounds/purple/receive.wav
    make flash
    play /usr/lib/openoffice.org/basis3.2/share/gallery/sounds/theetone.wav
    sleep 2
done
