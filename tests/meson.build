doctest_dep = dependency('doctest')

test_exe = executable('tiny_parse_test', 'tiny_parse_test.cpp',
  dependencies: [tiny_parse, doctest_dep])

test('tiny_parse', test_exe)
