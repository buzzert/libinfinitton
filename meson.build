project('infinitton', 'c',
  default_options: ['c_std=c11']
)

inc = include_directories('include')

subdir('include')
subdir('src')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  libraries: infinittonlib,
  version: '1.0',
  name: 'liblevitton',
  description: 'A library for controlling the Levitton button pad'
)

