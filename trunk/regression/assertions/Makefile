SCRIPT = 16f873.lkr
OBJECTS =  assertions.o
OUTPUT = assertions.hex

all : $(OUTPUT)

$(OUTPUT) : $(OBJECTS) $(SCRIPT)
	gplink --map -s $(SCRIPT) -o $(OUTPUT) $(OBJECTS)

%.o : %.asm
	gpasm -c $<

clean:
	rm -f *~ *.o *.lst *.map *.hex

assertions.o : assertions.asm
