#makefile for socket test.

GCC      = g++
CFLAGS   = `pkg-config --cflags glib-2.0`
LIBS     =  -lstdc++  `pkg-config --libs glib-2.0` -lgpsim

ASM      = gpasm
PICSRC   = gensquares

all: client $(PICSRC).cod

client.o: client.cc
	$(GCC) -c client.cc $(CFLAGS)

client: client.o
	$(GCC) -o client client.o $(LIBS)

$(PICSRC).cod: $(PICSRC).asm
	gpasm $(PICSRC).asm

clean:
	rm -f $(PICSRC).lst $(PICSRC).hex $(PICSRC).cod
	rm -f client client.o
	rm *~


