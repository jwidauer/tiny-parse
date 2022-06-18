project('tiny-parse', 'cpp',
  version : '0.1',
  default_options : ['warning_level=3', 'cpp_std=c++17'])

# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = ['-DBUILDING_TINY_PARSE']

include_dir = include_directories('include')

subdir('include/tiny_parse')

# Make this library usable as a Meson subproject.
tiny_parse = declare_dependency(include_directories: include_dir)

subdir('test')
subdir('examples')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  name : 'tiny-parse',
  filebase : 'tiny_parse',
  description : 'A tiny library for building string parsers.',
  subdirs : 'tiny_parse',
  version : '0.1',
)
