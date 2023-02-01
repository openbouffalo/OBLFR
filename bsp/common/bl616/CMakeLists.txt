sdk_add_include_directories(include)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/bl616_bsp.c)

sdk_set_linker_script(bl616_flash.ld)

