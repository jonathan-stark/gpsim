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


#include <stdio.h>

#include "../config.h"    // get the definition for HAVE_GUI
#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "../src/i2c-ee.h"
#include "i2c-eeprom.h"

namespace I2C_EEPROM_Modules {


  I2C_EE_Module::I2C_EE_Module(const char *_name)
    : Module(_name, "\
I2C EEProm\n\
") 
  {
    m_eeprom = new I2C_EE(256);
  }


  I2C_EE_Module::~I2C_EE_Module()
  {
    delete m_eeprom;
  }

  Module *I2C_EE_Module::construct(const char *_new_name)
  {

    I2C_EE_Module *pEE = new I2C_EE_Module(_new_name);
    pEE->create_iopin_map();

    //if(get_interface().bUsingGUI()) 
    //  pEE->create_widget(pEe);

  }


  void I2C_EE_Module::create_iopin_map()
  {

  }

} // end of namespace I2C_EEEPROM_Modules
