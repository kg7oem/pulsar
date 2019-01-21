cmake_minimum_required(VERSION 3.8.2)

project(pulsar)

# configuration
# Debug, Release, RelWithDebInfo, MinSizeRel
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_VERBOSE_MAKEFILE OFF)

# don't change anything below this line
set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.5.8 COMPONENTS system)
find_package(Threads)
find_package(yaml-cpp)

add_definitions(-fPIC)
add_definitions(-Wfatal-errors -Werror -Wall -Wextra)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(.)

add_library(
    pulsar SHARED

    pulsar/async.cxx
    pulsar/audio.cxx
    pulsar/audio.util.cxx
    pulsar/config.cxx
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
    pulsar-ext STATIC

    ext/logjam.cxx
)

add_executable(
    pulsar-dev

    pulsar-dev.cxx
)

target_link_libraries(pulsar ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pulsar ${Boost_LIBRARIES})
target_link_libraries(pulsar dl)
target_link_libraries(pulsar jack)
target_link_libraries(pulsar pulsar-ext)
target_link_libraries(pulsar yaml-cpp)

target_link_libraries(pulsar-dev pulsar)
