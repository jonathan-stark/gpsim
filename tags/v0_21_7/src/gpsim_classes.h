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

/*
 * gpsim_classes.h
 *
 * This include file contains most of the class names defined in gpsim
 * It's used to define forward references to classes and help alleviate
 * include file dependencies.
 */

#ifndef __GPSIM_CLASSES_H__
#define __GPSIM_CLASSES_H__

/*==================================================================
 *
 * Here are a few enum definitions 
 */


/*
 * Define all of the different types of reset conditions:
 */

enum RESET_TYPE
{
  POR_RESET,          // Power-on reset
  WDT_RESET,          // Watch Dog timer timeout reset
  EXTERNAL_RESET,     // I/O pin (e.g. MCLR going low) reset
  SOFT_RESET,         // Software initiated reset
  BOD_RESET,          // Brown out detection reset
  SIM_RESET,          // Simulation Reset
  
};



enum PROCESSOR_STATES
{

  POR_,
  IDLE

};

#endif //  __GPSIM_CLASSES_H__
