cmake_minimum_required(VERSION 3.8.2)

project(pulsar)

set(CMAKE_CXX_STANDARD 17)

find_package(Threads)

add_definitions(-Wfatal-errors -Werror -Wall -Wextra)
add_definitions(-g)

add_executable(
    pulsar-dev

    src/audio.cxx
    src/audio.util.cxx
    src/domain.cxx
    src/jackaudio.cxx
    src/ladspa.cxx
    src/main.cxx
    src/node.cxx
)

target_link_libraries(pulsar-dev ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pulsar-dev dl)
target_link_libraries(pulsar-dev jack)
