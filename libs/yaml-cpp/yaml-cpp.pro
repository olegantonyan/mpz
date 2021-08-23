TEMPLATE = lib
TARGET = yaml-cpp

CONFIG += warn_off
CONFIG += staticlib
CONFIG += c++11

win32: CONFIG -= debug_and_release

SOURCES += \
  yaml-cpp-0.7.0/src/convert.cpp \
  yaml-cpp-0.7.0/src/null.cpp \
  yaml-cpp-0.7.0/src/directives.cpp \
  yaml-cpp-0.7.0/src/exceptions.cpp \
  yaml-cpp-0.7.0/src/singledocparser.cpp \
  yaml-cpp-0.7.0/src/contrib/graphbuilder.cpp \
  yaml-cpp-0.7.0/src/contrib/graphbuilderadapter.cpp \
  yaml-cpp-0.7.0/src/nodeevents.cpp \
  yaml-cpp-0.7.0/src/emitter.cpp \
  yaml-cpp-0.7.0/src/depthguard.cpp \
  yaml-cpp-0.7.0/src/ostream_wrapper.cpp \
  yaml-cpp-0.7.0/src/simplekey.cpp \
  yaml-cpp-0.7.0/src/scantag.cpp \
  yaml-cpp-0.7.0/src/scanscalar.cpp \
  yaml-cpp-0.7.0/src/exp.cpp \
  yaml-cpp-0.7.0/src/emit.cpp \
  yaml-cpp-0.7.0/src/scanner.cpp \
  yaml-cpp-0.7.0/src/emitterstate.cpp \
  yaml-cpp-0.7.0/src/emitterutils.cpp \
  yaml-cpp-0.7.0/src/parser.cpp \
  yaml-cpp-0.7.0/src/memory.cpp \
  yaml-cpp-0.7.0/src/emitfromevents.cpp \
  yaml-cpp-0.7.0/src/nodebuilder.cpp \
  yaml-cpp-0.7.0/src/regex_yaml.cpp \
  yaml-cpp-0.7.0/src/node.cpp \
  yaml-cpp-0.7.0/src/node_data.cpp \
  yaml-cpp-0.7.0/src/binary.cpp \
  yaml-cpp-0.7.0/src/scantoken.cpp \
  yaml-cpp-0.7.0/src/parse.cpp \
  yaml-cpp-0.7.0/src/tag.cpp \
  yaml-cpp-0.7.0/src/stream.cpp \


HEADERS += \
  yaml-cpp-0.7.0/src/stringsource.h \
  yaml-cpp-0.7.0/src/tag.h \
  yaml-cpp-0.7.0/src/scantag.h \
  yaml-cpp-0.7.0/src/singledocparser.h \
  yaml-cpp-0.7.0/src/streamcharsource.h \
  yaml-cpp-0.7.0/src/contrib/graphbuilderadapter.h \
  yaml-cpp-0.7.0/src/indentation.h \
  yaml-cpp-0.7.0/src/nodebuilder.h \
  yaml-cpp-0.7.0/src/setting.h \
  yaml-cpp-0.7.0/src/ptr_vector.h \
  yaml-cpp-0.7.0/src/token.h \
  yaml-cpp-0.7.0/src/regeximpl.h \
  yaml-cpp-0.7.0/src/emitterstate.h \
  yaml-cpp-0.7.0/src/regex_yaml.h \
  yaml-cpp-0.7.0/src/collectionstack.h \
  yaml-cpp-0.7.0/src/stream.h \
  yaml-cpp-0.7.0/src/exp.h \
  yaml-cpp-0.7.0/src/emitterutils.h \
  yaml-cpp-0.7.0/src/scanner.h \
  yaml-cpp-0.7.0/src/scanscalar.h \
  yaml-cpp-0.7.0/src/nodeevents.h \
  yaml-cpp-0.7.0/src/directives.h \
  yaml-cpp-0.7.0/include/yaml-cpp/ostream_wrapper.h \
  yaml-cpp-0.7.0/include/yaml-cpp/null.h \
  yaml-cpp-0.7.0/include/yaml-cpp/noexcept.h \
  yaml-cpp-0.7.0/include/yaml-cpp/contrib/anchordict.h \
  yaml-cpp-0.7.0/include/yaml-cpp/contrib/graphbuilder.h \
  yaml-cpp-0.7.0/include/yaml-cpp/emittermanip.h \
  yaml-cpp-0.7.0/include/yaml-cpp/emitterstyle.h \
  yaml-cpp-0.7.0/include/yaml-cpp/emitterdef.h \
  yaml-cpp-0.7.0/include/yaml-cpp/depthguard.h \
  yaml-cpp-0.7.0/include/yaml-cpp/parser.h \
  yaml-cpp-0.7.0/include/yaml-cpp/emitter.h \
  yaml-cpp-0.7.0/include/yaml-cpp/binary.h \
  yaml-cpp-0.7.0/include/yaml-cpp/anchor.h \
  yaml-cpp-0.7.0/include/yaml-cpp/eventhandler.h \
  yaml-cpp-0.7.0/include/yaml-cpp/exceptions.h \
  yaml-cpp-0.7.0/include/yaml-cpp/emitfromevents.h \
  yaml-cpp-0.7.0/include/yaml-cpp/stlemitter.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/parse.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/detail/node_iterator.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/detail/node.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/detail/node_ref.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/detail/iterator.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/detail/impl.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/detail/node_data.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/detail/iterator_fwd.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/detail/memory.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/emit.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/node.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/type.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/iterator.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/impl.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/ptr.h \
  yaml-cpp-0.7.0/include/yaml-cpp/node/convert.h \
  yaml-cpp-0.7.0/include/yaml-cpp/dll.h \
  yaml-cpp-0.7.0/include/yaml-cpp/yaml.h \
  yaml-cpp-0.7.0/include/yaml-cpp/traits.h \
  yaml-cpp-0.7.0/include/yaml-cpp/mark.h \


INCLUDEPATH += \
  yaml-cpp-0.7.0/include \
  yaml-cpp-0.7.0/src \
  yaml-cpp-0.7.0/src/contrib \

