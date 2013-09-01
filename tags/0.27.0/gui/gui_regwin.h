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

#include "gui_register.h"


class StatusBar_Window;

//======================================================================
// The register window 
//
#define REGISTERS_PER_ROW    16
#define MAX_ROWS ((MAX_REGISTERS)/(REGISTERS_PER_ROW))

class Register_Window : public GUI_Object
{
 public:

  // This array is indexed with row, and gives the address of the
  // first cell in the given row.
  int row_to_address[MAX_ROWS];

  char normalfont_string[256];
#if GTK_MAJOR_VERSION >= 2
  PangoFontDescription *normalfont;
#else
  GdkFont *normalfont;
#endif
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
  virtual gboolean UpdateRegisterCell(unsigned int reg_number);
  GUIRegister *operator [] (int address);
  int column_width(int col);
  int row_height(int row);

  Register_Window(GUI_Processor *gp);
  Register_Window();

 private:
  // Formatting
  int register_size;    // The size (in bytes) of a single register
  char *pCellFormat;    // dynamically created format string
  int char_width;       // nominal character width.
  int char_height;      // nominal character height
  int chars_per_column; // width of 1 column
};



class RAM_RegisterWindow : public Register_Window
{
 public:
  RAM_RegisterWindow(GUI_Processor *gp);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void Update();

  StatusBar_Window *sbw;

};

class EEPROM_RegisterWindow : public Register_Window
{
 public:
  EEPROM_RegisterWindow(GUI_Processor *gp);
  virtual void NewProcessor(GUI_Processor *gp);

};


#endif // __GUI_REGWIN_H__
