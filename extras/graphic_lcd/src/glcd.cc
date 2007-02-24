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


#include "glcd.h"

//------------------------------------------------------------------------

gLCD::gLCD(GdkDrawable *parent,
	   unsigned int cols,
	   unsigned int rows,
	   unsigned int pixel_size_x,
	   unsigned int pixel_size_y
	   )
  : m_parent(parent),
    m_nColumns(cols), m_nRows(rows), m_border(3),
    m_xPixel(pixel_size_x), m_yPixel(pixel_size_y)
{

  printf("gLCD constructor %p, m_nColumns:%d, m_nRows:%d\n",this,m_nColumns,m_nRows);

  g_assert(m_parent != NULL);

  m_gc = gdk_gc_new(m_parent);
  g_assert(m_gc!= (GdkGC*)NULL);

  m_aColors[cBG].red   = 0x7800;
  m_aColors[cBG].green = 0xa800;
  m_aColors[cBG].blue  = 0x7800;

  m_aColors[cFG].red   = 0x1100;
  m_aColors[cFG].green = 0x3300;
  m_aColors[cFG].blue  = 0x1100;

  gboolean gbSuccess=FALSE;
  gdk_colormap_alloc_colors (gdk_colormap_get_system(),
			     m_aColors, sizeof(m_aColors)/sizeof(m_aColors[0]),
			     TRUE, TRUE, &gbSuccess);

  gdk_gc_set_foreground (m_gc, &m_aColors[cFG]);


  m_pixmap = gdk_pixmap_new(m_parent,
			    (m_nColumns+m_border*2)*m_xPixel,
			    (m_nRows+m_border*2)*m_yPixel,
			    -1);

  printf("m_pixmap %p  gbSuccess:%d\n",m_pixmap,gbSuccess);

}

void gLCD::clear()
{
  gdk_gc_set_foreground (m_gc, &m_aColors[cBG]);
  gdk_draw_rectangle (m_pixmap,
		      m_gc,
		      TRUE,
		      0,0,
		      (m_nColumns+m_border*2)*m_xPixel,
		      (m_nRows+m_border*2)*m_yPixel);
  gdk_gc_set_foreground (m_gc, &m_aColors[cFG]);

}

void gLCD::refresh()
{
  gdk_draw_drawable(m_parent, m_gc, m_pixmap,
		    0,0, 0,0, -1,-1);
}

void gLCD::setPixel(unsigned int col, unsigned int row)
{
  if (col < m_nColumns && row < m_nRows) {
    int x = (col + m_border) * m_xPixel;
    int y = (row + m_border) * m_yPixel;

    gdk_draw_point (m_pixmap,
		    m_gc, x, y);
    gdk_draw_point (m_pixmap,
		    m_gc, x+1, y);
    gdk_draw_point (m_pixmap,
		    m_gc, x, y+1);
    gdk_draw_point (m_pixmap,
		    m_gc, x+1, y+1);


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
