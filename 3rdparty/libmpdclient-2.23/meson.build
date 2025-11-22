project('libmpdclient', 'c', version: '2.23',
  meson_version: '>= 0.56.0',
  default_options: [
    'c_std=c99',
    'warning_level=3',
  ],
  license: 'BSD',
)

cc = meson.get_compiler('c')
project_root = meson.current_source_dir()

conf = configuration_data()
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('VERSION', meson.project_version())

if host_machine.system() != 'windows'
  conf.set_quoted('DEFAULT_SOCKET', get_option('default_socket'))
endif

conf.set_quoted('DEFAULT_HOST', get_option('default_host'))
conf.set('DEFAULT_PORT', get_option('default_port'))

conf.set('HAVE_STRNDUP', cc.has_function('strndup', prefix: '#define _GNU_SOURCE\n#include <string.h>'))
conf.set('HAVE_SETLOCALE', cc.has_function('setlocale', prefix: '#include <locale.h>'))
conf.set('HAVE_USELOCALE', cc.has_function('uselocale', prefix: '#define _GNU_SOURCE\n#include <locale.h>'))

platform_deps = []
if host_machine.system() == 'haiku'
  platform_deps = [cc.find_library('network')]
endif

if get_option('tcp')
  conf.set('ENABLE_TCP', true)
  conf.set('HAVE_GETADDRINFO', cc.has_function('getaddrinfo', dependencies: platform_deps))
endif

configure_file(output: 'config.h', configuration: conf)

splitted_version = meson.project_version().split('.')

common_cflags = [
]

if cc.get_id() != 'msvc'
  # for strdup() with glibc
  common_cflags += '-D_GNU_SOURCE'
endif

test_cflags = [
  '-Wcast-align',
  '-Wcast-qual',
  '-Wdouble-promotion',
  '-Wfloat-equal',
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wmissing-format-attribute',
  '-Wmissing-prototypes',
  '-Wno-deprecated-declarations',
  '-Wpointer-arith',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wstrict-prototypes',
  '-Wundef',
  '-Wunused',
  '-Wwrite-strings',

  # clang specific warning options:
  '-Wunreachable-code-aggressive',
]

add_project_arguments(common_cflags, language: 'c')
add_project_arguments(cc.get_supported_arguments(test_cflags), language: 'c')

common_ldflags = []

if host_machine.system() == 'linux' and cc.has_link_argument('-Wl,--version-script=' + join_paths(project_root, 'test.ld'))
  common_ldflags += '-Wl,--version-script=' + join_paths(project_root, 'libmpdclient.ld')
endif

if host_machine.system() == 'windows'
  platform_deps = [cc.find_library('ws2_32')]
endif

inc = include_directories(
  'src',
  'include',

  # for the generated config.h
  '.',
)

public_inc = include_directories(
  'include',
)

subdir('include/mpd')

libmpdclient = library('mpdclient',
  'src/async.c',
  'src/audio_format.c',
  'src/ierror.c',
  'src/resolver.c',
  'src/capabilities.c',
  'src/connection.c',
  'src/database.c',
  'src/directory.c',
  'src/rdirectory.c',
  'src/error.c',
  'src/feature.c',
  'src/fd_util.c',
  'src/fingerprint.c',
  'src/output.c',
  'src/coutput.c',
  'src/entity.c',
  'src/idle.c',
  'src/iso8601.c',
  'src/kvlist.c',
  'src/list.c',
  'src/mixer.c',
  'src/mount.c', 'src/cmount.c',
  'src/neighbor.c',
  'src/cneighbor.c',
  'src/parser.c',
  'src/password.c',
  'src/player.c',
  'src/playlist.c',
  'src/player.c',
  'src/rplaylist.c',
  'src/cplaylist.c',
  'src/queue.c',
  'src/quote.c',
  'src/recv.c',
  'src/replay_gain.c',
  'src/response.c',
  'src/run.c',
  'src/request.c',
  'src/search.c',
  'src/send.c',
  'src/socket.c',
  'src/song.c',
  'src/status.c',
  'src/cstatus.c',
  'src/stats.c',
  'src/cstats.c',
  'src/sync.c',
  'src/tag.c',
  'src/sticker.c',
  'src/settings.c',
  'src/message.c',
  'src/cmessage.c',
  'src/partition.c',
  'src/cpartition.c',
  'src/binary.c',
  'src/albumart.c',
  'src/readpicture.c',
  'src/position.c',
  link_depends: [
    'libmpdclient.ld'
  ],
  include_directories: inc,
  dependencies: [
    platform_deps,
  ],
  link_args: common_ldflags,
  version: meson.project_version(),
  soversion: splitted_version[0],
  install: true
)

libmpdclient_dep = declare_dependency(
  link_with: libmpdclient,
  include_directories: public_inc,
)

executable('example',
  'src/example.c',
  dependencies: [
    libmpdclient_dep,
  ])

docdir = join_paths(get_option('datadir'), 'doc', meson.project_name())
install_data('AUTHORS', 'LICENSES/BSD-2-Clause.txt', 'LICENSES/BSD-3-Clause.txt', 'NEWS', 'README.rst',
  install_dir: docdir)

pkg_mod = import('pkgconfig')
pkg_mod.generate(
  libraries: libmpdclient,
  version: meson.project_version(),
  name: 'libmpdclient',
  description: 'Music Player Daemon client library',
)

if get_option('documentation')
  doxygen = find_program('doxygen', required: false)
  if doxygen.found()
    subdir('doc')
  endif
endif

if get_option('test')
  check_dep = dependency('check')
  subdir('test')
endif
