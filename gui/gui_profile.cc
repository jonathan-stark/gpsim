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
#include <time.h>
#include <sys/errno.h>

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>
#include <math.h>

#include <assert.h>

#include "../src/interface.h"

#include "gui.h"

#include <gtkextra/gtkplot.h>
#include <gtkextra/gtkplotdata.h>
#include <gtkextra/gtkplotcanvas.h>
#include <gtkextra/gtkplotbar.h>
#include <gtkextra/gtkplotps.h>
#include <gtkextra/gtkplotprint.h>

#define PROFILE_COLUMNS    3
static char *profile_titles[PROFILE_COLUMNS]={"Address", "Cycles","Instruction"};

#define PROFILE_RANGE_COLUMNS    3
static char *profile_range_titles[PROFILE_RANGE_COLUMNS]={"Start address", "End address", "Cycles"};

#define PROFILE_REGISTER_COLUMNS    4
static char *profile_register_titles[PROFILE_REGISTER_COLUMNS]={"Address", "Register", "Read count", "Write count"};

#define PROFILE_EXESTATS_COLUMNS    9
static char *profile_exestats_titles[PROFILE_EXESTATS_COLUMNS]={"From address", "To address", "Executions", "Min", "Max",  "Median", "Average", "Std. Dev.", "Total"};

struct profile_entry {
    unsigned int pic_id;
    unsigned int address;
    struct cross_reference_to_gui *xref;
    guint64 last_count;
};

struct profile_range_entry {
    unsigned int pic_id;
    char startaddress_text[64];
    char endaddress_text[64];
    unsigned int startaddress;
    unsigned int endaddress;
    struct cross_reference_to_gui *xref;
    guint64 last_count;
};

struct profile_register_entry {
    unsigned int pic_id;
    unsigned int address;
    struct cross_reference_to_gui *xref;
    guint64 last_count_read;
    guint64 last_count_write;
};

typedef enum {
    MENU_REMOVE_GROUP,
    MENU_ADD_GROUP,
    MENU_ADD_ALL_LABELS,
    MENU_ADD_FUNCTION_LABELS,
    MENU_PLOT,
    MENU_SAVE_PS,
    MENU_PRINT,
} menu_id;

typedef struct _menu_item {
    char *name;
    menu_id id;
    GtkWidget *item;
} menu_item;

static menu_item range_menu_items[] = {
    {"Remove range", MENU_REMOVE_GROUP},
    {"Add range...", MENU_ADD_GROUP},
    {"Add all labels", MENU_ADD_ALL_LABELS},
    {"Add C functions (bad hack (labels not containing \"_DS_\"))", MENU_ADD_FUNCTION_LABELS},
    {"Snapshot to plot", MENU_PLOT},
};

static menu_item plot_menu_items[] = {
    {"Save postscript...", MENU_SAVE_PS},
    {"Print", MENU_PRINT},
};

static menu_item exestats_menu_items[] = {
    {"Plot distribution", MENU_PLOT},
};


extern int gui_message(char *message);

static GtkStyle *normal_style;

int plot_profile(Profile_Window *pw, char **pointlabel, guint64 *cyclearray, int numpoints);
int plot_routine_histogram(Profile_Window *pw);
float calculate_stddev(GList *start, GList *stop, float average);
double calculate_median(GList *start, GList *stop);

// Used only in popup menus
Profile_Window *popup_pw;

static void remove_entry(Profile_Window *pw, struct profile_entry *entry)
{
    gtk_clist_remove(GTK_CLIST(pw->profile_range_clist),pw->range_current_row);
    pw->profile_range_list=g_list_remove(pw->profile_range_list,entry);
    free(entry);
}

static unsigned int lookup_address_symbol(char *name)
{
    sym *s;
    gpsim_symbol_rewind((unsigned int)gp->pic_id);

    while(NULL != (s = gpsim_symbol_iter(gp->pic_id)))
    {
	if(!strcmp(name,s->name))
            return s->value;
    }
    return UINT_MAX;
}

static void add_range(Profile_Window *pw,
		      char *startaddress_text,
		      char *endaddress_text)
{
    guint64 cycles;
    struct profile_range_entry *profile_range_entry;
    unsigned int startaddress;
    unsigned int endaddress;
    char count_string[100];
    char *entry[PROFILE_COLUMNS]={startaddress_text,endaddress_text,count_string};
    int row;
    int i;
    GUI_Processor *gp;
    char *end;
    char msg[128];

    startaddress = strtoul(startaddress_text,&end,0);
    if(*end!='\0')
    {
	// Try to look the address up in symbol table.
	startaddress=lookup_address_symbol(startaddress_text);
	if(startaddress==UINT_MAX)
	{
	    startaddress=0;
	    sprintf(msg,"Could not find symbol \"%s\"",startaddress_text);
	    gui_message(msg);
	}
    }

    endaddress = strtoul(endaddress_text,&end,0);
    if(*end!='\0')
    {
	// Try to look the address up in symbol table.
        endaddress=lookup_address_symbol(endaddress_text);
	if(endaddress==UINT_MAX)
	{
	    endaddress=0;
	    sprintf(msg,"Could not find symbol \"%s\"",endaddress_text);
	    gui_message(msg);
	}
    }

    gp=pw->gui_obj.gp;

    cycles=0;
    for(i=startaddress;i<endaddress;i++)
    {
	cycles+=gpsim_get_cycles_used(gp->pic_id,i);
    }
    sprintf(count_string,"0x%Lx",cycles);

    row=gtk_clist_append(GTK_CLIST(pw->profile_range_clist), entry);

    // FIXME this memory is never freed?
    profile_range_entry = (struct profile_range_entry*)malloc(sizeof(struct profile_range_entry));
    strcpy(profile_range_entry->startaddress_text,startaddress_text);
    strcpy(profile_range_entry->endaddress_text,endaddress_text);
    profile_range_entry->startaddress=startaddress;
    profile_range_entry->endaddress=endaddress;
    profile_range_entry->pic_id=gp->pic_id;
    profile_range_entry->last_count=cycles;

    gtk_clist_set_row_data(GTK_CLIST(pw->profile_range_clist), row, (gpointer)profile_range_entry);

    pw->profile_range_list = g_list_append(pw->profile_range_list, (gpointer)profile_range_entry);

    gtk_clist_sort(GTK_CLIST(pw->profile_range_clist));
}

static void a_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=TRUE;
//    gtk_main_quit();
}

static void b_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=FALSE;
//    gtk_main_quit();
}

static void add_range_dialog(Profile_Window *pw)
{
    static GtkWidget *dialog=NULL;
    GtkWidget *button;
    GtkWidget *hbox;
    GtkWidget *label;
    static GtkWidget *startentry;
    static GtkWidget *endentry;
    int retval=-1;

    if(dialog==NULL)
    {
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog),"Add range");
	gtk_signal_connect_object(GTK_OBJECT(dialog),
				  "delete_event",
				  GTK_SIGNAL_FUNC(gtk_widget_hide),
				  GTK_OBJECT(dialog));

	label=gtk_label_new("addresses can be entered either as symbols, or as values. \nValues can be entered in decimal, hexadecimal, and octal.\nFor example: 31 is the same as 0x1f and 037");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,FALSE,FALSE,20);
	
	hbox = gtk_hbox_new(0,0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);

	button = gtk_button_new_with_label("Add range");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(a_cb),(gpointer)&retval);
	
	button = gtk_button_new_with_label("Cancel");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(b_cb),(gpointer)&retval);

	label=gtk_label_new("Enter start address");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label,
			   FALSE,FALSE, 20);

	startentry=gtk_entry_new();
	gtk_widget_show(startentry);
	gtk_box_pack_start(GTK_BOX(hbox), startentry,FALSE,FALSE,20);

	label=gtk_label_new("Enter stop address");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label,
			   FALSE,FALSE, 20);

	endentry=gtk_entry_new();
	gtk_widget_show(endentry);
	gtk_box_pack_start(GTK_BOX(hbox), endentry,FALSE,FALSE,20);


    }

    gtk_widget_show_now(dialog);

    gtk_grab_add(dialog);
    while(retval==-1 && GTK_WIDGET_VISIBLE(dialog))
	gtk_main_iteration();
    gtk_grab_remove(dialog);
    
    gtk_widget_hide(dialog);

    if(retval==TRUE)
    {
	// Add range.

	gchar *startentry_text;
	gchar *endentry_text;

	startentry_text = gtk_entry_get_text(GTK_ENTRY(startentry));
	if(*startentry_text!='\0')
	{
	    endentry_text = gtk_entry_get_text(GTK_ENTRY(endentry));
	    if(*endentry_text!='\0')
	    {
                add_range(pw,startentry_text,endentry_text);
	    }
	}
    }
    
    return;
}

/*
 this function compares sym pointers for g_list_sort()
 */
static gint
symcompare(sym *sym1, sym *sym2)
{
    if(sym1->value<sym2->value)
	return -1;
    if(sym1->value>sym2->value)
	return 1;
    return 0;
}

static void
file_selection_ok (GtkWidget        *w,
		   GtkFileSelection *fs)
{
    char *file;

    file=gtk_file_selection_get_filename (fs);
    gtk_plot_canvas_export_ps(GTK_PLOT_CANVAS(popup_pw->plot_canvas), file, 0, 0,
			      GTK_PLOT_LETTER);

    gtk_widget_hide (GTK_WIDGET (fs));
}

static void
print_plot (Profile_Window *pw)
{
    char *file;
    char cmd[200];

    file=tempnam("/tmp","gpsimplot");
    gtk_plot_canvas_export_ps(GTK_PLOT_CANVAS(popup_pw->plot_canvas), file, 0, 0,
			      GTK_PLOT_LETTER);
    sprintf(cmd,"lpr %s",file);
    system(cmd);
    remove(file);
}

extern int gui_question(char *question, char *a, char *b);

static GtkItemFactoryCallback 
open_plotsave_dialog(Profile_Window *pw)
{
    static GtkWidget *window = NULL;

    if (!window)
    {

	window = gtk_file_selection_new ("Save postscript to file...");

	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (window));

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

	gtk_signal_connect_object(GTK_OBJECT(window),
				  "delete_event",
				  GTK_SIGNAL_FUNC(gtk_widget_hide),
				  GTK_OBJECT(window));
//	gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
//				   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
//				   GTK_OBJECT(window));

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC(file_selection_ok),
			    window);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
				   "clicked", GTK_SIGNAL_FUNC(gtk_widget_hide),
				   GTK_OBJECT (window));
    }
    gtk_widget_show (window);
    return NULL;
}


// called when user has selected a menu item in plot window
static void
plot_popup_activated(GtkWidget *widget, gpointer data)
{
    menu_item *item;
    struct profile_entry *entry;
    unsigned int pic_id;

    if(widget==NULL || data==NULL)
    {
	printf("Warning plot_popup_activated(%p,%p)\n",widget,data);
	return;
    }
    
    item = (menu_item *)data;
    pic_id = ((GUI_Object*)popup_pw)->gp->pic_id;

    entry = (struct profile_entry *)gtk_clist_get_row_data(GTK_CLIST(popup_pw->profile_range_clist),popup_pw->range_current_row);

    switch(item->id)
    {
    case MENU_SAVE_PS:
	open_plotsave_dialog(popup_pw);
	break;
    case MENU_PRINT:
        print_plot(popup_pw);
	break;
    default:
	puts("Unhandled menuitem?");
	break;
    }
}

// called when user has selected a menu item in exestats tab
static void
exestats_popup_activated(GtkWidget *widget, gpointer data)
{
    menu_item *item;
    unsigned int pic_id;

    if(widget==NULL || data==NULL)
    {
	printf("Warning exestats_popup_activated(%p,%p)\n",widget,data);
	return;
    }
    
    item = (menu_item *)data;
    pic_id = ((GUI_Object*)popup_pw)->gp->pic_id;

    switch(item->id)
    {
    case MENU_PLOT:
        plot_routine_histogram(popup_pw);
	break;
    default:
	puts("Unhandled menuitem?");
	break;
    }
}

// called from plot_do_popup
static GtkWidget *
plot_build_menu(Profile_Window *pw)
{
  GtkWidget *menu;
  GtkWidget *item;
  int i;


  if(pw==NULL)
  {
      printf("Warning build_menu(%p)\n",pw);
      return NULL;
  }
    
  popup_pw = pw;
  
  menu=gtk_menu_new();

  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  
  for (i=0; i < (sizeof(plot_menu_items)/sizeof(plot_menu_items[0])) ; i++){
      plot_menu_items[i].item=item=gtk_menu_item_new_with_label(plot_menu_items[i].name);

      gtk_signal_connect(GTK_OBJECT(item),"activate",
			 (GtkSignalFunc) plot_popup_activated,
			 &plot_menu_items[i]);
      
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu),item);
  }

  return menu;
}

// called from exestats_do_popup
static GtkWidget *
exestats_build_menu(Profile_Window *pw)
{
  GtkWidget *menu;
  GtkWidget *item;
  int i;


  if(pw==NULL)
  {
      printf("Warning build_menu(%p)\n",pw);
      return NULL;
  }
    
  popup_pw = pw;
  
  menu=gtk_menu_new();

  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  
  for (i=0; i < (sizeof(exestats_menu_items)/sizeof(exestats_menu_items[0])) ; i++){
      exestats_menu_items[i].item=item=gtk_menu_item_new_with_label(exestats_menu_items[i].name);

      gtk_signal_connect(GTK_OBJECT(item),"activate",
			 (GtkSignalFunc) exestats_popup_activated,
			 &exestats_menu_items[i]);
      
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu),item);
  }

  return menu;
}

// button press handler
static gint
exestats_do_popup(GtkWidget *widget, GdkEventButton *event, Profile_Window *pw)
{

    GtkWidget *popup;

    if(widget==NULL || event==NULL || pw==NULL)
    {
	printf("Warning exestats_popup(%p,%p,%p)\n",widget,event,pw);
	return 0;
    }

    popup=pw->exestats_popup_menu;

    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL,
		     3, event->time);
    }
    return FALSE;
}

// button press handler
static gint
plot_do_popup(GtkWidget *widget, GdkEventButton *event, Profile_Window *pw)
{

    GtkWidget *popup;

    if(widget==NULL || event==NULL || pw==NULL)
    {
	printf("Warning do_popup(%p,%p,%p)\n",widget,event,pw);
	return 0;
    }

    popup=pw->plot_popup_menu;

    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL,
		     3, event->time);
    }
    return FALSE;
}

int plot_profile(Profile_Window *pw, char **pointlabel, guint64 *cyclearray, int numpoints)
{

    static GtkWidget *window1;
    GtkWidget *vbox1;
    GtkWidget *scrollw1;
    static GtkWidget *active_plot;
    static GtkWidget *canvas;
    static GdkColor color1;
    static GdkColor color2;
    static GdkColor bg_color;
    gint page_width, page_height;
    int scale = 1;
    static GtkPlotText *infotext;
    static GtkPlotText **bartext;
    GtkWidget *plot;
    static GtkPlotData *dataset;
    char infostring[128];
    char filename[128];

    int i;

    guint64 i64, x;

    guint64 maxy=0;

    static double *px2;//[] = {.1, .2, .3, .4, .5, .6, .7, .8};
    static double *py2;//[] = {.012*1000, .067*1000, .24*1000, .5*1000, .65*1000, .5*1000, .24*1000, .067*1000};
    gdouble tickdelta;
    gdouble barwidth;
    int pic_id;
    time_t t;

    static int has_old_graph=0;
    static int last_numpoints=0;

    if(gpsim_get_program_memory_size(((GUI_Object*)pw)->gp->pic_id)<=0)
        return 0;

    if(has_old_graph)
    {
	gtk_plot_remove_text(GTK_PLOT(active_plot),infotext);
	for(i=0;i<last_numpoints;i++)
	{
	    gtk_plot_remove_text(GTK_PLOT(active_plot),bartext[i]);

	}
        free(px2);
        free(py2);
        free(bartext);
    }

    px2=(double*)malloc(numpoints*sizeof(double));
    py2=(double*)malloc(numpoints*sizeof(double));
    bartext=(GtkPlotText**)malloc(numpoints*sizeof(GtkPlotText*));

#define WINDOWWIDTH 550
#define WINDOWHEIGHT 650

#define PLOTXPOS 0.25
#define PLOTWIDTH 0.50
#define PLOTYPOS 0.15
#define PLOTHEIGHT 0.50

    barwidth=PLOTWIDTH/(numpoints*1.1);

    for(i=0;i<numpoints;i++)
    {
	px2[i]=(i+1)*barwidth*2;
	if(maxy<cyclearray[i])
            maxy=cyclearray[i];
	py2[i]=cyclearray[i];
    }

    maxy=maxy + (maxy/10);


    // Compute tickdelta for easy reading.
    x=maxy;
    i64=1;
    while(x>=10L)
    {
	x/=10L;
        i64*=10L;
    }
    tickdelta=x*i64/10;

    if(tickdelta<1)
        tickdelta=1;

    

    pic_id = ((GUI_Object*)pw)->gp->pic_id;

    t=time(NULL);

    // Compute module name to put in infostring
    for(i=0;i<gpsim_get_number_of_source_files(pic_id);i++)
    {
	struct file_context *gpsim_file;
        char *file_name;
	gpsim_file = gpsim_get_file_context(pic_id, i);
	file_name = gpsim_file->name;
	if(!strcmp(file_name+strlen(file_name)-4,".asm")
	   ||!strcmp(file_name+strlen(file_name)-4,".ASM")
	   ||!strcmp(file_name+strlen(file_name)-4,".hex")
	   ||!strcmp(file_name+strlen(file_name)-4,".HEX")
	  )
	{
	    strncpy(filename,gpsim_file->name,strlen(file_name)-4);
	    filename[strlen(file_name)-4]=0;
            break;
	}
    }

    // This information is put at top of the plot
    sprintf(infostring,"\\BFile:\\N\"%s\" \\BDate:\\N%s \\BProcessor:\\N\"%s\"",
	    filename,
            ctime(&t),
	    gpsim_processor_get_name(pic_id));

    // ctime adds a newline. Remove it.
    for(i=0;infostring[i];i++)
	if(infostring[i]=='\n')
	    infostring[i]=' ';



    page_width = GTK_PLOT_LETTER_W * scale;
    page_height = GTK_PLOT_LETTER_H * scale;

    // Only create the window once.
    if(!window1)
    {
	window1=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window1), "Profile plot");
	gtk_widget_set_usize(window1,WINDOWWIDTH,WINDOWHEIGHT);
	gtk_container_border_width(GTK_CONTAINER(window1),0);

	gtk_signal_connect_object(GTK_OBJECT(window1),
				  "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(window1));
//	gtk_signal_connect_object (GTK_OBJECT (window1), "destroy",
//				   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
//				   GTK_OBJECT(window1));

	vbox1=gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(window1),vbox1);
	gtk_widget_show(vbox1);

	scrollw1=gtk_scrolled_window_new(NULL, NULL);
	gtk_container_border_width(GTK_CONTAINER(scrollw1),0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollw1),
				       GTK_POLICY_ALWAYS,GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox1),scrollw1, TRUE, TRUE,0);
	gtk_widget_show(scrollw1);

	pw->plot_canvas=canvas = gtk_plot_canvas_new(page_width, page_height, 1.);
	GTK_PLOT_CANVAS_SET_FLAGS(GTK_PLOT_CANVAS(canvas), GTK_PLOT_CANVAS_DND_FLAGS);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollw1), canvas);

	gtk_widget_show(canvas);


	pw->plot_popup_menu=plot_build_menu(pw);

	gtk_signal_connect(GTK_OBJECT(canvas),
			   "button_press_event",
			   (GtkSignalFunc) plot_do_popup,
			   pw);

	plot = gtk_plot_new_with_size(NULL, PLOTWIDTH, PLOTHEIGHT);
	gtk_widget_show(plot);

	active_plot=plot;

	gdk_color_parse("light yellow", &bg_color);
	gdk_color_alloc(gtk_widget_get_colormap(active_plot), &bg_color);
	gtk_plot_set_background(GTK_PLOT(active_plot), &bg_color);

	gdk_color_parse("black", &color1);
	gdk_color_alloc(gtk_widget_get_colormap(active_plot), &color1);
	gdk_color_parse("black", &color2);
	gdk_color_alloc(gtk_widget_get_colormap(canvas), &color2);

	gtk_plot_hide_legends(GTK_PLOT(active_plot));
	gtk_plot_axis_show_labels(GTK_PLOT(active_plot),GTK_PLOT_AXIS_TOP,0);
	gtk_plot_axis_show_labels(GTK_PLOT(active_plot),GTK_PLOT_AXIS_BOTTOM,0);
	gtk_plot_axis_show_labels(GTK_PLOT(active_plot),GTK_PLOT_AXIS_RIGHT,0);
	gtk_plot_axis_set_visible(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP, TRUE);
	gtk_plot_grids_set_visible(GTK_PLOT(active_plot), TRUE, TRUE, TRUE, TRUE);
	gtk_plot_canvas_add_plot(GTK_PLOT_CANVAS(canvas), GTK_PLOT(active_plot), PLOTXPOS, PLOTYPOS);
	gtk_plot_axis_hide_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP);
	gtk_plot_axis_hide_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_BOTTOM);
	gtk_plot_axis_hide_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_LEFT);
	gtk_plot_axis_hide_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_RIGHT);
	gtk_plot_set_legends_border(GTK_PLOT(active_plot), GTK_PLOT_BORDER_SHADOW, 3);
	gtk_plot_legends_move(GTK_PLOT(active_plot), .58, .05);
	gtk_widget_show(active_plot);



	dataset = GTK_PLOT_DATA(gtk_plot_bar_new(GTK_ORIENTATION_VERTICAL));
	gtk_plot_add_data(GTK_PLOT(active_plot), GTK_PLOT_DATA(dataset));
    }

    gtk_plot_axis_set_ticks(GTK_PLOT(active_plot), GTK_PLOT_AXIS_Y, tickdelta, 0);
    gtk_plot_set_range(GTK_PLOT(active_plot), 0., 1., 0., (gdouble)maxy);
    gtk_plot_axis_set_labels_numbers(GTK_PLOT(active_plot),
				     GTK_PLOT_AXIS_LEFT,
				     maxy<10000?0:GTK_PLOT_LABEL_EXP,
                                     0);

    gtk_plot_data_set_points(GTK_PLOT_DATA(dataset), px2, py2, NULL, NULL, numpoints);
    gtk_plot_data_set_symbol(GTK_PLOT_DATA(dataset),
			     GTK_PLOT_SYMBOL_NONE,
			     GTK_PLOT_SYMBOL_FILLED,
			     0, 4.0, &color1,&color2);
    gtk_plot_data_set_line_attributes(GTK_PLOT_DATA(dataset),
				      GTK_PLOT_LINE_NONE,
				      5, &color2);

    gtk_plot_data_set_connector(GTK_PLOT_DATA(dataset), GTK_PLOT_CONNECT_NONE);

    gtk_widget_show(GTK_WIDGET(dataset));

    // Put the description text under each bar in the plot.
    for(i=0;i<numpoints;i++)
    {

	bartext[i]=gtk_plot_put_text(GTK_PLOT(active_plot),
				     PLOTXPOS+px2[i]*PLOTWIDTH,
				     PLOTYPOS+PLOTHEIGHT+0.01,
				     NULL,
				     20,
				     270,
				     NULL,
				     NULL,
				     TRUE,
				     GTK_JUSTIFY_LEFT,
				     pointlabel[i]);

	gtk_plot_draw_text(GTK_PLOT(active_plot),*bartext[i]);

    }

    infotext=gtk_plot_put_text(GTK_PLOT(active_plot),
			       0.5,
			       PLOTYPOS-0.05,
			       NULL,
			       20,
			       00,
			       NULL,
			       NULL,
			       TRUE,
			       GTK_JUSTIFY_CENTER,
			       infostring);

    gtk_plot_draw_text(GTK_PLOT(active_plot),*infotext);

    gtk_widget_queue_draw(window1);

    gtk_widget_show(window1);

    has_old_graph=1;
    last_numpoints=numpoints;
    return 0;
}

int plot_routine_histogram(Profile_Window *pw)
{

    static GtkWidget *window1;
    GtkWidget *vbox1;
    GtkWidget *scrollw1;
    static GtkWidget *active_plot;
    static GtkWidget *canvas;
    static GdkColor color1;
    static GdkColor color2;
    static GdkColor bg_color;
    gint page_width, page_height;
    int scale = 1;
    static GtkPlotText *infotext1;
    static GtkPlotText *infotext2;
    GtkWidget *plot;
    static GtkPlotData *dataset;
    char infostring[128];
    char filename[128];

    int i,j;

    guint64 i64, x, y;
    guint64 mincycles, maxcycles, totalcycles, totalcount;
    double averagecycles, mediancycles, stddevcycles;

    guint64 maxy=0;
    guint64 maxx=0;
    guint64 minx=0xffffffffffffffffull;
    guint64 margin;

    static double *px2;//[] = {.1, .2, .3, .4, .5, .6, .7, .8};
    static double *py2;//[] = {.012*1000, .067*1000, .24*1000, .5*1000, .65*1000, .5*1000, .24*1000, .067*1000};
    gdouble tickdelta_x;
    gdouble tickdelta_y;
    gdouble barwidth;
    int pic_id;
    time_t t;

    static int has_old_graph=0;
    static int last_numpoints=0;
    int numpoints;
    GList *iter;

    if(gpsim_get_program_memory_size(((GUI_Object*)pw)->gp->pic_id)<=0)
	return 0;

    if(pw->histogram_profile_list==NULL)
        return 0;

    if(has_old_graph)
    {
	gtk_plot_remove_text(GTK_PLOT(active_plot),infotext1);
	gtk_plot_remove_text(GTK_PLOT(active_plot),infotext2);
        free(px2);
        free(py2);
    }

#define WINDOWWIDTH 550
#define WINDOWHEIGHT 650

#define PLOTXPOS 0.25
#define PLOTWIDTH 0.50
#define PLOTYPOS 0.15
#define PLOTHEIGHT 0.50

    // Find the number of points and allocate the point arrays
    numpoints=0;
    iter=pw->histogram_profile_list;
    while(iter!=NULL)
    {
	numpoints++;
	iter=iter->next;
    }
//    numpoints=10;
    px2=(double*)malloc(numpoints*sizeof(double));
    py2=(double*)malloc(numpoints*sizeof(double));

    totalcycles=0;
    totalcount=0;
    // Find values, and put them in the point arrays
    j=0;
    iter=pw->histogram_profile_list;
    puts("");
    while(iter!=NULL)
//    for(j=0;j<10;j++)
    {
	struct cycle_histogram_counter *chc;
        chc=(struct cycle_histogram_counter*)iter->data;

//	px2[j]=i;
//	py2[j]=i*4;
	px2[j]=chc->cycles;
	py2[j]=chc->count;
	if(maxy<py2[j])
	    maxy=py2[j];
	if(maxx<px2[j])
	    maxx=px2[j];
	if(minx>px2[j])
	    minx=px2[j];

	totalcycles+=chc->cycles*chc->count;
        totalcount+=chc->count;

        j++;
	iter=iter->next;
    }
    puts("");

    mincycles=minx;
    maxcycles=maxx;
    averagecycles=totalcycles/(float)totalcount;
    mediancycles=calculate_median(pw->histogram_profile_list,NULL);
    stddevcycles=calculate_stddev(pw->histogram_profile_list,NULL,averagecycles);

    barwidth=PLOTWIDTH/(1.3*(maxx-minx/*numpoints*/));

    // Compute tickdelta for easy reading.
    y=maxy;
    i64=1;
    while(y>=10L)
    {
	y/=10L;
        i64*=10L;
    }
    tickdelta_y=y*i64/10;
    if(tickdelta_y<1)
	tickdelta_y=1;

    // Compute tickdelta for easy reading.
    x=maxx-minx;
    i64=1;
    while(x>=10L)
    {
	x/=10L;
        i64*=10L;
    }
    tickdelta_x=x*i64/5;
    if(tickdelta_x<1)
        tickdelta_x=1;


    maxy=maxy*1.1+1;
    margin=((int)(maxx-minx))*.15+1;
    maxx=maxx+margin;
    if(minx>margin)
	minx=minx-margin;
    else
        minx=0;

    pic_id = ((GUI_Object*)pw)->gp->pic_id;

    page_width = GTK_PLOT_LETTER_W * scale;
    page_height = GTK_PLOT_LETTER_H * scale;

    // Only create the window once.
    if(!window1)
    {
	window1=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window1), "Routine histogram");
	gtk_widget_set_usize(window1,WINDOWWIDTH,WINDOWHEIGHT);
	gtk_container_border_width(GTK_CONTAINER(window1),0);

	gtk_signal_connect_object(GTK_OBJECT(window1),
				  "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(window1));
//	gtk_signal_connect_object (GTK_OBJECT (window1), "destroy",
//				   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
//				   GTK_OBJECT(window1));

	vbox1=gtk_vbox_new(FALSE,0);
	gtk_container_add(GTK_CONTAINER(window1),vbox1);
	gtk_widget_show(vbox1);

	scrollw1=gtk_scrolled_window_new(NULL, NULL);
	gtk_container_border_width(GTK_CONTAINER(scrollw1),0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollw1),
				       GTK_POLICY_ALWAYS,GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox1),scrollw1, TRUE, TRUE,0);
	gtk_widget_show(scrollw1);

	pw->plot_canvas=canvas = gtk_plot_canvas_new(page_width, page_height, 1.);
	GTK_PLOT_CANVAS_SET_FLAGS(GTK_PLOT_CANVAS(canvas), GTK_PLOT_CANVAS_DND_FLAGS);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollw1), canvas);

	gtk_widget_show(canvas);


	pw->plot_popup_menu=plot_build_menu(pw);

	gtk_signal_connect(GTK_OBJECT(canvas),
			   "button_press_event",
			   (GtkSignalFunc) plot_do_popup,
			   pw);

	plot = gtk_plot_new_with_size(NULL, PLOTWIDTH, PLOTHEIGHT);
	gtk_widget_show(plot);

	active_plot=plot;

	gdk_color_parse("light yellow", &bg_color);
	gdk_color_alloc(gtk_widget_get_colormap(active_plot), &bg_color);
	gtk_plot_set_background(GTK_PLOT(active_plot), &bg_color);

	gdk_color_parse("black", &color1);
	gdk_color_alloc(gtk_widget_get_colormap(active_plot), &color1);
	gdk_color_parse("black", &color2);
	gdk_color_alloc(gtk_widget_get_colormap(canvas), &color2);

	gtk_plot_hide_legends(GTK_PLOT(active_plot));
	gtk_plot_axis_show_labels(GTK_PLOT(active_plot),GTK_PLOT_AXIS_TOP,0);
	gtk_plot_axis_show_labels(GTK_PLOT(active_plot),GTK_PLOT_AXIS_RIGHT,0);
	gtk_plot_axis_set_visible(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP, TRUE);
	gtk_plot_grids_set_visible(GTK_PLOT(active_plot), TRUE, TRUE, TRUE, TRUE);
	gtk_plot_canvas_add_plot(GTK_PLOT_CANVAS(canvas), GTK_PLOT(active_plot), PLOTXPOS, PLOTYPOS);
	gtk_plot_axis_hide_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP);
	gtk_plot_axis_hide_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_RIGHT);
	gtk_plot_set_legends_border(GTK_PLOT(active_plot), GTK_PLOT_BORDER_SHADOW, 3);
	gtk_plot_legends_move(GTK_PLOT(active_plot), .58, .05);
	gtk_widget_show(active_plot);



	dataset = GTK_PLOT_DATA(gtk_plot_bar_new(GTK_ORIENTATION_VERTICAL));
	gtk_plot_add_data(GTK_PLOT(active_plot), GTK_PLOT_DATA(dataset));
    }

    gtk_plot_axis_set_ticks(GTK_PLOT(active_plot), GTK_PLOT_AXIS_Y, tickdelta_y, 1);
    gtk_plot_axis_set_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_LEFT, "Frequency");
    gtk_plot_axis_set_labels_numbers(GTK_PLOT(active_plot),
				     GTK_PLOT_AXIS_LEFT,
				     0,
                                     0);

    gtk_plot_axis_set_ticks(GTK_PLOT(active_plot), GTK_PLOT_AXIS_X, tickdelta_x, 1);
    gtk_plot_axis_set_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_BOTTOM, "Cycles");
    gtk_plot_axis_set_labels_numbers(GTK_PLOT(active_plot),
				     GTK_PLOT_AXIS_BOTTOM,
				     maxx<10000?0:GTK_PLOT_LABEL_EXP,
				     0);

    gtk_plot_set_range(GTK_PLOT(active_plot), minx, (gdouble)maxx, 0., (gdouble)maxy);

    gtk_plot_data_set_points(GTK_PLOT_DATA(dataset), px2, py2, NULL, NULL, numpoints);
    gtk_plot_data_set_symbol(GTK_PLOT_DATA(dataset),
			     GTK_PLOT_SYMBOL_NONE,
			     GTK_PLOT_SYMBOL_FILLED,
			     0, 4.0, &color1,&color2);
    gtk_plot_data_set_line_attributes(GTK_PLOT_DATA(dataset),
				      GTK_PLOT_LINE_NONE,
				      5, &color2);

    gtk_plot_data_set_connector(GTK_PLOT_DATA(dataset), GTK_PLOT_CONNECT_NONE);

    gtk_widget_show(GTK_WIDGET(dataset));


    // Infostring1
    t=time(NULL);
    // Compute module name to put in infostring
    for(i=0;i<gpsim_get_number_of_source_files(pic_id);i++)
    {
	struct file_context *gpsim_file;
        char *file_name;
	gpsim_file = gpsim_get_file_context(pic_id, i);
	file_name = gpsim_file->name;
	if(!strcmp(file_name+strlen(file_name)-4,".asm")
	   ||!strcmp(file_name+strlen(file_name)-4,".ASM")
	   ||!strcmp(file_name+strlen(file_name)-4,".hex")
	   ||!strcmp(file_name+strlen(file_name)-4,".HEX")
	  )
	{
	    strncpy(filename,gpsim_file->name,strlen(file_name)-4);
	    filename[strlen(file_name)-4]=0;
            break;
	}
    }
    // This information is put at top of the plot
    sprintf(infostring,"\\BFile:\\N\"%s\" \\BDate:\\N%s \\BProcessor:\\N\"%s\"",
	    filename,
            ctime(&t),
	    gpsim_processor_get_name(pic_id));
    // ctime adds a newline. Remove it.
    for(i=0;infostring[i];i++)
	if(infostring[i]=='\n')
	    infostring[i]=' ';
    infotext1=gtk_plot_put_text(GTK_PLOT(active_plot),
				0.5,
				PLOTYPOS-0.05,
				NULL,
				20,
				00,
				NULL,
				NULL,
				TRUE,
				GTK_JUSTIFY_CENTER,
				infostring);
    gtk_plot_draw_text(GTK_PLOT(active_plot),*infotext1);

    // Infostring2
    sprintf(infostring,"\\BMin:\\N\%lld \\BMax:\\N%lld \\BAverage:\\N%.1f \\BMedian:\\N%.1f \\BStandard deviation:\\N%.1f",
	    mincycles,
	    maxcycles,
	    averagecycles,
	    mediancycles,
	    stddevcycles);
    infotext2=gtk_plot_put_text(GTK_PLOT(active_plot),
				0.5,
				PLOTYPOS-0.03,
				NULL,
				20,
				00,
				NULL,
				NULL,
				TRUE,
				GTK_JUSTIFY_CENTER,
				infostring);
    gtk_plot_draw_text(GTK_PLOT(active_plot),*infotext2);

    gtk_widget_queue_draw(window1);

    gtk_widget_show(window1);

    has_old_graph=1;
    last_numpoints=numpoints;
    return 0;
}

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
    char fromaddress_string[256];
    char toaddress_string[256];
    menu_item *item;
    sym *s;
    GList *symlist=NULL;
    GList *iter;

    struct profile_entry *entry;
    struct profile_range_entry *range_entry=NULL;

    unsigned int pic_id;

    if(widget==NULL || data==NULL)
    {
	printf("Warning popup_activated(%p,%p)\n",widget,data);
	return;
    }
    
    item = (menu_item *)data;
    pic_id = ((GUI_Object*)popup_pw)->gp->pic_id;

    entry = (struct profile_entry *)gtk_clist_get_row_data(GTK_CLIST(popup_pw->profile_range_clist),popup_pw->range_current_row);

    switch(item->id)
    {
    case MENU_REMOVE_GROUP:
	remove_entry(popup_pw,entry);
	break;
    case MENU_ADD_GROUP:
        add_range_dialog(popup_pw);
	break;
    case MENU_ADD_ALL_LABELS:
	gpsim_symbol_rewind((unsigned int)gp->pic_id);

	while(NULL != (s = gpsim_symbol_iter(gp->pic_id)))
	{
	    sym *data;
	    if(s->type==SYMBOL_ADDRESS)
	    {
		data=(sym*)malloc(sizeof(sym));
		memcpy(data,s,sizeof(sym));
		data->name=(char*)malloc(strlen(s->name)+1);
                strcpy(data->name,s->name);
		symlist=g_list_append(symlist,data);
	    }
	}
	symlist=g_list_sort(symlist,(GCompareFunc)symcompare);
	strcpy(fromaddress_string,"0");
	iter=symlist;
	while(iter!=NULL)
	{
	    s=(sym*)iter->data;

	    strcpy(toaddress_string,s->name);
	    add_range(popup_pw,fromaddress_string,toaddress_string);
	    strcpy(fromaddress_string,toaddress_string);
	    toaddress_string[0]='\0';
            free(s->name);
	    free(s);
            iter=iter->next;
	}

	sprintf(toaddress_string,"%d",gpsim_get_program_memory_size(gp->pic_id));
	add_range(popup_pw,fromaddress_string,toaddress_string);

	while(symlist!=NULL)
	    symlist=g_list_remove(symlist,symlist->data);

	break;
    case MENU_ADD_FUNCTION_LABELS:
	gpsim_symbol_rewind((unsigned int)gp->pic_id);

	while(NULL != (s = gpsim_symbol_iter(gp->pic_id)))
	{
	    if(s->type==SYMBOL_ADDRESS)
	    {
		if(NULL==strstr(s->name,"_DS_"))
		{
		    sym *data;
		    data=(sym*)malloc(sizeof(sym));
		    memcpy(data,s,sizeof(sym));
		    data->name=(char*)malloc(strlen(s->name)+1);
		    strcpy(data->name,s->name);
		    symlist=g_list_append(symlist,data);
		}
	    }
	}
	symlist=g_list_sort(symlist,(GCompareFunc)symcompare);

	iter=symlist;
	if(iter!=NULL)
	{
	    s=(sym*)iter->data;
	    strcpy(fromaddress_string,s->name);
            free(s->name);
	    free(s);
	    iter=iter->next;
	    while(iter!=NULL)
	    {
		s=(sym*)iter->data;
		strcpy(toaddress_string,s->name);
		add_range(popup_pw,fromaddress_string,toaddress_string);
		strcpy(fromaddress_string,toaddress_string);
		toaddress_string[0]='\0';
		free(s);
		iter=iter->next;
	    }

	    sprintf(toaddress_string,"%d",gpsim_get_program_memory_size(gp->pic_id));
	    add_range(popup_pw,fromaddress_string,toaddress_string);

	}

	while(symlist!=NULL)
	    symlist=g_list_remove(symlist,symlist->data);

	break;
    case MENU_PLOT:
	{
	guint64 *cyclearray;//{100,200,300,400,500,600,900,555};
	char **pointlabel;/*={
	    "start - labelx 0",
	    "start - labelx 1",
	    "start - labelx 2",
	    "start - dgfdslabelx 3",
	    "start - labelx 4",
	    "start - labelx 5",
	    "start - labelx 6",
	    "start - labelx 7"
	};*/
	int numpoints=8;
        int i;

	pointlabel=(char**)malloc(sizeof(char*)*numpoints);
        cyclearray=(guint64*)malloc(sizeof(guint64)*numpoints);

	for(i=0;i<numpoints;i++)
	{
	    range_entry = (struct profile_range_entry *)gtk_clist_get_row_data(GTK_CLIST(popup_pw->profile_range_clist),i);
	    if(range_entry==NULL)
	    {
		if(i!=0)
		    plot_profile(popup_pw,pointlabel,cyclearray,i);
                break;
	    }
	    else
	    {
                pointlabel[i]=(char*)malloc(128);
		sprintf(pointlabel[i],"%s (end: %s)",range_entry->startaddress_text,range_entry->endaddress_text);
                cyclearray[i]=range_entry->last_count;
	    }
	}
        if(range_entry!=NULL)
	    plot_profile(popup_pw,pointlabel,cyclearray,numpoints);
	}
        break;
    default:
	puts("Unhandled menuitem?");
	break;
    }
}

static void update_menus(Profile_Window *pw)
{
    GtkWidget *item;
    struct profile_entry *entry;
    int i;

    for (i=0; i < (sizeof(range_menu_items)/sizeof(range_menu_items[0])) ; i++){
	item=range_menu_items[i].item;
//	if(range_menu_items[i].id!=MENU_ADD_GROUP)
	{
	    if(pw)
	    {
		entry = (struct profile_entry *)gtk_clist_get_row_data(GTK_CLIST(pw->profile_range_clist),pw->range_current_row);
		if(range_menu_items[i].id!=MENU_ADD_GROUP &&
		   range_menu_items[i].id!=MENU_ADD_ALL_LABELS &&
		   range_menu_items[i].id!=MENU_ADD_FUNCTION_LABELS &&
		   range_menu_items[i].id!=MENU_PLOT &&
		   entry==NULL)
		    gtk_widget_set_sensitive (item, FALSE);
		else
		    gtk_widget_set_sensitive (item, TRUE);
	    }
	    else
	    {
		gtk_widget_set_sensitive (item, FALSE);
	    }
	}
    }
}

static gint
key_press(GtkWidget *widget,
	  GdkEventKey *key, 
	  gpointer data)
{

    struct profile_range_entry *entry;
    Profile_Window *pw = (Profile_Window *) data;

  if(!pw) return(FALSE);
  if(!pw->gui_obj.gp) return(FALSE);
  if(!pw->gui_obj.gp->pic_id) return(FALSE);

  switch(key->keyval) {

  case GDK_Delete:
      entry = (struct profile_range_entry *)gtk_clist_get_row_data(GTK_CLIST(pw->profile_range_clist),pw->range_current_row);
      if(entry!=NULL)
	  remove_entry(pw,(struct profile_entry *)entry);
      break;
  }
  return TRUE;
}

static gint profile_range_list_row_selected(GtkCList *profilelist,gint row, gint column,GdkEvent *event, Profile_Window *pw)
{
    struct profile_range_entry *entry;
    //    int bit;
    GUI_Processor *gp;
    
    pw->range_current_row=row;
//    pw->current_column=column;

    gp=pw->gui_obj.gp;
    
    entry = (struct profile_range_entry *)gtk_clist_get_row_data(GTK_CLIST(pw->profile_clist), row);

    if(!entry)
	return TRUE;

    update_menus(pw);
  
    return 0;
}

// called from do_popup
static GtkWidget *
build_menu(Profile_Window *pw)
{
  GtkWidget *menu;
  GtkWidget *item;
  int i;


  if(pw==NULL)
  {
      printf("Warning build_menu(%p)\n",pw);
      return NULL;
  }
    
  popup_pw = pw;
  
  menu=gtk_menu_new();

  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  
  for (i=0; i < (sizeof(range_menu_items)/sizeof(range_menu_items[0])) ; i++){
      range_menu_items[i].item=item=gtk_menu_item_new_with_label(range_menu_items[i].name);

      gtk_signal_connect(GTK_OBJECT(item),"activate",
			 (GtkSignalFunc) popup_activated,
			 &range_menu_items[i]);
      
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu),item);
  }

  update_menus(pw);
  
  return menu;
}

// button press handler
static gint
do_popup(GtkWidget *widget, GdkEventButton *event, Profile_Window *pw)
{

    GtkWidget *popup;

  if(widget==NULL || event==NULL || pw==NULL)
  {
      printf("Warning do_popup(%p,%p,%p)\n",widget,event,pw);
      return 0;
  }

  popup=pw->range_popup_menu;
    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL,
		     3, event->time);
    }
    return FALSE;
}

/*
 the function comparing rows of profile list for sorting
 FIXME this can be improved. When we have equal cells in sort_column
 of the two rows, compare another column instead of returning 'match'.
 */
static gint
profile_compare_func(GtkCList *clist, gconstpointer ptr1,gconstpointer ptr2)
{
    char *text1, *text2;
    long val1, val2;
    GtkCListRow *row1 = (GtkCListRow *) ptr1;
    GtkCListRow *row2 = (GtkCListRow *) ptr2;
//    char *p;

    switch (row1->cell[clist->sort_column].type)
    {
    case GTK_CELL_TEXT:
	text1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
	break;
    case GTK_CELL_PIXTEXT:
	text1 = GTK_CELL_PIXTEXT (row1->cell[clist->sort_column])->text;
	break;
    default:
	assert(0);
	break;
    }

    switch (row2->cell[clist->sort_column].type)
    {
    case GTK_CELL_TEXT:
	text2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;
	break;
    case GTK_CELL_PIXTEXT:
	text2 = GTK_CELL_PIXTEXT (row2->cell[clist->sort_column])->text;
	break;
    default:
	assert(0);
	break;
    }

    if (!text2)
	assert(0);
    //	return (text1 != NULL);

    if (!text1)
	assert(0);
    //	return -1;

    if(1==sscanf(text1,"%li",&val1))
    {
	if(1==sscanf(text2,"%li",&val2))
	{
//	    printf("Value %d %d\n",val1,val2);
	    return val1-val2;
	}
    }
    return strcmp(text1,text2);
}



gint histogram_list_compare_func_cycles(gconstpointer a, gconstpointer b)
{
    const struct cycle_histogram_counter *h1=(struct cycle_histogram_counter*)a;
    const struct cycle_histogram_counter *h2=(struct cycle_histogram_counter*)b;

    if(h1->cycles > h2->cycles)
	return 1;
    if(h1->cycles == h2->cycles)
	return 0;
    return -1;
}

gint histogram_list_compare_func(gconstpointer a, gconstpointer b)
{
    const struct cycle_histogram_counter *h1=(struct cycle_histogram_counter*)a;
    const struct cycle_histogram_counter *h2=(struct cycle_histogram_counter*)b;

    if(h1->start_address > h2->start_address)
	return 1;
    if(h1->start_address == h2->start_address)
    {
	if(h1->stop_address > h2->stop_address)
	    return 1;
	if(h1->stop_address == h2->stop_address)
	{
	    if(h1->cycles*h1->count > h2->cycles*h2->count)
		return 1;
	    if(h1->cycles*h1->count == h2->cycles*h2->count)
		return 0;
	}
    }
    return -1;
}

double calculate_median(GList *start, GList *stop)
{
    GList *sorted_list=NULL;

    struct cycle_histogram_counter *chc_start, *chc_stop;//, *chc_result;
//    GList *result;
    int count_sum=0;

    if(start==NULL)
	return -4.2;

    if(stop==NULL)
    {
        stop=start;
	while(stop->next!=NULL)
	    stop=stop->next;
    }

    // Copy list and sort it on cycles
    while(start!=stop)
    {
	sorted_list=g_list_append(sorted_list,start->data);
        start=start->next;
    }
    sorted_list=g_list_append(sorted_list,start->data);

    sorted_list=g_list_sort(sorted_list,histogram_list_compare_func_cycles);

    start=sorted_list;
    stop=start;
    while(stop->next!=NULL)
	stop=stop->next;


    chc_start=(struct cycle_histogram_counter*)start->data;
    chc_stop=(struct cycle_histogram_counter*)stop->data;

    while(start!=stop)
    {
	if(count_sum>=0)
	{
	    // Move start to right
	    start = start->next;
	    count_sum-=chc_start->count;
	    chc_start=(struct cycle_histogram_counter*)start->data;
            continue;
	}
	else
	{
	    // Move stop to left
	    stop=stop->prev;
	    count_sum+=chc_stop->count;
	    chc_stop=(struct cycle_histogram_counter*)stop->data;
            continue;
	}
    }

    if(count_sum>chc_start->count)
    {
        start=start->next;
	chc_start=(struct cycle_histogram_counter*)start->data;
	g_list_free(sorted_list);
	return chc_start->cycles;
    }
    if(count_sum<-chc_start->count)
    {
        start=start->prev;
	chc_start=(struct cycle_histogram_counter*)start->data;
	g_list_free(sorted_list);
	return chc_start->cycles;
    }
    if(-count_sum==chc_start->count)
    {
        stop=stop->prev;
	chc_stop=(struct cycle_histogram_counter*)stop->data;
	g_list_free(sorted_list);
	return (chc_start->cycles+chc_stop->cycles)/2.0;
    }
    if(count_sum==chc_start->count)
    {
        stop=stop->next;
	chc_stop=(struct cycle_histogram_counter*)stop->data;
	g_list_free(sorted_list);
	return (chc_start->cycles+chc_stop->cycles)/2.0;
    }
    if(abs(count_sum)<chc_start->count)
    {
	g_list_free(sorted_list);
	return chc_start->cycles;
    }

    assert(0);
    return 0.0;
}

float calculate_stddev(GList *start, GList *stop, float average)
{
    float variance;
    int count=0;
    float sum=0;
    struct cycle_histogram_counter *chc_start, *chc_stop;

    if(start==stop)
	return 0.0;

    if(stop==NULL)
    {
        stop=start;
	while(stop->next!=NULL)
	    stop=stop->next;
    }

    while(start!=stop)
    {
	float diff, diff2;

	chc_start=(struct cycle_histogram_counter*)start->data;
	chc_stop=(struct cycle_histogram_counter*)stop->data;

	diff=chc_start->cycles-average;

	diff2=diff*diff;

	sum+=diff2*chc_start->count;

	count+=chc_start->count;

	start=start->next;
    }

    variance=sum/count;
    return sqrt(variance);
}


void ProfileWindow_update(Profile_Window *pw)
{
  GUI_Processor *gp;
  int i;

  char count_string[100];
  GList *iter;

  if(  (pw == NULL)  || (!((GUI_Object*)pw)->enabled))
    return;

  // Get the pointer to the `gui processor' structure
  gp = ((GUI_Object*)pw)->gp;

  if(gp==NULL || gp->pic_id==0)
  {
      puts("Warning gp or gp->pic_id == NULL in ProfileWindow_update");
      return;
  }

  // Update profile list
  iter=pw->profile_list;
  while(iter)
  {
      struct profile_entry *entry;
      guint64 count;

      entry=(struct profile_entry*)iter->data;

      count=gpsim_get_cycles_used(gp->pic_id,entry->address);

      if(entry->last_count!=count)
      {
	  int row;

	  entry->last_count=count;
	  row=gtk_clist_find_row_from_data(GTK_CLIST(pw->profile_clist),entry);
	  if(row==-1)
	  {
	      puts("\n\nwhooopsie\n");
	      break;
	  }

	  sprintf(count_string,"0x%Lx",count);
	  gtk_clist_set_text (GTK_CLIST(pw->profile_clist),row,1,count_string);
      }
      iter=iter->next;
  }
  gtk_clist_sort(pw->profile_clist);


  // Update range list
  iter=pw->profile_range_list;

  while(iter)
  {
      struct profile_range_entry *range_entry;
      guint64 count;
      range_entry=(struct profile_range_entry*)iter->data;

      count=0;
      for(i=range_entry->startaddress;i<range_entry->endaddress;i++)
      {
	  count+=gpsim_get_cycles_used(gp->pic_id,i);
      }

      if(range_entry->last_count!=count)
      {
	  int row;

	  range_entry->last_count=count;
	  row=gtk_clist_find_row_from_data(GTK_CLIST(pw->profile_range_clist),range_entry);
	  if(row==-1)
	  {
	      puts("\n\nwhooopsie\n");
	      break;
	  }

	  sprintf(count_string,"0x%Lx",count);
	  gtk_clist_set_text (GTK_CLIST(pw->profile_range_clist),row,2,count_string);
      }
      iter=iter->next;
  }
  gtk_clist_sort(pw->profile_range_clist);

  // Update register list
  iter=pw->profile_register_list;
  while(iter)
  {
      struct profile_register_entry *register_entry;
      guint64 count_read, count_write;

      register_entry=(struct profile_register_entry*)iter->data;

      count_read=gpsim_get_register_read_accesses(gp->pic_id,REGISTER_RAM,register_entry->address);
      count_write=gpsim_get_register_write_accesses(gp->pic_id,REGISTER_RAM,register_entry->address);

      if(register_entry->last_count_read!=count_read||
	 register_entry->last_count_write!=count_write)
      {
	  int row;

	  register_entry->last_count_read=count_read;
	  register_entry->last_count_write=count_write;
	  row=gtk_clist_find_row_from_data(GTK_CLIST(pw->profile_register_clist),register_entry);
	  if(row==-1)
	  {
	      puts("\n\nwhooopsie\n");
	      break;
	  }

	  sprintf(count_string,"0x%Lx",count_read);
	  gtk_clist_set_text (GTK_CLIST(pw->profile_register_clist),row,2,count_string);
	  sprintf(count_string,"0x%Lx",count_write);
	  gtk_clist_set_text (GTK_CLIST(pw->profile_register_clist),row,3,count_string);
      }
      iter=iter->next;
  }

  // Update cummulative statistics list
  pw->histogram_profile_list = g_list_sort(pw->histogram_profile_list,
					   histogram_list_compare_func);
  // Remove all of clist (for now)
  gtk_clist_freeze(GTK_CLIST(pw->profile_exestats_clist));
  gtk_clist_clear(GTK_CLIST(pw->profile_exestats_clist));
  if(pw->histogram_profile_list!=NULL)
  {
      struct cycle_histogram_counter *chc;
      int count_sum=0;
      unsigned int start=0xffffffff, stop=0xffffffff;
      guint64 min=0xffffffffffffffffULL, max=0;
      guint64 cycles_sum=0;
      GList *list_start=NULL, *list_end=NULL;
	char fromaddress_string[100]="";
	char toaddress_string[100]="";
	char executions_string[100]="";
	char min_string[100]="";
	char max_string[100]="";
	char median_string[100]="";
	char average_string[100]="";
	char stddev_string[100]="";
	char total_string[100]="";
	char *entry[PROFILE_EXESTATS_COLUMNS]={
	    fromaddress_string,
	    toaddress_string,
	    executions_string,
            min_string,
	    max_string,
            median_string,
	    average_string,
            stddev_string,
            total_string
	};

	iter=pw->histogram_profile_list;
        list_start = iter;
      while(iter!=NULL)
      {
	  chc=(struct cycle_histogram_counter*)iter->data;

	  if(start==chc->start_address &&
	     stop==chc->stop_address)
	  {
	      // Add data to statistics

	      count_sum+=chc->count;
	      if(chc->cycles<min)
		  min=chc->cycles;
	      if(chc->cycles>max)
		  max=chc->cycles;
              cycles_sum+=chc->cycles*chc->count;
	  }
	  else
	  {
	      if(count_sum!=0)
	      {
		  // We have data, display it.
		  sprintf(fromaddress_string,"0x%04x",start);
		  sprintf(toaddress_string,"0x%04x",stop);
		  sprintf(executions_string,"%d",count_sum);
		  sprintf(min_string,"%ld",(long)min);
		  sprintf(max_string,"%ld",(long)max);
		  sprintf(median_string,"%.1f", calculate_median(list_start,list_end));
                  sprintf(average_string,"%.1f",cycles_sum/(float)count_sum);
		  sprintf(stddev_string,"%.1f",calculate_stddev(list_start,list_end,cycles_sum/(float)count_sum));
                  sprintf(total_string,"%d",(int)cycles_sum);
		  gtk_clist_append(GTK_CLIST(pw->profile_exestats_clist),entry);
	      }

	      // Start new calculation
	      count_sum=chc->count;
	      start = chc->start_address;
	      stop = chc->stop_address;
	      min=chc->cycles;
	      max=chc->cycles;
	      cycles_sum=chc->cycles*chc->count;
	      list_start = iter;

	  }
          list_end=iter;
          iter=iter->next;
      }
      // add current to clist

      sprintf(fromaddress_string,"0x%04x",start);
      sprintf(toaddress_string,"0x%04x",stop);
      sprintf(executions_string,"%d",count_sum);
      sprintf(min_string,"%ld",(long)min);
      sprintf(max_string,"%ld",(long)max);
      sprintf(median_string,"%.1f", calculate_median(list_start,list_end));
      sprintf(average_string,"%.1f",cycles_sum/(float)count_sum);
      sprintf(stddev_string,"%.1f",calculate_stddev(list_start,list_end,cycles_sum/(float)count_sum));
      sprintf(total_string,"%d",(int)cycles_sum);
      gtk_clist_append(GTK_CLIST(pw->profile_exestats_clist),entry);
  }

  // print debug info:
/*  iter=pw->histogram_profile_list;
  while(iter!=NULL)
  {
      struct cycle_histogram_counter *chc;
      chc=(struct cycle_histogram_counter*)iter->data;
      printf("start %d, stop %d, cycles %d, count %d\n",
	     (int)chc->start_address,
	     (int)chc->stop_address,
	     (int)chc->cycles,
	     (int)chc->count);
      iter=iter->next;
  }*/

  gtk_clist_thaw(GTK_CLIST(pw->profile_exestats_clist));

}

#define END_OF_TIME 0xFFFFFFFFFFFFFFFFULL
static guint64 startcycle=END_OF_TIME;
static unsigned int startaddress;
static guint64 stopcycle=END_OF_TIME;
static unsigned int stopaddress;

void ProfileWindow_notify_start_callback(Profile_Window *pw)
{
    if(startcycle==END_OF_TIME)
    {
	startcycle=gpsim_get_cycles(((GUI_Object*)pw)->gp->pic_id);
	startaddress=gpsim_get_pc_value(((GUI_Object*)pw)->gp->pic_id);
    }
}

void ProfileWindow_notify_stop_callback(Profile_Window *pw)
{
    if(stopcycle==END_OF_TIME && startcycle!=END_OF_TIME)
    {
	stopcycle=gpsim_get_cycles(((GUI_Object*)pw)->gp->pic_id);
	if(startcycle==stopcycle)
	{
	    // This was probably an attempt to measure the whole loop.
	    // Set stopcycle to unset, and wait for the next one
	    stopcycle=END_OF_TIME;
	}
	else
	{
	    guint64 cycles;
            GList *iter;
	    stopaddress=gpsim_get_pc_value(((GUI_Object*)pw)->gp->pic_id);
	    // We have a new measurement
            cycles=(int)stopcycle-(int)startcycle;

	    // Search to see if there are an entry with this startaddress,
            // stopaddress and cycle count.
	    iter=pw->histogram_profile_list;
	    while(iter!=NULL)
	    {
		struct cycle_histogram_counter *chc;
		chc=(struct cycle_histogram_counter*)iter->data;
		if(chc->start_address == startaddress &&
		   chc->stop_address == stopaddress &&
		   chc->cycles == cycles)
		{
		    // If so then add 1 to the counter
		    chc->count++;
                    break;
		}
                iter=iter->next;
	    }
	    if(iter==NULL)
	    {
		// Else malloc a new struct, fill with values and add (sorted) to list
		struct cycle_histogram_counter *chc;

		chc=(struct cycle_histogram_counter*)malloc(sizeof(struct cycle_histogram_counter));
		chc->start_address=startaddress;
		chc->stop_address=stopaddress;
		chc->cycles=cycles;
		chc->count=1;

		pw->histogram_profile_list=g_list_append(pw->histogram_profile_list,chc);
	    }

	    startcycle=stopcycle=END_OF_TIME;
	}
    }
}

/*****************************************************************
 * ProfileWindow_new_program
 *
 * 
 */

void ProfileWindow_new_program(Profile_Window *pw, GUI_Processor *gp)
{
    int row;
    int i;
    int pic_id;

    if(pw == NULL || gp == NULL)
	return;

    pw->program=1;
    
    if( !((GUI_Object*)pw)->enabled)
	return;
    
    pic_id = gp->pic_id;

    gpsim_enable_profiling(gp->pic_id);

    // Instruction clist
    gtk_clist_freeze(pw->profile_clist);
    for(i=0; i < gpsim_get_program_memory_size(gp->pic_id); i++)
    {
	struct profile_entry *profile_entry;
	char buf[100];
	char address_string[100];
	char instruction_string[100];
	char count_string[100];
	char *entry[PROFILE_COLUMNS]={address_string,count_string,instruction_string};
	guint64 cycles;

	if(gpsim_address_has_opcode( gp->pic_id, i))
	{
	    sprintf(address_string,"0x%04x",i);
	    strcpy(instruction_string,gpsim_get_opcode_name( gp->pic_id, i,buf));

	    cycles=gpsim_get_cycles_used(gp->pic_id,i);
	    sprintf(count_string,"0x%Lx",cycles);

	    row=gtk_clist_append(GTK_CLIST(pw->profile_clist), entry);

	    // FIXME this memory is never freed?
	    profile_entry = (struct profile_entry*)malloc(sizeof(struct profile_entry));
	    profile_entry->address=i;
	    profile_entry->pic_id=pic_id;
//	profile_entry->type=type;
	    profile_entry->last_count=cycles;

	    gtk_clist_set_row_data(GTK_CLIST(pw->profile_clist), row, (gpointer)profile_entry);

	    pw->profile_list = g_list_append(pw->profile_list, (gpointer)profile_entry);
	}
    }
    gtk_clist_thaw(pw->profile_clist);

    // Register clist
    gtk_clist_freeze(pw->profile_register_clist);
    for(i=0; i < gpsim_get_register_memory_size(gp->pic_id,REGISTER_RAM); i++)
    {
	struct profile_register_entry *profile_register_entry;
	char address_string[100];
	char count_string_read[100];
	char count_string_write[100];
	char register_string[100];
	char *entry_register[PROFILE_REGISTER_COLUMNS]={address_string,register_string,count_string_read,count_string_write};
	guint64 read_cycles;
	guint64 write_cycles;
        char *name;

	if(!gpsim_register_is_sfr(gp->pic_id, REGISTER_RAM, i)&&
	   !gpsim_register_is_alias(gp->pic_id, REGISTER_RAM, i)&&
	   gpsim_register_is_valid(gp->pic_id, REGISTER_RAM, i))
	{
	    sprintf(address_string,"0x%04x",i);
	    name = gpsim_get_register_name( gp->pic_id, REGISTER_RAM, i);
	    if(name==NULL)
		name = address_string;
	    strcpy(register_string, name);

	    read_cycles=gpsim_get_register_read_accesses(gp->pic_id,REGISTER_RAM,i);
	    sprintf(count_string_read,"0x%Lx",read_cycles);

	    write_cycles=gpsim_get_register_write_accesses(gp->pic_id,REGISTER_RAM,i);
	    sprintf(count_string_write,"0x%Lx",write_cycles);

	    row=gtk_clist_append(GTK_CLIST(pw->profile_register_clist), entry_register);

	    // FIXME this memory is never freed?
	    profile_register_entry = (struct profile_register_entry*) malloc(sizeof(struct profile_register_entry));
	    profile_register_entry->address=i;
	    profile_register_entry->pic_id=pic_id;
	    profile_register_entry->last_count_read=read_cycles;
	    profile_register_entry->last_count_read=write_cycles;

	    gtk_clist_set_row_data(GTK_CLIST(pw->profile_register_clist), row, (gpointer)profile_register_entry);

	    pw->profile_register_list = g_list_append(pw->profile_register_list, (gpointer)profile_register_entry);
	}
    }
    gtk_clist_thaw(pw->profile_register_clist);

}

/*****************************************************************
 * ProfileWindow_new_processor
 *
 * 
 */

void ProfileWindow_new_processor(Profile_Window *pw, GUI_Processor *gp)
{

#define NAME_SIZE 32

    int pic_id;


    if(pw == NULL || gp == NULL)
	return;

    pw->processor=1;
    
    if( !((GUI_Object*)pw)->enabled)
	return;
    
    pw->gui_obj.gp = gp;
    pic_id = gp->pic_id;

}

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Register_Window *rw)
{
  ((GUI_Object *)rw)->change_view((GUI_Object*)rw,VIEW_HIDE);
  return TRUE;
}

gdouble gaussian(GtkPlot *plot, GtkPlotData *data, gdouble x, gboolean *err)
{
 gdouble y;
 *err = FALSE;
 y = 1000*x;//.65*exp(-.5*pow(x-.5,2)/.02);

 return y;
}

int
BuildProfileWindow(Profile_Window *pw)
{
  GtkWidget *window;
  GtkWidget *profile_clist;
  GtkWidget *profile_register_clist;
  GtkWidget *profile_range_clist;
  GtkWidget *profile_exestats_clist;
  GtkWidget *label;
  GtkWidget *main_vbox;
  GtkWidget *scrolled_window;
    
  gint column_width,char_width;

  int x,y,width,height;
  
	
  if(pw==NULL)
  {
      printf("Warning build_profile_viewer(%p)\n",pw);
      return 0;
  }

//  gui_message("There are bugs here in the profile viewer.\n\
//	      Please help them get reported and/or fixed.");
	
  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), pw);

  ((GUI_Object*)pw)->window=window;

//  gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
//			     GTK_SIGNAL_FUNC (gtk_widget_destroyed), GTK_OBJECT(window));

  main_vbox=gtk_vbox_new(FALSE,1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox),0); 
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  gtk_window_set_title(GTK_WINDOW(window), "profile viewer");

  pw->notebook = gtk_notebook_new();
  gtk_widget_show(pw->notebook);

  gtk_box_pack_start (GTK_BOX (main_vbox), pw->notebook, TRUE, TRUE, 0);


  // Instruction profile clist
  profile_clist=gtk_clist_new_with_titles(PROFILE_COLUMNS,profile_titles);
  pw->profile_clist = GTK_CLIST(profile_clist);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_clist),0,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_clist),1,TRUE);
//  gtk_clist_set_sort_column (pw->profile_clist,1);
//  gtk_clist_set_sort_type (pw->profile_clist,GTK_SORT_DESCENDING);
  gtk_clist_set_compare_func(GTK_CLIST(pw->profile_clist),
			     (GtkCListCompareFunc)profile_compare_func);

  GTK_WIDGET_UNSET_FLAGS(profile_clist,GTK_CAN_DEFAULT);

  scrolled_window=gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_clist);
  
  gtk_widget_show(profile_clist);

  gtk_widget_show(scrolled_window);

//  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  label=gtk_label_new("Instruction profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(pw->notebook),scrolled_window,label);
  ///////////////////////////////////////////////////
  ///////////////////////////////////////////////////


  // Instruction range profile clist
  profile_range_clist=gtk_clist_new_with_titles(PROFILE_RANGE_COLUMNS,profile_range_titles);
  pw->profile_range_clist = GTK_CLIST(profile_range_clist);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_range_clist),0,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_range_clist),1,TRUE);
  gtk_clist_set_sort_column (pw->profile_range_clist,2);
  gtk_clist_set_sort_type (pw->profile_range_clist,GTK_SORT_DESCENDING);
  gtk_clist_set_compare_func(GTK_CLIST(pw->profile_range_clist),
			     (GtkCListCompareFunc)profile_compare_func);

  GTK_WIDGET_UNSET_FLAGS(profile_range_clist,GTK_CAN_DEFAULT);
    
  pw->range_popup_menu=build_menu(pw);

  gtk_signal_connect(GTK_OBJECT(pw->profile_range_clist),
		     "button_press_event",
		     (GtkSignalFunc) do_popup,
		     pw);
  gtk_signal_connect(GTK_OBJECT(pw->profile_range_clist),"key_press_event",
		     (GtkSignalFunc) key_press,
		     (gpointer) pw);
  gtk_signal_connect(GTK_OBJECT(pw->profile_range_clist),"select_row",
		     (GtkSignalFunc)profile_range_list_row_selected,pw);

  scrolled_window=gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_range_clist);
  
  gtk_widget_show(profile_range_clist);

  gtk_widget_show(scrolled_window);

  label=gtk_label_new("Instruction range profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(pw->notebook),scrolled_window,label);

  ///////////////////////////////////////////////////


  // Register profile clist
  profile_register_clist=gtk_clist_new_with_titles(PROFILE_REGISTER_COLUMNS,profile_register_titles);
  pw->profile_register_clist = GTK_CLIST(profile_register_clist);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_register_clist),0,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_register_clist),1,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_register_clist),2,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_register_clist),3,TRUE);
//  gtk_clist_set_sort_column (pw->profile_register_clist,1);
//  gtk_clist_set_sort_type (pw->profile_register_clist,GTK_SORT_DESCENDING);
  gtk_clist_set_compare_func(GTK_CLIST(pw->profile_register_clist),
			     (GtkCListCompareFunc)profile_compare_func);

  GTK_WIDGET_UNSET_FLAGS(profile_register_clist,GTK_CAN_DEFAULT);
    
  scrolled_window=gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_register_clist);
  
  gtk_widget_show(profile_register_clist);

  gtk_widget_show(scrolled_window);

//  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  label=gtk_label_new("Register profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(pw->notebook),scrolled_window,label);

  ///////////////////////////////////////////////////


  // Execution time statistics tab
  profile_exestats_clist=gtk_clist_new_with_titles(PROFILE_EXESTATS_COLUMNS,profile_exestats_titles);
  pw->profile_exestats_clist = GTK_CLIST(profile_exestats_clist);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_exestats_clist),0,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_exestats_clist),1,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_exestats_clist),2,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_exestats_clist),3,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_exestats_clist),4,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_exestats_clist),5,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_exestats_clist),6,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_exestats_clist),7,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_exestats_clist),8,TRUE);

  GTK_WIDGET_UNSET_FLAGS(profile_exestats_clist,GTK_CAN_DEFAULT);

  pw->exestats_popup_menu=exestats_build_menu(pw);
  gtk_signal_connect(GTK_OBJECT(pw->profile_exestats_clist),
		     "button_press_event",
		     (GtkSignalFunc) exestats_do_popup,
		     pw);

  scrolled_window=gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_exestats_clist);
  
  gtk_widget_show(profile_exestats_clist);

  gtk_widget_show(scrolled_window);

  label=gtk_label_new("Routine profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(pw->notebook),scrolled_window,label);
  ///////////////////////////////////////////////////


  width=((GUI_Object*)pw)->width;
  height=((GUI_Object*)pw)->height;
  x=((GUI_Object*)pw)->x;
  y=((GUI_Object*)pw)->y;
  gtk_window_set_default_size(GTK_WINDOW(pw->gui_obj.window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(pw->gui_obj.window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(pw->gui_obj.window),pw->gui_obj.name,"Gpsim");

  normal_style = gtk_style_new ();
  char_width = gdk_string_width (normal_style->font,"9");
  column_width = 3 * char_width + 6;

  gtk_signal_connect_after(GTK_OBJECT(pw->gui_obj.window), "configure_event",
  			   GTK_SIGNAL_FUNC(gui_object_configure_event),pw);



  gtk_widget_show (window);


  pw->gui_obj.enabled=1;
  pw->gui_obj.is_built=1;

  if(pw->processor)
      ProfileWindow_new_processor(pw, ((GUI_Object*)pw)->gp);

  if(pw->program)
      ProfileWindow_new_program(pw, ((GUI_Object*)pw)->gp);

  ProfileWindow_update(pw);

  update_menu_item((GUI_Object*)pw);




  return 0;
}

int CreateProfileWindow(GUI_Processor *gp)
{
  Profile_Window *profile_window;

  profile_window = (Profile_Window *)malloc(sizeof(Profile_Window));

  profile_window->gui_obj.gp = gp;
  profile_window->gui_obj.name = "profile";
  profile_window->gui_obj.window = NULL;
  profile_window->gui_obj.wc = WC_data;
  profile_window->gui_obj.wt = WT_profile_window;
  profile_window->gui_obj.change_view = SourceBrowser_change_view;//change_view;
  profile_window->gui_obj.is_built = 0;
  profile_window->profile_list=NULL;
  profile_window->profile_range_list=NULL;
  profile_window->profile_register_list=NULL;
  profile_window->histogram_profile_list=NULL;

  gp->profile_window = profile_window;

  profile_window->processor=0;
  profile_window->program=0;

  gp->add_window_to_list((GUI_Object *)profile_window);

  profile_window->gui_obj.get_config();

  if(profile_window->gui_obj.enabled)
      BuildProfileWindow(profile_window);

  return 1;
}

#endif // HAVE_GUI
