qrelay
======

What is qrelay?
---------------

qrelay is a QMQP server that receives emails on a socket and forwards
them to another QMQP server.


Configuration
-------------

The file :file:`/etc/cm4all/qrelay/qrelay.conf` contains the qrelay
configuration.  It should start with a :envvar:`listen` directive that
tells qrelay which socket path to listen on.
Example::

  listen "/var/run/cm4all/qrelay.sock"

To use this socket from within a container, move it to a dedicated
directory and bind-mount this directory into the container.  Mounting
just the socket doesn't work because a daemon restart must create a
new socket, but the bind mount cannot be refreshed.

The next line contains the keyword :envvar:`default` which specifies
what to do with incoming emails.  It is followed by one of the
following actions:

* :samp:`connect "ADDRESS"`: Connect to this address and relay the
  email via QMQP.

* :samp:`exec "PROGRAM" "ARG ..."`: Execute the program and submit the
  email via QMQP on standard input.  Read the QMQP response from
  standard output.

* :samp:`discard`: Discard the email, pretending delivery was successful.

* :samp:`reject`: Reject the email with a permanent error.

The :envvar:`rule` keyword introduces a rule for routing emails to
other destinations based on the configured condition.  Syntax::

  rule CONDITION ACTION

Where condition is:

* :samp:`exclusive_recipient "ADDRESS"`: The envelope recipient is
  this address.  If there is more than one recipient, all of them must
  match, otherwise qrelay aborts delivery permanently.
