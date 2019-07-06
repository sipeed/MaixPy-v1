cmake_minimum_required(VERSION 3.9)


# Get SDK path
if(NOT SDK_PATH)
    get_filename_component(SDK_PATH ../../ ABSOLUTE)
    if(EXISTS $ENV{MY_SDK_PATH})
        set(SDK_PATH $ENV{MY_SDK_PATH})
    endif()
endif()

# Check SDK Path
if(NOT EXISTS ${SDK_PATH})
    message(FATAL_ERROR "SDK path Error, Please set SDK_PATH or MY_SDK_PATH variable")
endif()

# Get Toolchain path
if(NOT CONFIG_TOOLCHAIN_PATH)
    if(EXISTS $ENV{MY_TOOLCHAIN_PATH})
        set(CONFIG_TOOLCHAIN_PATH $ENV{MY_TOOLCHAIN_PATH})
    endif()
endif()

# Call compile
include(${SDK_PATH}/tools/cmake/compile.cmake)


# Project Name
project(maixpy)



