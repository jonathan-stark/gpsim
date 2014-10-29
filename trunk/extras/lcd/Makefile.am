# Makefile for the LCD module
# 

AM_CPPFLAGS = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_lcd.la

libgpsim_lcd_la_SOURCES = \
	lcdgui.cc module_manager.cc lcd.cc hd44780.cc

libgpsim_lcd_la_LDFLAGS =  @X_LDFLAGS@



EXTRA_DIST = lcd.gif caps.pl lcd.xpm lcdfont.h lcdfont.inc README lcd.h  \
	makefile.mingw hd44780.h t.gif


SUBDIRS = examples


CFLAGS = @CFLAGS@
