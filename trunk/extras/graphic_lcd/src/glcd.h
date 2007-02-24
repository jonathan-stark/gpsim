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


#if !defined(__GLCD_H__)
#define __GLCD_H__

#include <gtk/gtk.h>
#include <gpsim/modules.h>
#include <gpsim/gpsim_interface.h>

#define IN_BREADBOARD 0

//========================================================================
// gLCD - graphic LCD
//
// gLCD implements the graphical component of the gpsim graphical LCD
// modules. The I/O pins and various LCD controllers chips are implemented
// elsehere.

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

//========================================================================
class gLCD_Module; 
class gLCD_Interface : public Interface
{
private:
  gLCD_Module *plcd;

public:

  //virtual void UpdateObject (gpointer xref,int new_value);
  //virtual void RemoveObject (gpointer xref);
  virtual void SimulationHasStopped (gpointer object);
  //virtual void NewProcessor (unsigned int processor_id);
  //virtual void NewModule (Module *module);
  //virtual void NodeConfigurationChanged (Stimulus_Node *node);
  //virtual void NewProgram  (unsigned int processor_id);
  virtual void Update  (gpointer object);

  gLCD_Interface(gLCD_Module *_gp);
};


//========================================================================
// gLCD_Module - base class of graphic's LCD modules
// 
class gLCD_Module : public Module
{
public:

  gLCD_Module(const char *new_name, const char *desc,
              unsigned int nCols, unsigned int nRows);
  virtual void Update(GtkWidget *pw =0);

protected:
  GtkWidget    *window;
  GtkWidget    *darea;         // Drawable containing the graphics

  gLCD         *m_plcd;
  unsigned int  m_nColumns;
  unsigned int  m_nRows;
  gLCD_Interface *interface;

};
#endif
