cmake_minimum_required(VERSION 3.15)

include(proj.conf)

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

include(${SDK_PATH}/cmake/bflbsdk.cmake)

sdk_set_main_file(main.c)

project(helloworld)
