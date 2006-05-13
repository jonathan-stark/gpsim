include ../make.regression


SCRIPT = 16f628.lkr

PROJECT = compar_628
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
STC = $(PROJECT).stc
STARTUP_STC=startup.stc

all : p16c71.cod p18f452.cod p16f873a.cod p16f88.cod


p16f873a.cod : p16f873a.o
	gplink --map -s 16f873a.lkr -o $@  $<

p16f88.cod : p16f88.o
	gplink --map -s 16f88.lkr -o $@  $<

p16c71.cod : p16c71.o
	gplink --map -s 16c71.lkr -o $@  $<

p16c74.cod : p16c74.o
	gplink --map -s 16c74.lkr -o $@  $<

p18f452.cod : p18f452.o
	gplink --map -s 18f452.lkr -o $@  $<

sim: sim_71

sim_71: p16c71.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p16c71.cod"

sim_452: p18f452.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p18f452.cod"

sim_873a: p16f873a.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p16f873a.cod"

sim_88: p16f88.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p16f88.cod"
