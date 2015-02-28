/*
   Copyright (C) 1998,1999,2000,2001
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

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <cstring>

#include <gtkextra/gtkbordercombo.h>
#include <gtkextra/gtkcolorcombo.h>
#include <gtkextra/gtksheet.h>


#include "gui.h"
#include "preferences.h"
#include "gui_callbacks.h"
#include "gui_breadboard.h"
#include "gui_processor.h"
#include "gui_profile.h"
#include "gui_register.h"
#include "gui_regwin.h"
#include "gui_scope.h"
#include "gui_src.h"
#include "gui_stack.h"
#include "gui_stopwatch.h"
#include "gui_symbols.h"
#include "gui_trace.h"
#include "gui_watch.h"

#include "../cli/input.h"  // for gpsim_open()
#include "../src/gpsim_interface.h"


GtkUIManager *ui;

extern GUI_Processor *gpGuiProcessor;

static void
do_quit_app(GtkWidget *widget)
{
        exit_gpsim(0);
}


//========================================================================
static void
about_cb(GtkAction *action, gpointer user_data)
{
  static const gchar *authors[] = {
    "Scott Dattalo - <scott@dattalo.com>",
    "Ralf Forsberg - <rfg@home.se>",
    "Borut Ra" "\xc5\xbe" "em - <borut.razem@gmail.com>",
    NULL
  };

  gtk_show_about_dialog(NULL,
    "authors", authors,
    "comments", "A simulator for Microchip PIC microcontrollers.",
    "version", VERSION,
    "website", "http://gpsim.sourceforge.net/gpsim.html",
    "program-name", "The GNUPIC Simulator",
    NULL);
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//========================================================================
//
// class ColorButton
//
// Creates a GtkColorButton and places it into a parent widget.
// When the color button is clicked and changed, the signal will
// call back into this class and keep track of the selected
// color state.
class SourceBrowserPreferences;
class ColorButton
{
public:
  ColorButton (GtkWidget *pParent,
               TextStyle *pStyle,
               const char *label,
               SourceBrowserPreferences *
               );
  static void setColor_cb(GtkColorButton *widget,
                          ColorButton    *This);
  void apply();
  void cancel();
  TextStyle *m_pStyle;
private:
  SourceBrowserPreferences *m_prefs;
  const char *m_label;
};

//========================================================================
//
// class MarginButton
//
// Creates a GtkCheckButton that is used to select whether line numbers,
// addresses or opcodes will be displayed in the source browser margin.

class MarginButton
{
public:
  enum eMarginType {
    eLineNumbers,
    eAddresses,
    eOpcodes
  };
  MarginButton (GtkWidget *pParent,
                const char *pName,
                eMarginType id,
                SourceBrowserPreferences *
               );
  static void toggle_cb(GtkToggleButton *widget,
                        MarginButton    *This);
  void set_active();
private:
  GtkWidget *m_button;
  SourceBrowserPreferences *m_prefs;
  eMarginType m_id;
};

//========================================================================
//
// class TabButton
//
// Creates a GtkCheckButton that is used to select whether line numbers,
// addresses or opcodes will be displayed in the source browser margin.

class TabButton
{
public:
  TabButton (GtkWidget *pParent, GtkWidget *pButton,
             int id,
             SourceBrowserPreferences *
             );
  static void toggle_cb(GtkToggleButton *widget,
                        TabButton    *This);
  void set_active();
private:
  GtkWidget *m_button;
  SourceBrowserPreferences *m_prefs;
  int m_id;
};

//========================================================================
//
// class FontSelection
//

class FontSelection
{
public:
  FontSelection (GtkWidget *pParent,
                 SourceBrowserPreferences *
                 );
  static void setFont_cb(GtkFontButton *widget,
                        FontSelection  *This);
  void setFont();
private:
  SourceBrowserPreferences *m_prefs;
  GtkWidget *m_fontButton;
};

//========================================================================
//
class SourceBrowserPreferences : public SourceWindow
{
public:
  SourceBrowserPreferences(GtkWidget *pParent);

  void apply();
  void cancel();
  void update();
  void toggleBreak(int line);
  void movePC(int line);

  virtual int getPCLine(int page);
  virtual int getAddress(NSourcePage *pPage, int line);
  virtual bool bAddressHasBreak(int address);
  virtual int getOpcode(int address);
  void setTabPosition(int);
  void setFont(const char *);
  const char *getFont();
private:

  ColorButton *m_LabelColor;
  ColorButton *m_MnemonicColor;
  ColorButton *m_SymbolColor;
  ColorButton *m_CommentColor;
  ColorButton *m_ConstantColor;

  MarginButton *m_LineNumbers;
  MarginButton *m_Addresses;
  MarginButton *m_Opcodes;

  int m_currentTabPosition;
  int m_originalTabPosition;
  TabButton    *m_Up;
  TabButton    *m_Left;
  TabButton    *m_Down;
  TabButton    *m_Right;
  TabButton    *m_None;

  FontSelection *m_FontSelector;
};

//------------------------------------------------------------------------
class gpsimGuiPreferences
{
public:
  gpsimGuiPreferences();
  ~gpsimGuiPreferences();

  static void setup (GtkAction *action, gpointer user_data);


private:
  SourceBrowserPreferences *m_SourceBrowser;

  static void response_cb(GtkDialog *dialog, gint response_id,
    gpsimGuiPreferences *Self);
  void apply() { m_SourceBrowser->apply();}
  void cancel() { m_SourceBrowser->cancel();}
  GtkWidget *window;
};


void gpsimGuiPreferences::setup (GtkAction *action, gpointer user_data)
{
  new gpsimGuiPreferences();
}

void gpsimGuiPreferences::response_cb(GtkDialog *dialog, gint response_id,
  gpsimGuiPreferences *Self)
{
  if (response_id == gint(GTK_RESPONSE_CANCEL))
    Self->cancel();
  if (response_id == gint(GTK_RESPONSE_APPLY))
    Self->apply();
  delete Self;
}


//------------------------------------------------------------------------
// ColorButton Constructor
ColorButton::ColorButton(GtkWidget *pParent, TextStyle *pStyle,
                         const char *colorName,SourceBrowserPreferences *prefs)
  : m_pStyle(pStyle),m_prefs(prefs), m_label(colorName)
{
  GtkWidget *hbox        = gtk_hbox_new(0,0);
  gtk_box_pack_start (GTK_BOX (pParent), hbox, FALSE, TRUE, 0);

  GtkWidget *colorButton = gtk_color_button_new_with_color (pStyle->mFG.CurrentColor());
  gtk_color_button_set_title (GTK_COLOR_BUTTON(colorButton), colorName);
  gtk_box_pack_start (GTK_BOX(hbox),colorButton,FALSE, FALSE, 0);
  gtk_widget_show(colorButton);

  g_signal_connect (colorButton,
                      "color-set",
                      G_CALLBACK(setColor_cb),
                      this);

  GtkWidget *label       = gtk_label_new(colorName);
  gtk_box_pack_start (GTK_BOX(hbox),label,FALSE,FALSE, 10);
  gtk_widget_show (label);

  gtk_widget_show (hbox);
}

//------------------------------------------------------------------------
void ColorButton::setColor_cb(GtkColorButton *widget,
                              ColorButton    *This)
{
  GdkColor newColor;
  gtk_color_button_get_color (widget, &newColor);
  This->m_pStyle->setFG(&newColor);
}

void ColorButton::apply()
{
  m_pStyle->apply();
}
void ColorButton::cancel()
{
  m_pStyle->revert();
}

//------------------------------------------------------------------------
MarginButton::MarginButton(GtkWidget *pParent, const char *pName,
                           eMarginType id,
                           SourceBrowserPreferences *prefs)
  : m_prefs(prefs), m_id(id)
{
  m_button = gtk_check_button_new_with_label (pName);
  bool bState = false;
  switch (m_id) {
  case eLineNumbers:
    bState = m_prefs->margin().bLineNumbers();
    break;
  case eAddresses:
    bState = m_prefs->margin().bAddresses();
    break;
  case eOpcodes:
    bState = m_prefs->margin().bOpcodes();
    break;
  }
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(m_button),
                                bState);
  gtk_box_pack_start (GTK_BOX (pParent), m_button, FALSE, TRUE, 10);

  g_signal_connect (m_button,
                      "toggled",
                      G_CALLBACK(toggle_cb),
                      this);
}
//------------------------------------------------------------------------
void MarginButton::toggle_cb(GtkToggleButton *widget,
                             MarginButton    *This)
{
  This->set_active();
}
void MarginButton::set_active()
{
  bool bNewState = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_button)) ? true : false;
  switch (m_id) {
  case eLineNumbers:
    m_prefs->margin().enableLineNumbers(bNewState);
    break;
  case eAddresses:
    m_prefs->margin().enableAddresses(bNewState);
    break;
  case eOpcodes:
    m_prefs->margin().enableOpcodes(bNewState);
    break;
  }

}

//------------------------------------------------------------------------
TabButton::TabButton(GtkWidget *pParent, GtkWidget *pButton,
                     int id,
                     SourceBrowserPreferences *prefs)
  : m_button(pButton), m_prefs(prefs), m_id(id)
{
  gtk_box_pack_start (GTK_BOX (pParent), m_button, FALSE, TRUE, 5);

  g_signal_connect (m_button,
                      "toggled",
                      G_CALLBACK(toggle_cb),
                      this);
}
//------------------------------------------------------------------------
void TabButton::toggle_cb(GtkToggleButton *widget,
                          TabButton    *This)
{
  This->set_active();
}
void TabButton::set_active()
{
  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m_button)))
    m_prefs->setTabPosition(m_id);
}

//------------------------------------------------------------------------
FontSelection::FontSelection (GtkWidget *pParent,
                              SourceBrowserPreferences *pPrefs)
  : m_prefs(pPrefs)
{
  GtkWidget *frame = gtk_frame_new ("Font");

  gtk_box_pack_start (GTK_BOX (pParent), frame, FALSE, TRUE, 0);
  GtkWidget *hbox        = gtk_hbox_new(0,0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  m_fontButton = gtk_font_button_new_with_font (m_prefs->getFont());
  const char *fontDescription = "Font Selector";
  gtk_font_button_set_title (GTK_FONT_BUTTON(m_fontButton), fontDescription);
  gtk_box_pack_start (GTK_BOX(hbox),m_fontButton,FALSE, FALSE, 0);
  gtk_widget_show(m_fontButton);

  g_signal_connect (m_fontButton,
                      "font-set",
                      G_CALLBACK(setFont_cb),
                      this);

  GtkWidget *label = gtk_label_new("font");
  gtk_box_pack_start (GTK_BOX(hbox),label,TRUE, TRUE, 10);
  gtk_widget_show (label);

  gtk_widget_show (hbox);
}
//------------------------------------------------------------------------
void FontSelection::setFont_cb (GtkFontButton *pFontButton,
                                FontSelection *This)
{
  This->setFont();
}
void FontSelection::setFont()
{
  m_prefs->setFont(gtk_font_button_get_font_name (GTK_FONT_BUTTON(m_fontButton)));
}

//------------------------------------------------------------------------

void SourceBrowserPreferences::toggleBreak(int line)
{

}


void SourceBrowserPreferences::movePC(int line)
{
}


//========================================================================
SourceBrowserPreferences::SourceBrowserPreferences(GtkWidget *pParent)
  : SourceWindow(0,0,false,0)
{

  if (!gpGuiProcessor && !gpGuiProcessor->source_browser)
    return;

  GtkWidget *notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos((GtkNotebook*)notebook,GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (pParent), notebook, TRUE, TRUE, 0);
  gtk_widget_show(notebook);

  m_pParent = gpGuiProcessor->source_browser;
  GtkWidget *label;

  {
    // Color Frame for Source Browser configuration

    GtkWidget *vbox = gtk_vbox_new(0,0);

    GtkWidget *colorFrame = gtk_frame_new ("Colors");
    gtk_box_pack_start (GTK_BOX (vbox), colorFrame, FALSE, TRUE, 0);

    GtkWidget *colorVbox = gtk_vbox_new(0,0);
    gtk_container_add (GTK_CONTAINER (colorFrame), colorVbox);

    m_LabelColor    = new ColorButton(colorVbox,
                                      m_pParent->mLabel,
                                      "Label", this);
    m_MnemonicColor = new ColorButton(colorVbox,
                                      m_pParent->mMnemonic,
                                      "Mnemonic", this);
    m_SymbolColor   = new ColorButton(colorVbox,
                                      m_pParent->mSymbol,
                                      "Symbols", this);
    m_ConstantColor = new ColorButton(colorVbox,
                                      m_pParent->mConstant,
                                      "Constants", this);
    m_CommentColor  = new ColorButton(colorVbox,
                                      m_pParent->mComment,
                                      "Comments", this);

    // Font selector
    //preferences_AddFontSelect(GTK_WIDGET(vbox), "Font Selector", "font");
    m_FontSelector = new FontSelection(vbox,this);

    label = gtk_label_new("Font");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox,label);



  }

  {
    // Tab Frame for the Source browser
    m_currentTabPosition = m_pParent->getTabPosition();
    m_originalTabPosition = m_currentTabPosition;

    GtkWidget *hbox = gtk_hbox_new(0,0);
    GtkWidget *tabFrame = gtk_frame_new ("Notebook Tabs");
    gtk_box_pack_start (GTK_BOX (hbox), tabFrame, FALSE, TRUE, 0);

    GtkWidget *radioUp  = gtk_radio_button_new_with_label (NULL,"up");
    GtkRadioButton *rb  = GTK_RADIO_BUTTON(radioUp);

    GtkWidget *tabVbox = gtk_vbox_new(0,0);
    gtk_container_add (GTK_CONTAINER (tabFrame), tabVbox);

    m_Up    = new TabButton(tabVbox, radioUp, GTK_POS_TOP, this);
    m_Left  = new TabButton(tabVbox, gtk_radio_button_new_with_label_from_widget (rb,"left"),
                           GTK_POS_LEFT, this);
    m_Down  = new TabButton(tabVbox, gtk_radio_button_new_with_label_from_widget (rb,"down"),
                           GTK_POS_BOTTOM, this);
    m_Right = new TabButton(tabVbox, gtk_radio_button_new_with_label_from_widget (rb,"right"),
                           GTK_POS_RIGHT, this);
    m_None  = new TabButton(tabVbox, gtk_radio_button_new_with_label_from_widget (rb,"none"),
                           -1, this);


    // Source browser margin
    GtkWidget *marginFrame = gtk_frame_new ("Margin");
    gtk_box_pack_start (GTK_BOX (hbox), marginFrame, FALSE, TRUE, 0);
    GtkWidget *marginVbox = gtk_vbox_new(0,0);
    gtk_container_add (GTK_CONTAINER (marginFrame), marginVbox);

    m_LineNumbers = new MarginButton(marginVbox, "Line Numbers",
                                     MarginButton::eLineNumbers, this);
    m_Addresses   = new MarginButton(marginVbox, "Addresses",
                                     MarginButton::eAddresses, this);
    m_Opcodes     = new MarginButton(marginVbox, "Opcodes",
                                     MarginButton::eOpcodes, this);

    label = gtk_label_new("Margins");
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook),hbox,label);

  }

  {
    SourceBuffer *pBuffer = new SourceBuffer (m_pParent->getTagTable(),0,m_pParent);


    GtkWidget *frame = gtk_frame_new ("Sample");
    gtk_box_pack_start (GTK_BOX (pParent), frame, FALSE, TRUE, 0);

    m_Notebook = gtk_notebook_new();
    //m_currentTabPosition = m_pParent->getTabPosition();
    //gtk_notebook_set_tab_pos((GtkNotebook*)m_Notebook,m_TabPosition);
    setTabPosition(m_pParent->getTabPosition());

    gtk_container_add (GTK_CONTAINER (frame), m_Notebook);

    bIsBuilt = true;

    int id = AddPage (pBuffer, "file1.asm");

    pages[id]->m_pBuffer->parseLine( "        MOVLW   0x34       ; Comment",1);
    pages[id]->m_pBuffer->parseLine( "; Comment only",1);
    pages[id]->m_pBuffer->parseLine( "Label:  ADDWF  Variable,F  ; Comment",1);

    gtk_widget_show_all(frame);

    label = gtk_label_new("file2.asm");
    GtkWidget *emptyBox = gtk_hbox_new(0,0);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_Notebook),emptyBox,label);

  }

  gtk_widget_show_all(notebook);

}

void SourceBrowserPreferences::setTabPosition(int tabPosition)
{
  m_currentTabPosition = tabPosition;
  m_pParent->setTabPosition(tabPosition);
  if (tabPosition >= 0) {
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_Notebook),TRUE);
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(m_Notebook), (GtkPositionType) m_currentTabPosition);
  } else {
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_Notebook),FALSE);
  }
  Update();
}

void SourceBrowserPreferences::setFont(const char *cpFont)
{
  m_pParent->setFont(cpFont);
}

const char *SourceBrowserPreferences::getFont()
{
  return m_pParent->getFont();
}

void SourceBrowserPreferences::apply()
{
  m_LabelColor->apply();
  m_MnemonicColor->apply();
  m_SymbolColor->apply();
  m_ConstantColor->apply();
  m_CommentColor->apply();

  m_pParent->setTabPosition(m_currentTabPosition);
}

void SourceBrowserPreferences::cancel()
{

  m_LabelColor->cancel();
  m_MnemonicColor->cancel();
  m_SymbolColor->cancel();
  m_ConstantColor->cancel();
  m_CommentColor->cancel();

  m_pParent->setTabPosition(m_originalTabPosition);
}

int SourceBrowserPreferences::getPCLine(int page)
{
  return 1;
}
int SourceBrowserPreferences::getAddress(NSourcePage *pPage, int line)
{
  return 0x1234;
}
bool SourceBrowserPreferences::bAddressHasBreak(int address)
{
  return false;
}
int SourceBrowserPreferences::getOpcode(int address)
{
  return 0xABCD;
}

//========================================================================
gpsimGuiPreferences::gpsimGuiPreferences()
{
  window = gtk_dialog_new_with_buttons("Source Browser configuration",
    NULL,
    GTK_DIALOG_MODAL,
    GTK_STOCK_CANCEL, gint(GTK_RESPONSE_CANCEL),
    GTK_STOCK_APPLY, gint(GTK_RESPONSE_APPLY),
    NULL);

  g_signal_connect(window, "response",
    G_CALLBACK(gpsimGuiPreferences::response_cb), this);

  GtkWidget *box = gtk_dialog_get_content_area(GTK_DIALOG(window));

  m_SourceBrowser = new SourceBrowserPreferences(box);

  gtk_widget_show_all(window);
}

gpsimGuiPreferences::~gpsimGuiPreferences()
{


  gtk_widget_destroy (window);

  delete m_SourceBrowser;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//========================================================================

extern int gui_message(const char *message);

static void
fileopen_dialog(GtkAction *action, gpointer user_data)
{
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Open file",
    NULL,
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    NULL);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    if (!gpsim_open(gpGuiProcessor->cpu, filename, 0, 0)) {
      gchar *msg = g_strdup_printf(
        "Open failed. Could not open \"%s\"", filename);
      gui_message(msg);
      g_free(msg);
    }
    g_free(filename);
  }

  gtk_widget_destroy(dialog);
}




// Menuhandler for Windows menu buttons
static void
toggle_window(GtkToggleAction *action, gpointer user_data)
{
  if (gpGuiProcessor) {
    std::string item = gtk_action_get_name(GTK_ACTION(action));
    gboolean view_state = gtk_toggle_action_get_active(action);

    if (item == "Program memory") {
      gpGuiProcessor->program_memory->ChangeView(view_state);
    } else if (item == "Source") {
      gpGuiProcessor->source_browser->ChangeView(view_state);
    } else if (item == "Ram") {
      gpGuiProcessor->regwin_ram->ChangeView(view_state);
    } else if (item == "EEPROM") {
      gpGuiProcessor->regwin_eeprom->ChangeView(view_state);
    } else if (item == "Watch") {
      gpGuiProcessor->watch_window->ChangeView(view_state);
    } else if (item == "Symbols") {
      gpGuiProcessor->symbol_window->ChangeView(view_state);
    } else if (item == "Breadboard") {
      gpGuiProcessor->breadboard_window->ChangeView(view_state);
    } else if (item == "Stack") {
      gpGuiProcessor->stack_window->ChangeView(view_state);
    } else if (item == "Trace") {
      gpGuiProcessor->trace_window->ChangeView(view_state);
    } else if (item == "Profile") {
      gpGuiProcessor->profile_window->ChangeView(view_state);
    } else if (item == "Stopwatch") {
      gpGuiProcessor->stopwatch_window->ChangeView(view_state);
    } else if (item == "Scope") {
      gpGuiProcessor->scope_window->ChangeView(view_state);
    }
  }
}

//========================================================================
// Button callbacks
static void
runbutton_cb(GtkWidget *widget)
{
  get_interface().start_simulation();
}

static void
stopbutton_cb(GtkWidget *widget)
{
  if(gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->stop();
}

static void
stepbutton_cb(GtkWidget *widget)
{
  if(gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->step(1);
}

static void
overbutton_cb(GtkWidget *widget)
{
  if(gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->step_over();

}

static void
finishbutton_cb(GtkWidget *widget)
{
  if(gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->finish();
}

static void
resetbutton_cb(GtkWidget *widget)
{
  if(gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->reset(POR_RESET);
}


int gui_animate_delay; // in milliseconds


//========================================================================
//========================================================================
//
// UpdateRateMenuItem

//========================================================================
//
// Class declaration -- probaby should move to a .h file.

class UpdateRateMenuItem {
public:

  char id;
  int menu_index;
  bool bRealTime;
  bool bWithGui;
  bool bAnimate;

  int update_rate;

  UpdateRateMenuItem(GtkWidget *,char, const char *, int update_rate=0, bool _bRealTime=false, bool _bWithGui=false);

  void Select();
  static GtkWidget *menu;
  static int seq_no;
};
//========================================================================
// UpdateRateMenuItem members

GtkWidget * UpdateRateMenuItem::menu = 0;
int UpdateRateMenuItem::seq_no=0;

map<guint, UpdateRateMenuItem*> UpdateRateMenuItemMap;
map<guint, UpdateRateMenuItem*> UpdateRateMenuItemIndexed;


static void
gui_update_cb(GtkWidget *widget, gpointer data)
{
  GtkComboBox *combo_box = GTK_COMBO_BOX(widget);
  gint index = combo_box ? gtk_combo_box_get_active(combo_box) : 0;

  UpdateRateMenuItem *umi = UpdateRateMenuItemIndexed[index];
  if (umi)
    umi->Select();
  else
    cout << "Error UpdateRateMenuItem bad index:" << index << endl;
}

UpdateRateMenuItem::UpdateRateMenuItem(GtkWidget *parent,
                                       char _id,
                                       const char *label,
                                       int _update_rate,
                                       bool _bRealTime,
                                       bool _bWithGui)
  : id(_id), bRealTime(_bRealTime), bWithGui(_bWithGui), update_rate(_update_rate)
{

  if(update_rate <0) {
    bAnimate = true;
    update_rate = -update_rate;
  } else
    bAnimate = false;

  if(!menu)
    menu = gtk_menu_new();

  gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(parent), label);

  menu_index = seq_no;
  seq_no++;

  UpdateRateMenuItemMap[id] = this;
  UpdateRateMenuItemIndexed[menu_index] = this;

}

void UpdateRateMenuItem::Select()
{
  EnableRealTimeMode(bRealTime);
  EnableRealTimeModeWithGui(bWithGui);

  if(bAnimate) {
    gui_animate_delay = update_rate;
    get_interface().set_update_rate(1);
  } else {
    gui_animate_delay = 0;
    get_interface().set_update_rate(update_rate);
  }

  if(gpGuiProcessor && gpGuiProcessor->cpu)
    gpGuiProcessor->cpu->pma->stop();

  config_set_variable("dispatcher", "SimulationMode", id);

  if (0)
    cout << "Update gui refresh: " << hex << update_rate
         << " ID:" << id << "Seq no:" << menu_index
         << endl;
}


//========================================================================
//========================================================================
class TimeWidget;
class TimeFormatter
{
public:
  enum eMenuID {
    eCyclesHex=0,
    eCyclesDec,
    eMicroSeconds,
    eMilliSeconds,
    eSeconds,
    eHHMMSS
  } time_format;

  TimeFormatter(TimeWidget *_tw,GtkWidget *menu, const char*menu_text)
   : tw(_tw)
  {
    AddToMenu(menu,menu_text);
  }

  virtual ~TimeFormatter()
  {
  }

  void ChangeFormat();
  void AddToMenu(GtkWidget *menu, const char*menu_text);
  virtual void Format(char *, int)=0;
  TimeWidget *tw;
};

class TimeWidget : public EntryWidget
{
public:
  TimeWidget();
  void Create(GtkWidget *);
  virtual void Update();
  void NewFormat(TimeFormatter *tf);
  TimeFormatter *current_format;

  GtkWidget *menu;
};


class TimeMicroSeconds : public TimeFormatter
{
public:
  TimeMicroSeconds(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw,menu,"MicroSeconds") {}

  virtual ~TimeMicroSeconds()
  {
  }

  void Format(char *buf, int size)
  {
    double time_db = 0.;
    if(gpGuiProcessor && gpGuiProcessor->cpu)
        time_db = gpGuiProcessor->cpu->get_InstPeriod() * get_cycles().get() * 1e6;
    g_snprintf(buf,size, "%19.2f us",time_db);
  }
};

class TimeMilliSeconds : public TimeFormatter
{
public:
  TimeMilliSeconds(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw,menu,"MilliSeconds") {}

  virtual ~TimeMilliSeconds()
  {
  }

  void Format(char *buf, int size)
  {
    double time_db = 0.;
  if(gpGuiProcessor && gpGuiProcessor->cpu)
        time_db = gpGuiProcessor->cpu->get_InstPeriod() * get_cycles().get() * 1e3;
    g_snprintf(buf,size, "%19.3f ms",time_db);
  }
};

class TimeSeconds : public TimeFormatter
{
public:
  TimeSeconds(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw,menu,"Seconds") {}

  virtual ~TimeSeconds()
  {
  }

  void Format(char *buf, int size)
  {
    double time_db = 0.;
  if(gpGuiProcessor && gpGuiProcessor->cpu)
       time_db = gpGuiProcessor->cpu->get_InstPeriod() * get_cycles().get();
    g_snprintf(buf,size, "%19.3f Sec",time_db);
  }
};

class TimeHHMMSS : public TimeFormatter
{
public:
  TimeHHMMSS(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw,menu,"HH:MM:SS.mmm") {}

  virtual ~TimeHHMMSS()
  {
  }

  void Format(char *buf, int size)
  {
    double time_db = 0.;
    if(gpGuiProcessor && gpGuiProcessor->cpu)
        time_db = gpGuiProcessor->cpu->get_InstPeriod() * get_cycles().get();
    double v=time_db + 0.005;	// round msec
    int hh=(int)(v/3600),mm,ss,cc;
    v-=hh*3600.0;
    mm=(int)(v/60);
    v-=mm*60.0;
    ss=(int)v;
    cc=(int)((v-ss)*100.0);
    g_snprintf(buf,size,"    %02d:%02d:%02d.%02d",hh,mm,ss,cc);
  }
};

class TimeCyclesHex : public TimeFormatter
{
public:
  TimeCyclesHex(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw,menu,"Cycles (Hex)") {}

  virtual ~TimeCyclesHex()
  {
  }

  void Format(char *buf, int size)
  {
    g_snprintf(buf,size,"0x%016" PRINTF_GINT64_MODIFIER "x",get_cycles().get());
  }
};

class TimeCyclesDec : public TimeFormatter
{
public:
  TimeCyclesDec(TimeWidget *tw, GtkWidget *menu)
    : TimeFormatter(tw,menu,"Cycles (Dec)") {}

  virtual ~TimeCyclesDec()
  {
  }

  void Format(char *buf, int size)
  {
    g_snprintf(buf,size,"%016" PRINTF_GINT64_MODIFIER "d",get_cycles().get());
  }
};

//========================================================================
// called when user has selected a menu item from the time format menu
static void
cbTimeFormatActivated(GtkWidget *widget, gpointer data)
{
  if(!widget || !data)
    return;

  TimeFormatter *tf = static_cast<TimeFormatter *>(data);
  tf->ChangeFormat();
}
// button press handler
static gint
cbTimeFormatPopup(GtkWidget *widget, GdkEventButton *event, TimeWidget *tw)
{
  if(!widget || !event || !tw)
    return 0;

  if(event->type == GDK_BUTTON_PRESS) {

    gtk_menu_popup(GTK_MENU(tw->menu), 0, 0, 0, 0,
                   3, event->time);
    // It looks like we need it to avoid a selection in the entry.
    // For this we tell the entry to stop reporting this event.
    g_signal_stop_emission_by_name(tw->entry, "button_press_event");
  }
  return FALSE;
}


void TimeFormatter::ChangeFormat()
{
  if(tw)
    tw->NewFormat(this);
}

void TimeFormatter::AddToMenu(GtkWidget *menu,
                              const char*menu_text)
{
  GtkWidget *item = gtk_menu_item_new_with_label(menu_text);
  g_signal_connect(item, "activate",
                     G_CALLBACK(cbTimeFormatActivated),
                     this);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
}

void TimeWidget::Create(GtkWidget *container)
{
  EntryWidget::Create(false);

  gtk_container_add(GTK_CONTAINER(container),entry);

  SetEntryWidth(18);

  menu = gtk_menu_new();

  // Create an entry for each item in the formatter pop up window.

  new TimeMicroSeconds(this,menu);
  new TimeMilliSeconds(this,menu);
  new TimeSeconds(this,menu);
  new TimeHHMMSS(this,menu);
  NewFormat(new TimeCyclesHex(this,menu));
  new TimeCyclesDec(this,menu);

  // Associate a callback with the user button-click actions
  g_signal_connect(entry,
                     "button_press_event",
                     G_CALLBACK(cbTimeFormatPopup),
                     this);
}


void TimeWidget::NewFormat(TimeFormatter *tf)
{
  if(tf && tf != current_format) {
    current_format = tf;
    Update();
  }
}

void TimeWidget::Update()
{
  if(!current_format)
    return;

  char buffer[32];

  current_format->Format(buffer, sizeof(buffer));
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

}

TimeWidget::TimeWidget()
{
  menu = 0;
  current_format =0;
}


//========================================================================
static int dispatcher_delete_event(GtkWidget *widget,
                                   GdkEvent  *event,
                                   gpointer data)
{
    do_quit_app(0);

    return 0;
}

static const GtkActionEntry entries[] = {
  {"FileMenu", NULL, "_File"},
  {"Open", GTK_STOCK_OPEN, "_Open", "<control>O", NULL, G_CALLBACK(fileopen_dialog)},
  {"Quit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit", G_CALLBACK(do_quit_app)},
  {"Windows", NULL, "_Windows"},
  {"Edit", NULL, "_Edit"},
  {"Preferences", GTK_STOCK_PREFERENCES, "Preferences", NULL, NULL, G_CALLBACK(gpsimGuiPreferences::setup)},
  {"Help", NULL, "_Help"},
  {"About", GTK_STOCK_ABOUT, "_About", NULL, NULL, G_CALLBACK(about_cb)}
};

static const GtkToggleActionEntry toggle_entries[] = {
  {"Program memory", NULL, "Program _memory", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Source", NULL, "_Source", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Ram", NULL, "_Ram", NULL, NULL, G_CALLBACK(toggle_window)},
  {"EEPROM", NULL, "_EEPROM", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Watch", NULL, "_Watch", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Stack", NULL, "Sta_ck", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Symbols", NULL, "Symbo_ls", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Breadboard", NULL, "_Breadboard", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Trace", NULL, "_Trace", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Profile", NULL, "Pro_file", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Stopwatch", NULL, "St_opwatch", NULL, NULL, G_CALLBACK(toggle_window)},
  {"Scope", NULL, "Sco_pe", NULL, NULL, G_CALLBACK(toggle_window)}
};

static const gchar *ui_info =
"<ui>"
"  <menubar name='menu'>"
"    <menu action='FileMenu'>"
"      <menuitem action='Open'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='Windows'>"
"      <menuitem action='Program memory'/>"
"      <menuitem action='Source'/>"
"      <separator/>"
"      <menuitem action='Ram'/>"
"      <menuitem action='EEPROM'/>"
"      <menuitem action='Watch'/>"
"      <menuitem action='Stack'/>"
"      <separator/>"
"      <menuitem action='Symbols'/>"
"      <menuitem action='Breadboard'/>"
"      <separator/>"
"      <menuitem action='Trace'/>"
"      <menuitem action='Profile'/>"
"      <menuitem action='Stopwatch'/>"
"      <menuitem action='Scope'/>"
"    </menu>"
"    <menu action='Edit'>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='Help'>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";

GtkWidget *dispatcher_window = 0;
//========================================================================
//========================================================================

class MainWindow
{
public:

  TimeWidget   *timeW;

  void Update();
  void Create();

  MainWindow();
};

MainWindow TheWindow;

MainWindow::MainWindow()
  : timeW(0)
{
}

void MainWindow::Update()
{
  if(timeW)
   timeW->Update();
}


//========================================================================
//========================================================================

void dispatch_Update()
{
  if(!dispatcher_window)
    return;

  if(gpGuiProcessor && gpGuiProcessor->cpu) {

    TheWindow.Update();

  }
}

void MainWindow::Create ()
{

  if (dispatcher_window)
    return;

  GtkWidget *box1;

  GtkWidget *buttonbox;
  GtkWidget *separator;
  GtkWidget *button;
  GtkWidget *frame;
  int x,y,width,height;

  GtkWidget *update_rate_menu;

  int SimulationMode;

  dispatcher_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  if(!config_get_variable("dispatcher", "x", &x))
    x=10;
  if(!config_get_variable("dispatcher", "y", &y))
    y=10;
  if(!config_get_variable("dispatcher", "width", &width))
    width=1;
  if(!config_get_variable("dispatcher", "height", &height))
    height=1;
  gtk_window_resize(GTK_WINDOW(dispatcher_window), width, height);
  gtk_window_move(GTK_WINDOW(dispatcher_window), x, y);


  g_signal_connect (dispatcher_window, "delete-event",
                      G_CALLBACK(dispatcher_delete_event),
                      0);

  GtkActionGroup *actions = gtk_action_group_new("Actions");
  gtk_action_group_add_actions(actions, entries, G_N_ELEMENTS(entries), NULL);
  gtk_action_group_add_toggle_actions(actions, toggle_entries,
    G_N_ELEMENTS(toggle_entries), NULL);

  ui = gtk_ui_manager_new();
  gtk_ui_manager_insert_action_group(ui, actions, 0);
  g_object_unref(actions);
  gtk_window_add_accel_group(GTK_WINDOW(dispatcher_window),
    gtk_ui_manager_get_accel_group(ui));

  if (!gtk_ui_manager_add_ui_from_string(ui, ui_info, -1, NULL)) {
    g_error("building menus failed");
  }

  gtk_window_set_title (GTK_WINDOW (dispatcher_window),
                        VERSION);
  gtk_container_set_border_width (GTK_CONTAINER (dispatcher_window), 0);

  box1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dispatcher_window), box1);

  gtk_box_pack_start (GTK_BOX (box1),
                      gtk_ui_manager_get_widget(ui, "/menu"),
                      FALSE, FALSE, 0);



  buttonbox = gtk_hbox_new(FALSE,0);
  gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 1);
  gtk_box_pack_start (GTK_BOX (box1), buttonbox, TRUE, TRUE, 0);



  // Buttons
  button = gtk_button_new_with_label ("step");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(stepbutton_cb), 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("over");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(overbutton_cb), 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("finish");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(finishbutton_cb), 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("run");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(runbutton_cb), 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("stop");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(stopbutton_cb), 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("reset");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(resetbutton_cb), 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);


  //
  // Simulation Mode Frame
  //

  frame = gtk_frame_new("Simulation mode");
  if(!config_get_variable("dispatcher", "SimulationMode", &SimulationMode))
    {
      SimulationMode='4';
    }
  //  set_simulation_mode(SimulationMode);

  //
  // Gui Update Rate
  //
  cout << "SimulationMode:"<<SimulationMode<<endl;

  update_rate_menu = gtk_combo_box_text_new();
  gtk_container_add(GTK_CONTAINER(frame),update_rate_menu);

  new UpdateRateMenuItem(update_rate_menu,'5',"Without gui (fastest simulation)",0);
  new UpdateRateMenuItem(update_rate_menu,'4',"2000000 cycles/gui update",2000000);
  new UpdateRateMenuItem(update_rate_menu,'3',"100000 cycles/gui update",100000);
  new UpdateRateMenuItem(update_rate_menu,'2',"1000 cycles/gui update",1000);
  new UpdateRateMenuItem(update_rate_menu,'1',"Update gui every cycle",1);
  new UpdateRateMenuItem(update_rate_menu,'b',"100ms animate",-100);
  new UpdateRateMenuItem(update_rate_menu,'c',"300ms animate",-300);
  new UpdateRateMenuItem(update_rate_menu,'d',"700ms animate",-700);
  new UpdateRateMenuItem(update_rate_menu,'r',"Realtime without gui",0,true);
  new UpdateRateMenuItem(update_rate_menu,'R',"Realtime with gui",0,true,true);

  UpdateRateMenuItem *umi = UpdateRateMenuItemMap[SimulationMode];

  if(!umi)
    cout << "error selecting update rate menu\n";
  umi->Select();

  gtk_combo_box_set_active(GTK_COMBO_BOX(update_rate_menu), umi->menu_index);

  g_signal_connect(update_rate_menu, "changed",
                     G_CALLBACK(gui_update_cb),
                     (gpointer)update_rate_menu);

  gtk_box_pack_start (GTK_BOX (buttonbox), frame, FALSE, FALSE, 5);

  //
  // Simulation Time Frame
  //

  frame = gtk_frame_new("Simulation Time");
  gtk_box_pack_start (GTK_BOX (buttonbox), frame, FALSE, FALSE, 5);

  timeW = new TimeWidget();
  timeW->Create(frame);
  timeW->Update();

  //gtk_box_pack_start (GTK_BOX (buttonbox), frame, TRUE, TRUE, 5);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 5);
  button = gtk_button_new_with_label ("Quit gpsim");
  g_signal_connect(button, "clicked",
                     G_CALLBACK(do_quit_app), 0);

  gtk_box_pack_start (GTK_BOX (box1), button, FALSE, TRUE, 5);
  gtk_widget_show_all (dispatcher_window);


}

//========================================================================

void create_dispatcher ()
{
  TheWindow.Create();
}

#endif // HAVE_GUI
