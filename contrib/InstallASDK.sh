#!/bin/bash

################################################################################
################################ Alpao Project #################################
################################################################################
# This script let you install the Alpao libraries and headers in systems       #
# folders (/usr/local/*).                                                      #
# You can automatically install drivers for Adlinks and Interface cards or USB.#
# Simply use: sudo bash InstallASDK.sh                                           #
################################################################################

##	  Requirements
#	To properly install ALPAO Sdk and the drivers for the PEX-292144 interface
#	You need to make sure that all the following packages are present on the machine
#
#   apt-get install linux-headers-$(uname -r) # or linux-headers-generic
#   apt-get install build-essential
#	apt-get install linux-source
#
#	You also need to make sure that the kernel you use meets the following requirements
#		- If kernel vesion is superior than 4.8, it needs to be compiled with 
#		"CONFIG_HARDENED_USERCOPY=no"
#
#	Also, please note that the kernel module of the PEX is not yet signed so if enabled
#	you need to disable "Secure boot" in motherboard bios.
#
#	The minimum requirement to use the ALPAO Sdk and compile C or C++ programs
#	are the following : 
#	    GLIBC    - 2.2.5
#    	GLIBCXX  - 3.4
#   	GCC      - 3.0
##

################################ CHECK and MENU ################################
## Fail if script is not being run by root
if [ "$(id -u)" != "0" ]; then
   echo "This script must be run as root" 1>&2
   exit 1
fi

## Retrieve current directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LIB32="/usr/lib"
LIB64="/usr/lib64"
if [ -d "/usr/lib/x86_64-linux-gnu" ]; then
    LIB64="/usr/lib/x86_64-linux-gnu"
fi

## Select installation mode
echo
echo "Select the installation type you want."
PS3='Please enter your choice: '
options=("Install only ASDK libraries" "Install ASDK and Interface Corp. PEX-292144 support" "Install ASDK and Adlink card support" "Install ASDK and USB support" "Quit")
select opt in "${options[@]}"
do
    case $opt in
        "Install only ASDK libraries")
            echo "Only ASDK will be installed"
            mode=1;
            break;
            ;;
        "Install ASDK and Interface Corp. PEX-292144 support")
            mode=4;
            break
            ;;
        "Install ASDK and Adlink card support")
            echo
            mode=2;
            PS3='Please choose the Adlink card: '
            options=("PCI7200" "PCI7300" "PCI7350")
            select opt in "${options[@]}"
            do
                case $opt in
                    "PCI7200") card="7200";break;;
                    "PCI7300") card="7300";break;;
                    "PCI7350") card="7350";break;;
                    *) echo invalid option;;
                esac
            done
            break
            ;;
        "Install ASDK and USB support")
            mode=3;
            break
            ;;
        "Quit")
            exit
            ;;
        *) echo invalid option;;
    esac
done

################################## INSTALLER ###################################
## General Alpao SDK libraries installation

# Copy shared libraries
cp $DIR/Lib/x86/*.so $LIB32
chmod 644 $LIB32/libait_*.so $LIB32/libasdk.so
ldconfig -n $LIB32
if [ -d ${LIB64} ]; then
    cp $DIR/Lib/x64/*.so $LIB64
    chmod 644 $LIB64/libait_*.so $LIB64/libasdk.so
    ldconfig -n $LIB64
fi

# Copy headers
cp $DIR/../Include/*.h /usr/include
chmod 644 /usr/include/acedev5.h /usr/include/asdk*.h

## Drivers
case $mode in
    2)
    # Adlink installation
    cp $DIR/Drivers/pcis_dask/lib/libpci_dask.so $LIB32
    if [ -d ${LIB64} ]; then
        cp $DIR/Drivers/pcis_dask/lib/libpci_dask64.so $LIB64
    fi

    kern=$(uname -r)
    if [ -d "$DIR/Drivers/pcis_dask/drivers/$kern" ]
    then
        pdask_path=/lib/modules/$kern/kernel/drivers/pdask

        # Make module directory for pcis-dask
        mkdir -p $pdask_path

        # Copy modules and update list
        cp $DIR/Drivers/pcis_dask/drivers/$kern/p$card.ko $pdask_path
        cp $DIR/Drivers/pcis_dask/drivers/$kern/adl_mem_mgr.ko $pdask_path
        /sbin/depmod -a

        # Launch module at boot with correct parameters
        if [ -d "/etc/sysconfig/modules/" ]
        then
            cd /etc/sysconfig/modules/
            touch pdask.modules
            chmod 755 pdask.modules
            echo "#!/bin/sh
/sbin/insmod p$card" > pdask.modules
        else
            if ! grep -c "p$card" /etc/modules ; then
                echo "p$card" >> /etc/modules
            fi
        fi

        # Copy UDEV script to update device links
        cp $DIR/Drivers/pcis_dask/drivers/udev-rules/pci$card.rules /etc/udev/rules.d/60-pci$card.rules

        # -- Memory reservation
        memsize=$(vmstat -s | head -1 | awk '{print $1}');
        pagesz=$(getconf PAGESIZE);
        memio=$(expr $pagesz \* 256 / 1024 );
        memres=$(expr $memsize \* 1024 - $memio \* 2048);
        memrmo=$(expr $memres / 1024 / 1024);
        echo 'GRUB_CMDLINE_LINUX="mem='$memres'"'
        
        echo "options p$card BufSize=0,0,$memio,$memio" > /etc/modprobe.d/pdask.conf

        echo "Your version of Grub is:"
        /sbin/grub-install -v
        echo "The minimum reserved memory you needed is $memrmo Mio."
        echo "Please manually do the following."
        echo "1 - If you use Grub2:"
        echo '  Append: GRUB_CMDLINE_LINUX="mem='$memres'"'
        echo "  in file /etc/default/grub"
        echo "  And valid the changes with command: update-grub"
        echo "2 - If you use Legacy Grub:"
        echo "  Edit /etc/grub/conf and append mem=$memres at the end of the 'kernel' line used."
        echo "  e.g.: kernel /vmlinuz-2.6 ro mem=$memres"

        echo "-> And restart your computer"

    else
        echo "Your kernel revision is not yet supported by Adlink drivers"
        echo "Please contact Alpao or Adlink for further details"
    fi
    ;;
    
    3)
    # USB to Ethernet installation
    echo
    sh $DIR/Drivers/usb/mkudevrule.sh
    ;;
    
    4)
    # Interface Corp. PEX-292144 installation
    echo
    bash $DIR/Drivers/cp2x72c_kbuild/install.sh
    ;;
esac

echo "All done."

##################################### ~O~ ######################################
