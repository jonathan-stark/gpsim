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

#define EXPERIMENTAL

#include "gui.h"

#include <assert.h>

#define PROGRAM_MEMORY_WINDOW_COLUMNS 4   //yuk
#define DEFAULT_ROWS  256

#define PROFILE_COLUMN  0
#define ADDRESS_COLUMN  1
#define OPCODE_COLUMN   2
#define MNEMONIC_COLUMN 3

static char profile_buffer[128];
static char address_buffer[128];
static char opcode_buffer[128];
static char mnemonic_buffer[128];
static char *row_text[PROGRAM_MEMORY_WINDOW_COLUMNS]={
    profile_buffer,address_buffer,opcode_buffer,mnemonic_buffer
};

static GtkStyle *row_default_style;

static void filter(char *clean, char *dirty, int max)
{

    int i=0,j;

    do {


	if(*dirty == '\t')
	    for(j=0,dirty++; j<8 && i%8; j++,i++)
		*clean++ = ' ';
	else if (*dirty <' ')
	    dirty++;
	else
	    *clean++ = *dirty++;



  }while(*dirty && ++i < max);

  *clean = 0;

}

static void update(SourceBrowserOpcode_Window *sbow, int address)
{
    int row=address/16;
    int column=address%16;
    char buf[128];
    char buf2[128];
    GtkEntry *sheet_entry;
    GtkSheet *sheet=sbow->sheet;
    char *text;
    
    if(gpsim_address_has_breakpoint(sbow->sbw.gui_obj.gp->pic_id,  address))
	gtk_clist_set_row_style (GTK_CLIST (sbow->clist), address, breakpoint_line_number_style);
    else
	gtk_clist_set_row_style (GTK_CLIST (sbow->clist), address, row_default_style);
    
    sprintf (row_text[ADDRESS_COLUMN], "0x%04X", address);
    sprintf(row_text[OPCODE_COLUMN], "0x%04X", gpsim_get_opcode(sbow->sbw.gui_obj.gp->pic_id  ,address));
    filter(row_text[MNEMONIC_COLUMN], gpsim_get_opcode_name( sbow->sbw.gui_obj.gp->pic_id, address,buf), 128);
      
    gtk_clist_set_text (GTK_CLIST (sbow->clist), address, OPCODE_COLUMN, row_text[OPCODE_COLUMN]);
    gtk_clist_set_text (GTK_CLIST (sbow->clist), address, MNEMONIC_COLUMN, row_text[MNEMONIC_COLUMN]);

#ifdef EXPERIMENTAL
    gtk_sheet_set_cell(GTK_SHEET(sbow->sheet),
		       row,column,
		       GTK_JUSTIFY_RIGHT,row_text[OPCODE_COLUMN]);


#endif
//      gtk_clist_append (GTK_CLIST (sbow->clist), row_text);
//      gtk_clist_set_row_style (GTK_CLIST (sbow->clist), i, row_default_style);
      
/*      gtk_clist_set_text (GTK_CLIST (sbow->clist),
			  i, MNEMONIC_COLUMN, 
			  clean_buffer);*/

    
  sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));
    
  buf2[0] = 0;
  if(((GUI_Object*)sbow)->gp)
  {
      if(column < REGISTERS_PER_ROW) {
	  if(((GUI_Object*)sbow)->gp->pic_id != 0)
	  {
	      
		  filter(buf,gpsim_get_opcode_name( sbow->sbw.gui_obj.gp->pic_id, row*16+column,buf2),sizeof(buf));
//	      if(n==NULL)
//		  n="INVALID REGISTER";
	  }
	  else
//	      n = "00"; // FIXME

	      strncpy(buf,"hmm",4);
      }
      else
	  sprintf(buf,"  ascii  ");
  }

  gtk_label_set(GTK_LABEL(sbow->label), buf);

  gtk_entry_set_max_length(GTK_ENTRY(sbow->entry),
	GTK_ENTRY(sheet_entry)->text_max_length);

  
  gtk_entry_set_text(GTK_ENTRY(sbow->entry), row_text[OPCODE_COLUMN]);
}

#ifdef EXPERIMENTAL

static unsigned long get_number_in_string(char *number_string)
{
  unsigned long retval = 0;
  char *bad_position;
  int current_base = 16;
  
  if(number_string==NULL)
  {
      printf("Warning get_number_in_string(%x)\n",(unsigned int)number_string);
      errno = EINVAL;
      return -1;
  }


  errno = 0;

  retval = strtoul(number_string, &bad_position, current_base);

  if( strlen(bad_position) ) 
    errno = EINVAL;  /* string contains an invalid number */

  /*
  if(retval > 255)
    errno = ERANGE;
  */

  return(retval);
}


// when a new cell is selected, we write changes in
// previously selected cell to gpsim
static void
parse_numbers(GtkWidget *widget, int row, int col, SourceBrowserOpcode_Window *sbow)
{
  GtkSheet *sheet;
  gchar *text;
  int justification,n=0;

  GUI_Processor *gp;
  
  sheet=GTK_SHEET(widget);
  
  if(widget==NULL ||
     row>sheet->maxrow || row<0 ||
     col>sheet->maxcol || col<0 || sbow==NULL)
  {
      printf("Warning parse_numbers(%x,%x,%x,%x)\n",(unsigned int)widget,row,col,(unsigned int)sbow);
      return;
  }

  gp = ((GUI_Object*)sbow)->gp;
  
  printf ("parse_numbers %d %d\n", row, col);
  

  justification=GTK_JUSTIFY_RIGHT;

  if(col < REGISTERS_PER_ROW)
    {

      int reg = row*16+col;//rw->row_to_address[row] + col;
/*
      if( rw->row_to_address[row] == -1)
      {
	  puts("Warning row_to_address[row] == -1 in parse_numbers");
	  return;
      }*/

      text = gtk_entry_get_text(GTK_ENTRY(sheet->sheet_entry));

      errno = 0;
      if(strlen(text)>0)
	n = get_number_in_string(text);
      else
	errno = ERANGE;

      if(errno != 0)
      {
	  n = gpsim_get_opcode(sbow->sbw.gui_obj.gp->pic_id,reg);
	  sbow->memory[reg] = -1;
      }

//      if(gpsim_get_register_name(gp->pic_id,rw->type, reg))
//      {
	  if(n != sbow->memory[reg])
	  {
	      printf("Writing new value, new %d, last %d\n",n,sbow->memory[reg]);
	      sbow->memory[reg]=n;
	      gpsim_put_opcode(gp->pic_id, reg, n);
//	      gpsim_put_register_value(gp->pic_id, rw->type, reg, n&0xff);
//	      update_ascii(rw,row);
	  }
//      }
    }
  else
      ; // ignore user changes in ascii column for right now
}

/* when the entry above the sheet is changed (typed a digit), we
   copy it to the cell entry */
static void
show_sheet_entry(GtkWidget *widget, SourceBrowserOpcode_Window *sbow)
{
 char *text;
 GtkSheet *sheet;
 GtkEntry *sheet_entry;

 int row,col;
 
 if(widget==NULL|| sbow==NULL)
  {
      printf("Warning show_sheet_entry(%x,%x)\n",(unsigned int)widget,(unsigned int)sbow);
      return;
  }

 if(!GTK_WIDGET_HAS_FOCUS(widget)) return;
 
 sheet=GTK_SHEET(sbow->sheet);
 sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));

 row=sheet->active_cell.row; col=sheet->active_cell.col;

// if(gpsim_get_register_name(gp->pic_id,sbow->type, sbow->row_to_address[row]+col))
// {
     if((text=gtk_entry_get_text (GTK_ENTRY(sbow->entry))))
	 gtk_entry_set_text(sheet_entry, text);
// }

}

/* when we have new data in the entry above the sheet, we
 copy the data to the cells/registers

 this don't get called when it is clicked
 in, only when we hit return
 */
static void
activate_sheet_entry(GtkWidget *widget, SourceBrowserOpcode_Window *sbow)
{
  GtkSheet *sheet;
//  GtkEntry *sheet_entry;

  gint row, col;
//  gint justification=GTK_JUSTIFY_RIGHT;

  if(widget==NULL|| sbow==NULL)
  {
      printf("Warning activate_sheet_entry(%x,%x)\n",(unsigned int)widget,(unsigned int)sbow);
      return;
  }

  puts("Activate sheet entry");
  
  sheet=GTK_SHEET(sbow->sheet);
  row=sheet->active_cell.row; col=sheet->active_cell.col;

  parse_numbers(GTK_WIDGET(sheet),sheet->active_cell.row,sheet->active_cell.col,sbow);
//  update_ascii(rw,row);
      
}

/*
 we get here when the entry in a cell is changed (typed a digit), we
 copy it to the entry above the sheet.
 */
static void
show_entry(GtkWidget *widget, SourceBrowserOpcode_Window *sbow)
{
 char *text; 
 GtkSheet *sheet;
 GtkWidget * sheet_entry;
  gint row, col;

  if(widget==NULL|| sbow==NULL)
  {
      printf("Warning show_entry(%x,%x)\n",(unsigned int)widget,(unsigned int)sbow);
      return;
  }
  
 if(!GTK_WIDGET_HAS_FOCUS(widget)) return;

 sheet=GTK_SHEET(sbow->sheet);
 sheet_entry = gtk_sheet_get_entry(sheet);

 row=sheet->active_cell.row; col=sheet->active_cell.col;
// if(gpsim_get_register_name(gp->pic_id,sbow->type, sbow->row_to_address[row]+col))
// {
     if((text=gtk_entry_get_text (GTK_ENTRY(sheet_entry))))
	 gtk_entry_set_text(GTK_ENTRY(sbow->entry), text);
// }

}

/* when a cell is activated, we set the label and entry above the sheet
 */
static gint
activate_sheet_cell(GtkWidget *widget, gint row, gint column, SourceBrowserOpcode_Window *sbow)
{
    GtkSheet *sheet;
    GtkEntry *sheet_entry;
    char cell[100], buf[100];
    char *text;
    GtkSheetCellAttr attributes;
  
    sheet=GTK_SHEET(sbow->sheet);

    if(widget==NULL || row>sheet->maxrow || row<0||
       column>sheet->maxcol || column<0 || sbow==NULL)
    {
	printf("Warning activate_sheet_cell(%x,%x,%x,%x)\n",(unsigned int)widget,row,column,(unsigned int)sbow);
	return 0;
    }


    puts("Activate sheet cell");
    
    update(sbow,row*16+column);
    

    
  gtk_sheet_get_attributes(sheet,sheet->active_cell.row,
			   sheet->active_cell.col, &attributes);

  gtk_entry_set_editable(GTK_ENTRY(sbow->entry), attributes.is_editable);


  gtk_sheet_range_set_justification(sheet, sheet->range, GTK_JUSTIFY_RIGHT);

  
  return TRUE;
}


#endif

static void 
clist_click_column (GtkCList *clist, gint column, gpointer data)
{

}

void SourceBrowserOpcode_select_address(SourceBrowserOpcode_Window *sbow, int address)
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
    char buf[128];
    
    if(!sbow) return;
    
  if(! ((GUI_Object*)sbow)->enabled)
      return;

  assert(sbow->sbw.gui_obj.wt == WT_opcode_source_window);

  if(address >= 0)
  {
      update(sbow,address);

  }
}

void SourceBrowserOpcode_set_pc(SourceBrowserOpcode_Window *sbow, int address)
{
  gint new_row,last_address;

  unsigned int pic_id = ((GUI_Object*)sbow)->gp->pic_id;
  
  if(! ((GUI_Object*)sbow)->enabled)
      return;
  
    last_address = sbow->current_row;
    new_row=gpsim_get_pc_value(pic_id);
    if(new_row != sbow->current_row)
    {
//	  gtk_clist_freeze (GTK_CLIST (sbow->clist));

	  SourceBrowserOpcode_update_line( sbow, last_address, sbow->current_row);
	  //gtk_clist_set_row_style (GTK_CLIST (sbw->clist), sbw->current_row, row_default_style);
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
    char buf[128];

  gint i;
  int pic_id;
  int pm_size;
  
  struct cross_reference_to_gui *cross_reference;

  if(sbow == NULL || gp == NULL)
      return;

  puts("New program");
  
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

  gtk_clist_clear(GTK_CLIST(sbow->clist));
  
  pm_size = gpsim_get_program_memory_size(sbow->sbw.gui_obj.gp->pic_id);

#ifdef EXPERIMENTAL
  gtk_sheet_freeze(GTK_SHEET(sbow->sheet));
#endif
  
  for(i=0; i < pm_size; i++)
  {
      sprintf (row_text[ADDRESS_COLUMN], "0x%04X", i);
      sprintf(row_text[OPCODE_COLUMN], "0x%04X", gpsim_get_opcode(sbow->sbw.gui_obj.gp->pic_id  ,i));
      filter(row_text[MNEMONIC_COLUMN], gpsim_get_opcode_name( sbow->sbw.gui_obj.gp->pic_id, i,buf), 128);
      
//      sprintf(text_buffer, "0x%04X", gpsim_get_opcode(sbow->sbw.gui_obj.gp->pic_id,i));
//      gtk_clist_set_text (GTK_CLIST (sbow->clist), i, OPCODE_COLUMN, text_buffer);

#ifdef EXPERIMENTAL
      gtk_sheet_set_cell(GTK_SHEET(sbow->sheet),
			 i/16,
			 i%16,
			 GTK_JUSTIFY_RIGHT,row_text[OPCODE_COLUMN]);


#endif
      gtk_clist_append (GTK_CLIST (sbow->clist), row_text);
//      gtk_clist_set_row_style (GTK_CLIST (sbow->clist), i, row_default_style);
      
/*      gtk_clist_set_text (GTK_CLIST (sbow->clist),
			  i, MNEMONIC_COLUMN, 
			  clean_buffer);*/

    }
  sbow->current_row = gpsim_get_pc_value(sbow->sbw.gui_obj.gp->pic_id);
  gtk_clist_set_row_style (GTK_CLIST (sbow->clist), 0, current_line_number_style);

  gtk_clist_thaw (GTK_CLIST (sbow->clist));
#ifdef EXPERIMENTAL
  gtk_sheet_thaw(GTK_SHEET(sbow->sheet));
#endif

}

void SourceBrowserOpcode_new_processor(SourceBrowserOpcode_Window *sbow, GUI_Processor *gp)
{

    gint i, pm_size;

    if(sbow == NULL || gp == NULL)
	return;

    puts("New processor");

    sbow->processor=1;

    if(! ((GUI_Object*)sbow)->enabled)
	return;

    assert(sbow->sbw.gui_obj.wt==WT_opcode_source_window);

    pm_size = gpsim_get_program_memory_size(sbow->sbw.gui_obj.gp->pic_id);

    if(sbow->memory!=NULL)
	free(sbow->memory);
    sbow->memory=malloc(pm_size*sizeof(*sbow->memory));

    for(i=0;i<pm_size;i++)
	sbow->memory[i]=-1;


    gtk_clist_freeze (GTK_CLIST (sbow->clist));

    gtk_clist_clear(GTK_CLIST(sbow->clist));
    
    //    for(i=0; i < PROGRAM_MEMORY_WINDOW_COLUMNS; i++)
    //row_text[i] = NULL;

//    sbow->clist_rows=0;
    for(i=0; i < pm_size; i++)
    {
	  sprintf (row_text[ADDRESS_COLUMN], "0x%04X", i);
	  sprintf(row_text[OPCODE_COLUMN], "0x%04X", gpsim_get_opcode(sbow->sbw.gui_obj.gp->pic_id  ,i));
	  gtk_clist_append (GTK_CLIST (sbow->clist), row_text);
//	  gtk_clist_set_row_style (GTK_CLIST (sbow->clist), i, row_default_style);
	}

/*      for(i=0; i < pm_size; i++)
      {

	  gtk_clist_set_text (GTK_CLIST (sbow->clist), i, OPCODE_COLUMN, text_buffer);
	}*/

      gtk_clist_thaw (GTK_CLIST (sbow->clist));

#ifdef EXPERIMENTAL
  // Update sheet
  gtk_sheet_freeze(GTK_SHEET(sbow->sheet));
  for(i=0;i<pm_size;i+=16)
  {
      char row_label[100];
      if(GTK_SHEET(sbow->sheet)->maxrow<i/16)
      {
	  gtk_sheet_add_row(GTK_SHEET(sbow->sheet),1);
      }

      sprintf(row_label,"%x0",i/16);
      gtk_sheet_row_button_add_label(GTK_SHEET(sbow->sheet), i/16, row_label);
      gtk_sheet_set_row_title(GTK_SHEET(sbow->sheet), i/16, row_label);
      
  }
    if(i/16 < GTK_SHEET(sbow->sheet)->maxrow)
    {
//      printf(">>>>>>>%d %d %d\n",j,sheet->maxrow,sheet->maxrow+1-j);
	gtk_sheet_delete_rows(GTK_SHEET(sbow->sheet),i/16,GTK_SHEET(sbow->sheet)->maxrow-i/16);
    }
  
  gtk_sheet_thaw(GTK_SHEET(sbow->sheet));
#endif
  SourceBrowserOpcode_new_program(sbow, gp);

// How to make this work? FIXME.
//  SourceBrowserOpcode_set_pc(sbow, gpsim_get_pc_value(sbow->sbw.gui_obj.gp->pic_id));
}

void BuildSourceBrowserOpcodeWindow(SourceBrowserOpcode_Window *sbow)
{
    static GtkWidget *clist;
    GtkWidget *label;
    GtkWidget *entry;
    GtkWidget *vbox;
    GtkWidget *hbox;
  GtkWidget *scrolled_win;
  GtkRequisition request; 
  
	gchar name[10];
	gint column_width,char_width;
  gint i;
  char address[16];

  int x,y,width,height;
  
  CreateSBW((SourceBrowser_Window*)sbow);


    gtk_window_set_title (GTK_WINDOW (sbow->sbw.gui_obj.window), "Disassembly");




    sbow->notebook = gtk_notebook_new();
    gtk_widget_show(sbow->notebook);

    gtk_box_pack_start (GTK_BOX (sbow->sbw.vbox), sbow->notebook, TRUE, TRUE, 0);


  
  width=((GUI_Object*)sbow)->width;
  height=((GUI_Object*)sbow)->height;
  x=((GUI_Object*)sbow)->x;
  y=((GUI_Object*)sbow)->y;
  gtk_window_set_default_size(GTK_WINDOW(sbow->sbw.gui_obj.window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(sbow->sbw.gui_obj.window),x,y);


  
  /////////////////////////////////////////////////////////////////
  // create clist
  /////////////////////////////////////////////////////////////////
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


  label=gtk_label_new("clist page");
  gtk_notebook_append_page(GTK_NOTEBOOK(sbow->notebook),scrolled_win,label);

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


  for (i = 0; i < DEFAULT_ROWS; i++)
    {
      sprintf (row_text[ADDRESS_COLUMN], "0x%04X", i);
      gtk_clist_append (GTK_CLIST (sbow->clist), row_text);
      gtk_clist_set_row_style (GTK_CLIST (sbow->clist), i, row_default_style);

    }

  row_default_style=gtk_clist_get_row_style(GTK_CLIST (clist), 0);


#ifdef EXPERIMENTAL

  /////////////////////////////////////////////////////////////////
  // create sheet
  /////////////////////////////////////////////////////////////////
  vbox=gtk_vbox_new(FALSE,1);
  gtk_widget_show(vbox);

  // Create entry bar
  hbox=gtk_hbox_new(FALSE,1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
  
  sbow->label=gtk_label_new("Test");
  gtk_widget_size_request(sbow->label, &request);
  gtk_widget_set_usize(sbow->label, 160, request.height);
  gtk_widget_show(sbow->label);
  gtk_box_pack_start(GTK_BOX(hbox), sbow->label, FALSE, TRUE, 0);

  sbow->entry=gtk_entry_new();
  gtk_widget_show(sbow->entry);
  gtk_box_pack_start(GTK_BOX(hbox), sbow->entry, TRUE, TRUE, 0);
  
  // Create sheet
  scrolled_win=gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolled_win);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0);
  
  sbow->sheet=gtk_sheet_new(1,17,"where do this string go?");
  gtk_widget_show(sbow->sheet);
  gtk_container_add(GTK_CONTAINER(scrolled_win), sbow->sheet);
  
  label=gtk_label_new("sheet page");
  gtk_notebook_append_page(GTK_NOTEBOOK(sbow->notebook),vbox,label);

  char_width = gdk_string_width (normal_style->font,"9");
  column_width = 7 * char_width + 6;
  for(i=0; i<GTK_SHEET(sbow->sheet)->maxcol; i++){
      //sprintf(name,"0x%02x",i);
      sprintf(name,"%02x",i);
      gtk_sheet_column_button_add_label(GTK_SHEET(sbow->sheet), i, name);
      gtk_sheet_set_column_title(GTK_SHEET(sbow->sheet), i, name);
      gtk_sheet_set_column_width (GTK_SHEET(sbow->sheet), i, column_width);
  }
  sprintf(name,"ASCII");
  gtk_sheet_column_button_add_label(GTK_SHEET(sbow->sheet), i, name);
  gtk_sheet_set_column_title(GTK_SHEET(sbow->sheet), i, name);
  gtk_sheet_set_column_width (GTK_SHEET(sbow->sheet), i, REGISTERS_PER_ROW*char_width + 6);
  gtk_sheet_set_row_titles_width(GTK_SHEET(sbow->sheet), column_width);

  gtk_signal_connect(GTK_OBJECT(gtk_sheet_get_entry(GTK_SHEET(sbow->sheet))),
		     "changed", (GtkSignalFunc)show_entry, sbow);

  gtk_signal_connect(GTK_OBJECT(sbow->sheet),
		     "activate", (GtkSignalFunc)activate_sheet_cell,
		     (gpointer) sbow);

  gtk_signal_connect(GTK_OBJECT(sbow->entry),
		     "changed", (GtkSignalFunc)show_sheet_entry, sbow);

  gtk_signal_connect(GTK_OBJECT(sbow->entry),
		     "activate", (GtkSignalFunc)activate_sheet_entry,
		     sbow);
  gtk_signal_connect(GTK_OBJECT(sbow->sheet),
		     "set_cell",
		     (GtkSignalFunc) parse_numbers,
		     sbow);
  /////////////////////////////////////////////////////////////////



  
  gtk_widget_show(scrolled_win);
  gtk_widget_show(sbow->sheet);

#endif

  
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
  
  sbow->memory=NULL;

  sbow->processor=0;
  sbow->program=0;
  
    gp_add_window_to_list(gp, (GUI_Object *)sbow);

    gui_object_get_config((GUI_Object*)sbow);

    if(sbow->sbw.gui_obj.enabled)
	BuildSourceBrowserOpcodeWindow(sbow);
  
    return 1;
}
