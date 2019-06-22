### This file is used for build library standalone.

# set this will supress some warnings
set(BUILDING_SDK "yes" CACHE INTERNAL "")

# basic config
cmake_minimum_required(VERSION 3.0)
include(./common.cmake)
project(kendryte)

# config self use headers
include(./macros.internal.cmake)
header_directories(${SDK_ROOT}/lib)

# include lib make file
include(../lib/CMakeLists.txt)

# find headers files to INSTALL
file(GLOB_RECURSE LIB_HEADERS
        "../lib/*.h"
        "../lib/*.hpp"
        )
set_target_properties(kendryte PROPERTIES PUBLIC_HEADER "${LIB_HEADERS}")

# copy .a file and headers
install(TARGETS kendryte
        EXPORT kendryte
        ARCHIVE
        DESTINATION ${CMAKE_BINARY_DIR}/archive
        PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/archive/include
        )

# copy utils files
install(DIRECTORY
        ../lds
        ../utils
        ../cmake
        DESTINATION ${CMAKE_BINARY_DIR}/archive
        PATTERN "*internal*" EXCLUDE
        PATTERN "CMakeLists.txt" EXCLUDE
        )

# show information
include(./dump-config.cmake)
