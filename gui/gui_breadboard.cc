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

typedef enum {PIN_DIGITAL, PIN_ANALOG, PIN_OTHER} pintype;

GdkColor high_output_color;
GdkColor low_output_color;
GdkColor black_color;

#include "../src/modules.h"
#include "../src/stimuli.h"
#include "../src/pic-processor.h"
#include "../src/symbol.h"

#include <vector>

#define PINLENGTH (4*PINLINEWIDTH)

static int pinspacing = PINLENGTH;

#define LAYOUTSIZE_X 800
#define LAYOUTSIZE_Y 800

enum orientation {LEFT, RIGHT, UP, DOWN};
enum direction {PIN_INPUT, PIN_OUTPUT};

struct gui_pin
{
    Breadboard_Window *bbw;

    class IOPIN *iopin;

    GtkWidget *widget;
    GdkPixmap *pixmap;
    GdkGC *gc;

    int x;
    int y;
    int width;
    int height;

    int value;
    enum direction direction;
    enum orientation orientation;
    pintype type;
};

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

enum module_type {PIC_MODULE, EXTERNAL_MODULE};

struct gui_module
{
    enum module_type type;
    Breadboard_Window *bbw;
    class Module *module;
    GtkWidget *module_widget;  // As returned from module. If NULL, it becomes a static GtkPixmap.
    int x;    // Position in layout widget
    int y;    // Position in layout widget
    int width;  // Width of module_widget
    int height; // Height of module_widget

    int pinnamewidth;

    GdkPixmap *pixmap;

//    GtkWidget *left_pins;  // Set in draw_pins
//    GtkWidget *right_pins; // Set in draw_pins

    GList *pins;
};

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
    gtk_widget_hide(pin->bbw->node_frame);
    gtk_widget_hide(pin->bbw->module_frame);
    gtk_widget_hide(pin->bbw->pic_frame);
}

struct gui_node
{
    Breadboard_Window *bbw;
    Stimulus_Node *node;
};


static void treeselect_node(GtkItem *item, struct gui_node *gui_node)
{
    gtk_widget_show(gui_node->bbw->node_frame);
    gtk_widget_hide(gui_node->bbw->module_frame);
    gtk_widget_hide(gui_node->bbw->pic_frame);
}

static void treeselect_module(GtkItem *item, struct gui_module *p)
{
    char string[128];
    snprintf(string,sizeof(string),"%s settings",p->module->name());
    switch(p->type)
    {
    case PIC_MODULE:
	gtk_widget_hide(p->bbw->node_frame);
	gtk_widget_hide(p->bbw->module_frame);
	gtk_widget_show(p->bbw->pic_frame);
        gtk_frame_set_label(GTK_FRAME(p->bbw->pic_frame),string);
        break;
    case EXTERNAL_MODULE:
	gtk_widget_hide(p->bbw->node_frame);
	gtk_widget_hide(p->bbw->pic_frame);
	gtk_widget_show(p->bbw->module_frame);
        gtk_frame_set_label(GTK_FRAME(p->bbw->module_frame),string);
        break;
    }
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

	gtk_widget_show(da);

	gtk_layout_put(GTK_LAYOUT(bbw->layout),
		       da,p->x,p->y);



	p->pixmap = gdk_pixmap_new(bbw->gui_obj.window->window,
				p->width,
				p->height,
				-1);



	gdk_draw_rectangle (p->pixmap,
			    da->style->bg_gc[GTK_WIDGET_STATE (da)],
			    TRUE,
			    0, 0,
			    p->width,
			    p->height);

	gdk_draw_rectangle (p->pixmap,
			    da->style->white_gc,
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
	gdk_draw_arc(p->pixmap,da->style->bg_gc[GTK_WIDGET_STATE (da)],TRUE,p->width/2-FOORADIUS,CASEOFFSET-FOORADIUS,2*FOORADIUS,2*FOORADIUS,180*64,180*64);
	gdk_draw_arc(p->pixmap,p->bbw->case_gc,FALSE,p->width/2-FOORADIUS,CASEOFFSET-FOORADIUS,2*FOORADIUS,2*FOORADIUS,180*64,180*64);



/*	gdk_draw_pixmap(da->window,
			da->style->fg_gc[GTK_WIDGET_STATE (da)],
			p->pixmap,
			0, 0,
		    0, 0,
			p->width,
			p->height);**/

	gtk_signal_connect(GTK_OBJECT(da),
			   "expose_event",
			   (GtkSignalFunc) expose,
			   p);
    }

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
			   p->module->get_pin(i));

	gtk_layout_put(GTK_LAYOUT(bbw->layout),
		       pin->widget,p->x+pin->x,p->y+pin->y);


	p->pins = g_list_append(p->pins, pin);

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

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Watch_Window *ww)
{
    ((GUI_Object *)ww)->change_view((GUI_Object*)ww,VIEW_HIDE);
    return TRUE;
}

// calculate which pin is closest to (x,y)
static int get_pin(Breadboard_Window *bbw, int x, int y)
{
    int pin;

/*    x-=XOFFSET;
    y-=YOFFSET;

    y-=bbw->case_y;

    if(y<0)
	y=0;

    if(y>bbw->case_height-bbw->pinspacing/2)
	y=bbw->case_height-bbw->pinspacing/2;

    pin = y/bbw->pinspacing+1;

    if(x>bbw->case_x+bbw->case_width/2)
    {
	pin = bbw->nrofpins - pin + 1;
    }*/

    pin=1;

    return pin;
}

static int hit_state(Breadboard_Window *bbw, int x, int y)
{
/*    int center_offset;
    
    x-=XOFFSET;
    x-=bbw->case_x;
    x-=bbw->case_width/2;
    
    center_offset=abs(x);

    printf("offset %d\n",center_offset);

    if(center_offset<bbw->case_width/2+PINLENGTH)*/
	return FALSE;
//    return TRUE;
}

void BreadboardWindow_new_processor(Breadboard_Window *bbw, GUI_Processor *gp)
{
    char buf[128];
    int i;
    unsigned int pic_id;
    struct cross_reference_to_gui *cross_reference;
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


    for(pin=1;pin<=get_processor(pic_id)->get_pin_count();pin++)
    {
	cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
	cross_reference->parent_window_type = WT_breadboard_window;
	cross_reference->parent_window = (gpointer) bbw;
	cross_reference->data = (gpointer) NULL;
	cross_reference->update = xref_update;
	cross_reference->remove = NULL;
	gpsim_assign_pin_xref(pic_id,pin, cross_reference);
    }

    struct gui_module *p=create_module(bbw, PIC_MODULE, get_processor(pic_id),NULL);
//    draw_pins(p);

//    BreadboardWindow_update(bbw);
}

void BreadboardWindow_new_module(Breadboard_Window *bbw, Module *module)
{
    struct gui_module *p=create_module(bbw, EXTERNAL_MODULE, module, NULL);
}

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
  GtkWidget *node_add_stimuli_button;
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


/*  window=bbw->gui_obj.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

//  gtk_window_set_title(GTK_WINDOW(bbw->gui_obj.window), "Breadboard");
  da = gtk_drawing_area_new();
  gtk_widget_set_events(da,
			gtk_widget_get_events(da)|
			GDK_BUTTON_PRESS_MASK);

  gtk_container_add(GTK_CONTAINER(window),da);
  gtk_signal_connect(GTK_OBJECT(da),
		     "expose_event",
		     (GtkSignalFunc) expose,
		     bbw);
  
  bbw->da=da;
	
  width=((GUI_Object*)bbw)->width;
  height=((GUI_Object*)bbw)->height;
  x=((GUI_Object*)bbw)->x;
  y=((GUI_Object*)bbw)->y;
  gtk_window_set_default_size(GTK_WINDOW(bbw->gui_obj.window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(bbw->gui_obj.window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(bbw->gui_obj.window),bbw->gui_obj.name,"Gpsim");
  
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed), &window);
  gtk_signal_connect (GTK_OBJECT (bbw->gui_obj.window), "delete_event",
			    GTK_SIGNAL_FUNC(delete_event), (gpointer)bbw);
  gtk_signal_connect_after(GTK_OBJECT(bbw->gui_obj.window), "configure_event",
			   GTK_SIGNAL_FUNC(gui_object_configure_event),bbw);
  gtk_signal_connect(GTK_OBJECT(bbw->da),
		     "button_press_event",
		     (GtkSignalFunc) button,
		     bbw);
  gtk_signal_connect (GTK_OBJECT(bbw->da),"configure_event",
		      (GtkSignalFunc) configure_event, bbw);

  gtk_widget_show(da);
  gtk_widget_show(window);
*/




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

  pic_settings_clist = gtk_list_new ();
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

  bbw->node_frame = gtk_frame_new ("Node settings");
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

  node_settings_clist = gtk_list_new ();
  gtk_widget_ref (node_settings_clist);
  gtk_object_set_data_full (GTK_OBJECT (window), "node_settings_clist", node_settings_clist,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (node_settings_clist);
  gtk_container_add (GTK_CONTAINER (viewport7), node_settings_clist);

  hbox10 = gtk_hbox_new (FALSE, 0);
  gtk_widget_ref (hbox10);
  gtk_object_set_data_full (GTK_OBJECT (window), "hbox10", hbox10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbox10);
  gtk_box_pack_start (GTK_BOX (vbox11), hbox10, FALSE, FALSE, 0);

  node_add_stimuli_button = gtk_button_new_with_label ("Add stimuli to node ...");
  gtk_widget_ref (node_add_stimuli_button);
  gtk_object_set_data_full (GTK_OBJECT (window), "node_add_stimuli_button", node_add_stimuli_button,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (node_add_stimuli_button);
  gtk_box_pack_start (GTK_BOX (hbox10), node_add_stimuli_button, FALSE, FALSE, 0);

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

  module_settings_clist = gtk_list_new ();
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
  
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed), &window);
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

    gp_add_window_to_list(gp, (GUI_Object *)bbw);

    
    if(!gui_object_get_config((GUI_Object*)bbw))
      printf("warning: %s\n",__FUNCTION__);
    
    if(bbw->gui_obj.enabled)
	BuildBreadboardWindow(bbw);
  

    return 0;
}


#endif // HAVE_GUI
