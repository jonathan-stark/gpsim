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

#ifndef __CMD_STIMULUS_H__
#define __CMD_STIMULUS_H__
#include "../src/stimulus_orb.h"
#include "expr.h"

class cmd_stimulus : public command
{
public:
  int valid_options,options_entered;
  int have_data;
  /*
  unsigned int period,
    phase,
    high_time,
    start_cycle,
    bit_pos,
    states,
    initial_state,
    time_flag,
    digital;

  char *stim_name;

  vector<StimulusDataType>  temp_array;
  */
  cmd_stimulus(void);
  void stimulus(void);

  void stimulus(int bit_flag);
  void stimulus(cmd_options_expr *coe);
  void stimulus(cmd_options_str *cos);
  void stimulus(ExprList_t *);
  //void data_point(guint64 new_int_data_point);
  //void data_point(float new_float_data_point);
  void end(void);

  bool can_span_lines(void) {return 1;};
};

extern cmd_stimulus c_stimulus;
#endif

