# gpsim  executable Makefile
#
#

INCLUDES = @X_CFLAGS@ @Y_CFLAGS@

bin_PROGRAMS = gpsim

gpsim_SOURCES = main.cc

#gpsim_LDFLAGS =  $(shell gtk-config --cflags)
gpsim_LDADD = ../src/libgpsim.la ../cli/libgpsimcli.la ../gui/libgpsimgui.la \
  ../eXdbm/libgpsim_eXdbm.la @GTK@ @GDK@ @GLIB@ -lstdc++ -lpopt \
  @X_LDFLAGS@ @Y_LDFLAGS@ @LIBREADLINE@

EXTRA_DIST = makefile.mingw
