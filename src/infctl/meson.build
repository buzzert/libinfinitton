deps = [
    dependency('cairo'),
    dependency('pango'),
    dependency('pangocairo'),
]

infctl = executable(
  'infctl', 'main.c',
  include_directories : inc,
  dependencies: deps,
  link_with: infinittonlib,
  install: true
)
