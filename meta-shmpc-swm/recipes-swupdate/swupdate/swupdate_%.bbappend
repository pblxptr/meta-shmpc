FILESEXTRAPATHS:append := "${THISDIR}/${PN}/files:"

PACKAGECONFIG_CONFARGS = ""

SRC_URI += " \
    file://swupdate.cfg \
    file://09-swupdate-args \
"

do_install:append() {
    install -d ${D}${sysconfdir}
    install -m 644 ${WORKDIR}/swupdate.cfg ${D}${sysconfdir}

    install -m 644 ${WORKDIR}/09-swupdate-args ${D}${libdir}/swupdate/conf.d
}

#RDEPENDS_${PN} += " swupdate swupdate-www swupdate-tools lua util-linux-sfdisk"
