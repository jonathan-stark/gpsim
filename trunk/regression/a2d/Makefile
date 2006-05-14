include ../make.regression

STARTUP_STC=startup.stc

all : p16c71.cod p18f452.cod p16f873a.cod p16f88.cod p16f871.cod p16f874a.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_71 sim_452 sim_873a sim_871 sim_88 sim_874a

sim_71: p16c71.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p16c71.cod"

sim_452: p18f452.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p18f452.cod"

sim_873a: p16f873a.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p16f873a.cod"

sim_874a: p16f874a.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p16f874a.cod"

sim_871: p16f871.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p16f871.cod"

sim_88: p16f88.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="p16f88.cod"
