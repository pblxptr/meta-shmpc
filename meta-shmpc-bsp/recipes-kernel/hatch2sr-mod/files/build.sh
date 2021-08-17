#!/bin/bash


KERNEL_SRC=/opt/sheli-dev/1.0.0/sysroots/arm1176jzfshf-vfp-poky-linux-gnueabi/usr/src/kernel make 

cp hatch2sr.ko /home/pp/srv/tftp
