/*
   Copyright (C) 1999,2000,2001
   Ralf Forsberg

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

#include <gui.h>

#ifdef HAVE_GUI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>

#include <assert.h>

#define PINLINEWIDTH 3
#define CASELINEWIDTH 4

#define CASEOFFSET (CASELINEWIDTH/2)

#define FOORADIUS (CASELINEWIDTH) // radius of center top milling

#define LABELPAD 0 // increase this so wide lines doesn't clutter labels

GdkColor high_output_color;
GdkColor low_output_color;
GdkColor black_color;

#include "../src/modules.h"
#include "../src/stimuli.h"
#include "../src/pic-processor.h"
#include "../src/symbol.h"
#include "../src/stimuli.h"

#include <vector>

#define PINLENGTH (4*PINLINEWIDTH)

static int pinspacing = PINLENGTH;

#define LAYOUTSIZE_X 800
#define LAYOUTSIZE_Y 800

static void draw_pin(struct gui_pin *pin)
{
    int pointx;
    int wingheight, wingx;
    int casex, endx;
    int y;

    switch(pin->orientation)
    {
    case LEFT:
	casex = pin->width;
        endx = 0;
        break;
    default:
	casex = 0;
        endx = pin->width;
        break;
    }

    y = pin->height/2;

    // Clear pixmap
    gdk_draw_rectangle (pin->pixmap,
			pin->bbw->gui_obj.window->style->bg_gc[GTK_WIDGET_STATE (pin->widget)],
			TRUE,
			0, 0,
			pin->width,
			pin->height);


    if(pin->type==PIN_OTHER)
	gdk_gc_set_foreground(pin->gc,&black_color);
    else
	gdk_gc_set_foreground(pin->gc,pin->value>0?&high_output_color:&low_output_color);

    // Draw actual pin
    gdk_draw_line(pin->pixmap,pin->gc,
		  casex,y,endx,y);

    if(pin->type==PIN_OTHER)
	return;

    // Draw direction arrow
    wingheight=pin->height/3;
    
    if(casex>endx)
    {
	if(pin->direction==PIN_OUTPUT)
	{
	    pointx = endx + PINLENGTH/3;
	    wingx=endx+(PINLENGTH*2)/3;
	}
	else
	{
	    pointx = endx + (PINLENGTH*2)/3;
	    wingx=endx+PINLENGTH/3;
	}
    }
    else
    {
	if(pin->direction==PIN_OUTPUT)
	{
	    pointx = casex + (PINLENGTH*2)/3;
	    wingx=casex+PINLENGTH/3;
	}
	else
	{
	    pointx = casex + PINLENGTH/3;
	    wingx=casex+(PINLENGTH*2)/3;
	}
    }

    
    // Draw an arrow poining at (endx,endy)
    gdk_draw_line(pin->pixmap,pin->gc,
		  pointx,y,wingx,y+wingheight);
    gdk_draw_line(pin->pixmap,pin->gc,
		  pointx,y,wingx,y-wingheight);

    if(pin->widget->window!=NULL)
	gdk_draw_pixmap(pin->widget->window,
			pin->widget->style->fg_gc[GTK_WIDGET_STATE (pin->widget)],
			pin->pixmap,
			0, 0,
			0, 0,
			pin->width, pin->height);
}

//#define XOFFSET 20
//#define YOFFSET 20



static void expose_pin(GtkWidget *widget,
		       GdkEventExpose *event,
		       struct gui_pin *p)
{
    if(p->pixmap==NULL)
    {
	puts("bbw.c: no pixmap!");
	return;
    }

    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
}

static gint button(GtkWidget *widget,
		   GdkEventButton *event,
		   struct gui_pin *p)
{
    if(event->type==GDK_BUTTON_PRESS &&
       event->button==1)
    {
	if(p->direction==PIN_OUTPUT)
	{
	    p->iopin->put_state_value(p->value<0?1:0);
	}
	else
	{
	    p->iopin->toggle();
	}
	return 1;
    }
    return 0;
}



// get_value
static void a_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=TRUE;
    gtk_main_quit();
}
static void b_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=FALSE;
    gtk_main_quit();
}
// used for reading a value from user when break on value is requested
char *gui_get_string(char *prompt)
{
    static GtkWidget *dialog=NULL;
    static GtkWidget *label;
    static GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *hbox;
    
    int retval;

    char *string;
    
    if(dialog==NULL)
    {
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog),"enter value");
//	gtk_signal_connect(GTK_OBJECT(dialog),
//			   "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
	gtk_signal_connect_object(GTK_OBJECT(dialog),
				  "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(dialog));

	label=gtk_label_new("Enter string:");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,FALSE,FALSE,20);
	
	hbox = gtk_hbox_new(0,0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);

	button = gtk_button_new_with_label("OK");
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

	label=gtk_label_new(prompt);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label,
			   FALSE,FALSE, 20);

	entry=gtk_entry_new();
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry,FALSE,FALSE,20);

    }
    else
    {
	gtk_label_set_text(GTK_LABEL(label),prompt);
    }
    
//    gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
    gtk_widget_show_now(dialog);

    gtk_grab_add(dialog);
    gtk_main();
    gtk_grab_remove(dialog);
    
    gtk_widget_hide(dialog);

    if(retval)
	string=gtk_entry_get_text(GTK_ENTRY(entry));
    else
        string=NULL;
    
    return string;
}

static void add_new_snode(GtkWidget *button, Breadboard_Window *bbw)
{
    char *node_name = gui_get_string("Node name");

    if(node_name !=NULL)
	new Stimulus_Node(node_name);
}

static void xref_update(struct cross_reference_to_gui *xref, int new_value)
{
    Breadboard_Window *bbw;

    if(xref == NULL)
    {
	printf("Warning gui_breadboard.c: xref_update: xref=%p\n",xref);
	if(xref->data == NULL || xref->parent_window==NULL)
	{
	    printf("Warning gui_breadboard.c: xref_update: xref->data=%p, xref->parent_window=%p\n",xref->data,xref->parent_window);
	}
	return;
    }

    bbw  = (Breadboard_Window *) (xref->parent_window);

    BreadboardWindow_update(bbw);
}


////////////////////////////////////////////////////////////////////
/*static void ok_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=FALSE; // cancel=FALSE;
    gtk_main_quit();
}*/
static gint ok_cb(GtkWidget *widget,
		   GdkEventButton *event,
		    gpointer user_data)
{
    if(event->type==GDK_2BUTTON_PRESS &&
       event->button==1)
    {
	*(int*)user_data=FALSE; // cancel=FALSE;
	gtk_main_quit();
	return 1;
    }
    return 0;
}
static void cancel_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=TRUE; // cancel=TRUE;
    gtk_main_quit();
}

// Select row
static void node_cb(GtkCList       *clist,
		    gint            row,
		    gint            column,
		    GdkEvent       *event,
		    gpointer user_data)

{
    Stimulus_Node *snode;

    snode = (Stimulus_Node*)gtk_clist_get_row_data (clist, row);

    *((Stimulus_Node**) user_data)=snode;
}
static void copy_node_tree_to_clist(GtkWidget *item, gpointer clist)
{
    Stimulus_Node *node;
    char name[128];
    char *text[1]={name};
    int row;

//    text[0]=name;

    node = (Stimulus_Node*)gtk_object_get_data(GTK_OBJECT(item), "snode");


    strcpy(name,node->name());

    row = gtk_clist_append(GTK_CLIST(clist),
			   text);

    gtk_clist_set_row_data (GTK_CLIST(clist),
			    row,
			    (gpointer)node);
}

static Stimulus_Node *select_node_dialog(Breadboard_Window *bbw)
{
    GtkWidget *dialog;
    GtkWidget *node_clist;
    GtkWidget *okbutton;
    GtkWidget *cancelbutton;
    int cancel;

    Stimulus_Node *snode=NULL;

	GtkWidget *vbox;
	GtkWidget *scrolledwindow;
        GtkWidget *hbox;

//    if(window==NULL)
//    {

        // Build window
	dialog = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW (dialog), "Select node to connect to");

	vbox = GTK_DIALOG(dialog)->vbox;

	scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scrolledwindow);
	gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	node_clist = gtk_clist_new (1);
	gtk_widget_show (node_clist);
	gtk_container_add (GTK_CONTAINER(scrolledwindow), node_clist);

//	hbox = gtk_hbox_new (FALSE, 0);
//	gtk_widget_show (hbox);
//	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

/*	okbutton = gtk_button_new_with_label ("Add stimuli to node ...");
	gtk_widget_show (okbutton);
	gtk_box_pack_start (GTK_BOX (hbox), okbutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(okbutton),"clicked",
			   GTK_SIGNAL_FUNC(ok_cb),(gpointer)&cancel);*/

	cancelbutton = gtk_button_new_with_label ("Cancel");
	gtk_widget_show (cancelbutton);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(dialog)->action_area), cancelbutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(cancelbutton),"clicked",
			   GTK_SIGNAL_FUNC(cancel_cb),(gpointer)&cancel);
//    }

	gtk_signal_connect(GTK_OBJECT(node_clist),
			   "select_row",
			   (GtkSignalFunc) node_cb,
			   (gpointer)&snode);
//    gtk_widget_set_events(pin->widget,
//			  gtk_widget_get_events(pin->widget)|
//			  GDK_BUTTON_PRESS_MASK);
	gtk_signal_connect(GTK_OBJECT(node_clist),
			   "button_press_event",
			   GTK_SIGNAL_FUNC(ok_cb),
			   (gpointer)&cancel);

        // Add all nodes
	gtk_container_foreach(GTK_CONTAINER(bbw->node_tree),
			      copy_node_tree_to_clist,
			      (gpointer)node_clist);

	gtk_widget_set_usize(dialog, 320, 200);


    gtk_widget_show(dialog);

    gtk_grab_add(dialog);
    gtk_main();
    gtk_grab_remove(dialog);


    if(cancel)
    {
	gtk_widget_destroy(dialog);
	return NULL;
    }

    gtk_widget_destroy(dialog);
    // Get clist selection

    return snode;
}

static void stimuli_add_node(GtkWidget *button, Breadboard_Window *bbw)
{

    struct Stimulus_Node *node;


    node = select_node_dialog(bbw);

    if(node!=NULL && bbw->selected_pin!=NULL)
    {
        node->attach_stimulus(bbw->selected_pin->iopin);
    }
}

static struct gui_pin *create_iopin(Breadboard_Window *bbw, int x, int y, orientation orientation, IOPIN *iopin)
{
    struct gui_pin *pin;

    pin=(struct gui_pin *)malloc(sizeof(*pin));

    pin->iopin = iopin;
    pin->x=x;
    pin->y=y;
    pin->width=pinspacing;
    pin->height=pinspacing;
    pin->bbw=bbw;
    if(iopin!=NULL)
    {
	pin->value=iopin->get_state();
	pin->direction=iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;
	pin->orientation=orientation;
        pin->type=PIN_DIGITAL;
    }
    else
    {
	pin->value=0;
	pin->direction=PIN_INPUT;
        pin->orientation=orientation;
	pin->type=PIN_OTHER;
    }

    if(pin->orientation==LEFT)
        pin->x-=pin->width;

    // Create widget
    pin->widget = gtk_drawing_area_new();
    gtk_widget_set_events(pin->widget,
			  gtk_widget_get_events(pin->widget)|
			  GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect(GTK_OBJECT(pin->widget),
		       "button_press_event",
		       (GtkSignalFunc) button,
		       pin);

    gtk_drawing_area_size(GTK_DRAWING_AREA(pin->widget),pin->width,pin->height);
    gtk_signal_connect(GTK_OBJECT(pin->widget),
		       "expose_event",
		       (GtkSignalFunc) expose_pin,
		       pin);

    // Create pixmap
    pin->pixmap = gdk_pixmap_new(bbw->gui_obj.window->window,
				pin->width,
				pin->height,
				-1);

    // Draw pin
    pin->gc=gdk_gc_new(bbw->gui_obj.window->window);
    g_assert(pin->gc!=NULL);
    gdk_gc_set_line_attributes(pin->gc,PINLINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);
    draw_pin(pin);
//    gdk_draw_line(pin->pixmap,pin->gc,0,0,3,pin->height);

    gtk_widget_show(pin->widget);

    return pin;
}

static void expose(GtkWidget *widget, GdkEventExpose *event, struct gui_module *p)
{
    if(p->pixmap==NULL)
    {
	puts("bbw.c: no pixmap!");
	return;
    }

    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    p->pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
}

static void treeselect_stimuli(GtkItem *item, struct gui_pin *pin)
{
    char text[128];
    char string[128];

    gtk_widget_show(pin->bbw->stimuli_frame);
    gtk_widget_hide(pin->bbw->node_frame);
    gtk_widget_hide(pin->bbw->module_frame);
    gtk_widget_hide(pin->bbw->pic_frame);

    snprintf(string,sizeof(string),"Stimuli %s",pin->iopin->name());
    gtk_frame_set_label(GTK_FRAME(pin->bbw->stimuli_frame),string);

    if(pin->iopin->snode!=NULL)
	snprintf(text,sizeof(text),"Connected to node %s", pin->iopin->snode->name());
    else
        strcpy(text,"Not connected");

    gtk_label_set_text(GTK_LABEL(pin->bbw->stimuli_settings_label), text);

    pin->bbw->selected_pin = pin;
}

static void treeselect_node(GtkItem *item, struct gui_node *gui_node)
{
    char name[128];
    char *text[1];
    stimulus *stimulus;

    text[0]=name;

    if(gui_node->node!=NULL)
	gtk_widget_show(gui_node->bbw->node_frame);
    else
	gtk_widget_hide(gui_node->bbw->node_frame);
    gtk_widget_hide(gui_node->bbw->stimuli_frame);
    gtk_widget_hide(gui_node->bbw->module_frame);
    gtk_widget_hide(gui_node->bbw->pic_frame);

    // Clear node_clist
    gtk_clist_clear(GTK_CLIST(gui_node->bbw->node_settings_clist));

    if(gui_node->node!=NULL)
    {
	// Add to node_clist
	stimulus = gui_node->node->stimuli;

	while(stimulus!=NULL)
	{
	    strncpy(name, stimulus->name(), sizeof(name));
	    gtk_clist_append(GTK_CLIST(gui_node->bbw->node_settings_clist),
			     text);
	    stimulus = stimulus->next;
	}
    }

    gui_node->bbw->selected_node = gui_node;
}

static void treeselect_module(GtkItem *item, struct gui_module *p)
{
    char string[128];
    snprintf(string,sizeof(string),"%s settings",p->module->name());
    switch(p->type)
    {
    case PIC_MODULE:
	gtk_widget_hide(p->bbw->stimuli_frame);
	gtk_widget_hide(p->bbw->node_frame);
	gtk_widget_hide(p->bbw->module_frame);
	gtk_widget_show(p->bbw->pic_frame);
        gtk_frame_set_label(GTK_FRAME(p->bbw->pic_frame),string);
        break;
    case EXTERNAL_MODULE:
	gtk_widget_hide(p->bbw->stimuli_frame);
	gtk_widget_hide(p->bbw->node_frame);
	gtk_widget_hide(p->bbw->pic_frame);
	gtk_widget_show(p->bbw->module_frame);
        gtk_frame_set_label(GTK_FRAME(p->bbw->module_frame),string);
        break;
    }

    p->bbw->selected_module = p;
}

#define PACKAGESPACING 50

struct gui_module *create_module(Breadboard_Window *bbw,
                                 enum module_type type,
				 class Module *module,
				 GtkWidget *widget)
{
    static int x=30;
    static int y=30;
    static int max_y;

    struct gui_module *p;
    int i;

    p = (struct gui_module*) malloc(sizeof(*p));

    p->bbw=bbw;
    p->module=module;
    p->module_widget = widget;
    p->x=x;
    p->y=y;
    p->type=type;


    p->pins=NULL;


    GtkWidget *tree_item;
    tree_item = gtk_tree_item_new_with_label (p->module->name());
    gtk_signal_connect(GTK_OBJECT(tree_item),
		       "select",
		       (GtkSignalFunc) treeselect_module,
		       p);
    gtk_widget_show(tree_item);
    gtk_tree_append(GTK_TREE(bbw->tree), tree_item);

    if(p->module_widget==NULL)
    {
	// Create a static representation.
	int pin_x, pin_y;
	int pic_id;
	GtkWidget *da;
	
	pic_id = ((GUI_Object*)bbw)->gp->pic_id;

	p->pinnamewidth=0;
	for(i=1;i<=p->module->get_pin_count();i++)
	{
	    char *name;
	    int width;

	    name=p->module->get_pin_name(i);
	    if(name==NULL)
		continue;
	    width = gdk_string_width (bbw->pinnamefont,name)+LABELPAD;
	    if(width>p->pinnamewidth)
		p->pinnamewidth=width;
	}

        p->pinnamewidth+=FOORADIUS; // The 'U' at the top of the DIL module

	p->width=p->pinnamewidth*2; // pin name widthts
	p->width+=2*CASELINEWIDTH+2*LABELPAD;

	p->height=module->get_pin_count()/2*pinspacing; // pin name height
	if(module->get_pin_count()%2)
            p->height+=pinspacing;
        p->height+=2*CASELINEWIDTH+2*LABELPAD;


	if(p->x+p->width>LAYOUTSIZE_X-30)
	{
	    x=30;
	    y=max_y+PACKAGESPACING;
	}

	p->x=x;
	p->y=y;

	da = gtk_drawing_area_new();
/*	gtk_widget_set_events(da,
			      gtk_widget_get_events(da)|
			      GDK_BUTTON_PRESS_MASK);
	gtk_signal_connect(GTK_OBJECT(bbw->da),
			   "button_press_event",
			   (GtkSignalFunc) button,
			   p);
  */

	gtk_drawing_area_size(GTK_DRAWING_AREA(da),p->width,p->height);



	p->pixmap = gdk_pixmap_new(bbw->gui_obj.window->window,
				p->width,
				p->height,
				-1);



	gdk_draw_rectangle (p->pixmap,
			    ((GUI_Object*)bbw)->window->style->bg_gc[GTK_WIDGET_STATE (da)],
			    TRUE,
			    0, 0,
			    p->width,
			    p->height);

	gdk_draw_rectangle (p->pixmap,
			    ((GUI_Object*)bbw)->window->style->white_gc,
			    TRUE,
			    CASEOFFSET, CASEOFFSET,
			    p->width-CASEOFFSET,
			    p->height-CASEOFFSET);

	// Draw pin names
	for(i=1;i<=p->module->get_pin_count();i++)
	{
	    char *name;
	    int label_x, label_y;

	    if(i<=p->module->get_pin_count()/2 || i<=p->module->get_pin_count()%2)
	    {
		label_x=LABELPAD+CASELINEWIDTH;
		label_y=LABELPAD+CASELINEWIDTH-bbw->pinnameheight/3+(i-1)*pinspacing+pinspacing/2;
	    }
	    else
	    {
		label_x=LABELPAD+p->width/2+FOORADIUS;
		label_y=LABELPAD+CASELINEWIDTH-bbw->pinnameheight/3+(p->module->get_pin_count()-i)*pinspacing+pinspacing/2;
	    }

	    name=p->module->get_pin_name(i);
	    if(name==NULL)
		continue;
	    gdk_draw_text(p->pixmap,
			  p->bbw->pinnamefont,
			  p->bbw->pinname_gc,
			  label_x,
			  PINLENGTH/2+label_y,
			  name,strlen(name));
	}

	// Draw casing
//	x=XOFFSET+p->case_x;
//	y=XOFFSET+p->case_y;

	gdk_gc_set_foreground(p->bbw->case_gc,&black_color);
//	gdk_draw_line(p->pixmap,p->bbw->case_gc,10,10,30,130);

        // Draw case outline
	gdk_draw_line(p->pixmap,p->bbw->case_gc,CASEOFFSET,CASEOFFSET,p->width/2-FOORADIUS,CASEOFFSET);
	gdk_draw_line(p->pixmap,p->bbw->case_gc,p->width-CASEOFFSET,CASEOFFSET,p->width/2+FOORADIUS,CASEOFFSET);
	gdk_draw_line(p->pixmap,p->bbw->case_gc,p->width-CASEOFFSET,CASEOFFSET,p->width-CASEOFFSET,p->height-CASEOFFSET);
	gdk_draw_line(p->pixmap,p->bbw->case_gc,CASEOFFSET,p->height-CASEOFFSET,p->width-CASEOFFSET,p->height-CASEOFFSET);
	gdk_draw_line(p->pixmap,p->bbw->case_gc,CASEOFFSET,CASEOFFSET,CASEOFFSET,p->height-CASEOFFSET);
	gdk_draw_arc(p->pixmap,((GUI_Object*)bbw)->window->style->bg_gc[GTK_WIDGET_STATE (da)],TRUE,p->width/2-FOORADIUS,CASEOFFSET-FOORADIUS,2*FOORADIUS,2*FOORADIUS,180*64,180*64);
	gdk_draw_arc(p->pixmap,p->bbw->case_gc,FALSE,p->width/2-FOORADIUS,CASEOFFSET-FOORADIUS,2*FOORADIUS,2*FOORADIUS,180*64,180*64);



/*	gdk_draw_pixmap(da->window,
			((GUI_Object*)bbw)->window->style->fg_gc[GTK_WIDGET_STATE (da)],
			p->pixmap,
			0, 0,
		    0, 0,
			p->width,
			p->height);**/

	gtk_signal_connect(GTK_OBJECT(da),
			   "expose_event",
			   (GtkSignalFunc) expose,
			   p);

        p->module_widget=da;
    }

    gtk_layout_put(GTK_LAYOUT(bbw->layout),
		   p->module_widget,p->x,p->y);

    gtk_widget_show(p->module_widget);


    // Create pins
    GtkWidget *subtree = gtk_tree_new();
    gtk_widget_show(subtree);
    gtk_tree_item_set_subtree(GTK_TREE_ITEM(tree_item), subtree);
    for(i=1;i<=p->module->get_pin_count();i++)
    {
	int pin_x, pin_y;
	struct gui_pin *pin;
	enum orientation orientation;
        char *name;
	struct cross_reference_to_gui *cross_reference;
        IOPIN *iopin;

        iopin = p->module->get_pin(i);

	if(iopin!=NULL)
	{
	    // Create xref
	    cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
	    cross_reference->parent_window_type = WT_breadboard_window;
	    cross_reference->parent_window = (gpointer) bbw;
	    cross_reference->data = (gpointer) NULL;
	    cross_reference->update = xref_update;
	    cross_reference->remove = NULL;
	    //gpsim_assign_pin_xref(pic_id,pin, cross_reference);
	    iopin->xref->add(cross_reference);
	}

        // Put pin in layout
	if(i<=p->module->get_pin_count()/2 || i<=p->module->get_pin_count()%2)
	{
	    pin_x=0;
	    pin_y=(i-1)*pinspacing+pinspacing/2;
            orientation = LEFT;
	}
	else
	{
	    pin_x=p->width;
	    pin_y=(p->module->get_pin_count()-i)*pinspacing+pinspacing/2;
            orientation = RIGHT;
	}

	pin = create_iopin(bbw,
			   pin_x,
			   pin_y,
			   orientation,
			   iopin);

	gtk_layout_put(GTK_LAYOUT(bbw->layout),
		       pin->widget,p->x+pin->x,p->y+pin->y);


	p->pins = g_list_append(p->pins, pin);

        // Add pin to tree
	name=p->module->get_pin_name(i);
	if(name!=NULL)
	{
	    tree_item = gtk_tree_item_new_with_label (name);
	    gtk_signal_connect(GTK_OBJECT(tree_item),
			       "select",
			       (GtkSignalFunc) treeselect_stimuli,
			       pin);
	    gtk_widget_show(tree_item);
	    gtk_tree_append(GTK_TREE(subtree), tree_item);
	}
    }







    bbw->modules=g_list_append(bbw->modules, p);

    x+=p->width+PACKAGESPACING;

    if(y+p->height>max_y)
	max_y=y+p->height;
}


void BreadboardWindow_update(Breadboard_Window *bbw)
{
    GList *iter;


    // loop all modules and update their pins

    iter=bbw->modules;
    while(iter!=NULL)
    {
	GList *pin_iter;
	struct gui_module *p;

        p = (struct gui_module*)iter->data;

	pin_iter=p->pins;
	while(pin_iter!=NULL)
	{
	    struct gui_pin *pin;

	    int value;
            direction dir;

	    pin = (struct gui_pin *) pin_iter->data;

	    if(pin->iopin!=NULL)
	    {
		value=pin->iopin->get_state();
		dir=pin->iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;

		if(value!=pin->value || dir!=pin->direction)
		{
		    pin->value=value;
		    pin->direction=dir;

		    draw_pin(pin);
		}
	    }
            pin_iter = pin_iter->next;
	}
        iter = iter->next;
    }
}

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Watch_Window *ww)
{
    ((GUI_Object *)ww)->change_view((GUI_Object*)ww,VIEW_HIDE);
    return TRUE;
}

/* When a processor is created */
void BreadboardWindow_new_processor(Breadboard_Window *bbw, GUI_Processor *gp)
{
    char buf[128];
    int i;
    unsigned int pic_id;
    int pin;

    bbw->processor=1;

    if(!bbw->gui_obj.enabled)
	return;

    
    pic_id = ((GUI_Object*)bbw)->gp->pic_id;

    if(!pic_id)
    {
	puts("BreadboardWindow_new_processor(): pic_id==0");
	return;
    }

    struct gui_module *p=create_module(bbw, PIC_MODULE, get_processor(pic_id),NULL);
//    draw_pins(p);

//    BreadboardWindow_update(bbw);
}

/* When a module is created */
void BreadboardWindow_new_module(Breadboard_Window *bbw, Module *module)
{
    GtkWidget *widget=NULL;

    if(module->widget!=NULL)
        widget=GTK_WIDGET(module->widget);
    struct gui_module *p=create_module(bbw, EXTERNAL_MODULE, module, widget);
}


/* When a stimuli is being connected or disconnected, or a new node is created */
void BreadboardWindow_node_configuration_changed(Breadboard_Window *bbw,Stimulus_Node *node)
{
    struct gui_node * gn = (struct gui_node*) gtk_object_get_data(GTK_OBJECT(bbw->node_tree), node->name());

    if(gn==NULL)
    {
	GtkWidget *node_item;

	gn = (struct gui_node *) malloc(sizeof(*gn));

	gn->bbw=bbw;
        gn->node=node;

	node_item = gtk_tree_item_new_with_label (node->name());
	gtk_signal_connect(GTK_OBJECT(node_item),
			   "select",
			   (GtkSignalFunc) treeselect_node,
			   gn);
	gtk_widget_show(node_item);
	gtk_tree_append(GTK_TREE(bbw->node_tree), node_item);
	gtk_object_set_data(GTK_OBJECT(bbw->node_tree), node->name(), gn);
        gtk_object_set_data(GTK_OBJECT(node_item), "snode", node);
    }

/*    delete gn->stimuli_list;

    loop node->stimuli, and add them to gn->stimuli_list

    update_tree();*/
}

int BuildBreadboardWindow(Breadboard_Window *bbw)
{
    
    GtkWidget *window;

  GtkWidget *hpaned1;
  GtkWidget *vbox9;
  GtkWidget *vbox13;
  GtkWidget *scrolledwindow4;
  GtkWidget *viewport9;
  GtkWidget *tree1;
  GtkWidget *hbox12;
  GtkWidget *button5;
  GtkWidget *button6;
  GtkWidget *vbox12;
  GtkWidget *scrolledwindow3;
  GtkWidget *viewport8;
  GtkWidget *hbox11;
  GtkWidget *pic_settings_entry;
  GtkWidget *pic_settings_button;

  GtkWidget *vbox11;
  GtkWidget *scrolledwindow2;
  GtkWidget *viewport7;
  GtkWidget *hbox10;
  GtkWidget *remove_node_button;
  GtkWidget *remove_stimuli_button;

  GtkWidget *vbox14;
//  GtkWidget *scrolledwindow6;
//  GtkWidget *viewport10;
  GtkWidget *hbox13;

  GtkWidget *vbox10;
  GtkWidget *scrolledwindow1;
  GtkWidget *viewport6;
  GtkWidget *hbox9;
  GtkWidget *module_settings_entry;
  GtkWidget *module_settings_button;
  GtkWidget *save_stc_button;
  GtkWidget *scrolledwindow5;
  GtkWidget *pic_settings_clist;
  GtkWidget *node_settings_clist;
  GtkWidget *module_settings_clist;

    int x,y,width,height;

    GdkColormap *colormap = gdk_colormap_get_system();

    gdk_color_parse("red",&high_output_color);
    gdk_color_parse("darkgreen",&low_output_color);
    g_assert(gdk_color_parse("black",&black_color)!=FALSE);
  
    gdk_colormap_alloc_color(colormap, &high_output_color,FALSE,TRUE);
    gdk_colormap_alloc_color(colormap, &low_output_color,FALSE,TRUE);
    gdk_colormap_alloc_color(colormap, &black_color,FALSE,TRUE);





  window = bbw->gui_obj.window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (window), "window", window);
  gtk_window_set_title (GTK_WINDOW (window), "Breadboard [Currently in development]");

  hpaned1 = gtk_hpaned_new ();
  gtk_widget_ref (hpaned1);
  gtk_object_set_data_full (GTK_OBJECT (window), "hpaned1", hpaned1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hpaned1);
  gtk_container_add (GTK_CONTAINER (window), hpaned1);
  gtk_paned_set_position (GTK_PANED (hpaned1), 196);

  vbox9 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox9);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox9", vbox9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox9);
  gtk_paned_pack1 (GTK_PANED (hpaned1), vbox9, FALSE, TRUE);

  vbox13 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox13);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox13", vbox13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox13);
  gtk_box_pack_start (GTK_BOX (vbox9), vbox13, TRUE, TRUE, 2);

  scrolledwindow4 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow4);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow4", scrolledwindow4,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow4);
  gtk_box_pack_start (GTK_BOX (vbox13), scrolledwindow4, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow4), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  viewport9 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport9);
  gtk_object_set_data_full (GTK_OBJECT (window), "viewport9", viewport9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport9);
  gtk_container_add (GTK_CONTAINER (scrolledwindow4), viewport9);

  bbw->tree = tree1 = gtk_tree_new ();
  gtk_widget_ref (tree1);
  gtk_object_set_data_full (GTK_OBJECT (window), "tree1", tree1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_tree_set_selection_mode (GTK_TREE(bbw->tree),GTK_SELECTION_BROWSE);
  gtk_widget_show (tree1);
  gtk_container_add (GTK_CONTAINER (viewport9), tree1);

  hbox12 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox12);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox12", hbox12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox12);
  gtk_box_pack_start (GTK_BOX (vbox13), hbox12, FALSE, FALSE, 0);

  button5 = gtk_button_new_with_label ("Add node");
  gtk_widget_ref (button5);
  gtk_object_set_data_full (GTK_OBJECT (window), "button5", button5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button5);
  gtk_box_pack_start (GTK_BOX (hbox12), button5, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(button5),
		     "clicked",
		     (GtkSignalFunc) add_new_snode,
		     bbw);

  button6 = gtk_button_new_with_label ("Add module");
  gtk_widget_ref (button6);
  gtk_object_set_data_full (GTK_OBJECT (window), "button6", button6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (button6);
  gtk_box_pack_start (GTK_BOX (hbox12), button6, FALSE, FALSE, 0);





  bbw->pic_frame = gtk_frame_new ("PIC settings");
  gtk_widget_ref (bbw->pic_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_frame", bbw->pic_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
//  gtk_widget_show (pic_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), bbw->pic_frame, TRUE, TRUE, 0);

  vbox12 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox12);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox12", vbox12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox12);
  gtk_container_add (GTK_CONTAINER (bbw->pic_frame), vbox12);

  scrolledwindow3 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow3);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow3", scrolledwindow3,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow3);
  gtk_box_pack_start (GTK_BOX (vbox12), scrolledwindow3, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow3), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  viewport8 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport8);
  gtk_object_set_data_full (GTK_OBJECT (window), "viewport8", viewport8,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport8);
  gtk_container_add (GTK_CONTAINER (scrolledwindow3), viewport8);

  pic_settings_clist = gtk_clist_new (1);
  gtk_widget_ref (pic_settings_clist);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_settings_clist", pic_settings_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pic_settings_clist);
  gtk_container_add (GTK_CONTAINER (viewport8), pic_settings_clist);

  hbox11 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox11);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox11", hbox11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox11);
  gtk_box_pack_start (GTK_BOX (vbox12), hbox11, FALSE, FALSE, 0);

  pic_settings_entry = gtk_entry_new ();
  gtk_widget_ref (pic_settings_entry);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_settings_entry", pic_settings_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pic_settings_entry);
  gtk_box_pack_start (GTK_BOX (hbox11), pic_settings_entry, FALSE, FALSE, 0);

  pic_settings_button = gtk_button_new_with_label ("Set");
  gtk_widget_ref (pic_settings_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_settings_button", pic_settings_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pic_settings_button);
  gtk_box_pack_start (GTK_BOX (hbox11), pic_settings_button, FALSE, FALSE, 0);





  bbw->node_frame = gtk_frame_new ("Node connections");
  gtk_widget_ref (bbw->node_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "node_frame", bbw->node_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
//  gtk_widget_show (node_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), bbw->node_frame, TRUE, TRUE, 0);

  vbox11 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox11);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox11", vbox11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox11);
  gtk_container_add (GTK_CONTAINER (bbw->node_frame), vbox11);

  scrolledwindow2 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow2);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow2", scrolledwindow2,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow2);
  gtk_box_pack_start (GTK_BOX (vbox11), scrolledwindow2, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow2), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  viewport7 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport7);
  gtk_object_set_data_full (GTK_OBJECT (window), "viewport7", viewport7,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport7);
  gtk_container_add (GTK_CONTAINER (scrolledwindow2), viewport7);

  bbw->node_settings_clist = gtk_clist_new (1);
  gtk_widget_ref (bbw->node_settings_clist);
  gtk_object_set_data_full (GTK_OBJECT (window), "bbw->node_settings_clist", bbw->node_settings_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bbw->node_settings_clist);
  gtk_container_add (GTK_CONTAINER (viewport7), bbw->node_settings_clist);

  hbox10 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox10);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox10", hbox10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox10);
  gtk_box_pack_start (GTK_BOX (vbox11), hbox10, FALSE, FALSE, 0);

  remove_stimuli_button = gtk_button_new_with_label ("Remove stimuli");
  gtk_widget_ref (remove_stimuli_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "remove_stimuli_button", remove_stimuli_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (remove_stimuli_button);
  gtk_box_pack_start (GTK_BOX (hbox10), remove_stimuli_button, FALSE, FALSE, 0);

  remove_node_button = gtk_button_new_with_label ("Remove node");
  gtk_widget_ref (remove_node_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "remove_node_button", remove_node_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (remove_node_button);
  gtk_box_pack_start (GTK_BOX (hbox10), remove_node_button, FALSE, FALSE, 0);





  bbw->stimuli_frame = gtk_frame_new ("Stimuli settings");
  gtk_widget_ref (bbw->stimuli_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "stimuli_frame", bbw->stimuli_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
//  gtk_widget_show (stimuli_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), bbw->stimuli_frame, FALSE, FALSE, 0);

  vbox14 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox14);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox14", vbox14,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox14);
  gtk_container_add (GTK_CONTAINER (bbw->stimuli_frame), vbox14);


  bbw->stimuli_settings_label=gtk_label_new("");
  gtk_widget_show(bbw->stimuli_settings_label);
  gtk_box_pack_start(GTK_BOX(vbox14), bbw->stimuli_settings_label, FALSE,FALSE,0);
  /*
  scrolledwindow6 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow6);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow6", scrolledwindow6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow6);
  gtk_box_pack_start (GTK_BOX (vbox14), scrolledwindow6, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow6), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  viewport10 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport10);
  gtk_object_set_data_full (GTK_OBJECT (window), "viewport10", viewport10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport10);
  gtk_container_add (GTK_CONTAINER (scrolledwindow6), viewport10);

  bbw->stimuli_settings_clist = gtk_clist_new (1);
  gtk_widget_ref (bbw->stimuli_settings_clist);
  gtk_object_set_data_full (GTK_OBJECT (window), "bbw->stimuli_settings_clist", bbw->stimuli_settings_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bbw->stimuli_settings_clist);
  gtk_container_add (GTK_CONTAINER (viewport10), bbw->stimuli_settings_clist);
*/
  hbox13 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox13);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox13", hbox13,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox13);
  gtk_box_pack_start (GTK_BOX (vbox14), hbox13, FALSE, FALSE, 0);

  bbw->stimuli_add_node_button = gtk_button_new_with_label ("Connect stimuli to node");
  gtk_widget_ref (bbw->stimuli_add_node_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "stimuli_add_node_button", bbw->stimuli_add_node_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bbw->stimuli_add_node_button);
  gtk_box_pack_start (GTK_BOX (hbox13), bbw->stimuli_add_node_button, FALSE, FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(bbw->stimuli_add_node_button),
		     "clicked",
		     (GtkSignalFunc) stimuli_add_node,
		     bbw);





  bbw->module_frame = gtk_frame_new ("Module settings");
  gtk_widget_ref (bbw->module_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "module_frame", bbw->module_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
//  gtk_widget_show (module_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), bbw->module_frame, TRUE, TRUE, 0);

  vbox10 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox10);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox10", vbox10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox10);
  gtk_container_add (GTK_CONTAINER (bbw->module_frame), vbox10);

  scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow1);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow1", scrolledwindow1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow1);
  gtk_box_pack_start (GTK_BOX (vbox10), scrolledwindow1, TRUE, TRUE, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

  viewport6 = gtk_viewport_new (NULL, NULL);
  gtk_widget_ref (viewport6);
  gtk_object_set_data_full (GTK_OBJECT (window), "viewport6", viewport6,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (viewport6);
  gtk_container_add (GTK_CONTAINER (scrolledwindow1), viewport6);

  module_settings_clist = gtk_clist_new (1);
  gtk_widget_ref (module_settings_clist);
  gtk_object_set_data_full (GTK_OBJECT (window), "module_settings_clist", module_settings_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (module_settings_clist);
  gtk_container_add (GTK_CONTAINER (viewport6), module_settings_clist);

  hbox9 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox9);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox9", hbox9,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox9);
  gtk_box_pack_start (GTK_BOX (vbox10), hbox9, FALSE, FALSE, 0);

  module_settings_entry = gtk_entry_new ();
  gtk_widget_ref (module_settings_entry);
  gtk_object_set_data_full (GTK_OBJECT (window), "module_settings_entry", module_settings_entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (module_settings_entry);
  gtk_box_pack_start (GTK_BOX (hbox9), module_settings_entry, FALSE, FALSE, 0);

  module_settings_button = gtk_button_new_with_label ("Set");
  gtk_widget_ref (module_settings_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "module_settings_button", module_settings_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (module_settings_button);
  gtk_box_pack_start (GTK_BOX (hbox9), module_settings_button, FALSE, FALSE, 0);







  save_stc_button = gtk_button_new_with_label ("Save configuration  ...");
  gtk_widget_ref (save_stc_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "save_stc_button", save_stc_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (save_stc_button);
  gtk_box_pack_start (GTK_BOX (vbox9), save_stc_button, FALSE, FALSE, 0);

  scrolledwindow5 = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (scrolledwindow5);
  gtk_object_set_data_full (GTK_OBJECT (window), "scrolledwindow5", scrolledwindow5,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (scrolledwindow5);
  gtk_paned_pack2 (GTK_PANED (hpaned1), scrolledwindow5, TRUE, TRUE);

  bbw->layout = gtk_layout_new (NULL, NULL);
  gtk_widget_ref (bbw->layout);
  gtk_object_set_data_full (GTK_OBJECT (window), "bbw->layout", bbw->layout,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (bbw->layout);
  gtk_container_add (GTK_CONTAINER (scrolledwindow5), bbw->layout);
  gtk_layout_set_size (GTK_LAYOUT (bbw->layout), LAYOUTSIZE_X, LAYOUTSIZE_Y);
  GTK_ADJUSTMENT (GTK_LAYOUT (bbw->layout)->hadjustment)->step_increment = 10;
  GTK_ADJUSTMENT (GTK_LAYOUT (bbw->layout)->vadjustment)->step_increment = 10;






  width=((GUI_Object*)bbw)->width;
  height=((GUI_Object*)bbw)->height;
  x=((GUI_Object*)bbw)->x;
  y=((GUI_Object*)bbw)->y;
  gtk_window_set_default_size(GTK_WINDOW(bbw->gui_obj.window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(bbw->gui_obj.window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(bbw->gui_obj.window),bbw->gui_obj.name,"Gpsim");
  
  gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
			     GTK_SIGNAL_FUNC (gtk_widget_destroyed), GTK_OBJECT(window));
  gtk_signal_connect (GTK_OBJECT (bbw->gui_obj.window), "delete_event",
			    GTK_SIGNAL_FUNC(delete_event), (gpointer)bbw);
  gtk_signal_connect_after(GTK_OBJECT(bbw->gui_obj.window), "configure_event",
			   GTK_SIGNAL_FUNC(gui_object_configure_event),bbw);



  gtk_widget_show_now(window);

	bbw->pinname_gc=gdk_gc_new(bbw->gui_obj.window->window);

	bbw->case_gc=gdk_gc_new(bbw->gui_obj.window->window);
	gdk_gc_set_line_attributes(bbw->case_gc,CASELINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);

	bbw->pinstatefont = gdk_fontset_load ("-adobe-courier-bold-r-*-*-*-80-*-*-*-*-*-*");

	bbw->pinnamefont = gdk_fontset_load ("-adobe-courier-bold-r-*-*-*-80-*-*-*-*-*-*");

  bbw->pinnameheight = gdk_string_height (bbw->pinnamefont,"9y");

  if(pinspacing<bbw->pinnameheight)
      pinspacing=bbw->pinnameheight+2;



    GtkWidget *tree_item;
    struct gui_node *gn;

    gn = (struct gui_node *) malloc(sizeof(*gn));
    gn->bbw=bbw;
    gn->node=NULL; // indicates that this is the root node.
    tree_item = gtk_tree_item_new_with_label ("nodes");
    gtk_signal_connect(GTK_OBJECT(tree_item),
		       "select",
		       (GtkSignalFunc) treeselect_node,
		       gn);
    gtk_widget_show(tree_item);
    gtk_tree_append(GTK_TREE(bbw->tree), tree_item);
    bbw->node_tree= gtk_tree_new();
    gtk_widget_show(bbw->node_tree);
    gtk_tree_item_set_subtree(GTK_TREE_ITEM(tree_item), bbw->node_tree);
    gtk_object_set_data(GTK_OBJECT(bbw->node_tree), "root_of_nodes", gn);


  bbw->gui_obj.enabled=1;

  bbw->gui_obj.is_built=1;

  if(bbw->processor)
      BreadboardWindow_new_processor(bbw, ((GUI_Object*)bbw)->gp);

  update_menu_item((GUI_Object*)bbw);
  
  return 0;
}


int CreateBreadboardWindow(GUI_Processor *gp)
{
    Breadboard_Window *bbw;

    bbw = (struct _Breadboard_Window*)malloc(sizeof(struct _Breadboard_Window));

  bbw->gui_obj.gp = gp;
  bbw->gui_obj.name = "pinout";
  bbw->gui_obj.wc = WC_misc;
  bbw->gui_obj.wt = WT_breadboard_window;
  bbw->gui_obj.change_view = SourceBrowser_change_view;
  bbw->gui_obj.window = NULL;
  bbw->gui_obj.is_built = 0;

  gp->breadboard_window = bbw;

    bbw->processor=0;
//    bbw->da = NULL;
    bbw->pinstatefont = NULL;
    bbw->pinnamefont = NULL;
    bbw->pinname_gc = NULL;
    bbw->case_gc = NULL;
    bbw->gui_obj.enabled = 0;
    bbw->node_tree = NULL;

    bbw->modules=NULL;

    bbw->node_settings_clist=NULL;

    bbw->stimuli_settings_label=NULL;

    bbw->stimuli_add_node_button=NULL;

    bbw->selected_node=NULL;
    bbw->selected_pin=NULL;
    bbw->selected_module=NULL;

    gp_add_window_to_list(gp, (GUI_Object *)bbw);

    
    if(!gui_object_get_config((GUI_Object*)bbw))
      printf("warning: %s\n",__FUNCTION__);
    
    if(bbw->gui_obj.enabled)
	BuildBreadboardWindow(bbw);
  

    return 0;
}


#endif // HAVE_GUI
