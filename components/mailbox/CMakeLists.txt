sdk_generate_library(oblfr_mailbox)
sdk_add_include_directories(include)
sdk_library_add_sources(${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_mailbox.c ${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_usb_peripheral.c )

