# @PACKAGE@-@VERSION@.spec 
#
# Generated automatically from @SPEC_TEMPLATE@ by make.
#
# Copyright (C) 2001 Craig Franklin, Scott Dattalo
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
# Boston, MA 02111-1307, USA. 

Name: @PACKAGE@
Version: @VERSION@
Release: 1
Summary: A simulator for Microchip (TM) PIC (TM) microcontrollers
Copyright: GPL
Group: Development/Debuggers
Source: http://www.dattalo.com/gnupic/gpsim-%{version}.tar.gz
Packager: @NAME@ @EMAIL@
Distribution: Red Hat Linux
URL: http://www.dattalo.com/gnupic/gpsim.html
Buildroot: %{_builddir}/%{name}-%{version}-root

%description
gpsim is a simulator for Microchip (TM) PIC (TM) micro-controllers.
It supports most devices in Microchip's 12-bit, 14bit, and 16-bit
core families. In addition, gpsim supports dynamically loadable
modules such as LED's, LCD's, resistors, etc. to extend the simulation
enviroment beyond the PIC.

%prep
%setup

%build
./configure --prefix=/usr --disable-shared
make

%clean
rm -rf $RPM_BUILD_ROOT

%install
make DESTDIR="$RPM_BUILD_ROOT" install
./libtool --finish $RPM_BUILD_ROOT/usr/lib

%files
%defattr(-, bin, bin)
/usr/bin/gpsim
%doc AUTHORS COPYING ChangeLog INSTALL NEWS README 
%doc doc/gpsim.lyx doc/gpsim.pdf
