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


#if !defined(__TRIGGER_H__)
#define __TRIGGER_H__

class TriggerObject;

//========================================================================
//
//
class TriggerAction
{
public:
  TriggerAction();
  virtual ~TriggerAction() {}
  virtual bool evaluate(void);
  virtual bool getTriggerState(void);
  virtual void action(void);
};

class SimpleTriggerAction : public TriggerAction
{
public:
  SimpleTriggerAction(TriggerObject *_to);
  virtual void action(void);
protected:
  TriggerObject *to;

};

// TriggerObject - a base class for handling all of gpsim's breakpoints.
//
// The TriggerObject class is designed to be part of a multiple inheritance
// class heirarchy. Its main function is to provide interface to the 
// breakpoint functionality.

class TriggerObject
{
 public:

  unsigned int bpn;

  // When the TriggerObject becomes true, then the TriggerAction is 
  // evaluated. E.g. If the trigger object is an execution breakpoint,
  // then whenever the PC == break address, the Breakpoint_Instruction
  // class (which is derived from this class) will invoke action->evaluate()
  // which will in turn halt the execution.

  TriggerAction *action;

  // Enable the breakpoint and return true if successful
  virtual bool set_break(void) {return false;}

  // A unique number assigned when the break point is armed.
  int CallBackID;

  // When the breakpoint associated with this object is encountered,
  // then 'callback' is invoked.
  virtual void callback(void);

  // Invoked to display info about the breakpoint.
  virtual void callback_print(void);

  // clear_break is invoked when the breakpoint associated with
  // this object is cleared. 
  virtual void clear_break(void) {};

  // Will search for a place to store this break point.
  virtual int find_free(void);

  // This object has no cpu associated with it. However, derived
  // types may and can choose to provide access to it through here:
  //virtual Processor *get_cpu(void) { return 0; }

  // Display the breakpoint - Probably should tie into a stream...
  virtual void print(void);

  // Clear the breakpoint
  virtual void clear(void);

  virtual char const * bpName() { return "Generic"; }

  virtual void set_action(TriggerAction *ta) { action = ta; }
  virtual TriggerAction *get_action(void) { return action;}

  TriggerObject();
  TriggerObject(TriggerAction *);
  // Virtual destructor place holder
  virtual ~TriggerObject() { }

};



#endif // !defined(__TRIGGER_H__)
