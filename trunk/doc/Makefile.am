
gpsim.ps: gpsim.lyx
	lyx -e ps gpsim.lyx

gpsim.pdf: gpsim.lyx
	lyx -e pdf gpsim.lyx

EXTRA_DIST = \
	makefile.mingw \
	gpsim.lyx \
	gpsim.pdf \
	gpsim.ps

MOSTLYCLEANFILES =  *~
CLEANFILES =  *~
DISTCLEANFILES =  *~
MAINTAINERCLEANFILES =  gpsim.ps gpsim.pdf *~
