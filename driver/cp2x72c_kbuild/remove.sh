######################## Alpao PEX292144 Driver remove #########################

################################################################################
# Uninstall Interface Corp. drivers.                                           #
# Simply use: sudo sh remove.sh                                                #
################################################################################


# Unload modules (if exist)
rmmod cp2x72c dpg0101 dpg0100 > /dev/null 2>&1

# Remove kernel modules
rm -rf /lib/modules/`uname -r`/extra/cp2x72c
rm -rf /lib/modules/`uname -r`/extra/dpg0101
rm -rf /lib/modules/`uname -r`/extra/dpg0100

rm -f /etc/sysconfig/modules/2x72c.modules

# Remove sources directory
rm -rf /usr/src/interface_alpao

# Remove configuration files
rm -f /etc/interface/gpg2x72c.*

# Remove libraries
rm -f /usr/lib/libgpg2x72c.so* /usr/lib/libgpgconf.so.1*
rm -f /usr/lib64/libgpg2x72c.so* /usr/lib64/libgpgconf.so.1*
rm -f /usr/lib/x86_64-linux-gnu/libgpg2x72c.so* /usr/lib/x86_64-linux-gnu/libgpgconf.so.1*

# Update cache
ldconfig