deps = [
  dependency('cairo'),
  dependency('pangocairo'),
  dependency('threads'),
]

src = [
    'main.c'
]

pomodoro = executable('pomodoro',
  src,
  include_directories : inc,
  dependencies: deps,
  link_with: infinittonlib,
  install: true
)
