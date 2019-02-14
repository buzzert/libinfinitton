src = [
  'device.c',
  'pixmap.c',
  'util.c',
]

deps = [
  dependency('hidapi-libusb'),
  dependency('cairo'),
]

infinittonlib = shared_library(
  'infinitton',
  src,
  include_directories: inc,
  dependencies: deps,
  install: true
)

subdir('infctl')

