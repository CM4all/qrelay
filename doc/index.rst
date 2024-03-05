qrelay
======

What is qrelay?
---------------

qrelay is a QMQP server that receives emails on a socket and forwards
them to another QMQP server.


Configuration
-------------

.. highlight:: lua

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


``SIGHUP``
^^^^^^^^^^

On ``systemctl reload cm4all-qrelay`` (i.e. ``SIGHUP``), qrelay
calls the Lua function ``reload`` if one was defined.  It is up to the
Lua script to define the exact meaning of this feature.


Global Variables
^^^^^^^^^^^^^^^^

* ``max_size`` is the maximum size of an incoming email in bytes (used
  only during startup).  There must be a reasonable upper limit to
  avoid consuming too many resources.  The default value is
  ``16777216`` (``16 MiB``).


Inspecting Incoming Mail
^^^^^^^^^^^^^^^^^^^^^^^^

The following attributes can be queried:

* :samp:`sender`: The sender envelope address.  This attribute can
  also be written to.

* :samp:`recipients`: A list of recipient envelope addresses.

* :samp:`pid`: The client's process id.

* :samp:`uid`: The client's user id.

* :samp:`gid`: The client's group id.

* :samp:`cgroup`: The control group of the client process with the
  following attributes:

  * ``path``: the cgroup path as noted in :file:`/proc/self/cgroup`,
    e.g. :file:`/user.slice/user-1000.slice/session-42.scope`

  * ``xattr``: A table containing extended attributes of the
    control group.

  * ``parent``: Information about the parent of this cgroup; it is
    another object of this type (or ``nil`` if there is no parent
    cgroup).

Manipulating the Mail Object
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The method `insert_header(NAME, VALUE)` inserts a new MIME header at
the front of the email.  Example::

  m:insert_header('X-Cgroup', m.cgroup)

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

  The last parameter may be a table specifying options:

  - ``env``: a table with environment variables for the child process.

* :samp:`exec_raw("PROGRAM", "ARG", ...)`: Execute the program and
  submit the raw email message (headers and body, but no envelope) on
  standard input.  Translates the exit status to either "accepted (K)"
  (``EXIT_SUCCESS``), "temporary failure (Z)" or "permanent failure
  (D)" and read an error message from standard output/error.  This can
  be used to launch programs which implement the
  ``/usr/sbin/sendmail`` interface instead of QMQP.  Since the
  envelope is not submitted, the caller should translate the envelope
  to command-line arguments.

  The last parameter may be a table specifying options (the same as
  for ``exec()``).

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


libsodium
^^^^^^^^^

There are some `libsodium <https://www.libsodium.org/>`__ bindings.

`Sealed boxes
<https://libsodium.gitbook.io/doc/public-key_cryptography/sealed_boxes>`__::

  pk, sk = sodium.crypto_box_keypair()
  ciphertext = sodium.crypto_box_seal('hello world', pk)
  message = sodium.crypto_box_seal_open(ciphertext, pk, sk)

`Point*scalar multiplication
<https://doc.libsodium.org/advanced/scalar_multiplication>__::

  pk = sodium.crypto_scalarmult_base(sk)


PostgreSQL Client
^^^^^^^^^^^^^^^^^

The Lua script can query a PostgreSQL database.  First, a connection
should be established during initialization::

  db = pg:new('dbname=foo', 'schemaname')

In the handler function, queries can be executed like this (the API is
similar to `LuaSQL <https://keplerproject.github.io/luasql/>`__)::

  local result = assert(db:execute('SELECT id, name FROM bar'))
  local row = result:fetch({}, "a")
  print(row.id, row.name)

Query parameters are passed to ``db:execute()`` as an array after the
SQL string::

  local result = assert(
    db:execute('SELECT name FROM bar WHERE id=$1', {42}))

The functions ``pg:encode_array()`` and ``pg:decode_array()`` support
PostgreSQL arrays; the former encodes a Lua array to a PostgreSQL
array string, and the latter decodes a PostgreSQL array string to a
Lua array.

To listen for `PostgreSQL notifications
<https://www.postgresql.org/docs/current/sql-notify.html>`__, invoke
the ``listen`` method with a callback function::

  db:listen('bar', function()
    print("Received a PostgreSQL NOTIFY")
  end)


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
