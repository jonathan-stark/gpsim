/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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
  gpsim_modules.cc

  gpsim supports modules, or thingies, that are not part of gpsim proper.
This is to say, that the modules are not compiled and linked with the
core gpsim software. Instead, they are compiled and linked separately and
then dynamically loaded by gpsim. This approach provides a flexibility
to the user to create customized objects for simulation purposes. The
big benefit of course, is that the user doesn't have to get bogged down
in to the nitty-gritty details of the way gpsim is designed. The templates
provided here can serve as a relatively simple example of how one may
go about creating customized modules.

Please see the README.MODULES for more details on how modules are intended
to be used.

Here are a list of functions that a gpsim compliant module library should
support:

  void mod_list(void) - Prints a list of the modules in a library
  Module * getmodule(char *module_type) - creates a new module
 */

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <iostream>
#include <stdio.h>

#include "../config.h"    // get the definition for HAVE_GUI

#include "../src/modules.h"

#include "resistor.h"
#include "usart.h"
#ifndef _WIN32
//#include "paraface.h"
#endif
#include "switch.h"
#include "logic.h"
#ifdef HAVE_GUI
#include "led.h"
#include "push_button.h"
//#include "video.h"
#include "encoder.h"
#endif
#include "stimuli.h"
#include "ttl.h"

/*
class Module_Types
{
public:

  char *names[2];
  Module * (*module_constructor) (void);
};
*/
Module_Types available_modules[] =
{

  //#ifdef HAVE_GUI
#ifndef _WIN32
  // Parallel port interface
  /*
    TSD - removed 16APR06 - The parallel port interface uses the deprecated
    IOPORT class. 
  { {"parallel_interface",         "paraface"}, Paraface::construct},
  */
#endif

  // Switch
  { {"switch",         "sw"}, Switches::Switch::construct},

  // Logic
  { {"and2", "and2"}, AND2Gate::construct},
  { {"or2",  "or2"},  OR2Gate::construct},
  { {"xor2", "xor2"}, XOR2Gate::construct},
  { {"not",  "not"},  NOTGate::construct},

#ifdef HAVE_GUI
  // Leds
  { {"led_7segments", "led7s"}, Leds::Led_7Segments::construct},
  { {"led", "led"}, Leds::Led::construct},
#endif
  { {"pullup",           "pu"},   PullupResistor::pu_construct },
  { {"pulldown",         "pd"},   PullupResistor::pd_construct },
#ifdef HAVE_GUI
  { {"pushbutton",       "pb"},   PushButton::construct },
#endif
  { {"pulsegen",         "pg"},   ExtendedStimuli::PulseGen::construct },

  /*
    TSD Removed 17APR06
  // Video
  { {"PAL_video", "video"}, Video::construct},
  */
#ifdef HAVE_GUI
  // Encoder
  { {"Encoder", "encoder"}, Encoder::construct},
#endif
  // USART
  { {"usart",            "usart"}, USARTModule::USART_construct},

  // TTL devices
  { {"TTL377", "ttl377"}, TTL::TTL377::construct},
  //#endif

  // No more modules
  { {0,0},0}
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/********************************************************************************
 * mod_list - Display all of the modules in this library.
 *
 * This is a required function for gpsim compliant libraries.
 */

  Module_Types * get_mod_list(void)
  {

    return available_modules;

  }
/********************************************************************************
 * mod_list - Display all of the modules in this library.
 *
 * This is a required function for gpsim compliant libraries.
 */

void mod_list(void)
{

  unsigned int number_of = sizeof(available_modules) / sizeof(Module_Types);
  unsigned int i,j,l;
  unsigned int k,longest;

  for(i=0,longest=0; i<number_of; i++)
    {
      k = (unsigned int)strlen(available_modules[i].names[1]);
      if(k>longest)
	longest = k;
    }

  k=0;
  do
    {

      for(i=0; (i<4) && (k<number_of); i++)
	{
	  cout << available_modules[k].names[1];
	  if(i<3)
	    {
	      l = longest + 2 - (unsigned int)strlen(available_modules[k].names[1]);
	      for(j=0; j<l; j++)
		cout << ' ';
	    }
	  k++;
	}
      cout << '\n';
    } while (k < number_of);

}


/************************************************************
 *
 * _init() - this is called when the library is opened.
 */

void init(void)
{

  //cout << "gpsim modules has been opened\n";
  printf("%s\n",__FUNCTION__);
}

/************************************************************
 *
 * _fini() - this is called when the library is closed.
 */

void fini(void)
{

  //cout << "gpsim modules has been closed\n";
  printf("%s\n",__FUNCTION__);
}

void test(void)
{
  //cout << "This is a test\n";
  printf("%s\n",__FUNCTION__);

}

#ifdef __cplusplus
}
#endif /* __cplusplus */
