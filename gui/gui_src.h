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
class Value;
class SourceBrowserParent_Window;

class SourceWindow;

enum eSourceFileType {
  eUnknownSource=0, // Unknown File type
  eAsmSource,       // Assembly source files
  eCSource,         // C source files
  eJalSource,       // JAL source files
  eListSource,      // List files
  eAsmIncSource,    // Include files for assembly
  eCIncSource,      // Include files for C
  eHexSource        // Hex files
};

//========================================================================
class ColorHolder
{
public:
  ColorHolder (const char *pcColor);


  bool set(GdkColor *pNewColor, bool saveOld);
  char *get(char *, int );
  void apply();
  bool revert();
  GdkColor *CurrentColor();
protected:
  GdkColor mCurrentColor, mSaveColor;
};
//========================================================================
// TextStyle
//
// A TextSyle is wrapper around a GtkTextTag and provides a simple way
// to change the text foreground and background colors. Also, color
// editing is supported.
class TextStyle
{
public:
  TextStyle (const char *pName,
             const char *pFGColor,
             const char *pBGColor);

  virtual ~TextStyle()
  {
  }

  GtkTextTag *tag() { return m_pTag; }

  void apply();   // Permanently apply a change that the user has made.
  void revert();  // Remove a change the user has made

  void setFG(GdkColor *pNewColor);

  virtual void doubleClickEvent(GtkTextIter *);

  ColorHolder  mFG;  // Foreground color
  ColorHolder  mBG;  // Background color

protected:
  GtkTextTag *m_pTag;
};

//========================================================================
class SourceBuffer
{
public:
  SourceBuffer(GtkTextTagTable *,FileContext *,SourceBrowserParent_Window *);
  void parseLine(const char*, int parseStyle);

  void addTagRange(TextStyle *,
                   int start_index, int end_index);
  GtkTextBuffer *getBuffer();
  SourceBrowserParent_Window *m_pParent;
  FileContext   *m_pFC;
  eSourceFileType getSrcType();
  void setSrcType(eSourceFileType);

  bool IsParsed();
  void parse();
private:
  eSourceFileType m_SourceFile_t;
  bool m_bParsed;
  GtkTextBuffer *m_buffer;
};


//========================================================================
// The SourcePageMargin holds configuration information for the left margin
// of a SourcePage. The left margin is where line numbers, addresses, opcodes,
// breakpoints, and current program counter are all shown.
class SourcePageMargin
{
public:
  SourcePageMargin();

  void enableLineNumbers(bool b)
  {
    m_bShowLineNumbers = b;
  }
  void enableAddresses(bool b)
  {
    m_bShowAddresses = b;
  }
  void enableOpcodes(bool b)
  {
    m_bShowOpcodes = b;
  }

  bool formatMargin(char *,int, int line,int addr,int opcode,bool bBreak);
  bool bLineNumbers() { return m_bShowLineNumbers; }
  bool bAddresses()   { return m_bShowAddresses; }
  bool bOpcodes()     { return m_bShowOpcodes; }
private:
  bool m_bShowLineNumbers;
  bool m_bShowAddresses;
  bool m_bShowOpcodes;
};

//========================================================================
// SourcePage
// A single source file.
//
// The SourcePage class associates a single file with a single GtkTextBuffer.
// manages
class NSourcePage
{
public:
  NSourcePage(SourceWindow *, SourceBuffer  *, int file_id, GtkWidget *);

  GtkTextBuffer *buffer();
  GtkTextView   *getView();
  SourceWindow  *getParent();

  void invalidateView();
  void updateMargin(int y1, int y2);

  void Close(void);
  void setFont(const char *);
  void setSource();
  bool bHasSource();

  FileContext *getFC();

  // callbacks
  static gint KeyPressHandler(GtkTextView *pView,
                GdkEventKey *key,
                SourceWindow *pSW);
  static gint ButtonPressHandler(GtkTextView *pView,
                  GdkEventButton *pButton,
                  SourceWindow *pSW);
  static gint ViewExposeEventHandler(GtkTextView *pView,
                GdkEventExpose *pEvent,
                SourceWindow *pSW);


  unsigned int   m_fileid;
  SourceBuffer  *m_pBuffer;
  int            m_marginWidth;
private:
  SourceWindow  *m_Parent;
  char          *m_cpFont;
  GtkWidget     *m_pContainer;
  GtkTextView   *m_view;
};

class SearchDialog;

class SourceWindow : public GUI_Object
{
public:
  SourceWindow(GUI_Processor *gp,
               SourceBrowserParent_Window *,
               bool bUseConfig,
               const char *newName=0);

  virtual void Build();
  virtual void SetTitle();


  void set_pma(ProgramMemoryAccess *new_pma);
  void Create(void);
  virtual void SelectAddress(int address);
  virtual void SelectAddress(Value *);
  virtual void Update();
  virtual void UpdateLine(int address);
  virtual void SetPC(int address);
  virtual void CloseSource(void);
  virtual void NewSource(GUI_Processor *gp);

  virtual int getPCLine(int page);
  virtual int getAddress(NSourcePage *pPage, int line);
  virtual bool bAddressHasBreak(int address);
  virtual int getOpcode(int address);
  int  AddPage(SourceBuffer *pSourceBuffer, const char *fName);
  void step(int n=1);
  void step_over();
  void stop();
  void run();
  void finish();
  void reset();
  void toggleBreak(NSourcePage *pPage, int line);
  void movePC(int line);
  bool bSourceLoaded() { return m_bSourceLoaded; }
  void findText();
  int  findText(const char *, int, bool bDir, bool bCase);

  GtkTextTagTable *getTagTable();

  SourcePageMargin &margin();
  const char *getFont();
  gint switch_page_cb(guint newPage);

  // Font strings
  char commentfont_string[256];
  char sourcefont_string[256];

  GtkStyle *symbol_text_style;       // for symbols in .asm display
  GtkStyle *label_text_style;        // for label in .asm display
  GtkStyle *instruction_text_style;  // for instruction in .asm display
  GtkStyle *number_text_style;       // for numbers in .asm display
  GtkStyle *comment_text_style;      // for comments in .asm display
  GtkStyle *default_text_style;      // the rest

  GdkFont  *symbol_font;             // for symbols in .asm display
  GdkFont  *label_font;              // for label in .asm display
  GdkFont  *instruction_font;        // for instruction in .asm display
  GdkFont  *number_font;             // for numbers in .asm display
  GdkFont  *comment_font;            // for comments in .asm display
  GdkFont  *default_font;            // the rest

  // do we need this:
  bool m_bLoadSource;
  bool m_bSourceLoaded;
  int  m_LineAtButtonClick;

private:
  int AddPage(SourceBuffer *pSourceBuffer);

  ProgramMemoryAccess *pma;      // pointer to the processor's pma.
  StatusBar_Window *status_bar;  // display's PC, status, etc.
  SIMULATION_MODES last_simulation_mode;
  string sLastPmaName;
  unsigned int m_currentPage;          // Notebook page currently displayed.

  struct _PC {
    bool bIsActive;
    int  page;                // Notebook page containing the source
    int  line;                // Line within the page.
    GtkTextBuffer *pBuffer;   // Buffer containing the Program Counter
    GtkTextIter   iBegin;     // Start of where highlight begins
    GtkTextIter   iEnd;       // End of highlight
  } mProgramCounter;

  // Popup Menu
  SearchDialog *stPSearchDialog;
  GtkWidget * BuildPopupMenu();
  static void PopupMenuHandler(GtkWidget *widget,
                               gpointer data);

  // Callbacks
  static gint KeyPressHandler(GtkWidget *widget,
                GdkEventKey *key,
                SourceWindow *pSW);
  static int DeleteEventHandler(GtkWidget *widget,
                GdkEvent  *event,
                SourceWindow *sw);
  static gint cb_notebook_switchpage (GtkNotebook     *notebook,
                                    GtkNotebookPage *page,
                                    guint            page_num,
                                    SourceWindow     *pSW);


protected:
  void set_style_colors(const char *fg_color, const char *bg_color, GtkStyle **style);
  void addTagRange(NSourcePage *pPage, TextStyle *,
                   int start_index, int end_index);

  // FIXME - change these items to list objects
  NSourcePage **pages;

  GtkWidget *m_Notebook;

  GtkPositionType m_TabPosition;

  SourceBrowserParent_Window *m_pParent;


};

// #endif





class SourceBrowser_Window : public GUI_Object {
 public:
  GtkWidget *vbox;               // for children to put widgets in

  ProgramMemoryAccess *pma;      // pointer to the processor's pma.
  StatusBar_Window *status_bar;  // display's PC, status, etc.
  SIMULATION_MODES last_simulation_mode;
  string sLastPmaName;

  void set_pma(ProgramMemoryAccess *new_pma);

  void Create(void);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void SetTitle();
  virtual void SelectAddress(int address);
  virtual void SelectAddress(Value *);
  virtual void Update(void);
  virtual void UpdateLine(int address);
  virtual void SetPC(int address);
  virtual void CloseSource(void){};
  virtual void NewSource(GUI_Processor *gp){};

};


class BreakPointInfo {
public:
  BreakPointInfo(int _address, int _line, int _index, int _pos);
  BreakPointInfo(BreakPointInfo & Dup);
  ~BreakPointInfo();
  void Set(GtkWidget *, GdkPixmap *, GdkBitmap *);
  void Clear(GtkWidget *, GdkPixmap *, GdkBitmap *);
  void setBreakWidget(GtkWidget *);
  void setCanBreakWidget(GtkWidget *);
  int getLine() { return line; }
  GtkWidget *getBreakWidget() { return break_widget;}
  GtkWidget *getCanBreakWidget() { return canbreak_widget;}
  int address;
  int pos;
  unsigned int index;           // gtktext index to start of line
private:
  unsigned int line;            // line number, first line eq. 0
  GtkWidget *break_widget;      // breakpoint widget on this line.
  GtkWidget *canbreak_widget;   // 'can break' widget on this line.

};

class BreakPointList {
 public:

  GList *iter;

  BreakPointList(void);
  void Remove(int);
  void Add(int, GtkWidget *,GtkWidget *,int);
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

  // Something to set a debugger condition on when I want
  // to debug a specific source window.
  static int      m_SourceWindowCount;
  int             m_SourceWindowIndex;

  BreakPointList  breakpoints;
  BreakPointList  notify_start_list;
  BreakPointList  notify_stop_list;

  // Where the source is stored:

  int add_page(SourceBrowserAsm_Window *sbaw, int file_id);
  SourcePage pages[SBAW_NRFILES];
  static bool bGlobalInitialized;
  static GList *s_global_sa_xlate_list[SBAW_NRFILES];
  GList *sa_xlate_list[SBAW_NRFILES];
  static gint sigh_button_event(
    GtkWidget *widget,
    GdkEventButton *event,
    SourceBrowserAsm_Window *sbaw);

  int layout_offset;


  // Font strings
  char commentfont_string[256];
  char sourcefont_string[256];

  GtkWidget *popup_menu;

  BreakPointInfo *menu_data;  // used by menu callbacks

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

  GdkFont  *symbol_font;             // for symbols in .asm display
  GdkFont  *label_font;              // for label in .asm display
  GdkFont  *instruction_font;        // for instruction in .asm display
  GdkFont  *number_font;             // for numbers in .asm display
  GdkFont  *comment_font;            // for comments in .asm display
  GdkFont  *default_font;            // the rest

  GdkPixmap *pixmap_pc;
  GdkPixmap *pixmap_break;
  GdkPixmap *pixmap_canbreak;
  GdkPixmap *pixmap_profile_start;
  GdkPixmap *pixmap_profile_stop;

  PixmapObject canbreak;
  static int s_totallinesheight[SBAW_NRFILES];

  int m_bSourceLoaded;

  int m_bLoadSource;
  unsigned int current_page;        //Shadows the notebook->current_page;

  SourceBrowserParent_Window *parent;

  SourceBrowserAsm_Window(GUI_Processor *gp,char* new_name);
  virtual void SelectAddress(int address);
  virtual void SelectAddress(Value *);
  virtual void SetPC(int address);
  virtual void CloseSource(void);
  virtual void NewSource(GUI_Processor *gp);
  virtual void Update(void);
  virtual void UpdateLine(int address);
  virtual void SetText(int id, int file_id, FileContext *fc);
  void ParseSourceToFormattedText(
    int id,
    int &totallinesheight,
    bool &instruction_done,
    char *text_buffer,
    int &cblock,
    int &index,
    int &line,
    FileContext::Cache &FileCache,
    Processor *cpu,
    GtkWidget *pSourceWindow,
    FileContext *fc,
    int file_id  );

  void DetermineBreakinfos(int id);
  BreakPointInfo *getBPatLine(int id, unsigned int line);
  BreakPointInfo *getBPatPixel(int id, int pixel);
  BreakPointInfo *getBPatIndex(int id, unsigned int index);

  static void find_cb(GtkWidget *w, SourceBrowserAsm_Window *sbaw);
  static void find_clear_cb(GtkWidget *w, SourceBrowserAsm_Window *sbaw);

  // Popup Menu
  GtkWidget *     BuildPopupMenu(GtkWidget *sheet,
                                 SourceBrowserAsm_Window *sbaw);
  static void     PopupMenuHandler(GtkWidget *widget, gpointer data);
  static gint switch_page_cb(GtkNotebook     *notebook,
                           GtkNotebookPage *page,
                           guint            page_num,
                           SourceBrowserAsm_Window *sbaw);
  static void remove_all_points(SourceBrowserAsm_Window *sbaw);


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

  gchar **column_titles; //
  int  columns;         //

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
  virtual void Fill();

};



#define SOURCE_WINDOW SourceWindow


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

  SourceBrowserParent_Window(GUI_Processor *gp);
  virtual void Build(void);
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void SelectAddress(int address);
  virtual void SelectAddress(Value *);
  virtual void Update(void);
  virtual void UpdateLine(int address);
  virtual void SetPC(int address);
  virtual void CloseSource(void);
  virtual void NewSource(GUI_Processor *gp);
  virtual void ChangeView(int view_state);
  virtual int set_config();

  SOURCE_WINDOW *getChild(int);
  list<SOURCE_WINDOW *> children;

  ProgramMemoryAccess *pma;      // pointer to the processor's pma.

  GtkTextTagTable *getTagTable() { return mpTagTable; }
  void CreateSourceBuffers(GUI_Processor *gp);
  //void parseLine(gpsimTextBuffer *pBuffer, const char*, int parseStyle);
  void parseSource(SourceBuffer *pBuffer,FileContext *pFC);
  SourcePageMargin &margin();
  void setTabPosition(int tt);
  int getTabPosition() { return m_TabType; }
  void setFont(const char *);
  const char *getFont();
  //protected:

  GtkTextTagTable *mpTagTable;

  TextStyle *mLabel;       // for label in .asm display
  TextStyle *mMnemonic;    // for instruction in .asm display
  TextStyle *mSymbol;      // for symbols in .asm display
  TextStyle *mComment;     // for comments in .asm display
  TextStyle *mConstant;    // for numbers in .asm display
  TextStyle *mDefault;     // for everything else.

  TextStyle *mBreakpointTag;   // for breakpoints
  TextStyle *mNoBreakpointTag;
  TextStyle *mCurrentLineTag;  // Highlights the line at the PC.
  GtkStyle  *mBreakStyle;

  SourcePageMargin m_margin;
  int m_TabType;
  char *m_FontDescription;

  // FIXME - change these items to list objects
  SourceBuffer **ppSourceBuffers;

};





#endif // __GUI_SRC_H__

