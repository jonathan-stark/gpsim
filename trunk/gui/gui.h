#ifndef __GUI_H__
#define __GUI_H__

#include <gtk/gtk.h>
#include <gtkextra/gtksheet.h>
#include "../src/interface.h"

#define SBAW_NRFILES 8 // Max number of source files
#define MAX_BREAKPOINTS 32

//------------------------------------------------------------
//
// Create structures to generically access the pic-processor
//

//class pic_processor;

//
// Here's a list of all the types of windows that are supported
//

enum window_types {
  WT_register_window,
  WT_status_bar,
  WT_sfr_window,
  WT_watch_window,
  WT_stack_window,
  WT_symbol_window,
  WT_asm_source_window,
  WT_opcode_source_window,
  WT_list_source_window,
  WT_breadboard_window
};

//
// Here's a list of all the categories of windows that are supported
//
enum window_category {
  WC_misc,
  WC_source,
  WC_data
};

//
// This structure will cross reference the data in the simulator
// to its gui representation. There are cases when the same data
// appears in more than one place (e.g. the status register is in
// both the status bar and register windows). gpsim accomodates this
// with a singly-linked list. In other words, for each data element
// that is presented graphically there's a pointer within the simulator
// to reference it. The simulator keeps a linked listed of pointers
// to all instances of these graphical representations

struct cross_reference_to_gui {
  enum window_types parent_window_type;
  gpointer     parent_window;
  gpointer     data;
  void         (*update)  
       (struct cross_reference_to_gui  *_this,int new_value);
  void         (*remove)  
       (struct cross_reference_to_gui  *_this);
};


//
// Forward reference
//

struct _gui_processor;


//
// Make the declarations pretty
//

typedef struct _gui_processor GUI_Processor;

//
// GUI_Object 
//  All window attributes that are common are placed into the GUI_Object
// structure. This structure is then include in each of the other structures.
// It's also the very first item in these 'derived' structures. Consequently a
// pointer to one object may be type cast into another. 
//

struct _gui_object {
  GUI_Processor *gp;
  GtkWidget *window;
  enum window_category wc;
  enum window_types wt;

  char *name;

  // Window geometry. This info is saved when the window associated
  // with this gui object is hidden. Note: gtk saves the window origin
  // (x,y) but doesn't save the size (width,height).
  int x,y,width,height;
  int enabled;

  // A pointer to a function that will allow the window associated
  // with this gui object to be viewable or hidden.

#define VIEW_HIDE 0
#define VIEW_SHOW 1
#define VIEW_TOGGLE 2
  void (* change_view) (struct _gui_object *_this, int view_state);
};

typedef struct _gui_object GUI_Object;

//
// A 'register' has two attributes as far as the gui is concerned:
//   1) its location and 2) value that is being displayed
//

struct _register {
  int row;        // row & col in register window
  int col;
  int value;      // value displayed in register window
  gboolean update_full;
};

typedef struct _register Register;

//
// A 'labeled entry' is an object consisting of gtk entry
// widget that is labeled (with a gtk lable widget) and 
// has information about its parent.
//

struct _labeled_entry {
  GtkWidget *label;
  GtkWidget *entry;

  union {
    gint32    i32;
    guint64   ui64;
  } value;           // value displayed

  gpointer parent;   // a pointer to the owner
  int handle_id;     // unique identifier
};

typedef struct _labeled_entry labeled_entry;


//
// The register window 
//
#define MAX_REGISTERS      4096
#define REGISTERS_PER_ROW    16
#define MAX_ROWS ((MAX_REGISTERS)/(REGISTERS_PER_ROW))

struct _Register_Window {
    GUI_Object     gui_obj;

    // This array is indexed with row, and gives the address of the
    // first cell in the given row.
    int row_to_address[MAX_ROWS];
    
    REGISTER_TYPE type;
    Register **registers;
    GtkSheet *register_sheet;
    
    GtkWidget *entry;
    GtkWidget *location;
    GtkWidget *popup_menu;

    int allow_change_view;

    int registers_loaded; // non zero when registers array is loaded

    int processor; // if non-zero window has processor
};

typedef struct _Register_Window Register_Window;


//
// The watch window
//
struct _Watch_Window {
    GUI_Object     gui_obj;

    GList *watches;

    int current_row;
    int current_column;
    
    GtkWidget *watch_clist;
    GtkWidget *popup_menu;
};

typedef struct _Watch_Window Watch_Window;

//
// The symbol window
//
struct _Symbol_Window {
    GUI_Object     gui_obj;

//    GtkWidget *symbol_window;
    GtkWidget *symbol_clist;
    GList *symbols;
    
    int filter_addresses;
    int filter_constants;
    int filter_registers;

    int load_symbols;
};

typedef struct _Symbol_Window Symbol_Window;


//
// The Status Bar window 
//

struct _StatusBar_Window {
  GUI_Processor *gp;

  labeled_entry *status;
  labeled_entry *W;
  labeled_entry *pc;
  labeled_entry *cycles;
  
  int created;


};

typedef struct _StatusBar_Window StatusBar_Window;

struct _SourceBrowser_Window {
    GUI_Object gui_obj;
    GtkWidget *vbox; /* for children to put widgets in */
};

typedef struct _SourceBrowser_Window SourceBrowser_Window;


struct breakpoint_info {
    int address;
    GtkWidget *widget;
};

// the prefix 'sa' doesn't make sense anymore, FIXME.
struct sa_entry{         // entry in the sa_xlate_list
    int index;           // gtktext index to start of line
    int line;            // line number, first line eq. 0
    int pixel;           // pixels from top of text
    int font_center;     // from base line
};
//
// The Source Assembler Browser Data
//
struct _SourceBrowserAsm_Window {
    SourceBrowser_Window sbw;

    struct breakpoint_info breakpoint[MAX_BREAKPOINTS];
    int layout_offset;

    // We need one of theese for each source file
    GtkAdjustment *source_layout_adj[SBAW_NRFILES];
    GtkWidget *source_layout[SBAW_NRFILES];
    GtkWidget *source_text[SBAW_NRFILES];
    int pageindex_to_fileid[SBAW_NRFILES];
    GtkWidget *source_pcwidget[SBAW_NRFILES];
    GtkWidget *notebook_child[SBAW_NRFILES];
    
    GtkWidget *popup_menu;

    struct sa_entry *menu_data;  // used by men callbacks
    
    GdkBitmap *mask;
    GtkWidget *notebook;

    GtkStyle symbol_text_style;       // for symbols in .asm display
    GtkStyle label_text_style;        // for label in .asm display
    GtkStyle instruction_text_style;  // for instruction in .asm display
    GtkStyle number_text_style;       // for numbers in .asm display
    GtkStyle comment_text_style;      // for comments in .asm display
    GtkStyle default_text_style;      // the rest
    
    GdkPixmap *pixmap_pc;
    GdkPixmap *pixmap_break;
    int source_loaded;

    int load_source;
};

typedef struct _SourceBrowserAsm_Window SourceBrowserAsm_Window;


//
// The Source Opcode Browser Data
//
struct _SourceBrowserOpcode_Window {
    SourceBrowser_Window sbw;

    GtkWidget *clist;
//    int clist_rows;      // Number of rows in the clist
    int current_address;   // current PC

    char **column_titles; //
    int  columns;         //

    int processor;
    int program;
    
    GtkWidget *notebook;
    GtkWidget *sheet;
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *pcwidget;


    int *memory;
};

typedef struct _SourceBrowserOpcode_Window SourceBrowserOpcode_Window;

struct _Breadboard_Window {
    GUI_Object gui_obj;

    GtkWidget *da;
    
    GdkPixmap *pixmap;
    
    GdkFont *pinstatefont;
    int pinstateheight;
    int pinstatewidth;

    GdkFont *pinnamefont;
    int pinnameheight;
    int pinnamewidth;

    GdkFont *picnamefont;
    int picnameheight;
    int picnamewidth;

    int pinlength;
    int pinspacing;
    int nrofpins;
    GdkGC *pinline_gc;
    GdkGC *pinname_gc;
    GdkGC *case_gc;

    int picname_x, picname_y;

    int width, height;

    int case_x, case_y;
    int case_height, case_width;

    int processor;
};

typedef struct _Breadboard_Window Breadboard_Window;


//
// Future Items that will be declared.
//
/*
struct _Stack_Window {
  GUI_Processor *gp;
  stack data...
};

struct _sfr_window {
  GUI_Processor *gp;
  sfr data ...
};

struct watch_window {
  GUI_Processor *gp;
  watch data ...
};


*/


//
//  The gui_processor structure ties the gui window(s)
// to a pic that is being simulated.
//

struct _gui_processor {
  Register_Window *regwin_ram;
  Register_Window *regwin_eeprom;
  StatusBar_Window *status_bar;
  SourceBrowser_Window *program_memory;
  SourceBrowser_Window *source_browser;
  Symbol_Window *symbol_window;
  Watch_Window *watch_window;
  Breadboard_Window *breadboard_window;
  // GtkWidget *stack_window;
  // GtkWidget *sfr_window;
  // GtkWidget *watch_window;

  // Doubly-linked lists keep track of all of the windows
  // associated with this processor. 
  GList *misc_windows;
  GList *source_windows;
  GList *data_windows;

  // The pic that's associated with the gui
  //  pic_processor *p;
  unsigned int pic_id;
};


//
// External references and function prototypes
//
extern GdkColor item_has_changed_color;
extern GdkColor normal_fg_color;
extern GdkColor normal_bg_color;
extern GdkColor breakpoint_color;
extern GdkColor alias_color;
extern GdkColor invalid_color;
extern GdkColor sfr_bg_color;
extern GdkColor high_output_color;
extern GdkColor low_output_color;
extern GdkColor black_color;

extern GtkStyle *normal_style;
extern GtkStyle *current_line_number_style;
extern GtkStyle *breakpoint_line_number_style;


void exit_gpsim(void);


// gui_symbols.c
void SymbolWindow_select_symbol_regnumber(Symbol_Window *sw, int regnumber);
void SymbolWindow_select_symbol_name(Symbol_Window *sw, char *name);
void SymbolWindow_new_symbols(Symbol_Window *sw, GUI_Processor *gp);
int CreateSymbolWindow(GUI_Processor *gp);
int BuildSymbolWindow(Symbol_Window *sw);

// gui_statusbar.c
void StatusBar_create(GtkWidget *vbox_main, StatusBar_Window *sbw);
void StatusBar_update(StatusBar_Window *sbw);
void StatusBar_new_processor(StatusBar_Window *sbw, GUI_Processor *gp);

// gui_src_opcode.c
void SourceBrowserOpcode_select_address(SourceBrowserOpcode_Window *sbow,int address);
void SourceBrowserOpcode_update_line( SourceBrowserOpcode_Window *sbow, int address, int row);
void SourceBrowserOpcode_set_pc(SourceBrowserOpcode_Window *sbow, int address);
void SourceBrowserOpcode_new_program(SourceBrowserOpcode_Window *sbow, GUI_Processor *gp);
void SourceBrowserOpcode_new_processor(SourceBrowserOpcode_Window *sbow, GUI_Processor *gp);
int CreateSourceBrowserOpcodeWindow(GUI_Processor *gp);
void BuildSourceBrowserOpcodeWindow(SourceBrowserOpcode_Window *sbow);

// gui_src_asm.c
int CreateSourceBrowserAsmWindow(GUI_Processor *gp);
void SourceBrowserAsm_new_source(SourceBrowserAsm_Window *sbaw, GUI_Processor *gp);
void SourceBrowserAsm_close_source(SourceBrowserAsm_Window *sbaw, GUI_Processor *gp);
void SourceBrowserAsm_update_line( SourceBrowserAsm_Window *sbaw, int address);
void SourceBrowserAsm_set_pc(SourceBrowserAsm_Window *sbaw, int address);
void SourceBrowserAsm_new_processor(SourceBrowserAsm_Window *sbaw, GUI_Processor *gp);
void SourceBrowserAsm_select_address( SourceBrowserAsm_Window *sbaw, int address);
void BuildSourceBrowserAsmWindow(SourceBrowserAsm_Window *sbaw);

// gui_src.c
void SourceBrowser_select_address(SourceBrowser_Window *sbw,int address);
void SourceBrowser_update_line(struct cross_reference_to_gui *xref, int new_value);
void SourceBrowser_update(SourceBrowser_Window *sbw);
void CreateSBW(SourceBrowser_Window *sbw);
void SourceBrowser_change_view (struct _gui_object *_this, int view_state);

// gui_regwin.c
int gui_get_value(char *prompt);
void RegWindow_update(Register_Window *rw);
void RegWindow_select_symbol_name(Register_Window *rw, char *name);
void RegWindow_select_symbol_regnumber(Register_Window *rw, int n);
void RegWindow_select_register(Register_Window *rw, int regnumber);
int CreateRegisterWindow(GUI_Processor *gp, REGISTER_TYPE type);
void BuildRegisterWindow(Register_Window *rw);
void RegWindow_new_processor(Register_Window *rw, GUI_Processor *gp);

// gui_processor.c
extern GUI_Processor *gp;
GUI_Processor *new_GUI_Processor(void);
void gp_add_window_to_list(GUI_Processor *gp, GUI_Object *go);

int config_get_variable(char *module, char *entry, int *value);
int config_set_variable(char *module, char *entry, int value);
int gui_object_set_config(GUI_Object *obj);
int gui_object_get_config(GUI_Object *obj);
gint gui_object_configure_event(GtkWidget *widget, GdkEventConfigure *e, GUI_Object *go);

// gui_watch.c
void WatchWindow_add(Watch_Window *ww, unsigned int pic_id, REGISTER_TYPE type, int address);
int CreateWatchWindow(GUI_Processor *gp);
int BuildWatchWindow(Watch_Window *ww);
void WatchWindow_update(Watch_Window *ww);

// gui_breadboard.c
void BreadboardWindow_new_processor(Breadboard_Window *bbw, GUI_Processor *gp);
int BuildBreadboardWindow(Breadboard_Window *bbw);
int CreateBreadboardWindow(GUI_Processor *gp);


#endif // __GUI_H__

