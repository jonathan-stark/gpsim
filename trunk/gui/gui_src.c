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

#include "gui.h"

#include <assert.h>

static gint
key_press(GtkWidget *widget,
	  GdkEventKey *key, 
	  gpointer data)
{

  SourceBrowser_Window *sbw = (SourceBrowser_Window *) data;

  if(!sbw) return(FALSE);
  if(!sbw->gui_obj.gp) return(FALSE);
  if(!sbw->gui_obj.gp->pic_id) return(FALSE);

  switch(key->keyval) {

  case 's':  // Single Step
  case 'S':
  case GDK_F7:
      //sbw->gui_obj.gp->p->step(1);
      gpsim_step(sbw->gui_obj.gp->pic_id, 1);
      break;

  case 'o':  // Step Over Next instruction
  case 'O':
  case GDK_F8:
      //sbw->gui_obj.gp->p->step_over();
      gpsim_step_over(sbw->gui_obj.gp->pic_id);
      break;
  case 'r':
  case 'R':
  case GDK_F9:
      //sbw->gui_obj.gp->p->run();
      gpsim_run(sbw->gui_obj.gp->pic_id);
      break;
  case 'q':
  case 'Q':
      exit_gpsim();
  }

  return TRUE;
}

/*
static int
SourceBrowser_close(GtkWidget *widget,
		    SourceBrowser_Window *sbw)
{
  printf("closing browser\n");
  SourceBrowser_change_view((GUI_Object *)sbw,VIEW_HIDE);

  //gtk_widget_hide(widget);
  printf("browser should be closed\n");

  return(TRUE);

}
*/

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        SourceBrowser_Window *sbw)
{
    SourceBrowser_change_view((GUI_Object *)sbw,VIEW_HIDE);
    return TRUE;
}

static gint
button_press(GtkWidget *widget,
	     GdkEvent  *event, 
	     gpointer data)
{

    SourceBrowser_Window *sbw = (SourceBrowser_Window *) data;
    SourceBrowserOpcode_Window *sbow = (SourceBrowserOpcode_Window*)sbw;
  int break_row;

  if(!sbw) return(FALSE);
  if(!sbw->gui_obj.gp) return(FALSE);
  if(!sbw->gui_obj.gp->pic_id) return(FALSE);
  
  if ( ((event->type == GDK_BUTTON_PRESS) &&
        (event->button.button == 3))
       ||
       ((event->type == GDK_2BUTTON_PRESS) &&
        (event->button.button == 1)) )

  {
      switch(sbw->gui_obj.wt) {

      case WT_asm_source_window:
/*	  index = gtk_editable_get_position(GTK_EDITABLE(sbaw->source_text));
	  e = gui_index_to_entry(index);
	  sbw->gui_obj.gp->p->toggle_break_at_line(0, e->line);*/
	  return FALSE;

      case WT_opcode_source_window:
	  break_row =  GTK_CLIST (sbow->clist)->focus_row;

	  if(!sbow->clist_rows)
	      return TRUE;      // no code is in this window
	  //sbow->sbw.gui_obj.gp->p->toggle_break_at_address(break_row);
	   gpsim_toggle_break_at_address(sbow->sbw.gui_obj.gp->pic_id, break_row);
	  printf("Toggle at line %d\n",break_row);
	  return TRUE;

//      case WT_list_source_window:
//	  //sbw->gui_obj.gp->p->toggle_break_at_line(1, break_row);
//	  gpsim_toggle_break_at_line(sbow->sbw.gui_obj.gp->pic_id, 1, break_row);
//
//	  return TRUE;

      default:
	  printf("bad window type %d\n",sbw->gui_obj.wt);
      }

    }

  return FALSE;
}

void SourceBrowser_select_address(SourceBrowser_Window *sbw,int address)
{
    SourceBrowserOpcode_Window *sbow=(SourceBrowserOpcode_Window*)sbw;
    SourceBrowserAsm_Window *sbaw=(SourceBrowserAsm_Window*)sbw;
    switch(sbw->gui_obj.wt)
    {
    case WT_asm_source_window:
	SourceBrowserAsm_select_address(sbaw, address);
	break;

    case WT_opcode_source_window:
	SourceBrowserOpcode_select_address(sbow,address);
	break;
    default:
	puts("SourceBrowser_select_address(): unhandled case");
	break;
    }
}

void SourceBrowser_update_line(struct cross_reference_to_gui *xref, int new_value)
{
  GUI_Processor *gp;
  int address;

  gp = (GUI_Processor *)xref->parent_window;
  if(!gp) { printf("gp == null\n"); return;}

  assert(xref && xref->data);

  address = *(int *)xref->data;

  if(gp->source_browser)
      SourceBrowserAsm_update_line( (SourceBrowserAsm_Window*)gp->source_browser,  address);

  if(gp->program_memory)
      SourceBrowserOpcode_update_line( (SourceBrowserOpcode_Window*)gp->program_memory,  address, address);

}

void SourceBrowser_update(SourceBrowser_Window *sbw)
{
  gint new_address;
  
  SourceBrowserOpcode_Window *sbow = (SourceBrowserOpcode_Window*)sbw;
  SourceBrowserAsm_Window *sbaw = (SourceBrowserAsm_Window*)sbw;
  unsigned int pic_id;
  
  if(!sbw) return;     // this shouldn't happen

  // get a manageable pointer 
  pic_id = sbw->gui_obj.gp->pic_id;

  new_address=gpsim_get_pc_value(pic_id);
  
  //update the displayed values
  switch(sbw->gui_obj.wt) {

  case WT_asm_source_window:
//      new_row =  pic->program_memory[pic->pc.value]->get_src_line();
      //      new_address = pic->find_closest_address_to_line(0,new_row);
      //new_address=pic->pc.value;
      SourceBrowserAsm_set_pc( sbaw, new_address);
      break;

  case WT_opcode_source_window:
      SourceBrowserOpcode_set_pc( sbow, new_address);
      break;

  case WT_list_source_window:
      assert(0);/*
    last_address = pic->find_closest_address_to_line(1,sbw->current_row);
    new_row =  pic->program_memory[pic->pc.value]->get_lst_line();
    break;        */


  default:
    printf("SourceBrowser_update: bad window type %d\n",sbw->gui_obj.wt);
    return;
  }
  


}
/*
static void pc_changed(struct cross_reference_to_gui *xref, int new_address)
{
    SourceBrowserAsm_Window *sbaw=(SourceBrowserAsm_Window*)(xref->parent_window);
    SourceBrowserOpcode_Window *sbow=(SourceBrowserOpcode_Window*)(xref->parent_window);

    if(sbaw->source_loaded)
    {
	SourceBrowserAsm_set_pc(sbaw, new_address);
    }
    SourceBrowserOpcode_set_pc(sbow,new_address);
}
*/

void CreateSBW(SourceBrowser_Window *sbw)
{


  GtkWidget *window;
  GtkWidget *vbox;
  
  int x,y,width,height;
  
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

/*  gtk_widget_set_events(window,
			gtk_widget_get_events(window)|
			GDK_KEY_RELEASE_MASK);*/
  


  ((GUI_Object*)sbw)->window=window;
  
  width=((GUI_Object*)sbw)->width;
  height=((GUI_Object*)sbw)->height;
  x=((GUI_Object*)sbw)->x;
  y=((GUI_Object*)sbw)->y;
  gtk_window_set_default_size(GTK_WINDOW(sbw->gui_obj.window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(sbw->gui_obj.window),x,y);

  sbw->gui_obj.window = window;

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC(delete_event),
		      (gpointer) sbw);

  /* Add a signal handler for key press events. This will capture
   * key commands for single stepping, running, etc.
   */
  gtk_signal_connect(GTK_OBJECT(window),"key_press_event",
		     (GtkSignalFunc) key_press,
		     (gpointer) sbw);

  /* Add a signal handler for key press events. This will capture
   * key commands for single stepping, running, etc.
   */
//  gtk_signal_connect(GTK_OBJECT(window),"key_release_event",
//		     (GtkSignalFunc) key_release,
//		     (gpointer) sbw);

  /* Add a signal handler for button press events. This will capture
   * commands for setting and/or clearing break points
   */
  gtk_signal_connect(GTK_OBJECT(window),"button_press_event",
		     (GtkSignalFunc) button_press,
		     (gpointer) sbw);

  
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  sbw->vbox=vbox;
}

gint gui_object_configure_event(GtkWidget *widget, GdkEventConfigure *e, GUI_Object *go)
{
//    struct gui_config_winattr winattr;

    if(widget->window==NULL)
	return 0;
    
    gdk_window_get_root_origin(widget->window,&go->x,&go->y);
    gdk_window_get_size(widget->window,&go->width,&go->height);
    
    gui_object_set_config(go);
/*
    winattr.x=go->x;
    winattr.y=go->y;
    winattr.width=go->width;
    winattr.height=go->height;
    winattr.visible=go->visible;
    gui_config_save_winattr(go->name,&winattr);*/
    
    return 0; // what should be returned?, FIXME
}

void SourceBrowser_change_view (struct _gui_object *_this, int view_state)
{
/*    if (GTK_WIDGET_VISIBLE(GTK_WIDGET(_this->window)))
    {
	gdk_window_get_size(_this->window->window, &_this->width, &_this->height);

	gdk_window_get_root_origin (_this->window->window,
				    &_this->x,
				    &_this->y);
    }*/
    
    if( (view_state==VIEW_SHOW) || (_this->window==NULL) ||
	((view_state==VIEW_TOGGLE) &&
	 !GTK_WIDGET_VISIBLE(GTK_WIDGET(_this->window)) )
      )
    {
      if(!_this->enabled)
      {
	  gui_object_get_config(_this);
	  switch(_this->wt)
	  {
	  case WT_register_window:
	      BuildRegisterWindow((Register_Window*)_this);
	      break;
	  case WT_symbol_window:
	      BuildSymbolWindow((Symbol_Window*)_this);
	      break;
	  case WT_asm_source_window:
	      BuildSourceBrowserAsmWindow((SourceBrowserAsm_Window*)_this);
	      break;
	  case WT_opcode_source_window:
	      BuildSourceBrowserOpcodeWindow((SourceBrowserOpcode_Window*)_this);
	      break;
	  case WT_watch_window:
	      BuildWatchWindow((Watch_Window*)_this);
	      break;
	  case WT_breadboard_window:
	      BuildBreadboardWindow((Breadboard_Window*)_this);
	      break;
	  default:
	      puts("SourceBrowser_change_view(): unhandled case");
	      break;
	  }
      }
      else
      {
	  gtk_widget_show(_this->window);
          _this->enabled=1;
      }

    }
  else if (GTK_WIDGET_VISIBLE(GTK_WIDGET(_this->window)))
  {
/*      if(_this->wt == WT_register_window)
      {
	  Register_Window *rw = (Register_Window *)_this;
	  if(rw->type == REGISTER_RAM)
	      rw->gui_obj.gp->status_bar->created=0;
      }
      gtk_widget_destroy(_this->window);
      _this->window=NULL;*/
      _this->enabled=0;
      gtk_widget_hide(_this->window);
  }

    // we update config database
    gui_object_set_config(_this);
    
}
