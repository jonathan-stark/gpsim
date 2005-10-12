include ../make.regression


SCRIPT = 12c508.lkr

PROJECT = instructions_12bit
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
