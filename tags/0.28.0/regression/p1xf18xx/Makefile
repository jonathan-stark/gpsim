# P12f1822, P16F1823  processor regression tests
#
#

include ../make.regression


all :   p12f1822.cod  p16f1823.cod p16f1823_spi.cod p16f1823_i2c.cod p16f1823_i2c_v2.cod p16f1823_comp.cod p16f1788.cod p12f1822_usart.cod

p16f1823_i2c_v2.o : p16f1823_i2c_v2.asm i2c_low.inc


p%_spi.cod : p%_spi.o
	gplink --map -s $*_g.lkr -o $@  $<

p%_usart.cod : p%_usart.o
	gplink --map -s $*_g.lkr -o $@  $<

p%_i2c.cod : p%_i2c.o
	gplink --map -s $*_g.lkr -o $@  $<

p%_comp.cod : p%_comp.o
	gplink --map -s $*_g.lkr -o $@  $<

p%_i2c_v2.cod : p%_i2c_v2.o
	gplink --map -s $*_g.lkr -o $@  $<

p%.cod : p%.o
	gplink --map -s $*_g.lkr -o $@  $<

sim: sim_p12f1822 sim_p16f1823 sim_p16f1823_i2c_v2 sim_p16f1823_i2c sim_p16f1823_comp sim_p16f1823_spi sim_p12f1822_usart sim_p16f1788

