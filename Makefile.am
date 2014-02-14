AUTOMAKE_OPTIONS = foreign 1.11 subdir-objects

AM_CPPFLAGS += \
	-I$(srcdir)/src

bin_PROGRAMS = src/cm4all-qrelay
src_cm4all_qrelay_SOURCES = \
	src/Config.hxx \
	src/CommandLine.cxx src/CommandLine.hxx \
	src/Event.cxx src/Event.hxx \
	src/net/Error.cxx src/net/Error.hxx \
	src/net/SocketAddress.cxx src/net/SocketAddress.hxx \
	src/net/ServerSocket.cxx src/net/ServerSocket.hxx \
	src/net/TemplateServerSocket.hxx \
	src/net/ConnectSocket.cxx src/net/ConnectSocket.hxx \
	src/net/SendBuffer.cxx src/net/SendBuffer.hxx \
	src/djb/NetstringError.cxx src/djb/NetstringError.hxx \
	src/djb/NetstringServer.cxx src/djb/NetstringServer.hxx \
	src/djb/NetstringClient.cxx src/djb/NetstringClient.hxx \
	src/djb/NetstringInput.cxx src/djb/NetstringInput.hxx \
	src/djb/NetstringOutput.cxx src/djb/NetstringOutput.hxx \
	src/util/Range.hxx \
	src/util/ConstBuffer.hxx \
	src/util/Error.cxx src/util/Error.hxx \
	src/util/HugeAllocator.cxx src/util/HugeAllocator.hxx \
	src/Instance.hxx \
	src/Logger.hxx \
	src/QmqpRelayServer.hxx \
	src/QmqpRelayConnection.cxx src/QmqpRelayConnection.hxx \
	src/Main.cxx
src_cm4all_qrelay_CPPFLAGS = $(AM_CPPFLAGS) \
	$(LIBCM4ALL_INLINE_CFLAGS) \
	$(LIBCM4ALL_DAEMON_CFLAGS) \
	$(LIBEVENT_CFLAGS)
src_cm4all_qrelay_LDADD = $(AM_LDADD) \
	-lboost_program_options \
	$(LIBCM4ALL_DAEMON_LIBS) \
	$(LIBEVENT_LIBS)
