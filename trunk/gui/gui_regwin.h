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


#ifndef __GUI_REGWIN_H__
#define __GUI_REGWIN_H__

#include "gui_object.h"
#include "gui_register.h"
#include "gtkextra/gtksheet.h"

#include <string>

class GUI_Processor;

//======================================================================
// The register window
//
#define REGISTERS_PER_ROW    16
#define MAX_ROWS ((MAX_REGISTERS)/(REGISTERS_PER_ROW))

// Base class for RAM_RegisterWindow and EEPROM_RegisterWindow

class Register_Window : public GUI_Object
{
 public:

  // This array is indexed with row, and gives the address of the
  // first cell in the given row.
  int row_to_address[MAX_ROWS];

  std::string normalfont_string;
  PangoFontDescription *normalfont;

  GtkStyle *current_line_number_style;
  GtkStyle *breakpoint_line_number_style;

  REGISTER_TYPE type;
  GUIRegisterList *registers;
  GtkSheet *register_sheet;

  RegisterMemoryAccess *rma;  // Apointer to the Processor's rma or ema.

  GtkWidget *entry;
  GtkWidget *location;
  GtkWidget *popup_menu;

  int registers_loaded; // non zero when registers array is loaded


  virtual void Build();
  int LoadStyles();
  int SettingsDialog();
  void UpdateStyle();
  void SetRegisterSize();
  virtual void Update();
  virtual void UpdateASCII(int row);
  virtual void UpdateLabel();
  virtual void UpdateEntry();
  virtual void UpdateLabelEntry();
  virtual void SelectRegister(Value *);
  virtual void SelectRegister(int reg_number);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual GUIRegister *getRegister(int row, int col);
  virtual gboolean UpdateRegisterCell(int reg_number);
  GUIRegister *operator [] (int address);
  int column_width(int col);
  int row_height(int row);

protected:
  Register_Window(GUI_Processor *gp, REGISTER_TYPE type);

 private:
  GtkWidget *build_menu();
  void do_popup(GtkWidget *widget, GdkEventButton *event);
  static gboolean button_press(GtkWidget *widget, GdkEventButton *event,
    Register_Window *rw);
  static gboolean popup_menu_handler(GtkWidget *widget, Register_Window *rw);

  // Formatting
  int register_size;    // The size (in bytes) of a single register
  int char_width;       // nominal character width.
  int char_height;      // nominal character height
  int chars_per_column; // width of 1 column
};



class RAM_RegisterWindow : public Register_Window
{
 public:
  explicit RAM_RegisterWindow(GUI_Processor *gp);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void Update();

protected:
  virtual const char *name();
};

class EEPROM_RegisterWindow : public Register_Window
{
 public:
  explicit EEPROM_RegisterWindow(GUI_Processor *gp);
  virtual void NewProcessor(GUI_Processor *gp);

protected:
  virtual const char *name();
};


#endif // __GUI_REGWIN_H__
