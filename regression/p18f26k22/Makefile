include ../make.regression

all: hlvd_26k22.cod  ctmu_26k22.cod tbl_26k22.cod

%.cod : %.o
	gplink --map  -o $@  $<

sim: sim_hlvd_26k22 sim_ctmu_26k22 sim_tbl_26k22

.PHONY: all clean
