include ../make.regression


STARTUP_STC=startup.stc


all : tmr0.cod

tmr0.cod: tmr0.o
	gplink  --map -s 12c509.lkr -o $@  $<

tmr0.o: tmr0.asm


sim: sim_p12c509_tmr0

sim_p12c509_tmr0: tmr0.cod
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$<
