all: ds1820.cod  ds18b20.cod 

%.o : %.asm
	gpasm  -c $<

%.cod : %.o
	gplink --map -o $@  $<

clean:
	rm -f *~ *.o *.lst *.map *.hex *.cod

