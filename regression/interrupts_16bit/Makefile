include ../make.regression


all : interrupts_16bit.cod priority.cod


interrupts_16bit.cod : interrupts_16bit.o
	gplink --map -s 18f2321.lkr -o $@  $^

priority.cod : priority.o
	gplink --map -s 18f2321.lkr -o $@  $^


sim: interrupts_16bit.cod priority.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=interrupts_16bit.stc
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=priority.stc

