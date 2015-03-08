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
class SourceBrowserParent_Window;
class StatusBar_Window;
class Value;
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
  std::string   m_cpFont;
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
  std::string commentfont_string;
  std::string sourcefont_string;

  GtkStyle *symbol_text_style;       // for symbols in .asm display
  GtkStyle *label_text_style;        // for label in .asm display
  GtkStyle *instruction_text_style;  // for instruction in .asm display
  GtkStyle *number_text_style;       // for numbers in .asm display
  GtkStyle *comment_text_style;      // for comments in .asm display
  GtkStyle *default_text_style;      // the rest

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
  static void cb_notebook_switchpage(GtkNotebook    *notebook,
                                    gpointer         page,
                                    guint            page_num,
                                    SourceWindow     *pSW);

  std::string m_name;

protected:
  void set_style_colors(const char *fg_color, const char *bg_color, GtkStyle **style);
  void addTagRange(NSourcePage *pPage, TextStyle *,
                   int start_index, int end_index);

  // FIXME - change these items to list objects
  NSourcePage **pages;

  GtkWidget *m_Notebook;

  GtkPositionType m_TabPosition;

  SourceBrowserParent_Window *m_pParent;

  virtual const char *name();

public:
  const char *name_pub() {return name();}
};

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

//
// Source page
//

class SourcePage
{
 public:


  GtkAdjustment *source_layout_adj;
  GtkWidget *source_layout;
  GtkWidget *source_text;
  int 	    pageindex_to_fileid;
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
// The Source Opcode Browser Data
//
class SourceBrowserOpcode_Window : public SourceBrowser_Window
{
 public:

  GtkListStore *list;
  GtkWidget *tree;

  unsigned int current_address;   // current PC

  // Font strings
  std::string normalfont_string;

  PangoFontDescription *normalPFD;
  PangoFontDescription *current_line_numberPFD;
  PangoFontDescription *breakpoint_line_numberPFD;

  std::string breakpointfont_string;
  std::string pcfont_string;

  GtkStyle *normal_style;
  GtkStyle *current_line_number_style;
  GtkStyle *breakpoint_line_number_style;

  GdkColor pm_has_changed_color;
  GdkColor normal_pm_bg_color;
  GdkColor breakpoint_color;

  GtkWidget *notebook;
  GtkWidget *sheet;
  GtkWidget *entry;
  GtkWidget *label;

  GtkWidget *sheet_popup_menu;
  GtkWidget *list_popup_menu;

  GdkPixbuf *break_pix;
  GdkPixbuf *pc_pix;

  unsigned int ascii_mode; // 0, 1 or 2 equals
  // one byte/cell,
  // two bytes/cell MSB first
  // two bytes/cell LSB first

  unsigned int *memory;

  SourceBrowserOpcode_Window(GUI_Processor *gp);
  virtual void Build();
  virtual void NewProcessor(GUI_Processor *gp);
  virtual void SelectAddress(int address);
  virtual void SetPC(int address);
  virtual void NewSource(GUI_Processor *gp);
  virtual void UpdateLine(int address);
  virtual void Fill();

  void update_ascii(gint row);
  void load_styles();
  void settings_dialog();

protected:
  virtual const char *name();

private:
  GtkWidget *build_menu_for_sheet();
  GtkWidget *build_menu_for_list();

  void update_values(int address);
  void update_styles(int address);
  void update(int address);

  static void popup_activated(GtkWidget *widget, gpointer data);
  static void cell_renderer(GtkTreeViewColumn *tree_column,
    GtkCellRenderer *cell, GtkTreeModel *tree_model,
    GtkTreeIter *iter, gpointer data);
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

  SourceWindow *getChild(int);
  list<SourceWindow *> children;

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
  std::string m_FontDescription;

  // FIXME - change these items to list objects
  SourceBuffer **ppSourceBuffers;

protected:
  virtual const char *name();
};





#endif // __GUI_SRC_H__

