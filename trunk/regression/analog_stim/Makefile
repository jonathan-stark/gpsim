include ../make.regression

STARTUP_STC=startup.stc

all : p18f452.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_452

sim_452: p18f452.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="$^"
