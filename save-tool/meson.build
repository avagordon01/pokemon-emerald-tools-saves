project(
    'pokemon-tools',
    'cpp',
    default_options: [
        'warning_level=3',
        'cpp_std=c++20',
    ],
)

add_global_arguments(
  '-Wno-pedantic',
  language: 'cpp'
)

executable(
    'save-tool',
    ['save-tool.cc'],
)

executable(
    'gift-tool',
    ['gift-tool.cc'],
)

executable(
    'pokemon-info',
    ['pokemon-info.cc'],
)