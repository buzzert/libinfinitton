deps = [
    dependency('cairo'),
]

infctl = executable(
  'infctl', 'main.c',
  include_directories : inc,
  dependencies: deps,
  link_with: infinittonlib,
  install: true
)
