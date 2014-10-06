EXTRA_DIST = dht11_example.asm 16f627_g.lkr README

MOSTLYCLEANFILES = *.o *.hex *.cod *.lst *.map *~
CLEANFILES = *.o *.hex *.cod *.lst *.map *~
DISTCLEANFILES = *.o *.hex *.cod *.lst *.map *~
MAINTAINERCLEANFILES = *.o *.hex *.cod *.lst *.map *~

SUFFIXES = .o .asm .cod

all: dht11_example.cod

dht11_example.cod: dht11_example.o

.o.cod:
	gplink --map -s 16f627_g.lkr -o $@ $<

.asm.o:
	gpasm -c $<
