# Makefile for the usart_con module
# 
# running 'automake' within the same directory
# of this file will generate Makefile.in . Then
# running ./configure will generate the Makefile
# that is used to build this project

INCLUDES = @X_CFLAGS@

lib_LTLIBRARIES = libgpsim_usart_con.la

libgpsim_usart_con_la_SOURCES = \
	usart_con.cc module_manager.cc

libgpsim_usart_con_la_LDFLAGS =  @X_LDFLAGS@

EXTRA_DIST = usart_con.h autogen.sh makefile.mingw

CFLAGS = @CFLAGS@
