## RPMSG demo

This is a demo application that emulates a TTY and RAW channel between M0 and Linux running 
on the D0 core. 

Once running, the application will create a TTY device and a RAW device. The TTY device will
be available at /dev/ttyRPMSG0 and the RAW device will be available at /dev/rpmsg0. on linux

This requires the remoteproc driver to be enabled in the linux kernel.
