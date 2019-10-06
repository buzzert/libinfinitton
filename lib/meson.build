project('infinitton', 'c',
  default_options: ['c_std=c11']
)

deps = [
  dependency('hidapi-libusb'),
]

srcs = [
  'src/device.c',
  'src/pixmap.c',
]

infinitton_lib = static_library(
  meson.project_name(),
  srcs,
  dependencies: deps 
)

infinitton_dep = declare_dependency(
  link_with: [ infinitton_lib ],
  include_directories: [ 'include' ] 
)

