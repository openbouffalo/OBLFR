sdk_add_include_directories(include)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_button.c 
                           ${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_button_gpio.c)

