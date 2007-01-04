include ../make.regression



all : ccp_877a.cod pwm_877a.cod



ccp_877a.cod : ccp_877a.o
	gplink --map -s 16f877a.lkr -o $@  $^

pwm_877a.cod : pwm_877a.o
	gplink --map -s 16f877a.lkr -o $@  $^

sim: sim_ccp_877a sim_pwm_887a
