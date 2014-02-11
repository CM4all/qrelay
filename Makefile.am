AUTOMAKE_OPTIONS = foreign 1.11 subdir-objects

AM_CPPFLAGS += \
	-I$(srcdir)/src

bin_PROGRAMS = src/cm4all-qrelay
src_cm4all_qrelay_SOURCES = \
	src/Config.hxx \
	src/CommandLine.cxx src/CommandLine.hxx \
	src/Event.cxx src/Event.hxx \
	src/net/ServerSocket.cxx src/net/ServerSocket.hxx \
	src/djb/NetstringError.cxx src/djb/NetstringError.hxx \
	src/djb/NetstringServer.cxx src/djb/NetstringServer.hxx \
	src/djb/NetstringInput.cxx src/djb/NetstringInput.hxx \
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
