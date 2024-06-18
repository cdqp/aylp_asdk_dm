anyloop plugin for ALPAO DMs
============================

Linux only for now. Mainly focused on the eth interface.


Installing
----------

### libaylp dependency

Symlink or copy the `libaylp` directory from anyloop to `libaylp`. For example:

```sh
ln -s $HOME/git/anyloop/libaylp libaylp
```

### libasdk dependency

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

### Building

Use meson:

```sh
meson setup build
meson compile -C build
```


aylp_asdk_dm.so
---------------

Types and units: `[AYLP_T_VECTOR|AYLP_T_MATRIX, AYLP_U_MINMAX|AYLP_U_RAD] ->
[T_UNCHANGED, U_UNCHANGED]`.

This device controls [ALPAO][alpao] brand deformable mirrors using their
proprietary SDK. The fastest way to use it is to provide it with an
`AYLP_T_VECTOR` with the same length as the number of actuators in your
deformable mirror, and under the units `AYLP_U_MINMAX` where +1 means maximum
stroke and -1 means minimum stroke. However, the device is also capable of
taking a matrix as input and indexing into it based on a map of where each
actuator is. Also, units in radians are accepted and converted into equivalent
stroke values with a cost to performance.

This device does not currently clamp the mirror strokes to allowed values (±1)!
My experience with testing has been that if one tells the mirror to exceed these
bounds, it will simply not move.

For an example configuration, which uses a deformable mirror to apply a phase
screen simulating atmospheric turbulence, see [conf_test.json][conf_test].

### Parameters

- `sn` (string) (required)
  - Serial number of the mirror you are trying to manipulate. Appropriate (ALPAO
    style) config files must also exist in the current directory. For example,
    for serial number BAX472, files named "BAX472" and "BAX472.acfg" must exist
    in the working directory. The former is a file in a proprietary binary
    format that ALPAO distributes; the latter is simply a text file in an
    undocumented but fairly straightforward format.
- `peak_per_rad` (float) (optional)
  - The fraction of a full stroke represented by one radian of phase at the
    target wavelength. Only used if input is `AYLP_U_RAD`. Defaults to 0.0.
- `mat_is` (array of integers) (required if input is `AYLP_T_MATRIX`)
  - This is a one-dimensional array with length matching the number of
    actuators in the mirror. It provides the (0-indexed) row index for each
    actuator to look at in the pipeline matrix. For example, if there are four
    actuators, and `mat_is` is [0, 0, 1, 1], then the first two actuators will
    by controlled by the first row of the pipeline matrix and the last two
    actuators will be controlled by the second row.
- `mat_js` (array of integers) (required if input is `AYLP_T_MATRIX`)
  - This is a one-dimensional array with length matching the number of
    actuators in the mirror. It provides the (0-indexed) column index for each
    actuator to look at in the pipeline matrix. For example, if there are four
    actuators, and `mat_js` is [0, 1, 0, 1], then the first and third actuators
    will by controlled by the first column of the pipeline matrix and the second
    and fourth two actuators will be controlled by the second column.



[alpao]: https://www.alpao.com
[conf_test]: contrib/conf_test.json

