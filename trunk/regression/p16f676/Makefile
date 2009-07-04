# PSP (Parallel Slave Port) module regression test
#
#

include ../make.regression


all :   p16f676.cod 


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_p16f676

