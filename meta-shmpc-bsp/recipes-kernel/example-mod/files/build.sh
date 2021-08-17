#!/bin/bash

# Compile & copy module
KERNEL_SRC=/opt/sheli-dev/1.0.0/sysroots/arm1176jzfshf-vfp-poky-linux-gnueabi/usr/src/kernel make 
cp example.ko /home/pp/srv/tftp

# Compile & copy client_rw
$CC -o client_rw client_rw.c
cp client_rw /home/pp/srv/tftp


