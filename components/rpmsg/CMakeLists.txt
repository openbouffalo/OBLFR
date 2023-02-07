add_subdirectory(rpmsg-lite)
sdk_generate_library(oblfr_rpmsg)
sdk_add_include_directories(include)
sdk_library_add_sources(${CMAKE_CURRENT_SOURCE_DIR}/src/oblfr_rpmsg.c)
                            