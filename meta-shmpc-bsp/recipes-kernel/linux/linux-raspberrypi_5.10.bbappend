
#TODO: Add enums in patch when defining pull ups. Instead of 17, use "GPIO_ACTIVE_LOW | PULL_UP"
#TODO: Perhaps remove secion with &gpio, where gpio configuration is overrided
#TODO: Add entry for fan, e.g. only alias for gpio so to have visible it in /dev.

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI_append = " file://0001-Added-device-tree-overlay-for-hatch2sr.patch;md5sum=5de8b6d0bb68732f4f7c9404c5bad441"