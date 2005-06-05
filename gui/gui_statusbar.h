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


class RegisterLabeledEntry;	// in gui_statusbar.cc
class CyclesLabeledEntry;	// in gui_statusbar.cc
class TimeLabeledEntry;		// in gui_statusbar.cc
class MemoryAccess;		// in src/processor.h
class StatusBar_Window;

//========================================================================
//
// A LabeledEntry is an object consisting of gtk entry
// widget that is labeled (with a gtk lable widget)
//
/*
class LabeledEntry {
public:
  GtkWidget *label;
  GtkWidget *entry;
  StatusBar_Window *sbw;

  LabeledEntry(void);
  ~LabeledEntry();
  void Create(GtkWidget *box,char *clabel, int string_width, bool isEditable);
  void SetEntryWidth(int string_width);
  void NewLabel(char *clabel);
  virtual void Update(void);
  void AssignParent(StatusBar_Window *);
  virtual void put_value(unsigned int);

};

class RegisterLabeledEntry : public LabeledEntry {
public:

  Register *reg;
  char *pCellFormat;

  RegisterLabeledEntry(GtkWidget *,Register *,bool isEditable=true);
  ~RegisterLabeledEntry();

  virtual void put_value(unsigned int);
  void AssignRegister(Register *new_reg);
  virtual void Update(void);

};
*/

//
// The Status Bar window 
//

class StatusBar_Window {
 public:
  GUI_Processor *gp;

  CyclesLabeledEntry *cpu_cycles;
  TimeLabeledEntry *time;
  list<RegisterLabeledEntry *> entries;

  /*
  class RegisterLabeledList : public list<RegisterLabeledEntry*> {
  public:
    void push_back(GtkWidget *hbox, Register *reg) {
      list<RegisterLabeledEntry*>::push_back(new RegisterLabeledEntry(hbox, reg));
    }
    void clear() {
      iterator iRLE;
      for(iRLE = begin();
          iRLE != end();
          ++iRLE) {
        delete *iRLE;
      }
      list<RegisterLabeledEntry*>::clear();
    }
  };
  RegisterLabeledList entries;
  */

  StatusBar_Window(void);
  void NewProcessor(GUI_Processor *_gp, MemoryAccess *);
  void Create(GtkWidget *vbox_main);
  void Update(void);


private:
  MemoryAccess *ma;
  bool created;
  GtkWidget *hbox;

};

#endif //__GUI_STATUSBAR_H__
