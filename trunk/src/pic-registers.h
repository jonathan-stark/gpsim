
/*
   Copyright (C) 1998-2000 T. Scott Dattalo

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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __REGISTERS_H__
#define __REGISTERS_H__

class pic_processor;
class symbol;
class XrefObject;

//---------------------------------------------------------
// Base class for a file register.
class file_register
{
public:

  enum REGISTER_TYPES
  {
    INVALID_REGISTER,
    FILE_REGISTER,
    SFR_REGISTER,
    BP_REGISTER
  };

  char *name_str1;
  unsigned int value,address,break_point;

  // If non-zero, the alias_mask describes all address at which
  // this file register appears. The assumption (that is true so
  // far for all pic architectures) is that the aliased register
  // locations differ by one bit. For example, the status register
  // appears at addresses 0x03 and 0x83 in the 14-bit core. 
  // Consequently, alias_mask = 0x80 and address (above) is equal
  // to 0x03.

  unsigned int alias_mask;

  unsigned int por_value;  // power on reset value
  symbol *symbol_alias;
  pic_processor *cpu;

  //  #ifdef HAVE_GUI
  // If we are linking with a gui, then here are a
  // few declarations that are used to send data to it.
  // This is essentially a singly-linked list of pointers
  // to structures. The structures provide information
  // such as where the data is located, the type of window
  // it's in, and also the way in which the data is presented
  
  XrefObject *xref;

//    GSList *gui_xref;
//    void assign_xref(gpointer);
  //  #endif

  file_register(void);
  ~file_register(void);
  virtual void put(unsigned int new_value);
    /*{
    value = new_value;
    trace.register_write(address,value);
    }*/
  virtual unsigned int get(void);
    /*  {
      trace.register_read(address,value);
      return(value);
      }*/

  /* same as put(), but some extra stuff like interfacing
   * to the gui is done. (It's more efficient than burdening
   * the run time performance with (unnecessary) gui crap.)
   */

  virtual void put_value(unsigned int new_value);

  /* same as get(), but no trace is performed */
  virtual unsigned int get_value(void)
    {
      return(value);
    }


  virtual char *name(void) { return(name_str1);};
  virtual void new_name(char *);
  virtual REGISTER_TYPES isa(void) {return FILE_REGISTER;};
};

//---------------------------------------------------------
// Base class for a special function register.
class sfr_register : public file_register
{
public:
    sfr_register() :file_register(){}
  unsigned int wdtr_value; // wdt or mclr reset value

  virtual REGISTER_TYPES isa(void) {return SFR_REGISTER;};
  virtual void initialize(void) {return;};
};


class SFR_map                   // Special Function Register (e.g. status)
{
public:
  int address;			// Base address of the register

  sfr_register *sfr_reg;        // a pointer to it

  int por_value;
};





#endif
