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

#include <gtkextra/gtksheet.h>

#include "../src/interface.h"

#include "gui.h"


#define MAXTRACES  100
#define MAXCOLS    3

typedef enum {
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_READ_VALUE,
    MENU_BREAK_WRITE_VALUE,
    MENU_ADD_WATCH,
} menu_id;


guint64 row_to_cycle[MAXTRACES];

/*****************************************************************
 * xref_update
 *
 * This is called by the simulator when it has been determined that
 * that the trace buffer has changed and needs to be updated
 */
static void xref_update(struct cross_reference_to_gui *xref, int new_value)
{
  //struct watch_entry *entry;
    Trace_Window *tw;

    if(xref == NULL)
    {
	printf("Warning gui_trace.c: xref_update: xref=%x\n",(unsigned int)xref);
	return;
    }

    //entry = (struct watch_entry*) xref->data;
    tw  = (Trace_Window *) (xref->parent_window);

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
  GtkSheet *sheet;
  char buffer[50];
  guint64 cycle;

  if(  (tw == NULL)  || (!((GUI_Object*)tw)->enabled))
    return;

  // Get the pointer to the `gui processor' structure
  gp = ((GUI_Object*)tw)->gp;

  if(gp==NULL || gp->pic_id==0)
  {
      puts("Warning gp or gp->pic_id == NULL in TraceWindow_update");
      return;
  }


  // Get a convenient pointer to the gtk_sheet that the trace is in.
  sheet=GTK_SHEET(tw->trace_sheet);

  gtk_sheet_freeze(sheet);

  cycle = gpsim_get_cycles(gp->pic_id);

  if(tw->last_cycle +(MAXTRACES/4) <  cycle) {
    // redraw the whole thing

    
    gtk_sheet_set_cell(sheet,
		       sheet->maxrow,
		       0,  // column
		       GTK_JUSTIFY_RIGHT,"need to do a big update");
  } else {

    // update the most recent cycles
    
    gtk_sheet_delete_rows(sheet,0,1);

    gtk_sheet_add_row(sheet,1);
    sprintf(buffer,"%016x", cycle);

    //gtk_sheet_row_button_add_label(sheet, sheet->maxrow, buffer);
    //gtk_sheet_set_row_title(sheet, sheet->maxrow, buffer);

    gtk_sheet_set_cell(sheet,
		       sheet->maxrow,
		       0,  // column
		       GTK_JUSTIFY_RIGHT,buffer);


  }

  tw->last_cycle = cycle;
  gtk_sheet_thaw(sheet);
}


/*****************************************************************
 * TraceWindow_new_processor
 *
 * 
 */

void TraceWindow_new_processor(Trace_Window *tw, GUI_Processor *gp)
{


#define NAME_SIZE 32

    gint i,j,reg_number, border_mask, border_width;
    GtkSheet *sheet;
    struct cross_reference_to_gui *cross_reference;
    gboolean row_created;
    GtkSheetRange range;
    int pic_id;
    char row_label[50];

    if(tw == NULL || gp == NULL)
	return;

    tw->processor=1;
    
    if( !((GUI_Object*)tw)->enabled)
	return;
    
    tw->gui_obj.gp = gp;
    pic_id = gp->pic_id;

    
    sheet=GTK_SHEET(tw->trace_sheet);

    gtk_sheet_freeze(sheet);

/*    
    for(i=0; i<MAXTRACES; i++) {
      row_to_cycle[i] = 0;
      gtk_sheet_add_row(sheet,1);
      sprintf(row_label,"%x0",i);
      gtk_sheet_row_button_add_label(sheet, 0, row_label);
      gtk_sheet_set_row_title(sheet, 0, row_label);
    }
*/
/*
    if(j < sheet->maxrow)
	gtk_sheet_delete_rows(sheet,j,sheet->maxrow-j);
*/

    
    range.row0=0;
    range.rowi=sheet->maxrow;
    range.col0=0;
    range.coli=sheet->maxcol;

    gtk_sheet_range_set_font(sheet, &range, normal_style->font);

    border_mask = GTK_SHEET_RIGHT_BORDER |
	GTK_SHEET_LEFT_BORDER |
	GTK_SHEET_BOTTOM_BORDER |
	GTK_SHEET_TOP_BORDER;

    border_width = 1;

    gtk_sheet_range_set_border(sheet, &range, border_mask, border_width, 0);

    border_mask = GTK_SHEET_LEFT_BORDER;
    border_width = 3;

    range.col0=REGISTERS_PER_ROW;
    range.coli=REGISTERS_PER_ROW;

    gtk_sheet_range_set_border(sheet, &range, border_mask, border_width, 0);

    // set values in the sheet
    //    RegWindow_update(rw);

    gtk_sheet_thaw(sheet);


    cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
    cross_reference->parent_window_type =  WT_trace_window;
    cross_reference->parent_window = (gpointer) tw;
    cross_reference->data = NULL;
    cross_reference->update = xref_update;
    cross_reference->remove = NULL;
    //gpsim_assign_trace_xref(pic_id, (gpointer) cross_reference);

}

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Register_Window *rw)
{
//    puts("Delete");
    ((GUI_Object *)rw)->change_view((GUI_Object*)rw,VIEW_HIDE);
    return TRUE;
}

int
BuildTraceWindow(Trace_Window *tw)
{
  GtkWidget *window;
  GtkWidget *trace_sheet;
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

  trace_sheet=gtk_sheet_new(MAXTRACES,MAXCOLS-1,"gpsim Trace Viewer");
  gtk_window_set_title(GTK_WINDOW(window), "trace viewer");
  // Add a status bar
  //StatusBar_create(main_vbox,gp->status_bar);

  GTK_WIDGET_UNSET_FLAGS(trace_sheet,GTK_CAN_DEFAULT);
    
  tw->trace_sheet = GTK_SHEET(trace_sheet);

  /* create popupmenu */
  //rw->popup_menu=build_menu(rw);

  //build_entry_bar(main_vbox, rw);

  width=((GUI_Object*)tw)->width;
  height=((GUI_Object*)tw)->height;
  x=((GUI_Object*)tw)->x;
  y=((GUI_Object*)tw)->y;
  gtk_window_set_default_size(GTK_WINDOW(tw->gui_obj.window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(tw->gui_obj.window),x,y);


  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), tw);

  scrolled_window=gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), trace_sheet);
  
  GTK_SHEET_SET_FLAGS(trace_sheet, GTK_SHEET_CLIP_TEXT);

  gtk_widget_show(trace_sheet);

  gtk_widget_show(scrolled_window);

  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);

/*
  gtk_signal_connect(GTK_OBJECT(gtk_sheet_get_entry(GTK_SHEET(trace_sheet))),
		     "changed", (GtkSignalFunc)show_entry, tw);

  gtk_signal_connect(GTK_OBJECT(trace_sheet),
		     "activate", (GtkSignalFunc)activate_sheet_cell,
		     (gpointer) tw);

  gtk_signal_connect(GTK_OBJECT(tw->entry),
		     "changed", (GtkSignalFunc)show_sheet_entry, tw);

  gtk_signal_connect(GTK_OBJECT(tw->entry),
		     "activate", (GtkSignalFunc)activate_sheet_entry,
		     tw);
*/

//  gtk_widget_realize(window);


  char_width = gdk_string_width (normal_style->font,"9");
  column_width = 3 * char_width + 6;

  gtk_sheet_column_button_add_label(tw->trace_sheet, 0, "cycle");
  gtk_sheet_column_button_add_label(tw->trace_sheet, 1, "trace");
  //gtk_sheet_set_column_title(tw->trace_sheet, 1, "cycle");
  //gtk_sheet_set_column_title(tw->trace_sheet, 1, "trace");
  gtk_sheet_set_column_width (tw->trace_sheet, 0, char_width*16);
  gtk_sheet_set_column_width (tw->trace_sheet, 1, char_width*50);
  gtk_sheet_set_row_titles_width(tw->trace_sheet, char_width*16);

  //gtk_sheet_set_row_titles_width(rw->register_sheet, column_width);

/*	
	gtk_signal_connect(GTK_OBJECT(rw->register_sheet),
			   "key_press_event",
			   (GtkSignalFunc) clipboard_handler, 
			   NULL);

	gtk_signal_connect(GTK_OBJECT(rw->register_sheet),
			   "resize_range",
			   (GtkSignalFunc) resize_handler, 
			   rw);

	gtk_signal_connect(GTK_OBJECT(rw->register_sheet),
			   "move_range",
			   (GtkSignalFunc) move_handler, 
			   rw);
	
	gtk_signal_connect(GTK_OBJECT(rw->register_sheet),
			   "button_press_event",
			   (GtkSignalFunc) do_popup, 
			   rw);

	gtk_signal_connect(GTK_OBJECT(rw->register_sheet),
			   "set_cell",
			   (GtkSignalFunc) set_cell,
			   rw);
	
*/

  gtk_signal_connect_after(GTK_OBJECT(tw->gui_obj.window), "configure_event",
  			   GTK_SIGNAL_FUNC(gui_object_configure_event),tw);



  gtk_widget_show (window);

  tw->gui_obj.enabled=1;
  tw->gui_obj.is_built=1;
  tw->last_cycle = 0;

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


  gp->trace_window = trace_window;


  trace_window->processor=0;

  gp_add_window_to_list(gp, (GUI_Object *)trace_window);

  gui_object_get_config((GUI_Object*)trace_window);

  if(trace_window->gui_obj.enabled)
      BuildTraceWindow(trace_window);

  return 1;
}

#endif // HAVE_GUI
