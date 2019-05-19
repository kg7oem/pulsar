message("  Will download and compile yaml-cpp")

ExternalProject_Add(
    pulsar-yaml-cpp

    EXCLUDE_FROM_ALL ON

    URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.6.2.tar.gz
    URL http://deb.debian.org/debian/pool/main/y/yaml-cpp/yaml-cpp_0.6.2.orig.tar.gz
    URL_HASH SHA512=fea8ce0a20a00cbc75023d1db442edfcd32d0ac57a3c41b32ec8d56f87cc1d85d7dd7a923ce662f5d3a315f91a736d6be0d649997acd190915c1d68cc93795e4
    CMAKE_ARGS -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_SHARED_LIBS=OFF -DYAML_CPP_BUILD_TESTS=OFF -DYAML_CPP_BUILD_TOOLS=OFF -DYAML_CPP_BUILD_CONTRIB=OFF
    INSTALL_COMMAND ""
)

set(LOCAL_YAML_CPP ON)

ExternalProject_Get_Property(pulsar-yaml-cpp SOURCE_DIR)
set(YAML_CPP_INCLUDE_DIR "${SOURCE_DIR}/include")

ExternalProject_Get_Property(pulsar-yaml-cpp BINARY_DIR)
set(YAML_CPP_LIBRARIES "${BINARY_DIR}/libyaml-cpp.a")
