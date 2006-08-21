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

#ifndef __GUI_SCOPE_H__
#define __GUI_SCOPE_H__

#include "gui.h"

class TimeMarker;
class ZoomAttribute;
class PanAttribute;
class Waveform;

  
//
// The Scope Window
//

class Scope_Window : public GUI_Object
{
public:

  Scope_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void Update(void);
  void UpdateMarker(gdouble x, gdouble y, guint button, guint state);

  void Expose(Waveform *);

  /// zoom - positive values mean zoom in, negative values mean zoom out
  void zoom(int);

  /// pan - positive values mean pan right, negative values mean pan left.
  void pan(int);

  /// getSpan - returns time span currently cached (essentially tStop-tStart).
  gdouble getSpan();

private:
  /// mapPixelToTime - convert a pixel horizontal offset to time
  guint64 mapPixelToTime(int pixel);

  /// mapTimeToPixel - convert time to a pixel horizontal offset.
  int mapTimeToPixel(guint64 time);

  ///
  int waveXoffset();

  enum {
    eStart=0,
    eStop,
    eLeftButton,
    eRightButton,
    eNumMarkers
  } MarkerLabels;

  TimeMarker *m_Markers[eNumMarkers];
  ZoomAttribute *m_zoom;
  PanAttribute  *m_pan;

  GtkWidget *m_pHpaned;     /* The signal names and waves are rendered 
			       in a horizontal pane */
  GtkWidget *m_phScrollBar; // scroll bar for the waves

  unsigned int m_PixmapWidth; // Width of waveform pixmaps.
  bool m_bFrozen;
};


#endif // __GUI_SCOPE_H__
