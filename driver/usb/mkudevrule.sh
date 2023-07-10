cat > /etc/udev/rules.d/73-beaglebone.rules <<EOF
ACTION=="add", SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_interface", \
        ATTRS{idVendor}=="0403", ATTRS{idProduct}=="a6d0", \
        DRIVER=="", RUN+="/sbin/modprobe -b ftdi_sio"

ACTION=="add", SUBSYSTEM=="drivers", \
        ENV{DEVPATH}=="/bus/usb-serial/drivers/ftdi_sio", \
        ATTR{new_id}="0403 a6d0"

ACTION=="add", KERNEL=="ttyUSB*", \
	ATTRS{interface}=="BeagleBone", \
        ATTRS{bInterfaceNumber}=="00", \
	SYMLINK+="beaglebone-jtag"

ACTION=="add", KERNEL=="ttyUSB*", \
	ATTRS{interface}=="BeagleBone", \
        ATTRS{bInterfaceNumber}=="01", \
	SYMLINK+="beaglebone-serial"
EOF

if command -v udevadm &>/dev/null;
then
    udevadm control --reload-rules
fi
echo "You must restart your computer to validate changes."
