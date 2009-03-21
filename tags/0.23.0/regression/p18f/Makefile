
EXTENDED_INSTRUCTIONS=1

include ../make.regression


SCRIPT = 18f452.lkr

PROJECT = instructions
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
STC = $(PROJECT).stc

all : sim

instructions.cod: instructions.o $(SCRIPT)
	gplink --map -s $(SCRIPT) -o instructions.hex instructions.o

extended_instructions.cod: extended_instructions.o $(SCRIPT)
	gplink --map -s $(SCRIPT) -o extended_instructions.hex extended_instructions.o


test1: instructions.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<


test2: extended_instructions.cod
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<


sim: test1 test2
