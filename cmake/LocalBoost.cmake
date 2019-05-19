message("  Will download and compile Boost")

ExternalProject_Add(
    local-boost

    EXCLUDE_FROM_ALL ON
    BUILD_IN_SOURCE TRUE

    URL https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.tar.gz
    URL https://newcontinuum.dl.sourceforge.net/project/boost/boost/1.66.0/boost_1_66_0.tar.gz
    URL_HASH SHA512=8d537fa1fcd24dfa3bc107741d20d93624851b724883c5cfe0cdcd8b8390939869e0f41a36f52d3051f129a8405a574a1a095897f82ece747740b34a1e52ffdb

    CONFIGURE_COMMAND ./bootstrap.sh
    BUILD_COMMAND ./b2 --with-system --build-type=minimal --layout=system cxxflags=-fPIC link=static runtime-link=static threading=multi
    INSTALL_COMMAND ""
)

set(LOCAL_BOOST ON)

ExternalProject_Get_Property(local-boost SOURCE_DIR)
set(Boost_INCLUDE_DIR "${SOURCE_DIR}")
set(Boost_LIBRARIES "${SOURCE_DIR}/stage/lib/libboost_system.a")
