/*
   Copyright (C) 1998,1999,2000,2001
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


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "../config.h"

#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

#include <gtkextra/gtkbordercombo.h>
#include <gtkextra/gtkcolorcombo.h>
#include <gtkextra/gtksheet.h>

#include "gui.h"
#include "gui_breadboard.h"
#include "gui_processor.h"
#include "gui_profile.h"
#include "gui_register.h"
#include "gui_regwin.h"
#include "gui_scope.h"
#include "gui_src.h"
#include "gui_stack.h"
#include "gui_stopwatch.h"
#include "gui_symbols.h"
#include "gui_trace.h"
#include "gui_watch.h"
#include "gui_statusbar.h"

class linkXREF : public CrossReferenceToGUI
{
public:

  GUI_Processor *gp;
  
  void Update(int new_value)
  {
    int address;

    if(!gp) { printf("gp == null in linkXREF\n"); return;}

    address = *(int *)data;

    if(gp->source_browser)
      gp->source_browser->UpdateLine(address);

    if(gp->program_memory)
      gp->program_memory->UpdateLine(address);

  }
};

/*
 * link_src_to_gpsim
 *
 * After a new program has been loaded by gpsim, this routine is called
 * so that the gui can create links to it. This consists of creating
 * a 'cross_reference' structure and attaching it to each pic instruction.
 * The information in the cross_reference structure is a pointer to the
 * gp.
 */

void link_src_to_gpsim(GUI_Processor *gp)
{
  int i,*address,pm_size;
  linkXREF *cross_reference;


  if(gp) {

    // Create a cross reference between the pic's program memory and the gui.
    pm_size =  gp->cpu->program_memory_size();

    if(verbose) {
      printf("link_src_to_gpsim\n");
      printf(" processor pma = %d\n",pm_size);
    }

    for(i=0; i < pm_size; i++) {

      cross_reference = new linkXREF();
      //cross_reference->parent_window_type = WT_asm_source_window;
      //cross_reference->parent_window = (gpointer) gp;
      cross_reference-> gp = gp;
      address = (int *) malloc(sizeof(int *));
      *address = gp->cpu->map_pm_index2address(i);

      cross_reference->data = (gpointer) address;
      gp->cpu->pma->assign_xref(*address,(gpointer) cross_reference);
    }
  }
}

#endif //HAVE_GUI
