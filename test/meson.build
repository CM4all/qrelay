python3 = find_program('python3', required: get_option('test'))
if not python3.found()
  subdir_done()
endif

test('test', python3, args: [files('test.py'), meson.build_root()])
