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


#ifndef __LCD_H__
#define __LCD_H__

//#include "state_machine.h"

#include <gpsim/stimuli.h>
#include <gpsim/ioports.h>
#include <gpsim/symbol.h>
#include <gpsim/trace.h>
#include <gpsim/gpsim_interface.h>

#include <gtk/gtk.h>

class LcdDisplay;


// Create an interface to the simulator

class LCD_Interface : public Interface
{
private:
  LcdDisplay *lcd;

public:

  //virtual void UpdateObject (gpointer xref,int new_value);
  //virtual void RemoveObject (gpointer xref);
  virtual void SimulationHasStopped (gpointer object);
  //virtual void NewProcessor (unsigned int processor_id);
  //virtual void NewModule (Module *module);
  //virtual void NodeConfigurationChanged (Stimulus_Node *node);
  //virtual void NewProgram  (unsigned int processor_id);
  virtual void GuiUpdate  (gpointer object);


  LCD_Interface(LcdDisplay *_gp);
};


// Create a few classes from which an LCD may be constructed

class Lcd_Port : public IOPORT
{
public:

  LcdDisplay *lcd;
  virtual void trace_register_write(void);
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual void assert_event(void);

  Lcd_Port (unsigned int _num_iopins=8);

};

class ControlPort : public Lcd_Port, public BreakCallBack
{
public:

  guint64 break_delta;
  void put(unsigned int new_value);
  virtual void assert_event(void);
  ControlPort (unsigned int _num_iopins=8);
  virtual void callback(void);
};

class DataPort : public Lcd_Port
{
public:
  unsigned int direction;
  bool acceptingData;

  void put(unsigned int new_value);
  virtual void assert_event(void);
  virtual void setbit(unsigned int bit_number, bool new_value);
  DataPort (unsigned int _num_iopins=8);
  void update_pin_directions(unsigned int );

  unsigned int get(void);
};

// Create a class derived from the IO_input class that
// will allow us to intercept when the I/O input is being
// driven. (This isn't done for PIC I/O pins because the
// logic for handling I/O pin changes resides in the IOPORT
// class.)

class Lcd_Input : public IO_input
{
public:

  virtual void put_node_state( int new_state);

  Lcd_Input (IOPORT *i, unsigned int b, char *opt_name=NULL) : IO_input(i,b,opt_name) { };

};

class Lcd_bi_directional : public IO_bi_directional
{
public:

  virtual void put_node_state( int new_state);

  Lcd_bi_directional(IOPORT *i, unsigned int b,char *opt_name=NULL);

};

class LcdFont 
{
public:
  gint num_elements;
  GdkPixmap **pixmaps;

  //CreateFont(GtkWidget *, LcdDisplay *);

  LcdFont(gint, GtkWidget *, LcdDisplay *);
};


//
//     State Machine Events
//

enum ControlLineEvent {
  ERD = 0,
  ERC,
  EWD,
  EWC,
  eRD,
  eRC,
  eWD,
  eWC,
  DataChange,
  BAD_EVENT
};


class SMObject
{

public:
  char *nameP;            // Object name

  SMObject(void) {
    nameP = NULL;
  };

  SMObject(char *_nameP) {
    cout << "SMObject constructor\n";
    if(_nameP)
      nameP = strdup(_nameP);
    else
      nameP = NULL;
  };

  ~SMObject(void) {
    if(nameP)
      free(nameP);
  };

  const char *get_name(void) { return nameP; };
  void new_name(char *nP) {
    if(nameP)
      free(nameP);
    nameP = strdup(nP);
  };

};


class SMEvent : public SMObject
{
public:
  ControlLineEvent e;
  SMEvent(ControlLineEvent _e,char *_nameP): SMObject(_nameP) {e = _e;};
  SMEvent(char *_nameP=NULL) : SMObject(_nameP) {e = BAD_EVENT;};
  void init(ControlLineEvent _e,char *_nameP) {
    e = _e;
    new_name(_nameP);
  };

};


  
//
//     State Machine States
//

enum State {
  POWERON,
  ST_INITIALIZED,
  ST_COMMAND_PH0,
  ST_DATA_PH0,
  ST_STATUS_READ,

  //_8BITMODE_START,
  //_8BITMODE,
};


class LcdDisplay : public ExternalModule
{
public:

#define _8BIT_MODE_FLAG         (1<<0)
#define LARGE_FONT_MODE_FLAG    (1<<1)
#define _2LINE_MODE_FLAG        (1<<2)
#define DISPLAY_ON_FLAG         (1<<3)
#define CURSOR_ON_FLAG          (1<<4)
#define BLINK_ON_FLAG           (1<<5)

  LCD_Interface *interface;

  State current_state, previous_state;
  ControlLineEvent last_event;
  int mode_flag;
  int data_latch;
  int data_latch_phase;
  int debug;

  SMEvent ControlEvents[8];

  struct {
    int row;
    int col;
  } cursor;

  //
  // Here's the graphical portion
  //

  GdkGC *gc;

  gint rows,cols;   // e.g. 2x20 would be 2 rows, 20 columns
#define TWO_ROWS_IN_ONE 1
  unsigned disp_type;

  struct {          // Resolution in lcd pixels e.g. 5x7
    gint x;
    gint y;
  } dots;

  struct {          // Resolution in crt pixels - scaled
    gint x;
    gint y;
  } pixels;

  gfloat contrast;  // pixel on/off ratio

  LcdFont *fontP;

  GdkColor
    *dot_color;     // LCD pixel color (controlled by contrast)

  gint **ch_data;
  gchar *title;
  GtkWidget *window;
  GtkWidget *darea;
  gint w_width,w_height;

  //
  // Graphics member functions
  //

  void set_pixel_resolution(gint _x=5, gint _y=7) {
    dots.x =_x; dots.y = _y;
  };

  void set_crt_resolution(gint _x=3, gint _y=3) {
    pixels.x = _x; pixels.y= _y;
  };

  void set_contrast(gfloat _contrast=1.0) {
    contrast = _contrast;
  };

  gint get_char_width(void) {
    return (1+dots.x * pixels.x);
  };

  gint get_char_height(void) {
    return (dots.y * pixels.y);
  };

  gint get_border(void) {
    return 5;
  };

  GdkPixmap *get_pixmap(gint row, gint col);

  void move_cursor(int new_row, int new_column);
  void clear_display(void);
  void write_data(int new_data);
  void write_ddram_address(int data);

  // Here's the logic portion

  ControlPort *control_port;
  DataPort *data_port;

  //  LCDStateMachine state_machine;

  LcdDisplay(int aRows, int aCols, unsigned aType=0);
  ~LcdDisplay();


  void CreateGraphics (void);
  void InitStateMachine(void);
  void test(void);
  void build_window( void );
  void update(void);
  void update( GtkWidget *drawable,   
	       guint max_width,
	       guint max_height);

  // Inheritances from the Package class
  virtual void create_iopin_map(void);

  // Inheritance from Module class
  const virtual char *type(void) { return ("lcd_display"); };
  static ExternalModule *construct(const char *new_name);


  // State Machine Functions:
  void advanceState(ControlLineEvent e);
  void newState(State s);
  void revertState(void);
  void viewInternals(int);   // debugging

  char *getEventName( ControlLineEvent e);
  char *getStateName( State s);

  void execute_command(void);
  void start_data(void);
  void send_status(void);
  void release_port(void);
  void new_command(void);
  void new_data(void);

  void set_8bit_mode(void) { mode_flag |= _8BIT_MODE_FLAG;};
  void set_4bit_mode(void) { mode_flag &= ~_8BIT_MODE_FLAG;};
  void set_2line_mode(void) { mode_flag |= _2LINE_MODE_FLAG;};
  void set_1line_mode(void) { mode_flag &= ~_2LINE_MODE_FLAG;};
  void set_large_font_mode(void) { mode_flag |= LARGE_FONT_MODE_FLAG;};
  void set_small_font_mode(void) { mode_flag &= ~LARGE_FONT_MODE_FLAG;};
  void set_display_on(void) { mode_flag |= DISPLAY_ON_FLAG;};
  void set_display_off(void) { mode_flag &= ~DISPLAY_ON_FLAG;};
  void set_blink_on(void) { mode_flag |= BLINK_ON_FLAG;};
  void set_blink_off(void) { mode_flag &= ~BLINK_ON_FLAG;};
  void set_cursor_on(void) { mode_flag |= CURSOR_ON_FLAG;};
  void set_cursor_off(void) { mode_flag &= ~CURSOR_ON_FLAG;};

  bool in_8bit_mode(void) {return ((mode_flag & _8BIT_MODE_FLAG) != 0);};
  bool in_4bit_mode(void) {return ((mode_flag & _8BIT_MODE_FLAG) == 0);};
  bool in_2line_mode(void) {return ((mode_flag & _2LINE_MODE_FLAG) != 0);};
  bool in_1line_mode(void) {return ((mode_flag & _2LINE_MODE_FLAG) == 0);};
  bool in_large_font_mode(void) {return ((mode_flag & LARGE_FONT_MODE_FLAG) != 0);};
  bool in_small_font_mode(void) {return ((mode_flag & LARGE_FONT_MODE_FLAG) == 0);};
  bool display_is_on(void) {return ((mode_flag & DISPLAY_ON_FLAG) != 0);};
  bool display_is_off(void) {return ((mode_flag & DISPLAY_ON_FLAG) == 0);};


};

class LcdDisplayDisplaytech161A : public LcdDisplay
{
public:
  LcdDisplayDisplaytech161A(int aRows, int aCols, unsigned aType);
  ~LcdDisplayDisplaytech161A();

  // Inheritance from Module class
  const virtual char *type(void) { return ("lcd_dt161A"); };
  static ExternalModule *construct(const char *new_name);

};

#endif //  __LCD_H__
