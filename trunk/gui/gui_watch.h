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

#ifndef __GUI_WATCH_H__
#define __GUI_WATCH_H__

class WatchEntry : public GUIRegister {
public:

  Processor *cpu;
  REGISTER_TYPE type;

};



//
// The watch window
//
class Watch_Window : public  GUI_Object
{
 public:

  GList *watches;

  int current_row;
  int current_column;
    
  GtkWidget *watch_clist;
  GtkWidget *popup_menu;

  Watch_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void ClearWatches(void);
  virtual void ClearWatch(WatchEntry *entry);
  virtual void UpdateWatch(WatchEntry *entry);
  //virtual void Add(unsigned int pic_id, REGISTER_TYPE type, int address, Register *reg=0);
  //virtual void Add(GUIRegister *reg);
  virtual void Add(REGISTER_TYPE type,GUIRegister *reg);
  virtual void Update(void);
  virtual void UpdateMenus(void);
  
};


#endif //__GUI_WATCH_H__
