/*
   Copyright (C) 1999 T. Scott Dattalo

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


#ifndef __STIMULUS_ORB_H__
#define __STIMULUS_ORB_H__

//------------------------------------
// *** KNOWN CHANGE ***
//
//  THIS FILE IS (OR SHOULD BE) TEMPORARY
//
// Here are a set of support functions called by the CLI. Eventually
// these will be replaced with an indirect interface such as CORBA.

#define NEW_SQW    1         // Hardcode the valid stimulus types.
#define NEW_ASY    2
#define NEW_TRI    3

enum STIMULUS_DATA_POINT_TYPES {
   STIMULUS_DPT_INT,
   STIMULUS_DPT_FLOAT
};

typedef struct StimulusData{
  enum STIMULUS_DATA_POINT_TYPES data_type;

  union {
     guint64 i;
     float   f;
  } data_point;

} StimulusDataType;


class pic_processor;
struct char_list;

extern void create_stimulus(int type, char *name);
extern void dump_stimulus_list(void);
extern void stimorb_period(unsigned int _period);
extern void stimorb_duty(unsigned int _duty);
extern void stimorb_phase(unsigned int _phase);
extern void stimorb_initial_state(unsigned int _initial_state);
extern void stimorb_start_cycle(unsigned int _start_cycle);
extern void stimorb_asy(int digital, pic_processor *cpu,vector<StimulusDataType> temp_array );
extern void stimorb_attach(char *node, char_list *stimuli);
extern void stimorb_name(char *_name);

#endif  // __STIMULUS_ORB_H__
