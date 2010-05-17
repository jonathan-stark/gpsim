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


#include <config.h>

#ifdef HAVE_GUI


#include "glcd.h"
#include <src/trace.h>

//------------------------------------------------------------------------
gLCD::gLCD(GtkWidget *darea,
	   unsigned int cols,
	   unsigned int rows,
	   unsigned int pixel_size_x,
	   unsigned int pixel_size_y,
           unsigned int pixel_gap,
           unsigned int nColors
	   )
  : m_darea(darea), //m_parent(parent),
    m_nColumns(cols), m_nRows(rows), m_border(3),
    m_xPixel(pixel_size_x), m_yPixel(pixel_size_y),
    m_pixelGap(pixel_gap),
    m_nColors(nColors)
{

  printf("gLCD constructor %p, m_nColumns:%d, m_nRows:%d\n",this,m_nColumns,m_nRows);

  //g_assert(m_parent != NULL);
  g_assert(m_darea != NULL);

  // Allocate an RGB buffer for rendering the LCD screen.
  rgbbuf = new guchar[(m_nColumns+m_border*2)*m_xPixel * 
                      (m_nRows+m_border*2)*m_yPixel * 
                      3];

  m_Colors = new LCDColor[m_nColors];
  memset ((void *)m_Colors, 0, m_nColors*sizeof(LCDColor));

  if (m_nColors > 0)
    setColor(0, 0x78,0xa8,0x78);

  if (m_nColors > 1)
    setColor(1, 0x11,0x33,0x11);
}

gLCD::~gLCD()
{
  delete [] rgbbuf;
}

void gLCD::clear()
{
  unsigned int sz = m_xPixel*(m_nColumns+2*m_border) * m_yPixel*(m_nRows+2*m_border);
  guchar *pos = rgbbuf;

  guchar r = m_nColors > 0 ? m_Colors[0].r : 0x78;
  guchar g = m_nColors > 0 ? m_Colors[0].g : 0xa8;
  guchar b = m_nColors > 0 ? m_Colors[0].b : 0x78;

  for (unsigned int i=0; i<sz; i++) {

    *pos++ = r;
    *pos++ = g;
    *pos++ = b;
    
  }
    
}

void gLCD::refresh()
{
  gdk_draw_rgb_image (m_darea->window, m_darea->style->fg_gc[GTK_STATE_NORMAL],
                      0, 0, (m_nColumns+2*m_border)*m_xPixel, (m_nRows+2*m_border)*m_yPixel,
                      GDK_RGB_DITHER_MAX, rgbbuf, (m_nColumns+2*m_border)*m_xPixel *3);

}

void gLCD::setPixel(unsigned int col, unsigned int row, guchar r, guchar g, guchar b)
{
  int x = (col + m_border) * m_xPixel;
  int y = (row + m_border) * m_yPixel;

  // L = length of a row of pixels
  int L = (m_nColumns+2*m_border)*m_xPixel;
  unsigned int px = m_xPixel-m_pixelGap;
  unsigned int py = m_yPixel-m_pixelGap;

  for (int j=0; j<py; j++) {

    y = (row + m_border) * m_yPixel+j;
    guchar *pos = &rgbbuf[3* (y*L + x)];

    for (int i=0; i<px; i++) {
      /*
      if (*pos != r) 
        printf ("   x:%d y:%d r:%d g:%d b:%d\n",x,y,r,g,b);
      */
      *pos++ = r;
      *pos++ = g;
      *pos++ = b;
    }
  }

}

void gLCD::setPixel(unsigned int col, unsigned int row)
{
  if (col < m_nColumns && row < m_nRows) {

    guchar r = m_nColors > 1 ? m_Colors[1].r : 0x11;
    guchar g = m_nColors > 1 ? m_Colors[1].g : 0x33;
    guchar b = m_nColors > 1 ? m_Colors[1].b : 0x11;

    setPixel(col, row, r,g,b);
  }
}


void gLCD::setPixel(unsigned int col, unsigned int row, unsigned int colorIdx)
{
  if (colorIdx < m_nColors)
    setPixel(col, row, m_Colors[colorIdx].r,m_Colors[colorIdx].g,m_Colors[colorIdx].b);
}

void gLCD::setColor(unsigned int colorIdx, guchar r, guchar g, guchar b)
{

  if (colorIdx < m_nColors) {

    m_Colors[colorIdx].r = r;
    m_Colors[colorIdx].g = g;
    m_Colors[colorIdx].b = b;

  }
}


//========================================================================
//
// LCD interface to the simulator
//
gLCD_Interface::gLCD_Interface(gLCD_Module *_lcd) 
  : Interface ( (gpointer *) _lcd), plcd(_lcd)
{

}

//--------------------------------------------------
// SimulationHasStopped (gpointer)
//
// gpsim will call this function when the simulation
// has halt (e.g. a break point was hit.)

void gLCD_Interface::SimulationHasStopped (gpointer)
{
  if(plcd)
    plcd->Update();
}

void gLCD_Interface::Update (gpointer)
{
  if(plcd)
    plcd->Update();
}

//========================================================================
//
// 

gLCD_Module::gLCD_Module(const char *new_name, const char *desc, 
                         unsigned int nCols, unsigned int nRows)
  : Module(new_name,desc),
    window(0),darea(0),m_plcd(0),m_nColumns(nCols), m_nRows(nRows)
{
#if IN_BREADBOARD==0
  interface = new gLCD_Interface(this);
  get_interface().add_interface(interface);
#endif

}

void gLCD_Module::Update(GtkWidget *pw)
{

}

//========================================================================
//
LcdPortRegister::LcdPortRegister(gLCD_Module *plcd, const char * _name, const char *_desc)
  : PortRegister(plcd, _name,_desc,8, 0), m_pLCD(plcd)
{
  mMTT = new ModuleTraceType(plcd,1," Graphic LCD");
  trace.allocateTraceType(mMTT);

  RegisterValue rv(mMTT->type(), mMTT->type() + (1<<22));
  set_write_trace(rv);
  rv = RegisterValue(mMTT->type()+(2<<22), mMTT->type() + (3<<22));
  set_read_trace(rv);
}

LcdPortRegister::~LcdPortRegister()
{
  delete mMTT;
}

#endif //HAVE_GUI
