#include <stdio.h>
#include <stdlib.h>

#ifdef DOING_GNOME
#include <gnome.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

#include <gtkextra/gtkcombobox.h>
#include <gtkextra/gtkbordercombo.h>
#include <gtkextra/gtkcolorcombo.h>
#include <gtkextra/gtksheet.h>
//#include <gtkextra/gtksheetentry.h>

#include "gui.h"
#include "gui_callbacks.h"


void gp_add_window_to_list(GUI_Processor *gp, GUI_Object *go)
{

  if(!gp) return;

/*  gui_config_load_winattr(go->name,&winattr);
  
    go->x=winattr.x;
    go->y=winattr.y;
    go->width=winattr.width;
    go->height=winattr.height;
    go->visible=winattr.visible;
*/
  // add resize and move callbacks
/*  if(!go->visible)
  {
      puts("HASODJDASFHGJHFDSJ");
      go->change_view(go,VIEW_HIDE);
  }*/
  
  switch(go->wc)
    {
    case WC_misc:
      gp->misc_windows = g_list_append(gp->misc_windows, (gpointer)go);
      break;

    case WC_source:
      gp->source_windows = g_list_append(gp->source_windows, (gpointer)go);
      break;

    case WC_data:
      gp->data_windows = g_list_append(gp->data_windows, (gpointer)go);
      break;

    default:
      g_warning("bad window type in gp_add_window_to_list");
    }


}
  
GUI_Processor *new_GUI_Processor(void)
{

  GUI_Processor *gp;

  gp = (GUI_Processor *)malloc(sizeof(GUI_Processor));
  gp->source_windows = g_list_alloc();
  gp->data_windows = g_list_alloc();
  gp->misc_windows = g_list_alloc();
  gp->pic_id = 0;

  return gp;
}
