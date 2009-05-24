# PSP (Parallel Slave Port) module regression test
#
#

include ../make.regression


all :   p12f675.cod 


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_p12f675

