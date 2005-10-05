SCRIPT = 12c508.lkr

PROJECT = instructions_12bit
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
GPSIM = ../../gpsim/gpsim
STC = $(PROJECT).stc
STARTUP_STC=startup.stc

all : $(OUTPUT)

$(OUTPUT) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

$(COD) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

%.o : %.asm
	gpasm -c $<

clean:
	rm -f *~ *.o *.lst *.map *.hex

$(PROJECT).o : $(PROJECT).asm

sim: $(COD)
	$(GPSIM) -i -c $(STARTUP_STC) -D STC=$(STC)
