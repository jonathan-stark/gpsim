#!/usr/bin/awk

# configure_win32.awk - Genarate config.h using config_win32.h.in as template
#                       and insert the version number definitions from configure.in
#
# Written By - Borut Razem borut.razem@siol.net
#
# This file is part of gpsim.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

BEGIN {
  # get the version values from configure.in

  FS="=";
  while (getline <"configure.in" > 0) {
    if ($0 ~ "^GPSIM_MAJOR_VERSION=")
      majv = $2;
    else if ($0 ~ "^GPSIM_MINOR_VERSION=")
      minv = $2;
    else if ($0 ~ "^GPSIM_MICRO_VERSION=")
      micv = $2;
  }
  print "/* config.h                                                     */";
  print "/* Generated automatically by configure_win32.awk, DO NOT edit! */";
  print "/* To make changes to config.h edit config_win32.h.in instead.  */";
  print "" 

}

/^#define GPSIM_MAJOR_VERSION/ {
  print("#define GPSIM_MAJOR_VERSION " majv);
  next
}

/^#define GPSIM_MINOR_VERSION/ {
  print("#define GPSIM_MINOR_VERSION " minv);
  next
}

/^#define GPSIM_MICRO_VERSION/ {
  print("#define GPSIM_MICRO_VERSION " micv);
  next
}

{
  print;
}
