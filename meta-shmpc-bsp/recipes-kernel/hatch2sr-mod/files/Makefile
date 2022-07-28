obj-m := hatch2sr.o
hatch2sr-objs := hatch2sr_driver.o hatch2sr_ctrl.o engine.o sensor.o relay.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.symvers

deploy:
	cp hatch2sr.ko /srv/tftp