include ../make.regression

all : p16c71.cod p18f452.cod p16f873a.cod p16f88.cod p16f871.cod p16f874a.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_p16c71 sim_452 sim_873a sim_871 sim_88 sim_874a

