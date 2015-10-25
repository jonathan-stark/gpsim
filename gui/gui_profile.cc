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

#include <cstdio>
#include <cstdlib>

#include "../config.h"

#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <math.h>

#include <cassert>

#include "gui.h"
#include "gui_profile.h"
#include "gui_regwin.h"

#include "../src/symbol.h"

enum {
  PROFILE_ADDRESS,
  PROFILE_CYCLES,
  PROFILE_INSTRUCTION,
  PROFILE_DATA,
  PROFILE_COLUMNS
};

enum {
  PROFILE_RANGE_SADDRESS,
  PROFILE_RANGE_EADDRESS,
  PROFILE_RANGE_CYCLES,
  PROFILE_RANGE_DATA,
  PROFILE_RANGE_COLUMNS
};

enum {
  PROFILE_REGISTER_ADDRESS,
  PROFILE_REGISTER_REGISTER,
  PROFILE_REGISTER_READ,
  PROFILE_REGISTER_WRITE,
  PROFILE_REGISTER_DATA,
  PROFILE_REGISTER_COLUMNS
};

enum {
  PROFILE_EXESTATS_FADDRESS,
  PROFILE_EXESTATS_TADDRESS,
  PROFILE_EXESTATS_EXECUTIONS,
  PROFILE_EXESTATS_MIN,
  PROFILE_EXESTATS_MAX,
  PROFILE_EXESTATS_MEDIAN,
  PROFILE_EXESTATS_AVERAGE,
  PROFILE_EXESTATS_STDDEV,
  PROFILE_EXESTATS_TOTAL,
  PROFILE_EXESTATS_COLUMNS
};

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

static menu_item exestats_menu_items[] = {
    {"Plot distribution", MENU_PLOT},
};


void gui_message(const char *message);

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
static void remove_entry(Profile_Window *pw, GtkTreeIter *iter)
{
  gpointer e;
  gtk_tree_model_get(GTK_TREE_MODEL(pw->profile_range_list),
    iter, PROFILE_RANGE_DATA, &e, -1);
  struct profile_range_entry *entry = (struct profile_range_entry *) e;

  gtk_list_store_remove(pw->profile_range_list, iter);
  free(entry);
}

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

// called from exestats_do_popup
static GtkWidget *
exestats_build_menu(Profile_Window *pw)
{
  GtkWidget *menu;
  GtkWidget *item;

  if(pw==0)
  {
      printf("Warning build_menu(%p)\n",pw);
      return 0;
  }

  popup_pw = pw;

  menu=gtk_menu_new();

  for (size_t i = 0; i < G_N_ELEMENTS(exestats_menu_items) ; ++i) {
      exestats_menu_items[i].item=item=gtk_menu_item_new_with_label(exestats_menu_items[i].name);

      g_signal_connect(item, "activate",
                         G_CALLBACK (exestats_popup_activated),
                         &exestats_menu_items[i]);

      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
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

int plot_routine_histogram(Profile_Window *pw)
{
    return 0;
}

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
}

static void update_menus(Profile_Window *pw)
{
  GtkTreeSelection *selection
    = gtk_tree_view_get_selection(GTK_TREE_VIEW(pw->profile_range_tree));
  gboolean selected = gtk_tree_selection_get_selected(selection, NULL, NULL);

  for (size_t i = 0; i < G_N_ELEMENTS(range_menu_items) ; ++i) {
    GtkWidget *item = range_menu_items[i].item;
    if (range_menu_items[i].id != MENU_ADD_GROUP &&
      range_menu_items[i].id != MENU_ADD_ALL_LABELS &&
      range_menu_items[i].id != MENU_ADD_FUNCTION_LABELS &&
      range_menu_items[i].id != MENU_PLOT && !selected)
       gtk_widget_set_sensitive (item, FALSE);
    else
       gtk_widget_set_sensitive (item, TRUE);
  }
}

static gboolean
key_press(GtkWidget *widget,
          GdkEventKey *key,
          gpointer data)
{
  Profile_Window *pw = static_cast<Profile_Window *>(data);

  GtkTreeIter iter;
  GtkTreeSelection *selection
    = gtk_tree_view_get_selection(GTK_TREE_VIEW(pw->profile_range_tree));

  if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
    if (key->keyval == GDK_KEY_Delete) {
      remove_entry(pw, &iter);
    }
  }

  return TRUE;
}

// called from do_popup
static GtkWidget *
build_menu(Profile_Window *pw)
{
  GtkWidget *menu;
  GtkWidget *item;

  if(!pw)
  {
    printf("Warning profile window is null\n");
    return 0;
  }

  popup_pw = pw;

  menu = gtk_menu_new();

  for (size_t i = 0; i < G_N_ELEMENTS(range_menu_items) ; ++i) {
    range_menu_items[i].item = item
      = gtk_menu_item_new_with_label(range_menu_items[i].name);

    g_signal_connect(item, "activate", G_CALLBACK (popup_activated),
      &range_menu_items[i]);

    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  update_menus(pw);

  return menu;
}

// button press handler
static gboolean
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
      update_menus(pw);
      gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
                     3, event->time);
    }
    return FALSE;
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
    struct cycle_histogram_counter *chc_start;

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
  char count_string[100];

  if(!enabled)
    return;

  if(!gp || !gp->cpu)
  {
    return;
  }

  // Update profile list
  GtkTreeIter titer;
  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(profile_list), &titer)) {
    do {
      gpointer e;
      gtk_tree_model_get(GTK_TREE_MODEL(profile_list), &titer,
        PROFILE_DATA, &e, -1);
      struct profile_entry *entry = (struct profile_entry *) e;

      guint64 count;
      count = gp->cpu->cycles_used(gp->cpu->map_pm_address2index(entry->address));
      if (entry->last_count != count) {
        entry->last_count = count;
        g_snprintf(count_string, sizeof(count_string),
          "0x%" PRINTF_GINT64_MODIFIER "x", count);
        gtk_list_store_set(profile_list, &titer,
          PROFILE_CYCLES, count_string, -1);
      }
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(profile_list), &titer));
  }

  // Update range list

  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(profile_range_list), &titer)) {
    do {
      gpointer e;
      gtk_tree_model_get(GTK_TREE_MODEL(profile_range_list), &titer,
        PROFILE_RANGE_DATA, &e, -1);
      struct profile_range_entry *range_entry
        = (struct profile_range_entry *) e;

      guint64 count = 0;
      for (unsigned int i = range_entry->startaddress; i < range_entry->endaddress; ++i) {
        count += gp->cpu->cycles_used(i);
      }
      if (range_entry->last_count != count) {
        range_entry->last_count = count;
        sprintf(count_string, "0x%" PRINTF_GINT64_MODIFIER "x", count);
        gtk_list_store_set(profile_range_list, &titer,
          PROFILE_RANGE_CYCLES, count_string, -1);
      }
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(profile_range_list), &titer));
  }

  // Update register list

  if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(profile_register_list), &titer)) {
    do {
      gpointer e;
      gtk_tree_model_get(GTK_TREE_MODEL(profile_register_list), &titer,
        PROFILE_REGISTER_DATA, &e, -1);
      struct profile_register_entry *register_entry
        = (struct profile_register_entry *) e;

      Register *reg = gp->cpu->rma.get_register(register_entry->address);
      guint64 count_read  = reg->read_access_count;
      guint64 count_write = reg->write_access_count;

      if (register_entry->last_count_read != count_read ||
        register_entry->last_count_write != count_write) {

        register_entry->last_count_read = count_read;
        register_entry->last_count_write = count_write;

        sprintf(count_string, "0x%" PRINTF_GINT64_MODIFIER "x", count_read);
        gtk_list_store_set(profile_register_list, &titer,
          PROFILE_REGISTER_READ, count_string, -1);
        sprintf(count_string, "0x%" PRINTF_GINT64_MODIFIER "x", count_write);
        gtk_list_store_set(profile_register_list, &titer,
          PROFILE_REGISTER_WRITE, count_string, -1);
      }
    } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(profile_register_list), &titer));
  }

  // Update cummulative statistics list
  histogram_profile_list = g_list_sort(histogram_profile_list,
                                           histogram_list_compare_func);
  // Remove all of list (for now)
  gtk_list_store_clear(profile_exestats_list);
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
        char median_string[100]="";
        char average_string[100]="";
        char stddev_string[100]="";

        GList *iter=histogram_profile_list;
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
                  sprintf(median_string,"%.1f", calculate_median(list_start,list_end));
                  sprintf(average_string,"%.1f",cycles_sum/(float)count_sum);
                  sprintf(stddev_string,"%.1f",calculate_stddev(list_start,list_end,cycles_sum/(float)count_sum));
                  GtkTreeIter titer;
                  gtk_list_store_append(profile_exestats_list, &titer);
                  gtk_list_store_set(profile_exestats_list, &titer,
                    PROFILE_EXESTATS_FADDRESS, fromaddress_string,
                    PROFILE_EXESTATS_TADDRESS, toaddress_string,
                    PROFILE_EXESTATS_EXECUTIONS, count_sum,
                    PROFILE_EXESTATS_MIN, min,
                    PROFILE_EXESTATS_MAX, max,
                    PROFILE_EXESTATS_MEDIAN, median_string,
                    PROFILE_EXESTATS_AVERAGE, average_string,
                    PROFILE_EXESTATS_STDDEV, stddev_string,
                    PROFILE_EXESTATS_TOTAL, cycles_sum,
                    -1);
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
      sprintf(median_string,"%.1f", calculate_median(list_start,list_end));
      sprintf(average_string,"%.1f",cycles_sum/(float)count_sum);
      sprintf(stddev_string,"%.1f",calculate_stddev(list_start,list_end,cycles_sum/(float)count_sum));

      GtkTreeIter titer;
      gtk_list_store_append(profile_exestats_list, &titer);
      gtk_list_store_set(profile_exestats_list, &titer,
        PROFILE_EXESTATS_FADDRESS, fromaddress_string,
        PROFILE_EXESTATS_TADDRESS, toaddress_string,
        PROFILE_EXESTATS_EXECUTIONS, count_sum,
        PROFILE_EXESTATS_MIN, min,
        PROFILE_EXESTATS_MAX, max,
        PROFILE_EXESTATS_MEDIAN, median_string,
        PROFILE_EXESTATS_AVERAGE, average_string,
        PROFILE_EXESTATS_STDDEV, stddev_string,
        PROFILE_EXESTATS_TOTAL, cycles_sum,
        -1);
  }
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

          // Else create a new struct, fill with values and add (sorted) to list
          struct cycle_histogram_counter *chc = new cycle_histogram_counter;
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
  Processor *pProcessor = gp->cpu;
  ProgramMemoryAccess *pPMA = pProcessor->pma;
  for (uPMIndex = 0; uPMIndex < pProcessor->program_memory_size(); uPMIndex++) {

    guint64 cycles;
    instruction * pInstruction = pProcessor->pma->getFromIndex(uPMIndex);
    unsigned int uAddress = pProcessor->map_pm_index2address(uPMIndex);
    if(pPMA->hasValid_opcode_at_index(uPMIndex)) {
      struct profile_entry *profile_entry;
      char address_string[100];
      char count_string[100];

      sprintf(address_string, "0x%04x", uAddress);

      cycles = pProcessor->cycles_used(uPMIndex);
      sprintf(count_string,"0x%" PRINTF_GINT64_MODIFIER "x", cycles);

      // FIXME this memory is never freed?
      profile_entry = new struct profile_entry;
      profile_entry->address = uAddress;
      profile_entry->last_count = cycles;

      GtkTreeIter iter;
      gtk_list_store_append(profile_list, &iter);

      gtk_list_store_set(profile_list, &iter,
        PROFILE_ADDRESS, address_string,
        PROFILE_CYCLES, count_string,
        PROFILE_INSTRUCTION, pInstruction->name().c_str(),
        PROFILE_DATA, profile_entry,
        -1);
    }
  }

  // Register list
  for(unsigned int i = 0; i < pProcessor->rma.get_size(); i++) {
    Register *reg = pProcessor->rma.get_register(i);

    //
    // If the register is valid, but it's not aliased and it's not a special function
    // register, then we can profile it.
    //

    if(reg && reg->isa() != Register::INVALID_REGISTER    // i.e. the register is valid
          &&
       !((reg->isa() == Register::SFR_REGISTER) || (i != reg->address)) ) {

        struct profile_register_entry *profile_register_entry;
        char address_string[100];
        char count_string_read[100];
        char count_string_write[100];
        sprintf(address_string, "0x%04x", i);

        guint64 read_cycles = reg->read_access_count;
        sprintf(count_string_read, "0x%" PRINTF_GINT64_MODIFIER "x", read_cycles);

        guint64 write_cycles = reg->write_access_count;
        sprintf(count_string_write, "0x%" PRINTF_GINT64_MODIFIER "x", write_cycles);

        // FIXME this memory is never freed?
        profile_register_entry = new struct profile_register_entry;
        profile_register_entry->address = i;
        profile_register_entry->last_count_read = read_cycles;
        profile_register_entry->last_count_write = write_cycles;

        GtkTreeIter iter;
        gtk_list_store_append(profile_register_list, &iter);

        gtk_list_store_set(profile_register_list, &iter,
          PROFILE_REGISTER_ADDRESS, address_string,
          PROFILE_REGISTER_REGISTER, reg->name().c_str(),
          PROFILE_REGISTER_READ, count_string_read,
          PROFILE_REGISTER_WRITE, count_string_write,
          PROFILE_REGISTER_DATA, profile_register_entry, -1);
    }
  }
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

void Profile_Window::Build()
{
  if(bIsBuilt)
    return;

  GtkWidget *label;
  GtkWidget *main_vbox;
  GtkWidget *scrolled_window;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(window, "delete_event",
                     G_CALLBACK (delete_event), this);

  main_vbox=gtk_vbox_new(FALSE,1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox),0);
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  gtk_window_set_title(GTK_WINDOW(window), "profile viewer");

  notebook = gtk_notebook_new();
  gtk_widget_show(notebook);

  gtk_box_pack_start (GTK_BOX (main_vbox), notebook, TRUE, TRUE, 0);


  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  // Instruction profile list
  profile_list = gtk_list_store_new(PROFILE_COLUMNS,
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

  profile_tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(profile_list));

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Address",
    renderer, "text", PROFILE_ADDRESS, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Cycles",
    renderer, "text", PROFILE_CYCLES, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Instruction",
    renderer, "text", PROFILE_INSTRUCTION, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_tree), column);

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_tree);

  gtk_widget_show(profile_tree);

  gtk_widget_show(scrolled_window);

//  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  label=gtk_label_new("Instruction profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scrolled_window,label);
  ///////////////////////////////////////////////////
  ///////////////////////////////////////////////////


  // Instruction range profile clist
  profile_range_list = gtk_list_store_new(PROFILE_RANGE_COLUMNS,
    	G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

  profile_range_tree
    = gtk_tree_view_new_with_model(GTK_TREE_MODEL(profile_range_list));

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Start address",
    renderer, "text", PROFILE_RANGE_SADDRESS, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_range_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("End address",
    renderer, "text", PROFILE_RANGE_EADDRESS, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_range_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Cycles",
    renderer, "text", PROFILE_RANGE_CYCLES, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_range_tree), column);

  range_popup_menu = build_menu(this);

  g_signal_connect(profile_range_tree, "button_press_event",
    G_CALLBACK (do_popup), this);
  g_signal_connect(profile_range_tree, "key_press_event",
    G_CALLBACK (key_press), this);

  scrolled_window=gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_range_tree);

  gtk_widget_show(profile_range_tree);

  gtk_widget_show(scrolled_window);

  label=gtk_label_new("Instruction range profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scrolled_window,label);

  ///////////////////////////////////////////////////


  // Register profile clist
  profile_register_list = gtk_list_store_new(PROFILE_REGISTER_COLUMNS,
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
    G_TYPE_POINTER);

  profile_register_tree
    = gtk_tree_view_new_with_model(GTK_TREE_MODEL(profile_register_list));

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Address",
    renderer, "text", PROFILE_REGISTER_ADDRESS, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_register_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Register",
    renderer, "text", PROFILE_REGISTER_REGISTER, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_register_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Read count",
    renderer, "text", PROFILE_REGISTER_READ, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_register_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Write count",
    renderer, "text", PROFILE_REGISTER_WRITE, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_register_tree), column);

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_register_tree);

  gtk_widget_show(profile_register_tree);

  gtk_widget_show(scrolled_window);

//  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  label=gtk_label_new("Register profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scrolled_window,label);


  // Execution time statistics tab
  profile_exestats_list = gtk_list_store_new(PROFILE_EXESTATS_COLUMNS,
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_UINT64, G_TYPE_UINT64,
    G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_UINT64);

  profile_exestats_tree
    = gtk_tree_view_new_with_model(GTK_TREE_MODEL(profile_exestats_list));

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("From address",
    renderer, "text", PROFILE_EXESTATS_FADDRESS, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("To address",
    renderer, "text", PROFILE_EXESTATS_TADDRESS, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Executions",
    renderer, "text", PROFILE_EXESTATS_EXECUTIONS, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Min",
    renderer, "text", PROFILE_EXESTATS_MIN, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Max",
    renderer, "text", PROFILE_EXESTATS_MAX, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Median",
    renderer, "text", PROFILE_EXESTATS_MEDIAN, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Average",
    renderer, "text", PROFILE_EXESTATS_AVERAGE, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Std. Dev.",
    renderer, "text", PROFILE_EXESTATS_STDDEV, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("Total",
    renderer, "text", PROFILE_EXESTATS_TOTAL, NULL);
  gtk_tree_view_append_column(GTK_TREE_VIEW(profile_exestats_tree), column);

  exestats_popup_menu = exestats_build_menu(this);
  g_signal_connect(profile_exestats_tree, "button_press_event",
    G_CALLBACK (exestats_do_popup), this);

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), profile_exestats_tree);

  gtk_widget_show(profile_exestats_tree);

  gtk_widget_show(scrolled_window);

  label=gtk_label_new("Routine profile");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),scrolled_window,label);
  ///////////////////////////////////////////////////


  gtk_window_set_default_size(GTK_WINDOW(window), width, height);
  gtk_window_move(GTK_WINDOW(window), x, y);
  gtk_window_set_wmclass(GTK_WINDOW(window), name(), "Gpsim");

  g_signal_connect_after(window, "configure_event",
                           G_CALLBACK (gui_object_configure_event),this);



  gtk_widget_show (window);


  bIsBuilt=true;

  NewProcessor(gp);

  if(program)
    NewProgram(gp);

  Update();

  UpdateMenuItem();

}

const char *Profile_Window::name()
{
  return "profile";
}

Profile_Window::Profile_Window(GUI_Processor *_gp)
  : program(0)
{
  menu = "/menu/Windows/Profile";

  gp = _gp;

  histogram_profile_list=0;
  range_current_row = 0;

  get_config();

  if(enabled)
      Build();
}


#endif // HAVE_GUI
