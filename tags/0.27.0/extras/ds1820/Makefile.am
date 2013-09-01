# Makefile for the DS1820 module
# 

AM_CPPFLAGS = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_ds1820.la

libgpsim_ds1820_la_SOURCES = \
	module_manager.cc ds1820.cc bit1w.cc rom1w.cc

libgpsim_ds1820_la_LDFLAGS =  @X_LDFLAGS@



EXTRA_DIST = ds1820.h makefile.mingw bit1w.h  rom1w.h

SUBDIRS = examples

CFLAGS = @CFLAGS@
