# I2C module regression test
#
#

include ../make.regression

STARTUP_STC=startup.stc


all :   p16f876a.cod p16f88.cod 


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<

p16f88.o: p16f88.asm i2c_low.inc

sim: sim_88 sim_876

sim_88: p16f88.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$<

sim_876: p16f876a.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$<
