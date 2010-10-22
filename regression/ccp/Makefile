include ../make.regression



all : ccp_877a.cod pwm_877a.cod ccp_819.cod pwm_6520.cod



ccp_877a.cod : ccp_877a.o
	gplink --map -s 16f877a.lkr -o $@  $^

ccp_819.cod : ccp_819.o
	gplink --map -s 16f819.lkr -o $@  $^

pwm_877a.cod : pwm_877a.o
	gplink --map -s 16f877a.lkr -o $@  $^

pwm_6520.cod : pwm_6520.o
	gplink --map -s 18f6520.lkr -o $@  $^

sim: sim_ccp_877a sim_pwm_877a sim_ccp_819 sim_pwm_6520

