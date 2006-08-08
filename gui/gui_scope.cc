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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>

#include "../src/bitlog.h"
#include "../src/symbol.h"

#define DEBUG

#include "gui_scope.h"




// Number of Input Ports:
#define NUM_PORTS 8
// Number of signal lines bit points
#define MAX_BIT_POINTS 10000

// Zoom:
#define ZOOM_STEP 2
#define ZOOM_MIN 10
#define ZOOM_MAX MAX_BIT_POINTS

#define ZOOM_IN 0
#define ZOOM_OUT !ZOOM_IN

// Update Delay:
#define DELAY_DEFAULT_VALUE 10
#define DELAY_MIN_VALUE 0
#define DELAY_MAX_VALUE 100


static GtkObject *bit_adjust; // ,*delay_adjust;
static GdkColor signal_line_color,grid_line_color,grid_v_line_color;
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
  guint64 time;

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
GdkGC *drawing_gc=0;         // Line styles, etc.
GdkGC *text_gc=0;            // signal names
GdkGC *grid_gc=0;            // Grid color

/***********************************************************************
  Waveform class

  This holds the gui information related with a gpsim waveform
*/
class Waveform
{
public:

  bool isUpToDate;           // False when the waveform needs updating.

  Waveform(Scope_Window *parent, const char *name);

  void Build(PixMap *pWavePixmap, PixMap *pSignalPixmap);
  void Update(guint64 start=0, guint64 stop=0);
  void Expose();
  void Resize(int width, int height);
  void SearchAndPlot(timeMap &left, timeMap &right);
  void Dump(); // debug
  void setData(char c);
  void setSource(const char *);
  void setPixmaps(GdkPixmap *, GdkPixmap *);
  PixMap *wavePixmap() { return m_wavePixmap; }
  PixMap *signalPixmap() { return m_signalPixmap;}
protected:
  void PlotTo(timeMap &left, timeMap &right);

  Scope_Window *sw;          // Parent
  bool isBuilt;              // True after the gui has been built.
  PixMap *m_wavePixmap;      // The Waveform is rendered in this pixmap.
  PixMap *m_signalPixmap;    // The signal name is rendered in this pixmap.


  WaveformSink *m_pSink;
  ThreeStateEventLogger *m_logger;
  timeMap m_last;
  WaveformSource *m_pSourceName;
  PangoLayout *m_layout;     // Pango layout for rendering signal name


};


//------------------------------------------------------------------------
//
// Scope_Window data items that need to go into the Scope_Window class.
// The reason they're here now is simply for development convenience.
//
//

Waveform *signals[8];   // hack
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
protected:
  Waveform *m_pWaveform;
};


//========================================================================
class ZoomInEvent : public KeyEvent
{
public:
  void action(gpointer data)
  {
    //Scope_Window *sw = dynamic_cast<Scope_Window *>(data);
    //Scope_Window *sw = (Scope_Window *)(data);
    /*if (sw)
      sw->zoomIn();
    */
    cout << "ZoomIn\n";
  }
};
//========================================================================
// Signals

static int WaveformEntryActivate(GtkEntry *pEntry,
				 Waveform *pWaveform)
{
  if (pEntry) {

    if (pWaveform) {

      pWaveform->setSource(gtk_entry_get_text(pEntry));
    }

  }

  return TRUE;
}
//========================================================================

static map<guint, KeyEvent *> KeyMap;

static gint
key_press(GtkWidget *widget,
	  GdkEventKey *key, 
	  gpointer data)
{
  printf ("press\n");

  KeyEvent *pKE = KeyMap[key->keyval];
  if(pKE) 
    {
      pKE->action(data);
      return TRUE;
    }

  return FALSE;
}

static gint
key_release(GtkWidget *widget,
	  GdkEventKey *key, 
	  gpointer data)
{
  printf ("release\n");
  return TRUE;
}

static gint
button_press(GtkWidget *widget,
	  GdkEventKey *key, 
	  gpointer data)
{
  printf ("button\n");
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
  m_bClearableSymbol = false;
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
  m_bClearableSymbol = false;

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
  m_bClearableSymbol = false;

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

Waveform::Waveform(Scope_Window *parent, const char *name)
{
  isBuilt = false;
  isUpToDate = false;
  m_wavePixmap =0;
  m_signalPixmap = 0;

  sw = parent;

  //m_pEntry = 0;
  //m_pLabel = 0;
  m_layout = 0;

  m_pSink = new WaveformSink(this);
  m_logger = new ThreeStateEventLogger();
  m_pSourceName = new WaveformSource(this,name);
  get_symbol_table().add(m_pSourceName);

 // Test!!!
  m_logger->event('0');
}

void Waveform::setData(char c)
{
  m_logger->event(c);
}

void Waveform::setSource(const char *sourceName)
{
  IOPIN *ppin = dynamic_cast<IOPIN*>(get_symbol_table().findStimulus(sourceName));
  if (ppin) {
    PinMonitor *ppm = ppin->getMonitor();
    if (ppm)
      ppm->addSink(new WaveformSink(this));

    if (m_layout)
      pango_layout_set_text(m_layout,sourceName, -1);
    /*
    if (m_pEntry)
      gtk_entry_set_text(m_pEntry,sourceName);
    if (m_pLabel) {
      cout << "Set label to " << sourceName << endl;
      gtk_label_set_text(m_pLabel,sourceName);
      gtk_widget_show(GTK_WIDGET(m_pLabel));
      }*/
  } else if(sourceName)
     printf("%s is not a valid source for the scope\n",sourceName);
}

static gint Waveform_expose_event (GtkWidget *widget,
				   GdkEventExpose  *event,
				   gpointer   user_data)
{
  cout << __FUNCTION__ << endl;
  /*
  Waveform *wf = (Waveform *) user_data;
  wf->Expose();
  */
  return FALSE;
}

void Waveform::Build(PixMap *pWavePixmap, PixMap *pSignalPixmap)
{

  if (m_wavePixmap && m_wavePixmap->pixmap())
    gdk_pixmap_unref(m_wavePixmap->pixmap());
  
  m_wavePixmap = pWavePixmap;

  if (m_signalPixmap && m_signalPixmap->pixmap())
    gdk_pixmap_unref(m_signalPixmap->pixmap());
  
  m_signalPixmap = pSignalPixmap;

  KeyMap['z'] = new ZoomInEvent();
  KeyMap['Z'] = KeyMap['z'];
  //KeyMap['l'] = new PanLeftEvent();
  //KeyMap['r'] = new PanRightEvent();


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
  GTK_WIDGET_SET_FLAGS( waveDrawingArea,
			GTK_CAN_FOCUS );
  /*
  gtk_signal_connect (GTK_OBJECT (waveDrawingArea),
		      "expose_event",
		      GTK_SIGNAL_FUNC (Waveform_expose_event),
		      this);
  */

  m_layout = gtk_widget_create_pango_layout (GTK_WIDGET (signalDrawingArea), "");

  isBuilt = true;
  isUpToDate = false;

  Update(0,0);
}

//----------------------------------------
void Waveform::Resize(int w, int h)
{

  /*
  if(m_wavePixmap && w==width && h==height)
    return;

  if(w<100 || h<5)
    return;

  cout << "Waveform::" << __FUNCTION__ << endl;

    
  width = w;
  height = h;
  gdk_draw_layout (GDK_DRAWABLE(m_signalPixmap),
		   drawing_gc,
		   0,
		   10,
		   layout);

  //Build(row);

  isUpToDate = false;
  */
  Update();

}

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

  gdk_draw_line(m_wavePixmap->pixmap(),drawing_gc,
		right.pos, m_last.event,    // last point drawn
		right.pos, nextEvent); // next event

  // Draw a thicker line if there more than one event.
    
  if (right.eventIndex+1 > left.eventIndex)
    gdk_draw_line(m_wavePixmap->pixmap(),drawing_gc,
		  left.pos, m_last.event,
		  left.pos, nextEvent);
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
    // The region cannot be divided any smaller.
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
    mid.eventIndex = m_logger->get_index (mid.time);

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
  int x;
  GdkRectangle update_rect;
#if 0
  int line_separation,pin_number;
  int point,y_text,y_0,y_1;
  float x_scale,y_scale;
  int max_str,new_str,br_length;
  char *s,ss[10];
#endif
  isUpToDate = false;

  if(!isBuilt || isUpToDate)
    return;

  if(!m_wavePixmap) {
    cout << __FUNCTION__ << " pixmap is NULL\n";
    return;
  }

  //cout << "Waveform::" << __FUNCTION__ << endl;

  if (uiStart == 0) //fixme - boundary condition at t=0 is broken.
    uiStart = 1;

  if (uiEnd == 0) 
    uiEnd = get_cycles().value;

  gdk_draw_rectangle (m_wavePixmap->pixmap(),
		      waveDrawingArea->style->black_gc,
		      TRUE,
		      0, 0,
		      m_wavePixmap->width,
		      m_wavePixmap->height);

  gdk_draw_rectangle (m_signalPixmap->pixmap(),
		      waveDrawingArea->style->black_gc,
		      TRUE,
		      0, 0,
		      m_signalPixmap->width,
		      m_signalPixmap->height);

  if (m_layout) {
    int text_height=0;
    pango_layout_get_pixel_size(m_layout,
				NULL,
				&text_height);
    
    gdk_draw_layout (GDK_DRAWABLE(m_signalPixmap->pixmap()),
		     text_gc,
		     0,
		     (m_signalPixmap->height-text_height)/2,
		     m_layout);
  }

#if 0
  y_scale = (float)height / (float)(NUM_PORTS);
    

  
  char ntest[] = "test0";
  // Draw pin name:
  max_str = 0;
  for (pin_number=1;pin_number<=NUM_PORTS;pin_number++)
    {
      y = (int)((float)y_scale*(float)(pin_number)-(float)(y_scale/4));
      //s = Package::get_pin_name(pin_number);
      ntest[4] = pin_number + '0';
      gdk_draw_text (m_wavePixmap,drawing_area->style->font,
		     drawing_area->style->white_gc,0,y,ntest,strlen(ntest));
      new_str = gdk_text_width (drawing_area->style->font,ntest,strlen(ntest));
      if (new_str>max_str)
	max_str=new_str;
    }
  y_text = y;
#endif

  //
  // Draw Vertical Grid Lines:
  //

  for(x=0; x<m_wavePixmap->width; x+= m_wavePixmap->width/20)
    gdk_draw_line(m_wavePixmap->pixmap(),grid_gc,x,1,x,m_wavePixmap->height-1);

  //
  // Draw Horizontal Grid Lines:
  //
  gdk_draw_line(m_wavePixmap->pixmap(),grid_gc,
		0,m_wavePixmap->height-1,
		m_wavePixmap->width,m_wavePixmap->height-1);
  
  if (uiEnd == 0)
    return; 

  // Draw Signals:

  timeMap left;
  timeMap right;

  left.pos = 0;
  left.time = uiStart;
  left.eventIndex = m_logger->get_index(uiStart);
  left.event = (m_logger->get_state(left.eventIndex) == '1') ? 1 : (m_wavePixmap->height-3);

  m_last = left;

  right.pos = m_wavePixmap->width;
  right.time = uiEnd;
  right.eventIndex = m_logger->get_index(uiEnd);


  SearchAndPlot(left,right);
  if (right.pos > m_last.pos)
    gdk_draw_line(m_wavePixmap->pixmap(),drawing_gc,
		  m_last.pos, m_last.event,   // last point drawn
		  right.pos,  m_last.event);  // right most point


#if 0
  // Draw bit positions:
  sprintf (ss,"[%d]",bit_left);
  gdk_draw_text (m_wavePixmap,drawing_area->style->font,
		 drawing_area->style->white_gc,
		 max_str,(int)y,ss,strlen(ss));
  sprintf (ss,"[%d]",bit_right);
  br_length = gdk_text_width (drawing_area->style->font,ss,strlen(ss));
  gdk_draw_text (m_wavePixmap,drawing_area->style->font,
		 drawing_area->style->white_gc,
		 width-br_length,(int)y,ss,strlen(ss));

#endif

  isUpToDate = true;

  update_rect.x = 0;
  update_rect.y = 0;
  update_rect.width = m_wavePixmap->width;
  update_rect.height = m_wavePixmap->height;
  gtk_widget_draw (waveDrawingArea,
		   &update_rect);

  Expose();
}

//----------------------------------------
//
// Waveform Expose
//

void Waveform::Expose()
{
  if (sw)
    sw->Expose(this);
  /*
  if(!isBuilt || !m_wavePixmap || !waveDrawingArea)
    return;

  if(!isUpToDate)
    Update();

  gdk_draw_pixmap(waveDrawingArea->window,
		  waveDrawingArea->style->fg_gc[GTK_WIDGET_STATE (waveDrawingArea)],
		  m_wavePixmap->pixmap(),
		  0,0,   // x,y
		  0,0,
		  m_wavePixmap->width,m_wavePixmap->height);

  gtk_widget_show(waveDrawingArea);

  gdk_draw_pixmap(signalDrawingArea->window,
		  signalDrawingArea->style->fg_gc[GTK_WIDGET_STATE (signalDrawingArea)],
		  m_signalPixmap->pixmap(),
		  0,0,   // x,y
		  0,0,
		  m_signalPixmap->width,m_signalPixmap->height);
  gtk_widget_show(signalDrawingArea);
  */

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

static gint
analyzer_clear_callback (GtkWidget *widget, gpointer user_data)
{
  //    Analyzer_Screen *as=(Analyzer_Screen*)user_data;
  cout <<  "function:" << __FUNCTION__ << "\n";    

  //as->port->init_bit_points();
  //  as->update();

  cout <<  "End of function:" << __FUNCTION__ << "\n";
  return(FALSE);    
}

static gint
analyzer_update_scale (GtkAdjustment *adj,gpointer user_data)
{
  //    Analyzer_Screen *as=(Analyzer_Screen*)user_data;
//    cout << "value:" << (int)adj->value << "\n";
  cout <<  "function:" << __FUNCTION__ << "\n";    
  //as->set_bit_left((int)adj->value);
  //as->update();
  return(FALSE);
}

static gint Scope_Window_expose_event (GtkWidget *widget,
				   GdkEventExpose  *event,
				   gpointer   user_data)
{
  // Dprintf(( " %s\n",__FUNCTION__));

  g_return_val_if_fail (widget != NULL, TRUE);
  //  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  Scope_Window *sw = (Scope_Window *)user_data;
  if(sw) {
    for (int i=0; i<8; i++)
      if (signals[i])
	signals[i]->Expose();
  }

  return FALSE;
}
static gint DrawingArea_expose_event (GtkWidget *widget,
				      GdkEventExpose  *event,
				      gpointer   user_data)
{
  // Dprintf(( " %s\n",__FUNCTION__));

  Scope_Window *sw = (Scope_Window *)user_data;
  if(sw) {
    for (int i=0; i<8; i++)
      if (signals[i])
	signals[i]->Expose();
  }

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
  m_bClearableSymbol = false;
}
void TimeMarker::set(gint64 i)
{
  Integer::set(i);
  m_pParent->Update();
}

static gdouble gNormalizedHorizontalPosition=0.0;
static GtkWidget *pHpaned=0;

void Scope_Window::Expose(Waveform *wf)
{
  if(!wf || !waveDrawingArea)
    return;

  if (!wf->isUpToDate)
    wf->Update();

  
  //  int xoffset = (int)(1000*gNormalizedHorizontalPosition);
  PixMap *pm = wf->wavePixmap();
  GtkRequisition panesize;
  gtk_widget_size_request (pHpaned, &panesize);

  int xoffset = (int)((pm->width - (panesize.width - gtk_paned_get_position(GTK_PANED(pHpaned))))
		      *gNormalizedHorizontalPosition);

  gdk_draw_pixmap(waveDrawingArea->window,
		  waveDrawingArea->style->fg_gc[GTK_WIDGET_STATE (waveDrawingArea)],
		  pm->pixmap(),
		  xoffset,0,            // source
		  0,pm->yoffset,  // destination
		  pm->width,pm->height);

  gtk_widget_show(waveDrawingArea);

  pm = wf->signalPixmap();
  gdk_draw_pixmap(signalDrawingArea->window,
		  signalDrawingArea->style->fg_gc[GTK_WIDGET_STATE (signalDrawingArea)],
		  pm->pixmap(),
		  0,0,            // source
		  0,pm->yoffset,  // destination
		  pm->width,pm->height);
  gtk_widget_show(signalDrawingArea);

}


//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
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

static void hAdjVChange(GtkAdjustment *pAdj,
			gpointer       user_data)
{

  gdouble width = pAdj->upper - pAdj->lower - pAdj->page_size;
  gNormalizedHorizontalPosition = pAdj->value/ (width ? width : 1.0);

  /*
  Dprintf((" %s low=%g up=%g v=%g step=%g page_in=%g page_size=%g position=%g\n",__FUNCTION__,
	  pAdj->lower, pAdj->upper, pAdj->value,
	  pAdj->step_increment, pAdj->page_increment, pAdj->page_size,
	   gNormalizedHorizontalPosition));
  */
}

static void HorizontalAdjustment(GtkWidget *widget, Scope_Window *sw)
{
  if (!sw)
    return;

  GtkAdjustment *pAdj = GTK_ADJUSTMENT(widget);

  gdouble width = pAdj->upper - pAdj->lower - pAdj->page_size;
  gdouble normalized_position = pAdj->value/ (width ? width : 1.0);
  printf (" %s low=%g up=%g v=%g step=%g page_in=%g page_size=%g position=%g\n",__FUNCTION__,
	  pAdj->lower, pAdj->upper, pAdj->value,
	  pAdj->step_increment, pAdj->page_increment, pAdj->page_size,
	  normalized_position);

}
static void VerticalAdjustment(GtkWidget *widget, Scope_Window *sw)
{
  /*
    if(GTK_LAYOUT (bbw->layout)->bin_window==0)
	return;

    if(bbw->layout_pixmap==0)
    {
	puts("bbw.c: no pixmap4!");
	return;
    }

    int xoffset=0, yoffset=0;
    
    GtkAdjustment *xadj, *yadj;
    xadj = gtk_layout_get_hadjustment (GTK_LAYOUT(bbw->layout));
    yadj = gtk_layout_get_vadjustment (GTK_LAYOUT(bbw->layout));
    xoffset = (int) GTK_ADJUSTMENT(xadj)->value;
    yoffset = (int) GTK_ADJUSTMENT(yadj)->value;

    gdk_draw_pixmap(GTK_LAYOUT (bbw->layout)->bin_window,
		    bbw->window->style->white_gc,
		    bbw->layout_pixmap,
		    xoffset, yoffset,
#if GTK_MAJOR_VERSION >= 2
		    xoffset, yoffset,
#else
		    0, 0,
#endif
		    bbw->layout->allocation.width,
		    bbw->layout->allocation.height);
  */
  printf (" %s\n",__FUNCTION__);
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

void Scope_Window::Build(void)
{

  GtkWidget *scroll_bar,*button;
  GtkTooltips *tooltips;    

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (!window)
    return;


  // The "realize" operation creates basic resources like GC's etc. that 
  // are referenced below. Without this call, those resources will be NULL.
  gtk_widget_realize (window);

  gtk_window_set_title(GTK_WINDOW(window), "Scope");


  tooltips = gtk_tooltips_new();



  //
  // Control buttons
  // (this is changing...)

#if 0
  button = gtk_button_new_with_label ("Clear");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (analyzer_clear_callback),this);
  gtk_table_attach_defaults (GTK_TABLE(table),button,0,2,9,10);
#endif

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


#define port_get_max_bit_points 200

#if 0
  //
  // Horizontal Scroll Bar
  //

  bit_adjust = gtk_adjustment_new(0,0,port_get_max_bit_points,1,10,ZOOM_MIN);
  gtk_signal_connect (GTK_OBJECT (bit_adjust), "value_changed",
		      GTK_SIGNAL_FUNC (analyzer_update_scale), this);
  scroll_bar = gtk_hscrollbar_new(GTK_ADJUSTMENT(bit_adjust));
  gtk_table_attach_defaults (GTK_TABLE(table),scroll_bar,0,10,8,9);
#endif

#if 0

  // do we really want to have an update delay?

  delay_adjust = gtk_adjustment_new(update_delay,
				    DELAY_MIN_VALUE,
				    DELAY_MAX_VALUE,1,5,0);
  gtk_signal_connect (GTK_OBJECT (delay_adjust), "value_changed",
		      GTK_SIGNAL_FUNC (analyzer_update_delay), this);
  spin_button = gtk_spin_button_new(GTK_ADJUSTMENT(delay_adjust),0.5,0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button),TRUE);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_button),TRUE);
  gtk_table_attach_defaults (GTK_TABLE(table),spin_button,8,10,9,10);
  gtk_tooltips_set_tip(tooltips,spin_button,"Set Update Delay",NULL);
#endif

  //
  // Define the drawing colors
  //

  // The signal color is bright red
  signal_line_color.red = 0xff00;
  signal_line_color.green = 0x0000;
  signal_line_color.blue = 0x0000;
  gdk_color_alloc(gdk_colormap_get_system(), &signal_line_color);
  // The grid color is bright green
  grid_line_color.red = 0x4000;
  grid_line_color.green = 0x4000;
  grid_line_color.blue = 0x4000;
  gdk_color_alloc(gdk_colormap_get_system(), &grid_line_color);
  // The vertical grid color is dark green
  grid_v_line_color.red = 0x0000;
  grid_v_line_color.green = 0x2200;
  grid_v_line_color.blue = 0x0000;
  gdk_color_alloc(gdk_colormap_get_system(), &grid_v_line_color);


  waveDrawingArea = gtk_drawing_area_new ();
  gint dawidth  = 400;
  gint daheight = 100;

  gtk_widget_set_usize (waveDrawingArea,dawidth,daheight);
  gtk_widget_set_events (waveDrawingArea,
			 GDK_EXPOSURE_MASK | 
			 GDK_BUTTON_PRESS_MASK | 
			 GDK_KEY_PRESS_MASK | 
			 GDK_KEY_RELEASE_MASK  );

  signalDrawingArea = gtk_drawing_area_new ();
  gtk_widget_set_usize (signalDrawingArea,100,daheight);
  gtk_widget_set_events (signalDrawingArea,
			 GDK_EXPOSURE_MASK | 
			 GDK_BUTTON_PRESS_MASK | 
			 GDK_KEY_PRESS_MASK | 
			 GDK_KEY_RELEASE_MASK  );



  GtkWidget *vbox = gtk_vbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (window), vbox);
  


  pHpaned = gtk_hpaned_new ();
  gtk_widget_show (pHpaned);

  gtk_box_pack_start_defaults (GTK_BOX (vbox), pHpaned);


  GtkObject *hAdj = gtk_adjustment_new 
    (0.0,    // value
     0.0,    // lower
     1000.0,  // upper
     10.0,   // step_increment
     100.0,  // page_increment
     200.0); // page_size

  GtkWidget *hScrollBar = gtk_hscrollbar_new(GTK_ADJUSTMENT(hAdj));
  gtk_box_pack_start_defaults(GTK_BOX(vbox),hScrollBar);
  gtk_signal_connect(hAdj,"value-changed",
		     (GtkSignalFunc) hAdjVChange, this);

  // Add the drawing areas to the panes
  GtkWidget *pFrame = gtk_frame_new("");
  gtk_paned_add1(GTK_PANED(pHpaned), pFrame);
  gtk_container_add(GTK_CONTAINER(pFrame), signalDrawingArea);

  pFrame = gtk_frame_new("");
  gtk_paned_add2(GTK_PANED(pHpaned), pFrame);
  gtk_container_add(GTK_CONTAINER(pFrame), waveDrawingArea);

  gtk_widget_show(waveDrawingArea);

  // Graphics Context:
  drawing_gc = gdk_gc_new(waveDrawingArea->window);
  grid_gc = gdk_gc_new(waveDrawingArea->window);

  gdk_gc_set_foreground(grid_gc,&grid_line_color);    
  gdk_gc_set_foreground(drawing_gc,&signal_line_color);    
  text_gc = waveDrawingArea->style->white_gc;

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

  for(int i=0; i<8; i++) {
    int waveHeight=20;
    signals[i]->Build(new PixMap(waveDrawingArea->window, 1000, waveHeight, i*waveHeight),
		      new PixMap(waveDrawingArea->window, 100, waveHeight, i*waveHeight));
  }

  gtk_signal_connect (GTK_OBJECT (waveDrawingArea),
		      "expose_event",
		      GTK_SIGNAL_FUNC (DrawingArea_expose_event),
		      this);
  
  gtk_widget_show_all (window);
    
  //    cout <<  "end function:" << __FUNCTION__ << "\n";
  bIsBuilt = true;

  UpdateMenuItem();

  aw = window->allocation.width;
  ah = window->allocation.height;

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
  if(aw != window->allocation.width ||
     ah != window->allocation.height) {
    
    aw = window->allocation.width;
    ah = window->allocation.height;

    for(i=0; i<8; i++) {
      if(signals[i])
	signals[i]->Resize(aw-15,(ah-10)/10);
    }

  }

  guint64 start = m_tStart->getVal();
  guint64 stop  = m_tStop->getVal();

  for(i=0; i<8; i++) {

    if(signals[i])
      signals[i]->Update(start,stop);

  }
  // Debug
  //  signals[0]->Dump();

  gtk_widget_show_all(window);

    
}

Scope_Window::Scope_Window(GUI_Processor *_gp)
{

  gp = _gp;
  window = 0;
  wc = WC_data;
  wt = WT_scope_window;

  menu = "<main>/Windows/Scope";
  set_name("scope");

  get_config();

  m_tStart = new TimeMarker(this, "scope.start", "Scope window start time");
  m_tStop  = new TimeMarker(this, "scope.stop",  "Scope window stop time");

  m_zoom = new ZoomAttribute(this);
  m_pan  = new PanAttribute(this);

  get_symbol_table().add(m_tStart);
  get_symbol_table().add(m_tStop);
  get_symbol_table().add(m_zoom);
  get_symbol_table().add(m_pan);

  m_bFrozen = false;

  if(enabled)
    Build();

}

void Scope_Window::zoom(int i)
{
  cout << "zoom " << i << endl;

  m_bFrozen = true;
  gint64 start = (gint64) m_tStart->getVal();
  gint64 stop  = (gint64) m_tStop->getVal();

  gint64 mid  = (start + stop)/2;
  gint64 span = (stop - start)/2;
  if (i>0)
    span /= i;
  else
    span *= -i;
  start = mid - span;
  stop  = mid + span;

  gint64 now  = (gint64) get_cycles().value;

  if (start > stop) {
    start = mid - 1;
    stop = mid + 1;
  }
  
  start = (start < 0) ? 0 : start;
  stop  = (stop > now) ? now : stop;

  m_tStart->set(start);
  m_bFrozen = false;
  m_tStop->set(stop);

}

void Scope_Window::pan(int i)
{
  cout << "pan " << i << endl;
  m_bFrozen = true;
  gint64 start = i+(gint64) m_tStart->getVal();
  gint64 stop  = i+(gint64) m_tStop->getVal();
  gint64 now  = (gint64) get_cycles().value;

  start = (start < 0) ? 0 : start;
  stop  = (stop > now) ? now : stop;

  m_tStart->set(start);
  m_bFrozen = false;
  m_tStop->set(stop);

}
#endif //HAVE_GUI
