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


//
// p16f87x
//
//  This file supports:
//    P16F874 P16F877


#include <stdio.h>
#include <iostream.h>
#include <string>

#include "symbol.h"

#include "p16f87x.h"
#include "stimuli.h"


void P16F874::create_sfr_map(void)
{

  if(verbose)
    cout << "creating f874 registers \n";

  alias_file_registers(0x80,0x80,0x80);
  alias_file_registers(0x01,0x01,0x100);
  alias_file_registers(0x82,0x84,0x80);
  alias_file_registers(0x06,0x06,0x100);
  alias_file_registers(0x8a,0x8b,0x80);
  alias_file_registers(0x100,0x100,0x80);
  alias_file_registers(0x81,0x81,0x100);
  alias_file_registers(0x102,0x104,0x80);
  alias_file_registers(0x86,0x86,0x100);
  alias_file_registers(0x10a,0x10b,0x80);


  alias_file_registers(0x20,0x7f,0x100);
  alias_file_registers(0xa0,0xff,0x100);

}

void P16F874::create(void)
{

  cout << " f874 create \n";

  P16C65::create();

  status.rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf.base_address_mask = 0x80; // used for indirect accesses above 0x100

  P16F874::create_sfr_map();

}



pic_processor * P16F874::construct(void)
{

  P16F874 *p = new P16F874;

  if(verbose)
    cout << " f874 construct\n";

  p->create();

  p->name_str = "16F874";

  return p;

}

void P16F874::create_symbols(void)
{

  if(verbose)
    cout << "f874 create symbols\n";

}

P16F874::P16F874(void)
{
  if(verbose)
    cout << "f874 constructor, type = " << isa() << '\n';

}



void P16F877::create_sfr_map(void)
{

  if(verbose)
    cout << "creating f877 registers \n";

}

void P16F877::create(void)
{

  cout << " f877 create \n";

  P16F874::create();
  add_file_registers(0x110, 0x16f, 0);
  add_file_registers(0x190, 0x1ef, 0);
  delete_file_registers(0xf0,0xff);
  alias_file_registers(0x70,0x7f,0x80);
  alias_file_registers(0x70,0x7f,0x100);
  alias_file_registers(0x70,0x7f,0x180);

  P16F877::create_sfr_map();

}



pic_processor * P16F877::construct(void)
{

  P16F877 *p = new P16F877;

  if(verbose)
    cout << " f877 construct\n";

  p->create();

  p->name_str = "16F877";

  return p;

}

void P16F877::create_symbols(void)
{

  if(verbose)
    cout << "f877 create symbols\n";

}

P16F877::P16F877(void)
{
  if(verbose)
    cout << "f877 constructor, type = " << isa() << '\n';

}
