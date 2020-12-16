project('CM4all qrelay', 'cpp', version: '0.13',
  meson_version: '>= 0.47',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3',
  ],
)

# TODO: use get_option('b_ndebug'), but that doesn't work with Meson 0.37.1
if get_option('buildtype') == 'debug'
  debug = true
else
  debug = false
endif

compiler = meson.get_compiler('cpp')

common_flags = [
  '-D_GNU_SOURCE',
  '-Wwrite-strings', '-Wcast-qual', '-Wcast-align', '-Wfloat-equal',
  '-Wshadow', '-Wpointer-arith', '-Wsign-compare',
  '-Wmissing-declarations', '-Wmissing-noreturn', '-Wmissing-format-attribute',
  '-Wredundant-decls', '-Wno-long-long', '-Wundef',
  '-Wunused',
]

test_common_flags = [
  '-fvisibility=hidden',
]

if not debug
  test_common_flags += [
    '-ffunction-sections',
    '-fdata-sections',
  ]

  add_global_link_arguments(
    compiler.get_supported_link_arguments(
      '-Wl,--gc-sections',
      '-Wl,--icf=all',
    ),
    language: 'cpp'
  )
endif

test_cxxflags = test_common_flags + [
  '-fno-threadsafe-statics',
  '-fmerge-all-constants',

  '-Wno-non-virtual-dtor',
]

add_global_arguments(common_flags, language: 'cpp')
add_global_arguments(compiler.get_supported_arguments(test_cxxflags), language: 'cpp')

if debug
  common_flags += ['-DPOISON']
endif

boost_program_options = dependency('boost', modules: ['program_options'], version: '>= 1.54')
libsystemd = dependency('libsystemd')

inc = include_directories(
  'src',
  'libcommon/src',
)

libcommon_enable_c = false
libcommon_enable_DefaultFifoBuffer = false
libcommon_enable_net_log = false

subdir('libcommon/src/util')
subdir('libcommon/src/lua')
subdir('libcommon/src/io')
subdir('libcommon/src/system')
subdir('libcommon/src/event')
subdir('libcommon/src/net')
subdir('libcommon/src/event/net')

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
    boost_program_options, libsystemd,
    liblua,
    util_dep,
    lua_dep,
    event_net_dep,
  ],
  install: true,
  install_dir: 'sbin',
)

subdir('doc')
