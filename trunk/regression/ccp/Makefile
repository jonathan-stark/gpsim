include ../make.regression


STARTUP_STC=startup.stc

all : ccp_877a.cod pwm_877a.cod



ccp_877a.cod : ccp_877a.o
	gplink --map -s 16f877a.lkr -o $@  $^

pwm_877a.cod : pwm_877a.o
	gplink --map -s 16f877a.lkr -o $@  $^

sim: sim_877a sim_pwm887a

sim_877a: ccp_877a.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=ccp_877a.stc

sim_pwm877a: pwm_877a.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=pwm_877a.cod

