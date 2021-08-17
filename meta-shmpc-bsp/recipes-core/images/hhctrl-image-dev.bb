SUMMARY = "A image used during development"
IMAGE_LINGUAS = " "
LICENSE = "MIT"

inherit core-image

IMAGE_INSTALL = "\
  packagegroup-base \
"

IMAGE_FEATURES_append = " \
  ssh-server-dropbear \
  debug-tweaks \
  nfs-client \
"
