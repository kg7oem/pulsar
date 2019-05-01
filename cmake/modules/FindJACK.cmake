# - Try to find JACK include dirs and libraries
#
# Usage of this module as follows:
#
#     find_package(JACK)
#
# Variables used by this module, they can change the default behaviour and need
# to be set before calling find_package:
#
#  JACK_ROOT_DIR         Set this variable to the root installation of
#                            JACK if the module has problems finding the
#                            proper installation path.
#
# Variables defined by this module:
#
#  JACK_FOUND            System has JACK, include and lib dirs found
#  JACK_INCLUDE_DIR      The JACK include directories.
#  JACK_LIBRARY          The JACK library.

find_path(JACK_ROOT_DIR
    NAMES include/jack/jack.h
)

find_path(JACK_INCLUDE_DIR
    NAMES jack/jack.h
    HINTS ${JACK_ROOT_DIR}/include
)

find_library(JACK_LIBRARY
    NAMES jack
    HINTS ${JACK_ROOT_DIR}/lib
)

if(JACK_INCLUDE_DIR AND JACK_LIBRARY)
  set(JACK_FOUND TRUE)
else(JACK_INCLUDE_DIR AND JACK_LIBRARY)
  FIND_LIBRARY(JACK_LIBRARY NAMES JACK)
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(JACK DEFAULT_MSG JACK_INCLUDE_DIR JACK_LIBRARY )
  MARK_AS_ADVANCED(JACK_INCLUDE_DIR JACK_LIBRARY)
endif(JACK_INCLUDE_DIR AND JACK_LIBRARY)

mark_as_advanced(
    JACK_ROOT_DIR
    JACK_INCLUDE_DIR
    JACK_LIBRARY
)
