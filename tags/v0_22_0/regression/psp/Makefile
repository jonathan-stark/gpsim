# PSP (Parallel Slave Port) module regression test
#
#

include ../make.regression

STARTUP_STC=startup.stc


all : p16f871.cod p18f452.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<

sim: sim_871 sim_452

sim_871: p16f871.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$<

sim_452: p18f452.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$<
