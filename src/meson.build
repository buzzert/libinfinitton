src = [
  'device.c',
  'pixmap.c'
]

deps = [
  dependency('hidapi-libusb'),
]

infinittonlib = shared_library(
  'infinitton',
  src,
  include_directories: inc,
  dependencies: deps,
  install: true
)

subdir('infctl')

