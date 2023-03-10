# OpenBouffalo Firmware Repository

This is a collection of Firmware/Example Applications for the Bouffalo Range of Chips utilizing the [Bouffalo MCU SDK](https://github.com/bouffalolab/bl_mcu_sdk/) 


## Quick Start
For Bouffallo based boards:
```
git clone https://github.com/openbouffalo/OBLFR.git
git clone https://github.com/bouffalolab/bl_mcu_sdk.git
git clone git@gitee.com:bouffalolab/toolchain_gcc_t-head_linux.git
PATH=$PATH:<directory to toolchain_gcc_t-head_linux>/bin/
cd OBLFR/apps/helloworld
make
```
Then use BLDevCube to Flash the build/build_out/helloworld_m0.elf to your device

if your bl_mcu_sdk is located in a different path, then the following will work:

```BL_SDK_BASE=<path> make```

## Directory Layout
* apps/
	* Applications
* apps/examples/
	* Example Applications showing how to use included components
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


## Flashing Different CPU's
You can use "make flash" to flash the firmware from the commandline instead of using the BLDevCube. By Default, the apps are configured to flash to the M0 core, but you can specify the core to flash to by modifying the flash_prog_cfg.ini file in the root of the project.
* For M0 set address = 0x000000
* For D0 set address = 0x100000
* For LP set address = 0x200000

If you are flashing from BLDevCube, set the CPU to Group0 and use the following addresses for the different cores:
* For M0 set address = 0x58000000
* For D0 set address = 0x58100000
* For LP set address = 0x58200000

(you may need to modify the cmake/flash.cmake file to specify the correct serial port for your device, or during the initial build, set the COMX enviroment variable to your serial port)

## Configuring SDK Features

The Bouffalo SDK provides numerous configuration options, enabled via the proj.conf in a standard bl_mcu_sdk project. This repository has enabled menuconfig, to give a graphical interface to enable/disable features in the SDK. You can enter the configuration dialog by running in each applications directory: 
```
make config
``` 


Default Configuration Options for a application are saved in ```sdkconfig.default```, and when a project is built, a ```sdkconfig``` is created (and may be edited by hand, or updated with ```make config```). 
```build/config/sdkconfig.h``` header file is also created when compiling, and any configuration options that affect cmake are placed into ```proj.conf```

If a Application has a specific configuration options, a ```Kconfig``` file can be placed in the root of the application directory and it will be picked up automatically

*If you wish to help out, updating the [cmake/Kconfig](cmake/Kconfig)  with additional/new options or help text is greatly appreciated*

## Specifying Different CPU's or Cores

This SDK still respects the following commands to specify different CPU's/cores etc:
```
make BOARD=board_name
```
For the BL808, you can specify the following cores:
``` 
BOARD=ox64  CHIP_ID=m0 make
```

A list of boards can be found in [cmake/bflbsdk.cmake](cmake/bflbsdk.cmake) under the ```set(BOARD_LIST``` section

Please ensure you run ```make distclean``` if you change chips/boards or CPU's

## Specify Location of the Bouffalo Labs MCU SDK

If your bl_mcu_sdk located in different path, then the following will work:
```BL_SDK_BASE=<path> make```

## Board Support

We currently have the following boards supported:
* Pine64 OX64 (bl808)
* Sispeed M1SDock (bl808)
* Sispeed M0SDock (bl616)
* Sispeed M0Sense (bl702)

Other Boards are welcome, please submit a PR with your board support package!

