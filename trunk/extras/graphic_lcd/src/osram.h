/*
   Copyright (C) 2007 T. Scott Dattalo

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

#if !defined(__OSRAM_H__)
#define __OSRAM_H__

#include <gtk/gtk.h>

#include "glcd.h"
// gpsim graphics LCD modules for OSRAM Pictivia(tm) OLED Displays

class OSRAM_128064PK27_Series : public gLCD_Module
{
public:
  OSRAM_128064PK27_Series(const char *new_name);
  ~OSRAM_128064PK27_Series();
  /*
  static Module *construct(const char *new_name);
  bool dataBusDirection();
  void Update(GtkWidget *pw =0);
  void UpdatePinState(ePins, char);
  void create_iopin_map();
  void create_widget();
  */
private:
  /*
  PortRegister *m_dataBus;
  LCD_InputPin *m_A0;
  LCD_InputPin *m_E1;
  LCD_InputPin *m_E2;
  LCD_InputPin *m_RW;
  */
};

#endif
