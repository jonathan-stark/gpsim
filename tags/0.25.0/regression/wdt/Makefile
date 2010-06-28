# WDT module regression test
#
#

include ../make.regression


all :   wdt_16f88.cod nwdt_16f88.cod wdt_16f628.cod nwdt_16f628.cod \
	wdt_16c64.cod nwdt_16c64.cod wdt_18f452.cod nwdt_18f452.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<

wdt_%.cod : wdt_%.o
	gplink --map -s $*.lkr -o $@  $<

nwdt_%.cod : nwdt_%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_wdt_16f88 sim_nwdt_16f88 sim_wdt_16f628 sim_nwdt_16f628 \
	sim_wdt_16c64 sim_nwdt_16c64 sim_wdt_18f452 sim_nwdt_18f452

