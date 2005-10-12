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
STARTUP_STC=startup.stc

all : $(OUTPUT)

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

$(PROJECT).o : $(PROJECT).asm

sim: $(COD)
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$(STC)
