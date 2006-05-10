## Process this file with automake to produce Makefile.in

INCLUDES = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_modules.la

libgpsim_modules_la_SOURCES = \
	gpsim_modules.cc \
	resistor.cc resistor.h \
	usart.cc usart.h \
	switch.cc switch.h \
	logic.cc logic.h \
	led.cc led.h \
	push_button.cc push_button.h \
	encoder.cc encoder.h \
	i2c-eeprom.cc i2c-eeprom.h \
	ttl.cc ttl.h

##	paraface.cc paraface.h \  # TSD - removed 16APR06
##	video.cc video.h \ # TSD - removed 17APR06

libgpsim_modules_la_LDFLAGS =  @X_LDFLAGS@

