include ../make.regression


SCRIPT = 16f628.lkr

PROJECT = compar_628
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
STC = $(PROJECT).stc
STARTUP_STC=startup.stc

all : compar_873a.cod compar_628.cod compar_877a.cod


compar_877a.cod : compar_877a.o
	gplink --map -s 16f877a.lkr -o $@  $<

compar_873a.cod : compar_873a.o
	gplink --map -s 16f873a.lkr  -o $@  $<

compar_628.cod : compar_628.o
	gplink --map -s 16f628.lkr  -o $@  $<

sim: sim_628 sim_877a

sim_628: compar_628.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=compar_628.stc

sim_873a: compar_873a.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=compar_873a.stc

sim_877a: compar_877a.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=compar_877a.stc

