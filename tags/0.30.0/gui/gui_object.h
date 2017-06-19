/*
   Copyright (C) 1998,1999,2000,2001,2002,2003,2004
   T. Scott Dattalo and Ralf Forsberg

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

#ifndef __GUI_OBJECT_H__
#define __GUI_OBJECT_H__

//
// Forward reference
//

class GUI_Processor;

//========================================================================
// GUI_Object
//  All window attributes that are common are placed into the GUI_Object
// structure. This structure is then include in each of the other structures.
// It's also the very first item in these 'derived' structures. Consequently a
// pointer to one object may be type cast into another.
//

class GUI_Object {
 public:

  GUI_Processor *gp;

  GtkWidget *window;

  const char *menu;

  // Window geometry. This info is saved when the window associated
  // with this gui object is hidden. Note: gtk saves the window origin
  // (x,y) but doesn't save the size (width,height).
  int x,y,width,height;
  int enabled;   // Whether or not the window is up on the screen
  bool bIsBuilt;  // Whether or not the window is built

  // A pointer to a function that will allow the window associated
  // with this gui object to be viewable or hidden.

#define VIEW_HIDE 0
#define VIEW_SHOW 1
#define VIEW_TOGGLE 2

  GUI_Object();
  virtual ~GUI_Object();
  virtual void ChangeView(gboolean view_state);

  int get_config();
  void check();
  int set_default_config();
  virtual int set_config();

  virtual void Build() = 0;
  virtual void UpdateMenuItem();
  virtual void Update() = 0;
  virtual void NewProcessor(GUI_Processor *_gp)
    {
      gp = _gp;
    }

protected:
  virtual const char *name() = 0;

private:
};



#endif  // __GUI_OBJECT_H__
