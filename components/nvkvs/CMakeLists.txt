sdk_generate_library()
sdk_add_include_directories(include)
sdk_add_include_directories(kved)
sdk_library_add_sources(${CMAKE_CURRENT_SOURCE_DIR}/kved/kved.c
                        ${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_nvkvs.c)
                            
sdk_library_add_sources_ifdef(CONFIG_COMPONENT_NVKVS_MEM_BACKEND ${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_kved_memory.c)
sdk_library_add_sources_ifdef(CONFIG_COMPONENT_NVKVS_FLASH_BACKEND ${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_kved_flash.c)