## Process this file with automake to produce Makefile.in

INCLUDES = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim.la
gpsimincludedir = $(includedir)/gpsim

libgpsim_la_SOURCES = \
	12bit-hexdecode.cc \
	12bit-processors.cc \
	14bit-instructions.cc \
	14bit-processors.cc \
	14bit-registers.cc \
	14bit-tmrs.cc \
	14bit-hexdecode.cc \
	16bit-instructions.cc \
	16bit-processors.cc \
	16bit-registers.cc \
	16bit-hexdecode.cc \
	16bit-tmrs.cc \
	attributes.cc \
	bitlog.cc \
	bytelog.cc \
	breakpoints.cc \
	cod.cc \
	cmd_manager.cc \
	eeprom.cc \
	errors.cc \
	i2c-ee.cc \
	fopen-path.cc \
	gpsim_object.cc \
	gpsim_time.cc \
	hexutils.cc \
	init.cc \
	intcon.cc \
	interface.cc \
	ioports.cc \
        lxt_write.c \
	modules.cc \
	os_dependent.cc \
	p12x.cc \
	p16f62x.cc \
	p16x8x.cc \
	p16f87x.cc \
	p16x7x.cc \
	p16x5x.cc \
	p16x6x.cc \
	p17c75x.cc \
	p18x.cc \
	packages.cc \
	pic-processor.cc \
	pic-registers.cc \
	pic-instructions.cc \
	pie.cc \
	pir.cc \
	processor.cc \
	protocol.cc \
	registers.cc \
	sim_context.cc \
	stimuli.cc \
	symbol.cc \
	tmr0.cc \
	trace.cc \
	trigger.cc \
	ttoken.cc \
	uart.cc \
	ssp.cc \
	xref.cc \
	icd.cc \
	expr.cc	\
	operator.cc \
	value.cc

gpsiminclude_HEADERS = \
	12bit-instructions.h \
	12bit-processors.h \
	14bit-instructions.h \
	14bit-processors.h \
	14bit-registers.h \
	14bit-tmrs.h \
	16bit-instructions.h \
	16bit-processors.h \
	16bit-registers.h \
	16bit-tmrs.h \
	attributes.h \
	bitlog.h \
	breakpoints.h \
	bytelog.h \
	cmd_gpsim.h \
	cmd_manager.h \
	cod.h \
	eeprom.h \
	i2c-ee.h \
	fopen-path.h \
	gpsim_classes.h \
	gpsim_def.h \
	gpsim_interface.h \
	gpsim_object.h \
	gpsim_time.h \
	intcon.h \
	interface.h \
	ioports.h \
	lxt_write.h \
	modules.h \
	operator.h \
	p12x.h \
	p16x5x.h \
	p16f62x.h \
	p16x6x.h \
	p16x7x.h \
	p16x8x.h \
	p16f87x.h \
	p17c75x.h \
	p18x.h \
	packages.h \
	pic-instructions.h \
	pic-packages.h \
	pic-processor.h \
	pic-registers.h \
	picdis.h \
	pie.h \
	pir.h \
	processor.h \
	protocol.h \
	pthread-wrap.h \
	registers.h \
	sim_context.h \
	stimuli.h \
	stimulus_orb.h \
	symbol.h \
	symbol_orb.h \
	tmr0.h \
	trace.h \
	trigger.h \
	trace_orb.h \
	ttoken.h \
	uart.h \
	xref.h \
	icd.h \
	ssp.h \
	errors.h \
	expr.h \
	value.h



#libgpsim_la_LDFLAGS = $(shell gtk-config --libs)
libgpsim_la_LDFLAGS =  @X_LDFLAGS@ 

