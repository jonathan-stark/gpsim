
#include <gui.h>

#ifdef HAVE_GUI

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

#include <assert.h>

#define PINLINEWIDTH 4
#define CASELINEWIDTH 6

#define FOORADIUS 10 // also radius of casing-center-top-round-marker-milling

#define LABELPAD 10 // increase this so wide lines doesn't clutter labels

static enum _pintype {PIN_OUTPUT, PIN_INPUT, PIN_OTHER};

static void draw_pin(GdkDrawable  *drawable,
	      GdkGC *gc,
	      enum _pintype type,
	      Breadboard_Window *bbw,
	      int casex, int endx,
	      int y)
{
    int pointx;
    int wingheight, wingx;
    
    // Draw actual pin
    gdk_draw_line(drawable,gc,
		  casex,y,endx,y);

    if(type==PIN_OTHER)
	return;
    
    wingheight=bbw->pinlength/3;
    
    if(casex>endx)
    {
	if(type==PIN_OUTPUT)
	{
	    pointx = endx + (bbw->pinlength*2)/3;
	    wingx=endx+bbw->pinlength/3;
	}
	else
	{
	    pointx = endx + bbw->pinlength/3;
	    wingx=endx+(bbw->pinlength*2)/3;
	}
    }
    else
    {
	if(type==PIN_OUTPUT)
	{
	    pointx = casex + bbw->pinlength/3;
	    wingx=casex+(bbw->pinlength*2)/3;
	}
	else
	{
	    pointx = casex + (bbw->pinlength*2)/3;
	    wingx=casex+bbw->pinlength/3;
	}
    }

    
    // Draw an arrow poining at (endx,endy)
    gdk_draw_line(drawable,gc,
		  pointx,y,wingx,y+wingheight);
    gdk_draw_line(drawable,gc,
		  pointx,y,wingx,y-wingheight);
}

#define XOFFSET 20
#define YOFFSET 20

void BreadboardWindow_update(Breadboard_Window *bbw)
{
    int dy;
    GtkWidget *widget = bbw->da;
    unsigned int pic_id;
    int x,y;
    char str[200];

    pic_id = ((GUI_Object*)bbw)->gp->pic_id;

    if(bbw->pixmap==NULL)
	return;
    
    gdk_draw_rectangle (bbw->pixmap,
			widget->style->white_gc,
			TRUE,
			0, 0,
			widget->allocation.width,
			widget->allocation.height);

    if(bbw->pinline_gc==NULL)
	return;
    
    for(dy=0;dy<bbw->pinspacing*(bbw->nrofpins/2);dy+=bbw->pinspacing)
    {
	int pinnr;
	char *name;

	// Draw pin to the left
	pinnr=dy/bbw->pinspacing+1;
	name=gpsim_pin_get_name(pic_id,pinnr);
	if(name)
	{
	    int state=gpsim_pin_get_value(pic_id,pinnr);
	    int type=gpsim_pin_get_dir(pic_id, pinnr);
	    // Draw pin name
	    gdk_draw_text(bbw->pixmap,bbw->pinnamefont,bbw->pinname_gc,LABELPAD/2+XOFFSET+bbw->pinstatewidth+bbw->pinlength,LABELPAD/2+YOFFSET+dy+bbw->pinspacing/2,name,strlen(name));
	    // Draw pin
	    gdk_gc_set_foreground(bbw->pinline_gc,state>0?&high_output_color:&low_output_color);
	    draw_pin(bbw->pixmap,bbw->pinline_gc,type==0?PIN_INPUT:PIN_OUTPUT,
                     bbw,
		     XOFFSET+bbw->pinstatewidth+bbw->pinlength,
		     XOFFSET+bbw->pinstatewidth,
		     YOFFSET+dy+bbw->pinspacing/2);
	    // Draw lower pin state
	    sprintf(str,"%c",state>0?'H':'L');
	    name=str;
	    gdk_draw_text(bbw->pixmap,bbw->pinnamefont,bbw->pinname_gc,LABELPAD/2+XOFFSET,LABELPAD/2+YOFFSET+dy+bbw->pinspacing/2,name,strlen(name));
	}
	else
	{
	    g_assert(bbw->pinline_gc!=NULL);
	    gdk_gc_set_foreground(bbw->pinline_gc,&black_color);
	    draw_pin(bbw->pixmap,bbw->pinline_gc,PIN_OTHER,
                     bbw,
		     XOFFSET+bbw->pinstatewidth,
		     XOFFSET+bbw->pinstatewidth+bbw->pinlength,
		     YOFFSET+dy+bbw->pinspacing/2);
	}

        // Draw pin to the right
	pinnr=bbw->nrofpins-dy/bbw->pinspacing;
	name=gpsim_pin_get_name(pic_id,pinnr);
	if(name)
	{
	    int state=gpsim_pin_get_value(pic_id,pinnr);
	    int type=gpsim_pin_get_dir(pic_id, pinnr);
	    // Draw pin name
	    gdk_draw_text(bbw->pixmap,bbw->pinnamefont,bbw->pinname_gc,LABELPAD/2+XOFFSET+bbw->pinstatewidth+bbw->pinlength+bbw->case_width-bbw->pinnamewidth,LABELPAD/2+YOFFSET+dy+bbw->pinspacing/2,name,strlen(name));
	    // Draw pin
	    gdk_gc_set_foreground(bbw->pinline_gc,state>0?&high_output_color:&low_output_color);
	    draw_pin(bbw->pixmap,bbw->pinline_gc,type==0?PIN_INPUT:PIN_OUTPUT,
                     bbw,
		     XOFFSET+bbw->pinstatewidth+bbw->pinlength+bbw->case_width,
		     XOFFSET+bbw->pinstatewidth+bbw->pinlength+bbw->case_width+bbw->pinlength,
		     YOFFSET+dy+bbw->pinspacing/2);
	    // Draw lower pin state
	    sprintf(str,"%c",state>0?'H':'L');
	    name=str;
	    gdk_draw_text(bbw->pixmap,bbw->pinnamefont,bbw->pinname_gc,LABELPAD/2+XOFFSET+bbw->pinstatewidth+bbw->pinlength*2+bbw->case_width,LABELPAD/2+YOFFSET+dy+bbw->pinspacing/2,name,strlen(name));
	}
	else
	{
	    gdk_gc_set_foreground(bbw->pinline_gc,&black_color);
	    draw_pin(bbw->pixmap, bbw->pinline_gc,PIN_OTHER,
                     bbw,
		     XOFFSET+bbw->pinstatewidth+bbw->pinlength+bbw->case_width,
		     XOFFSET+bbw->pinstatewidth+bbw->pinlength+bbw->case_width+bbw->pinlength,
		     YOFFSET+dy+bbw->pinspacing/2);
	}

    }

    // Draw casing
    x=XOFFSET+bbw->case_x;
    y=XOFFSET+bbw->case_y;
    gdk_draw_line(bbw->pixmap,bbw->case_gc,x,y,x+bbw->case_width/2-FOORADIUS,y);
    gdk_draw_line(bbw->pixmap,bbw->case_gc,x+bbw->case_width,y,x+bbw->case_width/2+FOORADIUS,y);
    gdk_draw_line(bbw->pixmap,bbw->case_gc,x+bbw->case_width,y,x+bbw->case_width,y+bbw->case_height);
    gdk_draw_line(bbw->pixmap,bbw->case_gc,x,y+bbw->case_height,x+bbw->case_width,y+bbw->case_height);
    gdk_draw_line(bbw->pixmap,bbw->case_gc,x,y,x,y+bbw->case_height);
    gdk_draw_arc(bbw->pixmap,bbw->case_gc,0,x+bbw->case_width/2-FOORADIUS,y-FOORADIUS,2*FOORADIUS,2*FOORADIUS,180*64,180*64);

    gdk_draw_pixmap(bbw->da->window,
		    bbw->da->style->fg_gc[GTK_WIDGET_STATE (bbw->da)],
		    bbw->pixmap,
		    0, 0,
		    0, 0,
		    widget->allocation.width,
		    widget->allocation.height);
}



/* Create a new backing pixmap of the appropriate size */
static gint configure_event (GtkWidget *widget,
			     GdkEventConfigure *event,
			    Breadboard_Window *bbw)
{

    if (bbw->pixmap)
	gdk_pixmap_unref(bbw->pixmap);

    bbw->pixmap = gdk_pixmap_new(widget->window,
				 widget->allocation.width,
				 widget->allocation.height,
				 -1);
    BreadboardWindow_update(bbw);

    return TRUE;
}

static void expose(GtkWidget *widget, GdkEventExpose *event, Breadboard_Window *bbw)
{
    if(bbw->pixmap==NULL)
    {
	puts("bbw.c: no pixmap!");
	return;
    }

    gdk_draw_pixmap(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    bbw->pixmap,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
}

static void xref_update(struct cross_reference_to_gui *xref, int new_value)
{
    Breadboard_Window *bbw;

    if(xref == NULL)
    {
	printf("Warning gui_breadboard.c: xref_update: xref=%x\n",(unsigned int)xref);
	if(xref->data == NULL || xref->parent_window==NULL)
	{
	    printf("Warning gui_breadboard.c: xref_update: xref->data=%x, xref->parent_window=%x\n",(unsigned int)xref->data,(unsigned int)xref->parent_window);
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

    x-=XOFFSET;
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
    }

    return pin;
}

static int hit_state(Breadboard_Window *bbw, int x, int y)
{
    int center_offset;
    
    x-=XOFFSET;
    x-=bbw->case_x;
    x-=bbw->case_width/2;
    
    center_offset=abs(x);

    printf("offset %d\n",center_offset);

    if(center_offset<bbw->case_width/2+bbw->pinlength)
	return FALSE;
    return TRUE;
}

static gint button(GtkWidget *widget,
		   GdkEventButton *event,
		   Breadboard_Window *bbw)
{
    assert(event&&bbw);

    if(event->type==GDK_2BUTTON_PRESS &&
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
		value = gpsim_pin_get_value(pic_id, pin);
		value = !value;
		gpsim_pin_toggle(pic_id, pin);
	    }
	    else//if(hit_direction(bbw,x,y))
	    {
		dir = gpsim_pin_get_dir(pic_id, pin);
		dir = !dir;
		gpsim_pin_set_dir(pic_id, pin, dir);
		printf("hit direction, %d\n",dir);
	    }
	}
	
	return 1;
    }
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

    sprintf(buf,"%s's pinout",gpsim_processor_get_name(pic_id));
    gtk_window_set_title(GTK_WINDOW(bbw->gui_obj.window), buf);

    
    bbw->pinlength=5*PINLINEWIDTH;

    bbw->nrofpins=gpsim_package_pin_count(pic_id);

    bbw->pinnamewidth=0;
    for(i=1;i<=bbw->nrofpins;i++)
    {
	char *name;
	int width;
	
	name=gpsim_pin_get_name(pic_id,i);
	if(name==NULL)
	    continue;
	width = gdk_string_width (bbw->pinnamefont,name)+LABELPAD;
	if(width>bbw->pinnamewidth)
	    bbw->pinnamewidth=width;
    }
    
  bbw->pinstatewidth = gdk_string_width (bbw->pinstatefont,"H")+LABELPAD;
  bbw->pinstateheight = gdk_string_height (bbw->pinstatefont,"H")+LABELPAD;

  bbw->picnamewidth = gdk_string_width (bbw->picnamefont,gpsim_processor_get_name(pic_id))+LABELPAD;
  bbw->picnameheight = gdk_string_height (bbw->pinnamefont,"9y")+LABELPAD;

    
    bbw->pinspacing=bbw->pinlength;
    
//    bbw->pinspacing=(bbw->pinspacing*5)/4;

    // pin spacing is set

    //printf("pinspacing %d\n",bbw->pinspacing);
	bbw->height=(bbw->nrofpins/2)*bbw->pinspacing;
	bbw->width=bbw->pinstatewidth*2+bbw->pinlength*2+bbw->pinnamewidth*2+2*FOORADIUS;

	bbw->case_x = bbw->pinstatewidth+bbw->pinlength;
	bbw->case_y = 0;
	bbw->case_width = 2*bbw->pinnamewidth+2*FOORADIUS;
	bbw->case_height = bbw->height;

    // Center picname on case
/*    bbw->picname_x = bbw->case_height/6 + (bbw->case_width-bbw->case_height/6-bbw->picnamewidth)/2;
    bbw->picname_y = bbw->pinstateheight+bbw->pinlength+bbw->pinnameheight+bbw->case_height/2-bbw->picnameheight/3;
    gtk_widget_set_usize(bbw->da,bbw->width+40,bbw->height+40);
    */

    for(pin=1;pin<=bbw->nrofpins;pin++)
    {
	cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
	cross_reference->parent_window_type = WT_breadboard_window;
	cross_reference->parent_window = (gpointer) bbw;
	cross_reference->data = (gpointer) NULL;
	cross_reference->update = xref_update;
	cross_reference->remove = NULL;
	gpsim_assign_pin_xref(pic_id,pin, cross_reference);
    }
    
    BreadboardWindow_update(bbw);
}

int BuildBreadboardWindow(Breadboard_Window *bbw)
{
    GtkWidget *window, *da;
    
  int x,y,width,height;

  
  window=bbw->gui_obj.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

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

  bbw->pinnamefont = gdk_font_load ("-adobe-courier-bold-r-*-*-*-140-*-*-*-*-*-*");
  bbw->pinnameheight = gdk_string_height (bbw->pinnamefont,"9y")+LABELPAD;

	bbw->pinline_gc=gdk_gc_new(bbw->da->window);
	g_assert(bbw->pinline_gc!=NULL);
	gdk_gc_set_line_attributes(bbw->pinline_gc,PINLINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);

	bbw->pinname_gc=gdk_gc_new(bbw->da->window);

	bbw->case_gc=gdk_gc_new(bbw->da->window);
	gdk_gc_set_line_attributes(bbw->case_gc,CASELINEWIDTH,GDK_LINE_SOLID,GDK_CAP_ROUND,GDK_JOIN_ROUND);

	bbw->pinstatefont = gdk_font_load ("-adobe-courier-bold-r-*-*-*-340-*-*-*-*-*-*");

	bbw->picnamefont = gdk_font_load ("-adobe-courier-bold-o-*-*-*-340-*-*-*-*-*-*");

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
    bbw->da = NULL;
    bbw->pixmap = NULL;
    bbw->pinstatefont = NULL;
    bbw->picnamefont = NULL;
    bbw->pinline_gc = NULL;
    bbw->pinname_gc = NULL;
    bbw->case_gc = NULL;
    bbw->gui_obj.enabled = 0;

    gp_add_window_to_list(gp, (GUI_Object *)bbw);

    
    if(!gui_object_get_config((GUI_Object*)bbw))
      printf("warning: %s\n",__FUNCTION__);
    
    if(bbw->gui_obj.enabled)
	BuildBreadboardWindow(bbw);
  

    return 0;
}


#endif // HAVE_GUI
