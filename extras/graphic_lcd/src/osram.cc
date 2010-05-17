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

#include "osram.h"
#include "glcd.h"
#include "ssd0323.h"

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

namespace OSRAM
{

  //------------------------------------------------------------------------
  // Debug Stuff
  //------------------------------------------------------------------------

  class StateAttribute : public Integer
  {
  public:
    StateAttribute (SSD0323 *pSSD)
      : Integer("state",0,"Display the state of the SSD0323 graphics controller"),
        m_pSSD(pSSD)
    {
      assert(pSSD);
    }
    virtual string toString()
    {
      m_pSSD->showState();
      return string("");
    }
  private:
    SSD0323 *m_pSSD;
  };

  //------------------------------------------------------------------------
  // I/O pins for the SSD 0323 controller
  //------------------------------------------------------------------------
  // The LCD_InputPin is the base class for the E1, E2, RW and A0
  // pins. This class is derived from the IO_bi_directional class,
  // although the pins are never driven as outputs. The setDrivenState
  // method is overridden to capture when this pin is driven.
  // NOTE: an alternative implementation would be to instantiate
  // PinModule objects for each of the pins and supply them with
  // standard IO_bi_directional objects. Then SignalSinks could be
  // created to capture pin state changes.

  class SSD0323_InputPin : public IO_bi_directional
  {
  public:

    SSD0323_InputPin (SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName);

    // Capture when a node drives this pin.
    virtual void setDrivenState(bool new_dstate);

    virtual void UpdateControllerPin(bool bState) {}
    char CurrentState() { return m_cDrivenState; }

  protected:

    SSD0323 *m_pSSD0323;
    PortRegister *m_pDataBus;
    char m_cDrivenState;

  private:

    // The IO_bi_directional constructor will initialize the direction
    // to be an input. We'll override the update_direction method so that
    // this will never change.

    virtual void update_direction(unsigned int,bool refresh)
    {
      // Disallow pin direction changes.
    }

  };

  SSD0323_InputPin::SSD0323_InputPin (SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
    : IO_bi_directional(pinName), 
      m_pSSD0323(pSSD),
      m_pDataBus(pDataBus),
      m_cDrivenState('Z')
  {
    assert(m_pSSD0323);
  }
  void SSD0323_InputPin::setDrivenState(bool new_dstate)
  {
    IO_bi_directional::setDrivenState(new_dstate);

    if (!m_pSSD0323->dataBusDirection())
      m_pSSD0323->driveDataBus(m_pDataBus->get());

    char cState = getBitChar();
    if (m_cDrivenState != cState) {
      m_cDrivenState = cState;
      //cout << "LCD pin:" << name() << "-->"<<cState<<endl;
      UpdateControllerPin((cState =='1') || (cState =='W'));
    }

    if (m_pSSD0323->dataBusDirection())
      m_pDataBus->put(m_pSSD0323->getDataBus());

  }

  //------------------------------------------------------------------------
  // SSD E Pin
  //------------------------------------------------------------------------
  class SSD0323_EPin : public SSD0323_InputPin
  {
  public:
    SSD0323_EPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
      : SSD0323_InputPin(pSSD,pDataBus,pinName)
    {}
    virtual void UpdateControllerPin(bool);
  };

  void SSD0323_EPin::UpdateControllerPin(bool bState)
  {
    m_pSSD0323->setE_RD(bState);
  }

  //------------------------------------------------------------------------
  // SSD CS Pin
  //------------------------------------------------------------------------
  class SSD0323_CSPin : public SSD0323_InputPin
  {
  public:
    SSD0323_CSPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
      : SSD0323_InputPin(pSSD,pDataBus,pinName)
    {}
    virtual void UpdateControllerPin(bool);
  };

  void SSD0323_CSPin::UpdateControllerPin(bool bState)
  {
    m_pSSD0323->setCS(bState);
  }

  //------------------------------------------------------------------------
  // SSD RES Pin
  //------------------------------------------------------------------------
  class SSD0323_RESPin : public SSD0323_InputPin
  {
  public:
    SSD0323_RESPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
      : SSD0323_InputPin(pSSD,pDataBus,pinName)
    {}
    virtual void UpdateControllerPin(bool);
  };

  void SSD0323_RESPin::UpdateControllerPin(bool bState)
  {
    m_pSSD0323->setRES(bState);
  }

  //------------------------------------------------------------------------
  // SSD CS Pin
  //------------------------------------------------------------------------
  class SSD0323_DCPin : public SSD0323_InputPin
  {
  public:
    SSD0323_DCPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
      : SSD0323_InputPin(pSSD,pDataBus,pinName)
    {}
    virtual void UpdateControllerPin(bool);
  };

  void SSD0323_DCPin::UpdateControllerPin(bool bState)
  {
    m_pSSD0323->setDC(bState);
  }

  //------------------------------------------------------------------------
  // SSD RW Pin
  //------------------------------------------------------------------------
  class SSD0323_RWPin : public SSD0323_InputPin
  {
  public:
    SSD0323_RWPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName)
      : SSD0323_InputPin(pSSD,pDataBus,pinName)
    {}
    virtual void UpdateControllerPin(bool);
  };

  void SSD0323_RWPin::UpdateControllerPin(bool bState)
  {
    m_pSSD0323->setRW(bState);
  }

  //------------------------------------------------------------------------
  // SSD BS1, BS2 Pin
  //------------------------------------------------------------------------
  class SSD0323_BSPin : public SSD0323_InputPin
  {
  public:
    SSD0323_BSPin(SSD0323 *pSSD, PortRegister *pDataBus, const char *pinName, unsigned pin)
      : SSD0323_InputPin(pSSD,pDataBus,pinName),
        m_pin(pin)
    {
      assert(m_pin==1 || m_pin==2);
    }
    virtual void UpdateControllerPin(bool);
  private:
    unsigned int m_pin;
  };

  void SSD0323_BSPin::UpdateControllerPin(bool bState)
  {
    m_pSSD0323->setBS(m_pin,bState);
  }

  //------------------------------------------------------------------------
  // SSD BS1, BS2 Pin
  //------------------------------------------------------------------------
  class SSD_SPISignalSink : public SignalSink
  {
  public:
    SSD_SPISignalSink(SSD0323 *pSSD, bool bClk_or_Data)
      : m_pSSD0323(pSSD),
        m_bClk(bClk_or_Data),
        m_cState(0)
    {
      assert(m_pSSD0323);
    }
    virtual void release()
    {
    }
    virtual void setSinkState(char c)
    {
      if (m_cState != c) {
        //cout << __FUNCTION__ << " new state: " << c << endl;
        if (m_bClk)
          m_pSSD0323->setSCLK(c=='1' || c=='W');
        else
          m_pSSD0323->setSDIN(c=='1' || c=='W');

        m_cState = c;
      }
    }
  private:
    SSD0323 *m_pSSD0323;
    bool m_bClk;
    char m_cState;
  };

  //------------------------------------------------------------------------
  //

  class LCDSignalControl : public SignalControl
  {
  public:
    LCDSignalControl(PK27_Series *pLCD)
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
    PK27_Series *m_pLCD;
  };

  //------------------------------------------------------------------------
  // PK27_Series -- Module constructor.
  //------------------------------------------------------------------------
  
  Module *PK27_Series::construct(const char *_new_name)
  {
    PK27_Series *pLCD = new PK27_Series(_new_name);
    pLCD->create_widget();
    return pLCD;
  }
  
  //------------------------------------------------------------------------
  // PK27_Series -- 128X64 - 4-bit gray scale.
  //------------------------------------------------------------------------
  PK27_Series::PK27_Series(const char *_new_name)
    : gLCD_Module(_new_name,"OSRAM 128X64 Graphics OLED module",128,64)
  {

    m_pSSD0323 = new SSD0323();
    m_pSSD0323->setBS(0, false);

    m_dataBus = new LcdPortRegister(this,".data","LCD Data Port");
    addSymbol(m_dataBus);
    m_dataBus->setEnableMask(0xff);

    m_CS      = new SSD0323_CSPin(m_pSSD0323, m_dataBus, (name() + ".cs").c_str());
    m_RES     = new SSD0323_RESPin(m_pSSD0323, m_dataBus, (name() + ".res").c_str());
    m_DC      = new SSD0323_DCPin(m_pSSD0323, m_dataBus, (name() + ".dc").c_str());
    m_E       = new SSD0323_EPin(m_pSSD0323, m_dataBus, (name() + ".e").c_str());
    m_RW      = new SSD0323_RWPin(m_pSSD0323, m_dataBus, (name() + ".rw").c_str());
    m_BS1     = new SSD0323_BSPin(m_pSSD0323, m_dataBus, (name() + ".bs1").c_str(),1);
    m_BS2     = new SSD0323_BSPin(m_pSSD0323, m_dataBus, (name() + ".bs2").c_str(),2);

    m_state   = new StateAttribute(m_pSSD0323);
    addSymbol(m_state);
    /*
      m_sed1->randomizeRAM();
      m_sed2->randomizeRAM();
    */

    create_iopin_map();

    m_plcd = 0;

    //create_widget();

    printf ("OSRAM PK27_Series constructor this=%p\n",this);

  }


  //------------------------------------------------------------------------
  // ~PK27_Series
  //------------------------------------------------------------------------
  PK27_Series::~PK27_Series()
  {
  }


  static gboolean lcd_expose_event(GtkWidget *widget,
                                   GdkEventExpose *event,
                                   PK27_Series *pLCD)
  {
    printf ("Expose event widget %p pLCD  %p\n",widget,pLCD);
    pLCD->Update(widget);
    return TRUE;
  }

  //------------------------------------------------------------------------
  // PK27_Series::Update
  //------------------------------------------------------------------------
  const int pixelScale = 2;
  void PK27_Series::Update(GtkWidget *widget)
  {

    if (!m_plcd) {
      if (!darea || !darea->window)
        return;

      //m_plcd = new gLCD(GDK_DRAWABLE(darea->window), m_nColumns, m_nRows, 3, 3);
      m_plcd = new gLCD(darea, m_nColumns, m_nRows, pixelScale, pixelScale, 0, 16);

      for (int i=0; i<16; i++) {
        unsigned int c = (255*i) / 16;
        m_plcd->setColor(i, c,c,0);
      }
      printf("m_plcd %p\n",m_plcd);

      m_plcd->clear();

      for (int j=0; j<31; j++) {
        for (int i=0; i<32; i++)
          m_plcd->setPixel(j,i,j/2);
      }

    }

    assert (m_plcd !=0);

    m_plcd->clear();
    /**/

    for (unsigned int row=0; row<m_nRows; row++) {
      for (unsigned int i=0; i<m_nColumns/2; i++) {

        unsigned int displayByte = (*m_pSSD0323)[i + 64*row];
        for (unsigned int b=0; b < 2; b++ , displayByte<<=4)
          m_plcd->setPixel(2*i+b,row,(displayByte>>4)&0xf);
      }

    }


    m_plcd->refresh();
  }

  //------------------------------------------------------------------------
  // PK27_Series::create_widget
  //------------------------------------------------------------------------
#define IN_BREADBOARD 0
  void PK27_Series::create_widget()
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

      GtkWidget *frame = gtk_frame_new ("OSRAM PK27_Series");
      gtk_container_add (GTK_CONTAINER (window), frame);

      darea = gtk_drawing_area_new ();

      gtk_widget_set_usize (darea,
                            (m_nColumns+3)*pixelScale, (m_nRows+3)*pixelScale
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
  // PK27_Series::create_widget
  //------------------------------------------------------------------------

  void PK27_Series::create_iopin_map()
  {

    create_pkg(30);

    assign_pin(20,m_BS1);
    assign_pin(19,m_BS2);

    assign_pin(17,m_CS);
    assign_pin(16,m_RES);
    assign_pin(15,m_DC);
    assign_pin(14,m_RW);
    assign_pin(13,m_E);

    // Add the individual io pins to the data bus.
    IO_bi_directional *pD0;
    IO_bi_directional *pD1;

    assign_pin(12, m_dataBus->addPin(pD0 = new IO_bi_directional((name() + ".d0").c_str()),0));
    assign_pin(11, m_dataBus->addPin(pD1 = new IO_bi_directional((name() + ".d1").c_str()),1));
    assign_pin(10, m_dataBus->addPin(new IO_bi_directional((name() + ".d2").c_str()),2));
    assign_pin( 9, m_dataBus->addPin(new IO_bi_directional((name() + ".d3").c_str()),3));
    assign_pin( 8, m_dataBus->addPin(new IO_bi_directional((name() + ".d4").c_str()),4));
    assign_pin( 7, m_dataBus->addPin(new IO_bi_directional((name() + ".d5").c_str()),5));
    assign_pin( 6, m_dataBus->addPin(new IO_bi_directional((name() + ".d6").c_str()),6));
    assign_pin( 5, m_dataBus->addPin(new IO_bi_directional((name() + ".d7").c_str()),7));

    m_dataBus->addSink(new SSD_SPISignalSink(m_pSSD0323, true),0);  // SPI CLK
    m_dataBus->addSink(new SSD_SPISignalSink(m_pSSD0323, false),1); // SPI Data

    // Provide a SignalControl object that the dataBus port can query
    // to determine which direction to drive the data bus.
    // (See <gpsim/ioports.h> for more documentation on port behavior.
    //  But in summary, when an I/O port updates its I/O pins, it will
    //  query the pin drive direction via the SignalControl object. )

    SignalControl *pPortDirectionControl = new LCDSignalControl(this);
    for (int i=0; i<8; i++)
      (*m_dataBus)[i].setControl(pPortDirectionControl);


#if IN_BREADBOARD==1
    // Place pins along the left side of the package
    for (int i=1; i<=30; i++)
      package->setPinGeometry(i, 0.0, i*12.0, 0, true);
#endif

  }

  bool PK27_Series::dataBusDirection()
  {
    return  m_pSSD0323->dataBusDirection();
  }

} // end of namespace OSRAM

#endif // HAVE_GUI
