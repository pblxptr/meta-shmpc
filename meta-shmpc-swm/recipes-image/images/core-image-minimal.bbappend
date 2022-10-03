SRC_URI = "\
    file://emmcsetup.lua \
    file://sw-description \
"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

inherit core-image

IMAGE_INSTALL:append = "\
	libubootenv-bin \
	swupdate \
	swupdate-progress \
	u-boot-env \
"