sdk_add_include_directories(include src)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/bl808_bsp_common.c)
if(CPU_ID STREQUAL  "d0")
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/bl808_bsp_d0.c)
elseif(CPU_ID STREQUAL  "m0")
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/bl808_bsp_m0.c)
else()
message(FATAL_ERROR "Unknown CPU")
endif()



