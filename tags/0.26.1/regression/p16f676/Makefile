# PSP (Parallel Slave Port) module regression test
#
#

include ../make.regression


all :   p16f676.cod reset.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<

reset.cod : reset.o
	gplink --map -s 16f676.lkr -o $@  $<


sim: sim_p16f676 sim_reset

