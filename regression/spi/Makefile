# PSP (Parallel Slave Port) module regression test
#
#

include ../make.regression

STARTUP_STC=startup.stc


all :   p16f88.cod p18f242.cod p16c62.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_88 sim_242 sim_c62

sim_88: p16f88.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$<

sim_242: p18f242.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$<

sim_c62: p16c62.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$<
