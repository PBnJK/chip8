project(
  'chip8',
  'c',
  license: 'MIT',
  license_files: 'LICENSE',
  version: '0.1.0'
)

subdir('src')
inc = include_directories('inc')

sdl2 = dependency('sdl2')

add_project_arguments('-DDEBUG', language : 'c')

executable(
  'chip8',
  sources: src,
  include_directories: inc,
  dependencies: sdl2
)
