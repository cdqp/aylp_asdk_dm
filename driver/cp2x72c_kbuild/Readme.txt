===============================================================================
 Alpao SDK - GPH-2X72C drivers for PEX-292144
===============================================================================

That package present another way to build GPH-2X72C software from Interface
Corporation (JP), little modication have been made on module sources to be
compatible with newer Kernel.

Based on GPG/GPH-2X72C version 2.30.19 for CPU x86_64 and i386.

No prebuild version of kernel modules are provided, you need kernel
headers and toolchain to build modules.

Main purpose of those modifications is to unofficially support new kernel
revision such as version 3.1 or above.


Note from Interface Corp.:
 >> Power Management Capability
       The GPH-2X72C does not support power management functions. When power 
       management events such as standby, suspend, hibernation, and resume 
       occur, you may not successfully access to the device. Configure your 
       system as described below to disable the power management capability.
        - Disable the power management of your system on your BIOS settings. 
          Refer to the user's manual of your system for more details. 
        - Disable APM (Advanced Power Management) by setting the kernel option 
          as follows. 
                 append="apm=off"
          Stop the "apmd" daemon using the following command. 
                 # /etc/rc.d/init.d/apmd stop

        If ACPI(Advanced Configuration and Power Interface) is used, set the 
        kernel option as follows to disable ACPI.
                  append="acpi=off"
        Stop the "acpid" daemon using the following command. 
                  # /etc/rc.d/init.d/acpid stop

Incompatibility: 
 >> Kernel 4.8 or above compiled with "CONFIG_HARDENED_USERCOPY"
        The GPH-2X72C is not compatible with Kernel 4.8 or above when compiled
        using "CONFIG_HARDENED_USERCOPY=y". Kernel modules will build and load,
        but Kernel oops occured when used by user space application like SDK 
        or dpg0101. This problem caused CONFIG_HARDENED_USERCOPY to erroneously
        halt the system, issuing reports of the form:
            [DPG0101]: usercopy: kernel memory exposure attempt detected from 
             ffff8807e8f47a28 (<process stack>) (44 bytes)
             
            [SDK]: usercopy: kernel memory exposure attempt detected from 
             ffff88078da679d0 (<process stack>) (8 bytes)
            
        After a call to "gpg_diobm_ioctl" -> "gpg_copy_to_user" -> "copy_to_user"