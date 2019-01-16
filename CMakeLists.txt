cmake_minimum_required(VERSION 3.8.2)

project(pulsar)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads)

add_definitions(-Wfatal-errors -Werror -Wall -Wextra)
add_definitions(-g)

add_library(
    pulsar SHARED

    pulsar/audio.cxx
    pulsar/audio.util.cxx
    pulsar/domain.cxx
    pulsar/jackaudio.cxx
    pulsar/ladspa.cxx
    pulsar/node.cxx
)

add_executable(
    pulsar-dev

    pulsar-dev.cxx
)

target_link_libraries(pulsar ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pulsar dl)
target_link_libraries(pulsar jack)

target_link_libraries(pulsar-dev pulsar)
