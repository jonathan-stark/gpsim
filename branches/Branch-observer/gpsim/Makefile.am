# gpsim gui

INCLUDES = @X_CFLAGS@ @Y_CFLAGS@

bin_PROGRAMS = gpsim

gpsim_SOURCES = main.cc

gpsim_LDFLAGS =  $(shell gtk-config --cflags)
gpsim_LDADD = ../src/libgpsim.la ../cli/libgpsimcli.la ../gui/libgui.la  @GTK@ @GDK@ @GLIB@ @X_LDFLAGS@ @Y_LDFLAGS@
