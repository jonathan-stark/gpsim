# Makefile for the DS1307 module
# 

AM_CPPFLAGS = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_ds1307.la

libgpsim_ds1307_la_SOURCES = \
	module_manager.cc ds1307.cc

libgpsim_ds1307_la_LDFLAGS =  @X_LDFLAGS@



EXTRA_DIST = ds1307.h ChangeLog makefile.mingw


SUBDIRS = examples


CFLAGS = @CFLAGS@
