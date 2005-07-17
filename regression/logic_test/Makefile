SCRIPT = 16f873.lkr
OBJECTS =  logic_test.o
OUTPUT = logic_test.hex

all : $(OUTPUT)

$(OUTPUT) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

%.o : %.asm
	gpasm -c $<

clean:
	rm -f *~ *.o *.lst *.map *.hex

logic_test.o : logic_test.asm
