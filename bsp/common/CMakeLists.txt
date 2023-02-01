sdk_add_include_directories(include)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/bsp_common.c)

add_subdirectory(${SDK_PATH}/bsp/common/${CHIP} bsp_common_${CHIP})
