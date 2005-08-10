/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef __IOPORTS_H__
#define __IOPORTS_H__

#include "gpsim_classes.h"
#include "registers.h"
#include "breakpoints.h"
#include "stimuli.h"
#include <vector>

class IOPORT_TRIS;
class IOPORT_LATCH;
class USART_MODULE;
class SSP_MODULE;
class IOPIN;
class stimulus;
class Stimulus_Node;
class TMRL;


///**********************************************************************/
/// PortPinModule
///
/// Here's a general description of gpsim I/O pin design:
///
///    Data
///    Select  ======+
///                +-|-+  Outgoing
///    Source1 ===>| M |  data
///    Source2 ===>| U |=============+    
///    SourceN ===>| X |             |    
///                +---+             |    +-------+
///    Control                       +===>| IOPIN |
///    Select  ======+                    |       |
///                +-|-+  I/O Pin         |       |
///   Control1 ===>| M |  Direction       |       |<======> Physical
///   Control2 ===>| U |=================>|       |         Interface
///   ControlN ===>| X |                  |       |
///                +---+                  |       |
///    Sink Decode                   +===<|       |
///    Select  ======+               |    +-------+
///                +-|-+   Incoming  |
///    Sink1  <====| D |   data      |
///    Sink2  <====| E |<============|
///    SinkN  <====| C |
///                +---+
///
/// The PortPinModule models a Processor's I/O Pin. The schematic illustrates
/// the abstract description of the PortPinModule. Its job is to merge together
/// all of the Processor's peripherals that can control a processor's pin.
/// For example, a UART peripheral may be shared with a general purpose I/O
/// pin. The UART may have a transmit and receive pin and select whether it's
/// in control of the I/O pins. The uart transmit pin and the port's I/O pin
/// can both act as a source for the physical interface. The PortPinModule
/// arbitrates between the two. Similarly, the UART receive pin can be multiplexed
/// with a register pin. In this case, the PortPinModule will route signal
/// changes to both devices. Finally, a peripheral like the '622's comparators
/// may overide the output control. The PortPinModule again arbitrates.
///

/// SignalControl  - A pure virtual class that defines the interface for 
/// a signal control. The I/O Pin Modules will query the source's state
/// via SignalControl. The control is usually used to control the I/O pin 
/// direction (i.e. whether it's an input or output...), drive value, 
/// pullup state, etc.

class SignalControl
{
public:
  virtual char getState()=0;
};

/// SignalSink - A pure virtual class that allows signals driven by external
/// stimuli be route to one or more objects monitoring them (e.g. one
/// sink may be the register associtated with the port while another
/// may be a peripheral)

class SignalSink
{
public:
  virtual void setSinkState(char)=0;
};
;
class PinModule;


/// PortModule - Manages all of the I/O pins associated with a single
/// port. The PortModule supplies the interface to the I/O pin's.

class PortModule
{
public:

  PortModule(int numIopins);
  ~PortModule();

  void updatePort();
  void updatePin(int iPinNumber);

  SignalControl *addSource(SignalControl *, unsigned int iPinNumber);
  SignalControl *addControl(SignalControl *, unsigned int iPinNumber);
  SignalControl *addPullupControl(SignalControl *, unsigned int iPinNumber);
  SignalSink    *addSink(SignalSink *, unsigned int iPinNumber);
  IOPIN         *addPin(IOPIN *, unsigned int iPinNumber);
  void           addPinModule(PinModule *, unsigned int iPinNumber);

  PinModule &operator [] (unsigned int pin_number);

protected:
  unsigned int mNumIopins;
private:
  PinModule  **iopins;
};


/// PinModule - manages the interface to an I/O pin. The parent class
/// 'PinMonitor', allows the PinModule to be registered with the I/O
/// pin. In other words, when the I/O pin changes state, the PinModule
/// will be notified.

class PinModule : public PinMonitor
{
public:
  PinModule();
  PinModule(PortModule *, unsigned int _pinNumber, IOPIN *new_pin=0);
  virtual ~PinModule() {}
  void updatePinModule();

  void setPin(IOPIN *);
  void setDefaultSource(SignalControl *);
  void setSource(SignalControl *);
  void setDefaultControl(SignalControl *);
  void setControl(SignalControl *);
  void setPullupControl(SignalControl *);
  void setDefaultPullupControl(SignalControl *);
  void addSink(SignalSink *);

  char getControlState();
  char getSourceState();
  char getPullupControlState();

  IOPIN &getPin() { return *m_pin;}
  virtual void setDrivenState(char);
  virtual void setDrivingState(char);
  virtual void set_nodeVoltage(double);
  virtual void putState(char);
  virtual void setDirection();

protected:
  /// The SignalSink list is a list of all sinks that can receive data
  list <SignalSink *> sinks;

private:
  char          m_cLastControlState;
  char          m_cLastSinkState;
  char          m_cLastSourceState;
  char          m_cLastPullupControlState;

  SignalControl *m_defaultSource,  *m_activeSource;
  SignalControl *m_defaultControl, *m_activeControl;
  SignalControl *m_defaultPullupControl, *m_activePullupControl;

  IOPIN        *m_pin;
  PortModule   *m_port;
  unsigned int  m_pinNumber;
};

class PortRegister : public sfr_register, public PortModule
{
public:
  PortRegister(unsigned int numIopins, unsigned int enableMask);

  virtual void put(unsigned int new_value);
  virtual unsigned int get();
  virtual unsigned int get_value();
  virtual void putDrive(unsigned int new_drivingValue);
  virtual unsigned int getDriving();
  virtual void setbit(unsigned int bit_number, char new_value);
  virtual void setEnableMask(unsigned int nEnableMask)
  {
    mEnableMask = nEnableMask;
  }
  unsigned int getEnableMask()
  {
    return mEnableMask;
  }
protected:
  unsigned int  mEnableMask;
  unsigned int  drivingValue;
  RegisterValue rvDrivenValue;
};

class PicTrisRegister;
class PicPortRegister : public PortRegister
{
public:
  PicPortRegister(const char *port_name, unsigned int numIopins, unsigned int enableMask);
  void setTris(PicTrisRegister *new_tris);
  virtual void setEnableMask(unsigned int nEnableMask);
  Register *getTris();
protected:
  PicTrisRegister *m_tris;
};

class PicTrisRegister : public sfr_register
{

public:

  PicTrisRegister(const char *tris_name, PicPortRegister *);
  virtual void put(unsigned int new_value);
  virtual unsigned int get();
  
protected:
  PicPortRegister *m_port;
};

class PicPortBRegister : public PicPortRegister
{
public:
  PicPortBRegister(const char *port_name, unsigned int numIopins, unsigned int enableMask);

  virtual void put(unsigned int new_value);
  virtual unsigned int get(unsigned int new_value);
  virtual void setbit(unsigned int bit_number, char new_value);
  void setRBPU(bool);
  void setIntEdge(bool);
private:
  enum {
    eIntEdge = 1<<6,
    eRBPU    = 1<<7
  };
  bool m_bRBPU;
  bool m_bIntEdge;

};




/************************************************************************/
/*                                                                      */
/*       E X P E R I M E N T A L    C O D E   E N D                     */
/*                                                                      */
/*                                                                      */
/************************************************************************/









//---------------------------------------------------------
// IOPORT - Base class for all I/O ports
//
//  Register
//    |-> sfr_register
//            |--> IOPORT
//                   |--> PORTA
//                   |--> PORTB
//                   |--> PORTC
//                   |--> PORTD
//                   |--> PORTE
//

/// IOPORT - Base class for I/O ports

class IOPORT : public sfr_register
{
public:

  /// The I/O pins associated with this I/O port.
  /// This could be anything (or nothing.)
  IOPIN  **pins;



  unsigned int 
    valid_iopins,   // A mask that for those ports that don't have all 8 io bits.
    stimulus_mask,  // A mask indicating which io bits have a stimulus.
    internal_latch, // 
    num_iopins;     // Number of I/O pins attached to this port

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);

  // setbit() is called when a stimulus writes a value to one
  // of the I/O pins in this Port.
  virtual void setbit(unsigned int bit_number, bool new_value);

  virtual bool get_bit(unsigned int bit_number);
  virtual double get_bit_voltage(unsigned int bit_number);

  virtual unsigned int get(void);
  virtual unsigned int get_value(void);

  IOPIN *getIO(unsigned int pin_number);

  /// Stimuli 
  void attach_stimulus(stimulus *new_stim, unsigned int bit_position);
  virtual int update_stimuli(void);
  void attach_iopin(IOPIN * new_pin, unsigned int bit_position);
  void attach_node(Stimulus_Node *new_node, unsigned int bit_position);

  virtual void trace_register_write(void);
  virtual void change_pin_direction(unsigned int bit_number, bool new_direction);

  IOPORT(unsigned int _num_iopins=8);
  ~IOPORT();

};

#if defined(OLD_IOPORT_DESIGN)

//---------------------------------------------------------
// IOPORT

class PIC_IOPORT : public IOPORT
{
public:
  IOPORT_TRIS * tris;
  IOPORT_LATCH * latch;   // non-null on 18x parts only

  virtual void put(unsigned int new_value);
  void update_pin_directions(unsigned int new_tris);
  void change_pin_direction(unsigned int bit_number, bool new_direction);

  virtual void check_peripherals(RegisterValue rv) {}
  virtual unsigned int get(void);
  virtual bool get_bit(unsigned int bit_number);
  virtual void setbit(unsigned int bit_number, bool new_value);
  PIC_IOPORT(unsigned int _num_iopins=8);
private:
  unsigned int latch_data_out;
  unsigned int peripheral_data_out;
  unsigned int data_out_select;

  unsigned int latch_tris_out;
  unsigned int peripheral_tris_out;
  unsigned int tris_out_select;

  unsigned int data_in;
  unsigned int peripheral_data_in;
};

class IOPORT_TRIS : public sfr_register
{
public:
  PIC_IOPORT * port;

  unsigned int 
    valid_iopins;   // A mask that for those ports that don't have all 8 io bits.

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  virtual void setbit(unsigned int bit_number, bool new_value);
  unsigned int get(void);
  IOPORT_TRIS(void);

};

class IOPORT_LATCH : public sfr_register  // latch register for 18x cores
{
public:
  PIC_IOPORT * port;

  unsigned int 
    valid_iopins;   // A mask that for those ports that don't have all 8 io bits.

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  virtual unsigned int get(void);
  virtual void setbit(unsigned int bit_number, bool new_value);
  IOPORT_LATCH(void);

};

class PORTB : public PIC_IOPORT
{
public:
#define intedg_MASK (1<<6)
#define rbpu_MASK   (1<<7)

  unsigned int rbpu;
  unsigned int intedg;

  PORTB(void);

  void rbpu_intedg_update(unsigned int);
  virtual void put(unsigned int new_value);
  virtual unsigned int get(void);
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual void check_peripherals(RegisterValue rv);

  virtual void reset(RESET_TYPE r);

};

// PORTB on the 62x devices is totally different than PORTB on
// other PICs.
class CCPCON;
class PORTB_62x : public PORTB
{
 public:
  enum
    {

      RX    = 1 << 1,
      DT    = 1 << 1,
      TX    = 1 << 2,
      CK    = 1 << 2,
      CCP1  = 1 << 3,
      T1CKI = 1 << 6,
      T1OSO = 1 << 6,
      T1OSI = 1 << 7,
      //SCK   = 1 << 3,
      //SCL   = 1 << 3,  /* SCL and SCK share the same pin */
      //SDI   = 1 << 4,
      //SDA   = 1 << 4,  /* SDA and SDI share the same pin */
      //SDO   = 1 << 5
    };

  CCPCON *ccp1con;
  USART_MODULE *usart;

  PORTB_62x(void);
  unsigned int get(void);
  void setbit(unsigned int bit_number, bool new_value);
  virtual void check_peripherals(RegisterValue rv);
};

class PORTA : public PIC_IOPORT
{
public:

  PORTA(void);

  enum {
	// there should probably be other things listed here, but I don't
	// really know what and I figure better no data than wrong data.
	SS = 1 << 5,
  };

  void setbit(unsigned int bit_number, bool new_value);
  virtual unsigned int get(void);
  virtual void check_peripherals(RegisterValue rv);

  SSP_MODULE *ssp;

};

class COMPARATOR_MODULE;
class PORTA_62x : public PIC_IOPORT
{
 public:

  // The 62x PORT A can have a different state on the I/O pins
  // then what is obtained via a "MOVF  PORTA,W" reading of the
  // I/O Port. 'pin_value' reflects the digital states on the I/O
  // pins. The normal 'value' reflects the state when PORTA
  // is read.

  unsigned int pin_value;

  // auxillary functions of the port bits:
  enum
    {
      AN0 = 1 << 0,
      AN1 = 1 << 1,
      AN2 = 1 << 2,
      AN3 = 1 << 3,
      VREF = 1 << 2,
      CMP1 = 1 << 3,
      CMP2 = 1 << 4,
      TOCKI = 1 << 4,
      THV = 1 << 5,
      SS = 1 << 5,

    };

  COMPARATOR_MODULE *comparator;
  SSP_MODULE *ssp;

  PORTA_62x(void);

  void setbit(unsigned int bit_number, bool new_value);
  virtual unsigned int get(void);
  virtual void put(unsigned int new_value);
  virtual void check_peripherals(RegisterValue rv);

};

class PORTC : public PIC_IOPORT
{
 public:

  /* Define the I/O pins that are common among the various PIC's
   * endowed with a port C.
   */

  enum
    {
      T1CKI = 1 << 0,
      CCP1  = 1 << 2,
      SCK   = 1 << 3,
      SCL   = 1 << 3,  /* SCL and SCK share the same pin */
      SDI   = 1 << 4,
      SDA   = 1 << 4,  /* SDA and SDI share the same pin */
      SDO   = 1 << 5
    };

  /* Now define the I/O pins that are device dependent */
  enum
    {
      T1OSO = 1 << 0,
      T1OSI = 1 << 1,
      _T1OSO = 1 << 1,  /* For some bizarre reason, TMR1's OS interface */
      _T1OSI = 1 << 0,  /* varies from pic to pic... */
      CCP2  = 1 << 1,
      TX    = 1 << 6,  /* Not all pics with a port C have a usart, */
      CK    = 1 << 6,  /* but the ones that do share TX and CK */
      RX    = 1 << 7,  /* Same goes for RX and DT */
      DT    = 1 << 7
    };

  /* A flag to sort out how the TMR1 OS interface is implemented.
   *   set: T1OSO == RC0 && T1OSI == RC1
   * clear: T1OSO == RC1 && T1OSI == RC0
   */
  bool t1oso_order;

  CCPCON *ccp1con;
  USART_MODULE *usart;
  SSP_MODULE *ssp;
  TMRL *tmrl;

  PORTC(void);
  virtual unsigned int get(void);
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual void check_peripherals(RegisterValue rv);
};

#endif // OLD_IOPORT_DESIGN

#endif  // __IOPORTS_H__
