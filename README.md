Anyloop plugin for ALPAO DMs
============================

Linux only for now. Mainly focused on the eth interface.

Building
--------

Symlink or copy the `libaylp` directory from anyloop to `libaylp`. For example:

```sh
ln -s $HOME/git/anyloop/libaylp libaylp
```

Then use meson:

```sh
meson setup build
meson compile -C build
```

