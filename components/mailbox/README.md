## Mailbox
This components supports running linux on the D0 core
Include this in your M0 firmware to allow linux to continue to operate

## Description
Several Peripherals of the BL808 only have their IRQ attached to the M0 core.
This component "forwards" these IRQs to the M0 core to allow Linux to operate. 

This component also includes support for the bl808-ipc driver on linux, which is 
mailbox driver. It allows Linux running on the D0 core to send interupts to the M0 core.

## Peripherals Forwarded
The following Peripherals can be forwarded to the M0 core. (You can 
change this via ```make config```) under your project configuartion, but be aware, 
some peripherals might not work in linux if you disable them here.

* SD Card Support (Mandatory to have your rootfs on the SD card)
* UART2 of the M0 core forwarded to Linux
* USB Support
* GPIO Pins (See below for caveats)
* Ethernet

## GPIO Interupt Support
To be documented. 

## Ethernet Support
Enabling Ethernet Support also initilizes the Ethernet Peripheral via the M0 core.

## SDHCI Support
Enabling SD Card Support also initializes the SDHCI Peripheral via the M0 core.
