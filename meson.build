project('gtkmm_example','cpp','c')

sources = ['hello-world-main.cc',
           'hello-world.cc',
          ]

gtkmmdep = dependency('gtkmm-3.0')
deps = [gtkmmdep]

gnome = import('gnome')

resources = gnome.compile_resources(
  'resources',
  'example-resource.xml',
  c_name :'resources'
)

name='hello-gtkmm'
name_cap = 'HelloGtkMM'
version = '0.1.0'

exe = executable(name_cap,
                 sources,
                 resources,
                 dependencies: deps,
                 gui_app : true
                )

r = run_command('git', 'rev-parse', 'HEAD')
if r.returncode() != 0
  # it failed
endif

sha1 = r.stdout().strip()
# meson version 0.56 will have substr()
sha1_short = sha1[0]+sha1[1]+sha1[2]+sha1[3]+sha1[4]+sha1[5]

fs = import('fs')
builddir = fs.name(meson.current_build_dir())

if host_machine.system() == 'windows'
  arch = 'x86_64-w64-mingw32'
  host = 'w64'

  nsis_cmd = [
    find_program('makensis'),
    '-DOUTDIR='+builddir,
    '-DCOMMITID_SHORT='+sha1_short,
    '-DNAME='+name,
    '-DNAME_CAP='+name_cap,
    '-DICON_NAME='+name,
    '-DARCH='+arch,
    '-DHOST='+host,
    '-DVERSION='+version,
    '-DLIBGCCDLL=libgcc_s_seh-1.dll',
    '@INPUT@',
  ]

  nsis = custom_target('nsis',
                       output: 'install-hello-world.exe',
                       input: files('hello-world.nsi'),
                       build_always_stale: true,
                       command: nsis_cmd,
                       depends: [exe]
                       )
  alias_target('installer', nsis)
endif
