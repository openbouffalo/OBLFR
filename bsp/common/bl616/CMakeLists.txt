sdk_add_include_directories(include)

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/bl616_bsp.c ${CMAKE_CURRENT_SOURCE_DIR}/src/fw_header.c)

sdk_add_link_options(-ufw_header)
sdk_set_linker_script(bl616_flash.ld)

