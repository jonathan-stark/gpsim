/*
   Copyright (C) 1998 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

#ifndef __IOPORTS_H__
#define __IOPORTS_H__

#include "registers.h"
#include "stimuli.h"

class stimulus;
class Stimulus_Node;
class PinModule;


///**********************************************************************/
///
/// I/O ports
///
/// An I/O port is collection of I/O pins. For a PIC processor, these 
/// are the PORTA, PORTB, etc, registers. gpsim models I/O ports in
/// a similar way it models other registers; there's a base class from
/// which all specific I/O ports are derived. However, I/O ports are 
/// special in that they're an interface between a processor's core
/// and the outside world. The requirements vary wildly from one processor
/// to the next; in fact they even vary dynamically within one processor.
/// gpsim attempts to abstract this interface with a set of base classes
/// that are responsible for routing signal state information. These
/// base classes make no attempt to decipher this information, instead
/// this job is left to the peripherals and stimuli connected to the
/// I/O pins and ports.
///
///
/// PinModule
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
/// The PinModule models a Processor's I/O Pin. The schematic illustrates
/// the abstract description of the PinModule. Its job is to merge together
/// all of the Processor's peripherals that can control a processor's pin.
/// For example, a UART peripheral may be shared with a general purpose I/O
/// pin. The UART may have a transmit and receive pin and select whether it's
/// in control of the I/O pins. The uart transmit pin and the port's I/O pin
/// can both act as a source for the physical interface. The PinModule
/// arbitrates between the two. Similarly, the UART receive pin can be multiplexed
/// with a register pin. In this case, the PinModule will route signal
/// changes to both devices. Finally, a peripheral like the '622's comparators
/// may overide the output control. The PinModule again arbitrates.
///
///
/// PortModule
///
/// The PortModule is the base class for processor I/O ports. It's essentially
/// a register that contains an array of PinModule's.
///
///  Register               PortModule
///    |-> sfr_register         |
///             |               |
///             \------+--------/
///                    |
///                    +--> PortRegister
///                            |--> PicPortRegister



///------------------------------------------------------------
///
/// SignalControl  - A pure virtual class that defines the interface for 
/// a signal control. The I/O Pin Modules will query the source's state
/// via SignalControl. The control is usually used to control the I/O pin 
/// direction (i.e. whether it's an input or output...), drive value, 
/// pullup state, etc.
/// When a Pin Module is through with the SignalControl, it will call
/// the release() method. This is primarily used to delete the SignalControl
/// objects.

class SignalControl
{
public:
  virtual ~SignalControl();  //// fixme
  virtual char getState()=0;
  virtual void release()=0;
};

///------------------------------------------------------------
/// PeripheralSignalSource - A class to interface I/O pins with
/// peripheral outputs.

class PeripheralSignalSource : public SignalControl
{
public:
  PeripheralSignalSource(PinModule *_pin);
  virtual ~PeripheralSignalSource();
  virtual void release();

  /// getState - called by the PinModule to determine the source state
  virtual char getState();

  /// putState - called by the peripheral to set a new state
  virtual void putState(const char new3State);

  /// toggle - called by the peripheral to toggle the current state.
  virtual void toggle();
private:
  PinModule *m_pin;
  char m_cState;
};

///------------------------------------------------------------
/// PortModule - Manages all of the I/O pins associated with a single
/// port. The PortModule supplies the interface to the I/O pin's. It
/// is designed to handle a group of I/O pins. However, the low level
/// I/O pin processing is handled by PinModule objects contained within
/// the PortModule.

class PortModule
{
public:

  PortModule(unsigned int numIopins);
  virtual ~PortModule();

  /// updatePort -- loop through update all I/O pins

  virtual void updatePort();

  /// updatePin -- Update a single I/O pin

  virtual void updatePin(unsigned int iPinNumber);

  /// updatePins -- Update several I/O pins

  virtual void updatePins(unsigned int iPinBitMask);

  /// updateUI -- convey pin state info to a User Interface (e.g. the gui).

  virtual void   updateUI();

  /// addPinModule -- supply a pin module at a particular bit position.
  ///      Most of the low level I/O pin related processing will be handled
  ///      here. The PortModule per-pin helper methods below essentially
  ///      call methods in the PinModule to do the dirty work.
  ///      Each bit position can have only one PinModule. If multiple 
  ///      modules are added, only the first will be used and the others
  ///      will be ignored.

  void           addPinModule(PinModule *, unsigned int iPinNumber);

  /// addSource -- supply a pin with a source of data. There may

  SignalControl *addSource(SignalControl *, unsigned int iPinNumber);

  /// addControl -- supply a pin with a data direction control

  SignalControl *addControl(SignalControl *, unsigned int iPinNumber);

  /// addPullupControl -- supply a pin with a pullup control

  SignalControl *addPullupControl(SignalControl *, unsigned int iPinNumber);

  /// addSink -- supply a sink to receive info driven on to a pin

  SignalSink    *addSink(SignalSink *, unsigned int iPinNumber);

  /// addPin -- supply an I/O pin. Note, this will create a default pin module
  ///           if one is not already created.

  IOPIN         *addPin(IOPIN *, unsigned int iPinNumber);

  /// getPin -- an I/O pin accessor. This returns the I/O pin at a particular
  ///           bit position.

  IOPIN         *getPin(unsigned int iPinNumber);

  /// operator[] -- PinModule accessor. This returns the pin module at
  ///               a particular bit position.

  PinModule &operator [] (unsigned int pin_number);

  PinModule * getIOpins(unsigned int pin_number);

  // set/get OutputMask which controls bits returned on I/O
  // port register get() call. Used to return 0 for  analog pins
  virtual void setOutputMask (unsigned int OutputMask) 
	{ mOutputMask = OutputMask;}
  virtual unsigned int getOutputMask () 
	{ return(mOutputMask);}
protected:
  unsigned int mNumIopins;
  unsigned int  mOutputMask;

private:

  /// PinModule -- The array of PinModules that are handled by PortModule.

  PinModule  **iopins;
};

///------------------------------------------------------------
/// PinModule - manages the interface to a physical I/O pin. Both
/// simple and complex I/O pins are handled. An example of a simple
/// I/O is one where there is a single data source, data sink and
/// control, like say the GPIO pin on a small PIC. A complex pin
/// is one that is multiplexed with peripherals.
/// 
/// The parent class 'PinMonitor', allows the PinModule to be 
/// registered with the I/O pin. In other words, when the I/O pin 
/// changes state, the PinModule will be notified.
#define ANALOG_TABLE_SIZE 3

class PinModule : public PinMonitor
{
public:
  PinModule();
  PinModule(PortModule *, unsigned int _pinNumber, IOPIN *new_pin=0);
  virtual ~PinModule();

  /// updatePinModule -- The low level I/O pin state is resolved here 
  /// by examining the direction and state of the I/O pin. 

  void updatePinModule();

  /// refreshPinOnUpdate - modal behavior. If set to true, then
  /// a pin's state will always be refreshed whenever the PinModule
  /// is updated. If false, then the pin is updated only if there
  /// is a detected state change.
  void refreshPinOnUpdate(bool bForcedUpdate);

  void setPin(IOPIN *);
  void clrPin() { m_pin = NULL; } 
  void setDefaultSource(SignalControl *);
  void setSource(SignalControl *);
  void setDefaultControl(SignalControl *);
  void setControl(SignalControl *);
  void setPullupControl(SignalControl *);
  void setDefaultPullupControl(SignalControl *);

  char getControlState();
  char getSourceState();
  char getPullupControlState();
  unsigned int getPinNumber() { return m_pinNumber;}
  void AnalogReq(Register *reg, bool analog, const char *newName);

  IOPIN &getPin() { return *m_pin;}

  /// 
  virtual void setDrivenState(char);
  virtual void setDrivingState(char);
  virtual void set_nodeVoltage(double);
  virtual void putState(char);
  virtual void setDirection();
  virtual void updateUI();


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
  bool          m_bForcedUpdate;
  Register     *m_analog_reg[ANALOG_TABLE_SIZE];	
  bool		m_analog_active[ANALOG_TABLE_SIZE];
};



///------------------------------------------------------------
class PortRegister : public sfr_register, public PortModule
{
public:
  PortRegister(Module *pCpu, const char *pName, const char *pDesc,
               unsigned int numIopins, unsigned int enableMask);

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get();
  virtual unsigned int get_value();
  virtual void putDrive(unsigned int new_drivingValue);
  virtual unsigned int getDriving();
  virtual void setbit(unsigned int bit_number, char new_value);
  virtual void setEnableMask(unsigned int nEnableMask);

  unsigned int getEnableMask()
  {
    return mEnableMask;
  }
  virtual void   updateUI();

protected:
  unsigned int  mEnableMask;
  unsigned int  drivingValue;
  RegisterValue rvDrivenValue;
};

class PortSink : public SignalSink
{
public:
  PortSink(PortRegister *portReg, unsigned int iobit);

  virtual ~PortSink()
  {
  }

  virtual void setSinkState(char);
  virtual void release();
private:
  PortRegister *m_PortRegister;
  unsigned int  m_iobit;
};



/// IOPORT - Base class for I/O ports - deprecated
#if defined(OLD_IOPORT_DESIGN)

class IOPORT : public sfr_register
{
public:

  IOPORT(unsigned int _num_iopins=8);
  ~IOPORT();

  IOPIN *addPin(IOPIN *, unsigned int iPinNumber);
  IOPIN *getIO(unsigned int pin_number);

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);

  // setbit() is called when a stimulus writes a value to one
  // of the I/O pins in this Port.
  virtual void setbit(unsigned int bit_number, bool new_value);

  virtual bool get_bit(unsigned int bit_number);

  virtual unsigned int get(void);
  virtual unsigned int get_value(void);


  virtual void trace_register_write(void);

protected:

  /// The I/O pins associated with this I/O port.
  /// This could be anything (or nothing.)
  IOPIN  **pins;



  unsigned int 
    valid_iopins,   // A mask that for those ports that don't have all 8 io bits.
    stimulus_mask,  // A mask indicating which io bits have a stimulus.
    internal_latch, // 
    num_iopins;     // Number of I/O pins attached to this port




  // Deprecated functions of the IOPORT class 

  /// Stimuli 
  void attach_stimulus(stimulus *new_stim, unsigned int bit_position);
  virtual int update_stimuli(void);
  void attach_iopin(IOPIN * new_pin, unsigned int bit_position);
  void attach_node(Stimulus_Node *new_node, unsigned int bit_position);
  virtual double get_bit_voltage(unsigned int bit_number);
  virtual void change_pin_direction(unsigned int bit_number, bool new_direction);

};

#endif // OLD_IOPORT_DESIGN

#endif  // __IOPORTS_H__
