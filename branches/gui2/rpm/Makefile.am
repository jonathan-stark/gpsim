#! /bin/sh

# Copyright (C) 2001 Craig Franklin
# Scott Dattalo - copied from gpasm distro to gpsim
# 
# This file is part of gpsim.
# 
# gpsim is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# gpsim is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with gpsim; see the file COPYING.  If not, write to
# the Free Software Foundation, 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.  */

SRC_DIST = ${top_srcdir}/@PACKAGE@-@VERSION@.tar.gz
SPEC_TEMPLATE = template.spec
SPEC_FILE = @PACKAGE@-@VERSION@.spec
RPMSCRIPT = buildrpm

RPMPATH = `pwd`
SRCPATH = `cd ${top_srcdir} && pwd`

spec: $(SPEC_FILE)

$(SPEC_FILE): $(SPEC_TEMPLATE)
	rm -f $(SPEC_FILE).new
	sed \
	-e 's%\@VERSION\@%$(VERSION)%g' \
	-e 's%\@PACKAGE\@%$(PACKAGE)%g' \
	-e 's%\@SPEC_TEMPLATE\@%$(SPEC_TEMPLATE)%g' \
	-e 's%\@NAME\@%$(NAME)%g' \
	-e 's%\@EMAIL\@%$(EMAIL)%g' \
	$(SPEC_TEMPLATE) > $(SPEC_FILE).new
	mv -f $(SPEC_FILE).new $(SPEC_FILE)

$(RPMSCRIPT):
	echo -e "#! /bin/sh \
          \nrpm \
	  --define '_topdir    $(RPMPATH)' \
	  --define '_specdir   $(RPMPATH)' \
	  --define '_rpmdir    $(RPMPATH)' \
	  --define '_srcrpmdir $(RPMPATH)' \
	  --define '_sourcedir $(SRCPATH)' \
	  -bb $(SPEC_FILE)" > $(RPMSCRIPT)
	chmod a+x $(RPMSCRIPT)
        
rpm: $(SPEC_FILE) $(RPMSCRIPT)
	if !(test -f $(SRC_DIST)); then \
	    echo "run \"make distcheck\" in the top level first"; \
	    exit 1; \
	fi;
	rm -Rf BUILD
	mkdir BUILD
	./$(RPMSCRIPT)
	rm -Rf BUILD

clean:
	rm -rf $(SPEC_FILE) $(RPMSCRIPT) *.rpm i386 BUILD

EXTRA_DIST = README template.spec
