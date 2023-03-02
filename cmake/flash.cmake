if ($ENV{BAUDRATE})
    set(BAUDRATE $ENV{BAUDRATE})
else()
    set(BAUDRATE 2000000)
endif()

if ($ENV{COMX})
    set(COMX $ENV{COMX})
else()
    set(COMX /dev/ttyUSB1)
endif()

add_custom_target(flash
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND $ENV{BL_SDK_BASE}/tools/bflb_tools/bouffalo_flash_cube/BLFlashCommand --interface=uart --baudrate=${BAUDRATE} --port=${COMX} --chipname=${CHIP} --cpu_id=${CPU_ID} --config=${CMAKE_CURRENT_SOURCE_DIR}/flash_prog_cfg.ini
        )

add_custom_target(monitor
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND tio --baudrate=${BAUDRATE} ${COMX}
        )