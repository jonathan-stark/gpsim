include ../make.regression



all : ccp_877a.cod pwm_877a.cod ccp_819.cod pwm_6520.cod pwm_26k22.cod



%.cod : %.o
	gplink --map  -o $@  $<

sim: sim_ccp_877a sim_pwm_877a sim_ccp_819 sim_pwm_6520 sim_pwm_26k22

