cmake_minimum_required(VERSION 3.15)

# Get SDK path
if(NOT SDK_PATH)
    get_filename_component(SDK_PATH ../../../ ABSOLUTE)
    if(EXISTS $ENV{OBLFR_SDK_PATH})
        set(SDK_PATH $ENV{OBLFR_SDK_PATH})
    endif()
endif()

# Check SDK Path
if(NOT EXISTS ${SDK_PATH})
    message(FATAL_ERROR "SDK path Error, Please set OBLFR_SDK_PATH variable")
endif()

include(${SDK_PATH}/cmake/bflbsdk.cmake)

sdk_add_include_directories(.)
sdk_set_main_file(main.c)

project(timer)
