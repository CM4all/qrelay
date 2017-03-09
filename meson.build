project('CM4all qrelay', 'cpp', version: '0.6',
  default_options : ['cpp_std=c++14'])

add_global_arguments('-D_REENTRANT', '-D_GNU_SOURCE',
  '-Wall',
  '-Wextra',
  '-Wwrite-strings', '-Wcast-qual', '-Wcast-align', '-Wfloat-equal',
  '-Wshadow', '-Wpointer-arith', '-Wsign-compare',
  '-Wmissing-declarations', '-Wmissing-noreturn', '-Wmissing-format-attribute',
  '-Wredundant-decls', '-Wno-long-long', '-Wundef',
  '-Wunused',
  '-pedantic',
  '-Wno-non-virtual-dtor',
  language: 'cpp')

boost = dependency('boost', modules: ['program_options'], version: '>= 1.54')
liblua = dependency('luajit')
libevent = dependency('libevent', version: '>= 2.0.19')
libsystemd = dependency('libsystemd')
libinline = dependency('libcm4all-inline', version: '>= 0.3.4')
libsocket = dependency('libcm4all-socket', version: '>= 0.3.5')

inc = include_directories('src', 'libcommon/src')

util = static_library('util',
  'libcommon/src/util/AllocatedString.cxx',
  'libcommon/src/util/Exception.cxx',
  'libcommon/src/util/LeakDetector.cxx',
  'libcommon/src/util/PrintException.cxx',
  'libcommon/src/util/StringBuilder.cxx',
  'libcommon/src/util/StringParser.cxx',
  'libcommon/src/util/StringUtil.cxx',
  'libcommon/src/util/StringView.cxx',
  dependencies: [
    libinline
  ])

lua = static_library('lua',
  'src/lua/Error.cxx',
  'src/lua/RunFile.cxx',
  'src/lua/State.cxx',
  dependencies: [
    liblua,
  ]
)

event = static_library('event',
  'libcommon/src/event/Loop.cxx',
  'libcommon/src/event/ShutdownListener.cxx',
  'libcommon/src/event/CleanupTimer.cxx',
  'libcommon/src/event/DeferEvent.cxx',
  'libcommon/src/event/SignalEvent.cxx',
  include_directories: inc,
  dependencies: [
    libinline,
    libevent,
    declare_dependency(link_with: util),
  ])

net = static_library('net',
  'libcommon/src/net/SocketAddress.cxx',
  'libcommon/src/net/StaticSocketAddress.cxx',
  'libcommon/src/net/AllocatedSocketAddress.cxx',
  'libcommon/src/net/Resolver.cxx',
  'src/net/ServerSocket.cxx',
  'src/net/ConnectSocket.cxx',
  'libcommon/src/net/UniqueSocketDescriptor.cxx',
  include_directories: inc,
  dependencies: [
    libinline, libsocket,
    declare_dependency(link_with: event),
  ])

executable('cm4all-qrelay',
  'src/CommandLine.cxx',
  'src/io/WriteBuffer.cxx',
  'src/io/MultiWriteBuffer.cxx',
  'src/djb/NetstringParser.cxx',
  'src/djb/NetstringServer.cxx',
  'src/djb/NetstringClient.cxx',
  'src/djb/NetstringInput.cxx',
  'src/djb/NetstringGenerator.cxx',
  'src/djb/QmqpMail.cxx',
  'src/system/SetupProcess.cxx',
  'src/CgroupProc.cxx',
  'src/MountProc.cxx',
  'src/Instance.cxx',
  'src/MutableMail.cxx',
  'src/LMail.cxx',
  'src/LAction.cxx',
  'src/LAddress.cxx',
  'src/Connection.cxx',
  'src/Main.cxx',
  include_directories: inc,
  dependencies: [
    boost, libevent, libsystemd,
    liblua,
    libinline, libsocket,
    declare_dependency(link_with: util),
    declare_dependency(link_with: lua),
    declare_dependency(link_with: net),
    declare_dependency(link_with: event),
  ])
