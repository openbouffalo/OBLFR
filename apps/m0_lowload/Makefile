PINE64_SDK_PATH := $(realpath $(dir $(realpath $(lastword $(MAKEFILE_LIST))))../../)

-include sdkconfig

CPU_ID = m0

include $(PINE64_SDK_PATH)/cmake/project.build
