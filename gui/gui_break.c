#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

//#include <iostream.h>

#include <gtkextra/gtkcombobox.h>
#include <gtkextra/gtkbordercombo.h>
#include <gtkextra/gtkcolorcombo.h>
#include <gtkextra/gtksheet.h>
//#include <gtkextra/gtksheetentry.h>

#include "gui.h"

extern GUI_Processor *gp;

void gui_simulation_has_stopped(void)
{
  if(gp)
    {
      RegWindow_update(gp->regwin_ram);
      RegWindow_update(gp->regwin_eeprom);
      StatusBar_update(gp->status_bar);
      SourceBrowser_update(gp->program_memory);
      SourceBrowser_update(gp->source_browser);
      WatchWindow_update(gp->watch_window);
    }
}


/*
 * gui_cycle_callback
 *
 * This callback routine is invoked on 'cycle' break points.
 * In other words, the gui will set a break point at a specific
 * simulation cycle number in order to regain control (for 
 * updating displays or whatever). When the simulator hits
 * that break point, this is the routine that is called.
 */

void  gui_cycle_callback (gpointer callback_data)
{

  if(callback_data)
    {
      GUI_Processor *gp = (GUI_Processor *) callback_data;
      RegWindow_update(gp->regwin_ram);
      RegWindow_update(gp->regwin_eeprom);
      StatusBar_update(gp->status_bar);
//      SourceBrowser_update(gp->program_memory);
//      SourceBrowser_update(gp->source_browser);
      WatchWindow_update(gp->watch_window);
    }
  else
    {
      printf("*** warning - the gui got an unkown break point (gui_break.c)\n");
    }

}

/*
 * init_link_to_gpsim
 *
 * After a processor has been loaded, this routine will be called.
 * Its purpose is to establish a communication link between the
 * simulator and the gui. 
 */

void init_link_to_gpsim(GUI_Processor *gp)
{
  int cycle;
  int i,*address;
  struct cross_reference_to_gui *cross_reference;

  if(verbose)
    printf("init link to gpsim\n");

  if(gp)
    {


      //      cycle = gp->p->cycles.value_lo + 0x100000;

      //      gui_set_cycle_break_point(cycle, gui_cycle_callback, (gpointer) gp);

      gpsim_set_cyclic_break_point( gp->pic_id, 
				   gui_cycle_callback, 
				   (gpointer) gp,
				   gpsim_get_update_rate());

      if(verbose)
	printf("gui break was set\n");

    }



}
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
  int cycle;
  int i,*address,pm_size;
  struct cross_reference_to_gui *cross_reference;


  if(gp)
    {
      printf("link_src_to_gpsim\n");
      // Create a cross reference between the pic's program memory and the gui.
      pm_size = gpsim_get_program_memory_size( gp->pic_id);

      for(i=0; i < pm_size; i++)
	{
	  cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
	  cross_reference->parent_window_type =   WT_asm_source_window;
	  cross_reference->parent_window = (gpointer) gp;
	  address = (int *) malloc(sizeof(int));
	  *address = i;

	  cross_reference->data = (gpointer) address;
	  cross_reference->update = SourceBrowser_update_line;

	  //gp->p->program_memory[i]->assign_xref((gpointer) cross_reference);
	  gpsim_assign_program_xref(gp->pic_id, i,(gpointer) cross_reference);
	}
    }
}
