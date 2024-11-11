qrelay
======

qrelay is a QMQP server that receives emails on a socket and forwards
them to another QMQP server.

For more information, `read the manual
<https://qrelay.readthedocs.io/en/latest/>`__ in the ``doc``
directory.


Building qrelay
---------------

You need:

- Linux kernel 5.4 or newer
- a C++20 compliant compiler (e.g. gcc or clang)
- `libfmt <https://fmt.dev/>`__
- `LuaJIT <http://luajit.org/>`__
- `Meson 1.0 <http://mesonbuild.com/>`__ and `Ninja <https://ninja-build.org/>`__

Optional dependencies:

- `libpq <https://www.postgresql.org/>`__ for PostgreSQL support in
  Lua code
- `libsodium <https://www.libsodium.org/>`__
- `systemd <https://www.freedesktop.org/wiki/Software/systemd/>`__

Get the source code::

 git clone --recursive https://github.com/CM4all/qrelay.git

Run ``meson``::

 meson setup output

Compile and install::

 ninja -C output
 ninja -C output install


Building the Debian package
---------------------------

After installing the build dependencies, run::

 dpkg-buildpackage -rfakeroot -b -uc -us
