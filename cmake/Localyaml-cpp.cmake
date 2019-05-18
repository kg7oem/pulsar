if (NOT DOWNLOAD_DEPS)
    message(SEND_ERROR "Could not find a system yaml-cpp and DOWNLOAD_DEPS is OFF")
endif (NOT DOWNLOAD_DEPS)

message("  Will download and compile yaml-cpp")

ExternalProject_Add(
    yaml-cpp

    EXCLUDE_FROM_ALL ON

    URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.2.tar.gz
    URL_HASH SHA512=fea8ce0a20a00cbc75023d1db442edfcd32d0ac57a3c41b32ec8d56f87cc1d85d7dd7a923ce662f5d3a315f91a736d6be0d649997acd190915c1d68cc93795e4
    CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF YAML_CPP_BUILD_TESTS=OFF YAML_CPP_BUILD_TOOLS=OFF YAML_CPP_INSTALL=OFF
    TEST_EXCLUDE_FROM_MAIN ON
    INSTALL_COMMAND ""
)

set(LOCAL_YAML_CPP ON)
set(YAML_CPP_INCLUDE_DIR "${CMAKE_BINARY_DIR}/yaml-cpp-prefix/src/yaml-cpp/include")
set(YAML_CPP_LIBRARIES "${CMAKE_BINARY_DIR}/yaml-cpp-prefix/src/yaml-cpp-build/libyaml-cpp.a")
