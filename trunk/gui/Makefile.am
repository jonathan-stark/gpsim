# gpsim gui

INCLUDES = @X_CFLAGS@ @Y_CFLAGS@

lib_LTLIBRARIES = libgui.la

libgui_la_SOURCES =  gui_break.c gui_callbacks.c gui_dialog.c gui_init.c gui_main.c \
	gui_menu.c gui_processor.c gui_regwin.c gui_src.c gui_src_asm.c \
	gui_src_opcode.c gui_statusbar.c gui.h gui_callbacks.h gui_interface.h \
	gui_symbols.c gui_watch.c

libgui_la_LDFLAGS = $(shell gtk-config --libs) $(shell gtkextra-config --libs)

pixmapdir = $(datadir)/gpsim

#pixmap_DATA = break.xpm  bullet.xpm  center.xpm  font.xpm  font_large.xpm \
#	left.xpm  paint.xpm  pc.xpm  right.xpm

#CFLAGS = @CFLAGS@ -D DATADIR=\"$(pixmapdir)/\"
#CXXFLAGS = @CXXFLAGS@ -D DATADIR=\"$(pixmapdir)/\"
#CPPFLAGS = @CPPFLAGS@ -D DATADIR=\"$(pixmapdir)/\"

CFLAGS = @CFLAGS@ -D DATADIR=\"~/.gpsim/\"
CXXFLAGS = @CXXFLAGS@ -D DATADIR=\"~/.gpsim/\"
CPPFLAGS = @CPPFLAGS@ -D DATADIR=\"~/.gpsim/\"
