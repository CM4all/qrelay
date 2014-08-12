ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign 1.11 subdir-objects

AM_CPPFLAGS += \
	-I$(srcdir)/src

bin_PROGRAMS = src/cm4all-qrelay
src_cm4all_qrelay_SOURCES = \
	src/Config.cxx src/Config.hxx \
	src/CommandLine.cxx src/CommandLine.hxx \
	src/io/WriteBuffer.cxx src/io/WriteBuffer.hxx \
	src/io/MultiWriteBuffer.cxx src/io/MultiWriteBuffer.hxx \
	src/djb/NetstringError.cxx src/djb/NetstringError.hxx \
	src/djb/NetstringParser.cxx src/djb/NetstringParser.hxx \
	src/djb/NetstringServer.cxx src/djb/NetstringServer.hxx \
	src/djb/NetstringClient.cxx src/djb/NetstringClient.hxx \
	src/djb/NetstringInput.cxx src/djb/NetstringInput.hxx \
	src/djb/NetstringGenerator.cxx src/djb/NetstringGenerator.hxx \
	src/io/TextFile.cxx src/io/TextFile.hxx \
	src/Instance.hxx \
	src/Logger.hxx \
	src/Mail.cxx src/Mail.hxx \
	src/QmqpRelayServer.hxx \
	src/QmqpRelayConnection.cxx src/QmqpRelayConnection.hxx \
	src/Main.cxx
src_cm4all_qrelay_CPPFLAGS = $(AM_CPPFLAGS) \
	$(LIBCM4ALL_INLINE_CFLAGS) \
	$(LIBCM4ALL_SOCKET_CFLAGS) \
	$(LIBCM4ALL_DAEMON_CFLAGS) \
	$(LIBEVENT_CFLAGS)
src_cm4all_qrelay_LDADD = $(AM_LDADD) \
	libevent.a libnet.a libutil.a \
	$(BOOST_PROGRAM_OPTIONS_LIB) \
	$(LIBCM4ALL_SOCKET_LIBS) \
	$(LIBCM4ALL_DAEMON_LIBS) \
	$(LIBEVENT_LIBS)

noinst_LIBRARIES = libutil.a libnet.a libevent.a

libutil_a_SOURCES = \
	src/util/BindMethod.hxx \
	src/util/CharUtil.hxx \
	src/util/StringUtil.cxx src/util/StringUtil.hxx \
	src/util/Tokenizer.cxx src/util/Tokenizer.hxx \
	src/util/Range.hxx \
	src/util/ConstBuffer.hxx \
	src/util/Domain.hxx \
	src/util/Error.cxx src/util/Error.hxx \
	src/util/HugeAllocator.cxx src/util/HugeAllocator.hxx
libutil_a_CPPFLAGS = $(AM_CPPFLAGS) \
	$(LIBCM4ALL_INLINE_CFLAGS)

libnet_a_SOURCES = \
	src/net/Error.cxx src/net/Error.hxx \
	src/net/SocketAddress.cxx src/net/SocketAddress.hxx \
	src/net/StaticSocketAddress.cxx src/net/StaticSocketAddress.hxx \
	src/net/ServerSocket.cxx src/net/ServerSocket.hxx \
	src/net/TemplateServerSocket.hxx \
	src/net/ConnectSocket.cxx src/net/ConnectSocket.hxx \
	src/net/SocketDescriptor.cxx src/net/SocketDescriptor.hxx
libnet_a_CPPFLAGS = $(AM_CPPFLAGS) \
	$(LIBCM4ALL_SOCKET_CFLAGS) \
	$(LIBCM4ALL_DAEMON_CFLAGS) \
	$(LIBCM4ALL_INLINE_CFLAGS)

libevent_a_SOURCES = \
	src/event/Loop.cxx src/event/Loop.hxx \
	src/event/Callback.hxx \
	src/event/Duration.hxx \
	src/event/Event.hxx \
	src/event/DeferEvent.cxx src/event/DeferEvent.hxx \
	src/event/SignalEvent.hxx \
	src/event/TimerEvent.hxx
libevent_a_CPPFLAGS = $(AM_CPPFLAGS) \
	$(LIBCM4ALL_INLINE_CFLAGS) \
	$(LIBEVENT_CFLAGS)

#
# documentation
#

MOSTLYCLEANFILES = doc/qrelay.html

if DOCUMENTATION
doc/qrelay.html: doc/qrelay.xml
	$(XMLTO) -o $(@D) html-nochunks $<

all-local: doc/qrelay.html

doc_DATA = doc/qrelay.html
endif
