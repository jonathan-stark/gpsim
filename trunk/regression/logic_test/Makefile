SCRIPT = 16f873.lkr

PROJECT = logic_test
OBJECTS =  $(PROJECT).o
OUTPUT = $(PROJECT).hex
COD = $(PROJECT).cod
GPSIM = ../../gpsim/gpsim
STC = $(PROJECT).stc

all : $(OUTPUT)

$(OUTPUT) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

%.o : %.asm
	gpasm -c $<

clean:
	rm -f *~ *.o *.lst *.map *.hex

$(PROJECT).o : $(PROJECT).asm

sim: $(COD) $(STC)
	$(GPSIM) -i -c $(STC)
