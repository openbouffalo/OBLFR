# OpenBouffalo Firmware Repository

This is a collection of Firmware/Example Applications for the Bouffalo Range of Chips utilizing the [Bouffalo MCU SDK](https://github.com/bouffalolab/bl_mcu_sdk/) 


## Quick Start
For Pine64 OX64 Dev Board:
```
mkdir obfr && cd obfr
git clone https://github.com/XXX
git clone https://github.com/bouffalolab/bl_mcu_sdk.git
git clone git@gitee.com:bouffalolab/toolchain_gcc_t-head_linux.git
PATH=$PATH:<directory to toolchain_gcc_t-head_linux>/bin/
cd XXX/apps/helloworld
make
```
Then use BLDevCube to Flash the build/build_out/helloworld_m0.elf to your device

## Directory Layout
* apps/
	* Applications
* components/
	* Useful Libraries ported to the Boufallo Range of Chips
* cmake/
	* Support Files for CMake/Make and Kconfig
* bsp/
	* Board Support Packages for different boards using the Bouffalo Range of Chips
* tools/
	* Various Tools 

## Included Apps
* HelloWorld
* LowLoad
	* lowload firmware is used to assist Linux to boot on the BL808 Device. Please consult the [OpenBouffalo Buildroot repo](https://github.com/openbouffalo/buildroot_bouffalo) for more information


## Configuring SDK Features

The Bouffalo SDK provides numerous configuration options, enabled via the proj.conf in a standard bl_mcu_sdk project. This repository has enabled menuconfig, to give a graphical interface to enable/disable features in the SDK. You can enter the configuration dialog by running
```make config``` in each applications directory. 

Default Configuration Options for a application are saved in ```sdkconfig.default```, and when a project is built, a ```sdkconfig``` is created (and may be edited by hand, or updated with ```make config```). 
```build/config/sdkconfig.h``` header file is also created when compiling, and any configuration options that affect cmake are placed into ```proj.conf```

If a Application has a specific configuration options, a ```Kconfig``` file can be placed in the root of the application directory and it will be picked up automatically

*If you wish to help out, updating the [cmake/Kconfig](cmake/Kconfig)  with additional/new options or help text is greatly appreciated*

## Specifying Different CPU's or Cores

This SDK still respects the following commands to specify different CPU's/cores etc:
```
make  CHIP=chip_name  BOARD=board_name CPU_ID=d0
```
Please ensure you run ```make clean``` and remove the sdkconfig file if you change chips/boards or CPU's

## Specify Location of the Bouffalo Labs MCU SDK

If your bl_mcu_sdk located in different path, then the following will work:
```BL_SDK_BASE=<path> make```

## Board Support
We have started to write a board support package for the Pine64 OX64 in bsp/ox64. This is WIP. Other Boards are welcome

