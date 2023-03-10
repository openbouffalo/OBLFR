OBLFR_SDK_PATH := $(realpath $(dir $(realpath $(lastword $(MAKEFILE_LIST))))../../)

-include sdkconfig

CPU_ID = m0

include $(OBLFR_SDK_PATH)/cmake/project.build

flash:
	bflb-mcu-tool --port /dev/ttyUSB1 --baudrate 2000000 --chipname=bl808 --firmware build/build_out/linux_mgr_bl808_m0.bin

monitor:
	screen /dev/ttyUSB1 2000000