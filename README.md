# Smart Home Management Platform Control
BSP layer for hhctrl. Custom kernel drivers and configuration.

Most importants parts are:
- hatch2sr kernel driver (meta-shmpc-bsp/recipes-kernel/hatch2sr-mod), developed to control doors engine as well as doors (open/close) sensors
- device tree-overlay (meta-shmpc-bsp/recipes-kernel/linux)
- ubboot configuration (meta-shmpc/blob/meta-shmpc-bsp/conf/machine/hhctrl.conf)