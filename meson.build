project('yamc', 'c', default_options : ['c_std=c11'])
glfw = dependency('glfw3')
glew = dependency('glew')
png = dependency('libpng')
threads = dependency('threads')
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

args_default = [ '-D_GNU_SOURCE' ]

src = ['src/config.h',  'src/cube.c',  'src/cube.h', 
'src/item.c',  'src/item.h', 'src/draw.h', 'src/draw.c', 'src/main.h',
'src/main.c',  'src/map.c', 'src/map.h',
'src/physics.c', 'src/physics.h',
'src/util.c',  'src/util.h', 'src/world.c',  'src/world.h',
'src/threads.h', 'src/noise.h', 'src/noise.c'
]
executable('yamc', src, dependencies : [glfw, m_dep, threads, glew, png], c_args: args_default)

cleaner = find_program('clang-format')
r = run_command(cleaner, '-i', 'src/*')
