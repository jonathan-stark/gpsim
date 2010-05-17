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
#include <src/modules.h>
#include <src/gpsim_interface.h>
#include <src/ioports.h>

#define IN_BREADBOARD 0

struct LCDColor
{
  guchar r,g,b;
};
//========================================================================
// gLCD - graphic LCD
//
// gLCD implements the graphical component of the gpsim graphical LCD
// modules. The I/O pins and various LCD controllers chips are implemented
// elsehere.

class gLCD
{
public:
  gLCD(GtkWidget *darea, //GdkDrawable *parent, 
       unsigned int cols, 
       unsigned int rows,
       unsigned int pixel_size_x,
       unsigned int pixel_size_y,
       unsigned int pixel_gap,
       unsigned int nColors=2
       );
  ~gLCD();

  void refresh();
  void clear();
  void setPixel(unsigned int col, unsigned int row);
  void setPixel(unsigned int col, unsigned int row, unsigned int colorIdx);
  void setColor(unsigned int colorIdx, guchar r, guchar g, guchar b);

protected:
  GtkWidget    *m_darea;         // Drawable containing the graphic

  // LCD graphics are rendered here. 
  guchar *rgbbuf;

  unsigned int  m_nColumns;
  unsigned int  m_nRows;
  unsigned int  m_border;

  // pixel size Mapping between graphical pixmap and LCD.
  unsigned int  m_xPixel;
  unsigned int  m_yPixel;
  unsigned int  m_pixelGap;

  // Colors
  LCDColor *m_Colors;
  unsigned int m_nColors;

  void setPixel(unsigned int col, unsigned int row, guchar r, guchar g, guchar b);

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

//------------------------------------------------------------------------
class ModuleTraceType;
class LcdPortRegister : public PortRegister
{
public:
  LcdPortRegister(gLCD_Module *plcd, const char * _name, const char *_desc);
  virtual ~LcdPortRegister();
private:
  gLCD_Module *m_pLCD;
  ModuleTraceType *mMTT;
};

#endif
