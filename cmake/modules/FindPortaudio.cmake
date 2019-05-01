# - Try to find PORTAUDIO include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(PORTAUDIO)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  PORTAUDIO_ROOT_DIR         Set this variable to the root installation of
#                            PORTAUDIO if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  PORTAUDIO_FOUND            System has PORTAUDIO, include and lib dirs found
#  PORTAUDIO_INCLUDE_DIR      The PORTAUDIO include directories.
#  PORTAUDIO_LIBRARY          The PORTAUDIO library.

find_path(PORTAUDIO_ROOT_DIR
    NAMES include/portaudio.h
)

find_path(PORTAUDIO_INCLUDE_DIR
    NAMES portaudio.h
    HINTS ${PORTAUDIO_ROOT_DIR}/include
)

find_library(PORTAUDIO_LIBRARY
    NAMES portaudio
    HINTS ${PORTAUDIO_ROOT_DIR}/lib
)

if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
  set(PORTAUDIO_FOUND TRUE)
else(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
  FIND_LIBRARY(PORTAUDIO_LIBRARY NAMES PORTAUDIO)
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(PORTAUDIO DEFAULT_MSG PORTAUDIO_INCLUDE_DIR PORTAUDIO_LIBRARY )
  MARK_AS_ADVANCED(PORTAUDIO_INCLUDE_DIR PORTAUDIO_LIBRARY)
endif(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)

mark_as_advanced(
    PORTAUDIO_ROOT_DIR
    PORTAUDIO_INCLUDE_DIR
    PORTAUDIO_LIBRARY
)
