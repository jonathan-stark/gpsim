include ../make.regression


all : p10f320.cod

%.cod : %.o
	gplink --map -o $@ $<


sim: sim_p10f320 sim_reset

