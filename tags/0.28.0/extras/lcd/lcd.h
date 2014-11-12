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

#include <src/stimuli.h>
#include <src/ioports.h>
#include <src/symbol.h>
#include <src/trace.h>
#include <src/gpsim_interface.h>

#include <gtk/gtk.h>

class LcdDisplay;
class LCD_InputPin;
class HD44780;

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
  virtual void Update  (gpointer object);


  LCD_Interface(LcdDisplay *_gp);
};


typedef char _5X7 [7][6];

class LcdFont 
{
public:
  gint num_elements;
  GdkPixmap **pixmaps;
  GdkWindow *mywindow;	// stashed to allow font regenaration

  //CreateFont(GtkWidget *, LcdDisplay *);

  LcdFont(gint, GtkWidget *, LcdDisplay *);
  void update_pixmap(int, _5X7 *, LcdDisplay *);
  GdkPixmap *getPixMap(unsigned int);
};

//------------------------------------------------------------------------
//
// lcd tracing
//
class LcdDisplay;
class LcdTraceType : public TraceType {
public:
  LcdDisplay *lcd;

  LcdTraceType(LcdDisplay *_lcd, 
	       unsigned int s)
    : TraceType(s, "LCD"), lcd(_lcd)
  {
  }

  virtual TraceObject *decode(unsigned int tbi) = 0;
};

class LcdWriteTT : public LcdTraceType {
public:
  LcdWriteTT(LcdDisplay *lcd, unsigned int s);

  virtual TraceObject *decode(unsigned int tbi);
  virtual int dump_raw(unsigned tbi, char *buf, int bufsize);

};

class LcdReadTT : public LcdTraceType {
public:
  LcdReadTT(LcdDisplay *lcd, unsigned int s);

  virtual TraceObject *decode(unsigned int tbi);
  virtual int dump_raw(unsigned tbi, char *buf, int bufsize);

};

class LcdTraceObject : public TraceObject {
public:
  LcdDisplay *lcd;

  LcdTraceObject(LcdDisplay *_lcd) : TraceObject() , lcd(_lcd)
  {
  }

  virtual void print(FILE *)=0;
};

class LcdWriteTO : public LcdTraceObject {
public:
  LcdWriteTO(LcdDisplay *_lcd);
  virtual void print(FILE *);
};

class LcdReadTO : public LcdTraceObject {
public:
  LcdReadTO(LcdDisplay *_lcd);
  virtual void print(FILE *);
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
  ST_STATUS_READ,
  ST_DATA_READ,

  //_8BITMODE_START,
  //_8BITMODE,
};
enum ePins {
  eDC,
  eE,
  eRW
};


//========================================================================
class LcdDisplay : public Module
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
  //  int mode_flag;
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

  guint rows,cols;   // e.g. 2x20 would be 2 rows, 20 columns
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

  //gint **ch_data;
  gchar *title;
  GtkWidget *window;
  GtkWidget *darea;
  gint w_width,w_height;

  //
  // Tracing
  //
  TraceType *readTT, *writeTT;
  TraceType *getWriteTT();
  TraceType *getReadTT();


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

  void move_cursor(unsigned int new_row, unsigned int new_column);
  void clear_display(void);
  void write_data(int new_data);

  LcdDisplay(const char *, int aRows, int aCols, unsigned aType=0);
  ~LcdDisplay();


  void CreateGraphics (void);
  //void InitStateMachine(void);
  //void test(void);
  void build_window( void );
  void update(void);
  void update( GtkWidget *drawable,   
	       guint max_width,
	       guint max_height);
  void update_cgram_pixmaps(void);

  // Inheritances from the Package class
  virtual void create_iopin_map(void);

  // Inheritance from Module class
  const virtual char *type(void) { return ("lcd_display"); };
  static Module *construct(const char *new_name);
  void testHD44780();
  bool dataBusDirection();
  void UpdatePinState(ePins, char);
protected:

  LCD_InputPin *m_E;
  LCD_InputPin *m_RW;
  LCD_InputPin *m_DC;
  PortRegister *m_dataBus;

  HD44780 *m_hd44780;
  unsigned int m_controlState;
  bool cgram_updated;

};

class LcdDisplayDisplaytech161A : public LcdDisplay
{
public:
  LcdDisplayDisplaytech161A(const char *, int aRows, int aCols, unsigned aType);
  ~LcdDisplayDisplaytech161A();

  // Inheritance from Module class
  const virtual char *type(void) { return ("lcd_dt161A"); };
  static Module *construct(const char *new_name);

};

// bit flags that can be set in the GPSIM_LCD_DEBUG environment variable
#define LCD_DEBUG_ENABLE	0x01	// Enable debug printfs
#define LCD_DEBUG_DUMP_PINS	0x02	// Dump changes in the LCD pins
#define LCD_DEBUG_TRACE_DATA	0x04	// Trace changes in CGRAM/DDRAM
#define LCD_DEBUG_TRACE_PORT	0x08	// Trace data on port

#endif //  __LCD_H__
