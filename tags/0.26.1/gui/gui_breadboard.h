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

#include "../src/packages.h"


//
// The Breadboard window data
//

class Breadboard_Window;

enum eOrientation {LEFT, UP, RIGHT, DOWN};
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


class GuiModule;
//========================================================================

//
// GuiBreadBoardObject - Base class for things that can be drawn in the
//                       breadboard window.
//

class GuiBreadBoardObject
{
public:
  GuiBreadBoardObject(Breadboard_Window *,int x, int y);
  virtual ~GuiBreadBoardObject();

  virtual void Draw()=0;
  virtual void Update()=0;
  virtual void Destroy()=0;

  
  bool IsBuilt() { return m_bIsBuilt;}
  Breadboard_Window *bbw() { return m_bbw;}
  void SetPosition(int x, int y);
  int x()      { return m_x;}
  int y()      { return m_y;}
  int width()  { return m_width; }
  int height() { return m_height; }

protected:
  Breadboard_Window *m_bbw;

  int    m_x;               // Position in layout widget
  int    m_y;               // Position in layout widget
  int    m_width;           // 
  int    m_height;          //
  bool   m_bIsBuilt;        // True after the object gets displayed.
};

//------------------------------------------------------------------------
// GuiPin
class GuiPin : public GuiBreadBoardObject
{
public:
  GuiPin(Breadboard_Window *, GuiModule *,Package *, unsigned int pin_number);
  bool getState() {return value;}
  void putState(bool bNewState) { value = bNewState;}
  void toggleState();
  void toggleDirection();

  void addXref(CrossReferenceToGUI *);
  virtual void Draw();
  virtual void Update();
  virtual void Destroy();

  void SetModulePosition(int x, int y);
  int module_x() { return m_module_x; }
  int module_y() { return m_module_y; }

  void SetLabelPosition(int x, int y);
  int label_x() { return m_label_x; }
  int label_y() { return m_label_y; }
  int DrawGUIlabel(GdkPixmap *pix_map,  int pinnameWidths[]);
  void DrawLabel(GdkPixmap *pix_map);

  void SetOrientation(eOrientation o) { orientation = o; }

  unsigned int number() { return m_pkgPinNumber; }
  Stimulus_Node *getSnode() {return getIOpin() ? getIOpin()->snode : 0;}
  IOPIN *getIOpin() {return package->get_pin(m_pkgPinNumber);}
  const char *pinName();

  GtkWidget *m_pinDrawingArea;
  GdkPixmap *pixmap;
  GdkGC *gc;

  eDirection direction;
  eOrientation orientation;
  pintype type;

protected:
  bool   value;
//  IOPIN *iopin;
  Package *package;
  CrossReferenceToGUI *xref;

  GuiModule *m_pModule;     // Module to which this pin belongs.
  int    m_module_x;        // Pin coordinates within parent module
  int    m_module_y;        //

  int    m_label_x;         // Pin Label coordinates (within parent module).
  int    m_label_y;

  int    m_pkgPinNumber;    // 
};


enum module_type {PIC_MODULE, EXTERNAL_MODULE};

//------------------------------------------------------------------------
// GuiModule
// 
// The GuiModule holds the graphics for a gpsim module that is displayed in
// the bread board window. The GuiModule serves as the link between the gui
// and the simulation engine. In other words, the GuiModule provides an 
// interface through which the simulation engine may be accessed.
//
// All GuiModules have a one-to-one with a gpsim Module. (see src/modules.h)
// The GuiModule knows how to get access to a Module's pin information.
//
class GuiModule : public GuiBreadBoardObject
{
public:
  GuiModule(Module *, Breadboard_Window *);
  void SetPosition(int x, int y);
  void GetPosition(int &x, int &y);
  double Distance(int x, int y);

  virtual void Update();
  virtual void UpdatePins();
  virtual void Build();
  virtual void Draw();
  virtual void Destroy();
  virtual void DrawCaseOutline(GtkWidget *);

  virtual void AddPin(unsigned int);
  virtual void AddPinGeometry(GuiPin *);

  void BuildReferenceDesignator();

  int pin_count() { return m_pin_count; }
  GList * pins() { return m_pins; }
  Module *module() { return m_module; }
  GtkWidget *module_widget() { return m_module_widget; }
  GtkWidget *pinLabel_widget() { return m_pinLabel_widget; }
  GtkWidget *name_widget() { return m_name_widget; }
  GtkWidget *tree_item() { return m_tree_item; }
  GdkPixmap *module_pixmap() { return m_module_pixmap; }
  GdkPixmap *name_pixmap() { return m_name_pixmap; }

protected:

  Module    *m_module;
  GtkWidget *m_module_widget;  // As returned from module. If NULL, it becomes a static GtkPixmap
  GtkWidget *m_pinLabel_widget;// A drawing area for pin labels.
  int        m_module_x;       // These coordinates are an offset from m_x and m_y defined in  
  int        m_module_y;       /* GuiBreadBoardObject. Their purpose is to allow clients to 
				* reposition a customly created module_widget */

  GtkWidget *m_name_widget;    // Name of widget, positioned above module_widget.

  int pinnameWidths[4];
  int m_pin_count;

  GdkPixmap *m_module_pixmap;
  GdkPixmap *m_name_pixmap;

  GtkWidget *m_tree_item;

  GList *m_pins;


};

class GuiDipModule : public GuiModule
{
public:
  GuiDipModule(Module *, Breadboard_Window *);
  virtual void DrawCaseOutline(GtkWidget *);
};

struct gui_node
{
    Breadboard_Window *bbw;
    Stimulus_Node *node;
    GtkWidget *tree_item;
    int selected_row;

    GList *pins;
};



class Breadboard_Window : public GUI_Object
{
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
  GList *nodes;

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

  GuiPin *selected_pin;
  struct gui_node *selected_node;
  GuiModule *selected_module;


  Breadboard_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void Update(void);
  virtual void NewModule(Module *module);
  virtual void NodeConfigurationChanged(Stimulus_Node *node);

  GtkWidget *add_button(const char *label, const char *name,
			GtkSignalFunc f, GtkWidget *box);

private:
  GuiModule *m_MainCpuModule;
};



#endif //__GUI_BREADBOARD_H__

