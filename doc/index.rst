qrelay
======

What is qrelay?
---------------

qrelay is a QMQP server that receives emails on a socket and forwards
them to another QMQP server.


Configuration
-------------

The file :file:`/etc/cm4all/qrelay/config.lua` is a Lua script which
is executed at startup.  It contains at least one
:samp:`qmqp_listen()` call, for example::

  qmqp_listen('/run/cm4all/qrelay/qrelay.sock', function(m)
    return m:connect('192.168.1.99')
  end)

The first parameter is the socket path to listen on.

To use this socket from within a container, move it to a dedicated
directory and bind-mount this directory into the container.  Mounting
just the socket doesn't work because a daemon restart must create a
new socket, but the bind mount cannot be refreshed.

The second parameter is a callback function which shall decide what to
do with an incoming email.  This function receives a mail object which
can be inspected.

The following attributes can be queried:

* :samp:`sender`: The sender envelope address.

* :samp:`recipients`: A list of recipient envelope addresses.

* :samp:`pid`: The client's process id.

* :samp:`uid`: The client's user id.

* :samp:`gid`: The client's group id.

The following actions are possible:

* :samp:`connect("ADDRESS")`: Connect to this address and relay the
  email via QMQP.

* :samp:`exec("PROGRAM", "ARG", ...)`: Execute the program and submit
  the email via QMQP on standard input.  Read the QMQP response from
  standard output.

* :samp:`discard()`: Discard the email, pretending delivery was successful.

* :samp:`reject*(`: Reject the email with a permanent error.
