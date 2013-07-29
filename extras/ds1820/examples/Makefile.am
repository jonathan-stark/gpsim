EXTRA_DIST = ds1820.asm ds18b20.asm 16f1823_g.lkr README

MOSTLYCLEANFILES = *.o *.hex *.cod *.lst *.map *~
CLEANFILES = *.o *.hex *.cod *.lst *.map *~
DISTCLEANFILES = *.o *.hex *.cod *.lst *.map *~
MAINTAINERCLEANFILES = *.o *.hex *.cod *.lst *.map *~


ds1820.cod : ds1820.o
	gplink --map -s 16f1823_g.lkr -o $@  $<

ds18b20.cod : ds18b20.o
	gplink --map -s 16f1823_g.lkr -o $@  $<

%.o : %.asm
	gpasm -c $<

