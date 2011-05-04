INCLUDES = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_graphicLCD.la

libgpsim_graphicLCD_la_SOURCES = \
	gpsim_modules.cc \
	glcd.cc glcd.h \
	glcd_100X32_sed1520.cc glcd_100X32_sed1520.h \
	osram.cc osram.h \
	sed1520.cc sed1520.h \
	ssd0323.cc ssd0323.h

libgpsim_graphicLCD_la_LDFLAGS =  @X_LDFLAGS@

EXTRA_DIST = makefile.mingw
