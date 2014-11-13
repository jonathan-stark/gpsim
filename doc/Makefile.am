
gpsim.ps: gpsim.lyx
	lyx -e ps gpsim.lyx

gpsim.pdf: gpsim.lyx
	lyx -e pdf gpsim.lyx

EXTRA_DIST = \
	screenshots/control.png
	screenshots/breadboard.png \
	screenshots/registerview.png \
	screenshots/scope1.png \
	screenshots/scope2.png  \
	screenshots/source_browser.png \
	makefile.mingw \
	gpsim.lyx \
	gpsim.pdf \
	gpsim.ps

MOSTLYCLEANFILES =  *~
CLEANFILES =  *~
DISTCLEANFILES =  *~
MAINTAINERCLEANFILES =  gpsim.ps gpsim.pdf *~
