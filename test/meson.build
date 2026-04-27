gtest = dependency('gtest', main: true,
                   include_type: 'system',
                   disabler: true,
                   required: get_option('test'),
                   fallback: ['gtest', 'gtest_main_dep'])

python3 = find_program('python3',
                       disabler: true,
                       required: get_option('test'))

test('test', python3, args: [files('test.py'), meson.build_root()])
