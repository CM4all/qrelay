qrelay
======

qrelay is a QMQP server that receives emails on a socket and forwards
them to another QMQP server.

For more information, read the manual in the `doc` directory.


Building qrelay
---------------

You need:

- a C++20 compliant compiler (e.g. gcc or clang)
- `LuaJIT <http://luajit.org/>`__
- `D-Bus <https://www.freedesktop.org/wiki/Software/dbus/>`__
- `systemd <https://www.freedesktop.org/wiki/Software/systemd/>`__
- `Meson 0.47 <http://mesonbuild.com/>`__ and `Ninja <https://ninja-build.org/>`__

Get the source code::

 git clone --recursive https://github.com/CM4all/qrelay.git

Run ``meson``::

 meson . output

Compile and install::

 ninja -C output
 ninja -C output install


Building the Debian package
---------------------------

After installing the build dependencies, run::

 dpkg-buildpackage -rfakeroot -b -uc -us
