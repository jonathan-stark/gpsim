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

#define PINLENGTH (4*PINLINEWIDTH)

static int pinspacing = PINLENGTH;

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

struct gui_pin *create_iopin(Breadboard_Window *bbw, int x, int y, orientation orientation, IOPIN *iopin)
{
    struct gui_pin *gs;

    gs=(struct gui_pin *)malloc(sizeof(*gs));

    gs->iopin = iopin;
    gs->x=x;
    gs->y=y;
    gs->width=pinspacing;
    gs->height=pinspacing;
    gs->bbw=bbw;
    if(iopin!=NULL)
    {
	gs->value=iopin->get_state();
	gs->direction=iopin->get_direction()==0?PIN_INPUT:PIN_OUTPUT;
	gs->orientation=orientation;
        gs->type=PIN_DIGITAL;
    }
    else
    {
	gs->value=0;
	gs->direction=PIN_INPUT;
        gs->orientation=orientation;
	gs->type=PIN_OTHER;
    }

    if(gs->orientation==LEFT)
        gs->x-=gs->width;

    // Create widget
    gs->widget = gtk_drawing_area_new();
//	gtk_widget_set_events(gs->widget,
//			      gtk_widget_get_events(gs->widget)|
//			      GDK_BUTTON_PRESS_MASK);

    gtk_drawing_area_size(GTK_DRAWING_AREA(gs->widget),gs->width,gs->height);
    gtk_signal_connect(GTK_OBJECT(gs->widget),
		       "expose_event",
		       (GtkSignalFunc) expose_pin,
		       gs);



    // Create pixmap
    gs->pixmap = gdk_pixmap_new(bbw->gui_obj.window->window,
				gs->width,
				gs->height,
				-1);


    // Draw pin
    gs->gc=gdk_gc_new(bbw->gui_obj.window->window);
    g_assert(gs->gc!=NULL);
    gdk_gc_set_line_attributes(gs->gc,PINLINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);
    draw_pin(gs);
//    gdk_draw_line(gs->pixmap,gs->gc,0,0,3,gs->height);

    gtk_widget_show(gs->widget);

    return gs;
}

struct package
{
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

static void expose(GtkWidget *widget, GdkEventExpose *event, struct package *p)
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


struct package *create_package(Breadboard_Window *bbw, class Module *module, GtkWidget *widget)
{
    struct package *p;
    int i;

    p = (struct package*) malloc(sizeof(*p));

    p->bbw=bbw;
    p->module=module;
    p->x=50;
    p->y=50;
    p->module_widget = widget;

    p->pins=NULL;

    if(p->module_widget==NULL)
    {
	// Create a static representation.
	int pin_x, pin_y;
        int x,y;
	int pic_id;
	GtkWidget *da;
	
	pic_id = ((GUI_Object*)bbw)->gp->pic_id;

	p->pinnamewidth=0;
	for(i=1;i<=p->module->get_pin_count();i++)
	{
	    char *name;
	    int width;

	    name=gpsim_pin_get_name(pic_id,i);
	    if(name==NULL)
		continue;
	    width = gdk_string_width (bbw->pinnamefont,name)+LABELPAD;
	    if(width>p->pinnamewidth)
		p->pinnamewidth=width;
	}

        p->pinnamewidth+=FOORADIUS; // The 'U' at the top of the DIL package

	p->width=p->pinnamewidth*2; // pin name widthts
	p->width+=2*CASELINEWIDTH+2*LABELPAD;

	p->height=module->get_pin_count()/2*pinspacing; // pin name height
        p->height+=2*CASELINEWIDTH+2*LABELPAD;



	da = gtk_drawing_area_new();
	gtk_widget_set_events(da,
			      gtk_widget_get_events(da)|
			      GDK_BUTTON_PRESS_MASK);

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

	    if(i<=p->module->get_pin_count()/2)
	    {
		label_x=LABELPAD+CASELINEWIDTH;
		label_y=LABELPAD+CASELINEWIDTH-bbw->pinnameheight/3+(i-1)*pinspacing+pinspacing/2;
	    }
	    else
	    {
		label_x=LABELPAD+p->width/2+FOORADIUS;
		label_y=LABELPAD+CASELINEWIDTH-bbw->pinnameheight/3+(p->module->get_pin_count()-i)*pinspacing+pinspacing/2;
	    }

	    name=gpsim_pin_get_name(pic_id,i);
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
	/*  gtk_signal_connect(GTK_OBJECT(bbw->da),
	 "button_press_event",
	 (GtkSignalFunc) button,
	 bbw);
	 */
    }

    // Create pins
    for(i=1;i<=p->module->get_pin_count();i++)
    {
	int pin_x, pin_y;
	struct gui_pin *pin;
        enum orientation orientation;

	if(i<=p->module->get_pin_count()/2)
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

    }










    
//  bbw->pinstatewidth = gdk_string_width (bbw->pinstatefont,"H")+LABELPAD;
//  bbw->pinstateheight = gdk_string_height (bbw->pinstatefont,"H")+LABELPAD;

//  bbw->pinnamewidth = gdk_string_width (bbw->pinnamefont,gpsim_processor_get_name(pic_id))+LABELPAD;
//  bbw->pinnameheight = gdk_string_height (bbw->pinnamefont,"9y")+LABELPAD;

    
  /*      bbw->case_x = bbw->pinstatewidth+PINLENGTH;
	bbw->case_y = 0;
	bbw->case_width = 2*bbw->pinnamewidth+2*FOORADIUS;
	bbw->case_height = bbw->height;
        */

    bbw->packages=g_list_append(bbw->packages, p);
}


void BreadboardWindow_update(Breadboard_Window *bbw)
{
    GList *iter;


    // loop all packages and update their pins

    iter=bbw->packages;
    while(iter!=NULL)
    {
	GList *pin_iter;
	struct package *p;

        p = (struct package*)iter->data;

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

static gint button(GtkWidget *widget,
		   GdkEventButton *event,
		   Breadboard_Window *bbw)
{
    assert(event&&bbw);

    if(!bbw->processor)
        return 0;

/*    if(event->type==GDK_2BUTTON_PRESS &&
       event->button==1)
    {
	int x,y;
	int pin;
	int value, dir;
	int pic_id;
	
	pic_id = ((GUI_Object*)bbw)->gp->pic_id;

	x=event->x;
	y=event->y;

	pin = get_pin(bbw,x,y);
	if(pin>0)
	{
	    printf("\nPin nr %d, ",pin);
	    if(hit_state(bbw,x,y))
	    {
		printf("hit state\n");
		value = p->get_value(p, pin);
		value = !value;
		gpsim_pin_toggle(pic_id, pin);
	    }
	    else//if(hit_direction(bbw,x,y))
	    {
		dir = p->get_dir(p, pin);
		dir = !dir;
		gpsim_pin_set_dir(pic_id, pin, dir);
		printf("hit direction, %d\n",dir);
	    }
	}
	
	return 1;
    }*/
    return 0;
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


    struct package *p=create_package(bbw, get_processor(pic_id),NULL);
//    draw_pins(p);

//    BreadboardWindow_update(bbw);
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
  GtkWidget *pic_frame;
  GtkWidget *vbox12;
  GtkWidget *scrolledwindow3;
  GtkWidget *viewport8;
  GtkWidget *pic_settings_clist;
  GtkWidget *hbox11;
  GtkWidget *pic_settings_entry;
  GtkWidget *pic_settings_button;
  GtkWidget *node_frame;
  GtkWidget *vbox11;
  GtkWidget *scrolledwindow2;
  GtkWidget *viewport7;
  GtkWidget *node_settings_clist;
  GtkWidget *hbox10;
  GtkWidget *node_add_stimuli_button;
  GtkWidget *module_frame;
  GtkWidget *vbox10;
  GtkWidget *scrolledwindow1;
  GtkWidget *viewport6;
  GtkWidget *module_settings_clist;
  GtkWidget *hbox9;
  GtkWidget *module_settings_entry;
  GtkWidget *module_settings_button;
  GtkWidget *save_stc_button;
  GtkWidget *scrolledwindow5;

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
  gtk_window_set_title (GTK_WINDOW (window), "Breadboard");
  gtk_window_set_default_size (GTK_WINDOW (window), 500, 400);

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

  tree1 = gtk_tree_new ();
  gtk_widget_ref (tree1);
  gtk_object_set_data_full (GTK_OBJECT (window), "tree1", tree1,
                            (GtkDestroyNotify) gtk_widget_unref);
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

  pic_frame = gtk_frame_new ("PIC settings");
  gtk_widget_ref (pic_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "pic_frame", pic_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pic_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), pic_frame, TRUE, TRUE, 0);

  vbox12 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox12);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox12", vbox12,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox12);
  gtk_container_add (GTK_CONTAINER (pic_frame), vbox12);

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
  gtk_box_pack_start (GTK_BOX (vbox12), hbox11, TRUE, TRUE, 0);

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

  node_frame = gtk_frame_new ("Node settings");
  gtk_widget_ref (node_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "node_frame", node_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (node_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), node_frame, TRUE, TRUE, 0);

  vbox11 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox11);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox11", vbox11,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox11);
  gtk_container_add (GTK_CONTAINER (node_frame), vbox11);

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

  module_frame = gtk_frame_new ("Module settings");
  gtk_widget_ref (module_frame);
  gtk_object_set_data_full (GTK_OBJECT (window), "module_frame", module_frame,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (module_frame);
  gtk_box_pack_start (GTK_BOX (vbox9), module_frame, TRUE, TRUE, 0);

  vbox10 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox10);
  gtk_object_set_data_full (GTK_OBJECT (window), "vbox10", vbox10,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox10);
  gtk_container_add (GTK_CONTAINER (module_frame), vbox10);

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
  gtk_box_pack_start (GTK_BOX (vbox10), hbox9, TRUE, TRUE, 0);

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
  gtk_layout_set_size (GTK_LAYOUT (bbw->layout), 500, 500);
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



  bbw->pinnameheight = gdk_string_height (bbw->pinnamefont,"9y");

  if(pinspacing<bbw->pinnameheight)
      pinspacing=bbw->pinnameheight+2;

  gtk_widget_show_now(window);

	bbw->pinname_gc=gdk_gc_new(bbw->gui_obj.window->window);

	bbw->case_gc=gdk_gc_new(bbw->gui_obj.window->window);
	gdk_gc_set_line_attributes(bbw->case_gc,CASELINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);

	bbw->pinstatefont = gdk_fontset_load ("-adobe-courier-bold-r-*-*-*-80-*-*-*-*-*-*");

	bbw->pinnamefont = gdk_fontset_load ("-adobe-courier-bold-r-*-*-*-80-*-*-*-*-*-*");

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

    bbw->packages=NULL;

    gp_add_window_to_list(gp, (GUI_Object *)bbw);

    
    if(!gui_object_get_config((GUI_Object*)bbw))
      printf("warning: %s\n",__FUNCTION__);
    
    if(bbw->gui_obj.enabled)
	BuildBreadboardWindow(bbw);
  

    return 0;
}


#endif // HAVE_GUI
