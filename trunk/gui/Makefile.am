# gpsim gui

INCLUDES = @X_CFLAGS@ @Y_CFLAGS@

lib_LTLIBRARIES = libgpsimgui.la

libgpsimgui_la_SOURCES =  gui_break.c gui_callbacks.c gui_dialog.c gui_init.c gui_main.c \
	gui_menu.c gui_processor.c gui_regwin.c gui_src.c gui_src_asm.c \
	gui_src_opcode.c gui_statusbar.c gui.h gui_callbacks.h gui_interface.h \
	gui_symbols.c gui_watch.c gui_breadboard.c gui_stack.c gui_trace.c gui_profile.c

libgpsimgui_la_LDFLAGS = @X_LDFLAGS@ @Y_LDFLAGS@

pixmapdir = $(datadir)/gpsim

CFLAGS = @CFLAGS@ -D DATADIR=\"~/.gpsim/\"
CXXFLAGS = @CXXFLAGS@ -D DATADIR=\"~/.gpsim/\"
CPPFLAGS = @CPPFLAGS@ -D DATADIR=\"~/.gpsim/\"
