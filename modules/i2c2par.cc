/*
   Copyright (C) 2015 Roy R Rankin

This file is part of gpsim.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
    This module simulates a 7 bit address slave I2C device with an 8 bit 
    I/O bus. The direction of the 8 bit bus is controlled by the R/W bit
    sent from the master. 

    This module allows multi-pin devices such as LCD displays to be
    connected to the processor via the 2 pin I2C bus.
*/

#include <cstdio>

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include "config.h"    // get the definition for HAVE_GUI
#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "src/gpsim_time.h"
#include "src/stimuli.h"
#include "src/ioports.h"
#include "src/symbol.h"
#include "src/value.h"
#include "src/packages.h"
#include "src/gpsim_interface.h"
#include "i2c2par.h"


class AddAttribute : public Integer
{
public:
    I2C2PAR_Modules::i2c2par *i2cpt;

    AddAttribute(I2C2PAR_Modules::i2c2par *_i2cpt) : 
	Integer("Slave_Address", 0x27, "I2C Slave Address"), i2cpt(_i2cpt)
    {

    }
    virtual void set(int v)
    {
	Integer::set(v);
	if (i2cpt)
	    i2cpt->i2c_slave_address = (v<<1);
    }
};

class IOPort : public PortModule
//class IOPort : public PortRegister
{
public:
    unsigned int direction;

//    virtual void put(unsigned int new_value);
    IOPort (unsigned int _num_iopins=8);
    void update_pin_directions(unsigned int );
    void put(unsigned int);
    unsigned int get();
};

//IOPort::IOPort(unsigned int _num_iopins) : PortRegister(_num_iopins, "P", "")
IOPort::IOPort(unsigned int _num_iopins) : PortModule(_num_iopins), direction(0)
{}

void IOPort::put(unsigned int value)
{
    IOPIN *m_pin;

    for(int i = 0; i<8; i++)
    {
	unsigned int bit = 1<<i;
	if ((m_pin = getPin(i)))
	    m_pin->putState((value & bit) == bit);
    }
}
unsigned int IOPort::get()
{
    IOPIN *m_pin;
    unsigned int value = 0;

    for(int i = 0; i<8; i++)
    {
	unsigned int bit = 1<<i;
	if ((m_pin = getPin(i)))
	{
	    if (m_pin->getState())
		value |= bit;
	}
    }
    return(value);
}

void IOPort::update_pin_directions(unsigned int new_direction)
{
    IOPIN *m_pin;

    if((new_direction ^ direction) & 1) 
    {
        direction = new_direction & 1;

       for(int i=0; i<8; i++) 
       {
	   if ((m_pin = getPin(i)))
	   {
		m_pin->update_direction(direction,true);
		if (m_pin->snode)
		    m_pin->snode->update();
           }
        }
    }
}


namespace I2C2PAR_Modules {

  i2c2par::i2c2par(const char *_name) : i2c_slave(), Module(_name, "i2c2par")
  {
    io_port = new IOPort(8);
    Addattr = new AddAttribute(this);
    addSymbol(Addattr);
    Addattr->set(0x27);
  }
  i2c2par::~i2c2par() 
  {
	delete io_port;
	delete Addattr;
  }



  void i2c2par::put_data(unsigned int data)
  {
       io_port->put(data);
  }

  unsigned int i2c2par::get_data()
  {
	return(io_port->get());
  }

  void i2c2par::slave_transmit(bool yes)
  {
      if (yes)
          io_port->update_pin_directions(IOPIN::DIR_INPUT);
      else
          io_port->update_pin_directions(IOPIN::DIR_OUTPUT);
  }

  bool i2c2par::match_address()
  {
	return((xfr_data & 0xfe) == i2c_slave_address);
  }


  Module *i2c2par::construct(const char *_new_name)
  {
     string att_name = _new_name;

    i2c2par *pEE = new i2c2par(_new_name);
    pEE->create_iopin_map();
   return(pEE);

  }


  void i2c2par::create_iopin_map()
  {
        string pinName;


	pinName = name() + ".SDA";
	((IOPIN *)(sda))->new_name(pinName.c_str());
	pinName = name() + ".SCL";
	((IOPIN *)(scl))->new_name(pinName.c_str());

	package = new Package(10);
        pinName = name() + ".p0";
	package->assign_pin( 1, io_port->addPin(
		new IO_bi_directional_pu(pinName.c_str()),0));
        pinName = name() + ".p1";
	package->assign_pin( 2, io_port->addPin(
		new IO_bi_directional_pu(pinName.c_str()),1));
        pinName = name() + ".p2";
	package->assign_pin( 3, io_port->addPin(
		new IO_bi_directional_pu(pinName.c_str()),2));
        pinName = name() + ".p3";
	package->assign_pin( 4, io_port->addPin(
		new IO_bi_directional_pu(pinName.c_str()),3));
        pinName = name() + ".p4";
	package->assign_pin( 7, io_port->addPin(
		new IO_bi_directional_pu(pinName.c_str()),4));
        pinName = name() + ".p5";
	package->assign_pin( 8, io_port->addPin(
		new IO_bi_directional_pu(pinName.c_str()),5));
        pinName = name() + ".p6";
	package->assign_pin( 9, io_port->addPin(
		new IO_bi_directional_pu(pinName.c_str()),6));
        pinName = name() + ".p7";
	package->assign_pin( 10, io_port->addPin(
		new IO_bi_directional_pu(pinName.c_str()),7));
	package->assign_pin( 5, (IOPIN *)(sda));
	package->assign_pin( 6, (IOPIN *)(scl));

  }

} // end of namespace I2C2PAR_Modules
