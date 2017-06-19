/*
   Copyright (C) 1999 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __CMD_MODULE_H__
#define __CMD_MODULE_H__

#include <list>
using namespace std;
#include "command.h"

class cmd_module : public command
{
public:

  cmd_module(void);

  void module(void);

  void module(cmd_options *opt);

  void module(cmd_options_str *cos, list <string> *);

private:

  void module(cmd_options_str *cos);
  void module(cmd_options_str *cos, const char *op1);


};

extern cmd_module c_module;
#endif

