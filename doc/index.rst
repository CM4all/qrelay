qrelay
======

What is qrelay?
---------------

qrelay is a QMQP server that receives emails on a socket and forwards
them to another QMQP server.


Configuration
-------------

The file :file:`/etc/cm4all/qrelay/config.lua` is a `Lua
<http://www.lua.org/>`_ script which is executed at startup.  It
contains at least one :samp:`qmqp_listen()` call, for example::

  qmqp_listen('/run/cm4all/qrelay/qrelay.sock', function(m)
    return m:connect('192.168.1.99')
  end)

The first parameter is the socket path to listen on.  Passing the
global variable :envvar:`systemd` (not the string literal
:samp:`"systemd"`) will listen on the sockets passed by systemd::

  qmqp_listen(systemd, function(m) ...

To use this socket from within a container, move it to a dedicated
directory and bind-mount this directory into the container.  Mounting
just the socket doesn't work because a daemon restart must create a
new socket, but the bind mount cannot be refreshed.

The second parameter is a callback function which shall decide what to
do with an incoming email.  This function receives a mail object which
can be inspected.  Multiple listeners can share the same handler by
declaring the function explicitly::

  function handler(m)
    return m:reject()
  end

  qmqp_listen('/foo', handler)
  qmqp_listen('/bar', handler)

It is important that the function finishes quickly.  It must never
block, because this would block the whole daemon process.  This means
it must not do any network I/O, launch child processes, and should
avoid anything but querying the email's parameters.


Inspecting Incoming Mail
^^^^^^^^^^^^^^^^^^^^^^^^

The following attributes can be queried:

* :samp:`sender`: The sender envelope address.

* :samp:`recipients`: A list of recipient envelope addresses.

* :samp:`pid`: The client's process id.

* :samp:`uid`: The client's user id.

* :samp:`gid`: The client's group id.

The following methods can access more data:

* :samp:`get_cgroup('CONTROLLERNAME')`: Obtain the cgroup membership
  path for the given controller.

* :samp:`get_mount_info('MOUNTPOINT')`: Obtain information about a
  mount point in the client's filesystem namespace.  The return value
  is :samp:`nil` if the given path is not a mount point, or a table
  containing the items :envvar:`root` (root of the mount within the
  filesystem), :envvar:`filesystem` (the name of the filesystem) and
  :envvar:`source` (the device which is mounted here)

Manipulating the Mail Object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The method `insert_header(NAME, VALUE)` inserts a new MIME header at
the front of the email.  Example::

  m:insert_header('X-Cgroup', m:get_cgroup('cpuacct'))

Both name and value must conform to RFC 2822 2.2.


Actions
^^^^^^^

The handler function shall return an object describing what to do with
the email.  The mail object contains several methods which create such
action objects; they do not actually perform the action.

The following actions are possible:

* :samp:`connect("ADDRESS")`: Connect to this address and relay the
  email via QMQP.  The address is either a string containing a (numeric)
  IP address, or an `address` object created by `qmqp_resolve()`.

* :samp:`exec("PROGRAM", "ARG", ...)`: Execute the program and submit
  the email via QMQP on standard input.  Read the QMQP response from
  standard output.

* :samp:`discard()`: Discard the email, pretending delivery was successful.

* :samp:`reject()`: Reject the email with a permanent error.


Addresses
^^^^^^^^^

It is recommended to create all `address` objects during startup, to
avoid putting unnecessary pressure on the Lua garbage collector, and
to reduce the overhead for invoking the system resolver (which blocks
qrelay execution).  The function `qmqp_resolve()` creates such an
`address` object::

  server1 = qmqp_resolve('192.168.0.2')
  server2 = qmqp_resolve('[::1]:4321')
  server3 = qmqp_resolve('server1.local:1234')
  server4 = qmqp_resolve('/run/server5.sock')
  server5 = qmqp_resolve('@server4')

These examples do the following:

- convert a numeric IPv4 address to an `address` object (port defaults
  to 628, the QMQP standard port)
- convert a numeric IPv6 address with a non-standard port to an
  `address` object
- invoke the system resolver to resolve a host name to an IP address
  (which blocks qrelay startup; not recommended)
- convert a path string to a "local" socket address
- convert a name to an abstract "local" socket address (prefix '@' is
  converted to a null byte, making the address "abstract")


Examples
^^^^^^^^

TODO


About Lua
^^^^^^^^^

`Programming in Lua <https://www.lua.org/pil/1.html>`_ (a tutorial
book), `Lua 5.3 Reference Manual <https://www.lua.org/manual/5.3/>`_.

Note that in Lua, attributes are referenced with a dot
(e.g. :samp:`m.sender`), but methods are referenced with a colon
(e.g. :samp:`m:reject()`).
