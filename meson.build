project('infinitton', 'c',
  default_options: ['std=c11']
)

add_global_arguments('-D_XOPEN_SOURCE=600', language: 'c')

inc = include_directories('include')

subdir('include')
subdir('src')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  libraries: infinittonlib,
  version: '1.0',
  name: 'libinfinitton',
  description: 'A library for controlling the Infinitton button pad'
)

