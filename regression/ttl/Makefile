
EXTENDED_INSTRUCTIONS=1

include ../make.regression


SCRIPT = 18f452.lkr

PROJECT = ttl377
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod

all : sim_377

$(OUTPUT) $(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

sim_377: $(COD)
	$(GPSIM) -i -I $(STARTUP_STC) -D STC=$<


