include ../make.regression


all : p10f320.cod p10f320_cwg.cod p10f320_nco.cod

%.cod : %.o
	gplink --map -o $@ $<


sim: sim_p10f320 sim_p10f320_cwg sim_p10f320_nco

