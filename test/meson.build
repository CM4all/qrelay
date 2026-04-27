gtest = dependency('gtest', main: true,
                   include_type: 'system',
                   disabler: true,
                   required: get_option('test'),
                   fallback: ['gtest', 'gtest_main_dep'])

test(
  'TestDjb',
  executable(
    'TestDjb',
    'TestNetstringParser.cxx',
    'TestQmqpMail.cxx',
    '../src/djb/NetstringParser.cxx',
    '../src/djb/QmqpMail.cxx',
    include_directories: inc,
    install: false,
    dependencies: [
      util_dep,
      uri_dep,
      gtest,
    ],
  ),
  env: ['TZ=CET'],
)

python3 = find_program('python3',
                       disabler: true,
                       required: get_option('test'))

test('test', python3, args: [files('test.py'), meson.build_root()])
