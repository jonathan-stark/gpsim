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

#ifndef __GUI_SRC_H__
#define __GUI_SRC_H__


// forward references
class SourceBrowserAsm_Window;     
class SourceBrowserParent_Window;
class StatusBar_Window;

class SourceBrowser_Window : public GUI_Object {
 public:

  GtkWidget *vbox;               // for children to put widgets in

  ProgramMemoryAccess *pma;      // pointer to the processor's pma.
  StatusBar_Window *status_bar;  // display's PC, status, etc.

  void set_pma(ProgramMemoryAccess *new_pma);

  void Create(void);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void SelectAddress(int address);
  virtual void Update(void);
  virtual void UpdateLine(int address);
  virtual void SetPC(int address);
  virtual void CloseSource(void){};
  virtual void NewSource(GUI_Processor *gp){};

};



class breakpoint_info {
 public:
  int address;
  GtkWidget *widget;
};

class BreakPointList {
 public:

  GList *iter;

  BreakPointList(void);
  void Remove(int);
  void Add(int, GtkWidget *,GtkWidget *,int);
};

// the prefix 'sa' doesn't make sense anymore, FIXME.
struct sa_entry {      // entry in the sa_xlate_list
  unsigned int index;           // gtktext index to start of line
  unsigned int line;            // line number, first line eq. 0
  unsigned int pixel;           // pixels from top of text
  unsigned int font_center;     // from base line
  GtkWidget *bpwidget; // breakpoint widget on this line.
  
};

class PixmapObject {
public:
  PixmapObject(void)
   { 
     mask = 0;
     pixmap = 0;
     widget = 0;
   }

  void CreateFromXPM(GdkWindow *window,
		      GdkColor *transparent_color,
		      gchar **data);

  GdkBitmap *mask;
  GdkPixmap *pixmap;
  GtkWidget *widget;
};

//
// Source page
//

class SourcePage
{
 public:


  GtkAdjustment *source_layout_adj;
  GtkWidget *source_layout;
  GtkWidget *source_text;
  unsigned int pageindex_to_fileid;
  GtkWidget *source_pcwidget;
  GtkWidget *notebook;   // parent
  GtkWidget *notebook_child;

  SourcePage(void) {
    source_layout_adj = 0;
    source_layout = 0;
    source_text = 0;
    pageindex_to_fileid = INVALID_VALUE;
    source_pcwidget = 0;
    notebook_child = 0;
    notebook = 0;
  }
    
  void Close(void);
};

//
// The Source Assembler Browser 
//
class SourceBrowserAsm_Window :public  SourceBrowser_Window
{
 public:


  BreakPointList breakpoints;
  BreakPointList notify_start_list;
  BreakPointList notify_stop_list;

  // Where the source is stored:

  SourcePage pages[SBAW_NRFILES];

  int layout_offset;


  // Font strings
  char commentfont_string[256];
  char sourcefont_string[256];

  GtkWidget *popup_menu;

  struct sa_entry *menu_data;  // used by menu callbacks
    
  GdkBitmap *pc_mask;
  GdkBitmap *bp_mask;
  GdkBitmap *canbp_mask;
  GdkBitmap *startp_mask;
  GdkBitmap *stopp_mask;
  GtkWidget *notebook;

  GtkStyle *symbol_text_style;       // for symbols in .asm display
  GtkStyle *label_text_style;        // for label in .asm display
  GtkStyle *instruction_text_style;  // for instruction in .asm display
  GtkStyle *number_text_style;       // for numbers in .asm display
  GtkStyle *comment_text_style;      // for comments in .asm display
  GtkStyle *default_text_style;      // the rest
    
  GdkPixmap *pixmap_pc;
  GdkPixmap *pixmap_break;
  GdkPixmap *pixmap_canbreak;
  GdkPixmap *pixmap_profile_start;
  GdkPixmap *pixmap_profile_stop;

  PixmapObject canbreak;

  int source_loaded;

  int load_source;

  SourceBrowserParent_Window *parent;

  SourceBrowserAsm_Window(GUI_Processor *gp,char* new_name);
  virtual void Build(void);
  virtual void SelectAddress(int address);
  virtual void SetPC(int address);
  virtual void CloseSource(void);
  virtual void NewSource(GUI_Processor *gp);
  virtual void Update(void);
  virtual void UpdateLine(int address);
  virtual void SetText(int id, int file_id);

};

//
// The Source Opcode Browser Data
//
class SourceBrowserOpcode_Window : public SourceBrowser_Window
{
 public:

  GtkWidget *clist;

  unsigned int current_address;   // current PC

  // Font strings
  char normalfont_string[256];
  char breakpointfont_string[256];
  char pcfont_string[256];
  GtkStyle *normal_style;
  GtkStyle *current_line_number_style;
  GtkStyle *breakpoint_line_number_style;
  GdkColor pm_has_changed_color;
  GdkColor normal_pm_bg_color;
  GdkColor breakpoint_color;

  char **column_titles; //
  int  columns;         //

  int program;
    
  GtkWidget *notebook;
  GtkWidget *sheet;
  GtkWidget *entry;
  GtkWidget *label;
  //    GtkWidget *pcwidget;
  GtkWidget *sheet_popup_menu;
  GtkWidget *clist_popup_menu;

  unsigned int ascii_mode; // 0, 1 or 2 equals
  // one byte/cell,
  // two bytes/cell MSB first
  // two bytes/cell LSB first
    
  unsigned int *memory;

  SourceBrowserOpcode_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void SelectAddress(int address);
  virtual void SetPC(int address);
  virtual void NewSource(GUI_Processor *gp);
  virtual void UpdateLine(int address);

};


//
// The Source Browser Child window.
// 
// The gui supports "context" debugging, where a context
// may be code written for interrupt routines, non-interrupt
// routines, high versus low interrupt priorities, etc. Each
// context has a dedicated source browser. The SourceBrowserChild_Window
// class manages this.

class SourceBrowserParent_Window : public GUI_Object
{
 public:
  list<SourceBrowserAsm_Window *> children;

  SourceBrowserParent_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void SelectAddress(int address);
  virtual void Update(void);
  virtual void UpdateLine(int address);
  virtual void SetPC(int address);
  virtual void CloseSource(void);
  virtual void NewSource(GUI_Processor *gp);
  virtual void ChangeView(int view_state);
  virtual int set_config();

};




#endif // __GUI_SRC_H__

