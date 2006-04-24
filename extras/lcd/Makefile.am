# Makefile for the LCD module
# 
# running 'automake' within the same directory
# of this file will generate Makefile.in . Then
# running ./configure will generate the Makefile
# that is used to build this project

INCLUDES = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_lcd.la

libgpsim_lcd_la_SOURCES = \
	lcdgui.cc module_manager.cc lcd.cc hd44780.cc

libgpsim_lcd_la_LDFLAGS =  @X_LDFLAGS@



EXTRA_DIST = lcd.gif caps.pl lcd.xpm lcdfont.h lcdfont.inc README lcd.h  \
	autogen.sh makefile.mingw hd44780.h


SUBDIRS = examples


CFLAGS = @CFLAGS@
