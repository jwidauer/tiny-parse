project('tiny-parse', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17'])

# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = ['-DBUILDING_TINY_PARSE']

shlib = shared_library('tiny_parse', 'tiny_parse.hpp',
  install : true,
  cpp_args : lib_args,
  gnu_symbol_visibility : 'hidden',
)

test_exe = executable('tiny_parse_test', 'tiny_parse_test.cpp',
  link_with : shlib)
test('tiny_parse', test_exe)

# Make this library usable as a Meson subproject.
tiny_parse_dep = declare_dependency(
  include_directories: include_directories('.'),
  link_with : shlib)

# Make this library usable from the system's
# package manager.
install_headers('tiny_parse.hpp', subdir : 'tiny_parse')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'tiny-parse',
  filebase : 'tiny_parse',
  description : 'Meson sample project.',
  subdirs : 'tiny_parse',
  libraries : shlib,
  version : '0.1',
)
