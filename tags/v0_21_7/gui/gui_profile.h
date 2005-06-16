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

#ifndef __GUI_PROFILE_H__
#define __GUI_PROFILE_H__


//
// The profile window
//

struct cycle_histogram_counter{
    // Three variables that determine which cycle_histogram_counter we add
    // the differences in cycle counter to:
    unsigned int start_address; // Start profile address
    unsigned int stop_address; // Stop profile address
    guint64 histo_cycles; // The number of cycles that this counter counts.

    unsigned int count; // The number of time 'cycles' cycles are used.
};

class Profile_Window : public GUI_Object
{
 public:

  int program;    // if non-zero window has program

  GtkCList *profile_clist;
  GtkCList *profile_range_clist;
  GtkCList *profile_register_clist;
  GtkCList *profile_exestats_clist;
  GList *profile_list;
  GList *profile_range_list;
  GList *profile_register_list;
  GList *profile_exestats_list;
  GtkWidget *notebook;
  gint range_current_row;
  GtkWidget *exestats_popup_menu;
  GtkWidget *range_popup_menu;
  GtkWidget *plot_popup_menu;
  GtkWidget *plot_canvas;

  // List of cycle_count structs
  GList *histogram_profile_list;

  Profile_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void Update(void);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void NewProgram(GUI_Processor *gp);
  virtual void StopExe(int address);
  virtual void StartExe(int address);

};



#endif // __GUI_PROFILE_H__

