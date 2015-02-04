# WDT module regression test
#
#

include ../make.regression


all :   wdt_16f88.cod nwdt_16f88.cod wdt_16f628.cod nwdt_16f628.cod \
	wdt_16c64.cod nwdt_16c64.cod wdt_18f452.cod nwdt_18f452.cod \
	wdt_10f200.cod nwdt_16f648a.cod wdt_18f4620.cod \
	nwdt_18f4620.cod pwdt_16f1823.cod nwdt_16f1823.cod


p%.cod : p%.o
	gplink --map -s $*.lkr -o $@  $<

wdt_%.cod : wdt_%.o
	gplink --map -s $*.lkr -o $@  $<

nwdt_%.cod : nwdt_%.o
	gplink --map -s $*.lkr -o $@  $<

pwdt_%.cod : pwdt_%.o
	gplink --map -s $*.lkr -o $@  $<


sim: sim_wdt_16f88 sim_nwdt_16f88 sim_wdt_16f628 sim_nwdt_16f628 \
	sim_wdt_16c64 sim_nwdt_16c64 sim_wdt_18f452 sim_nwdt_18f452 \
	sim_wdt_10f200 sim_wdt_18f4620 sim_nwdt_18f4620

