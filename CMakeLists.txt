cmake_minimum_required(VERSION 3.8.2)

project(pulsar)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE OFF)

find_package(Boost 1.5.8 COMPONENTS system)
find_package(Threads)
find_package(yaml-cpp)

add_definitions(-Wfatal-errors -Werror -Wall -Wextra)
add_definitions(-g)

include_directories(${Boost_INCLUDE_DIRS})

add_library(
    pulsar SHARED

    pulsar/async.cxx
    pulsar/audio.cxx
    pulsar/audio.util.cxx
    pulsar/configfile.cxx
    pulsar/domain.cxx
    pulsar/jackaudio.cxx
    pulsar/ladspa.cxx
    pulsar/logjam.cxx
    pulsar/node.cxx
    pulsar/property.cxx
    pulsar/system.cxx
)

add_executable(
    pulsar-dev

    pulsar-dev.cxx
)

target_link_libraries(pulsar ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pulsar ${Boost_LIBRARIES})
target_link_libraries(pulsar dl)
target_link_libraries(pulsar jack)
target_link_libraries(pulsar yaml-cpp)

target_link_libraries(pulsar-dev pulsar)
