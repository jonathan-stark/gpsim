/*
   Copyright (C) 2004
   T. Scott Dattalo

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


#ifndef __GUI_STATUSBAR_H__
#define __GUI_STATUSBAR_H__

#include "gui.h"


class LabeledEntry;    // in gui_statusbar.cc
class MemoryAccess;    // in src/processor.h

//
// The Status Bar window 
//

class StatusBar_Window {
 public:
  GUI_Processor *gp;

  LabeledEntry *status;
  LabeledEntry *W;
  LabeledEntry *pc;
  LabeledEntry *cpu_cycles;
  LabeledEntry *time;
  
  bool created;

  StatusBar_Window(void);
  void NewProcessor(GUI_Processor *_gp, MemoryAccess *);
  void Create(GtkWidget *vbox_main);
  void Update(void);

private:
  MemoryAccess *ma;
  GtkWidget *hbox;

};

#endif //__GUI_STATUSBAR_H__
