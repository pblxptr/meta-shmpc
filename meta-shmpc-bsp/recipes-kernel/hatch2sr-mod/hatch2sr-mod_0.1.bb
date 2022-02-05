SUMMARY = "Example of how to build an external Linux kernel module"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=12f884d2ae1ff87c09e5b7ccc2c4ca7e"

inherit module

SRC_URI = "file://Makefile \
           file://hatch2sr_ctrl.h \
           file://engine.h \
           file://sensor.h \
           file://relay.h \
           file://hatch2sr_driver.c \
           file://hatch2sr_ctrl.c \
           file://engine.c \
           file://sensor.c \
           file://relay.c \
           file://COPYING \
          "

S = "${WORKDIR}"

# The inherit of module.bbclass will automatically name module packages with
# "kernel-module-" prefix as required by the oe-core build environment.

RPROVIDES:${PN} += "kernel-module-hatch2sr"