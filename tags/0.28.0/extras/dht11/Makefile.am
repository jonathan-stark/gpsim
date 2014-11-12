# Makefile for the DHT11 module
# 

AM_CPPFLAGS = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_dht11.la

libgpsim_dht11_la_SOURCES = \
	dht11.cc module_manager.cc

libgpsim_dht11_la_LDFLAGS =  @X_LDFLAGS@



EXTRA_DIST = dht11.h README AUTHORS makefile.mingw

SUBDIRS = examples


CFLAGS = @CFLAGS@
