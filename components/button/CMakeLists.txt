sdk_generate_library(oblfr_button)
sdk_add_include_directories(include)
sdk_library_add_sources(${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_button.c 
                        ${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_button_gpio.c)

