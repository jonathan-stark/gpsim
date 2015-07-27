EXTRA_DIST = i2c2par.asm

MOSTLYCLEANFILES = *.o *.hex *.cod *.lst *.map *~
CLEANFILES = *.o *.hex *.cod *.lst *.map *~
DISTCLEANFILES = *.o *.hex *.cod *.lst *.map *~
MAINTAINERCLEANFILES = *.o *.hex *.cod *.lst *.map *~



i2c2par.cod : i2c2par.o
	gplink --map -o $@  $<

i2c2par.o : i2c2par.asm
	gpasm -c $<



