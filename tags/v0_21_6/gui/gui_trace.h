/*
   Copyright (C) 1998,1999,2000,2001,2002,2003,2004
   T. Scott Dattalo and Ralf Forsberg

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

#ifndef __GUI_TRACE_H__
#define __GUI_TRACE_H__
//
// The trace window 
//

struct TraceMapping {

  guint64 cycle;
  int simulation_trace_index;
};

class Trace_Window : public GUI_Object
{
 public:

  GtkCList *trace_clist;
  guint64   last_cycle;   // The cycle of the last trace in the window.

  GtkWidget *location;
  GtkWidget *popup_menu;

  /* trace_flags bit definitions
     bit0 - enable xref updates to refresh the display
            0 disables, 1 enables
     bit1-bit31 are unused.
   */
  int trace_flags;

  /* trace_map is a pointer to an array of cross references
   * between the trace window and gpsim trace buffer */
  struct TraceMapping *trace_map;
  int trace_map_index;


  Trace_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void Update(void);
  virtual void NewProcessor(GUI_Processor *gp);

};




#endif // __GUI_TRACE_H__

