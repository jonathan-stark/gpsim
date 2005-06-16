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

#ifndef __GUI_SYMBOLS_H__
#define __GUI_SYMBOLS_H__


//
// The symbol window
//
class Symbol_Window : public GUI_Object
{
 public:

  GtkWidget *symbol_clist;
  GList *symbols;
    
  GtkWidget *popup_menu;
  
  int current_row;

  int filter_addresses;
  int filter_constants;
  int filter_registers;

  GtkWidget *addressesbutton;
  GtkWidget *constantsbutton;
  GtkWidget *registersbutton;

  int load_symbols;

  Symbol_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void Update(void);
  void NewSymbols(void);
  void SelectSymbolName(char *name);

};



#endif // __GUI_SYMBOLS_H__

