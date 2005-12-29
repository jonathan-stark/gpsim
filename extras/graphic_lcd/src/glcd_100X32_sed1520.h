/*
   Copyright (C) 2005 T. Scott Dattalo

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


#if !defined(__GLCD_100X32_SED1520_H__)
#define __GLCD_100X32_SED1520_H__


#include <gtk/gtk.h>

#include <gpsim/modules.h>

class SED1520;
class LCD_InputPin;
class PortRegister;

class gLCD
{
public:
  gLCD(GdkDrawable *parent, 
       unsigned int cols, 
       unsigned int rows,
       unsigned int pixel_size_x,
       unsigned int pixel_size_y
       );

  void       refresh();
  void       clear();
  void       setPixel(unsigned int col, unsigned int row);

protected:
  GdkDrawable  *m_parent;
  // Graphic's context for the LCD:
  GdkGC        *m_gc;

  // LCD graphics are rendered here. 
  GdkPixmap    *m_pixmap;

  unsigned int  m_nColumns;
  unsigned int  m_nRows;
  unsigned int  m_border;

  // pixel size Mapping between graphical pixmap and LCD.
  unsigned int  m_xPixel;
  unsigned int  m_yPixel;

  // LCD foreground and background colors
  enum {
    cFG=0,
    cBG=1
  };
  GdkColor m_aColors[2];

};

enum ePins {
  eA0,
  eE1,
  eE2,
  eRW
};

class gLCD_100X32_SED1520 : public Module
{
public:
  gLCD_100X32_SED1520(const char *new_name);
  ~gLCD_100X32_SED1520();

  static Module *construct(const char *new_name);
  bool dataBusDirection();
  void Update(_GtkWidget *);
  void UpdatePinState(ePins, char);
  void create_iopin_map();
  void create_widget();
private:

  PortRegister *m_dataBus;
  LCD_InputPin *m_A0;
  LCD_InputPin *m_E1;
  LCD_InputPin *m_E2;
  LCD_InputPin *m_RW;

  SED1520      *m_sed1;
  SED1520      *m_sed2;

  GtkWidget    *window;
  GtkWidget    *darea;         // Drawable containing the graphics

  gLCD         *m_plcd;
  unsigned int  m_nColumns;
  unsigned int  m_nRows;
};

#endif //__GLCD_100X32_SED1520_H__
