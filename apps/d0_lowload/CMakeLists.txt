cmake_minimum_required(VERSION 3.15)

# Get SDK path
if(NOT SDK_PATH)
    get_filename_component(SDK_PATH ../../ ABSOLUTE)
    if(EXISTS $ENV{OBLFR_SDK_PATH})
        set(SDK_PATH $ENV{OBLFR_SDK_PATH})
    endif()
endif()

# Check SDK Path
if(NOT EXISTS ${SDK_PATH})
    message(FATAL_ERROR "SDK path Error, Please set OBLFR_SDK_PATH variable")
endif()

include(${SDK_PATH}/cmake/bflbsdk.cmake)

target_sources(app PRIVATE 
    src/rv32i_xtheade_lz4.S)

target_compile_options(app PRIVATE -ggdb -Os)

sdk_set_main_file(src/main.c)
sdk_set_linker_script(bl808_flash_d0.ld)
project(d0_lowload)
