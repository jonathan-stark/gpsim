/*
   Copyright (C) 2015 Roy R Rankin

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


#ifndef __I2C2PAR_H__
#define __I2C2PAR_H__

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "src/modules.h"
#include "src/ioports.h"
#include "src/stimuli.h"
#include "src/trigger.h"
#include "src/i2c-ee.h"

class I2C_SLAVE_SCL;
class I2C_SLAVE_SDA;
class IOPort;
class AddAttribute;
namespace I2C2PAR_Modules {

  class i2c2par : public i2c_slave, public Module, public TriggerObject
  {

  public:
    i2c2par(const char *_name);
    ~i2c2par();
    static Module *construct_i2c2par(const char *new_name);
    virtual void create_iopin_map();
    virtual void setEnable(bool bNewState, unsigned int m_bit){};
    virtual bool match_address();
    virtual void put_data(unsigned int data);
    virtual unsigned int get_data();
    virtual void slave_transmit(bool yes);

    IOPort *io_port;

  protected:

    AddAttribute *Addattr;
  };

  

} // end of namespace I2C2PAR_Modules

#endif // __I2C2PAR_H__
