#include <stdio.h>
#include <stdlib.h>
#include <sys/errno.h>
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

/*
unsigned int gpsim_get_opcode(unsigned int processor_id, unsigned int address);
char *gpsim_get_opcode_name(unsigned int processor_id, unsigned int address);
unsigned int gpsim_get_program_memory_size(unsigned int processor_id);
*/


#include "gui.h"

#include <assert.h>

#define PROGRAM_MEMORY_WINDOW_COLUMNS 4   //yuk
#define DEFAULT_ROWS  256

#define PROFILE_COLUMN  0
#define ADDRESS_COLUMN  1
#define OPCODE_COLUMN   2
#define MNEMONIC_COLUMN 3

static char *default_row_text[PROGRAM_MEMORY_WINDOW_COLUMNS];
static gint pc_row = 0;

static void 
clist_click_column (GtkCList *clist, gint column, gpointer data)
{

}

static void filter(char *clean, char *dirty, int max)
{

  int i=0,j;

  do {


    if(*dirty == '\t')

      for(j=0,*dirty++; j<8 && i%8; j++,i++)
	*clean++ = ' ';

    else if (*dirty <' ') 

      *dirty++;

    else

      *clean++ = *dirty++;



  }while(*dirty && ++i < max);

  *clean = 0;

}

void SourceBrowserOpcode_select_address(SourceBrowserOpcode_Window *sbow,int address)
{
  if(! ((GUI_Object*)sbow)->enabled)
      return;
  
    gtk_clist_unselect_all(GTK_CLIST(sbow->clist));
    gtk_clist_select_row(GTK_CLIST(sbow->clist),address,0);
    if(GTK_VISIBILITY_FULL != gtk_clist_row_is_visible (GTK_CLIST (sbow->clist),
							address))
    {
	gtk_clist_moveto (GTK_CLIST (sbow->clist), address, 0, .5, 0.0);
    }
}

void SourceBrowserOpcode_update_line( SourceBrowserOpcode_Window *sbow, int address, int row)
{
    if(!sbow) return;
    
  if(! ((GUI_Object*)sbow)->enabled)
      return;

  assert(sbow->sbw.gui_obj.wt == WT_opcode_source_window);

  if(address >= 0)
  {
      //      if(sbow->sbw.gui_obj.gp->p->program_memory[address]->isa() == instruction::BREAKPOINT_INSTRUCTION)
      if(gpsim_address_has_breakpoint(sbow->sbw.gui_obj.gp->pic_id,  address))
	  gtk_clist_set_row_style (GTK_CLIST (sbow->clist), row, breakpoint_line_number_style);
      else
	  gtk_clist_set_row_style (GTK_CLIST (sbow->clist), row, normal_style);

  }
}

void SourceBrowserOpcode_set_pc(SourceBrowserOpcode_Window *sbow, int address)
{
  char buffer[20];
  gint new_row,last_address, new_address;

  unsigned int pic_id = ((GUI_Object*)sbow)->gp->pic_id;
  
  if(! ((GUI_Object*)sbow)->enabled)
      return;
  
    last_address = sbow->current_row;
    new_row=gpsim_get_pc_value(pic_id);
    if(new_row != sbow->current_row)
    {
//	  gtk_clist_freeze (GTK_CLIST (sbow->clist));

	  SourceBrowserOpcode_update_line( sbow, last_address, sbow->current_row);
	  //gtk_clist_set_row_style (GTK_CLIST (sbw->clist), sbw->current_row, normal_style);
	  sbow->current_row = new_row;
	  gtk_clist_set_row_style (GTK_CLIST (sbow->clist), sbow->current_row, current_line_number_style);


//	  gtk_clist_thaw (GTK_CLIST (sbow->clist));
      }
      if(GTK_VISIBILITY_FULL != gtk_clist_row_is_visible (GTK_CLIST (sbow->clist),
							  sbow->current_row))
      {
	  gtk_clist_moveto (GTK_CLIST (sbow->clist), sbow->current_row, 0, .5, 0.0);
      }
}

static void pc_changed(struct cross_reference_to_gui *xref, int new_address)
{
    SourceBrowserOpcode_Window *sbow;

    sbow = (SourceBrowserOpcode_Window*)(xref->parent_window);

}

void SourceBrowserOpcode_new_program(SourceBrowserOpcode_Window *sbow, GUI_Processor *gp)
{
  char text_buffer[128];
  char clean_buffer[128];

  char *default_row_text[PROGRAM_MEMORY_WINDOW_COLUMNS];

  gint i;
  int pic_id;
  int pm_size;
  
  struct cross_reference_to_gui *cross_reference;

  if(sbow == NULL || gp == NULL)
      return;

  sbow->program=1;
  
  if(! ((GUI_Object*)sbow)->enabled)
      return;
  
  assert(sbow->sbw.gui_obj.wt==WT_opcode_source_window);

  pic_id = ((GUI_Object*)sbow)->gp->pic_id;
  sbow->sbw.gui_obj.gp = gp;
  gp->program_memory = (SourceBrowser_Window*)sbow;
  
  /* Now create a cross-reference link that the
   * simulator can use to send information back to the gui
   */
  cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
  cross_reference->parent_window_type =   WT_opcode_source_window;
  cross_reference->parent_window = (gpointer) sbow;
  cross_reference->data = (gpointer) NULL;
  cross_reference->update = pc_changed;
  cross_reference->remove = NULL;
  gpsim_assign_pc_xref(pic_id, cross_reference);

  gtk_clist_freeze (GTK_CLIST (sbow->clist));

  pm_size = gpsim_get_program_memory_size(sbow->sbw.gui_obj.gp->pic_id);

  for(i=0; i < pm_size; i++)
  {
      sprintf(text_buffer, "0x%04X", gpsim_get_opcode(sbow->sbw.gui_obj.gp->pic_id,i));
      gtk_clist_set_text (GTK_CLIST (sbow->clist), i, OPCODE_COLUMN, text_buffer);

      filter(clean_buffer, gpsim_get_opcode_name( sbow->sbw.gui_obj.gp->pic_id, i,text_buffer), 128);

      
      gtk_clist_set_text (GTK_CLIST (sbow->clist),
			  i, MNEMONIC_COLUMN, 
			  clean_buffer);
    }
  sbow->current_row = gpsim_get_pc_value(sbow->sbw.gui_obj.gp->pic_id);
  gtk_clist_set_row_style (GTK_CLIST (sbow->clist), 0, current_line_number_style);

  gtk_clist_thaw (GTK_CLIST (sbow->clist));

}

void SourceBrowserOpcode_new_processor(SourceBrowserOpcode_Window *sbow, GUI_Processor *gp)
{
  char text_buffer[128];
  char *default_row_text[PROGRAM_MEMORY_WINDOW_COLUMNS];

  gint i, pm_size;

  if(sbow == NULL || gp == NULL)
    return;

  sbow->processor=1;
  
  if(! ((GUI_Object*)sbow)->enabled)
      return;
  
  assert(sbow->sbw.gui_obj.wt==WT_opcode_source_window);

  pm_size = gpsim_get_program_memory_size(sbow->sbw.gui_obj.gp->pic_id);

  if(pm_size > sbow->clist_rows)
    {
	gtk_clist_freeze (GTK_CLIST (sbow->clist));
	
      for(i=0; i < PROGRAM_MEMORY_WINDOW_COLUMNS; i++)
	default_row_text[i] = NULL;
      default_row_text[ADDRESS_COLUMN] = text_buffer;


      for(i=sbow->clist_rows; i < pm_size; i++)
	{
	  sprintf (default_row_text[ADDRESS_COLUMN], "0x%04X", sbow->clist_rows++);
	  gtk_clist_append (GTK_CLIST (sbow->clist), default_row_text);
	  gtk_clist_set_row_style (GTK_CLIST (sbow->clist), i, normal_style);
	}

      for(i=0; i < pm_size; i++)
      {
	  sprintf(text_buffer, "0x%04X", gpsim_get_opcode(sbow->sbw.gui_obj.gp->pic_id  ,i));

	  gtk_clist_set_text (GTK_CLIST (sbow->clist), i, OPCODE_COLUMN, text_buffer);
	}

      gtk_clist_thaw (GTK_CLIST (sbow->clist));
    }

  SourceBrowserOpcode_new_program(sbow, gp);

// How to make this work? FIXME.
//  SourceBrowserOpcode_set_pc(sbow, gpsim_get_pc_value(sbow->sbw.gui_obj.gp->pic_id));
}

void BuildSourceBrowserOpcodeWindow(SourceBrowserOpcode_Window *sbow)
{
  GtkWidget *window;
  GtkWidget *vbox;
  GtkWidget *hbox;
  static GtkWidget *clist;
  GtkWidget *button;
  GtkWidget *separator;
  GtkWidget *scrolled_win;
  GtkWidget *check;

  GtkWidget *undo_button;
  GtkWidget *label;
  
  gint i;
  char address[16];

  int x,y,width,height;
  
  sbow->clist_rows = 0;

  CreateSBW((SourceBrowser_Window*)sbow);

  gtk_window_set_title (GTK_WINDOW (sbow->sbw.gui_obj.window), "Disassembly");
  
  
  width=((GUI_Object*)sbow)->width;
  height=((GUI_Object*)sbow)->height;
  x=((GUI_Object*)sbow)->x;
  y=((GUI_Object*)sbow)->y;
  gtk_window_set_default_size(GTK_WINDOW(sbow->sbw.gui_obj.window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(sbow->sbw.gui_obj.window),x,y);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show(scrolled_win);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);

  /* create GtkCList here so we have a pointer to throw at the 
   * button callbacks -- more is done with it later */
  clist = gtk_clist_new_with_titles (sbow->columns, sbow->column_titles);
  gtk_widget_show(clist);
  sbow->clist = clist;
  gtk_container_add (GTK_CONTAINER (scrolled_win), clist);
  gtk_signal_connect (GTK_OBJECT (clist), "click_column",
		      (GtkSignalFunc) clist_click_column, NULL);

  gtk_box_pack_start (GTK_BOX (sbow->sbw.vbox), scrolled_win, TRUE, TRUE, 0);
  gtk_clist_set_row_height (GTK_CLIST (clist), 18);
  gtk_widget_set_usize (clist, 300, 100);
  gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_EXTENDED);

  for (i = 0; i < sbow->columns; i++)
    {
      gtk_clist_set_column_width (GTK_CLIST (clist), i, 80);

      gtk_clist_set_column_auto_resize (GTK_CLIST (clist), i, FALSE);
      gtk_clist_set_column_justification (GTK_CLIST (clist), i,
					  GTK_JUSTIFY_LEFT);
      // %%% FIX ME
      //   Hide the profile column for now...
      if(i == 0)
	gtk_clist_set_column_visibility (GTK_CLIST (clist), i, FALSE);
    }


  for (i = 0; i < sbow->columns; i++)
    default_row_text[i] = NULL;


  default_row_text[ADDRESS_COLUMN] = address;

  for (i = 0; i < DEFAULT_ROWS; i++)
    {
      sprintf (default_row_text[ADDRESS_COLUMN], "0x%04X", sbow->clist_rows++);
      gtk_clist_append (GTK_CLIST (sbow->clist), default_row_text);
      gtk_clist_set_row_style (GTK_CLIST (sbow->clist), i, normal_style);

    }
  
  gtk_signal_connect_after(GTK_OBJECT(sbow->sbw.gui_obj.window), "configure_event",
			   GTK_SIGNAL_FUNC(gui_object_configure_event),sbow);
  
//  if (((GUI_Object*)sbow)->visible)
      gtk_widget_show(sbow->sbw.gui_obj.window);
/*  else
  {
	puts("\n\n\n\n\n\nadfhkadfhkljrewttrtq");

	sbow->clist_rows = 0;
      gtk_widget_destroy (sbow->sbw.gui_obj.window);
      }*/

      sbow->sbw.gui_obj.enabled=1;


      if(sbow->processor)
	  SourceBrowserOpcode_new_processor(sbow, sbow->sbw.gui_obj.gp);
  if(sbow->program)
      SourceBrowserOpcode_new_program(sbow, sbow->sbw.gui_obj.gp);
  
}

int CreateSourceBrowserOpcodeWindow(GUI_Processor *gp)
{

  static char *titles[] =
  {
    "profile", "address", "opcode", "instruction"
  };


  gint i;
  static SourceBrowserOpcode_Window *sbow;
  
//  SourceBrowserOpcode_Data *sbow;

  sbow = (SourceBrowserOpcode_Window *) malloc(sizeof(SourceBrowserOpcode_Window));
  sbow->sbw.gui_obj.gp = NULL;
  sbow->sbw.gui_obj.window = NULL;

  sbow->column_titles = titles;
  sbow->columns = 4;

  sbow->sbw.gui_obj.gp = gp;
  gp->program_memory = (SourceBrowser_Window*)sbow;
  sbow->sbw.gui_obj.name = "disassembly";
  sbow->sbw.gui_obj.wc = WC_source;
  sbow->sbw.gui_obj.wt = WT_opcode_source_window;

  sbow->sbw.gui_obj.change_view = SourceBrowser_change_view;
  
  sbow->clist_rows = 0;

  sbow->processor=0;
  sbow->program=0;
  
    gp_add_window_to_list(gp, (GUI_Object *)sbow);

    gui_object_get_config((GUI_Object*)sbow);

    if(sbow->sbw.gui_obj.enabled)
	BuildSourceBrowserOpcodeWindow(sbow);
  
    return 1;
}
