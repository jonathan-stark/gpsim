# gpsim gui

INCLUDES = @X_CFLAGS@ @Y_CFLAGS@

lib_LTLIBRARIES = libgpsimgui.la

libgpsimgui_la_SOURCES =  gui_break.cc gui_callbacks.cc gui_dialog.cc gui_init.cc gui_main.cc \
	gui_menu.cc gui_processor.cc gui_regwin.cc gui_src.cc gui_src_asm.cc \
	gui_src_opcode.cc gui_statusbar.cc gui.h gui_callbacks.h gui_interface.h \
	gui_symbols.cc gui_watch.cc gui_breadboard.cc gui_stack.cc gui_trace.cc gui_profile.cc \
        gui_stopwatch.cc gui_object.cc

libgpsimgui_la_LDFLAGS = @X_LDFLAGS@ @Y_LDFLAGS@

pixmapdir = $(datadir)/gpsim

CFLAGS = @CFLAGS@ -D DATADIR=\"~/.gpsim/\"
CXXFLAGS = @CXXFLAGS@ -D DATADIR=\"~/.gpsim/\"
CPPFLAGS = @CPPFLAGS@ -D DATADIR=\"~/.gpsim/\"
