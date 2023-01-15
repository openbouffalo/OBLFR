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

target_sources(app PRIVATE 
     src/bl808_ipc.c)

target_compile_options(app PRIVATE -ggdb -Os)
sdk_add_include_directories(include)
sdk_set_main_file(src/main.c)
project(m0_lowload)
