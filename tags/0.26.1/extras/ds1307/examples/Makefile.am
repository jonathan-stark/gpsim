EXTRA_DIST = ds1307.asm i2c_low.inc 16f876a.lkr README

MOSTLYCLEANFILES = *.o *.hex *.cod *.lst *.map *~
CLEANFILES = *.o *.hex *.cod *.lst *.map *~
DISTCLEANFILES = *.o *.hex *.cod *.lst *.map *~
MAINTAINERCLEANFILES = *.o *.hex *.cod *.lst *.map *~



ds1307.cod : ds1307.o
	gplink --map -s 16f876a.lkr -o $@  $<

ds1307.o : ds1307.asm
	gpasm -c $<



