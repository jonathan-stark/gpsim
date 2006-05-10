/*
   Copyright (C) 2006 T. Scott Dattalo

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


#ifndef __I2C_EEPROM_H__
#define __I2C_EEPROM_H__

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/modules.h"
class I2C_EE;

namespace I2C_EEPROM_Modules {


  class I2C_EE_Module : public Module
  {
  public:
    I2C_EE_Module(const char *_name);
    ~I2C_EE_Module();
    static Module *construct(const char *new_name);

    virtual void create_iopin_map();

  protected:

    I2C_EE *m_eeprom;
  };
  

} // end of namespace I2C_EEEPROM_Modules

#endif // __I2C_EEPROM_H__
