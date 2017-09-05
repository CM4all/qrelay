project('CM4all qrelay', 'cpp', version: '0.10',
  default_options : ['cpp_std=c++14'])

# TODO: use get_option('b_ndebug'), but that doesn't work with Meson 0.37.1
if get_option('buildtype') == 'debug'
  debug = true
else
  debug = false
endif

common_flags = [
  '-D_GNU_SOURCE',
  '-Wall',
  '-Wextra',
  '-Wwrite-strings', '-Wcast-qual', '-Wcast-align', '-Wfloat-equal',
  '-Wshadow', '-Wpointer-arith', '-Wsign-compare',
  '-Wmissing-declarations', '-Wmissing-noreturn', '-Wmissing-format-attribute',
  '-Wredundant-decls', '-Wno-long-long', '-Wundef',
  '-Wunused',
]

add_global_arguments(common_flags,
  '-Wno-non-virtual-dtor',
  language: 'cpp')

if debug
  common_flags += ['-DPOISON']
endif

boost = dependency('boost', modules: ['program_options'], version: '>= 1.54')
liblua = dependency('luajit')
libevent = dependency('libevent', version: '>= 2.0.19')
libsystemd = dependency('libsystemd')

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
  include_directories: inc,
  dependencies: [
  ])
util_dep = declare_dependency(link_with: util)

lua = static_library('lua',
  'libcommon/src/lua/Error.cxx',
  'libcommon/src/lua/RunFile.cxx',
  'libcommon/src/lua/State.cxx',
  dependencies: [
    liblua,
  ]
)
lua_dep = declare_dependency(link_with: lua)

event = static_library('event',
  'libcommon/src/event/Loop.cxx',
  'libcommon/src/event/ShutdownListener.cxx',
  'libcommon/src/event/CleanupTimer.cxx',
  'libcommon/src/event/DeferEvent.cxx',
  'libcommon/src/event/SignalEvent.cxx',
  include_directories: inc,
  dependencies: [
    libevent,
    util_dep,
  ])
event_dep = declare_dependency(link_with: event)

io = static_library('io',
  'libcommon/src/io/FileDescriptor.cxx',
  'libcommon/src/io/WriteBuffer.cxx',
  'libcommon/src/io/MultiWriteBuffer.cxx',
  include_directories: inc,
  dependencies: [
  ])
io_dep = declare_dependency(link_with: io)

net = static_library('net',
  'libcommon/src/net/SocketAddress.cxx',
  'libcommon/src/net/StaticSocketAddress.cxx',
  'libcommon/src/net/AllocatedSocketAddress.cxx',
  'libcommon/src/net/IPv4Address.cxx',
  'libcommon/src/net/IPv6Address.cxx',
  'libcommon/src/net/HostParser.cxx',
  'libcommon/src/net/AddressInfo.cxx',
  'libcommon/src/net/Resolver.cxx',
  'libcommon/src/net/Parser.cxx',
  'libcommon/src/net/ToString.cxx',
  'libcommon/src/net/ServerSocket.cxx',
  'libcommon/src/net/ConnectSocket.cxx',
  'libcommon/src/net/SocketDescriptor.cxx',
  'libcommon/src/net/UniqueSocketDescriptor.cxx',
  'libcommon/src/net/djb/NetstringServer.cxx',
  'libcommon/src/net/djb/NetstringClient.cxx',
  'libcommon/src/net/djb/NetstringInput.cxx',
  'libcommon/src/net/djb/NetstringHeader.cxx',
  'libcommon/src/net/djb/NetstringGenerator.cxx',
  include_directories: inc,
  dependencies: [
    declare_dependency(link_with: event),
    io_dep,
  ])
net_dep = declare_dependency(link_with: net)

executable('cm4all-qrelay',
  'src/CommandLine.cxx',
  'src/djb/NetstringParser.cxx',
  'src/djb/QmqpMail.cxx',
  'src/system/SetupProcess.cxx',
  'src/CgroupProc.cxx',
  'src/MountProc.cxx',
  'src/Instance.cxx',
  'src/MutableMail.cxx',
  'src/LMail.cxx',
  'src/LAction.cxx',
  'src/LAddress.cxx',
  'src/LResolver.cxx',
  'src/Connection.cxx',
  'src/Main.cxx',
  include_directories: inc,
  dependencies: [
    boost, libevent, libsystemd,
    liblua,
    util_dep,
    lua_dep,
    net_dep,
    event_dep,
  ])
