# USART module regression test
#
#

include ../make.regression


SCRIPT = 16f877.lkr

PROJECT = usart_test
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
STC = $(PROJECT).stc

all : usart_pir1v1.cod usart_pir1v2.cod eusart.cod


sim: sim_pir1v1 sim_pir1v2 sim_eusart
 
sim_pir1v1: usart_pir1v1.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_pir1v2: usart_pir1v2.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

sim_eusart: eusart.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<

usart_pir1v1.cod : usart_pir1v1.o
	gplink --map -s 16f628.lkr -o $@  $<

usart_pir1v2.cod : usart_pir1v2.o
	gplink --map -s 16f877.lkr -o $@  $<

eusart.cod : eusart.o
	gplink --map -s 18f2321.lkr -o $@  $<

