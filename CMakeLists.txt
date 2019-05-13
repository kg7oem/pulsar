# FIXME when building in a directory besides the root
# of the repo then files are rebuilt even if it is
# not needed - this is probably because of the generated
# DBUS interface header which has done this before

cmake_minimum_required(VERSION 3.8.2)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules/" ${CMAKE_MODULE_PATH})
include(ExternalProject)

project(pulsar)
set(CMAKE_BUILD_TYPE Debug)

option(BUILD_DOC "Build documentation" OFF)
option(BUILD_PROFILE "Perform a profile build" OFF)
option(DOWNLOAD_DEPS "Download and build missing dependencies" OFF)
option(VERBOSE "Verbose builds" OFF)

option(ENABLE_DBUS "Enable DBUS support" ON)
option(ENABLE_JACKAUDIO "Enable JACK Audio support" ON)
option(ENABLE_LADSPA "Enable LADSPA support" ON)
option(ENABLE_LV2 "Enable LV2 support" ON)
option(ENABLE_PORTAUDIO "Enable Portaudio support" ON)

if (VERBOSE)
    set(CMAKE_VERBOSE_MAKEFILE ON)
endif (VERBOSE)

# don't change anything below this line
set(CMAKE_CXX_STANDARD 17)

find_package(Boost 1.66.0 COMPONENTS system)
if (NOT Boost_FOUND)
    include(cmake/LocalBoost.cmake)
endif (NOT Boost_FOUND)

find_package(yaml-cpp 0.6.2)
if (NOT YAML_CPP_INCLUDE_DIR)
    include(cmake/LocalYAML-cpp.cmake)
endif (NOT YAML_CPP_INCLUDE_DIR)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# FIXME how to enable only in Debug build type?
add_definitions(-Og)

add_definitions(-fPIC)
add_definitions(-Wfatal-errors -Werror)
add_definitions(-Wall -Wextra)

# FIXME these are only needed because of the
# auto generated dbus adapter header but
# a way to mark that as a system header has not
# yet been found
add_definitions(-Wno-unused-variable)
if (CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-unused-but-set-variable)
endif (CMAKE_COMPILER_IS_GNUCXX)

# . gives us pulsar/ so it is not a system directory
include_directories(.)
include_directories(SYSTEM ext/)
include_directories(SYSTEM ${Boost_INCLUDE_DIR})
include_directories(SYSTEM ${YAML_CPP_INCLUDE_DIR})

if (BUILD_PROFILE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
endif (BUILD_PROFILE)

add_library(
    pulsar SHARED

    pulsar/async.cxx
    pulsar/audio.cxx
    pulsar/audio.util.cxx
    pulsar/daemon.cxx
    pulsar/debug.cxx
    pulsar/domain.cxx
    pulsar/library.cxx
    pulsar/node.cxx
    pulsar/property.cxx
    pulsar/system.cxx
    pulsar/thread.cxx
    pulsar/util.cxx
    pulsar/zeronode.cxx
)

add_library(
    logjam STATIC

    ext/logjam/logjam.cxx
)

add_executable(
    pulsar-dev

    pulsar-dev.cxx
    pulsar/config.cxx
)

if (LOCAL_BOOST)
    add_dependencies(pulsar boost)
    add_dependencies(logjam boost)
endif (LOCAL_BOOST)

if (LOCAL_YAML_CPP)
    add_dependencies(pulsar-dev yaml-cpp)
endif (LOCAL_YAML_CPP)

target_link_libraries(pulsar ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(pulsar ${Boost_LIBRARIES})
target_link_libraries(pulsar dl)
target_link_libraries(pulsar logjam)

target_link_libraries(pulsar-dev pulsar stdc++ ${YAML_CPP_LIBRARIES})

if (ENABLE_DBUS)
    message("Checking for dbus-c++")
    pkg_check_modules(DBUSLIB dbus-c++-1)

    if (DBUSLIB_FOUND)
        message("  DBUS support is enabled")
        add_custom_command(
            OUTPUT pulsar/dbus.adaptor.h
            COMMAND dbusxx-xml2cpp pulsar/dbus.adaptor.xml --adaptor=pulsar/dbus.adaptor.h
            DEPENDS pulsar/dbus.adaptor.xml
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

        add_definitions(-DCONFIG_ENABLE_DBUS)
        target_compile_options(pulsar PUBLIC ${DBUSLIB_CFLAGS_OTHER})
        include_directories(SYSTEM ${DBUSLIB_INCLUDE_DIRS})
        target_sources(pulsar PRIVATE pulsar/dbus.cxx pulsar/dbus.adaptor.h )
        target_link_libraries(pulsar ${DBUSLIB_LIBRARIES})
    endif (DBUSLIB_FOUND)
endif (ENABLE_DBUS)

if (ENABLE_JACKAUDIO)
    message("Checking for JACK audio")
    find_package(JACK)

    if (JACK_FOUND)
        message("  JACK audio support is enabled")
        add_definitions(-DCONFIG_ENABLE_JACKAUDIO)
        target_sources(pulsar PRIVATE pulsar/jackaudio.cxx)
        target_link_libraries(pulsar ${JACK_LIBRARY})
    endif (JACK_FOUND)
endif (ENABLE_JACKAUDIO)

if (ENABLE_LADSPA)
    message("LADSPA support is enabled")
    add_definitions(-DCONFIG_ENABLE_LADSPA)
    target_sources(pulsar PRIVATE pulsar/ladspa.cxx)
endif (ENABLE_LADSPA)

if (ENABLE_LV2)
    message("Checking for lilv")
    pkg_check_modules(LILV lilv-0)

    if (LILV_FOUND)
        message("  LV2 support is enabled")
        add_definitions(-DCONFIG_ENABLE_LV2)
        include_directories(SYSTEM ${LILV_INCLUDE_DIRS})
        target_sources(pulsar PRIVATE pulsar/LV2.cxx)
        target_link_libraries(pulsar ${LILV_LDFLAGS})
    endif (LILV_FOUND)
endif (ENABLE_LV2)

if (ENABLE_PORTAUDIO)
    message("Checking for Portaudio")
    find_package(Portaudio)

    if (PORTAUDIO_FOUND)
        message("  Portaudio support is enabled")
        add_definitions(-DCONFIG_ENABLE_PORTAUDIO)
        target_sources(pulsar PRIVATE pulsar/portaudio.cxx)
        target_link_libraries(pulsar ${PORTAUDIO_LIBRARY})
    endif (PORTAUDIO_FOUND)
endif (ENABLE_PORTAUDIO)

if (BUILD_DOC)
    message("Checking for Doxygen")
    find_package(Doxygen)

    if (DOXYGEN_FOUND)
        set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/etc/Doxyfile.in)
        set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

        # request to configure the file
        configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
        message("  Setting up documention builds")

        # note the option ALL which allows to build the docs together with the application
        add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            VERBATIM )
    else (DOXYGEN_FOUND)
        message(SEND_ERROR "BUILD_DOC is ON but Doxygen is missing; aborting build")
    endif (DOXYGEN_FOUND)
endif (BUILD_DOC)
