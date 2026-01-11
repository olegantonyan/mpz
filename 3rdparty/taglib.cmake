set(TAGLIB_DIR 3rdparty/taglib-2.1.1)
message(STATUS "using vendored taglib from ${TAGLIB_DIR}")
add_subdirectory(${TAGLIB_DIR})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
 ${TAGLIB_DIR}
 ${TAGLIB_DIR}/taglib
 ${TAGLIB_DIR}/taglib/ape
 ${TAGLIB_DIR}/taglib/asf
 ${TAGLIB_DIR}/taglib/dsdiff
 ${TAGLIB_DIR}/taglib/dsf
 ${TAGLIB_DIR}/taglib/flac
 ${TAGLIB_DIR}/taglib/it
 ${TAGLIB_DIR}/taglib/mod
 ${TAGLIB_DIR}/taglib/mp4
 ${TAGLIB_DIR}/taglib/mpeg
 ${TAGLIB_DIR}/taglib/mpeg/id3v1
 ${TAGLIB_DIR}/taglib/mpeg/id3v2
 ${TAGLIB_DIR}/taglib/mpeg/id3v2/frames
 ${TAGLIB_DIR}/taglib/ogg
 ${TAGLIB_DIR}/taglib/ogg/flac
 ${TAGLIB_DIR}/taglib/ogg/opus
 ${TAGLIB_DIR}/taglib/ogg/speex
 ${TAGLIB_DIR}/taglib/ogg/vorbis
 ${TAGLIB_DIR}/taglib/riff
 ${TAGLIB_DIR}/taglib/riff/aiff
 ${TAGLIB_DIR}/taglib/riff/wav
 ${TAGLIB_DIR}/taglib/s3m
 ${TAGLIB_DIR}/taglib/toolkit
 ${TAGLIB_DIR}/taglib/trueaudio
 ${TAGLIB_DIR}/taglib/wavpack
 ${TAGLIB_DIR}/taglib/xm
)
