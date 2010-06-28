# Makefile for the LCD module
# 
# running 'automake' within the same directory
# of this file will generate Makefile.in . Then
# running ./configure will generate the Makefile
# that is used to build this project

INCLUDES = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_ds1307.la

libgpsim_ds1307_la_SOURCES = \
	module_manager.cc ds1307.cc

libgpsim_ds1307_la_LDFLAGS =  @X_LDFLAGS@



EXTRA_DIST = ds1307.h ChangeLog makefile.mingw


SUBDIRS = examples


CFLAGS = @CFLAGS@
