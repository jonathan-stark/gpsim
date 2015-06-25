include ../make.regression



all : compar_873a.cod compar_628.cod compar_877a.cod compar_10f204.cod \
	compar_882.cod compar_2321.cod compar_6520.cod


compar_877a.cod : compar_877a.o
	gplink --map -s 16f877a.lkr -o $@  $<

compar_873a.cod : compar_873a.o
	gplink --map -s 16f873a.lkr  -o $@  $<

compar_628.cod : compar_628.o
	gplink --map -s 16f628.lkr  -o $@  $<

compar_882.cod : compar_882.o
	gplink --map -s 16f882.lkr  -o $@  $<

compar_10f204.cod: compar_10f204.o
	gplink  --map -s 10f204.lkr -o $@  $<

compar_2321.cod: compar_2321.o
	gplink  --map -o $@  $<

compar_6520.cod: compar_6520.o
	gplink  --map -o $@  $<

sim: sim_628 sim_873a  sim_877a sim_10f204 sim_882 sim_2321 sim_6520

sim_628: compar_628.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_873a: compar_873a.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=compar_873a.stc

sim_877a: compar_877a.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_10f204: compar_10f204.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_882: compar_882.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_2321: compar_2321.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_6520: compar_6520.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<
