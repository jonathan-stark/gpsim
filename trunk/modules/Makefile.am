## Process this file with automake to produce Makefile.in

INCLUDES = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_modules.la

libgpsim_modules_la_SOURCES = \
	gpsim_modules.cc \
	binary_indicator.cc binary_indicator.h \
	resistor.cc resistor.h \
	usart.cc usart.h \
	paraface.cc paraface.h \
	switch.cc switch.h \
	logic.cc logic.h \
	led.cc led.h \
	video.cc video.h \
	push_button.cc push_button.h \
	encoder.cc encoder.h


libgpsim_modules_la_LDFLAGS =  @X_LDFLAGS@

