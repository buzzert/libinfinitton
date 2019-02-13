project('infctl', 'c',
  default_options: ['c_std=c11']
)

deps = [
  dependency('hidapi-libusb'),
]

srcs = [
  'main.c'
]

executable(
  meson.project_name(),
  srcs,
  dependencies: deps 
)

