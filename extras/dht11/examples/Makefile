all: dht11_example.cod

%.o : %.asm
	gpasm  -c $<

%.cod : %.o
	gplink --map -o dht11_example.hex $<

clean:
	rm -f *~ *.o *.lst *.map *.hex *.cod

