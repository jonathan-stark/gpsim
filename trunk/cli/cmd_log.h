/*
   Copyright (C) 1999,2000,2001,2002 T. Scott Dattalo

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

#ifndef __CMD_LOG_H__
#define __CMD_LOG_H__

#include <glib.h>
#include "expr.h"

class cmd_log : public command
{
public:

  cmd_log(void);
  void log(void);
  void log(cmd_options *opt, ExprList_t *el);
  void log(cmd_options *opt, char *, ExprList_t *el);

private:
  void log(cmd_options *opt);
  void log(cmd_options *opt, char *str,guint64,guint64);
  void log(cmd_options *opt, guint64,guint64,guint64);

};

extern cmd_log c_log;
#endif

