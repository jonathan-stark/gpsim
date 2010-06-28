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

#ifdef DOING_GNOME
#include <gnome.h>
#endif

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

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

typedef struct _note_book_item
{
  gchar        *name;
  GtkSignalFunc func;
} NotebookItem;

GtkItemFactory *item_factory=0;

extern GUI_Processor *gpGuiProcessor;

static void
do_quit_app(GtkWidget *widget)
{
        exit_gpsim(0);
}


static void
show_message (const char *title, const char *message)
{
  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *button;

  window = gtk_dialog_new ();

  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);


  button = gtk_button_new_with_label ("close");
  gtk_container_set_border_width (GTK_CONTAINER (button), 10);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             GTK_SIGNAL_FUNC(gtk_widget_destroy),
                             GTK_OBJECT (window));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);


  label = gtk_label_new (message);
  gtk_misc_set_padding (GTK_MISC (label), 10, 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), label, TRUE, TRUE, 0);

  gtk_widget_show (label);
  gtk_widget_show (window);

  gtk_grab_add (window);

}
//========================================================================
static void
about_cb (gpointer             callback_data,
          guint                callback_action,
          GtkWidget           *widget)
{

  show_message(  "The GNUPIC Simulator - " VERSION, "A simulator for Microchip PIC microcontrollers.\n"
                 "by T. Scott Dattalo - mailto:scott@dattalo.com\n"
                 "   Ralf Forsberg - mailto:rfg@home.se\n"
                 "   Borut Ra" "\xc5\xbe" "em - mailto:borut.razem@siol.net\n\n"
                 "gpsim homepage: http://gpsim.sourceforge.net/gpsim.html\n"
                 "gpsimWin32: http://gpsim.sourceforge.net/gpsimWin32/gpsimWin32.html\n");

}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#if defined(NEW_SOURCE_BROWSER)

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

  static void setup (gpointer             callback_data,
                     guint                callback_action,
                     GtkWidget           *widget);


private:
  SourceBrowserPreferences *m_SourceBrowser;

  static gint cancel_cb (gpsimGuiPreferences *Self);
  static gint apply_cb  (gpsimGuiPreferences *Self);
  void apply() { m_SourceBrowser->apply();}
  void cancel() { m_SourceBrowser->cancel();}
  GtkWidget *window;
};
static GtkWidget *LocalWindow=0;


void gpsimGuiPreferences::setup (gpointer             callback_data,
                                 guint                callback_action,
                                 GtkWidget           *widget)
{
  new gpsimGuiPreferences();
}


gint gpsimGuiPreferences::cancel_cb (gpsimGuiPreferences  *Self)
{

  Self->cancel();
  delete Self;
  return TRUE;
}
gint gpsimGuiPreferences::apply_cb  (gpsimGuiPreferences *Self)
{

  Self->apply();
  delete Self;
  return TRUE;
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

  gtk_signal_connect (GTK_OBJECT(colorButton),
                      "color-set",
                      GTK_SIGNAL_FUNC(setColor_cb),
                      this);
  const int cBORDER = 10; // pixels
  GtkWidget *label       = gtk_label_new(colorName);
  gtk_box_pack_start (GTK_BOX(hbox),label,FALSE,FALSE, cBORDER);
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

  gtk_signal_connect (GTK_OBJECT(m_button),
                      "toggled",
                      GTK_SIGNAL_FUNC(toggle_cb),
                      this);
}
//------------------------------------------------------------------------
void MarginButton::toggle_cb(GtkToggleButton *widget,
                             MarginButton    *This)
{
  //  This->m_pStyle->setFG(&newColor);
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

  gtk_signal_connect (GTK_OBJECT(m_button),
                      "toggled",
                      GTK_SIGNAL_FUNC(toggle_cb),
                      this);
}
//------------------------------------------------------------------------
void TabButton::toggle_cb(GtkToggleButton *widget,
                          TabButton    *This)
{
  //  This->m_pStyle->setFG(&newColor);
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

  gtk_signal_connect (GTK_OBJECT(m_fontButton),
                      "font-set",
                      GTK_SIGNAL_FUNC(setFont_cb),
                      this);

  const char *fontName = "font";
  const int cBORDER = 10; // pixels
  GtkWidget *label       = gtk_label_new(fontName);
  gtk_box_pack_start (GTK_BOX(hbox),label,TRUE, TRUE, cBORDER);
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
#if 0 // defined but not used
static bool isButtonEvent (GdkEventType type)
{
  return
    type == GDK_BUTTON_PRESS ||
    type == GDK_2BUTTON_PRESS ||
    type == GDK_3BUTTON_PRESS ||
    type == GDK_BUTTON_RELEASE;

}

static gboolean    TagEvent  (GtkTextTag *texttag,
                       GObject *arg1,
                       GdkEvent *event,
                       GtkTextIter *arg2,
                       gpointer user_data)
{
  printf("Received tag event signal Tag:%p arg1:%p Event:%p iter:%p user:%p %08X Line:%d\n",
         texttag, arg1,event,arg2, user_data, event->type, gtk_text_iter_get_line(arg2));

  if (isButtonEvent(event->type)) {
    GdkEventButton *evtButton = (GdkEventButton *) event;

    printf("Button Event: button:%d  modifier:%d coords(%g,%g)\n",
           evtButton->button, evtButton->state, evtButton->x,evtButton->y);
    // If the right mouse button is pressed then suppress the GTK pop up menu.
    if (evtButton->button == 3)
      return TRUE;
  }
  return FALSE;
}
//------------------------------------------------------------------------

static void preferences_AddFontSelect(GtkWidget *pParent, const char *fontDescription,
                                      const char *fontName  )
{
}
#endif


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
#if 1
    label = gtk_label_new("file2.asm");
    GtkWidget *emptyBox = gtk_hbox_new(0,0);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_Notebook),emptyBox,label);

#endif



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
  GtkWidget *button;

  LocalWindow = gtk_dialog_new ();
  window = LocalWindow;
  gtk_widget_show (window);

  gtk_window_set_title (GTK_WINDOW (window), "Source Browser configuration");
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);

  GtkWidget *vbox = GTK_DIALOG (window)->vbox;

  m_SourceBrowser = new SourceBrowserPreferences(vbox);
  gtk_widget_show_all(vbox);


  // Cancel and Apply buttons

  GtkWidget *buttonBox = gtk_hbutton_box_new();
  gtk_box_pack_start (GTK_BOX (vbox), buttonBox, TRUE, TRUE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_container_set_border_width (GTK_CONTAINER (button), 10);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             GTK_SIGNAL_FUNC(gpsimGuiPreferences::cancel_cb),
                             this);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

  gtk_box_pack_start (GTK_BOX (buttonBox), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);

  button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
  gtk_container_set_border_width (GTK_CONTAINER (button), 10);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                             GTK_SIGNAL_FUNC(gpsimGuiPreferences::apply_cb),
                             this);

  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (buttonBox), button, TRUE, TRUE, 0);
  gtk_widget_show_all(buttonBox);


  gtk_widget_show (window);

}

gpsimGuiPreferences::~gpsimGuiPreferences()
{


  gtk_widget_destroy (window);

  //delete m_SourceBrowser;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

#endif //if defined(NEW_SOURCE_BROWSER)

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

//========================================================================
void
file_selection_hide_fileops (GtkWidget *widget,
                             GtkFileSelection *fs)
{
  gtk_file_selection_hide_fileop_buttons (fs);
}

extern int gui_message(const char *message);

void
file_selection_ok (GtkWidget        *w,
                   GtkFileSelection *fs)
{

  const char *file;
  char msg[200];

  if(gpGuiProcessor)
  {
    file=gtk_file_selection_get_filename (fs);
    if(!gpsim_open(gpGuiProcessor->cpu, file, 0, 0))
    {
      sprintf(msg, "Open failedCould not open \"%s\"", (char *)file);
      gui_message(msg);
    }
  }
  gtk_widget_hide (GTK_WIDGET (fs));
}

extern int gui_question(char *question, char *a, char *b);

static GtkItemFactoryCallback
fileopen_dialog(gpointer             callback_data,
              guint                callback_action,
              GtkWidget           *widget)
{
  static GtkWidget *window = 0;
  GtkWidget *button;

  if (!window)
  {

    window = gtk_file_selection_new ("file selection dialog");

    gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (window));

    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
                        "clicked", GTK_SIGNAL_FUNC(file_selection_ok),
                        window);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
                               "clicked", GTK_SIGNAL_FUNC(gtk_widget_hide),
                               GTK_OBJECT (window));

    button = gtk_button_new_with_label ("Hide Fileops");
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        (GtkSignalFunc) file_selection_hide_fileops,
                        (gpointer) window);
    gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (window)->action_area),
                        button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    button = gtk_button_new_with_label ("Show Fileops");
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
                        (GtkSignalFunc) gtk_file_selection_show_fileop_buttons,
                        (gpointer) window);
    gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (window)->action_area),
                        button, FALSE, FALSE, 0);
    gtk_widget_show (button);
  }
    gtk_widget_show (window);
    return 0;
}




// Menuhandler for Windows menu buttons
static GtkItemFactoryCallback
toggle_window (gpointer             callback_data,
              guint                callback_action,
              GtkWidget           *widget)
{
  GtkWidget *menu_item = 0;

  menu_item = gtk_item_factory_get_item (item_factory,
                                         gtk_item_factory_path_from_widget (widget));
  if(gpGuiProcessor && menu_item) {

    int view_state =  GTK_CHECK_MENU_ITEM(menu_item)->active ? VIEW_SHOW : VIEW_HIDE;

    switch(callback_action) {
    case WT_opcode_source_window:
      gpGuiProcessor->program_memory->ChangeView(view_state);
      break;
    case WT_asm_source_window:
      gpGuiProcessor->source_browser->ChangeView(view_state);
      break;
    case WT_register_window:
      gpGuiProcessor->regwin_ram->ChangeView(view_state);
      break;
    case WT_eeprom_window:
      gpGuiProcessor->regwin_eeprom->ChangeView(view_state);
      break;
    case WT_watch_window:
      gpGuiProcessor->watch_window->ChangeView(view_state);
      break;
    case WT_symbol_window:
      gpGuiProcessor->symbol_window->ChangeView(view_state);
      break;
    case WT_breadboard_window:
      gpGuiProcessor->breadboard_window->ChangeView(view_state);
      break;
    case WT_stack_window:
      gpGuiProcessor->stack_window->ChangeView(view_state);
      break;
    case WT_trace_window:
      gpGuiProcessor->trace_window->ChangeView(view_state);
      break;
    case WT_profile_window:
      gpGuiProcessor->profile_window->ChangeView(view_state);
      break;
    case WT_stopwatch_window:
      gpGuiProcessor->stopwatch_window->ChangeView(view_state);
      break;
    case WT_scope_window:
      gpGuiProcessor->scope_window->ChangeView(view_state);
      //cout << " The Scope is disabled right now\n";
      break;
    default:
      puts("unknown menu action");
    }

  }
  return 0;
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

  gtk_combo_box_append_text (GTK_COMBO_BOX(parent), label);

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
    snprintf(buf,size, "%19.2f us",time_db);
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
    snprintf(buf,size, "%19.3f ms",time_db);
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
    snprintf(buf,size, "%19.3f Sec",time_db);
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
    snprintf(buf,size,"    %02d:%02d:%02d.%02d",hh,mm,ss,cc);
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
    snprintf(buf,size,"0x%016" PRINTF_GINT64_MODIFIER "x",get_cycles().get());
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
    snprintf(buf,size,"%016" PRINTF_GINT64_MODIFIER "d",get_cycles().get());
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

  if( (event->type == GDK_BUTTON_PRESS) ) {// &&  (event->button == 3) ) {

    gtk_menu_popup(GTK_MENU(tw->menu), 0, 0, 0, 0,
                   3, event->time);
    // It looks like we need it to avoid a selection in the entry.
    // For this we tell the entry to stop reporting this event.
    gtk_signal_emit_stop_by_name(GTK_OBJECT(tw->entry),"button_press_event");
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
  gtk_signal_connect(GTK_OBJECT(item),"activate",
                     (GtkSignalFunc) cbTimeFormatActivated,
                     this);
  gtk_widget_show(item);
  gtk_menu_append(GTK_MENU(menu),item);
}

void TimeWidget::Create(GtkWidget *container)
{
  EntryWidget::Create(false);

  gtk_container_add(GTK_CONTAINER(container),entry);

  SetEntryWidth(18);

  menu = gtk_menu_new();
  GtkWidget *item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);


  // Create an entry for each item in the formatter pop up window.

  new TimeMicroSeconds(this,menu);
  new TimeMilliSeconds(this,menu);
  new TimeSeconds(this,menu);
  new TimeHHMMSS(this,menu);
  NewFormat(new TimeCyclesHex(this,menu));
  new TimeCyclesDec(this,menu);

  // Associate a callback with the user button-click actions
  gtk_signal_connect(GTK_OBJECT(entry),
                     "button_press_event",
                     (GtkSignalFunc) cbTimeFormatPopup,
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

static GtkItemFactoryCallback
gtk_ifactory_cb (gpointer             callback_data,
                 guint                callback_action,
                 GtkWidget           *widget)
{
  g_message ("\"%s\" is not supported yet.", gtk_item_factory_path_from_widget (widget));
    return 0;
}

#define TOGGLE_WINDOW (GtkItemFactoryCallback)toggle_window

static GtkItemFactoryEntry menu_items[] =
{
  { "/_File",            0,         0,                     0, "<Branch>" },
  { "/File/tearoff1",    0,         (GtkItemFactoryCallback)gtk_ifactory_cb,       0, "<Tearoff>" },
  //{ "/File/_New",        "<control>N", (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  { "/File/_Open",       "<control>O", (GtkItemFactoryCallback)fileopen_dialog,       0 },
  //{ "/File/_Save",       "<control>S", (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  //{ "/File/Save _As...", 0,         (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  { "/File/sep1",        0,         (GtkItemFactoryCallback)gtk_ifactory_cb,       0, "<Separator>" },
  { "/File/_Quit",       "<control>Q", (GtkItemFactoryCallback)do_quit_app,         0 },

  //  { "/_Processor",           0,       0,               0, "<Branch>" },
  //  { "/_Processor/New",   0,       (GtkItemFactoryCallback)new_processor_dialog,       0 },
  //  { "/_Processor/Delete",0,       (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  //  { "/_Processor/Switch",0,       (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },


  //  { "/_Break",       0, 0,               0, "<Branch>" },
  //  { "/_Break/Set",       0, (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  //  { "/_Break/Clear",     0, (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },

  { "/_Windows",     0, 0,       0, "<Branch>" },
  { "/Windows/Program _memory", 0, TOGGLE_WINDOW,WT_opcode_source_window,"<ToggleItem>" },
  { "/Windows/_Source",         0, TOGGLE_WINDOW,WT_asm_source_window,"<ToggleItem>" },
  { "/Windows/sep1",            0, (GtkItemFactoryCallback)gtk_ifactory_cb,0,"<Separator>"  },
  { "/Windows/_Ram",            0, TOGGLE_WINDOW,WT_register_window,"<ToggleItem>" },
  { "/Windows/_EEPROM",         0, TOGGLE_WINDOW,WT_eeprom_window,"<ToggleItem>" },
  { "/Windows/_Watch",          0, TOGGLE_WINDOW,WT_watch_window,"<ToggleItem>" },
  { "/Windows/Sta_ck",          0, TOGGLE_WINDOW,WT_stack_window,"<ToggleItem>" },
  { "/Windows/sep2",            0, (GtkItemFactoryCallback)gtk_ifactory_cb,0,"<Separator>"  },
  { "/Windows/Symbo_ls",        0, TOGGLE_WINDOW,WT_symbol_window,"<ToggleItem>" },
  { "/Windows/_Breadboard",     0, TOGGLE_WINDOW,WT_breadboard_window,"<ToggleItem>" },
  { "/Windows/sep3",            0, (GtkItemFactoryCallback)gtk_ifactory_cb,0,"<Separator>"  },
  { "/Windows/_Trace",          0, TOGGLE_WINDOW,WT_trace_window,"<ToggleItem>" },
  { "/Windows/Pro_file",        0, TOGGLE_WINDOW,WT_profile_window,"<ToggleItem>" },
  { "/Windows/St_opwatch",      0, TOGGLE_WINDOW,WT_stopwatch_window,"<ToggleItem>" },
  { "/Windows/Sco_pe",          0, TOGGLE_WINDOW,WT_scope_window,"<ToggleItem>" },

#if defined(NEW_SOURCE_BROWSER)
  { "/_Edit",     0, 0,       0, "<Branch>" },
  { "/Edit/Preferences",        0, (GtkItemFactoryCallback)gpsimGuiPreferences::setup, 0 },
#endif

  { "/_Help",            0,         0,                     0, "<LastBranch>" },
  { "/Help/_About",      0,         (GtkItemFactoryCallback)about_cb,       0 },

};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


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
{
  timeW=0;
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
  GtkAccelGroup *accel_group;

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
  gtk_window_set_default_size(GTK_WINDOW(dispatcher_window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(dispatcher_window),x,y);


  gtk_signal_connect (GTK_OBJECT (dispatcher_window), "delete-event",
                      GTK_SIGNAL_FUNC (dispatcher_delete_event),
                      0);

#if GTK_MAJOR_VERSION >= 2
  accel_group = gtk_accel_group_new();
#else
  accel_group = gtk_accel_group_get_default ();
#endif
  item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
  gtk_object_set_data_full (GTK_OBJECT (dispatcher_window),
                            "<main>",
                            item_factory,
                            (GtkDestroyNotify) gtk_object_unref);
  //      gtk_accel_group_attach (accel_group, GTK_OBJECT (dispatcher_window));
  gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, 0);
  gtk_window_set_title (GTK_WINDOW (dispatcher_window),
                        VERSION);
  gtk_container_set_border_width (GTK_CONTAINER (dispatcher_window), 0);

  box1 = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dispatcher_window), box1);

  gtk_box_pack_start (GTK_BOX (box1),
                      gtk_item_factory_get_widget (item_factory, "<main>"),
                      FALSE, FALSE, 0);



  buttonbox = gtk_hbox_new(FALSE,0);
  gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 1);
  gtk_box_pack_start (GTK_BOX (box1), buttonbox, TRUE, TRUE, 0);



  // Buttons
  button = gtk_button_new_with_label ("step");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc) stepbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("over");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc) overbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("finish");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc) finishbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("run");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc) runbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("stop");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc) stopbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  button = gtk_button_new_with_label ("reset");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc) resetbutton_cb, 0);
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

  update_rate_menu = gtk_combo_box_new_text();
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

  gtk_signal_connect(GTK_OBJECT(update_rate_menu),"changed",
                     (GtkSignalFunc) gui_update_cb,
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
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
                     (GtkSignalFunc) do_quit_app, 0);

  gtk_box_pack_start (GTK_BOX (box1), button, FALSE, TRUE, 5);
  gtk_widget_show_all (dispatcher_window);


}

//========================================================================

void create_dispatcher ()
{
  TheWindow.Create();
}

#endif // HAVE_GUI
