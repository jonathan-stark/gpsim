# Makefile for the i2c2par module
# 

AM_CPPFLAGS = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_i2c2par.la

libgpsim_i2c2par_la_SOURCES = \
	module_manager.cc i2c2par.cc

libgpsim_i2c2par_la_LDFLAGS =  @X_LDFLAGS@



EXTRA_DIST = i2c2par.h makefile.mingw


SUBDIRS = examples


CFLAGS = @CFLAGS@
