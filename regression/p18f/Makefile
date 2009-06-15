
EXTENDED_INSTRUCTIONS=1

include ../make.regression


SCRIPT = 18f452.lkr

all : instructions.cod extended_instructions.cod reset.cod

%.cod : %.o
	gplink --map -s $(SCRIPT) -o $@  $<

