include ../make.regression


STARTUP_STC=startup.stc

all : ccp_877a.cod



ccp_877a.cod : ccp_877a.o
	gplink --map -s 16f877a.lkr -o $@  $^

sim: sim_877a

sim_877a: ccp_877a.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=ccp_877a.stc

