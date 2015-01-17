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
  GtkWidget *symbol_view;
  GtkListStore *symbol_list;
    
  GtkWidget *popup_menu;
  
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
  void SelectSymbolName(const char *name);

protected:
  virtual const char *name();

  // Signals
  static void toggle_addresses (GtkToggleButton *button, Symbol_Window *sw);
  static void toggle_constants (GtkToggleButton *button, Symbol_Window *sw);
  static void toggle_registers (GtkToggleButton *button, Symbol_Window *sw);
  static gboolean do_popup(GtkWidget *widget, GdkEventButton *event, Symbol_Window *sw);
  static void symbol_list_row_selected(GtkTreeSelection *treeselection,
    gpointer user_data);
  static gboolean delete_event(GtkWidget *widget, GdkEvent  *event,
    Symbol_Window *sw);

  GtkWidget *build_menu(GtkWidget *sheet);
  void do_symbol_select(Value *e);
};



#endif // __GUI_SYMBOLS_H__

