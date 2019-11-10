deps = [
  dependency('cairo'),
  dependency('pangocairo'),
  dependency('threads'),
  dependency('x11')
]

src = [
    'main.c'
]

pomodoro = executable('taskman',
  src,
  include_directories : inc,
  dependencies: deps,
  link_with: infinittonlib,
  install: true
)
