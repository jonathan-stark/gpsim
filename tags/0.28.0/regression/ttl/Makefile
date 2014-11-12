
include ../make.regression


SCRIPT = 18f452.lkr


all : ttl377.cod ttl165.cod

ttl%.cod : ttl%.o
	gplink --map -s $(SCRIPT) -o $@  $<

sim : sim_ttl377 sim_ttl165

