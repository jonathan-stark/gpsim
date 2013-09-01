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

#include "config.h"
#ifdef HAVE_GUI
#include "glcd_100X32_sed1520.h"
#include "glcd.h"
#include "sed1520.h"

#include <src/stimuli.h>
#include <src/ioports.h>
#include <src/packages.h>
#include <src/symbol.h>
#include <src/trace.h>
#include <src/gpsim_interface.h>


#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


//========================================================================
//
// Describe the intent.....
//
//========================================================================



//------------------------------------------------------------------------
// I/O pins for the graphic LCD
//------------------------------------------------------------------------
// The LCD_InputPin is the base class for the E1, E2, RW and A0
// pins. This class is derived from the IO_bi_directional class,
// although the pins are never driven as outputs. The setDrivenState
// method is overridden to capture when this pin is driven.

class LCD_InputPin : public IO_bi_directional
{
public:

  LCD_InputPin (gLCD_100X32_SED1520 *, const char *pinName, ePins pin);

  // Capture when a node drives this pin.
  virtual void setDrivenState(bool new_dstate);

  char CurrentState() { return m_cDrivenState; }
private:

  // The IO_bi_directional constructor will initialize the direction
  // to be an input. We'll override the update_direction method so that
  // this will never change.

  virtual void update_direction(unsigned int,bool refresh)
  {
    // Disallow pin direction changes.
  }

  gLCD_100X32_SED1520 *m_pLCD;
  ePins m_pin;
  char m_cDrivenState;
};


//------------------------------------------------------------------------
//
LCD_InputPin::LCD_InputPin (gLCD_100X32_SED1520 *pLCD , const char *pinName, ePins pin)
  : IO_bi_directional(pinName), m_pLCD(pLCD), m_pin(pin), m_cDrivenState('Z')
{
  assert(m_pLCD);
}


void LCD_InputPin::setDrivenState(bool new_dstate)
{
  IO_bi_directional::setDrivenState(new_dstate);
  char cState = getBitChar();
  if (m_cDrivenState != cState) {
    m_cDrivenState = cState;
    m_pLCD->UpdatePinState(m_pin, cState);
  }
}

//------------------------------------------------------------------------
//

class LCDSignalControl : public SignalControl
{
public:
  LCDSignalControl(gLCD_100X32_SED1520 *pLCD)
    : m_pLCD(pLCD)
  {
    assert(m_pLCD);
  }
  virtual void release()
  {
  }
  virtual char getState()
  {
    // return the data bus direction (note the negative true logic.
    return m_pLCD->dataBusDirection() ? '0' : '1';
  }

private:
  gLCD_100X32_SED1520 *m_pLCD;
};


//------------------------------------------------------------------------
bool gLCD_100X32_SED1520::dataBusDirection()
{
  // FIXME, if both sed's are driving the data bus, then they'll be in
  // contention.
  return m_sed1->dataBusDirection() || m_sed2->dataBusDirection();
}
//------------------------------------------------------------------------
Module *gLCD_100X32_SED1520::construct(const char *_new_name=0)
{
  gLCD_100X32_SED1520 *gLCD = new gLCD_100X32_SED1520(_new_name);

  return gLCD;
}

//------------------------------------------------------------------------
gLCD_100X32_SED1520::gLCD_100X32_SED1520(const char *_new_name)
  : gLCD_Module(_new_name,"SED1520 100X32 Graphics LCD module",100,32)
{

  m_dataBus = new LcdPortRegister(this,".data","LCD Data Port");
  addSymbol(m_dataBus);
  m_dataBus->setEnableMask(0xff);

  m_A0      = new LCD_InputPin(this,(name() + ".a0").c_str(),eA0);
  m_E1      = new LCD_InputPin(this,(name() + ".e1").c_str(),eE1);
  m_E2      = new LCD_InputPin(this,(name() + ".e2").c_str(),eE2);
  m_RW      = new LCD_InputPin(this,(name() + ".rw").c_str(),eRW);

  m_sed1    = new SED1520();
  m_sed2    = new SED1520();

  m_sed1->randomizeRAM();
  m_sed2->randomizeRAM();

  create_iopin_map();

  m_plcd = 0;

  create_widget();

  printf ("gLCD_100X32_SED1520 constructor this=%p\n",this);
}

gLCD_100X32_SED1520::~gLCD_100X32_SED1520()
{
  delete m_dataBus;
  delete m_A0;
  delete m_E1;
  delete m_E2;
  delete m_RW;

  delete m_sed1;
  delete m_sed2;
}


//------------------------------------------------------------------------
void gLCD_100X32_SED1520::create_iopin_map()
{

  create_pkg(18);

  // Add the individual io pins to the data bus.

  assign_pin( 9, m_dataBus->addPin(new IO_bi_directional((name() + ".d0").c_str()),0));
  assign_pin(10, m_dataBus->addPin(new IO_bi_directional((name() + ".d1").c_str()),1));
  assign_pin(11, m_dataBus->addPin(new IO_bi_directional((name() + ".d2").c_str()),2));
  assign_pin(12, m_dataBus->addPin(new IO_bi_directional((name() + ".d3").c_str()),3));
  assign_pin(13, m_dataBus->addPin(new IO_bi_directional((name() + ".d4").c_str()),4));
  assign_pin(14, m_dataBus->addPin(new IO_bi_directional((name() + ".d5").c_str()),5));
  assign_pin(15, m_dataBus->addPin(new IO_bi_directional((name() + ".d6").c_str()),6));
  assign_pin(16, m_dataBus->addPin(new IO_bi_directional((name() + ".d7").c_str()),7));

  // Provide a SignalControl object that the dataBus port can query
  // to determine which direction to drive the data bus.
  // (See <gpsim/ioports.h> for more documentation on port behavior.
  //  But in summary, when an I/O port updates its I/O pins, it will
  //  query the pin drive direction via the SignalControl object. )

  SignalControl *pPortDirectionControl = new LCDSignalControl(this);
  for (int i=0; i<8; i++)
    (*m_dataBus)[i].setControl(pPortDirectionControl);

  assign_pin( 4, m_A0);
  assign_pin( 5, m_RW);
  assign_pin( 6, m_E1);
  assign_pin( 7, m_E2);

#if IN_BREADBOARD==1
  // Place pins along the left side of the package
  for (int i=1; i<=18; i++)
    package->setPinGeometry(i, 0.0, i*12.0, 0, true);
#endif

}

static gboolean lcd_expose_event(GtkWidget *widget,
				 GdkEventExpose *event,
				 gLCD_100X32_SED1520 *pLCD)
{
  //printf ("Expose event widget %p pLCD  %p\n",widget,pLCD);
  pLCD->Update(widget);
  return TRUE;
}

//------------------------------------------------------------------------
void gLCD_100X32_SED1520::Update(GtkWidget *widget)
{

  if (!m_plcd) {
    if (!darea || !darea->window)
      return;

    m_plcd = new gLCD(darea, m_nColumns, m_nRows, 3, 3, 1);

    printf("m_plcd %p\n",m_plcd);
  }

  assert (m_plcd !=0);

  m_plcd->clear();

  for (unsigned int i=0; i<m_nColumns; i++) {

    unsigned sedIndex = i < 50 ? i : (i-50);

    SED1520 *sed = i<50 ? m_sed1 : m_sed2;

    for (unsigned int j=0; j<m_nRows/8; j++) {

      unsigned int row = 8*j;
      unsigned displayByte = (*sed)[sedIndex + ((j&3)*80)];

      for (unsigned int b=0; b < 8; b++ , displayByte>>=1)
	if (displayByte & 1)
	  m_plcd->setPixel(i,row+b);
    }

  }
  m_plcd->refresh();
}

//------------------------------------------------------------------------
void gLCD_100X32_SED1520::create_widget()
{

#if IN_BREADBOARD==1
  window = gtk_vbox_new (FALSE, 0);
#else
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if(window) {
    //gtk_window_set_wmclass(GTK_WINDOW(window),type(),"Gpsim");
    gtk_window_set_wmclass(GTK_WINDOW(window),"glcd","Gpsim");
    gtk_widget_realize (window);
    gtk_window_set_title(GTK_WINDOW(window), "LCD");
  }
#endif

  if(window) {

    GtkWidget *frame = gtk_frame_new ("gLCD_100X32");
    gtk_container_add (GTK_CONTAINER (window), frame);

    darea = gtk_drawing_area_new ();

    gtk_widget_set_usize (darea,
			  (m_nColumns+4)*3, (m_nRows+4)*3
			  );
    gtk_container_add (GTK_CONTAINER (frame), darea);

    gtk_signal_connect (GTK_OBJECT (darea),
			"expose_event",
			GTK_SIGNAL_FUNC (lcd_expose_event),
			this);

    gtk_widget_set_events (darea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);

    /*
    gtk_signal_connect (GTK_OBJECT (darea),
			"button_press_event",
			GTK_SIGNAL_FUNC (cursor_event),
			NULL);
    */

    gtk_widget_show (frame);
    gtk_widget_show (darea);

    gtk_widget_show (window);

#if IN_BREADBOARD==1
    set_widget(window);
#endif
  }

}

//------------------------------------------------------------------------

void gLCD_100X32_SED1520::UpdatePinState(ePins pin, char cState)
{

  // One of the control lines has changed states. So refresh the
  // sed's with the most current data bus value.

  if (!m_sed1->dataBusDirection())
    m_sed1->driveDataBus(m_dataBus->get());

  if (!m_sed2->dataBusDirection())
    m_sed2->driveDataBus(m_dataBus->get());


  bool bState = (cState =='1') || (cState =='W');
  switch (pin) {
  case eA0:
    m_sed1->setA0(bState);
    m_sed2->setA0(bState);
    break;
  case eE1:
    m_sed1->setE(bState);
    break;
  case eE2:
    m_sed2->setE(bState);
    break;
  case eRW:
    m_sed1->setRW(bState);
    m_sed2->setRW(bState);
    break;
  }

  // If either one of the sed's is driving, then place that
  // data on to the data bus.
  if (m_sed1->dataBusDirection())
    m_dataBus->put(m_sed1->getDataBus());
  else if (m_sed2->dataBusDirection())
    m_dataBus->put(m_sed2->getDataBus());
  else
    m_dataBus->updatePort();

}
#endif // HAVE_GUI
