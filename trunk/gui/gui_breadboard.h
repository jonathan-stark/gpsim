/*
   Copyright (C) 1998,1999,2000,2001,2002,2003,2004
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

#ifndef __GUI_BREADBOARD_H__
#define __GUI_BREADBOARD_H__


//
// The Breadboard window data
//

class Breadboard_Window;

enum eOrientation {LEFT, RIGHT, UP, DOWN};
enum eDirection {PIN_INPUT, PIN_OUTPUT};
typedef enum {PIN_DIGITAL, PIN_ANALOG, PIN_OTHER} pintype;

// Routing types
typedef enum {R_NONE,R_LEFT, R_RIGHT, R_UP, R_DOWN} route_direction;
typedef struct
{
    int x;
    int y;
} point;
typedef struct _path
{
    point p;
    route_direction dir;
    struct _path *next;
} path;
// End routing types

class GuiPin
{
 public:
  Breadboard_Window *bbw;

  IOPIN *iopin;
  CrossReferenceToGUI *xref;

  GtkWidget *widget;
  GdkPixmap *pixmap;
  GdkGC *gc;

  int x;
  int y;
  int width;
  int height;

  int layout_xpos, layout_ypos;

  bool value;
  eDirection direction;
  eOrientation orientation;
  pintype type;

  GuiPin(Breadboard_Window *, int x, int y, eOrientation _or, IOPIN *);
};


enum module_type {PIC_MODULE, EXTERNAL_MODULE};

struct gui_module
{

    GtkWidget *fixed; // Main layout that contains everything about the module

    enum module_type type;
    Breadboard_Window *bbw;
    Module *module;
    GtkWidget *module_widget;  // As returned from module. If NULL, it becomes a static GtkPixmap.
    GtkWidget *name_widget;    // Name of widget, positioned above module_widget.
    int x;    // Position in layout widget
    int y;    // Position in layout widget
    int width;  // Width of module_widget
    int height; // Height of module_widget

    int pinnamewidth;

    int pin_count;

    GdkPixmap *module_pixmap;
    GdkPixmap *name_pixmap;

    GtkWidget *tree_item;

    GList *pins;
};



struct gui_node
{
    Breadboard_Window *bbw;
    Stimulus_Node *node;
    GtkWidget *tree_item;
    int selected_row;

    GList *pins;
};



class Breadboard_Window : public GUI_Object {
 public:

#if GTK_MAJOR_VERSION >= 2
    PangoFontDescription *pinstatefont;
    PangoFontDescription *pinnamefont;
#else
    GdkFont *pinstatefont;
    GdkFont *pinnamefont;
#endif
    int pinnameheight;

    GtkWidget *layout;

    GdkGC *pinname_gc;
    GdkGC *pinline_gc;
    GdkGC *case_gc;

    GList *modules;

    GtkWidget *tree;

    GtkWidget *pic_frame;
    GtkWidget *node_frame;
    GtkWidget *module_frame;
    GtkWidget *stimulus_frame;

    GtkWidget *pic_settings_clist;
    GtkWidget *attribute_clist;
    GtkWidget *attribute_entry;
    GtkWidget *attribute_button;

    GtkWidget *node_tree;

    GtkWidget *node_clist;

    GtkWidget *stimulus_settings_label;

    GtkWidget *stimulus_add_node_button;

    GdkPixmap *layout_pixmap;

    GtkAdjustment *hadj, *vadj;

    struct GuiPin *selected_pin;
    struct gui_node *selected_node;
    struct gui_module *selected_module;


  Breadboard_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void Update(void);
  virtual void NewModule(Module *module);
  virtual void NodeConfigurationChanged(Stimulus_Node *node);

  GtkWidget *add_button(const char *label, const char *name,
			GtkSignalFunc f, GtkWidget *box);
};



#endif //__GUI_BREADBOARD_H__

