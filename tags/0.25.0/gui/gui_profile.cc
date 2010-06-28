/*
   Copyright (C) 1998,1999,2000,2001,2002,2003
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
#include <errno.h>

#include "../config.h"

//#define GTKEXTRA_2

#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>
#include <math.h>

#include <assert.h>

#include "../src/sim_context.h"
#include "../src/interface.h"
#include "../src/errors.h"

#include "gui.h"
#include "gui_profile.h"
#include "gui_regwin.h"

#include "../src/symbol.h"

#include <gtkextra/gtkplot.h>
#include <gtkextra/gtkplotdata.h>
#include <gtkextra/gtkplotcanvas.h>
#ifdef GTKEXTRA_2
#include <gtkextra/gtkplotcanvasplot.h>
#endif
#include <gtkextra/gtkplotbar.h>
#include <gtkextra/gtkplotps.h>
#include <gtkextra/gtkplotprint.h>

#define PROFILE_COLUMNS    3
static gchar *profile_titles[PROFILE_COLUMNS]={"Address", "Cycles","Instruction"};

#define PROFILE_RANGE_COLUMNS    3
static gchar *profile_range_titles[PROFILE_RANGE_COLUMNS]={"Start address", "End address", "Cycles"};

#define PROFILE_REGISTER_COLUMNS    4
static gchar *profile_register_titles[PROFILE_REGISTER_COLUMNS]={"Address", "Register", "Read count", "Write count"};

#define PROFILE_EXESTATS_COLUMNS    9
static gchar *profile_exestats_titles[PROFILE_EXESTATS_COLUMNS]={"From address", "To address", "Executions", "Min", "Max",  "Median", "Average", "Std. Dev.", "Total"};

struct profile_entry {
    unsigned int address;
    guint64 last_count;
};

struct profile_range_entry {
    char startaddress_text[64];
    char endaddress_text[64];
    unsigned int startaddress;
    unsigned int endaddress;
    guint64 last_count;
};

struct profile_register_entry {
    unsigned int address;
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
    const char *name;
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

#if 0 // defined but not used
static menu_item plot_menu_items[] = {
    {"Save postscript...", MENU_SAVE_PS},
    {"Print", MENU_PRINT},
};
#endif

static menu_item exestats_menu_items[] = {
    {"Plot distribution", MENU_PLOT},
};


extern int gui_message(const char *message);

static GtkStyle *normal_style;

int plot_profile(Profile_Window *pw, char **pointlabel, guint64 *cyclearray, int numpoints);
int plot_routine_histogram(Profile_Window *pw);
float calculate_stddev(GList *start, GList *stop, float average);
double calculate_median(GList *start, GList *stop);

// Used only in popup menus
Profile_Window *popup_pw;


//========================================================================
class ProfileEntry : public GUIRegister {
public:

  Processor *cpu;
  unsigned int address;
  guint64 last_count;

};

//========================================================================
static void remove_entry(Profile_Window *pw, struct profile_entry *entry)
{
    gtk_clist_remove(GTK_CLIST(pw->profile_range_clist),pw->range_current_row);
    pw->profile_range_list=g_list_remove(pw->profile_range_list,entry);
    free(entry);
}

#if 0 // defined but not used
static unsigned int lookup_address_symbol(const char *name)
{
  Symbol_Table &st = CSimulationContext::GetContext()->GetSymbolTable();
  Value *pValue = st.find(name);
  if(pValue != NULL) {
    int i;
    pValue->get(i);
    return i;
  }
  return UINT_MAX;
}

static void add_range(Profile_Window *pw,
                      const char *startaddress_text,
                      const char *endaddress_text)
{
    guint64 gcycles;
    struct profile_range_entry *profile_range_entry;
    unsigned int startaddress;
    unsigned int endaddress;
    char count_string[100];
    char *entry[PROFILE_COLUMNS]={(char *)startaddress_text,(char *)endaddress_text,count_string};
    int row;
    unsigned int i;

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

    gcycles=0;
    for(i=startaddress;i<endaddress;i++)
    {
      gcycles+=gpGuiProcessor->cpu->cycles_used(i);
    }
    sprintf(count_string,"0x%" PRINTF_INT64_MODIFIER "x",gcycles);

    row=gtk_clist_append(GTK_CLIST(pw->profile_range_clist), entry);

    // FIXME this memory is never freed?
    profile_range_entry = (struct profile_range_entry*)malloc(sizeof(struct profile_range_entry));
    strcpy(profile_range_entry->startaddress_text,startaddress_text);
    strcpy(profile_range_entry->endaddress_text,endaddress_text);
    profile_range_entry->startaddress=startaddress;
    profile_range_entry->endaddress=endaddress;
    profile_range_entry->last_count=gcycles;

    gtk_clist_set_row_data(GTK_CLIST(pw->profile_range_clist), row, (gpointer)profile_range_entry);

    pw->profile_range_list = g_list_append(pw->profile_range_list, (gpointer)profile_range_entry);

    gtk_clist_sort(GTK_CLIST(pw->profile_range_clist));
}

static void a_cb(GtkWidget *w, gpointer user_data)
{
  *(int*)user_data=TRUE;
}

static void b_cb(GtkWidget *w, gpointer user_data)
{
  *(int*)user_data=FALSE;
}

static void add_range_dialog(Profile_Window *pw)
{
    static GtkWidget *dialog=0;
    GtkWidget *button;
    GtkWidget *hbox;
    GtkWidget *label;
    static GtkWidget *startentry;
    static GtkWidget *endentry;
    int retval=-1;

    if(dialog==0)
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

    if(retval==(int)TRUE)
    {
        // Add range.

        const gchar *startentry_text;
        const gchar *endentry_text;

        startentry_text = gtk_entry_get_text(GTK_ENTRY(startentry));
        if(*startentry_text!='\0')
        {
            endentry_text = gtk_entry_get_text(GTK_ENTRY(endentry));
            if(*endentry_text!='\0')
            {
                add_range(pw, startentry_text, endentry_text);
            }
        }
    }

    return;
}

/*
 this function compares sym pointers for g_list_sort()
 */
static gint
symcompare(Value *sym1, Value *sym2)
{
  try {
    int i1,i2;

    sym1->get(i1);
    sym2->get(i2);
    if(i1 < i2)
      return -1;

    if(i1 > i2)
      return 1;
  }
  catch (Error *e) {

    delete e;
  }
  return 0;
}

static void
file_selection_ok (GtkWidget        *w,
                   GtkFileSelection *fs)
{
    const char *file;

    file=gtk_file_selection_get_filename (fs);
#if GTK_MAJOR_VERSION >= 2
    gtk_plot_canvas_export_ps(GTK_PLOT_CANVAS(popup_pw->plot_canvas), (char *)file,
                              GTK_PLOT_PORTRAIT, 0, GTK_PLOT_LETTER);
#else
    gtk_plot_canvas_export_ps(GTK_PLOT_CANVAS(popup_pw->plot_canvas), (char *)file, 0, 0,
                              GTK_PLOT_LETTER);
#endif

    gtk_widget_hide (GTK_WIDGET (fs));
}

static void
print_plot (Profile_Window *pw)
{
    char *file;
    char cmd[200];

    file=tempnam("/tmp","gpsimplot");
#if GTK_MAJOR_VERSION >= 2
    gtk_plot_canvas_export_ps(GTK_PLOT_CANVAS(popup_pw->plot_canvas), file,
                              GTK_PLOT_PORTRAIT, 0, GTK_PLOT_LETTER);
#else
    gtk_plot_canvas_export_ps(GTK_PLOT_CANVAS(popup_pw->plot_canvas), file, 0, 0,
                              GTK_PLOT_LETTER);
#endif
    sprintf(cmd,"lpr %s",file);
    system(cmd);
    remove(file);
}

extern int gui_question(char *question, char *a, char *b);

static GtkItemFactoryCallback
open_plotsave_dialog(Profile_Window *pw)
{
    static GtkWidget *window = 0;

    if (!window)
    {

        window = gtk_file_selection_new ("Save postscript to file...");

        gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (window));

        gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

        gtk_signal_connect_object(GTK_OBJECT(window),
                                  "delete_event",
                                  GTK_SIGNAL_FUNC(gtk_widget_hide),
                                  GTK_OBJECT(window));
//      gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
//                                 GTK_SIGNAL_FUNC(gtk_widget_destroyed),
//                                 GTK_OBJECT(window));

        gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
                            "clicked", GTK_SIGNAL_FUNC(file_selection_ok),
                            window);
        gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
                                   "clicked", GTK_SIGNAL_FUNC(gtk_widget_hide),
                                   GTK_OBJECT (window));
    }
    gtk_widget_show (window);
    return 0;
}


// called when user has selected a menu item in plot window
static void
plot_popup_activated(GtkWidget *widget, gpointer data)
{
    menu_item *item;
    struct profile_entry *entry;

    if(widget==0 || data==0)
    {
        printf("Warning plot_popup_activated(%p,%p)\n",widget,data);
        return;
    }

    item = (menu_item *)data;

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
#endif

// called when user has selected a menu item in exestats tab
static void
exestats_popup_activated(GtkWidget *widget, gpointer data)
{
    menu_item *item;

    if(widget==0 || data==0)
    {
        printf("Warning exestats_popup_activated(%p,%p)\n",widget,data);
        return;
    }

    item = (menu_item *)data;

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

#if 0 // defined but not used
// called from plot_do_popup
static GtkWidget *
plot_build_menu(Profile_Window *pw)
{
  GtkWidget *menu;
  GtkWidget *item;
  unsigned int i;


  if(pw==0)
  {
      printf("Warning build_menu(%p)\n",pw);
      return 0;
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
#endif

// called from exestats_do_popup
static GtkWidget *
exestats_build_menu(Profile_Window *pw)
{
  GtkWidget *menu;
  GtkWidget *item;
  unsigned int i;


  if(pw==0)
  {
      printf("Warning build_menu(%p)\n",pw);
      return 0;
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

    if(widget==0 || event==0 || pw==0)
    {
        printf("Warning exestats_popup(%p,%p,%p)\n",widget,event,pw);
        return 0;
    }

    popup=pw->exestats_popup_menu;

    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
                     3, event->time);
    }
    return FALSE;
}

#if 0 // defined but not used
// button press handler
static gint
plot_do_popup(GtkWidget *widget, GdkEventButton *event, Profile_Window *pw)
{

    GtkWidget *popup;

    if(widget==0 || event==0 || pw==0)
    {
        printf("Warning do_popup(%p,%p,%p)\n",widget,event,pw);
        return 0;
    }

    popup=pw->plot_popup_menu;

    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
                     3, event->time);
    }
    return FALSE;
}
#endif

int plot_profile(Profile_Window *pw, char **pointlabel, guint64 *cyclearray, int numpoints)
{

#if 0
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
    time_t t;

    static int has_old_graph=0;
    static int last_numpoints=0;

    if(pw->gp->cpu->program_memory_size() <=0)
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


    if(!pw || !pw->gp || !pw->gp->cpu)
      return 0;

    t=time(0);

    // Compute module name to put in infostring
    for(i=0;i<pw->gp->cpu->files.nsrc_files();i++)
    {
      //struct file_context *gpsim_file;
      FileContext *fc = pw->gp->cpu->files[i];

      const char *file_name;

      if(fc)
        file_name = fc->name().c_str();
      else
        continue;


      //gpsim_file = &(gp->cpu->files[i]);
      //file_name = gpsim_file->name;
        if(!strcmp(file_name+strlen(file_name)-4,".asm")
           ||!strcmp(file_name+strlen(file_name)-4,".ASM")
           ||!strcmp(file_name+strlen(file_name)-4,".hex")
           ||!strcmp(file_name+strlen(file_name)-4,".HEX")
          )
        {
            strncpy(filename,file_name,strlen(file_name)-4);
            filename[strlen(file_name)-4]=0;
            break;
        }
    }


    // This information is put at the top of the plot
    sprintf(infostring,"\\BFile:\\N\"%s\" \\BDate:\\N%s \\BProcessor:\\N\"%s\"",
            filename,
            ctime(&t),
            pw->gp->cpu->name().c_str());

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
//      gtk_signal_connect_object (GTK_OBJECT (window1), "destroy",
//                                 GTK_SIGNAL_FUNC(gtk_widget_destroyed),
//                                 GTK_OBJECT(window1));

        vbox1=gtk_vbox_new(FALSE,0);
        gtk_container_add(GTK_CONTAINER(window1),vbox1);
        gtk_widget_show(vbox1);

        scrollw1=gtk_scrolled_window_new(0, 0);
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

        plot = gtk_plot_new_with_size(0, PLOTWIDTH, PLOTHEIGHT);
        gtk_widget_show(plot);

        active_plot=plot;

        gdk_color_parse("light yellow", &bg_color);
        gdk_colormap_alloc_color(gtk_widget_get_colormap(active_plot), &bg_color, FALSE, TRUE);
        gtk_plot_set_background(GTK_PLOT(active_plot), &bg_color);

        gdk_color_parse("black", &color1);
        gdk_colormap_alloc_color(gtk_widget_get_colormap(active_plot), &color1, FALSE, TRUE);
        gdk_color_parse("black", &color2);
        gdk_colormap_alloc_color(gtk_widget_get_colormap(canvas), &color2, FALSE, TRUE);

#ifdef GTKEXTRA_2
        gtk_plot_hide_legends(GTK_PLOT(active_plot));
        gtk_plot_axis_show_labels(gtk_plot_get_axis(GTK_PLOT(active_plot),GTK_PLOT_AXIS_TOP),0);
        gtk_plot_axis_show_labels(gtk_plot_get_axis(GTK_PLOT(active_plot),GTK_PLOT_AXIS_BOTTOM),0);
        gtk_plot_axis_show_labels(gtk_plot_get_axis(GTK_PLOT(active_plot),GTK_PLOT_AXIS_RIGHT),0);
        gtk_plot_axis_set_visible(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP), TRUE);
        gtk_plot_grids_set_visible(GTK_PLOT(active_plot), TRUE, TRUE, TRUE, TRUE);
        gtk_plot_canvas_put_child(GTK_PLOT_CANVAS(canvas), gtk_plot_canvas_plot_new(GTK_PLOT(active_plot)), PLOTXPOS, PLOTYPOS, PLOTWIDTH, PLOTHEIGHT);
        gtk_plot_axis_hide_title(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP));
        gtk_plot_axis_hide_title(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_BOTTOM));
        gtk_plot_axis_hide_title(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_LEFT));
        gtk_plot_axis_hide_title(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_RIGHT));
#else
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
#endif
        gtk_plot_set_legends_border(GTK_PLOT(active_plot), GTK_PLOT_BORDER_SHADOW, 3);
        gtk_plot_legends_move(GTK_PLOT(active_plot), .58, .05);
        gtk_widget_show(active_plot);



        dataset = GTK_PLOT_DATA(gtk_plot_bar_new(GTK_ORIENTATION_VERTICAL));
        gtk_plot_add_data(GTK_PLOT(active_plot), GTK_PLOT_DATA(dataset));
    }

#ifdef GTKEXTRA_2
    gtk_plot_axis_set_ticks(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_RIGHT), tickdelta, 0);
    gtk_plot_set_range(GTK_PLOT(active_plot), 0., 1., 0., (gdouble)maxy);
    gtk_plot_axis_set_labels_numbers(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_LEFT),
                                     maxy < 10000 ? GTK_PLOT_LABEL_FLOAT : GTK_PLOT_LABEL_EXP, 0);
#else
    gtk_plot_axis_set_ticks(GTK_PLOT(active_plot), GTK_PLOT_AXIS_Y, tickdelta, 0);
    gtk_plot_set_range(GTK_PLOT(active_plot), 0., 1., 0., (gdouble)maxy);
    gtk_plot_axis_set_labels_numbers(GTK_PLOT(active_plot),
                                     GTK_PLOT_AXIS_LEFT,
                                     maxy<10000?0:GTK_PLOT_LABEL_EXP,
                                     0);
#endif

    gtk_plot_data_set_points(GTK_PLOT_DATA(dataset), px2, py2, 0, 0, numpoints);
    gtk_plot_data_set_symbol(GTK_PLOT_DATA(dataset),
                             GTK_PLOT_SYMBOL_NONE,
                             GTK_PLOT_SYMBOL_FILLED,
                             0, 4.0, &color1,&color2);
#if GTK_MAJOR_VERSION >= 2
    gtk_plot_data_set_line_attributes(GTK_PLOT_DATA(dataset),
                                      GTK_PLOT_LINE_NONE,
                                      GDK_CAP_BUTT, GDK_JOIN_MITER,
                                      5, &color2);
#else
    gtk_plot_data_set_line_attributes(GTK_PLOT_DATA(dataset),
                                      GTK_PLOT_LINE_NONE,
                                      5, &color2);
#endif

    gtk_plot_data_set_connector(GTK_PLOT_DATA(dataset), GTK_PLOT_CONNECT_NONE);

    gtk_widget_show(GTK_WIDGET(dataset));

    // Put the description text under each bar in the plot.
    for(i=0;i<numpoints;i++)
    {

        bartext[i]=gtk_plot_put_text(GTK_PLOT(active_plot),
                                     PLOTXPOS+px2[i]*PLOTWIDTH,
                                     PLOTYPOS+PLOTHEIGHT+0.01,
                                     0,
                                     20,
                                     270,
                                     0,
                                     0,
                                     TRUE,
                                     GTK_JUSTIFY_LEFT,
                                     pointlabel[i]);

        gtk_plot_draw_text(GTK_PLOT(active_plot),*bartext[i]);

    }

    infotext=gtk_plot_put_text(GTK_PLOT(active_plot),
                               0.5,
                               PLOTYPOS-0.05,
                               0,
                               20,
                               00,
                               0,
                               0,
                               TRUE,
                               GTK_JUSTIFY_CENTER,
                               infostring);

    gtk_plot_draw_text(GTK_PLOT(active_plot),*infotext);

    gtk_widget_queue_draw(window1);

    gtk_widget_show(window1);

    has_old_graph=1;
    last_numpoints=numpoints;
#endif //
    return 0;
}

int plot_routine_histogram(Profile_Window *pw)
{
#if 0
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
    time_t t;

    static int has_old_graph=0;
    static int last_numpoints=0;
    int numpoints;
    GList *iter;

    if(!pw || !pw->gp || !pw->gp->cpu)
      return 0;

    if(pw->gp->cpu->program_memory_size() <=0)
      return 0;

    if(pw->histogram_profile_list==0)
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
    while(iter!=0)
    {
        numpoints++;
        iter=iter->next;
    }

    px2=(double*)malloc(numpoints*sizeof(double));
    py2=(double*)malloc(numpoints*sizeof(double));

    totalcycles=0;
    totalcount=0;
    // Find values, and put them in the point arrays
    j=0;
    iter=pw->histogram_profile_list;

    while(iter!=0)
    {
        struct cycle_histogram_counter *chc;
        chc=(struct cycle_histogram_counter*)iter->data;

        px2[j]=(double)chc->histo_cycles;
        py2[j]=(double)chc->count;
        if(maxy<chc->count)
            maxy=chc->count;
        if(maxx<chc->histo_cycles)
            maxx=chc->histo_cycles;
        if(minx>chc->histo_cycles)
            minx=chc->histo_cycles;

        totalcycles+=chc->histo_cycles*chc->count;
        totalcount+=chc->count;

        j++;
        iter=iter->next;
    }

    mincycles=minx;
    maxcycles=maxx;
    averagecycles=totalcycles/(float)totalcount;
    mediancycles=calculate_median(pw->histogram_profile_list,0);
    stddevcycles=calculate_stddev(pw->histogram_profile_list,0,averagecycles);

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


    maxy=maxy +(maxy>>3)+1;
    margin = maxx-minx;
    margin= margin + (margin>>3) + (margin>>5)+1;
    maxx=maxx+margin;
    if(minx>margin)
        minx=minx-margin;
    else
        minx=0;

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

        vbox1=gtk_vbox_new(FALSE,0);
        gtk_container_add(GTK_CONTAINER(window1),vbox1);
        gtk_widget_show(vbox1);

        scrollw1=gtk_scrolled_window_new(0, 0);
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

        plot = gtk_plot_new_with_size(0, PLOTWIDTH, PLOTHEIGHT);
        gtk_widget_show(plot);

        active_plot=plot;

        gdk_color_parse("light yellow", &bg_color);
        gdk_colormap_alloc_color(gtk_widget_get_colormap(active_plot), &bg_color, FALSE, TRUE);
        gtk_plot_set_background(GTK_PLOT(active_plot), &bg_color);

        gdk_color_parse("black", &color1);
        gdk_colormap_alloc_color(gtk_widget_get_colormap(active_plot), &color1, FALSE, TRUE);
        gdk_color_parse("black", &color2);
        gdk_colormap_alloc_color(gtk_widget_get_colormap(canvas), &color2, FALSE, TRUE);

#ifdef GTKEXTRA_2
        gtk_plot_hide_legends(GTK_PLOT(active_plot));
        gtk_plot_axis_show_labels(gtk_plot_get_axis(GTK_PLOT(active_plot),GTK_PLOT_AXIS_TOP),0);
        gtk_plot_axis_show_labels(gtk_plot_get_axis(GTK_PLOT(active_plot),GTK_PLOT_AXIS_RIGHT),0);
        gtk_plot_axis_set_visible(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP), TRUE);
        gtk_plot_grids_set_visible(GTK_PLOT(active_plot), TRUE, TRUE, TRUE, TRUE);
        gtk_plot_canvas_put_child(GTK_PLOT_CANVAS(canvas), gtk_plot_canvas_plot_new(GTK_PLOT(active_plot)), PLOTXPOS, PLOTYPOS, PLOTWIDTH, PLOTHEIGHT);
        gtk_plot_axis_hide_title(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP));
        gtk_plot_axis_hide_title(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_RIGHT));
#else
        gtk_plot_hide_legends(GTK_PLOT(active_plot));
        gtk_plot_axis_show_labels(GTK_PLOT(active_plot),GTK_PLOT_AXIS_TOP,0);
        gtk_plot_axis_show_labels(GTK_PLOT(active_plot),GTK_PLOT_AXIS_RIGHT,0);
        gtk_plot_axis_set_visible(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP, TRUE);
        gtk_plot_grids_set_visible(GTK_PLOT(active_plot), TRUE, TRUE, TRUE, TRUE);
        gtk_plot_canvas_add_plot(GTK_PLOT_CANVAS(canvas), GTK_PLOT(active_plot), PLOTXPOS, PLOTYPOS);
        gtk_plot_axis_hide_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_TOP);
        gtk_plot_axis_hide_title(GTK_PLOT(active_plot), GTK_PLOT_AXIS_RIGHT);
#endif
        gtk_plot_set_legends_border(GTK_PLOT(active_plot), GTK_PLOT_BORDER_SHADOW, 3);
        gtk_plot_legends_move(GTK_PLOT(active_plot), .58, .05);
        gtk_widget_show(active_plot);



        dataset = GTK_PLOT_DATA(gtk_plot_bar_new(GTK_ORIENTATION_VERTICAL));
        gtk_plot_add_data(GTK_PLOT(active_plot), GTK_PLOT_DATA(dataset));
    }

#ifdef GTKEXTRA_2
    gtk_plot_axis_set_ticks(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_RIGHT), tickdelta_y, 1);
    gtk_plot_axis_set_title(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_LEFT), "Frequency");
    gtk_plot_axis_set_labels_numbers(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_LEFT),
                                     GTK_PLOT_LABEL_FLOAT, 0);

    gtk_plot_axis_set_ticks(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_LEFT), tickdelta_x, 1);
    gtk_plot_axis_set_title(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_BOTTOM), "Cycles");
    gtk_plot_axis_set_labels_numbers(gtk_plot_get_axis(GTK_PLOT(active_plot), GTK_PLOT_AXIS_BOTTOM),
                                     maxx<10000 ? GTK_PLOT_LABEL_FLOAT : GTK_PLOT_LABEL_EXP, 0);
#else
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
#endif
    gtk_plot_set_range(GTK_PLOT(active_plot), minx, (gdouble)maxx, 0., (gdouble)maxy);

    gtk_plot_data_set_points(GTK_PLOT_DATA(dataset), px2, py2, 0, 0, numpoints);
    gtk_plot_data_set_symbol(GTK_PLOT_DATA(dataset),
                             GTK_PLOT_SYMBOL_NONE,
                             GTK_PLOT_SYMBOL_FILLED,
                             0, 4.0, &color1,&color2);
#if GTK_MAJOR_VERSION >= 2
    gtk_plot_data_set_line_attributes(GTK_PLOT_DATA(dataset),
                                      GTK_PLOT_LINE_NONE,
                                      GDK_CAP_BUTT, GDK_JOIN_MITER,
                                      5, &color2);
#else
    gtk_plot_data_set_line_attributes(GTK_PLOT_DATA(dataset),
                                      GTK_PLOT_LINE_NONE,
                                      5, &color2);
#endif

    gtk_plot_data_set_connector(GTK_PLOT_DATA(dataset), GTK_PLOT_CONNECT_NONE);

    gtk_widget_show(GTK_WIDGET(dataset));


    // Infostring1
    t=time(0);
    // Compute module name to put in infostring
    for(i=0;i<pw->gp->cpu->files.nsrc_files();i++)
    {
      //struct file_context *gpsim_file;
      FileContext *fc = pw->gp->cpu->files[i];

      const char *file_name;

      if(fc)
        file_name = fc->name().c_str();
      else
        continue;


      //gpsim_file = &(gp->cpu->files[i]);
      //file_name = gpsim_file->name;
        if(!strcmp(file_name+strlen(file_name)-4,".asm")
           ||!strcmp(file_name+strlen(file_name)-4,".ASM")
           ||!strcmp(file_name+strlen(file_name)-4,".hex")
           ||!strcmp(file_name+strlen(file_name)-4,".HEX")
          )
        {
            strncpy(filename,file_name,strlen(file_name)-4);
            filename[strlen(file_name)-4]=0;
            break;
        }
    }
    // This information is put at top of the plot
    sprintf(infostring,"\\BFile:\\N\"%s\" \\BDate:\\N%s \\BProcessor:\\N\"%s\"",
            filename,
            ctime(&t),
            pw->gp->cpu->name().c_str());

    // ctime adds a newline. Remove it.
    for(i=0;infostring[i];i++)
        if(infostring[i]=='\n')
            infostring[i]=' ';
    infotext1=gtk_plot_put_text(GTK_PLOT(active_plot),
                                0.5,
                                PLOTYPOS-0.05,
                                0,
                                20,
                                00,
                                0,
                                0,
                                TRUE,
                                GTK_JUSTIFY_CENTER,
                                infostring);
    gtk_plot_draw_text(GTK_PLOT(active_plot),*infotext1);

    static char * pInfoStringFormat =
      "\\BMin:\\N\%" PRINTF_INT64_MODIFIER
      "d \\BMax:\\N%" PRINTF_INT64_MODIFIER
      "d \\BAverage:\\N%.1f \\BMedian:\\N%.1f \\BStandard deviation:\\N%.1f";
    // Infostring2
    sprintf(infostring,pInfoStringFormat, // "\\BMin:\\N\%lld \\BMax:\\N%lld \\BAverage:\\N%.1f \\BMedian:\\N%.1f \\BStandard deviation:\\N%.1f",
            mincycles,
            maxcycles,
            averagecycles,
            mediancycles,
            stddevcycles);
    infotext2=gtk_plot_put_text(GTK_PLOT(active_plot),
                                0.5,
                                PLOTYPOS-0.03,
                                0,
                                20,
                                00,
                                0,
                                0,
                                TRUE,
                                GTK_JUSTIFY_CENTER,
                                infostring);
    gtk_plot_draw_text(GTK_PLOT(active_plot),*infotext2);

    gtk_widget_queue_draw(window1);

    gtk_widget_show(window1);

    has_old_graph=1;
    last_numpoints=numpoints;
#endif
    return 0;
}

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
#if 0

  ////////
  ///
  ///  THIS CODE IS BROKEN.
  ///
  ////////
    char fromaddress_string[256];
    char toaddress_string[256];
    menu_item *item;
    sym *s;
    GList *symlist=0;
    GList *iter;

    struct profile_entry *entry;
    struct profile_range_entry *range_entry=0;

    if(widget==0 || data==0)
    {
        printf("Warning popup_activated(%p,%p)\n",widget,data);
        return;
    }


    Value *sym=0;
    Symbol_Table_Iterator sti;

    item = (menu_item *)data;
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
      for(sym=sti.begin(); sym != sti.end(); sym = sti.next()) {

      if(typeid(*sym) == typeid(address_symbol)) {

        char *pstr=(char*)malloc(sym->name().length()+1);
        strncpy(pstr,
          sym->name().data(),
          sym->name().length());
        pstr[sym->name().length()]=0;

        sym * data=(sym*)malloc(sizeof(sym));
        data->name = pstr;
        data->type = (*sti)->isa();
        (*sti)->get(data->value);
        symlist=g_list_append(symlist,data);
        }
      }
      symlist=g_list_sort(symlist,(GCompareFunc)symcompare);
      strcpy(fromaddress_string,"0");
      iter=symlist;
      while(iter!=0)
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

      sprintf(toaddress_string,"%d",gp->cpu->program_address_limit());
      add_range(popup_pw,fromaddress_string,toaddress_string);

      while(symlist!=0)
        symlist=g_list_remove(symlist,symlist->data);

      break;
    case MENU_ADD_FUNCTION_LABELS:
      for(Symbol_Table::iterator sti = st.begin(); sti != st.end(); sti++) {

        if(((*sti)->isa() == SYMBOL_ADDRESS) &&
          !strstr((*sti)->name().data(),"_DS_")) {

          char *pstr=(char*)malloc((*sti)->name().length()+1);
          strncpy(pstr,
                  (*sti)->name().data(),
                  (*sti)->name().length());
          pstr[(*sti)->name().length()]=0;

          sym * data=(sym*)malloc(sizeof(sym));
          data->name = pstr;
          data->type = (*sti)->isa();
          (*sti)->get(data->value);
          symlist=g_list_append(symlist,data);
        }
      }

      symlist=g_list_sort(symlist,(GCompareFunc)symcompare);

      iter=symlist;
      if(iter!=0)
      {
        s=(sym*)iter->data;
        strcpy(fromaddress_string,s->name);
              free(s->name);
        free(s);
        iter=iter->next;
        while(iter!=0)
        {
          s=(sym*)iter->data;
          strcpy(toaddress_string,s->name);
          add_range(popup_pw,fromaddress_string,toaddress_string);
          strcpy(fromaddress_string,toaddress_string);
          toaddress_string[0]='\0';
          free(s);
          iter=iter->next;
              }
              sprintf(toaddress_string,"%d",gp->cpu->program_address_limit());
              add_range(popup_pw,fromaddress_string,toaddress_string);
      }

      while(symlist!=0)
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
            if(range_entry==0)
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
        if(range_entry!=0)
            plot_profile(popup_pw,pointlabel,cyclearray,numpoints);
        }
        break;
    default:
        puts("Unhandled menuitem?");
        break;
    }
#endif
}

static void update_menus(Profile_Window *pw)
{
    GtkWidget *item;
    struct profile_entry *entry;
    unsigned int i;

    for (i=0; i < (sizeof(range_menu_items)/sizeof(range_menu_items[0])) ; i++){
        item=range_menu_items[i].item;
//      if(range_menu_items[i].id!=MENU_ADD_GROUP)
        {
            if(pw)
            {
                entry = (struct profile_entry *)gtk_clist_get_row_data(GTK_CLIST(pw->profile_range_clist),pw->range_current_row);
                if(range_menu_items[i].id!=MENU_ADD_GROUP &&
                   range_menu_items[i].id!=MENU_ADD_ALL_LABELS &&
                   range_menu_items[i].id!=MENU_ADD_FUNCTION_LABELS &&
                   range_menu_items[i].id!=MENU_PLOT &&
                   entry==0)
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
  if(!pw->gp) return(FALSE);
  if(!pw->gp->cpu) return(FALSE);

  switch(key->keyval) {

  case GDK_Delete:
      entry = (struct profile_range_entry *)gtk_clist_get_row_data(GTK_CLIST(pw->profile_range_clist),pw->range_current_row);
      if(entry!=0)
          remove_entry(pw,(struct profile_entry *)entry);
      break;
  }
  return TRUE;
}

static gint profile_range_list_row_selected(GtkCList *profilelist,gint row, gint column,GdkEvent *event, Profile_Window *pw)
{
    struct profile_range_entry *entry;
    //    int bit;

    pw->range_current_row=row;
//    pw->current_column=column;

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
  unsigned int i;


  if(!pw)
  {
    printf("Warning profile window is null\n");
    return 0;
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

  if(widget==0 || event==0 || pw==0)
  {
      printf("Warning do_popup(%p,%p,%p)\n",widget,event,pw);
      return 0;
  }

  popup=pw->range_popup_menu;
    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
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
    //  return (text1 != 0);

    if (!text1)
        assert(0);
    //  return -1;

    if(1==sscanf(text1,"%li",&val1))
    {
        if(1==sscanf(text2,"%li",&val2))
        {
//          printf("Value %d %d\n",val1,val2);
            return val1-val2;
        }
    }
    return strcmp(text1,text2);
}



gint histogram_list_compare_func_cycles(gconstpointer a, gconstpointer b)
{
    const struct cycle_histogram_counter *h1=(struct cycle_histogram_counter*)a;
    const struct cycle_histogram_counter *h2=(struct cycle_histogram_counter*)b;

    if(h1->histo_cycles > h2->histo_cycles)
        return 1;
    if(h1->histo_cycles == h2->histo_cycles)
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
            if(h1->histo_cycles*h1->count > h2->histo_cycles*h2->count)
                return 1;
            if(h1->histo_cycles*h1->count == h2->histo_cycles*h2->count)
                return 0;
        }
    }
    return -1;
}

double calculate_median(GList *start, GList *stop)
{
    GList *sorted_list=0;

    struct cycle_histogram_counter *chc_start, *chc_stop;//, *chc_result;
//    GList *result;
    int count_sum=0;

    if(start==0)
        return -4.2;

    if(stop==0)
    {
        stop=start;
        while(stop->next!=0)
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
    while(stop->next!=0)
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

    if(count_sum>(int)chc_start->count)
    {
        start=start->next;
        chc_start=(struct cycle_histogram_counter*)start->data;
        g_list_free(sorted_list);
        return (double)chc_start->histo_cycles;
    }
    if(-count_sum>(int)chc_start->count)
    {
        start=start->prev;
        chc_start=(struct cycle_histogram_counter*)start->data;
        g_list_free(sorted_list);
        return (double)chc_start->histo_cycles;
    }
    if(-count_sum==(int)chc_start->count)
    {
        stop=stop->prev;
        chc_stop=(struct cycle_histogram_counter*)stop->data;
        g_list_free(sorted_list);
        return (chc_start->histo_cycles+chc_stop->histo_cycles)/2.0;
    }
    if(count_sum==(int)chc_start->count)
    {
        stop=stop->next;
        chc_stop=(struct cycle_histogram_counter*)stop->data;
        g_list_free(sorted_list);
        return (chc_start->histo_cycles+chc_stop->histo_cycles)/2.0;
    }
    if((unsigned int)abs(count_sum)<chc_start->count)
    {
        g_list_free(sorted_list);
        return (double)chc_start->histo_cycles;
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

    if(stop==0)
    {
        stop=start;
        while(stop->next!=0)
            stop=stop->next;
    }

    while(start!=stop)
    {
        float diff, diff2;

        chc_start=(struct cycle_histogram_counter*)start->data;
        chc_stop=(struct cycle_histogram_counter*)stop->data;

        diff=chc_start->histo_cycles-average;

        diff2=diff*diff;

        sum+=diff2*chc_start->count;

        count+=chc_start->count;

        start=start->next;
    }

    variance=sum/count;
    return sqrt(variance);
}


void Profile_Window::Update()
{

  unsigned int i;

  char count_string[100];
  GList *iter;

  if(!enabled)
    return;

  if(!gp || !gp->cpu)
  {
    return;
  }

  // Update profile list
  iter=profile_list;
  while(iter)
  {
      struct profile_entry *entry;
      guint64 count;

      entry=(struct profile_entry*)iter->data;

      count=gp->cpu->cycles_used(gp->cpu->map_pm_address2index(entry->address));

      if(entry->last_count!=count)
      {
        int row;

        entry->last_count=count;
        row=gtk_clist_find_row_from_data(GTK_CLIST(profile_clist),entry);
        if(row==-1)
        {
          break;
        }

        sprintf(count_string,"0x%" PRINTF_GINT64_MODIFIER "x",count);
        gtk_clist_set_text (GTK_CLIST(profile_clist),row,1,count_string);
      }
      iter=iter->next;
  }
  gtk_clist_sort(profile_clist);


  // Update range list
  iter=profile_range_list;

  while(iter)
  {
      struct profile_range_entry *range_entry;
      guint64 count;
      range_entry=(struct profile_range_entry*)iter->data;

      count=0;
      for(i=range_entry->startaddress;i<range_entry->endaddress;i++)
      {
        count+=gp->cpu->cycles_used(i);
      }

      if(range_entry->last_count!=count)
      {
          int row;

          range_entry->last_count=count;
          row=gtk_clist_find_row_from_data(GTK_CLIST(profile_range_clist),range_entry);
          if(row==-1)
          {
              break;
          }

          sprintf(count_string,"0x%" PRINTF_GINT64_MODIFIER "x",count);
          gtk_clist_set_text (GTK_CLIST(profile_range_clist),row,2,count_string);
      }
      iter=iter->next;
  }
  gtk_clist_sort(profile_range_clist);

  // Update register list
  iter=profile_register_list;
  while(iter)
  {
      struct profile_register_entry *register_entry;
      guint64 count_read, count_write;

      register_entry=(struct profile_register_entry*)iter->data;

      Register *reg = gp->cpu->rma.get_register(register_entry->address);
      count_read  = reg->read_access_count;
      count_write = reg->write_access_count;

      if(register_entry->last_count_read!=count_read||
         register_entry->last_count_write!=count_write)
      {
          int row;

          register_entry->last_count_read=count_read;
          register_entry->last_count_write=count_write;
          row=gtk_clist_find_row_from_data(GTK_CLIST(profile_register_clist),register_entry);
          if(row==-1)
          {
              break;
          }

          sprintf(count_string,"0x%" PRINTF_GINT64_MODIFIER "x",count_read);
          gtk_clist_set_text (GTK_CLIST(profile_register_clist),row,2,count_string);
          sprintf(count_string,"0x%" PRINTF_GINT64_MODIFIER "x",count_write);
          gtk_clist_set_text (GTK_CLIST(profile_register_clist),row,3,count_string);
      }
      iter=iter->next;
  }

  // Update cummulative statistics list
  histogram_profile_list = g_list_sort(histogram_profile_list,
                                           histogram_list_compare_func);
  // Remove all of clist (for now)
  gtk_clist_freeze(GTK_CLIST(profile_exestats_clist));
  gtk_clist_clear(GTK_CLIST(profile_exestats_clist));
  if(histogram_profile_list!=0)
  {
      struct cycle_histogram_counter *chc;
      int count_sum=0;
      unsigned int start=0xffffffff, stop=0xffffffff;
      guint64 min=0xffffffffffffffffULL, max=0;
      guint64 cycles_sum=0;
      GList *list_start=0, *list_end=0;
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

        iter=histogram_profile_list;
        list_start = iter;
      while(iter!=0)
      {
          chc=(struct cycle_histogram_counter*)iter->data;


          if(start==chc->start_address &&
             stop==chc->stop_address)
          {
              // Add data to statistics

              count_sum+=chc->count;
              if(chc->histo_cycles<min)
                  min=chc->histo_cycles;
              if(chc->histo_cycles>max)
                  max=chc->histo_cycles;
              cycles_sum+=chc->histo_cycles*chc->count;
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
                  gtk_clist_append(GTK_CLIST(profile_exestats_clist),entry);
              }

              // Start new calculation
              count_sum=chc->count;
              start = chc->start_address;
              stop = chc->stop_address;
              min=chc->histo_cycles;
              max=chc->histo_cycles;
              cycles_sum=chc->histo_cycles*chc->count;
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
      gtk_clist_append(GTK_CLIST(profile_exestats_clist),entry);
  }
  gtk_clist_thaw(GTK_CLIST(profile_exestats_clist));

}

#define END_OF_TIME 0xFFFFFFFFFFFFFFFFULL
static guint64 startcycle=END_OF_TIME;
static unsigned int startaddress;
static guint64 stopcycle=END_OF_TIME;
static unsigned int stopaddress;

//------------------------------------------------------------------------
//
// ProfileStart class
//
class ProfileStart : public TriggerObject
{

public:
  ProfileStart(Profile_Window *_pw, int _address)
  {
    pw = _pw;
    address = _address;
  }

  void callback(void)
  {
    if(!gpGuiProcessor || !gpGuiProcessor->cpu || !pw->gp->cpu)
      return;

    if(startcycle==END_OF_TIME) {
      startcycle   = get_cycles().get();
      startaddress = pw->gp->cpu->pma->get_PC();
    }
  }

private:
  Profile_Window *pw;
  int address;
};

//------------------------------------------------------------------------
//
// ProfileStop class
//
class ProfileStop : public TriggerObject
{

public:
  ProfileStop(Profile_Window *_pw, int _address)
  {
    pw = _pw;
    address = _address;
  }

  void callback(void)
  {
    if(!gpGuiProcessor || !gpGuiProcessor->cpu || !pw->gp->cpu)
      return;

    if(stopcycle==END_OF_TIME && startcycle!=END_OF_TIME) {

      stopcycle = get_cycles().get();
      if(startcycle==stopcycle)
        // This was probably an attempt to measure the whole loop.
        // Set stopcycle to unset, and wait for the next one
        stopcycle=END_OF_TIME;

      else {

        guint64 cycles;
        GList *iter;
  stopaddress=pw->gp->cpu->pma->get_PC();

        // We have a new measurement
        cycles=(int)stopcycle-(int)startcycle;

        // Search to see if there are an entry with this startaddress,
        // stopaddress and cycle count.
        iter=pw->histogram_profile_list;
        while(iter!=0) {

          struct cycle_histogram_counter *chc;
          chc=(struct cycle_histogram_counter*)iter->data;
          if(chc->start_address == startaddress &&
             chc->stop_address == stopaddress &&
             chc->histo_cycles == cycles)
            {
              // If so then add 1 to the counter
              chc->count++;
              break;
            }
          iter=iter->next;
        }

        if(iter==0) {

          // Else malloc a new struct, fill with values and add (sorted) to list
          struct cycle_histogram_counter *chc;

          chc=(struct cycle_histogram_counter*)malloc(sizeof(struct cycle_histogram_counter));
          chc->start_address=startaddress;
          chc->stop_address=stopaddress;
          chc->histo_cycles=cycles;
          chc->count=1;

          pw->histogram_profile_list=g_list_append(pw->histogram_profile_list,chc);
        }

        startcycle=stopcycle=END_OF_TIME;
      }
    }
  }


private:
  Profile_Window *pw;
  int address;
};

/*****************************************************************
 * StartExe
 *
 * Create a 'profile start' object for the program memory.
 *
 */
void Profile_Window::StartExe(int address)
{

  if(!enabled)
    ChangeView(VIEW_SHOW);

  if(gp->cpu->pma->address_has_profile_start(address))
    gp->cpu->pma->clear_profile_start_at_address(address);
  else {

    if(gp->cpu->pma->address_has_profile_stop(address))
      // Can't have both start and stop at the same address
      // ..it becomes difficult to calculate the cycles
      gp->cpu->pma->clear_profile_stop_at_address(address);

    // FIXME -- memory leak...
    gp->cpu->pma->set_profile_start_at_address(address,
                                              new ProfileStart(this,address));

  }

}

/*****************************************************************
 * SopExe
 *
 * Create a 'profile stop' object for the program memory.
 *
 */
void Profile_Window::StopExe(int address)
{
  if(enabled)
    ChangeView(VIEW_SHOW);

  if(gp->cpu->pma->address_has_profile_stop(address))
    gp->cpu->pma->clear_profile_stop_at_address(address);
  else {

    if(gp->cpu->pma->address_has_profile_start(address))
      // Can't have both start and stop at the same address
      // ..it becomes difficult to calculate the cycles
      gp->cpu->pma->clear_profile_start_at_address(address);

    // FIXME -- memory leak...
    gp->cpu->pma->set_profile_stop_at_address(address,
                                              new ProfileStop(this,address));
  }
}

/*****************************************************************
 * ProfileWindow_new_program
 *
 *
 */

void Profile_Window::NewProgram(GUI_Processor *_gp)
{
  int row;
  unsigned int uPMIndex;

  if(!_gp)
    return;

  gp = _gp;

  if(!gp->cpu)
    return;

  program=1;

  if(!enabled)
    return;

  profile_keeper.enable_profiling();

  // Instruction clist
  gtk_clist_freeze(profile_clist);
  Processor *pProcessor = gp->cpu;
  ProgramMemoryAccess *pPMA = pProcessor->pma;
  for(uPMIndex=0; uPMIndex < pProcessor->program_memory_size(); uPMIndex++) {

    struct profile_entry *profile_entry;
    char address_string[100];
    char instruction_string[100];
    char count_string[100];
    char *entry[PROFILE_COLUMNS]={address_string,count_string,instruction_string};
    guint64 cycles;
    instruction * pInstruction = pProcessor->pma->getFromIndex(uPMIndex);
    unsigned int uAddress = pProcessor->map_pm_index2address(uPMIndex);
    if(pPMA->hasValid_opcode_at_index(uPMIndex)) {

      sprintf(address_string, "0x%04x",uAddress);
      strcpy(instruction_string, pInstruction->name().c_str());

      cycles=pProcessor->cycles_used(uPMIndex);
      sprintf(count_string,"0x%" PRINTF_GINT64_MODIFIER "x",cycles);

      row=gtk_clist_append(GTK_CLIST(profile_clist), entry);

      // FIXME this memory is never freed?
      profile_entry = (struct profile_entry*)malloc(sizeof(struct profile_entry));
      profile_entry->address=uAddress;
      profile_entry->last_count=cycles;

      gtk_clist_set_row_data(GTK_CLIST(profile_clist), row, (gpointer)profile_entry);

      profile_list = g_list_append(profile_list, (gpointer)profile_entry);
    }
  }
  gtk_clist_thaw(profile_clist);

  // Register clist
  gtk_clist_freeze(profile_register_clist);
  for(unsigned int i=0; i < pProcessor->rma.get_size(); i++) {

    struct profile_register_entry *profile_register_entry;
    char address_string[100];
    char count_string_read[100];
    char count_string_write[100];
    char register_string[100];
    char *entry_register[PROFILE_REGISTER_COLUMNS]={address_string,register_string,count_string_read,count_string_write};
    guint64 read_cycles;
    guint64 write_cycles;
    char *name;

    Register *reg = pProcessor->rma.get_register(i);

    //
    // If the register is valid, but it's not aliased and it's not a special function
    // register, then we can profile it.
    //

    if(reg && reg->isa() != Register::INVALID_REGISTER    // i.e. the register is valid
          &&
       !((reg->isa() == Register::SFR_REGISTER) || (i != reg->address)) ) {

        sprintf(address_string,"0x%04x",i);
        name = (char*)reg->name().c_str();
        if(name==0)
          name = address_string;
        strcpy(register_string, name);

        read_cycles=reg->read_access_count;
        sprintf(count_string_read,"0x%" PRINTF_GINT64_MODIFIER "x",read_cycles);

        write_cycles=reg->write_access_count;
        sprintf(count_string_write,"0x%" PRINTF_GINT64_MODIFIER "x",write_cycles);

        row=gtk_clist_append(GTK_CLIST(profile_register_clist), entry_register);

        // FIXME this memory is never freed?
        profile_register_entry = (struct profile_register_entry*) malloc(sizeof(struct profile_register_entry));
        profile_register_entry->address=i;
        profile_register_entry->last_count_read=read_cycles;
        profile_register_entry->last_count_read=write_cycles;

        gtk_clist_set_row_data(GTK_CLIST(profile_register_clist), row, (gpointer)profile_register_entry);

        profile_register_list = g_list_append(profile_register_list, (gpointer)profile_register_entry);
      }
  }
  gtk_clist_thaw(profile_register_clist);

}

/*****************************************************************
 * ProfileWindow_new_processor
 *
 *
 */

void Profile_Window::NewProcessor(GUI_Processor *_gp)
{

  if(!gp)
    return;

  if(!enabled)
    return;
}

static int delete_event(GtkWidget *widget,
                        GdkEvent  *event,
                        Register_Window *rw)
{
  rw->ChangeView(VIEW_HIDE);

  return TRUE;
}

gdouble gaussian(GtkPlot *plot, GtkPlotData *data, gdouble x, gboolean *err)
{
 gdouble y;
 *err = FALSE;
 y = 1000*x;//.65*exp(-.5*pow(x-.5,2)/.02);

 return y;
}

void Profile_Window::Build(void)
{
  if(bIsBuilt)
    return;

  GtkWidget *label;
  GtkWidget *main_vbox;
  GtkWidget *scrolled_window;

  gint column_width,char_width;
  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
                     GTK_SIGNAL_FUNC(delete_event), this);

  main_vbox=gtk_vbox_new(FALSE,1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox),0);
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  gtk_window_set_title(GTK_WINDOW(window), "profile viewer");

  notebook = gtk_notebook_new();
  gtk_widget_show(notebook);

  gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);


  // Instruction profile clist
  profile_clist=GTK_CLIST(gtk_clist_new_with_titles(PROFILE_COLUMNS,profile_titles));
  //profile_clist = GTK_CLIST(profile_clist);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_clist),0,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(profile_clist),1,TRUE);
//  gtk_clist_set_sort_column (pw->profile_clist,1);
//  gtk_clist_set_sort_type (pw->profile_clist,GTK_SORT_DESCENDING);
  gtk_clist_set_compare_func(GTK_CLIST(profile_clist),
                             (GtkCListCompareFunc)profile_compare_func);

  GTK_WIDGET_UNSET_FLAGS(profile_clist,GTK_CAN_DEFAULT);

  scrolled_window=gtk_scrolled_window_new(0, 0);

  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(profile_clist));

  gtk_widget_show(GTK_WIDGET(profile_clist));

  gtk_widget_show(scrolled_window);

//  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  label=gtk_label_new("Instruction profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scrolled_window,label);
  ///////////////////////////////////////////////////
  ///////////////////////////////////////////////////


  // Instruction range profile clist
  profile_range_clist=GTK_CLIST(gtk_clist_new_with_titles(PROFILE_RANGE_COLUMNS,profile_range_titles));
  gtk_clist_set_column_auto_resize(profile_range_clist,0,TRUE);
  gtk_clist_set_column_auto_resize(profile_range_clist,1,TRUE);
  gtk_clist_set_sort_column (profile_range_clist,2);
  gtk_clist_set_sort_type (profile_range_clist,GTK_SORT_DESCENDING);
  gtk_clist_set_compare_func(GTK_CLIST(profile_range_clist),
                             (GtkCListCompareFunc)profile_compare_func);

  GTK_WIDGET_UNSET_FLAGS(profile_range_clist,GTK_CAN_DEFAULT);

  range_popup_menu=build_menu(this);

  gtk_signal_connect(GTK_OBJECT(profile_range_clist),
                     "button_press_event",
                     (GtkSignalFunc) do_popup,
                     this);
  gtk_signal_connect(GTK_OBJECT(profile_range_clist),"key_press_event",
                     (GtkSignalFunc) key_press,
                     (gpointer) this);
  gtk_signal_connect(GTK_OBJECT(profile_range_clist),"select_row",
                     (GtkSignalFunc)profile_range_list_row_selected,this);

  scrolled_window=gtk_scrolled_window_new(0, 0);

  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(profile_range_clist));

  gtk_widget_show(GTK_WIDGET(profile_range_clist));

  gtk_widget_show(scrolled_window);

  label=gtk_label_new("Instruction range profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scrolled_window,label);

  ///////////////////////////////////////////////////


  // Register profile clist
  profile_register_clist=GTK_CLIST(gtk_clist_new_with_titles(PROFILE_REGISTER_COLUMNS,profile_register_titles));
  gtk_clist_set_column_auto_resize(profile_register_clist,0,TRUE);
  gtk_clist_set_column_auto_resize(profile_register_clist,1,TRUE);
  gtk_clist_set_column_auto_resize(profile_register_clist,2,TRUE);
  gtk_clist_set_column_auto_resize(profile_register_clist,3,TRUE);
//  gtk_clist_set_sort_column (pw->profile_register_clist,1);
//  gtk_clist_set_sort_type (pw->profile_register_clist,GTK_SORT_DESCENDING);
  gtk_clist_set_compare_func(profile_register_clist,
                             (GtkCListCompareFunc)profile_compare_func);

  GTK_WIDGET_UNSET_FLAGS(profile_register_clist,GTK_CAN_DEFAULT);

  scrolled_window=gtk_scrolled_window_new(0, 0);

  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(profile_register_clist));

  gtk_widget_show(GTK_WIDGET(profile_register_clist));

  gtk_widget_show(scrolled_window);

//  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  label=gtk_label_new("Register profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scrolled_window,label);

  ///////////////////////////////////////////////////


  // Execution time statistics tab
  profile_exestats_clist=GTK_CLIST(gtk_clist_new_with_titles(PROFILE_EXESTATS_COLUMNS,profile_exestats_titles));
  gtk_clist_set_column_auto_resize(profile_exestats_clist,0,TRUE);
  gtk_clist_set_column_auto_resize(profile_exestats_clist,1,TRUE);
  gtk_clist_set_column_auto_resize(profile_exestats_clist,2,TRUE);
  gtk_clist_set_column_auto_resize(profile_exestats_clist,3,TRUE);
  gtk_clist_set_column_auto_resize(profile_exestats_clist,4,TRUE);
  gtk_clist_set_column_auto_resize(profile_exestats_clist,5,TRUE);
  gtk_clist_set_column_auto_resize(profile_exestats_clist,6,TRUE);
  gtk_clist_set_column_auto_resize(profile_exestats_clist,7,TRUE);
  gtk_clist_set_column_auto_resize(profile_exestats_clist,8,TRUE);

  GTK_WIDGET_UNSET_FLAGS(profile_exestats_clist,GTK_CAN_DEFAULT);

  exestats_popup_menu=exestats_build_menu(this);
  gtk_signal_connect(GTK_OBJECT(profile_exestats_clist),
                     "button_press_event",
                     (GtkSignalFunc) exestats_do_popup,
                     this);

  scrolled_window=gtk_scrolled_window_new(0, 0);

  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(profile_exestats_clist));

  gtk_widget_show(GTK_WIDGET(profile_exestats_clist));

  gtk_widget_show(scrolled_window);

  label=gtk_label_new("Routine profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scrolled_window,label);
  ///////////////////////////////////////////////////


  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");

  normal_style = gtk_style_new ();
#if GTK_MAJOR_VERSION >= 2
  char_width = gdk_string_width(gtk_style_get_font(normal_style) ,"9");
#else
  char_width = gdk_string_width (normal_style->font,"9");
#endif
  column_width = 3 * char_width + 6;

  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
                           GTK_SIGNAL_FUNC(gui_object_configure_event),this);



  gtk_widget_show (window);


  bIsBuilt=true;

  NewProcessor(gp);

  if(program)
    NewProgram(gp);

  Update();

  UpdateMenuItem();

}

Profile_Window::Profile_Window(GUI_Processor *_gp)
{

  menu = "<main>/Windows/Profile";

  gp = _gp;
  set_name("profile");
  window = 0;
  wc = WC_data;
  wt = WT_profile_window;

  profile_list=0;
  profile_range_list=0;
  profile_register_list=0;
  histogram_profile_list=0;

  program=0;

  get_config();

  if(enabled)
      Build();

}


#endif // HAVE_GUI
