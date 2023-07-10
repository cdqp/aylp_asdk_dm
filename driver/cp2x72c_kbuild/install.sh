#!/bin/bash

###################### Alpao PEX292144 Driver installation #####################

################################################################################
# Install Interface Corp. drivers.                                             #
# Simply use: sudo bash install.sh                                               #
################################################################################

## Requirements
# To build kernel module and utilities, you need the following tools:
# RedHat/CentOS/Fedora:
#   yum install kernel-devel
#   yum groupinstall "Development Tools"
# Debian/Ubuntu/Mint:
#   apt-get install linux-headers-$(uname -r) # or linux-headers-generic
#   apt-get install build-essential

## Fail if script is not being run by root
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

MACHINE_TYPE=`uname -m`
DESTINATION=/usr/src/interface_alpao
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

echo "Installation of Interface Corp. drivers for Alpao drive electronics"
echo $MACHINE_TYPE
echo

echo "Copy source and binaries"

# Create destination folders
mkdir -p $DESTINATION/ /etc/interface/

# Copy kernel module sources
cp -r $DIR/modules $DESTINATION/

# Copy utilities
cp -r $DIR/extra/util $DESTINATION/
if [ ${MACHINE_TYPE} == 'x86_64' ]; then
        cp -f $DIR/extra/diobminsmod_x64 $DESTINATION/diobminsmod
else
        cp -f $DIR/extra/diobminsmod $DESTINATION/diobminsmod
fi

chmod +x $DESTINATION/diobminsmod

# Copy INF file
cp $DIR/extra/gpg2x72c.inf /etc/interface/

# Copy libraries
cp $DIR/extra/lib/libgpg2x72c.so.2.2.5 $DIR/extra/lib/libgpgconf.so.1.5.6 /usr/lib
ldconfig -n /usr/lib

# Create symbolic link
rm -f /usr/lib/libgpg2x72c.so
ln -s /usr/lib/libgpg2x72c.so.1 /usr/lib/libgpg2x72c.so

rm -f /usr/lib/libgpgconf.so
ln -s /usr/lib/libgpgconf.so.1 /usr/lib/libgpgconf.so

LIBPATH="/usr/lib64"
if [ -d "/usr/lib/x86_64-linux-gnu" ]; then
        LIBPATH="/usr/lib/x86_64-linux-gnu"
fi

# If /usr/lib64 exist, do the same for 64-bit libraries
if [ -d ${LIBPATH} ]; then
        # Copy libraries
        cp $DIR/extra/lib64/libgpg2x72c.so.2.2.5 $DIR/extra/lib64/libgpgconf.so.1.5.6 $LIBPATH
        ldconfig -n $LIBPATH

        # Create symbolic link
        rm -f $LIBPATH/libgpg2x72c.so
        ln -s $LIBPATH/libgpg2x72c.so.1 $LIBPATH/libgpg2x72c.so

        rm -f $LIBPATH/libgpgconf.so
        ln -s $LIBPATH/libgpgconf.so.1 $LIBPATH/libgpgconf.so
fi

# Unload modules (if exist)
rmmod cp2x72c dpg0101 dpg0100 > /dev/null 2>&1

# Build and install kernel modules (kbuild dpg0100/dpg0101/cp2x72c)
echo "Build and install Interface Corp. kernel modules"
make -C $DESTINATION/modules all install > /dev/null
/sbin/depmod -a # Update module list

# Build utility
echo "Build configuration utility"
make -C $DESTINATION/util > /dev/null

# Load modules
echo "Load kernel modules"
/sbin/modprobe cp2x72c
/sbin/modprobe dpg0101
$DESTINATION/diobminsmod # If fail, dpg0101 will return "another error"

# Build /etc/interface/gpg2x72c.conf (use -q to disable output)
echo "Create configuration file gpg2x72c.conf"
$DESTINATION/util/dpg0101 -s 2x72c

# Define rules to load kernel module with PEX board
# for LPC/PEX-292144 (VEN_1147/DEV_0B69)
#cat > /lib/udev/rules.d/85-cp2x72c.rules <<EOF
#
#EOF

if command -v udevadm &>/dev/null
then
    udevadm control --reload-rules
fi
