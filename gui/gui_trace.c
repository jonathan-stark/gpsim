#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

#include <assert.h>

#include "../src/interface.h"

#include "gui.h"


#define MAXTRACES  100
#define TRACE_COLUMNS    2

typedef enum {
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_READ_VALUE,
    MENU_BREAK_WRITE_VALUE,
    MENU_ADD_WATCH,
} menu_id;

static char *trace_titles[TRACE_COLUMNS]={"Cycle", "Trace"};

// gui trace flags:
#define GTF_ENABLE_XREF_UPDATES    (1<<0)

guint64 row_to_cycle[MAXTRACES];


//struct TraceMapping trace_map[MAXTRACES];


/*****************************************************************
 * xref_update
 *
 * This is called by the simulator when it has been determined that
 * that the trace buffer has changed and needs to be updated
 */
static void xref_update(struct cross_reference_to_gui *xref, int new_value)
{

  GUI_Processor *gp;
  guint64 cycle;
  int trace_index;

#define TRACE_STRING 100
//  char str[TRACE_STRING];
  GtkCList *clist;

  char cycle_string[TRACE_STRING];
  char trace_string[TRACE_STRING];
  char *entry[TRACE_COLUMNS]={cycle_string,trace_string};

  Trace_Window *tw;

  if(xref == NULL)
    {
      printf("Warning gui_trace.c: xref_update: xref=%x\n",(unsigned int)xref);
      return;
    }

  tw  = (Trace_Window *) (xref->parent_window);
  if(  (tw == NULL)  || (!((GUI_Object*)tw)->enabled))
    return;

  // Get the pointer to the `gui processor' structure
  gp = ((GUI_Object*)tw)->gp;

  if(gp==NULL || gp->pic_id==0)
    {
      puts("Warning gp or gp->pic_id == NULL in TraceWindow_update");
      return;
    }

  // If we're not allowing xref updates then exit
  if( !(tw->trace_flags & GTF_ENABLE_XREF_UPDATES))
    return;

  trace_string[0] = 0;  // Assume that the trace is empty.
  gpsim_get_current_trace(&cycle, &trace_index, trace_string, TRACE_STRING);

  if(trace_string[0] && (cycle>=tw->last_cycle)) {
    tw->last_cycle = cycle;
    tw->trace_map[tw->trace_map_index].cycle = cycle;
    tw->trace_map[tw->trace_map_index].simulation_trace_index = index;

    // Advance the trace_map_index using rollover arithmetic
    if(++tw->trace_map_index >= MAXTRACES)
      tw->trace_map_index = 0;

    clist=GTK_CLIST(tw->trace_clist);
//    gtk_clist_freeze(clist);

    // Delete the first row in the clist
//    gtk_clist_delete_rows(clist,0,1);

    // and then add a row at the end for the new trace data
//    gtk_clist_add_row(clist,1);
    
/*    gtk_clist_set_cell(clist,
		       clist->maxrow,
		       1,  // column
		       GTK_JUSTIFY_LEFT,str);*/

    sprintf(cycle_string,"0x%016x", cycle);

    //trace_map[trace_index].cycle = cycle;
    //trace_map[trace_index].simulation_trace_index = gpsim_get;
    
/*    gtk_clist_set_cell(clist,
		       clist->maxrow,
		       0,  // column
		       GTK_JUSTIFY_LEFT,str);*/
    gtk_clist_append  (clist, entry);

    if(clist->rows>MAXTRACES)
        gtk_clist_remove(clist,0);

//    gtk_clist_thaw(clist);
  }


  //update(tw,entry,new_value);
}

/*****************************************************************
 * TraceWindow_update
 *
 * The purpose of this routine is to refresh the trace window with
 * the latest trace information. The current pic simulation cycle (should
 * this be change to real time???) is examined and compared to what
 * is currently displayed in the trace window. If the info in the 
 * trace window is really old, this the entire window is deleted and
 * the trace is redrawn with the latest. If the trace window is rather
 * recent then the older trace info is deleted and the new is appended
 * to the end.
 *
 * INPUTS: *tw a pointer to a Trace_Window structure.
 */

void TraceWindow_update(Trace_Window *tw)
{
  GUI_Processor *gp;
  GtkCList *trace_clist;
  char buffer[50];
  guint64 cycle;
  guint64 count;
  int i;

  if(  (tw == NULL)  || (!((GUI_Object*)tw)->enabled))
    return;

  // Get the pointer to the `gui processor' structure
  gp = ((GUI_Object*)tw)->gp;

  if(gp==NULL || gp->pic_id==0)
  {
      puts("Warning gp or gp->pic_id == NULL in TraceWindow_update");
      return;
  }

  // Get a convenient pointer to the gtk_clist that the trace is in.
  trace_clist=GTK_CLIST(tw->trace_clist);

  gtk_clist_freeze(trace_clist);

  cycle = gpsim_get_cycles(gp->pic_id);

  tw->trace_flags |= GTF_ENABLE_XREF_UPDATES;
  if(cycle-tw->last_cycle>=MAXTRACES) {
    // redraw the whole thing
    gpsim_trace_dump_to_file(MAXTRACES, NULL);

  } else {
    gpsim_trace_dump_to_file(cycle-tw->last_cycle, NULL);


  }

  tw->trace_flags &= ~GTF_ENABLE_XREF_UPDATES;
  tw->last_cycle = cycle;
  gtk_clist_thaw(trace_clist);

}


/*****************************************************************
 * TraceWindow_new_processor
 *
 * 
 */

void TraceWindow_new_processor(Trace_Window *tw, GUI_Processor *gp)
{

#define NAME_SIZE 32

    gint i,j, border_mask, border_width;
    GtkCList *clist;
    struct cross_reference_to_gui *cross_reference;
    gboolean row_created;
//    GtkCListRange range;
    int pic_id;
    char row_label[50];


    if(tw == NULL || gp == NULL)
	return;

    tw->processor=1;
    
    if( !((GUI_Object*)tw)->enabled)
	return;
    
    tw->gui_obj.gp = gp;
    pic_id = gp->pic_id;


    cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
    cross_reference->parent_window_type =  WT_trace_window;
    cross_reference->parent_window = (gpointer) tw;
    cross_reference->data = NULL;
    cross_reference->update = xref_update;
    cross_reference->remove = NULL;
    gpsim_assign_trace_xref((gpointer) cross_reference);
}

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Register_Window *rw)
{
  ((GUI_Object *)rw)->change_view((GUI_Object*)rw,VIEW_HIDE);
  return TRUE;
}

int
BuildTraceWindow(Trace_Window *tw)
{
  GtkWidget *window;
  GtkWidget *trace_clist;
  GtkWidget *main_vbox;
  GtkWidget *scrolled_window;

    
  gchar name[10];
  gint i;
  gint column_width,char_width;

  int x,y,width,height;
  
	
  if(tw==NULL)
  {
      printf("Warning build_trace_viewer(%x)\n",(unsigned int)tw);
      return;
  }

	
  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  ((GUI_Object*)tw)->window=window;

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed), &window);

  main_vbox=gtk_vbox_new(FALSE,1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox),0); 
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  gtk_window_set_title(GTK_WINDOW(window), "trace viewer");

  // Trace clist
  trace_clist=gtk_clist_new_with_titles(TRACE_COLUMNS,trace_titles);
  gtk_clist_set_column_auto_resize(GTK_CLIST(trace_clist),0,TRUE);

  GTK_WIDGET_UNSET_FLAGS(trace_clist,GTK_CAN_DEFAULT);
    
  tw->trace_clist = GTK_CLIST(trace_clist);

  width=((GUI_Object*)tw)->width;
  height=((GUI_Object*)tw)->height;
  x=((GUI_Object*)tw)->x;
  y=((GUI_Object*)tw)->y;
  gtk_window_set_default_size(GTK_WINDOW(tw->gui_obj.window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(tw->gui_obj.window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(tw->gui_obj.window),tw->gui_obj.name,"Gpsim");


  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), tw);

  scrolled_window=gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), trace_clist);
  
  gtk_widget_show(trace_clist);
  gtk_widget_show(scrolled_window);

  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  ///////////////////////////////////////////////////
  ///////////////////////////////////////////////////



  char_width = gdk_string_width (normal_style->font,"9");
  column_width = 3 * char_width + 6;

  gtk_signal_connect_after(GTK_OBJECT(tw->gui_obj.window), "configure_event",
  			   GTK_SIGNAL_FUNC(gui_object_configure_event),tw);



  gtk_widget_show (window);

  if(!tw->trace_map) { 
    tw->trace_map = (struct TraceMapping *)malloc(MAXTRACES * sizeof(struct TraceMapping));
    
    for(i=0; i<MAXTRACES; i++) {
      tw->trace_map[i].cycle = 0;
      tw->trace_map[i].simulation_trace_index = 0;
    }
    tw->trace_map_index = 0;
  }

  tw->gui_obj.enabled=1;
  tw->gui_obj.is_built=1;
  tw->last_cycle = 0;

  if(tw->processor)
      TraceWindow_new_processor(tw, ((GUI_Object*)tw)->gp);

  TraceWindow_update(tw);

  update_menu_item((GUI_Object*)tw);

  return 0;
}

int CreateTraceWindow(GUI_Processor *gp)
{
    int i;
  Trace_Window *trace_window;

  trace_window = (Trace_Window *)malloc(sizeof(Trace_Window));

  trace_window->gui_obj.gp = gp;
  trace_window->gui_obj.name = "trace";
  trace_window->gui_obj.window = NULL;
  trace_window->gui_obj.wc = WC_data;
  trace_window->gui_obj.wt = WT_trace_window;
  trace_window->gui_obj.change_view = SourceBrowser_change_view;//change_view;
  trace_window->gui_obj.is_built = 0;
  trace_window->trace_map = NULL;
  gp->trace_window = trace_window;

  trace_window->trace_flags = 0;
  trace_window->processor=0;

  gp_add_window_to_list(gp, (GUI_Object *)trace_window);

  gui_object_get_config((GUI_Object*)trace_window);

  if(trace_window->gui_obj.enabled)
      BuildTraceWindow(trace_window);

  return 1;
}

#endif // HAVE_GUI
