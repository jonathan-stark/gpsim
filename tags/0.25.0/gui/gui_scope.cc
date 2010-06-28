/*
   Copyright (C) 2004 T. Scott Dattalo

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


/*
           S C O P E

A simple waveform viewer for gpsim. Inspired from Nuno Sucena Almeida's
gpsim_la - plug in.

*/

/*

TODO:

 -- Scopewindow is emitting an expose event each time a waveform is drawn.

*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>

#include "../src/bitlog.h"
#include "../src/symbol.h"

//#define DEBUG

#include "gui_scope.h"




//static GtkObject *bit_adjust; // ,*delay_adjust;
static GdkColor signal_line_color,grid_line_color,grid_v_line_color;
static GdkColor highDensity_line_color;
//static int bit_left,bit_right,bit_points,update_delay;


class WaveformSink;
class Waveform;
/***********************************************************************
  timeMap - a structure that's used to map horizontal screen coordinates
  into simulation time.
*/
struct timeMap
{
  // simulation time
  double time;

  // pixel x-coordinate
  int     pos;

  // index into array holding event (fixme - exposes Logger implementation)
  unsigned int eventIndex;

  // The event
  int event;
};

/***********************************************************************
  WaveSource

  This is an attribute that provides the interface between the WaveForm
  class (below) and the names of nodes or stimuli that source information.

*/
class WaveformSource : public String
{
public:
  WaveformSource(Waveform *pParent, const char *_name);

  // Override the String::set method so that name assignments can
  // be intercepted.
  virtual void set(const char *cp, int len=0);

private:
  Waveform *m_pParent;
  bool m_bHaveSource;
};

/***********************************************************************
 zoom
 */
class ZoomAttribute : public Integer
{
public:
  ZoomAttribute(Scope_Window *);
  virtual void set(gint64 i);

private:
  Scope_Window *m_pSW;
};

/***********************************************************************
 pan
 */
class PanAttribute : public Integer
{
public:
  PanAttribute(Scope_Window *);
  virtual void set(gint64 i);

private:
  Scope_Window *m_pSW;
};

/***********************************************************************
 */
class PixMap
{
public:
  PixMap(GdkDrawable *pParent, gint w, gint h, gint y);
  GdkPixmap *pixmap() { return m_pixmap; }
  gint width;
  gint height;
  gint yoffset;  //
protected:
  GdkPixmap *m_pixmap;
};

PixMap::PixMap(GdkDrawable *pParent, gint w, gint h, gint y)
  : width(w), height(h), yoffset(y)
{
  m_pixmap = gdk_pixmap_new(pParent, width, height, -1);

}

GtkWidget *waveDrawingArea=0;
GtkWidget *signalDrawingArea=0;
// use this if signalDrawingArea is just a drawing area
//#define SignalWindow signalDrawingArea->window
// use this if signalDrawingArea is a layout widget
#define SignalWindow GTK_LAYOUT (signalDrawingArea)->bin_window

GdkGC *drawing_gc=0;         // Line styles, etc.
GdkGC *highDensity_gc=0;     // Line styles, etc.
GdkGC *text_gc=0;            // signal names
GdkGC *grid_gc=0;            // Grid color
GdkGC *leftMarker_gc=0;      // Marker style & color

/***********************************************************************
  SignalNameEntry - a class to control gui editing of signal names
 */
class SignalNameEntry
{
public:
  SignalNameEntry(Scope_Window *parent,GtkEntry *);


  bool Select(WaveBase *);
  bool unSelect();
  bool isSelected(WaveBase *);
  WaveBase *getSelected();
  GtkEntry *m_entry;
protected:
  Scope_Window *m_parent;
  WaveBase *m_selectedWave;

};

/***********************************************************************
  Wavebase class

*/
class WaveBase
{
public:
  WaveBase(Scope_Window *parent, const char *name);

  virtual ~WaveBase()
  {
  }

  virtual void Update(guint64 start=0, guint64 stop=0)=0;
  virtual void Build(PixMap *pWavePixmap, PixMap *pSignalPixmap);

  void setPixmaps(GdkPixmap *, GdkPixmap *);
  PixMap *wavePixmap() { return m_wavePixmap; }
  PixMap *signalPixmap() { return m_signalPixmap;}
  const char *get_text();
  virtual void setSource(const char *) { }

protected:

  Scope_Window *sw;          // Parent
  bool isBuilt;              // True after the gui has been built.
  guint64 m_start;           // Start time of plotted waveform
  guint64 m_stop;            // Stop time of plotted waveform
  PixMap *m_wavePixmap;      // The Waveform is rendered in this pixmap.
  PixMap *m_signalPixmap;    // The signal name is rendered in this pixmap.
  PangoLayout *m_layout;     // Pango layout for rendering signal name



};
/***********************************************************************
  TimeAxis - A special 'Waveform' for plotting the time
 */
class TimeAxis : public WaveBase
{
public:
  TimeAxis(Scope_Window *parent, const char *name);

  virtual ~TimeAxis()
  {
  }

  virtual void Build(PixMap *pWavePixmap, PixMap *pSignalPixmap);
  virtual void Update(guint64 start=0, guint64 stop=0);

protected:
  PangoLayout *m_TicText;
};

TimeAxis *m_TimeAxis;

/***********************************************************************
  Waveform class

  This holds the gui information related with a gpsim waveform
*/
class Waveform : public WaveBase
{
public:
  Waveform(Scope_Window *parent, const char *name);

  virtual ~Waveform()
  {
  }

  virtual void Update(guint64 start=0, guint64 stop=0);
  virtual void Build(PixMap *pWavePixmap, PixMap *pSignalPixmap);

  void Resize(int width, int height);
  void SearchAndPlot(timeMap &left, timeMap &right);
  void Dump(); // debug
  void setData(char c);
  virtual void setSource(const char *);
  void updateLayout();
protected:
  void PlotTo(timeMap &left, timeMap &right);

  PinMonitor *m_ppm;
  WaveformSink *m_pSink;
  ThreeStateEventLogger *m_logger;
  timeMap m_last;
  WaveformSource *m_pSourceName;
};


//------------------------------------------------------------------------
//
// Scope_Window data items that need to go into the Scope_Window class.
// The reason they're here now is simply for development convenience.
//
//
const int nSignals=8;
Waveform *signals[nSignals];   // hack
int aw=0;
int ah=0;





/***********************************************************************
  WaveformSink - A "sink" is an object that receives data from a
  "source". In the context of waveform viewer, a source is a stimulus
  and the viewer is the sink.
*/
class WaveformSink : public SignalSink
{
public:
  WaveformSink(Waveform *pParent);
  virtual void setSinkState(char);
  virtual void release()
  {
    cout << "Fixme - WaveformSink::release need to notify scope\n";
  }
protected:
  Waveform *m_pWaveform;
};


//========================================================================
class ZoomInEvent : public KeyEvent
{
public:
  virtual ~ZoomInEvent()
  {
  }

  void press(gpointer data)
  {
    Scope_Window *sw = (Scope_Window *)(data);
    if (sw)
      sw->zoom(2);
  }
  void release(gpointer data) {}
};
//========================================================================
class ZoomOutEvent : public KeyEvent
{
public:
  virtual ~ZoomOutEvent()
  {
  }

  void press(gpointer data)
  {
    Scope_Window *sw = (Scope_Window *)(data);
    if (sw)
      sw->zoom(-2);
  }
  void release(gpointer data) {}
};

//========================================================================
class PanLeftEvent : public KeyEvent
{
public:
  virtual ~PanLeftEvent()
  {
  }

  void press(gpointer data)
  {
    Scope_Window *sw = (Scope_Window *)(data);
    if (sw)
      sw->pan(-( (gint64) sw->getSpan()/4));
  }
  void release(gpointer data) {}
};
//========================================================================
class PanRightEvent : public KeyEvent
{
public:
  virtual ~PanRightEvent()
  {
  }

  void press(gpointer data)
  {
    Scope_Window *sw = (Scope_Window *)(data);
    if (sw)
      sw->pan( (gint64) sw->getSpan()/4);
  }
  void release(gpointer data) {}
};

//========================================================================

static map<guint, KeyEvent *> KeyMap;

static gint
key_press(GtkWidget *widget,
          GdkEventKey *key,
          gpointer data)
{
  Dprintf (("press 0x%x\n",key->keyval));

  KeyEvent *pKE = KeyMap[key->keyval];
  if(pKE)
    {
      pKE->press(data);
      return TRUE;
    }

  return FALSE;
}

static gint
key_release(GtkWidget *widget,
          GdkEventKey *key,
          gpointer data)
{
  return TRUE;
}

static gint
scroll_event(GtkWidget *widget,
             GdkEventScroll *scroll,
             gpointer data)
{
  //printf ("scroll: x:%g y:%g dir:%d keymod:0x%x\n",scroll->x,scroll->y,scroll->direction,scroll->state);

  return TRUE;
}

//========================================================================
// WaveformSource

WaveformSource::WaveformSource(Waveform *pParent, const char *_name)
  : String(_name, "", "view or set gui scope waveforms"),
    m_pParent(pParent), m_bHaveSource(false)
{
  assert(m_pParent);
  // Prevent removal from the symbol table (all clearable symbols are
  // removed from the symbol table when a new processor is loaded).
}

void WaveformSource::set(const char *cp, int len)
{
  if (!m_bHaveSource) {
    String::set(cp,len);
    m_pParent->setSource(cp);
    m_bHaveSource = true;
  }

}

//========================================================================

ZoomAttribute::ZoomAttribute(Scope_Window *pSW)
  : Integer("scope.zoom",0,"Scope Zoom; positive values zoom in, negative values zoom out"),
    m_pSW(pSW)
{
  assert(m_pSW);
}
void ZoomAttribute::set(gint64 i)
{
  Integer::set(i);
  m_pSW->zoom(i);
}

//========================================================================

PanAttribute::PanAttribute(Scope_Window *pSW)
  : Integer("scope.pan",0,"Scope Pan; positive values pan right, negative values pan left"),
    m_pSW(pSW)
{
  assert(m_pSW);
}
void PanAttribute::set(gint64 i)
{
  Integer::set(i);
  m_pSW->pan(i);
}

//========================================================================
WaveformSink::WaveformSink(Waveform *pParent)
  : SignalSink(), m_pWaveform(pParent)
{
  assert(m_pWaveform);
}

void WaveformSink::setSinkState(char c)
{
  m_pWaveform->setData(c);
}


//************************************************************************
WaveBase::WaveBase(Scope_Window *parent, const char *name)
  : sw(parent),
    isBuilt(false),
    m_start(1), m_stop(1),
    m_wavePixmap(0),
    m_signalPixmap(0),
    m_layout(0)
{

}
void WaveBase::Build(PixMap *pWavePixmap, PixMap *pSignalPixmap)
{

  if (m_wavePixmap && m_wavePixmap->pixmap())
    gdk_pixmap_unref(m_wavePixmap->pixmap());

  m_wavePixmap = pWavePixmap;

  if (m_signalPixmap && m_signalPixmap->pixmap())
    gdk_pixmap_unref(m_signalPixmap->pixmap());

  m_signalPixmap = pSignalPixmap;

  m_layout = gtk_widget_create_pango_layout (GTK_WIDGET (signalDrawingArea), "");

  isBuilt = true;
  Update(0,0);
}

const char *WaveBase::get_text()
{
  return m_layout ? pango_layout_get_text(m_layout) : 0;
}

//************************************************************************
Waveform::Waveform(Scope_Window *parent, const char *name)
  : WaveBase(parent,name), m_ppm(0)
{

  m_pSink = new WaveformSink(this);
  m_logger = new ThreeStateEventLogger();
  m_pSourceName = new WaveformSource(this,name);
  globalSymbolTable().addSymbol(m_pSourceName);

  m_logger->event('0');
}

void Waveform::Build(PixMap *pWavePixmap, PixMap *pSignalPixmap)
{
  WaveBase::Build(pWavePixmap, pSignalPixmap);
  updateLayout();
}
void Waveform::setData(char c)
{
  m_logger->event(c);
}

void Waveform::updateLayout()
{
  if (m_layout) {
    char buffer[100];
    m_pSourceName->get(buffer, sizeof(buffer));
    pango_layout_set_text(m_layout,buffer,-1);
  }
}

void Waveform::setSource(const char *sourceName)
{
  //IOPIN *ppin = dynamic_cast<IOPIN*>(get_symbol_table().findStimulus(sourceName));
  IOPIN *ppin = dynamic_cast<IOPIN*>(globalSymbolTable().find(string(sourceName)));
  if (ppin) {
    if (m_ppm)
      m_ppm->removeSink(m_pSink);

    m_ppm = ppin->getMonitor();
    if (m_ppm)
      m_ppm->addSink(m_pSink);

    updateLayout();

    // Invalidate wave area.
    m_start = m_stop = 1;
    Update(0,0);
    if (sw)
      sw->Expose(this);

  } else if(sourceName)
     printf("'%s' is not a valid source for the scope\n",sourceName);
}
//----------------------------------------
void Waveform::Resize(int w, int h)
{

  Update();

}
static bool plotDebug=false;
//----------------------------------------
//
void Waveform::PlotTo(timeMap &left, timeMap &right)
{
  // Event(s) has(have) been found.
  // The plotting region has been subdivided as finely as possible
  // and there are one or more events.
  // First draw a horizontal line from the last known event to here:


  gdk_draw_line(m_wavePixmap->pixmap(),drawing_gc,
                m_last.pos, m_last.event,   // last point drawn
                right.pos,  m_last.event);  // right most point of this region.

  // Now draw a vertical line for the event

  int nextEvent = (m_logger->get_state(right.eventIndex) == '1') ? 1 : (m_wavePixmap->height-3);

  // Draw a thicker line if there is more than one event.
  unsigned int nEvents = m_logger->get_nEvents(left.eventIndex,right.eventIndex);
  if (nEvents>1) {

    guint16 c = (nEvents < 4) ? (0x4000*nEvents+0x8000) : 0xffff;

    if (c != highDensity_line_color.blue) {
      gdk_colormap_free_colors(gdk_colormap_get_system(),
                               &highDensity_line_color,
                               1);
      // variations of yellow
      highDensity_line_color.green = 0xffff;
      highDensity_line_color.red = 0xffff;
      highDensity_line_color.blue = c;

      gdk_colormap_alloc_color(gdk_colormap_get_system(), &highDensity_line_color, TRUE, TRUE);

      gdk_gc_set_foreground(highDensity_gc,&highDensity_line_color);

    }

    gdk_draw_line(m_wavePixmap->pixmap(),highDensity_gc,
                  right.pos, 1,
                  right.pos, m_wavePixmap->height-3);
    if (left.pos != right.pos)
      gdk_draw_line(m_wavePixmap->pixmap(),highDensity_gc,
                    left.pos, 1,
                    left.pos, m_wavePixmap->height-3);
  } else
    gdk_draw_line(m_wavePixmap->pixmap(),drawing_gc,
                  right.pos, m_last.event,    // last point drawn
                  right.pos, nextEvent); // next event

  if (plotDebug)
    printf("pos=%d time=%g\n",right.pos,right.time);

  m_last = right;
  m_last.event = nextEvent;

}
//----------------------------------------
//
// Waveform SearchAndPlot
//
//  Recursively divide the plotting area into smaller and smaller
// regions until either only one event exists or the region can
// be divided no smaller.
//

void Waveform::SearchAndPlot(timeMap &left, timeMap &right)
{
  if (right.eventIndex == left.eventIndex)
    // The time span cannot be divided any smaller.
    // If there are no events in this subdivided region
    // So just return.
    // m_last = left;
    ;
  else if (left.pos+1 >= right.pos)
    PlotTo(left,right);
  else {
    // the subdivided region is larger than 1-pixel wide
    // and there is at least one event. So subdivide even smaller
    // and recursively call

    timeMap mid;

    mid.time = (left.time + right.time) / 2;
    mid.pos  = (left.pos  + right.pos)  / 2;
    mid.eventIndex = m_logger->get_index ((guint64)mid.time);

    if (plotDebug)
      cout << " Mid pos="<<mid.pos
           << " Mid.time=" << mid.time
           << " left.time=" << left.time
           << " right.time=" << right.time
           << " evt idx=" << mid.eventIndex
           << " evt time=" << m_logger->get_time(mid.eventIndex)
           << endl;

    SearchAndPlot(left, mid);
    SearchAndPlot(mid, right);

  }

}

//----------------------------------------
//
// Waveform Update
//

void Waveform::Dump()
{
  m_logger->dump(0);
}
//----------------------------------------
//
// Waveform Update
//

void Waveform::Update(guint64 uiStart, guint64 uiEnd)
{
  if(!isBuilt)
    return;

  if(!m_wavePixmap) {
    cout << __FUNCTION__ << " pixmap is NULL\n";
    return;
  }

  if (uiEnd == 0)
    uiEnd = get_cycles().get();

  if (m_start == uiStart && m_stop == uiEnd)
    return;

  m_start = uiStart;
  m_stop  = uiEnd;

  gdk_draw_rectangle (m_wavePixmap->pixmap(),
                      waveDrawingArea->style->black_gc,
                      TRUE,
                      0, 0,
                      m_wavePixmap->width,
                      m_wavePixmap->height);


  gdk_draw_rectangle (m_signalPixmap->pixmap(),
                      signalDrawingArea->style->bg_gc[GTK_STATE_NORMAL],
                      //signalDrawingArea->style->black_gc,
                      TRUE,
                      0, 0,
                      m_signalPixmap->width,
                      m_signalPixmap->height);
  if (m_layout) {
    updateLayout();
    int text_height=0;
    pango_layout_get_pixel_size(m_layout,
                                NULL,
                                &text_height);
    Dprintf(("signal name update:%s\n",pango_layout_get_text(m_layout)));
    gdk_draw_layout (GDK_DRAWABLE(m_signalPixmap->pixmap()),
                     signalDrawingArea->style->fg_gc[GTK_STATE_NORMAL],
                     0,
                     (m_signalPixmap->height-text_height)/2,
                     m_layout);
  }

  //
  // Draw Vertical Grid Lines:
  //

  int i;
  for (i=0; i<sw->MajorTicks().sze(); i++) {

    int x = sw->MajorTicks().pixel(i);

    gdk_draw_line(m_wavePixmap->pixmap(),grid_gc,
                  x,1,
                  x,m_wavePixmap->height-1);
  }

  //
  // Draw Horizontal Grid Lines:
  //
  gdk_draw_line(m_wavePixmap->pixmap(),grid_gc,
                0,m_wavePixmap->height-1,
                m_wavePixmap->width,m_wavePixmap->height-1);

  if (m_stop == 0)
    return;

  // Draw Signals:

  timeMap left;
  timeMap right;

  left.pos = 0;
  left.time = m_start;
  left.eventIndex = m_logger->get_index(m_start);
  left.event = (m_logger->get_state(left.eventIndex) == '1') ? 1 : (m_wavePixmap->height-3);

  m_last = left;

  right.pos = m_wavePixmap->width;
  right.time = m_stop;
  right.eventIndex = m_logger->get_index(m_stop);


  SearchAndPlot(left,right);
  if (right.pos > m_last.pos)
    gdk_draw_line(m_wavePixmap->pixmap(),drawing_gc,
                  m_last.pos, m_last.event,   // last point drawn
                  right.pos,  m_last.event);  // right most point


}


//========================================================================
TimeAxis::TimeAxis(Scope_Window *parent, const char *name)
  : WaveBase(parent,name),
    m_TicText(0)
{

}
//----------------------------------------
//
// TimeAxis Update
//

void TimeAxis::Update(guint64 uiStart, guint64 uiEnd)
{
  if(!isBuilt)
    return;

  if(!m_wavePixmap) {
    cout << __FUNCTION__ << " pixmap is NULL\n";
    return;
  }

  if (uiEnd == 0)
    uiEnd = get_cycles().get();

  if (m_start == uiStart && m_stop == uiEnd)
    return;

  m_start = uiStart;
  m_stop  = uiEnd;

  gdk_draw_rectangle (m_wavePixmap->pixmap(),
                      waveDrawingArea->style->bg_gc[GTK_STATE_NORMAL],
                      TRUE,
                      0, 0,
                      m_wavePixmap->width,
                      m_wavePixmap->height);

  gdk_draw_rectangle (m_signalPixmap->pixmap(),
                      signalDrawingArea->style->bg_gc[GTK_STATE_NORMAL],
                      TRUE,
                      0, 0,
                      m_signalPixmap->width,
                      m_signalPixmap->height);

  //
  // Draw Major Ticks:
  //
  int i;
  for (i=0; i<sw->MajorTicks().sze(); i++) {

    int x = sw->MajorTicks().pixel(i);

    gdk_draw_line(m_wavePixmap->pixmap(),grid_gc,
                  x,m_wavePixmap->height-3,
                  x,m_wavePixmap->height-1);

    if (m_TicText) {
      char buff[100];
      snprintf(buff, sizeof(buff),"%" PRINTF_GINT64_MODIFIER "d", sw->MajorTicks().cycle(i));
      pango_layout_set_text(m_TicText, buff, -1);

      int text_height=0;
      int text_width=0;

      pango_layout_get_pixel_size(m_TicText,
                                  &text_width,
                                  &text_height);
      text_width /= 2;
      x = ((x-text_width) < 0) ? 0 : (x-text_width);
      x = ((x+text_width) > m_wavePixmap->width) ? (x-text_width) : x;

      gdk_draw_layout (GDK_DRAWABLE(m_wavePixmap->pixmap()),
                       waveDrawingArea->style->fg_gc[GTK_STATE_NORMAL],
                       x,
                       (m_wavePixmap->height-text_height)/2,
                       m_TicText);

      }

  }

  //
  // Draw Minor Ticks:
  //
  for (i=0; i<sw->MinorTicks().sze(); i++) {

    int x = sw->MinorTicks().pixel(i);

    gdk_draw_line(m_wavePixmap->pixmap(),grid_gc,
                  x,m_wavePixmap->height-3,
                  x,m_wavePixmap->height-1);
  }
  //
  // Draw Horizontal Grid Lines:
  //
  gdk_draw_line(m_wavePixmap->pixmap(),grid_gc,
                0,m_wavePixmap->height-1,
                m_wavePixmap->width,m_wavePixmap->height-1);

}



void TimeAxis::Build(PixMap *pWavePixmap, PixMap *pSignalPixmap)
{
  WaveBase::Build(pWavePixmap, pSignalPixmap);
  // Invalidate the start time after the build.
  // This is for the case where the scope window gets opened after
  // the simulation has already been started.
  m_start=m_stop=0;
  /*
  if (m_layout)
    pango_layout_set_text(m_layout,"Time", -1);
  */
  m_TicText = gtk_widget_create_pango_layout (GTK_WIDGET (waveDrawingArea), "");

}


//------------------------------------------------------------------------
// Signals


//----------------------------------------
//
// When a user clicks on the "X" in the window managers border of the
// scope window, we'll capture that event and hide the scope.
//
static int delete_event(GtkWidget *widget,
                        GdkEvent  *event,
                        Scope_Window *sw)
{

  sw->ChangeView(VIEW_HIDE);
  return TRUE;
}

static gint Scope_Window_expose_event (GtkWidget *widget,
                                       GdkEventExpose  *event,
                                      Scope_Window   *sw)
{
  Dprintf(( " %s\n",__FUNCTION__));

  g_return_val_if_fail (widget != NULL, TRUE);
  //  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);


  if(sw) {
    sw->refreshSignalNameGraphics();

  }

  return FALSE;
}
static gint DrawingArea_expose_event (GtkWidget *widget,
                                      GdkEventExpose *event,
                                      Scope_Window   *sw)
{
  Dprintf(( " %s\n",__FUNCTION__));

  if(sw)
    sw->Update();

  return FALSE;
}

//------------------------------------------------------------------------
class TimeMarker : public Integer
{
public:
  TimeMarker(Scope_Window *parent, const char *_name, const char *desc);
  virtual void set(gint64 i);
private:
  Scope_Window *m_pParent;
};

TimeMarker::TimeMarker(Scope_Window *parent, const char *_name, const char *desc)
  : Integer(_name,0,desc),
    m_pParent(parent)
{
  assert(m_pParent);
}
void TimeMarker::set(gint64 i)
{
  Integer::set(i);
  m_pParent->Update();
}

//========================================================================
GridPointMapping::GridPointMapping(int nPointsToMap)
  : m_pointsAllocated(nPointsToMap), m_nPoints(0)
{
  m_pixel = new int[m_pointsAllocated];
  m_cycle = new guint64[m_pointsAllocated];

}

//========================================================================
// SignalNameEntry
SignalNameEntry::SignalNameEntry(Scope_Window *parent,GtkEntry *entry)
  : m_entry(entry), m_parent(parent), m_selectedWave(0)
{
  gtk_widget_hide (GTK_WIDGET(m_entry));
}
bool SignalNameEntry::Select(WaveBase *pWave)
{

  if (pWave) {
    gtk_entry_set_text (GTK_ENTRY(m_entry),
                        pWave->get_text());
    gtk_widget_show (GTK_WIDGET(m_entry));
    gtk_widget_grab_focus (GTK_WIDGET(m_entry));

    m_selectedWave = pWave;
    return true;
  }

  return unSelect();
}

bool SignalNameEntry::unSelect()
{
  gtk_widget_hide (GTK_WIDGET(m_entry));
  m_selectedWave = 0;
  return false;
}
bool SignalNameEntry::isSelected(WaveBase *pWave)
{
  return pWave == m_selectedWave;
}

WaveBase *SignalNameEntry::getSelected()
{
  return m_selectedWave;
}

//========================================================================
void Scope_Window::refreshSignalNameGraphics()
{
  GTKWAIT;

  Expose(m_TimeAxis);

  for (int i=0; i<nSignals; i++)
    Expose(signals[i]);
}

//========================================================================
void Scope_Window::refreshWaveFormGraphics()
{
}

//========================================================================
void Scope_Window::gridPoints(guint64 *uiStart, guint64 *uiEnd)
{
  guint64 start = m_Markers[eStart]->getVal();
  guint64 stop  = m_Markers[eStop]->getVal();
  if (!stop)
    stop = get_cycles().get();

  if (uiStart)
    *uiStart = start;
  if (uiEnd)
    *uiEnd = stop;

  //
  // Draw Vertical Grid Lines:
  //

  double t1 = (double) start;
  double t2 = (double) stop;
  double dt = t2-t1;

  int iMajor=0;
  int iMinor=0;
  m_MajorTicks.m_nPoints = 0;
  m_MinorTicks.m_nPoints = 0;

  if (dt > 1.0) {

    double exp1 = floor(log10(dt));
    double dt_tic = pow(10.0,exp1);

    double nTics = floor(dt/dt_tic);
    if (nTics < 5.0  && exp1>0.0)
      dt_tic /= 2.0;

    const int nMinorTics=5;
    const double dt_minor= dt_tic/nMinorTics;

    double ta = ceil(t1/dt_tic);
    double tb = floor(t2/dt_tic);

    for (double t=ta; t<=tb; t+=1.0) {
      double ttic=t*dt_tic;
      guint64 uiTime = (guint64)(floor(ttic));

      m_MajorTicks.m_pixel[iMajor] = mapTimeToPixel(uiTime);
      m_MajorTicks.m_cycle[iMajor] = uiTime;

      /*
      printf ("t=%g dt_tic=%g tmajor=%g %d %lld\n",
              t,dt_tic,ttic,m_MajorTicks.m_pixel[iMajor],uiTime);
      */
      iMajor++;

      ttic+=dt_minor;

      for (int it=1; it<nMinorTics; it++,ttic+=dt_minor) {
        uiTime = (guint64)(ttic);
        m_MinorTicks.m_pixel[iMinor] = mapTimeToPixel(uiTime);
        m_MinorTicks.m_cycle[iMinor] = uiTime;
        /*
        printf ("     tminor=%g %d %lld\n",
                ttic,m_MinorTicks.m_pixel[iMinor],uiTime);
        */
        iMinor++;

      }
    }
  }

  m_MajorTicks.m_nPoints = iMajor;
  m_MinorTicks.m_nPoints = iMinor;

}

static gdouble gNormalizedHorizontalPosition=0.0;
static GtkWidget *pvbox=0;
GtkObject *m_hAdj=0;


int Scope_Window::waveXoffset()
{
  return (int)((m_PixmapWidth -
                (m_pHpaned->allocation.width - gtk_paned_get_position(GTK_PANED(m_pHpaned))))
               *gNormalizedHorizontalPosition);
}

void Scope_Window::Expose(WaveBase *wf)
{
  if(!wf || !waveDrawingArea)
    return;

  int xoffset = waveXoffset();

  PixMap *pm = wf->wavePixmap();

  gdk_draw_pixmap(waveDrawingArea->window,
                  waveDrawingArea->style->fg_gc[GTK_STATE_NORMAL],
                  pm->pixmap(),
                  xoffset,0,      // source
                  0,pm->yoffset,  // destination
                  pm->width,pm->height);

  pm = wf->signalPixmap();

  // If the Waveform is selected for editing, then there's a gtk_entry
  // widget exposed showing the signal name. If we're not editing, then
  // show the pixmap with the signal name:

  if (!m_entry->isSelected(wf)) {

    gdk_draw_pixmap(SignalWindow,
                    signalDrawingArea->style->fg_gc[GTK_STATE_NORMAL],
                    pm->pixmap(),
                    0,0,            // source
                    0,pm->yoffset,  // destination
                    pm->width,pm->height);
  }
}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#if 0 // defined but not used
static void ScrollAdjustments(GtkViewport   *viewport,
                                GtkAdjustment *arg1,
                                GtkAdjustment *arg2,
                                gpointer       user_data)
{
  printf("%s\n",__FUNCTION__);
}

static void ScrollChildren(GtkScrolledWindow *scrolledwindow,
                            GtkScrollType     *arg1,
                            gboolean           arg2,
                            gpointer           user_data)
{
  printf("%s\n",__FUNCTION__);
}
#endif

static void hAdjVChange(GtkAdjustment *pAdj,
                        gpointer       user_data)
{

  gdouble width = pAdj->upper - pAdj->lower - pAdj->page_size;
  gNormalizedHorizontalPosition = pAdj->value/ (width ? width : 1.0);

}
static gint
button_press(GtkWidget *widget,
             GdkEventButton *pEventButton,
             Scope_Window *sw)
{

  if (pEventButton) {

    Dprintf (("button: button:%d x=%g y=%g evt=%d modifier=0x%x\n",
              pEventButton->button,
              pEventButton->x,pEventButton->y,pEventButton->type,pEventButton->state));
    if (sw) {
      sw->UpdateMarker(pEventButton->x,pEventButton->y, pEventButton->button, pEventButton->state);
    }
  }

  return TRUE;
}

bool Scope_Window::endSignalNameSelection(bool bAccept)
{

  gtk_widget_grab_focus (GTK_WIDGET(waveDrawingArea));

  WaveBase *pwf = m_entry->getSelected();
  if (pwf) {

    if (bAccept)
      pwf->setSource(gtk_entry_get_text(m_entry->m_entry));

    m_entry->Select(0);

    return true;
  }

  return false;

}

//------------------------------------------------------------------------
bool Scope_Window::selectSignalName(int y)
{
  int timeHeight=15;
  int waveHeight=20;
  bool bRet=true;

  y = (y > timeHeight) ? ((y-timeHeight)/waveHeight) : -1;

  if (y>=0 && y<nSignals) {

    if (m_entry->isSelected(signals[y]))
      return false;

    m_entry->unSelect();

    PixMap *pm = signals[y]->wavePixmap();

    if (pm)
      gtk_layout_move(GTK_LAYOUT(signalDrawingArea),
                      GTK_WIDGET(m_entry->m_entry),
                      0,pm->yoffset-2);
    bRet = m_entry->Select(signals[y]);

  } else
    bRet = endSignalNameSelection(true);

  if (bRet)
    refreshSignalNameGraphics();

  return bRet;
}


//************************************************************************
//           S I G N A L S
//************************************************************************

gint Scope_Window::signalEntryKeyPress(GtkEntry *widget,
                                       GdkEventKey *key,
                                       Scope_Window *sw)
{
  Dprintf (("Entry keypress 0x%x\n",key->keyval));

  if ( key){

    if ( key->keyval==GDK_Return)
      sw->endSignalNameSelection(true);

    if ( key->keyval==GDK_Escape)
      sw->endSignalNameSelection(false);
  }

  return FALSE;
}


gint Scope_Window::signalButtonPress(GtkWidget *widget,
                                     GdkEventButton *pEventButton,
                                     Scope_Window *sw)
{

  if (pEventButton) {

    Dprintf ((" Signal: button:%d x=%g y=%g evt=%d modifier=0x%x\n",
              pEventButton->button,
              pEventButton->x,pEventButton->y,pEventButton->type,pEventButton->state));

    sw->selectSignalName((int)(pEventButton->y));

  }

  return TRUE;
}
//------------------------------------------------------------------------
//
// Scope_Window member functions
//
//
//         ------------------------------------------
//         |                  Window
//         |   --------------------------------------
//         |   |            Horizontal pane
//         |   |  ---------------+  +------------------------+
//         |   |  | signal name  |  |    Wave layout
//         |   |  | drawing area |  |  +---------------------
//         |   |  |
//
//------------------------------------------------------------------------

void Scope_Window::Build()
{

  GtkTooltips *tooltips;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (!window)
    return;


  // The "realize" operation creates basic resources like GC's etc. that
  // are referenced below. Without this call, those resources will be NULL.
  gtk_widget_realize (window);

  gtk_window_set_title(GTK_WINDOW(window), "Scope");

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);


  tooltips = gtk_tooltips_new();



  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
                     GTK_SIGNAL_FUNC(delete_event), this);

  gtk_signal_connect (GTK_OBJECT (window),
                      "expose_event",
                      GTK_SIGNAL_FUNC (Scope_Window_expose_event),
                      this);

#if 0
  GtkWidget *spin_button;
  button = gtk_button_new_with_label ("Zoom In");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (analyzer_zoom_in_callback),this);
  gtk_table_attach_defaults (GTK_TABLE(table),button,2,4,9,10);
  button = gtk_button_new_with_label ("Zoom Out");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (analyzer_zoom_out_callback),this);
  gtk_table_attach_defaults (GTK_TABLE(table),button,4,6,9,10);
  button = gtk_button_new_with_label ("About");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (analyzer_screen_about),this);
  gtk_table_attach_defaults (GTK_TABLE(table),button,6,8,9,10);
#endif


  //
  // Define the drawing colors
  //

  // The signal color is bright red
  signal_line_color.red = 0xff00;
  signal_line_color.green = 0x0000;
  signal_line_color.blue = 0x0000;
  gdk_colormap_alloc_color(gdk_colormap_get_system(), &signal_line_color, FALSE, TRUE);
  // The grid color is bright green
  grid_line_color.red = 0x4000;
  grid_line_color.green = 0x4000;
  grid_line_color.blue = 0x4000;
  gdk_colormap_alloc_color(gdk_colormap_get_system(), &grid_line_color, FALSE, TRUE);
  // The vertical grid color is dark green
  grid_v_line_color.red = 0x0000;
  grid_v_line_color.green = 0x2200;
  grid_v_line_color.blue = 0x0000;
  gdk_colormap_alloc_color(gdk_colormap_get_system(), &grid_v_line_color, FALSE, TRUE);
  // The vertical grid color is dark green
  highDensity_line_color.red = 0xff00;
  highDensity_line_color.green = 0xff00;
  highDensity_line_color.blue = 0xff00;
  gdk_colormap_alloc_color(gdk_colormap_get_system(), &highDensity_line_color, TRUE, TRUE);


  waveDrawingArea = gtk_drawing_area_new ();
  gint dawidth  = 400;
  gint daheight = 100;

  gtk_widget_set_usize (waveDrawingArea,dawidth,daheight);
  gtk_widget_set_events (waveDrawingArea,
                         GDK_EXPOSURE_MASK |
                         GDK_BUTTON_PRESS_MASK |
                         GDK_KEY_PRESS_MASK |
                         GDK_KEY_RELEASE_MASK  );

  signalDrawingArea = gtk_layout_new (NULL, NULL);
  gtk_widget_set_usize (signalDrawingArea,100,daheight);
  gtk_widget_set_events (signalDrawingArea,
                         GDK_EXPOSURE_MASK |
                         GDK_BUTTON_PRESS_MASK |
                         GDK_KEY_PRESS_MASK |
                         GDK_KEY_RELEASE_MASK  );



  pvbox = gtk_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (window), pvbox);



  m_pHpaned = gtk_hpaned_new ();
  gtk_widget_show (m_pHpaned);

  gtk_box_pack_start_defaults (GTK_BOX (pvbox), m_pHpaned);


  m_hAdj = gtk_adjustment_new
    (0.0,    // value
     0.0,    // lower
     m_PixmapWidth,      // upper
     m_PixmapWidth/100.0,// step_increment
     m_PixmapWidth/10.0, // page_increment
     m_PixmapWidth/5.0); // page_size

  m_phScrollBar = gtk_hscrollbar_new(GTK_ADJUSTMENT(m_hAdj));
  gtk_box_pack_start_defaults(GTK_BOX(pvbox),m_phScrollBar);
  gtk_signal_connect(m_hAdj,"value-changed",
                     (GtkSignalFunc) hAdjVChange, this);

  // Add the drawing areas to the panes
  gtk_paned_add1(GTK_PANED(m_pHpaned), signalDrawingArea);
  gtk_paned_add2(GTK_PANED(m_pHpaned), waveDrawingArea);
  gtk_paned_set_position(GTK_PANED(m_pHpaned),50);

  gtk_widget_show(waveDrawingArea);
  gtk_widget_show(signalDrawingArea);

  // Graphics Context:
  drawing_gc = gdk_gc_new(waveDrawingArea->window);
  grid_gc = gdk_gc_new(waveDrawingArea->window);
  highDensity_gc = gdk_gc_new(waveDrawingArea->window);
  leftMarker_gc = gdk_gc_new(waveDrawingArea->window);

  gdk_gc_set_foreground(grid_gc,&grid_line_color);
  gdk_gc_set_foreground(drawing_gc,&signal_line_color);
  gdk_gc_set_foreground(highDensity_gc,&highDensity_line_color);
  gdk_gc_set_foreground(leftMarker_gc,&highDensity_line_color);
  gdk_gc_set_function(leftMarker_gc,GDK_XOR);
  text_gc = waveDrawingArea->style->white_gc;

  guint64 start,stop;
  gridPoints(&start,&stop);

  int timeHeight = 15;
  m_TimeAxis->Build(new PixMap(waveDrawingArea->window, m_PixmapWidth, timeHeight, 0),
                    new PixMap(waveDrawingArea->window, 100, timeHeight, 0));
  m_TimeAxis->Update(start,stop);

  for(int i=0; i<nSignals; i++) {
    int waveHeight=20;
    int yoffset = timeHeight +i*waveHeight;
    signals[i]->Build(new PixMap(waveDrawingArea->window, m_PixmapWidth, waveHeight, yoffset),
                      new PixMap(waveDrawingArea->window, 100, waveHeight, yoffset));
  }

  gtk_signal_connect (GTK_OBJECT (waveDrawingArea),
                      "expose_event",
                      GTK_SIGNAL_FUNC (DrawingArea_expose_event),
                      this);

  KeyMap['z'] = new ZoomInEvent();
  KeyMap['Z'] = new ZoomOutEvent();
  KeyMap['l'] = new PanLeftEvent();
  KeyMap['r'] = new PanRightEvent();


  /* Add a signal handler for key press events. This will capture
   * key commands for single stepping, running, etc.
   */
  gtk_signal_connect(GTK_OBJECT(waveDrawingArea),
                     "key_press_event",
                     (GtkSignalFunc) key_press,
                     (gpointer) this);

  gtk_signal_connect(GTK_OBJECT(waveDrawingArea),
                     "button_press_event",
                     (GtkSignalFunc) button_press,
                     (gpointer) this);

  gtk_signal_connect(GTK_OBJECT(waveDrawingArea),
                     "key_release_event",
                     (GtkSignalFunc) key_release,
                     (gpointer) this);
  gtk_signal_connect(GTK_OBJECT(waveDrawingArea),
                     "scroll-event",
                     (GtkSignalFunc) scroll_event,
                     (gpointer) this);
  GTK_WIDGET_SET_FLAGS( waveDrawingArea,
                        GTK_CAN_FOCUS );


  gtk_signal_connect(GTK_OBJECT(signalDrawingArea),
                     "button_press_event",
                     (GtkSignalFunc) signalButtonPress,
                     (gpointer) this);

  gtk_widget_show_all (window);

  bIsBuilt = true;

  UpdateMenuItem();

  aw = window->allocation.width;
  ah = window->allocation.height;

  m_entry = new SignalNameEntry(this, GTK_ENTRY(gtk_entry_new ()));

  gtk_layout_put(GTK_LAYOUT(signalDrawingArea),
                 GTK_WIDGET(m_entry->m_entry),
                 0,0);

  gtk_signal_connect(GTK_OBJECT(m_entry->m_entry),
                     "key_press_event",
                     (GtkSignalFunc) signalEntryKeyPress,
                     (gpointer) this);

}


void Scope_Window::Update()
{
  int i;

  if(!enabled)
    return;

  if(!bIsBuilt)
    Build();

  if(m_bFrozen)
    return;
  /*
  cout << "function:" << __FUNCTION__ << "\n";
  cout << " a  x "  << window->allocation.x
       << " a y "  << window->allocation.y
       << " a  width "  << window->allocation.width
       << " a height "  << window->allocation.height
       << endl;
  cout << " r  width "  << window->requisition.width
       << " r height "  << window->requisition.height
       << endl;
  */

  // Compute the grid points
  guint64 start,stop;
  gridPoints(&start,&stop);


  // Horizontal scroll
  // the scroll bar maps pixel positions into time.
  // the span of the scroll bar is the span of time that is currently
  //  cached (or soon to be cached) in the waveform pixmaps.
  // the thumb position is the current view into the cache.
  // the page-size property is 20% of the span


  double dspan = stop-start;
  dspan = (dspan<m_PixmapWidth) ? m_PixmapWidth : dspan;
  g_object_set(G_OBJECT(m_hAdj),
               "page-size", m_PixmapWidth * 200.0/dspan,
               NULL);
  gtk_widget_queue_draw (m_phScrollBar);


  m_TimeAxis->Update(start,stop);
  Expose(m_TimeAxis);

  for(i=0; i<nSignals; i++) {
    //plotDebug = i==0;
    if(signals[i]) {
      signals[i]->Update(start,stop);
      Expose(signals[i]);
    }

  }

  // Markers
  int xpos = mapTimeToPixel(m_Markers[eLeftButton]->getVal()) + waveXoffset();
  if (xpos)
    gdk_draw_line(waveDrawingArea->window,leftMarker_gc,
                  xpos, 0,
                  xpos, 1000);
  /*
  cout << "Left marker pos="<<dec<<xpos
       <<" time=" <<m_Markers[eLeftButton]->getVal()
       <<" start=" << start
       <<" stop=" << stop
       <<endl;
  */
  gtk_widget_show_all(window);

  if (m_entry->isSelected(0)) {
    gtk_widget_hide (GTK_WIDGET(m_entry->m_entry));
  }

}

void Scope_Window::UpdateMarker(gdouble x, gdouble y, guint button, guint state)
{
  switch (button) {

  case 1: // Left Button

  case 2: // Right Button
    ;
  }

}

Scope_Window::Scope_Window(GUI_Processor *_gp)
  : m_pHpaned(0), m_phScrollBar(0), m_PixmapWidth(1024),
    m_MajorTicks(32), m_MinorTicks(256),
    m_entry(0)
{

  gp = _gp;
  window = 0;
  wc = WC_data;
  wt = WT_scope_window;

  menu = "<main>/Windows/Scope";
  set_name("scope");

  get_config();

  m_Markers[eStart] = new TimeMarker(this, "scope.start", "Scope window start time");
  m_Markers[eStop]  = new TimeMarker(this, "scope.stop",  "Scope window stop time");
  m_Markers[eLeftButton]  = new TimeMarker(this, "scope.left",  "Scope window left marker");
  m_Markers[eRightButton] = new TimeMarker(this, "scope.right", "Scope window right marker");

  m_zoom = new ZoomAttribute(this);
  m_pan  = new PanAttribute(this);

  globalSymbolTable().addSymbol(m_Markers[eStart]);
  globalSymbolTable().addSymbol(m_Markers[eStop]);
  globalSymbolTable().addSymbol(m_Markers[eLeftButton]);
  globalSymbolTable().addSymbol(m_Markers[eRightButton]);
  globalSymbolTable().addSymbol(m_zoom);
  globalSymbolTable().addSymbol(m_pan);

  m_bFrozen = false;

  //
  // Create the signals for the scope window.
  //

  signals[0] = new Waveform(this,"scope.ch0");
  signals[1] = new Waveform(this,"scope.ch1");
  signals[2] = new Waveform(this,"scope.ch2");
  signals[3] = new Waveform(this,"scope.ch3");
  signals[4] = new Waveform(this,"scope.ch4");
  signals[5] = new Waveform(this,"scope.ch5");
  signals[6] = new Waveform(this,"scope.ch6");
  signals[7] = new Waveform(this,"scope.ch7");

  m_TimeAxis = new TimeAxis(this,"scope.time");

  if(enabled)
    Build();

}

// zoom(i)
//
  const int minSpan = 10;
void Scope_Window::zoom(int i)
{
  m_bFrozen = true;
  gint64 start = (gint64) m_Markers[eStart]->getVal();
  gint64 stop  = (gint64) m_Markers[eStop]->getVal();
  gint64 now  = (gint64) get_cycles().get();

  if (!stop)
    stop = now;

  gint64 mid  = (start + stop)/2;
  gint64 span = (stop - start)/2;


  if (i>0)
    span /= i;
  else
    span *= -i;

  span = span < minSpan ? minSpan : span;

  start = mid - span;
  stop  = mid + span;

  if (start > stop) {
    start = mid - 1;
    stop = mid + 1;
  }

  start = (start < 0) ? 0 : start;
  stop  = (stop >= now) ? 0 : stop;

  m_Markers[eStart]->set(start);
  m_Markers[eStop]->set(stop);

  m_bFrozen = false;

  Update();
}

void Scope_Window::pan(int i)
{

  gint64 start = i+(gint64) m_Markers[eStart]->getVal();
  gint64 stop  = (gint64) m_Markers[eStop]->getVal();
  gint64 now  = (gint64) get_cycles().get();

  if (start < 0)
    return;

  if (!stop)
    return;

  if ((stop+i) > now)
    return;

  m_Markers[eStart]->set(start);
  m_Markers[eStop]->set(stop+i);

}

gdouble Scope_Window::getSpan()
{
  guint64 start = m_Markers[eStart]->getVal();
  guint64 stop  = m_Markers[eStop]->getVal();
  stop = stop ? stop : get_cycles().get();
  return start > stop ? 0.0 : ((gdouble)(stop-start));
}

guint64 Scope_Window::mapPixelToTime(unsigned int pixel)
{
  gdouble x = (pixel>=0 && pixel<m_PixmapWidth) ? pixel : 0;

  return (guint64)(getSpan()*x/m_PixmapWidth + m_Markers[eStart]->getVal());
}

/// mapTimeToPixel - convert time to a pixel horizontal offset.
int Scope_Window::mapTimeToPixel(guint64 time)
{
  gdouble span = getSpan();
  guint64 start = m_Markers[eStart]->getVal();

  return (int) ((time>start && time<=(start+span)) ? ((time-start)*m_PixmapWidth)/span : 0);
}

#endif //HAVE_GUI
