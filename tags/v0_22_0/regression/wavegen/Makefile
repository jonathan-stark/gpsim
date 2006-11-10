include ../make.regression


STARTUP_STC=startup.stc

all : pulsegen.cod



pulsegen.cod : pulsegen.o
	gplink --map -s 16f877a.lkr -o $@  $^

sim: pulsegen

pulsegen: pulsegen.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=pulsegen.stc

