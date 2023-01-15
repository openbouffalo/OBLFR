message("Running KConfig")
add_custom_target(config-dir ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory config)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/config/sdkconfig.h
                   COMMAND ${SDK_PATH}/tools/kconfig/gensdkconfig.py --sdkpath ${SDK_PATH} --header ${CMAKE_CURRENT_BINARY_DIR}/config/ --projectdir ${CMAKE_CURRENT_SOURCE_DIR} --env BFLB_BOARD_DIR=${BOARD_DIR} --env BFLB_BOARD=${BOARD} --env BFLB_CHIP=${CHIP} --env BFLB_CPU_ID=${CPU_ID}
                   DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sdkconfig
                   COMMENT "Generating SDKConfig"
                   VERBATIM)

target_sources(app PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/config/sdkconfig.h)

target_include_directories(app PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/config)
