if (NOT DOWNLOAD_DEPS)
    message(SEND_ERROR "Could not find a system YAML-cpp and DOWNLOAD_DEPS is OFF")
endif (NOT DOWNLOAD_DEPS)

message("  Will download and compile YAML-cpp")

set(BUILD_SHARED_LIBS OFF)
set(YAML_CPP_BUILD_TESTS OFF)
set(YAML_CPP_BUILD_TOOLS OFF)
set(YAML_CPP_INSTALL OFF)

ExternalProject_Add(
    yaml-cpp

    EXCLUDE_FROM_ALL ON

    URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.2.tar.gz
    URL_HASH SHA512=fea8ce0a20a00cbc75023d1db442edfcd32d0ac57a3c41b32ec8d56f87cc1d85d7dd7a923ce662f5d3a315f91a736d6be0d649997acd190915c1d68cc93795e4
    INSTALL_COMMAND ""
)

set(LOCAL_YAML_CPP ON)
