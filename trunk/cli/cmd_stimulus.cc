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


#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

#include "command.h"
#include "cmd_stimulus.h"
#include "../src/pic-processor.h"
#include "../src/stimulus_orb.h"
#include "../src/stimuli.h"


static asynchronous_stimulus *last_stimulus=0;
cmd_stimulus c_stimulus;

#define ASYNCHRONOUS_STIMULUS    1
#define SYNCHRONOUS_STIMULUS     2

#define  STIM_NOTHING          0
#define  STIM_PERIOD          (1 << 0)
#define  STIM_PHASE           (1 << 1)
#define  STIM_HIGH_TIME       (1 << 2)
#define  STIM_INITIAL_STATE   (1 << 3)
#define  STIM_START_CYCLE     (1 << 4)
#define  STIM_DATA            (1 << 5)
#define  STIM_IOPORT          (1 << 6)
#define  STIM_ASY             (1 << 7)
#define  STIM_SQW             (1 << 8)
#define  STIM_NAME            (1 << 9)
#define  STIM_TRI             (1 << 10)
#define  STIM_AD              (1 << 11)
#define  STIM_ANALOG          (1 << 12)
#define  STIM_DIGITAL         (1 << 13)
#define  STIM_DUMP            (1 << 14)
#define  STIM_PORT            (1 << 15)

const unsigned int
SQW_OPTIONS = STIM_SQW | STIM_PERIOD | STIM_PHASE | STIM_HIGH_TIME | STIM_START_CYCLE;
const unsigned int
ASY_OPTIONS = STIM_ASY | STIM_PERIOD | STIM_PHASE | STIM_HIGH_TIME | STIM_START_CYCLE | STIM_DATA;
const unsigned int
TRI_OPTIONS = STIM_TRI | STIM_PERIOD | STIM_PHASE | STIM_HIGH_TIME | STIM_START_CYCLE;

static cmd_options cmd_stimulus_options[] =
{
  {"asy",                   STIM_ASY,           OPT_TT_SUBTYPE},
  {"asynchronous_stimulus", STIM_ASY,           OPT_TT_SUBTYPE},
  {"period",                STIM_PERIOD,        OPT_TT_NUMERIC},
  {"phase",                 STIM_PHASE,         OPT_TT_NUMERIC},
  {"high_time",             STIM_HIGH_TIME,     OPT_TT_NUMERIC},
  {"initial_state",         STIM_INITIAL_STATE, OPT_TT_NUMERIC},
  {"start_cycle",           STIM_START_CYCLE,   OPT_TT_NUMERIC},
  {"start",                 STIM_START_CYCLE,   OPT_TT_NUMERIC},
  {"name",                  STIM_NAME,          OPT_TT_STRING},
  {"digital",               STIM_DIGITAL,       OPT_TT_BITFLAG},
  {"analog",                STIM_ANALOG,        OPT_TT_BITFLAG},
  {"d",                     STIM_DUMP,          OPT_TT_BITFLAG},
  {"dump",                  STIM_DUMP,          OPT_TT_BITFLAG},
  {"sqw",                   STIM_SQW,           OPT_TT_SUBTYPE},
  {"square_wave",           STIM_SQW,           OPT_TT_SUBTYPE},
  {"tri",                   STIM_TRI,           OPT_TT_SUBTYPE},
  {"triangle_wave",         STIM_TRI,           OPT_TT_SUBTYPE},
  {"port",                  STIM_PORT,          OPT_TT_STRING},

 { 0,0,0}
};


cmd_stimulus::cmd_stimulus(void)
{ 
  name = "stimulus";

  brief_doc = string("Create a stimulus");

  long_doc = string ("\nstimulus [[type] options]\n"
    "\tstimulus will create a signal that can be tied to an io port.\n"
    "\tNote that in most cases it is easier to create a stimulus\n"
    "\tfile then to type all this junk by hand.\n"
    "\n"
    "\t  Supported stimuli:\n"
    "\n"
    "\tsquare_wave | sqw  [period p] [high_time h] [phase ph] [initial_state i]\n"
    "\t  port port_name bit_pos end\n"
    "\t\t  creates a square wave with a period of \"p\" cpu cycles.\n"
    "\t\t  If the high time is specified then that's the number of cycles\n"
    "\t\t  the square wave will be high.\n"
    "\t\t  The phase is with respect to the cpu's cycle counter.\n"
    "\t\t  The \"port_name\" and \"bit_pos\" describe where the stimulus\n"
    "\t\t  will be attached.\n"
    "\tasynchronous_stimulus | asy  [period p] [phase ph]  [initial_state i]\n"
    "\t  d0 [d1 d2 ... dn] port port_name bit_pos end\n"
    "\t\t  creates an asynchronous square wave with a period of \"p\" cpu\n"
    "\t\t  cycles.  The phase is with respect to the cpu's cycle counter.\n"
    "\t\t  The \"port_name\" and \"bit_pos\" describe where the stimulus\n"
    "\t\t  will be attached.\n"
    "\n"
    "\texamples:\n"
    "\n"
    "\t  stimulus sqw period 200 high_time 20 phase 60 port portb 0 end\n"
    "\t  create a square wave stimulus that repeats every 200 cpu cycles,\n"
    "\t  is high for 20 cpu cycles (and low for 200-20=180 cycles). The\n"
    "\t  first rising edge will occur at cycle\n"
    "\t  60, the second at 260, . . . Bit 0 of portb will receive the stimulus.\n"
    "\n");

  op = cmd_stimulus_options;

  options_entered = 0;

}

void cmd_stimulus::stimulus(void)
{
  dump_stimulus_list();
}

//------------------------------------------------------------------
// stimulus(int bit_flag)
//
// For the bit_flags of SQW, ASY, TRI:
//   A new stimulus is dynamically created and a pointer to it is
// is assigned to 'last_stimulus'. The last_stimulus also acts like
// a flag. If it is non-null then a stimulus is in the process of
// being created. When the stimulus 'end' option is specified at the
// cli, then 'last_stimulus' is set to NULL. Note the memory for 
// used by the last stimulus is created here, but destroyed by the
// stimulus code in ../src/stimuli.cc .
//
void cmd_stimulus::stimulus(int bit_flag)
{

  switch(bit_flag)
    {
    case STIM_SQW:
      if(verbose)
	cout << "creating sqw stimulus\n";
      if(!last_stimulus) {
	//create_stimulus(NEW_SQW,stim_name);
	valid_options = SQW_OPTIONS;
	options_entered = STIM_SQW;
	//last_stimulus = new square_wave;
      } else
	cout << "warning: ignoring sqw stim creation";
      break;

    case STIM_ASY:
      if(verbose)
	cout << "creating asy stimulus\n";

      if(!last_stimulus) {
	//create_stimulus(NEW_ASY,stim_name);
	last_stimulus = new asynchronous_stimulus;
	valid_options = ASY_OPTIONS;
	options_entered = STIM_ASY;
      }else
	cout << "warning: ignoring asy stim creation";
      break;

    case STIM_TRI:
      if(verbose)
	cout << "creating tri stimulus\n";

      if(!last_stimulus) {
	//create_stimulus(NEW_TRI,stim_name);
	//last_stimulus = new triangle_wave;
	valid_options = TRI_OPTIONS;
	options_entered = STIM_TRI;
      } else
	cout << "warning: ignoring tri stim creation";
      break;

    case STIM_DUMP:
      stimulus();      // Display the list of stimuli.
      return;

    case STIM_DIGITAL:
      if(last_stimulus)
	last_stimulus->set_digital();
      //digital = 1;
      return;

    case STIM_ANALOG:
      if(last_stimulus)
	last_stimulus->set_analog();
      //digital = 0;
      return;

    default:
      cout << " Invalid stimulus option\n";
      return;
    }

  // Initialize the default stimulus parameters
  /*
  period = 1000;
  phase = 0;
  high_time = 500;
  initial_state = 1;
  start_cycle = 0;
  bit_pos = 0;
  states = 0;
  time_flag = 1;
  digital = 1;
  have_data = 0;

  if(temp_array.size())
    temp_array.erase(temp_array.begin(), temp_array.end());
  */
}


void cmd_stimulus::stimulus(cmd_options_num *con)
{

  switch(con->co->value)
    {
    case STIM_PHASE:
      if(verbose)
	cout << "stimulus command got the phase " << con->n << '\n';

      if(last_stimulus)
	last_stimulus->put_phase(con->n);
      //stimorb_phase( con->n);
      break;

    case STIM_PERIOD:
      if(verbose)
	cout << "stimulus command got the period " << con->n << '\n';

      if(last_stimulus)
	last_stimulus->put_period(con->n);
      //stimorb_period( con->n);
      break;

    case STIM_HIGH_TIME:
      if(verbose)
	cout << "stimulus command got the high_time " << con->n << '\n';

      if(last_stimulus)
	last_stimulus->put_duty(con->n);
      //stimorb_duty( con->n);
      break;

    case STIM_INITIAL_STATE:
      if(verbose)
	cout << "stimulus command got the initial_state " << con->n << '\n';

      if(last_stimulus)
	last_stimulus->put_initial_state(con->n);
      //stimorb_initial_state( con->n);
      break;

    case STIM_START_CYCLE:
      if(verbose)
	cout << "stimulus command got the start_cycle " << con->n << '\n';

      if(last_stimulus)
	last_stimulus->put_start_cycle(con->n);
      //stimorb_start_cycle( con->n);
      break;

    default:
      cout << " Invalid stimulus option\n";
      return;
    }

  options_entered |= con->co->value;

}

// %%% FIXME %%%
void cmd_stimulus::stimulus(cmd_options_float *cof)
{

  if(!last_stimulus) {
    cout << "warning: Ignoring stimulus (float) option because there's no stimulus defined.";
    return;
  }

  int n;

  if(cof->f > 0.0)
    n = 1;
  else
    n = 0;

  if(verbose)
    cout << "stimulus command got floating point option\n";

  switch(cof->co->value)
    {
    case STIM_PHASE:
      if(verbose)
	cout << "stimulus command got the phase " << n << '\n';

      last_stimulus->put_phase(n);
      break;

    case STIM_PERIOD:
      if(verbose)
	cout << "stimulus command got the period " << n << '\n';

      last_stimulus->put_period(n);
      break;

    case STIM_HIGH_TIME:
      if(verbose)
	cout << "stimulus command got the high_time " << n << '\n';

      last_stimulus->put_duty(n);
      break;

    case STIM_INITIAL_STATE:
      if(verbose)
	cout << "stimulus command got the initial_state " << n << '\n';

      last_stimulus->put_initial_state(n);
      break;

    case STIM_START_CYCLE:
      if(verbose)
	cout << "stimulus command got the start_cycle " << n << '\n';

      last_stimulus->put_start_cycle(n);
      break;

    default:
      cout << " Invalid stimulus option\n";
      return;
    }

  options_entered |= cof->co->value;

}

void cmd_stimulus::stimulus(cmd_options_str *cos)
{

  if(!last_stimulus) {
    cout << "warning: Ignoring stimulus (string) option because there's no stimulus defined.\n";
    return;
  }


  switch(cos->co->value)
    {
    case STIM_NAME:
      if(verbose)
	cout << "stimulus command got the name " << cos->str << '\n';

      last_stimulus->new_name(cos->str);

      break;
    case STIM_PORT:
      cout << "the port option has been deprecated\n";
    }

  options_entered |= cos->co->value;
}

void cmd_stimulus::data_point(guint64 new_data_point)
{

  if(verbose)
    cout << "stimulus command got integer data\n";

  if(last_stimulus)
    last_stimulus->put_data(new_data_point);

  have_data = 1;

}
void cmd_stimulus::data_point(float new_data_point)
{
  if(verbose)
    cout << "stimulus command got got float data\n";

  if(last_stimulus)
    last_stimulus->put_data(new_data_point);

  have_data = 1;

}


//-----------------
// end()
// All of the stimulus' options have been entered. Now it's time
// to create the stimulus.

void cmd_stimulus::end(void)
{
  if(!last_stimulus) {
    cout << "warning: Ignoring stimulus (string) option because there's no stimulus defined.";
    return;
  }

  switch( options_entered & (STIM_SQW | STIM_TRI | STIM_ASY))
    {
    case STIM_SQW:
      if(verbose)
	cout << "created sqw stimulus\n";
      break;

    case STIM_ASY:
      if(verbose)
	cout << "created asy stimulus\n";
      last_stimulus->start();
      break;

    case STIM_TRI:
      if(verbose)
	cout << "creating tri stimulus\n";
      break;

    }

  last_stimulus = 0;
}
