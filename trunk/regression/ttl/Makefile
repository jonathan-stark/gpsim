
EXTENDED_INSTRUCTIONS=1

include ../make.regression


SCRIPT = 18f452.lkr

PROJECT = ttl377
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
STC = $(PROJECT).stc
STARTUP_STC=startup.stc

all : sim_377

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

sim_377: $(COD)
	$(GPSIM) -i -c $(STARTUP_STC) -D STC="ttl377.cod"


