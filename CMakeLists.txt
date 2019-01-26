cmake_minimum_required(VERSION 3.8.2)

project(pulsar)

# configuration
# Debug, Release, RelWithDebInfo, MinSizeRel
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE OFF)

include_directories(${Boost_INCLUDE_DIRS})

# don't change anything below this line
set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.58.0 COMPONENTS system)
find_package(Doxygen)
find_package(Threads)
find_package(yaml-cpp)

# FIXME how to enable only in Debug build type?
add_definitions(-Og)

add_definitions(-fPIC)
add_definitions(-Wfatal-errors -Werror -Wall -Wextra)

include_directories(.)
include_directories(ext/)

option(BUILD_DOC "Build documentation" ON)

add_library(
    pulsar SHARED

    pulsar/async.cxx
    pulsar/audio.cxx
    pulsar/audio.util.cxx
    pulsar/config.cxx
    pulsar/debug.cxx
    pulsar/domain.cxx
    pulsar/jackaudio.cxx
    pulsar/ladspa.cxx
    pulsar/library.cxx
    pulsar/node.cxx
    pulsar/property.cxx
    pulsar/system.cxx
    pulsar/util.cxx
)

add_library(
    logjam STATIC

    ext/logjam/logjam.cxx
)

add_executable(
    pulsar-dev

    pulsar-dev.cxx
)

target_link_libraries(pulsar ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pulsar ${Boost_LIBRARIES})
target_link_libraries(pulsar dl)
target_link_libraries(pulsar jack)
target_link_libraries(pulsar logjam)
target_link_libraries(pulsar yaml-cpp)

target_link_libraries(pulsar-dev pulsar)

if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/pulsar/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    # request to configure the file
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    message("Doxygen build started")

    # note the option ALL which allows to build the docs together with the application
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the doxygen documentation")
endif (DOXYGEN_FOUND)
