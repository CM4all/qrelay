project('CM4all qrelay', 'cpp', version: '0.5',
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
  '-Werror', '-pedantic-errors',
  language: 'cpp')

boost = dependency('boost', modules: ['program_options'], version: '>= 1.54')
liblua = dependency('luajit')
libevent = dependency('libevent', version: '>= 2.0.19')
libsystemd = dependency('libsystemd')
libinline = dependency('libcm4all-inline', version: '>= 0.3.4')
libsocket = dependency('libcm4all-socket', version: '>= 0.3.5')

inc = include_directories('src')

util = static_library('util',
  'src/util/HugeAllocator.cxx',
  'src/util/StringUtil.cxx',
  'src/util/StringView.cxx',
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
  'src/event/Loop.cxx',
  'src/event/ShutdownListener.cxx',
  'src/event/DeferEvent.cxx',
  'src/event/SignalEvent.cxx',
  include_directories: inc,
  dependencies: [
    libinline,
    declare_dependency(link_with: util),
  ])

net = static_library('net',
  'src/net/SocketAddress.cxx',
  'src/net/StaticSocketAddress.cxx',
  'src/net/AllocatedSocketAddress.cxx',
  'src/net/Resolver.cxx',
  'src/net/ServerSocket.cxx',
  'src/net/ConnectSocket.cxx',
  'src/net/SocketDescriptor.cxx',
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
  'src/system/SetupProcess.cxx',
  'src/CgroupProc.cxx',
  'src/MountProc.cxx',
  'src/Instance.cxx',
  'src/Mail.cxx',
  'src/LMail.cxx',
  'src/LAction.cxx',
  'src/LAddress.cxx',
  'src/QmqpRelayConnection.cxx',
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
