Anyloop plugin for ALPAO DMs
============================

Linux only for now. Mainly focused on the eth interface.

libaylp dependency
------------------

Symlink or copy the `libaylp` directory from anyloop to `libaylp`. For example:

```sh
ln -s $HOME/git/anyloop/libaylp libaylp
```

libasdk dependency
------------------

Download the Alpao SDK from <https://alpao.fr/Download/AlpaoSDK> and extract the
necessary files to configure a `libasdk` directory as follows:

```
cdqp git/aylp_asdk_dm % tree libasdk 
libasdk
├── acedev5.h
├── asdkDM.h
├── asdkErrNo.h
├── asdkMultiDM.h
├── asdkType.h
├── asdkWrapper.h
├── libait_eth.so
├── libait_gbit.so
├── libait_pci7200.so
├── libait_pci7300.so
├── libait_pci7350.so
├── libait_pex292144.so
├── libait_sim.so
└── libasdk.so

1 directory, 14 files
```

Then, when running *anyloop* with this plugin, make sure you include `libasdk`
in your `LD_LIBRARY_PATH`.

Note also that your Alpao config files must be in the current working directory
when you run *anyloop*.


Building
--------

Use meson:

```sh
meson setup build
meson compile -C build
```

