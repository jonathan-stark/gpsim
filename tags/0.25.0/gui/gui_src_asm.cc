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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "../config.h"


#ifdef HAVE_GUI


#include <unistd.h>
#define GTK_ENABLE_BROKEN
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>
#include <ctype.h>
#include <map>

#include <gtkextra/gtkbordercombo.h>
#include <gtkextra/gtkcolorcombo.h>
#include <gtkextra/gtksheet.h>

#include "gui.h"
#include "gui_src.h"
#include "gui_profile.h"
#include "gui_symbols.h"
#include "gui_statusbar.h"
#include "gui_watch.h"

#include <assert.h>

#include "../src/fopen-path.h"
#include "../xpms/pc.xpm"
#include "../xpms/break.xpm"
#include "../xpms/canbreak.xpm"
#include "../xpms/startp.xpm"
#include "../xpms/stopp.xpm"


#define PAGE_BORDER 3
#define PIXMAP_SIZE 14

extern int gui_question(const char *question, const char *a, const char *b);

static int load_fonts(SOURCE_WINDOW *sbaw);


static void find_char_and_skip(char **str, char c)
{
  char *res = strrchr(*str,c);
  if(res) {
    *str = ++res;
  }
}
//------------------------------------------------------------------------
//
static char *strReverse(const char *start, char *dest, int nChars)
{
  *dest-- = 0;
  while (nChars--)
    *dest-- = *start++;
  dest++;
  return dest;
}

//========================================================================

class SourceXREF : public CrossReferenceToGUI
{
public:

  void Update(int new_value)
  {
    SOURCE_WINDOW *sbaw = (SOURCE_WINDOW*)(parent_window);

    if(sbaw->bSourceLoaded())
      sbaw->SetPC(new_value);
  }

  void Remove(void) {}
};



#define BP_PIXEL_SIZE        10
#define PC_PIXEL_SIZE        10
#define MARGIN_WIDTH    (PC_PIXEL_SIZE + BP_PIXEL_SIZE)
#define PC_START  (MARGIN_WIDTH - PC_PIXEL_SIZE)
#define BP_START  (MARGIN_WIDTH - PC_PIXEL_SIZE-BP_PIXEL_SIZE)



static map<GtkTextView*, NSourcePage *> PageMap;



/* This function is taken from gtk+/tests/testtext.c */
static void
gtk_source_view_get_lines (GtkTextView  *text_view,
                           gint          first_y,
                           gint          last_y,
                           GArray       *buffer_coords,
                           GArray       *numbers,
                           gint         *countp)
{
  GtkTextIter iter;
  gint count;
  gint size;
  gint last_line_num = -1;

  g_array_set_size (buffer_coords, 0);
  g_array_set_size (numbers, 0);

  /* Get iter at first y */
  gtk_text_view_get_line_at_y (text_view, &iter, first_y, NULL);

  /* For each iter, get its location and add it to the arrays.
  * Stop when we pass last_y
  */
  count = 0;
  size = 0;

  while (!gtk_text_iter_is_end (&iter))
  {
    gint y, height;

    gtk_text_view_get_line_yrange (text_view, &iter, &y, &height);

    g_array_append_val (buffer_coords, y);
    last_line_num = gtk_text_iter_get_line (&iter);
    g_array_append_val (numbers, last_line_num);

    ++count;

    if ((y + height) >= last_y)
      break;

    gtk_text_iter_forward_line (&iter);
  }

  if (gtk_text_iter_is_end (&iter))
  {
    gint y, height;
    gint line_num;

    gtk_text_view_get_line_yrange (text_view, &iter, &y, &height);

    line_num = gtk_text_iter_get_line (&iter);

    if (line_num != last_line_num)
    {
      g_array_append_val (buffer_coords, y);
      g_array_append_val (numbers, line_num);
      ++count;
    }
  }

  *countp = count;
}


//------------------------------------------------------------------------
//
gint
NSourcePage::KeyPressHandler(GtkTextView *pView,
               GdkEventKey *key,
               SourceWindow *pSW)
{
  GtkTextBuffer *pBuffer = gtk_text_view_get_buffer(pView);
  GtkTextMark *pMark = gtk_text_buffer_get_insert(pBuffer);
  GtkTextIter iter;
  gtk_text_buffer_get_iter_at_mark (pBuffer, &iter, pMark);
  int line = gtk_text_iter_get_line (&iter);

  NSourcePage *page = PageMap[pView];

  Dprintf(("Received key press for view. line=%d page%p\n",line,page));

  switch (key->keyval) {
  case 'b':
  case 'B':
    pSW->toggleBreak(page,line);
    break;
  default:
    return FALSE;
  }

  return TRUE;
}

gint
NSourcePage::ViewExposeEventHandler(GtkTextView *pView,
            GdkEventExpose *pEvent,
            SourceWindow *pSW)
{

  if (pEvent->window == gtk_text_view_get_window (pView,
    GTK_TEXT_WINDOW_LEFT))
  {
    //gtk_source_view_paint_margin (view, event);
    //event_handled = TRUE;
    Dprintf(("Expose event for view margin %p\n",pSW));


    gint y1 = pEvent->area.y;
    gint y2 = y1 + pEvent->area.height;

    NSourcePage *pPage = PageMap[pView];


    gtk_text_view_window_to_buffer_coords (pView,
      GTK_TEXT_WINDOW_LEFT,
      0,
      y1,
      NULL,
      &y1);

    gtk_text_view_window_to_buffer_coords (pView,
      GTK_TEXT_WINDOW_LEFT,
      0,
      y2,
      NULL,
      &y2);

    pPage->updateMargin(y1,y2);

  }
  else {
    Dprintf(("Expose event for view %p\n",pSW));
  }
  return FALSE;
}
//------------------------------------------------------------------------
//
gint
SourceWindow::KeyPressHandler(GtkWidget *widget,
          GdkEventKey *key,
          SourceWindow *pSW)
{
  if (!pSW || !key)
    return FALSE;

  switch (key->keyval) {
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    pSW->step(key->keyval - '0');
    break;
  case 's':
  case 'S':
  case GDK_F7:
    pSW->step();
    break;
  case 'o':
  case 'O':
  case GDK_F8:
    pSW->step_over();
    break;
  case 'r':
  case 'R':
  case GDK_F9:
    pSW->run();
    break;
  case 'f':
  case 'F':
    pSW->finish();
    break;
  case GDK_Escape:
    pSW->stop();
    break;
  default:
    return FALSE;
  }

  return TRUE;

}

//======================================================================
//
// When a user right-clicks in the source browser, a menu will popup.
// There can only be one menu active at any given time.


//
//  'aPopupMenu' pointer is a local pointer to a GtkMenu.
//

static GtkWidget *aPopupMenu=0;

//
// 'pViewContainingPopup' is a pointer to the GtkTextView that was active
// when the popup menu was opened.
//

static GtkTextView *pViewContainingPopup=0;


typedef enum {
  MENU_FIND_TEXT,
  MENU_FIND_PC,
  MENU_MOVE_PC,
  MENU_RUN_HERE,
  MENU_BP_HERE,
  MENU_SELECT_SYMBOL,
  MENU_STEP,
  MENU_STEP_OVER,
  MENU_RUN,
  MENU_STOP,
  MENU_FINISH,
  MENU_RESET,
  MENU_SETTINGS,
  MENU_PROFILE_START_HERE,
  MENU_PROFILE_STOP_HERE,
  MENU_ADD_TO_WATCH,
} menu_id;


typedef struct _menu_item {
  const char *name;
  menu_id id;
  GtkWidget *item;
} menu_item;

static menu_item menu_items[] = {
  {"Find PC",         MENU_FIND_PC,0},
  {"Run to here",     MENU_RUN_HERE,0},
  {"Move PC here",    MENU_MOVE_PC,0},
  {"Breakpoint here", MENU_BP_HERE,0},
  {"Profile start here", MENU_PROFILE_START_HERE,0},
  {"Profile stop here", MENU_PROFILE_STOP_HERE,0},
  {"Add to watch",    MENU_ADD_TO_WATCH,0},
  {"Find text...",    MENU_FIND_TEXT,0},
  {"Settings...",     MENU_SETTINGS,0},
};

static menu_item submenu_items[] = {
  {"Step",            MENU_STEP,0},
  {"Step Over",       MENU_STEP_OVER,0},
  {"Run",             MENU_RUN,0},
  {"Stop",            MENU_STOP,0},
  {"Reset",           MENU_RESET,0},
  {"Finish",          MENU_FINISH,0},
};


//------------------------------------------------------------------------
// ButtonPressHandler
// Event handler for text view mouse clicks.
gint
NSourcePage::ButtonPressHandler(GtkTextView *pView,
                  GdkEventButton *pButton,
                  SourceWindow *pSW)
{
  if (pButton->window == gtk_text_view_get_window (pView,
    GTK_TEXT_WINDOW_LEFT))
  {
    // Margin
    NSourcePage *pPage = PageMap[pView];
    gint x = (gint) pButton->x;
    gint y = (gint) pButton->y;

    gtk_text_view_window_to_buffer_coords (pView,
      GTK_TEXT_WINDOW_LEFT,
      x,
      y,
      &x,
      &y);
    GtkTextIter iter;
    gint line;
    gtk_text_view_get_line_at_y (pView, &iter, y, NULL);
    line = gtk_text_iter_get_line (&iter);
    pSW->toggleBreak(pPage, line);

  } else {
    // Text (i.e. not the margin
    if (pButton->button == 3) {

      if (aPopupMenu) {
        if (GTK_IS_TEXT_VIEW(pView))
          pViewContainingPopup = pView;

        gtk_menu_popup(GTK_MENU(aPopupMenu), 0, 0, 0, 0,
          3, pButton->time);
      }

      return TRUE;
    }

  }

  return FALSE;
}

//========================================================================
int SourceWindow::DeleteEventHandler(GtkWidget *widget,
                        GdkEvent  *event,
                        SourceWindow *sw)
{
  sw->ChangeView(VIEW_HIDE);
  return TRUE;
}


//========================================================================
// Helper functions for parsing
static int isString(const char *cP)
{
  int i=0;

  if (isalpha(*cP) || *cP=='_')
    while (isalnum(cP[i]) || cP[i]=='_')
      i++;
  return i;
}

static int isWhiteSpace(const char *cP)
{
  int i=0;

  while (cP[i]==' ' || cP[i]=='\t')
    i++;
  return i;
}

static int isHexNumber(const char *cP)
{
  int i=0;
  if ((*cP == '0' && toupper(cP[1])=='X') ||
    (*cP == '$')) {
      i = (*cP=='0') ? 2 : 1;

      while (isxdigit(cP[i]))
        i++;
    }
    return i;
}
static int isNumber(const char *cP)
{
  int i=isHexNumber(cP);
  if (!i)
    while (isdigit(cP[i]))
      i++;
  return i;
}

static bool isEnd(const char c)
{
  return c=='\n' || c==0;
}

static int isComment(const char *cP)
{
  int i = (*cP==';') ? 1 : 0;
  if (i)
    while (!isEnd(cP[i]))
      i++;
  return i;
}

//------------------------------------------------------------------------
static bool isButtonEvent (GdkEventType type)
{
  return
    type == GDK_BUTTON_PRESS ||
    type == GDK_2BUTTON_PRESS ||
    type == GDK_3BUTTON_PRESS ||
    type == GDK_BUTTON_RELEASE;

}

//------------------------------------------------------------------------
static gboolean TagEvent (GtkTextTag *texttag,
                          GObject *arg1,
                          GdkEvent *event,
                          GtkTextIter *arg2,
                          TextStyle *pTextStyle)
{
  /*
  static int seq=0;
  printf("Received tag event signal Tag:%p arg1:%p seq %d Event:%p iter:%p user:%p %08X Line:%d\n",
  texttag, arg1,seq++,event,arg2, pTextStyle, event->type, gtk_text_iter_get_line(arg2));
  */
  if (isButtonEvent(event->type)) {
    GdkEventButton *evtButton = (GdkEventButton *) event;

    if (event->type == GDK_2BUTTON_PRESS  && evtButton->button == 1) {
      Dprintf (("Double click left mouse\n"));
      if (pTextStyle)
        pTextStyle->doubleClickEvent(arg2);

      gint signal_id =  g_signal_lookup ("button_press_event",
        G_TYPE_FROM_INSTANCE(arg1));

      GSignalQuery query;
      g_signal_query (signal_id, &query);

      Dprintf (("Signal id=%d name=%s n_params=%d\n",signal_id,query.signal_name,
        query.n_params));

      GtkWidget *toplevel = gtk_widget_get_toplevel (GTK_WIDGET (arg1));
      gboolean b=FALSE;
      if (GTK_WIDGET_TOPLEVEL (toplevel)) {
        //g_signal_emit_by_name (toplevel, "button_press_event",evtButton,&b);
        g_signal_emit_by_name (GTK_WIDGET (arg1), "button_press_event",evtButton,&b);
      } else
        printf("TagEvent: arg1 is not toplevel\n");

    }
    /*
    printf("Button Event: button:%d  modifier:%d coords(%g,%g)\n",
    evtButton->button, evtButton->state, evtButton->x,evtButton->y);
    */
    // If the right mouse button is pressed then suppress the GTK pop up menu.
    if (evtButton->button == 3) {

      if (aPopupMenu) {
        if (GTK_IS_TEXT_VIEW(arg1)) {
          pViewContainingPopup = GTK_TEXT_VIEW(arg1);
          NSourcePage *pPage = PageMap[pViewContainingPopup];
          gint x = (gint) evtButton->x;
          gint y = (gint) evtButton->y;

          gtk_text_view_window_to_buffer_coords (pViewContainingPopup,
            GTK_TEXT_WINDOW_LEFT,
            x,
            y,
            &x,
            &y);
          GtkTextIter iter;
          gtk_text_view_get_line_at_y (pViewContainingPopup, &iter, y, NULL);
          pPage->getParent()->m_LineAtButtonClick =
            gtk_text_iter_get_line (&iter);

        }
        gtk_menu_popup(GTK_MENU(aPopupMenu), 0, 0, 0, 0,
          3, evtButton->time);
      }

      return TRUE;
    }

    return FALSE;
  }
  return FALSE;
}

//------------------------------------------------------------------------
class SearchDialog
{
public:
  SearchDialog();
  void Build();
  void Show(SourceWindow *);
  bool bDirection();
  bool bCase();
protected:
  bool m_bIsBuilt;
  bool m_bFound;
  bool m_bLooped;
  int  m_iStart;
  int  m_iLast;
  int  m_iLastID;

  GtkWidget   *m_Window;           // The Search Dialog Window
  GtkWidget   *m_Entry;            // Widget that holds the search text.
  GtkWidget   *m_BackButton;       //
  GtkWidget   *m_CaseButton;       //
  GList       *m_comboStrings;     //


  SourceWindow *m_pSourceWindow; // The last source window that requested a search.

  static void find_cb(GtkWidget *w, SearchDialog *);
  static void find_clear_cb(GtkWidget *w, SearchDialog *pSearchDialog);
  void find(const char *);
  static gint configure_event(GtkWidget *widget, GdkEventConfigure *e, gpointer data);
};

//------------------------------------------------------------------------
SearchDialog::SearchDialog()
: m_bIsBuilt(false), m_bFound(false),
m_bLooped(false),m_iStart(0),m_iLast(0),m_iLastID(0),
m_pSourceWindow(0)
{

}

gint SearchDialog::configure_event(GtkWidget *widget, GdkEventConfigure *e, gpointer data)
{
  static int dlg_x=200, dlg_y=200;

  if(widget->window==0)
    return 0;

  gdk_window_get_root_origin(widget->window,&dlg_x,&dlg_y);
  return 0;
}
bool SearchDialog::bDirection()
{
  return GTK_TOGGLE_BUTTON(m_BackButton)->active == TRUE;
}
bool SearchDialog::bCase()
{
  return GTK_TOGGLE_BUTTON(m_CaseButton)->active == TRUE;
}

void SearchDialog::find_cb(GtkWidget *w, SearchDialog *pSearchDialog)
{

  const char *p=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(pSearchDialog->m_Entry)->entry));
  pSearchDialog->find(p);
}
void SearchDialog::find(const char *cpPattern)
{
  if (m_pSourceWindow)
    m_iStart = m_pSourceWindow->findText(cpPattern,m_iStart,!bDirection(), bCase());
}



void SearchDialog::find_clear_cb(GtkWidget *w, SearchDialog *pSearchDialog)
{
  //gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(searchdlg.entry)->entry),"");
  printf("find_clear_cb\n");
}

//------------------------------------------------------------------------
void SearchDialog::Build()
{
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;

  if (m_bIsBuilt)
    return;

  m_iLastID=-1;  // will reset search

  m_Window = gtk_dialog_new();

  gtk_signal_connect(GTK_OBJECT(m_Window),
    "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
  gtk_signal_connect_object(GTK_OBJECT(m_Window),
    "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),
    GTK_OBJECT(m_Window));

  gtk_window_set_title(GTK_WINDOW(m_Window),"Find");

  hbox = gtk_hbox_new(FALSE,15);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(m_Window)->vbox),hbox,
    FALSE,TRUE,5);
  label = gtk_label_new("Find:");
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox),label,
    FALSE,FALSE,5);
  m_Entry = gtk_combo_new();
  gtk_widget_show(m_Entry);
  gtk_box_pack_start(GTK_BOX(hbox),m_Entry,
    TRUE,TRUE,5);
  gtk_combo_disable_activate(GTK_COMBO(m_Entry));
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(m_Entry)->entry),"activate",
    GTK_SIGNAL_FUNC(find_cb),this);

  hbox = gtk_hbox_new(FALSE,15);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(m_Window)->vbox),hbox,
    FALSE,TRUE,5);
  gtk_widget_show(hbox);
  m_CaseButton = gtk_check_button_new_with_label("Case Sensitive");
  gtk_widget_show(m_CaseButton);
  gtk_box_pack_start(GTK_BOX(hbox),m_CaseButton,
    FALSE,FALSE,5);
  m_BackButton = gtk_check_button_new_with_label("Find Backwards");
  gtk_widget_show(m_BackButton);
  gtk_box_pack_start(GTK_BOX(hbox),m_BackButton,
    FALSE,FALSE,5);

  button = gtk_button_new_with_label("Find");
  gtk_widget_show(button);
  gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(m_Window)->action_area),button);
  gtk_signal_connect(GTK_OBJECT(button),"clicked",
    GTK_SIGNAL_FUNC(find_cb),this);
  GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
  gtk_widget_grab_default(button);

  button = gtk_button_new_with_label("Clear");
  gtk_widget_show(button);
  gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(m_Window)->action_area),button);
  gtk_signal_connect(GTK_OBJECT(button),"clicked",
    GTK_SIGNAL_FUNC(find_clear_cb),this);

  button = gtk_button_new_with_label("Close");
  gtk_widget_show(button);
  gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(m_Window)->action_area),button);
  gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
    GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(m_Window));

  m_bIsBuilt = true;
}

void SearchDialog::Show(SourceWindow *pSourceWindow)
{
  if (!m_bIsBuilt)
    Build();

  m_pSourceWindow = pSourceWindow;
  m_iStart = 0;

  if (m_Window)
    gtk_widget_show(m_Window);
}

//------------------------------------------------------------------------
ColorHolder::ColorHolder (const char *pcColor)
/*  : m_cpCurr(0), m_cpTemp(0) */
{
  if (pcColor) {
    gdk_color_parse(pcColor, &mCurrentColor);
    mSaveColor  = mCurrentColor;
  }

}
bool ColorHolder::set(GdkColor *pNewColor, bool saveOld)
{
  if (!saveOld)
    mSaveColor  = *pNewColor;

  if (!gdk_color_equal(pNewColor,&mCurrentColor)) {
    mCurrentColor = *pNewColor;
    return true;
  }

  return false;
}
char *ColorHolder::get(char *cParr, int size)
{
  if (cParr)
    snprintf(cParr,size,"#%04X%04X%04X",
    mCurrentColor.red,mCurrentColor.green,mCurrentColor.blue);
  return cParr;
}

void ColorHolder::apply()
{
  mSaveColor = mCurrentColor;
}
bool ColorHolder::revert()
{
  return set(&mSaveColor,true);
}
GdkColor *ColorHolder::CurrentColor()
{
  return &mCurrentColor;
}

//------------------------------------------------------------------------
TextStyle::TextStyle (const char *cpName,
                      const char *pFGColor,
                      const char *pBGColor)
                      : mFG(pFGColor), mBG(pBGColor)
{

  m_pTag = gtk_text_tag_new(cpName);

  g_object_set(G_OBJECT (m_pTag),
    "foreground-gdk", mFG.CurrentColor(),
    "background-gdk", mBG.CurrentColor(),NULL);

  g_signal_connect (G_OBJECT (m_pTag), "event",
    GTK_SIGNAL_FUNC(TagEvent),
    this);
}

void TextStyle::setFG(GdkColor *pNewColor)
{
  if (mFG.set(pNewColor,true)) {
    g_object_set(G_OBJECT (m_pTag),
      "foreground-gdk", mFG.CurrentColor(),NULL);
  }
}
//------------------------------------------------------------------------
void TextStyle::doubleClickEvent(GtkTextIter *pIter)
{

}
void TextStyle::apply()
{
  mFG.apply();
  mBG.apply();
}
void TextStyle::revert()
{
  if (mBG.revert())
    g_object_set(G_OBJECT (m_pTag),
    "background-gdk", mBG.CurrentColor(),NULL);

  if (mFG.revert())
    g_object_set(G_OBJECT (m_pTag),
    "foreground-gdk", mFG.CurrentColor(),NULL);

}

//========================================================================
//========================================================================
SourcePageMargin::SourcePageMargin()
: m_bShowLineNumbers(true), m_bShowAddresses(false), m_bShowOpcodes(true)
{
}
//========================================================================
//========================================================================
SourceBuffer::SourceBuffer(GtkTextTagTable *pTagTable, FileContext *pFC,
                           SourceBrowserParent_Window *pParent)

                           :  m_pParent(pParent), m_pFC(pFC), m_SourceFile_t(eUnknownSource),
                           m_bParsed(false)
{

  assert(pTagTable);
  assert(pParent);
  m_buffer = gtk_text_buffer_new (pTagTable);

  assert(m_buffer);

}
//------------------------------------------------------------------------

eSourceFileType SourceBuffer::getSrcType()
{
  return m_SourceFile_t;
}
void SourceBuffer::setSrcType(eSourceFileType new_SrcType)
{
  m_SourceFile_t = new_SrcType;
}


//------------------------------------------------------------------------
// addTagRange(TextStyle *pStyle,int start_index, int end_index)
//
// Addtag range applies the tag state to a range of text in the buffer
// using a given text style (i.e. the style contains a gtkTextTag)

void SourceBuffer::addTagRange(TextStyle *pStyle,
                               int start_index, int end_index)
{

  if (!pStyle)
    return;

  GtkTextIter    start;
  GtkTextIter    end;
  gtk_text_buffer_get_iter_at_offset (m_buffer, &start, start_index);
  gtk_text_buffer_get_iter_at_offset (m_buffer, &end, end_index);

  gtk_text_buffer_apply_tag (m_buffer, pStyle->tag(), &start, &end);
}

//------------------------------------------------------------------------
bool SourceBuffer::IsParsed()
{
  return m_bParsed;
}

//------------------------------------------------------------------------
void SourceBuffer::parse()
{
  if (IsParsed() || !m_pParent)
    return;

  Dprintf(("parsing source buffer %s\n",m_pFC->name().c_str()));
  m_pParent->parseSource(this, m_pFC);
  m_bParsed = true;

}
GtkTextBuffer *SourceBuffer::getBuffer()
{
  parse();
  return m_buffer;
}

//------------------------------------------------------------------------
//
/*
void SourceBuffer::setBreak(int line)
{
Dprintf ((" setBreak line %d\n",line));

GtkTextIter iBegin, iEnd;
gtk_text_buffer_get_iter_at_line_offset
(m_buffer,
&iBegin,
line,
STROFFSET_OF_OPCODE);
gtk_text_buffer_get_iter_at_line_offset
(m_buffer,
&iEnd,
line,
STROFFSET_OF_OPCODE+STRLEN_OF_OPCODE);

gtk_text_buffer_apply_tag (m_buffer,
m_pParent->mBreakpointTag->tag(),
&iBegin,
&iEnd);

}
*/
//------------------------------------------------------------------------
/*
void SourceBuffer::clearBreak(int line)
{
Dprintf ((" clearBreak line %d\n",line));

GtkTextIter iBegin, iEnd;
gtk_text_buffer_get_iter_at_line_offset
(m_buffer,
&iBegin,
line,
STROFFSET_OF_OPCODE);
gtk_text_buffer_get_iter_at_line_offset
(m_buffer,
&iEnd,
line,
STROFFSET_OF_OPCODE+STRLEN_OF_OPCODE);

gtk_text_buffer_remove_tag (m_buffer,
m_pParent->mBreakpointTag->tag(),
&iBegin,
&iEnd);
}
*/


//========================================================================
NSourcePage::NSourcePage(SourceWindow *pParent,
                         SourceBuffer *pBuffer,
                         int file_id,
                         GtkWidget *pContainer)
                         : m_fileid(file_id),
                         m_pBuffer(pBuffer),
                         m_marginWidth(0),
                         m_Parent(pParent),
                         m_cpFont(0),
                         m_pContainer(pContainer),
                         m_view(0)
{
}


GtkTextBuffer *NSourcePage::buffer()
{
  return m_pBuffer ? m_pBuffer->getBuffer() : 0;
}
//------------------------------------------------------------------------
SourceWindow::SourceWindow(GUI_Processor *pgp,
                           SourceBrowserParent_Window *pParent,
                           bool bUseConfig,
                           const char *newName)
                           : GUI_Object (),
                           pma(0),
                           status_bar(0),
                           last_simulation_mode(eSM_INITIAL),
                           m_pParent(pParent)
{
  Dprintf(("Constructor \n"));

  gp = pgp;

  stPSearchDialog=0;
  m_bLoadSource = false;
  m_bSourceLoaded = false;
  m_LineAtButtonClick = -1;

  if (newName)
    set_name(newName);
  else
    set_name("source_browser");

  wc = WC_source;
  wt = WT_SourceWindow;

  m_Notebook = 0;

  mProgramCounter.bIsActive = false;

  pages = new NSourcePage *[SBAW_NRFILES];
  for (int i=0; i<SBAW_NRFILES; i++)
    pages[i] = 0;


  if (bUseConfig) {
    get_config();

    if (enabled)
      Build();
  }

}
//------------------------------------------------------------------------
void SourceWindow::step(int n)
{
  if (pma)
    pma->step(n);
}
//------------------------------------------------------------------------
void SourceWindow::step_over()
{
  if (pma)
    pma->step_over();
}
//------------------------------------------------------------------------
void SourceWindow::stop()
{
  if (pma)
    pma->stop();
}
//------------------------------------------------------------------------
void SourceWindow::run()
{
  // if (pma)
  //  pma->run();
  get_interface().start_simulation();
}
//------------------------------------------------------------------------
void SourceWindow::finish()
{
  if (pma)
    pma->finish();
}

//------------------------------------------------------------------------
void SourceWindow::reset()
{
  if (gp && gp->cpu)
    gp->cpu->reset(POR_RESET);
}

//------------------------------------------------------------------------
void SourceWindow::set_style_colors(const char *fg_color, const char *bg_color, GtkStyle **style)
{
  GdkColor text_fg;
  GdkColor text_bg;

  gdk_color_parse(fg_color, &text_fg);
  gdk_color_parse(bg_color, &text_bg);
  *style = gtk_style_new();
  (*style)->base[GTK_STATE_NORMAL] = text_bg;
  (*style)->fg[GTK_STATE_NORMAL] = text_fg;

}
//------------------------------------------------------------------------
// toggleBreak
//
//
void SourceWindow::toggleBreak(NSourcePage *pPage, int line)
{
  if (pma && pPage) {
    int address = pma->find_address_from_line(pPage->getFC(),line+1);
    if (address >= 0)
      pma->toggle_break_at_address(address);

  }

}
//------------------------------------------------------------------------
// movePC
//
//
void SourceWindow::movePC(int line)
{
}

//------------------------------------------------------------------------
void SourceWindow::findText()
{
  if (!stPSearchDialog)
    stPSearchDialog = new SearchDialog();

  stPSearchDialog->Show(this);
}


//------------------------------------------------------------------------
// strcasestr is a non standard function
//
#if defined _MSC_VER || defined __MINGW32__ || defined __CYGWIN__
char *strcasestr(const char *searchee, const char *lookfor)
{
  if (*searchee == '\0')
  {
    if (*lookfor)
      return NULL;
    return (char *) searchee;
  }

  while (*searchee != '\0')
  {
    size_t i;

    for (i = 0; ; ++i)
    {
      if (lookfor[i] == '\0')
        return (char *) searchee;

      if (tolower(lookfor[i]) != tolower(searchee[i]))
        break;
    }
    searchee++;
  }

  return NULL;
}
#endif

//------------------------------------------------------------------------
// findText
//
//  Search for the pattern 'pText' in the source window.
//  if bDirection is true then search forward.
int SourceWindow::findText(const char *pText, int start, bool bDirection, bool bCase)
{
  if (!pText)
    return 0;

  unsigned int patternLen = strlen(pText);
  char buff[1024];
  patternLen = (patternLen < sizeof(buff)) ? patternLen : sizeof(buff);
  const char *pattern = bDirection ? pText :
  strReverse(pText, &buff[patternLen], patternLen);

  //printf("findText %s view:%p\n",pattern,pViewContainingPopup);

  NSourcePage *pPage = PageMap[pViewContainingPopup];

  if (!pPage)
    return 0;

  GtkTextIter iStart;
  GtkTextIter iEnd;
  int line = 0;
  int offset = 0;

  int totalLines = gtk_text_buffer_get_line_count(pPage->buffer());
  if (!start) {
    if (bDirection) {
      gtk_text_buffer_get_start_iter(pPage->buffer(),
        &iStart);
      gtk_text_buffer_get_iter_at_line(pPage->buffer(),
        &iEnd,
        line+1);
    } else {

      gtk_text_buffer_get_end_iter(pPage->buffer(),
        &iEnd);
      gtk_text_buffer_get_end_iter(pPage->buffer(),
        &iStart);
      gtk_text_iter_backward_line (&iStart);
      line = totalLines-2;
    }

  } else {
    gtk_text_buffer_get_iter_at_offset(pPage->buffer(),
      &iStart,
      start);
    line = gtk_text_iter_get_line (&iStart);

    if (bDirection) {
      if (line >= totalLines) {
        line = 0;
        gtk_text_buffer_get_iter_at_offset(pPage->buffer(),
          &iStart,
          0);
      }
    } else {
      if (line <= 0) {
        line = totalLines-1;
        gtk_text_buffer_get_iter_at_line(pPage->buffer(),
          &iStart,
          line--);
      }
    }

    gtk_text_buffer_get_iter_at_line(pPage->buffer(),
      &iEnd,
      line);

    offset = start - gtk_text_iter_get_offset (&iEnd);

    gtk_text_buffer_get_iter_at_line(pPage->buffer(),
      &iEnd,
      line+1);
  }


  while (totalLines--) {

    const char *str = gtk_text_buffer_get_text(pPage->buffer(),
      &iStart, &iEnd, FALSE);
    unsigned int srcLen = strlen(str);

    const char *cpSource = str;
    char buffer2[1024];
    if (!bDirection) {
      srcLen = (srcLen < sizeof(buffer2)) ? srcLen : sizeof(buffer2);
      cpSource = strReverse(cpSource, &buffer2[srcLen], srcLen);
    }

    const char *pFound = bCase ? strstr(cpSource, pattern) : strcasestr(cpSource, pattern);

    if (pFound) {
      int pos = bDirection ? (pFound - cpSource) : (srcLen - (pFound - cpSource));
      pos += offset;
      //printf("Found %s in %s starting at %s, pos=%d\n",pattern, str, pFound,pos);

      gtk_text_view_scroll_to_iter (pViewContainingPopup,
        &iStart,
        0.0,
        TRUE,
        0.0, 0.3);

      gtk_text_buffer_get_iter_at_line_offset(pPage->buffer(),
        &iStart,
        line, pos);
      gtk_text_buffer_get_iter_at_line_offset(pPage->buffer(),
        &iEnd,
        line,
        pos+ (bDirection ? patternLen : -patternLen));

      gtk_text_buffer_select_range (pPage->buffer(),
        &iStart,
        &iEnd);
      return gtk_text_iter_get_offset(bDirection ? &iEnd : &iStart);
    }

    // Now we'll search whole lines.
    offset = 0;

    if (bDirection) {
      if (gtk_text_iter_forward_line (&iStart)==FALSE)
        return 0;
      gtk_text_iter_forward_line (&iEnd);
      line++;
    } else {
      if (gtk_text_iter_backward_line (&iStart)==FALSE)
        return gtk_text_buffer_get_char_count(pPage->buffer()) - 1;

      gtk_text_iter_backward_line (&iEnd);
      line--;
    }
  }

  printf("Did not find %s\n",pattern);

  return 0;
}
//------------------------------------------------------------------------
gint SourceWindow::cb_notebook_switchpage (GtkNotebook     *notebook,
                                    GtkNotebookPage *page,
                                    guint            page_num,
                                    SourceWindow     *pSW)
{
  return pSW->switch_page_cb(page_num);
}

gint SourceWindow::switch_page_cb(guint newPage)
{
  Dprintf((" Switch page call back-- page=%d\n",newPage));
  if (m_currentPage != newPage) {
    m_currentPage = newPage;

    NSourcePage *pPage = pages[m_currentPage];

    if (!pPage)
      return TRUE;
    if(gp->cpu->files[pPage->m_fileid]->IsHLL())
      pma->set_hll_mode(ProgramMemoryAccess::HLL_MODE);
    else
      pma->set_hll_mode(ProgramMemoryAccess::ASM_MODE);
    pPage->setSource();
    pPage->invalidateView();

  }
  return TRUE;
}
//------------------------------------------------------------------------
//
void NSourcePage::invalidateView()
{
  if (m_view) {
    GdkRectangle vRect;

    vRect.x=0;
    vRect.y=0;
    vRect.width=100;
    vRect.height=100;
    gdk_window_invalidate_rect
      (gtk_text_view_get_window (m_view,
                                 GTK_TEXT_WINDOW_LEFT),
                                 &vRect,
                                 TRUE);
  }

}

static int settings_dialog(SOURCE_WINDOW *sbaw);

//------------------------------------------------------------------------
static Register *findRegister(string text)
{
  Register *pReg = dynamic_cast<Register *>(globalSymbolTable().find(text));
  if (!pReg)
    pReg = dynamic_cast<Register *>(globalSymbolTable().find(text+string("_")));
  if (!pReg) {
    toupper(text);
    pReg = dynamic_cast<Register *>(globalSymbolTable().find(text));
  }
  if (!pReg)
    pReg = dynamic_cast<Register *>(globalSymbolTable().find(text+string("_")));
  return pReg;
}
//------------------------------------------------------------------------

void
SourceWindow::PopupMenuHandler(GtkWidget *widget, gpointer data)
{
  unsigned int address;
  menu_item *item;

  SourceWindow *pSW = 0;
  NSourcePage *pPage = 0;

  // pViewContainingPopup is initialized when the view_button_press()
  // event handler is called. That function also initiates the event
  // that invokes this callback.

  if (!pViewContainingPopup) {
    printf("Warning popup without a textview\n");
  } else {

    pPage = PageMap[pViewContainingPopup];
    pSW = pPage ? pPage->getParent() : 0;
  }

  if (!pSW) {
    printf ("Warning (bug?): popup cannot be associate with any source\n");
    return;
  }

  item = (menu_item *)data;

  switch(item->id) {

  case MENU_SETTINGS:
    settings_dialog(pSW);
    break;
  case MENU_FIND_TEXT:
    pSW->findText();
    break;
  case MENU_FIND_PC:
    address=pSW->pma->get_PC();
    pSW->SetPC(address);
    break;
  case MENU_MOVE_PC:
    if(-1 != pSW->m_LineAtButtonClick) {
      address = pSW->pma->find_closest_address_to_line(
        pPage->m_fileid,pSW->m_LineAtButtonClick + 1);
      if(address!=INVALID_VALUE) {
        pSW->pma->set_PC(address);
        pSW->SetPC(pSW->pma->get_PC());
      }
    }
    break;

  case MENU_RUN_HERE:
    if(-1 != pSW->m_LineAtButtonClick) {
      address = pSW->pma->find_closest_address_to_line(
          pPage->m_fileid,pSW->m_LineAtButtonClick + 1);
      if(address!=INVALID_VALUE) {
        pSW->gp->cpu->run_to_address(address);
        pSW->SetPC(pSW->pma->get_PC());     // RP - update GUI after running
      }
    }
    break;

  case MENU_BP_HERE:
    if(-1 != pSW->m_LineAtButtonClick) {
      pSW->toggleBreak(pPage, pSW->m_LineAtButtonClick);
    }
    break;
  case MENU_PROFILE_START_HERE:
    if(-1 != pSW->m_LineAtButtonClick) {
      address = pSW->pma->find_closest_address_to_line(
        pPage->m_fileid,pSW->m_LineAtButtonClick + 1);

      pSW->gp->profile_window->StartExe(address);
    }
    break;

  case MENU_PROFILE_STOP_HERE:
    if(-1 != pSW->m_LineAtButtonClick) {
      address = pSW->pma->find_closest_address_to_line(
        pPage->m_fileid,pSW->m_LineAtButtonClick + 1);

      pSW->gp->profile_window->StopExe(address);
    }
    break;

  case MENU_SELECT_SYMBOL:
  case MENU_ADD_TO_WATCH: {
    GtkTextBuffer *pBuffer = pPage->buffer();
    GtkTextIter itBegin, itEnd;
    if(gtk_text_buffer_get_selection_bounds(
         pBuffer, &itBegin, &itEnd) ) {
      gchar *text = gtk_text_buffer_get_text(pBuffer, &itBegin, &itEnd, FALSE);
      if(text != 0) {
        TrimWhiteSpaceFromString(text);

        if(text[0] != 0) {
          //register_symbol *pReg = get_symbol_table().findRegisterSymbol(text);
          /*
          if(pReg == NULL) {
            // We also try upper case.
            string sName(text);
            toupper(sName);
            pReg = get_symbol_table().findRegisterSymbol(sName.c_str());
          }
          if(pReg == NULL) {
            // We also try with a '_' prefix.
            string sName("_");
            sName.append(text);
            pReg = get_symbol_table().findRegisterSymbol(sName.c_str());
            if(pReg == NULL) {
              // We also try upper case.
              toupper(sName);
              pReg = get_symbol_table().findRegisterSymbol(sName.c_str());
            }
          }
          */
          Register *pReg = findRegister(string(text));

          if(!pReg) {
            GtkWidget *dialog = gtk_message_dialog_new( GTK_WINDOW(pSW->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK,
            "The symbol '%s' does not exist as a register symbol.\n"
            "Only register based symbols may be added to the Watch window.",
            text);
            gtk_dialog_run (GTK_DIALOG (dialog));
            gtk_widget_destroy (dialog);
          }
          else {
            pSW->gp->watch_window->Add(pReg);
          }
        }
      }
    }
    }
    break;
  case MENU_STEP:
    if (pSW)
      pSW->step();
    break;
  case MENU_STEP_OVER:
    if (pSW)
      pSW->step_over();
    break;
  case MENU_RUN:
    if (pSW)
      pSW->run();
    break;
  case MENU_STOP:
    if (pSW)
      pSW->stop();
    break;
  case MENU_RESET:
    if (pSW)
      pSW->reset();
    break;
  case MENU_FINISH:
    if (pSW)
      pSW->finish();
    break;
  default:
    puts("Unhandled menuitem?");
    break;
  }

}


//------------------------------------------------------------------------
GtkWidget *
SourceWindow::BuildPopupMenu()
{
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *item;
  unsigned int i;

  menu=gtk_menu_new();
  for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
    item=gtk_menu_item_new_with_label(menu_items[i].name);
    menu_items[i].item=item;
    gtk_signal_connect(GTK_OBJECT(item),"activate",
      (GtkSignalFunc) PopupMenuHandler,
      &menu_items[i]);

    gtk_widget_show(item);
    gtk_menu_append(GTK_MENU(menu),item);
  }

  submenu=gtk_menu_new();
  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (submenu), item);
  gtk_widget_show (item);
  for (i=0; i < (sizeof(submenu_items)/sizeof(submenu_items[0])) ; i++){
    item=gtk_menu_item_new_with_label(submenu_items[i].name);
    submenu_items[i].item=item;
    gtk_signal_connect(GTK_OBJECT(item),"activate",
      (GtkSignalFunc) PopupMenuHandler,
      &submenu_items[i]);

    GTK_WIDGET_SET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);

    gtk_widget_show(item);
    gtk_menu_append(GTK_MENU(submenu),item);
  }
  item = gtk_menu_item_new_with_label ("Controls");
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

  return menu;
}

//------------------------------------------------------------------------
// Build
//
//
void SourceWindow::Build()
{
  char *fontstring;

  Dprintf((" \n"));

  if(bIsBuilt)
    return;

  Dprintf((" \n"));
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  //  get_config();

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");

  g_signal_connect(GTK_OBJECT(window),"key_press_event",
    (GtkSignalFunc) KeyPressHandler,
    (gpointer) this);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
    GTK_SIGNAL_FUNC(DeleteEventHandler),
    (gpointer) this);

  gtk_container_set_border_width (GTK_CONTAINER (window), 0);

  SetTitle();

  GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  m_Notebook = gtk_notebook_new();
  m_currentPage = 0;
  gtk_signal_connect (GTK_OBJECT (m_Notebook), "switch-page",
    GTK_SIGNAL_FUNC(cb_notebook_switchpage),
    (gpointer) this);

  gtk_notebook_set_tab_pos((GtkNotebook*)m_Notebook,m_TabPosition);
  gtk_notebook_set_scrollable ((GtkNotebook*)m_Notebook, TRUE);
  gtk_box_pack_start (GTK_BOX (vbox), m_Notebook, TRUE, TRUE, 0);

  status_bar = new StatusBar_Window();

  if(status_bar) {
    Dprintf ((" creating status bar\n"));
    status_bar->Create(vbox);
  }

  gtk_widget_show_all(window);
  gtk_widget_show_all(vbox);
  gtk_widget_show_all(m_Notebook);


  aPopupMenu = BuildPopupMenu();

  set_style_colors("black", "white", &default_text_style);
  set_style_colors("dark green", "white", &symbol_text_style);
  set_style_colors("orange", "white", &label_text_style);
  set_style_colors("red", "white", &instruction_text_style);
  set_style_colors("blue", "white", &number_text_style);
  set_style_colors("dim gray", "white", &comment_text_style);

#define DEFAULT_COMMENTFONT "-adobe-courier-bold-o-*-*-*-120-*-*-*-*-*-*"
#define DEFAULT_SOURCEFONT "-adobe-courier-bold-r-*-*-*-120-*-*-*-*-*-*"

  if(config_get_string(name(),"commentfont",&fontstring))
    strcpy(commentfont_string,fontstring);
  else
    strcpy(commentfont_string,DEFAULT_COMMENTFONT);

  if(config_get_string(name(),"sourcefont",&fontstring))
    strcpy(sourcefont_string,fontstring);
  else
    strcpy(sourcefont_string,DEFAULT_SOURCEFONT);

  while(!load_fonts(this)) {

    if(gui_question("Some fonts did not load.","Open font dialog","Try defaults")==FALSE)
    {
      strcpy(sourcefont_string,DEFAULT_SOURCEFONT);
      strcpy(commentfont_string,DEFAULT_COMMENTFONT);
      config_set_string(name(),"sourcefont",sourcefont_string);
      config_set_string(name(),"commentfont",commentfont_string);
    }
    else
    {
      settings_dialog(this);
    }
  }

  symbol_font       = gtk_style_get_font(symbol_text_style);
  label_font        = gtk_style_get_font(label_text_style);
  instruction_font  = gtk_style_get_font(instruction_text_style);
  number_font       = gtk_style_get_font(number_text_style);
  comment_font      = gtk_style_get_font(comment_text_style);
  default_font      = gtk_style_get_font(default_text_style);

  bIsBuilt = true;

  if(m_bLoadSource) {
    Dprintf((" \n"));

    NewSource(gp);
  }

}

//------------------------------------------------------------------------
//
void SourceWindow::SetTitle()
{

  if (!gp || !gp->cpu || !pma)
    return;


  if (last_simulation_mode != eSM_INITIAL &&
    ((last_simulation_mode == eSM_RUNNING &&
    gp->cpu->simulation_mode == eSM_RUNNING) ||
    (last_simulation_mode != eSM_RUNNING &&
    gp->cpu->simulation_mode != eSM_RUNNING)) &&
    sLastPmaName == pma->name()) {
      return;
    }

    last_simulation_mode = gp->cpu->simulation_mode;
    const char * sStatus;
    if (gp->cpu->simulation_mode == eSM_RUNNING)
      sStatus = "Run";
    else // if (gp->cpu->simulation_mode == eSM_STOPPED)
      sStatus = "Stopped";
    char buffer[256];
    snprintf(buffer,sizeof(buffer), "Source Browser: [%s] %s", sStatus, pma != NULL ?
      pma->name().c_str() : "" );
    sLastPmaName = pma->name();
    gtk_window_set_title (GTK_WINDOW (window), buffer);

}


//------------------------------------------------------------------------
//
void SourceWindow::set_pma(ProgramMemoryAccess *new_pma)
{
  Dprintf((" \n"));

  pma = new_pma;

  if(window && pma) {

    SetTitle();
  }

  if(status_bar)
    status_bar->NewProcessor(gp, pma);
}

void SourceWindow::Create(void)
{
  Dprintf((" \n"));
}
void SourceWindow::SelectAddress(int address)
{
  Dprintf((" \n"));
}
void SourceWindow::SelectAddress(Value *)
{
  Dprintf((" \n"));
}
//------------------------------------------------------------------------
// Update
//
// Called whenever the source window needs to be updated (like after break points).
void SourceWindow::Update()
{
  Dprintf((" \n"));
  if (!window || !enabled)
    return;

  if (m_Notebook &&
    ((gtk_notebook_get_show_tabs(GTK_NOTEBOOK(m_Notebook))==FALSE
    && m_pParent->getTabPosition()<0) ||
    (m_pParent->getTabPosition() != gtk_notebook_get_tab_pos(GTK_NOTEBOOK(m_Notebook))))) {

      if (m_pParent->getTabPosition()<0) {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_Notebook),FALSE);
      } else {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(m_Notebook),TRUE);
        gtk_notebook_set_tab_pos(GTK_NOTEBOOK(m_Notebook), (GtkPositionType) m_pParent->getTabPosition());
      }
    }

    if (m_Notebook) {
      gint currPage = gtk_notebook_get_current_page (GTK_NOTEBOOK(m_Notebook));

      if (currPage>=0 && currPage < SBAW_NRFILES) {
        pages[currPage]->setSource();
        pages[currPage]->setFont(m_pParent->getFont());
      }
    }

    if(!gp || !pma || ! window)
      return;

    SetTitle();
    SetPC(pma->get_PC());
    if(status_bar)
      status_bar->Update();


}

//------------------------------------------------------------------------
void SourceWindow::UpdateLine(int address)
{
  assert(address>=0);

  Dprintf((" UpdateLine at address=%d\n",address));

  if(!bSourceLoaded() || !pma || !enabled)
    return;

  gint currPage = gtk_notebook_get_current_page (GTK_NOTEBOOK(m_Notebook));
  if (currPage < 0 || currPage > SBAW_NRFILES)
      return;

  NSourcePage *pPage = pages[currPage];

  if (!pPage)
    return;

  pPage->setSource();


  int line = (pPage->getFC()->IsList()) ?
    pma->getFromAddress(address)->get_lst_line() :
  pma->get_src_line(address);

  //int line  = pma->get_src_line(address) - 1;

  line -= 1;

  GtkTextIter iBegin;
  gtk_text_buffer_get_iter_at_line
    (gtk_text_view_get_buffer(pPage->getView()),
    &iBegin,
    line);

  int y, h;

  gtk_text_view_get_line_yrange (pPage->getView(),
    &iBegin,
    &y,
    &h);
  if (pPage->m_marginWidth) {

    GdkRectangle vRect;

    gtk_text_view_buffer_to_window_coords
      (pPage->getView(),
      GTK_TEXT_WINDOW_LEFT,
      0,
      y,
      NULL,
      &y);

    vRect.x=0;
    vRect.y=y;
    vRect.width=pPage->m_marginWidth;
    vRect.height=h;

    Dprintf((" UpdateLine line=%d invalidating region %d,%d  %d,%d\n",line,0,y,vRect.width,h));
    // Send an expose event to repaint the whole margin
    gdk_window_invalidate_rect
      (gtk_text_view_get_window (pPage->getView(), GTK_TEXT_WINDOW_LEFT), &vRect, TRUE);
  }

  return;
}

//------------------------------------------------------------------------
//
int SourceWindow::getPCLine(int page)
{
  if (mProgramCounter.bIsActive && mProgramCounter.page == page)
    return mProgramCounter.line;

  NSourcePage *pPage = pages[page];

  return (pPage->getFC()->IsList()) ?
    pma->getFromAddress(pma->get_PC())->get_lst_line() :
  pma->get_src_line(pma->get_PC());
}
int SourceWindow::getAddress(NSourcePage *pPage, int line)
{
  return pma->find_address_from_line(pPage->getFC(),line);
}
bool SourceWindow::bAddressHasBreak(int address)
{
  return address>=0 && pma->address_has_break(address);
}
int SourceWindow::getOpcode(int address)
{
  return (address >= 0) ? gp->cpu->pma->get_opcode(address) : address;
}
//------------------------------------------------------------------------
bool SourcePageMargin::formatMargin(char *str, int len, int line, int addr, int opcode, bool bBreak)
{
  if (str) {

    int pos = 0;
    int npos = 0;

    *str=0;

    npos = bBreak ? g_snprintf(&str[pos], len, "<span foreground=\"red\"><b>") : 0;
    pos += npos;
    len -= npos;

    npos = m_bShowLineNumbers ? g_snprintf(&str[pos], len, "%d",line) : 0;
    pos += npos;
    len -= npos;

    npos = (m_bShowAddresses && addr >= 0) ? g_snprintf(&str[pos], len, " %04X",addr) : 0;
    pos += npos;
    len -= npos;


    npos = (m_bShowOpcodes && opcode >= 0) ?
      g_snprintf(&str[pos], len, "%c%04X", m_bShowAddresses?':':' ', opcode)
      : 0;
    pos += npos;
    len -= npos;

    pos += bBreak ? g_snprintf(&str[pos], len, "</b></span>") : 0;

    return pos != 0;
  }

  return false;
}
//------------------------------------------------------------------------
SourceWindow *NSourcePage::getParent()
{
  return m_Parent;
}

//------------------------------------------------------------------------
bool NSourcePage::bHasSource()
{
  return m_pBuffer != 0;
}
FileContext * NSourcePage::getFC()
{
  return m_pBuffer ? m_pBuffer->m_pFC : 0;
}

//------------------------------------------------------------------------
GtkTextView *NSourcePage::getView()
{
  setSource();
  return m_view;
}

//------------------------------------------------------------------------
void NSourcePage::setSource()
{
  Dprintf((" \n"));

  if (!m_pBuffer)
    return;
  if (m_view)
    return;
  if (!m_pContainer)
    return;

  Dprintf(("SetSource fileid %d\n",m_fileid));

  m_pBuffer->parse();

  m_view = (GtkTextView *)gtk_text_view_new_with_buffer(m_pBuffer->getBuffer());
  Dprintf(("NSourcePage::setSource() - view=%p\n",m_view));
  gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (m_view),
    GTK_TEXT_WINDOW_LEFT,
    MARGIN_WIDTH);

  PageMap[m_view] = this;

  g_signal_connect(GTK_OBJECT(m_view),"key_press_event",
    (GtkSignalFunc) KeyPressHandler,
    (gpointer) m_Parent);
  g_signal_connect(GTK_OBJECT(m_view),"button_press_event",
    (GtkSignalFunc) ButtonPressHandler,
    (gpointer) m_Parent);
  g_signal_connect(GTK_OBJECT(m_view),"expose_event",
    (GtkSignalFunc) ViewExposeEventHandler,
    (gpointer) m_Parent);

  GtkWidget *pSW = gtk_scrolled_window_new (0,0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pSW),
    GTK_POLICY_AUTOMATIC,
    GTK_POLICY_AUTOMATIC);

  gtk_container_add (GTK_CONTAINER (m_pContainer), pSW);
  gtk_container_add (GTK_CONTAINER (pSW), GTK_WIDGET(m_view));

  gtk_text_view_set_wrap_mode (m_view, GTK_WRAP_NONE);
  gtk_text_view_set_editable  (m_view, FALSE);

  setFont(m_Parent->getFont());

  gtk_widget_show_all(m_pContainer);

}

//------------------------------------------------------------------------
void NSourcePage::updateMargin(int y1, int y2)
{

  Dprintf((" updateMargin y1=%d y2=%d\n",y1,y2));

  GtkTextView * text_view = m_view;
  GArray *numbers;
  GArray *pixels;
  gint count;

  int PCline = m_Parent->getPCLine(m_fileid);

  numbers = g_array_new (FALSE, FALSE, sizeof (gint));
  pixels = g_array_new (FALSE, FALSE, sizeof (gint));

  GdkWindow *win = gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_LEFT);

  if (y1 < 0) {
    gint width;
    y1 = 0;
    gdk_window_get_size(win,&width,&y2);
    Dprintf((" updateMargin updating whole margin y1=%d y2=%d\n",y1,y2));
  }

  /* get the line numbers and y coordinates. */
  gtk_source_view_get_lines (text_view,
    y1,
    y2,
    pixels,
    numbers,
    &count);



  /* set size. */
  gchar str [256];
  PangoLayout *layout=0;
  gint text_width=0;
  FileContext *pFC = getFC();
  gint addr_opcode = (pFC && !pFC->IsList()) ? 0x9999 : -1;
  if ( m_Parent->margin().formatMargin(str, sizeof(str),
    MAX (99, gtk_text_buffer_get_line_count (text_view->buffer)),
    addr_opcode, addr_opcode,false) ) {

      layout = gtk_widget_create_pango_layout (GTK_WIDGET (text_view), str);

      pango_layout_get_pixel_size (layout, &text_width, NULL);
      text_width+=2;
    }

    m_marginWidth = text_width + MARGIN_WIDTH;
    gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (text_view),
      GTK_TEXT_WINDOW_LEFT,
      m_marginWidth);

    int i=0;
    while (i < count) {

      gint pos;
      gint line = g_array_index (numbers, gint, i) + 1;

      gtk_text_view_buffer_to_window_coords (text_view,
        GTK_TEXT_WINDOW_LEFT,
        0,
        g_array_index (pixels, gint, i),
        NULL,
        &pos);

      int address    = pFC && !pFC->IsList() ? m_Parent->getAddress(this,line) : - 1;
      int opcode     = pFC && !pFC->IsList() && !pFC->IsHLL() ? m_Parent->getOpcode(address) : -1;
      bool bHasBreak = m_Parent->bAddressHasBreak(m_Parent->getAddress(this,line));


      if (layout) {

        if ( m_Parent->margin().formatMargin(str, sizeof(str),
          line, address,opcode,bHasBreak)) {

            pango_layout_set_markup (layout, str, -1);

            gtk_paint_layout (GTK_WIDGET (text_view)->style,
              win,
              GTK_STATE_NORMAL,
              FALSE,
              NULL,
              GTK_WIDGET (text_view),
              NULL,
              2, //text_width + 2,
              pos,
              layout);

          }

      }


      if (line == PCline) {
        gtk_paint_arrow
          (GTK_WIDGET (text_view)->style,
          win,
          GTK_STATE_NORMAL,
          GTK_SHADOW_OUT,    // GtkShadowType shadow_type,
          NULL,
          GTK_WIDGET (text_view),
          NULL,
          GTK_ARROW_RIGHT,   //GtkArrowType arrow_type,
          TRUE,              //gboolean fill,
          text_width+PC_START,pos, PC_PIXEL_SIZE,15);
        Dprintf((" updating PC at line %d\n", line));
      }

      if (m_Parent->getAddress(this,line) >= 0) {
        // There is code associated with this line.

        gtk_paint_diamond
          (GTK_WIDGET (text_view)->style,
          win,
          GTK_STATE_NORMAL,
          bHasBreak ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
          NULL,
          GTK_WIDGET (text_view),
          NULL,
          text_width+BP_START,
          pos,
          BP_PIXEL_SIZE,
          BP_PIXEL_SIZE);
      }
      ++i;

    }

    g_array_free (pixels, TRUE);
    g_array_free (numbers, TRUE);

}
//------------------------------------------------------------------------
void NSourcePage::setFont(const char *cp_newFont)
{
  if (m_view && cp_newFont) {

    if (m_cpFont && strcmp(cp_newFont,m_cpFont)==0)
      return;

    g_free(m_cpFont);
    m_cpFont = g_strndup(cp_newFont,256);


    /* Change default font throughout the widget */
    PangoFontDescription *font_desc;
    font_desc = pango_font_description_from_string (m_cpFont);
    gtk_widget_modify_font (GTK_WIDGET (m_view), font_desc);
    pango_font_description_free (font_desc);

  }
}
//------------------------------------------------------------------------
// SetPC
//
// Highlight the line corresponding to the current program counter.
//

void SourceWindow::SetPC(int address)
{
  Dprintf((" \n"));

  if (!bSourceLoaded() || !pma)
    return;

  int currPage = m_Notebook ?
    gtk_notebook_get_current_page (GTK_NOTEBOOK(m_Notebook)) :
  -1;

  if (currPage>=0 && currPage < SBAW_NRFILES)
    pages[currPage]->setSource();

  // Get the file id associated with the program counter address
  unsigned int sbawFileId  = pma->get_file_id(address);
  if(sbawFileId == 0xffffffff)
    return;

  int id = -1;
  int PCline=-1;

  if (currPage>=0  && pages[currPage]->getFC()->IsList()) {
    // Don't automatically switch away from a page if it is a list file
    id = currPage;
    PCline = pma->getFromAddress(address)->get_lst_line();
  } else {
    for (id=0; id<SBAW_NRFILES; id++)
      if(pages[id]->m_fileid == sbawFileId)
        break;

    if (id >= SBAW_NRFILES)
      return;   // page was not found.

    // Switch to the source browser page that contains the program counter.
    if (currPage != id)
      gtk_notebook_set_page(GTK_NOTEBOOK(m_Notebook),id);

    // Get the source line number associated with the program counter address.
    PCline = pma->get_src_line(address);
    if(PCline==(int)INVALID_VALUE)
      return;
    //PCline--;
  }

  int oldPCpage = -1;
  bool bFirstUpdate=true;
  if (mProgramCounter.bIsActive) {
    oldPCpage = mProgramCounter.page;
    bFirstUpdate=false;
  } else
    GTKWAIT;

  mProgramCounter.page = id;
  mProgramCounter.line = PCline;

  // Get a pointer to text_view margin window.
  GdkWindow *win = gtk_text_view_get_window (pages[id]->getView(), GTK_TEXT_WINDOW_LEFT);
  GdkRectangle PCloc;

  mProgramCounter.bIsActive = true;
  mProgramCounter.pBuffer = pages[id]->buffer();
  gtk_text_buffer_get_iter_at_line(mProgramCounter.pBuffer,
    &mProgramCounter.iBegin,
    PCline);

  // Now we're going to check if the program counter is in view or not.

  // Get the program counter location
  gtk_text_view_get_iter_location (pages[id]->getView(),
    &mProgramCounter.iBegin,
    &PCloc);
  // Get the viewable region of the text buffer. The region is in buffer coordinates.
  GdkRectangle vRect;
  gtk_text_view_get_visible_rect  (pages[id]->getView(),
    &vRect);

  // Now normalize the program counter's location. If yloc is between
  // 0 and 1.0 then the program counter is viewable. If not, then we
  // we need to scroll the text view so that the program counter is
  // viewable.
  double yloc = (PCloc.y - vRect.y) / (double) (vRect.height);

  if ( yloc < 0.05  || yloc > 0.95 || bFirstUpdate) {
    gtk_text_view_scroll_to_iter (pages[id]->getView(),
      &mProgramCounter.iBegin,
      0.0,
      TRUE,
      0.0, 0.3);
    gtk_text_view_get_visible_rect  (pages[id]->getView(),
      &vRect);

  }

  // If there is a margin, then invalidate it so gtk will go off and redraw it.
  if (pages[id]->m_marginWidth) {
    vRect.x=0;
    vRect.y=0;
    vRect.width=pages[id]->m_marginWidth;
    // Send an expose event to repaint the whole margin
    gdk_window_invalidate_rect
      (win, &vRect, TRUE);
  }
}

void SourceWindow::CloseSource(void)
{
  Dprintf((" \n"));
}

SourcePageMargin &SourceWindow::margin()
{
  return m_pParent->margin();
}
const char *SourceWindow::getFont()
{
  return m_pParent->getFont();
}

void SourceWindow::NewSource(GUI_Processor *gp)
{
  Dprintf((" \n"));

  int i;

  unsigned int address;

  if(!gp || !gp->cpu || !gp->cpu->pma)
    return;
  Dprintf((" \n"));

  Processor * pProc = gp->cpu;
  if(!enabled)
  {
    m_bLoadSource=true;
    return;
  }
  Dprintf((" \n"));

  if(!pma)
    pma = pProc->pma;

  assert(wt==WT_SourceWindow);

  CloseSource();

  m_bLoadSource=true;

  Dprintf(("NewSource\n"));

  /* Now create a cross-reference link that the
  * simulator can use to send information back to the gui
  */
  if(pProc->pc) {
    SourceXREF *cross_reference = new SourceXREF();

    cross_reference->parent_window_type =   WT_asm_source_window;
    cross_reference->parent_window = (gpointer) this;
    cross_reference->data = (gpointer) 0;

    pProc->pc->add_xref((gpointer) cross_reference);
    if(pProc->pc != pma->GetProgramCounter()) {
      pma->GetProgramCounter()->add_xref((gpointer) cross_reference);
    }
  }


  i=0;
  while (m_pParent->ppSourceBuffers[i]) {

    AddPage(m_pParent->ppSourceBuffers[i]);
    i++;
  }

  m_bSourceLoaded = 1;


  // update breakpoint widgets
  unsigned uPMMaxIndex = pProc->program_memory_size();
  for(unsigned int uPMIndex=0; uPMIndex < uPMMaxIndex; uPMIndex++) {
    int address = pProc->map_pm_index2address(uPMIndex);
    if(pma->address_has_break(address))
      UpdateLine(address);
  }

  address=pProc->pma->get_PC();
  if(address==INVALID_VALUE)
    puts("Warning, PC is invalid?");
  else
    SetPC(address);

  Dprintf((" Source is loaded\n"));

}



//------------------------------------------------------------------------
// AddPage
// Adds a page to the notebook, and returns notebook-id for that page.
//
int SourceWindow::AddPage(SourceBuffer *pSourceBuffer)
{
  if (pSourceBuffer && pSourceBuffer->m_pFC)
    return AddPage(pSourceBuffer,  pSourceBuffer->m_pFC->name().c_str());
  return -1;
}

int SourceWindow::AddPage(SourceBuffer *pSourceBuffer, const char *fName)
{

  if (!bIsBuilt || !pSourceBuffer)
    return -1;

  GTKWAIT;

  char str[256], *label_string;
  GtkWidget *label;

  strncpy(str,fName,sizeof(str));

  label_string=str;

  find_char_and_skip(&label_string,'/');
  find_char_and_skip(&label_string,'\\');

  label=gtk_label_new(label_string);

  GtkWidget *pFrame = gtk_frame_new(NULL);

  gtk_notebook_append_page(GTK_NOTEBOOK(m_Notebook),pFrame,label);

  int id = gtk_notebook_page_num(GTK_NOTEBOOK(m_Notebook),pFrame);

  assert(id<SBAW_NRFILES && id >=0);

  NSourcePage *page = new NSourcePage(this, pSourceBuffer, id,pFrame);

  pages[id] = page;

  //page->setSource();

  gtk_widget_show_all(pFrame);

  return id;

}



//########################################################################
//
// Everything below is in the process of being deprecated...
//
//########################################################################

//#define PIXMAP_POS(sbaw,e) ((e)->pixel+(sbaw)->layout_offset+-PIXMAP_SIZE/2-(e)->font_center)

static int file_id_to_source_mode[100];

// this should be in SourceBrowserAsm struct FIXME
static struct {
  int found;                   //
  int looped;                  // if search hit start or end of text
  int start;                   //
  int lastfound;               // index into text for start of last found string
  int i;                       //
  int lastid;                  //
  GtkWidget *window;           // the window for find dialog
  GtkWidget *entry;            // string GtkCombo
  GtkWidget *backwards_button; // togglebutton for direction
  GtkWidget *case_button;      // togglebutton for case sensitivity
  GList *combo_strings;        // list of strings for combo
  char *string;                // current string, extracted from entry
} searchdlg = {0,0,-1,0,0,0,0,0,0,0};

static int dlg_x=200, dlg_y=200;

//========================================================================
void PixmapObject::CreateFromXPM(GdkWindow *window,
                                 GdkColor *transparent_color,
                                 gchar **xpm)
{
  pixmap = gdk_pixmap_create_from_xpm_d(window,
    &mask,
    transparent_color,
    (gchar**)xpm);
  widget = gtk_pixmap_new(pixmap,mask);

}

//========================================================================
BreakPointInfo::BreakPointInfo(int _address, int _line, int _index, int _pos)
: address(_address), pos(_pos), index(_index), line(_line),
break_widget(0), canbreak_widget(0)
{
}

BreakPointInfo::BreakPointInfo(BreakPointInfo & Dup)
{
  address = Dup.address;
  index = Dup.index;
  line = Dup.line;
  pos = Dup.pos;
  break_widget = 0;
  canbreak_widget = 0;
}

BreakPointInfo::~BreakPointInfo()
{
  if (break_widget)
    gtk_widget_destroy (break_widget);
  if (canbreak_widget)
    gtk_widget_destroy (canbreak_widget);

}
void BreakPointInfo::Set(GtkWidget *layout, GdkPixmap *pixmap_break, GdkBitmap *bp_mask)
{
  if(!break_widget) {
    break_widget = gtk_pixmap_new(pixmap_break,bp_mask);

    gtk_layout_put(GTK_LAYOUT(layout),
      break_widget,
      PIXMAP_SIZE*0,
      pos
      );
  }
  if(canbreak_widget) {
    gtk_widget_hide(canbreak_widget);
  }

  gtk_widget_show(break_widget);

}

void BreakPointInfo::Clear(GtkWidget *layout, GdkPixmap *pixmap_canbreak, GdkBitmap *bp_mask)
{

  if(break_widget) {
    gtk_widget_hide(break_widget);
  }
  if(!canbreak_widget) {
    canbreak_widget = gtk_pixmap_new(pixmap_canbreak,bp_mask);

    gtk_layout_put(GTK_LAYOUT(layout),
      canbreak_widget,
      PIXMAP_SIZE*0,
      pos
      );
  }

  gtk_widget_show(canbreak_widget);

}

void BreakPointInfo::setBreakWidget(GtkWidget *pBreak)
{
  assert (break_widget==0);
  break_widget=pBreak;
}
void BreakPointInfo::setCanBreakWidget(GtkWidget *pCanBreak)
{
  assert (canbreak_widget==0);
  canbreak_widget=pCanBreak;
}
//========================================================================

// all of these gui_xxxx_to_entry() do linear search.
// Binary search is possible, the list is sorted.
// pixel is 0 -> maxfont-1 for line zero.
// maxfont -> maxfont*2-1 for line one
//         ...
BreakPointInfo *SourceBrowserAsm_Window::getBPatPixel(int id, int pixel)
{

  BreakPointInfo *e; // to simplify expressions
  GList *p;          // iterator

  if(!sa_xlate_list[id])
    return 0;

  if(pixel<0)
    return (BreakPointInfo*)sa_xlate_list[id]->data;

  p=sa_xlate_list[id];

  // find listentry with address larger than argument
  while(p->next!=0) {

    e = (BreakPointInfo*)p->data;
    if(e->pos+12 > pixel)
      break;
    p=p->next;
  }

  e=(BreakPointInfo*)p->data;

  return e;
}

BreakPointInfo *SourceBrowserAsm_Window::getBPatLine(int id, unsigned int line)
{
  BreakPointInfo *e;
  GList *p;

  if(!sa_xlate_list[id])
    return 0;

  if(line>0xffff0000)
    return 0;

  p=sa_xlate_list[id];

  /*
  locate listentry with index larger than argument
  */

  while(p->next!=0)
  {
    e = (BreakPointInfo*)p->data;

    if(e->getLine() > (int)line)
      break;
    p=p->next;
  }

  assert(p->prev); // FIXME, happens if only one line of source
  p=p->prev;

  e=(BreakPointInfo*)p->data;
  return e;
}

BreakPointInfo *SourceBrowserAsm_Window::getBPatIndex(int id, unsigned int index)
{
  BreakPointInfo *e;
  GList *p;

  if(!sa_xlate_list[id] || index<0)
    return 0;

  p=sa_xlate_list[id];

  /*
  locate listentry with index larger than argument
  */
  while(p->next!=0)
  {
    e = (BreakPointInfo*)p->data;

    if(e->index > index)
      break;
    p=p->next;
  }

  assert(p->prev); // FIXME, could happen

  p=p->prev;

  e=(BreakPointInfo*)p->data;
  return e;
}

class CFormattedTextFragment : public gpsimObject {
public:
  CFormattedTextFragment(const char *pFragment, int length,
    GtkStyle *pStyle, GdkFont *font);

  GdkFont *   m_font;
  GtkStyle *  m_text_style;
  static int  s_lineascent;
  static int  s_linedescent;
  int         m_length;
  string &    m_Fragment;
  int         s_TotalLength;

  void SetText(GtkText *source_text);
  int  GetLength();

};

int  CFormattedTextFragment::s_lineascent =-1;
int  CFormattedTextFragment::s_linedescent = -1;


CFormattedTextFragment::CFormattedTextFragment(
  const char *pFragment, int length,
  GtkStyle *pStyle, GdkFont *font)
  : m_Fragment(name_str)
{
  s_TotalLength = 0;
  m_text_style  = pStyle;
  m_font = font;
  // Capture the largest line ascent and descent for
  // use in BreakPointInfo.
  if (s_lineascent < m_font->ascent)
    s_lineascent = m_font->ascent;
  if (s_linedescent < m_font->descent)
    s_linedescent = m_font->descent;
  if(length>0)
    m_Fragment.assign(pFragment, length);
  else
    m_Fragment.assign(pFragment);
  m_length      = length;
}

void CFormattedTextFragment::SetText(GtkText *source_text)
{
  gtk_text_insert(source_text,
    m_font,
    &m_text_style->fg[GTK_STATE_NORMAL],
    &m_text_style->base[GTK_STATE_NORMAL],
    m_Fragment.c_str(),
    m_length);
}

void SourceBrowserAsm_Window::DetermineBreakinfos(int id)
{
  GList *iter = sa_xlate_list[id];
  /*
  for(iter=sa_xlate_list[id];iter!=0;)
  {
    GList *next=iter->next;
    free( (BreakPointInfo*)iter->data );
    g_list_remove(iter,iter->data);
    iter=next;
  }
  */
  while (iter) {
    free( (BreakPointInfo*)iter->data );
    iter = g_list_remove(iter,iter->data);
  }
  sa_xlate_list[id]=0;
  for(iter=s_global_sa_xlate_list[id];iter!=0;) {
    GList *next=iter->next;
    BreakPointInfo *pGlobal = (BreakPointInfo*)iter->data;
    BreakPointInfo *entry= new BreakPointInfo(*pGlobal);
    sa_xlate_list[id]=g_list_append(sa_xlate_list[id],entry);
    iter=next;
  }
}

void SourceBrowserAsm_Window::SetPC(int address)
{
  BreakPointInfo *e;
  int row;
  gdouble inc;
  unsigned int i;
  unsigned int sbawFileId;

  GtkWidget *new_pcw;
  int id=-1;

  if(!m_bSourceLoaded)
    return;
  if(!pma)
    return;

  // find notebook page containing address 'address'
  sbawFileId = pma->get_file_id(address);

  if(sbawFileId == 0xffffffff)
    return;

  for(i=0;i<SBAW_NRFILES;i++) {
    if(pages[i].pageindex_to_fileid == sbawFileId)
      id=i;
    else {
      if( pages[i].source_pcwidget!=0 &&
        GTK_WIDGET_VISIBLE(pages[i].source_pcwidget) ) {
          //cout << " SetPC: " << name() << "  hiding page "  << i << endl;
          gtk_widget_hide(pages[i].source_pcwidget);
        }
    }
  }


  if(id==-1) {
    puts("SourceBrowserAsm_set_pc(): could not find notebook page");
    return;
  }

  new_pcw = pages[id].source_pcwidget;

  row = pma->get_src_line(address);

  if(row==(int)INVALID_VALUE)
    return;
  row--;

  if((int)current_page != id)
    gtk_notebook_set_page(GTK_NOTEBOOK(notebook),id);

  if(layout_offset<0)
  {   // can it normally be less than zero?
    // FIXME, this should be done whenever windows are reconfigured.
    int xtext,ytext;
    int xfixed, yfixed;

    if(GTK_TEXT(pages[id].source_text)->text_area!=0 &&
      pages[id].source_layout->window!=0)
    {
      gdk_window_get_origin(GTK_TEXT(pages[id].source_text)->text_area,&xtext,&ytext);
      gdk_window_get_origin(pages[id].source_layout->window,&xfixed,&yfixed);

      layout_offset = ytext-yfixed;
      //cout << " SetPC: " << name() << "  updating layout offset "  << layout_offset << endl;
    }
  }
  e = getBPatLine(id, row);
  if(e==0)
    return;

  inc = GTK_ADJUSTMENT(GTK_TEXT(pages[id].source_text)->vadj)->page_increment;

  if( (unsigned int)e->pos< GTK_TEXT(pages[id].source_text)->first_onscreen_ver_pixel ||
    (unsigned int)e->pos> GTK_TEXT(pages[id].source_text)->first_onscreen_ver_pixel+inc ) {

      GtkAdjustment *adj = GTK_ADJUSTMENT( GTK_TEXT(pages[id].source_text)->vadj);

      gdouble nvalue = e->pos - inc/2;
      //printf("%d: setting adjustment to %g old value = %g\n",__LINE__,nvalue,adj->value);
      gtk_adjustment_set_value(adj, nvalue);

    }

    if(!GTK_WIDGET_VISIBLE(new_pcw)) {
      gtk_widget_show(new_pcw);
    }
    gtk_layout_move(GTK_LAYOUT(pages[id].source_layout),
      new_pcw,
      PIXMAP_SIZE,
      e->pos+1
      );

}

void SourceBrowserAsm_Window::SelectAddress(int address)
{
  BreakPointInfo *e;
  int id=-1, i;
  gdouble inc;
  unsigned int line;

  if(!m_bSourceLoaded)
    return;
  if(!pma)
    return;

  for(i=0;i<SBAW_NRFILES;i++) {
    if(pages[i].pageindex_to_fileid==pma->get_file_id(address))
      id=i;
  }

  if(id==-1)
  {
    puts("SourceBrowserAsm_select_address(): could not find notebook page");
    return;
  }

  gtk_notebook_set_page(GTK_NOTEBOOK(notebook),id);

  line = pma->get_src_line(address);

  if(line==INVALID_VALUE)
    return;

  e = getBPatLine(id, line);
  if(e==0)
    return;

  inc = GTK_ADJUSTMENT(GTK_TEXT(pages[id].source_text)->vadj)->page_increment;

  if( (unsigned int)e->pos <= GTK_TEXT(pages[id].source_text)->first_onscreen_ver_pixel ||
    (unsigned int)e->pos >= GTK_TEXT(pages[id].source_text)->first_onscreen_ver_pixel+inc ) {
      gtk_adjustment_set_value(GTK_ADJUSTMENT( GTK_TEXT( pages[id].source_text)->vadj),
        e->pos-inc/2);
    }
}

void SourceBrowserAsm_Window::SelectAddress(Value *addrSym)
{
  SourceBrowser_Window::SelectAddress(addrSym);
}

void SourceBrowserAsm_Window::Update(void)
{
  if(!gp || !pma || ! window)
    return;

  SetTitle();

  SetPC(pma->get_PC());
  if(status_bar)
    status_bar->Update();
}

/*
this happens when breakpoint is set or unset
( Can it happen for another reason? )
*/
void SourceBrowserAsm_Window::UpdateLine(int address)
{
  unsigned int row;

  int i,id=-1;
  BreakPointInfo *e;

  assert(address>=0);

  if(!m_bSourceLoaded || !pma)
    return;

  for(i=0;i<SBAW_NRFILES && id<0;i++) {
    if(pages[i].pageindex_to_fileid==pma->get_file_id(address))
      id=i;
  }

  if(id==-1)
  {
    static int warned=0;
    if(warned<10) {
      puts("SourceBrowserAsm_update_line(): could not find notebook page");
      warned++;
    }
    return;
  }
  if(id != (int)current_page) {
    return;
  }
  row = pma->get_src_line(address);

  if(row==INVALID_VALUE)
    return;
  row--;


  e = getBPatLine(id,row);

  if(e==0)
    return;
  /*
  printf("SrcBrowserAsm_Window::UpdateLine - address=%d line=%d\n",
  address,e->getLine());
  */
  breakpoints.Remove(address);
  notify_start_list.Remove(address);
  notify_stop_list.Remove(address);


  if(pma->address_has_profile_start(address))
    notify_start_list.Add(address,
    gtk_pixmap_new(pixmap_profile_start,startp_mask),
    pages[id].source_layout,
    e->pos);

  else if(pma->address_has_profile_stop(address))
    notify_stop_list.Add(address,
    gtk_pixmap_new(pixmap_profile_stop,stopp_mask),
    pages[id].source_layout,
    e->pos);

  else if(pma->address_has_break(address)) {
    e->Set(pages[id].source_layout,pixmap_break, bp_mask);
    breakpoints.Add(address,
      gtk_pixmap_new(pixmap_break,bp_mask),
      pages[id].source_layout,
      e->pos);
  } else {
    e->Clear(pages[id].source_layout, pixmap_canbreak, canbp_mask);
  }
}

SourceBrowserAsm_Window *popup_sbaw;

void
SourceBrowserAsm_Window::PopupMenuHandler(GtkWidget *widget, gpointer data)
{
  menu_item *item;
  unsigned int id, address, line;
  char text[256];
  if(!popup_sbaw || !popup_sbaw->gp || !popup_sbaw->gp->cpu || !popup_sbaw->pma)
    return;

  item = (menu_item *)data;
  id = gtk_notebook_get_current_page(GTK_NOTEBOOK(popup_sbaw->notebook));

  switch(item->id) {

  case MENU_SETTINGS:
    break;
  case MENU_FIND_TEXT:
    gtk_widget_set_uposition(GTK_WIDGET(searchdlg.window),dlg_x,dlg_y);
    gtk_widget_show(searchdlg.window);
    break;
  case MENU_FIND_PC:
    address=popup_sbaw->pma->get_PC();
    popup_sbaw->SetPC(address);
    break;
  case MENU_MOVE_PC:
    line = popup_sbaw->menu_data->getLine();

    address = popup_sbaw->pma->find_closest_address_to_line(popup_sbaw->pages[id].pageindex_to_fileid,line+1);
    if(address!=INVALID_VALUE)
      popup_sbaw->pma->set_PC(address);
    break;

  case MENU_RUN_HERE:
    line = popup_sbaw->menu_data->getLine()+1;

    address = popup_sbaw->pma->find_closest_address_to_line(popup_sbaw->pages[id].pageindex_to_fileid,line);

    if(address!=INVALID_VALUE)
      popup_sbaw->gp->cpu->run_to_address(address);
    break;

  case MENU_BP_HERE:
    line = popup_sbaw->menu_data->getLine() + 1;

    popup_sbaw->pma->toggle_break_at_line(popup_sbaw->pages[id].pageindex_to_fileid,line);

    break;
  case MENU_PROFILE_START_HERE:
    /*
    line = popup_sbaw->menu_data->line;
    address = popup_sbaw->pma->find_closest_address_to_line(popup_sbaw->pages[id].pageindex_to_fileid,line+1);

    popup_sbaw->gp->profile_window->StartExe(address);
    */
    break;

  case MENU_PROFILE_STOP_HERE:
    /*
    line = popup_sbaw->menu_data->line;

    address = popup_sbaw->pma->find_closest_address_to_line(popup_sbaw->pages[id].pageindex_to_fileid,line+1);

    popup_sbaw->gp->profile_window->StopExe(address);
    */
    break;

  case MENU_SELECT_SYMBOL:
  case MENU_ADD_TO_WATCH:
    {
      //FIXME replicated code...
      gint i, temp;
      gint start, end;

      if (!gtk_editable_get_selection_bounds(
        GTK_EDITABLE(popup_sbaw->pages[id].source_text),
        &start, &end))
        break;
      if(start != end) {
        if(start>end)
        {
          temp=start;
          start=end;
          end=temp;
        }
        if((end-start+2)>256) // FIXME bounds?
          end=start+256-2;
        for(i=start;i<end;i++)
          text[i-start]=GTK_TEXT_INDEX(GTK_TEXT(popup_sbaw->pages[id].source_text),(guint)i);

        unsigned int uLastCharIndex = i-start;
        text[uLastCharIndex]=0;
        TrimWhiteSpaceFromString(text);

        if(text[0] != 0) {
          /*
          register_symbol *pReg = get_symbol_table().findRegisterSymbol(text);
          if(pReg == NULL) {
            // We also try upper cased.
            string sName(text);
            toupper(sName);
            pReg = get_symbol_table().findRegisterSymbol(sName.c_str());
          }
          if(pReg == NULL) {
            // We also try with a '_' prefix.
            string sName("_");
            sName.append(text);
            pReg = get_symbol_table().findRegisterSymbol(sName.c_str());
            if(pReg == NULL) {
              // We also try upper cased.
              toupper(sName);
              pReg = get_symbol_table().findRegisterSymbol(sName.c_str());
            }
          }
          */
          Register *pReg = findRegister(string(text));
          if(pReg == NULL) {
            GtkWidget *dialog = gtk_message_dialog_new( GTK_WINDOW(popup_sbaw->window),
              GTK_DIALOG_MODAL,
              GTK_MESSAGE_WARNING,
              GTK_BUTTONS_OK,
              "The symbol '%s' does not exist as a register symbol.\n"
              "Only register based symbols may be added to the Watch window.",
              text);
            gtk_dialog_run (GTK_DIALOG (dialog));
            gtk_widget_destroy (dialog);
          }
          else {
            popup_sbaw->gp->watch_window->Add(pReg);
          }
        }
      }
      //      popup_sbaw->gp->symbol_window->SelectSymbolName(text);
    }
    break;
  case MENU_STEP:
    popup_sbaw->pma->step(1);
    break;
  case MENU_STEP_OVER:
    popup_sbaw->pma->step_over();
    break;
  case MENU_RUN:
    popup_sbaw->gp->cpu->run();
    break;
  case MENU_STOP:
    popup_sbaw->pma->stop();
    break;
  case MENU_RESET:
    popup_sbaw->gp->cpu->reset(POR_RESET);
    break;
  case MENU_FINISH:
    popup_sbaw->pma->finish();
    break;
  default:
    puts("Unhandled menuitem?");
    break;
  }
}

GtkWidget *
SourceBrowserAsm_Window::BuildPopupMenu(GtkWidget *sheet, SourceBrowserAsm_Window *sbaw)
{
  GtkWidget *menu;
  GtkWidget *submenu;
  GtkWidget *item;
  unsigned int i;
  int id;

  popup_sbaw=sbaw;

  id = gtk_notebook_get_current_page(GTK_NOTEBOOK(popup_sbaw->notebook));
  menu=gtk_menu_new();
  for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
    item=gtk_menu_item_new_with_label(menu_items[i].name);
    menu_items[i].item=item;
    gtk_signal_connect(GTK_OBJECT(item),"activate",
      (GtkSignalFunc) PopupMenuHandler,
      &menu_items[i]);

    gtk_widget_show(item);
    gtk_menu_append(GTK_MENU(menu),item);
  }

  submenu=gtk_menu_new();
  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (submenu), item);
  gtk_widget_show (item);
  for (i=0; i < (sizeof(submenu_items)/sizeof(submenu_items[0])) ; i++){
    item=gtk_menu_item_new_with_label(submenu_items[i].name);
    submenu_items[i].item=item;
    gtk_signal_connect(GTK_OBJECT(item),"activate",
      (GtkSignalFunc) PopupMenuHandler,
      &submenu_items[i]);

    GTK_WIDGET_SET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);

    gtk_widget_show(item);
    gtk_menu_append(GTK_MENU(submenu),item);
  }
  item = gtk_menu_item_new_with_label ("Controls");
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);

  return menu;
}

//========================================================================
// BreakPointList - a helper class to assist in managing breakpoints
BreakPointList::BreakPointList(void)
{
  iter = 0;
}

//----------------------------------------
// Remove items from a breakpoint list
//
// Input: address - if this is less than 0 then all items are removed from
//        the list. Otherwise, only the items that match the address field
//        in the breakpoint structure are removed.

void BreakPointList::Remove(int address = -1)
{
  GList *li = iter;

  while(li)
  {
    GList *next = li->next;

    BreakPointInfo *bpi = (BreakPointInfo*)li->data;

    // remove the breakpoint
    if(address<0 || bpi->address==address) {
      iter = g_list_remove(li,li->data);
      if(bpi)
        delete bpi;
    }

    li = next;
  }

  if(address<0)
    iter = 0;
}

//----------------------------------------
// Add - add a new item to the breakpoint list.
//
void BreakPointList::Add(int address, GtkWidget *pwidget, GtkWidget *layout, int pos)
{
  BreakPointInfo *bpi= new BreakPointInfo(address, 0,0,0);
  //printf("Add: address:%d, pos:%d\n",address,pos);
  bpi->setBreakWidget(pwidget);
  gtk_layout_put(GTK_LAYOUT(layout),
    pwidget,
    PIXMAP_SIZE*0,
    pos
    );
  gtk_widget_show(pwidget);
  iter=g_list_append(iter,bpi);
}

void SourceBrowserAsm_Window::remove_all_points(
  SourceBrowserAsm_Window *sbaw)
{
  sbaw->breakpoints.Remove();
  sbaw->notify_start_list.Remove();
  sbaw->notify_stop_list.Remove();
}

gint SourceBrowserAsm_Window::switch_page_cb(
                            GtkNotebook     *notebook,
                            GtkNotebookPage *page,
                            guint            page_num,
                            SourceBrowserAsm_Window *sbaw)
{
  if(!sbaw || !sbaw->gp || !sbaw->gp->cpu)
    return 1;


  if(sbaw->current_page!=page_num) {

    //printf("switch_page_cb: %s, from:%d to%d\n",sbaw->name(),sbaw->current_page,page_num);
    int id;
    unsigned int address;

    sbaw->current_page=page_num;
    id=sbaw->pages[page_num].pageindex_to_fileid;
    if (id != -1)
      sbaw->pma->set_hll_mode(file_id_to_source_mode[id]);

    // Update pc widget
    // address=sbaw->gp->cpu->pc->get_raw_value();
    address = sbaw->pma->get_PC();
    sbaw->SetPC(address);

    remove_all_points(sbaw);

    // update breakpoint widgets
    unsigned uPMMaxIndex = sbaw->gp->cpu->program_memory_size();
    for(unsigned int uPMIndex=0; uPMIndex < uPMMaxIndex; uPMIndex++)
      sbaw->UpdateLine(sbaw->gp->cpu->map_pm_index2address(uPMIndex));
  }
  return 1;
}

/*
button event handler for sbaw->pages[id].source_text.
If we get button1 doubleclick event then we toggle breakpoint
If we get button3 buttonpress then we popup menu.
*/
gint SourceBrowserAsm_Window::sigh_button_event(
  GtkWidget *widget,
  GdkEventButton *event,
  SourceBrowserAsm_Window *sbaw)
{
  int id;
  unsigned int i;
  GtkWidget *item;
  int vadj_value=0;

  assert(event&&sbaw);

  assert(sbaw->notebook != 0);
  id = gtk_notebook_get_current_page(GTK_NOTEBOOK(sbaw->notebook));

  assert(id >= 0 && id < SBAW_NRFILES);
  assert(sbaw->pages[id].source_text != 0);
  assert(GTK_TEXT(sbaw->pages[id].source_text)->vadj != 0);
  vadj_value=(int)GTK_TEXT(sbaw->pages[id].source_text)->vadj->value;

  if(event->type==GDK_BUTTON_PRESS &&
    event->button==3)
  {
    popup_sbaw=sbaw;

    sbaw->menu_data = sbaw->getBPatPixel(id, (int) (event->y+vadj_value));

    for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++) {
      item=menu_items[i].item;

      switch(menu_items[i].id){
      case MENU_SELECT_SYMBOL:

        {
          gint start, end;

          if (!gtk_editable_get_selection_bounds(
            GTK_EDITABLE(popup_sbaw->pages[id].source_text),
            &start, &end))
          {
            gtk_widget_set_sensitive (item, FALSE);
          }
          else
          {
            gtk_widget_set_sensitive (item, TRUE);
          }
          break;
        }
      default:
        break;
      }
    }

    assert(GTK_MENU(sbaw->popup_menu));
    gtk_menu_popup(GTK_MENU(sbaw->popup_menu), 0, 0, 0, 0,
      3, event->time);

    // override pages[id].source_text's handler
    // is there a better way? FIXME
    gtk_signal_emit_stop_by_name(GTK_OBJECT(sbaw->pages[id].source_text),"button_press_event");

    return 1;
  }

  // FIXME, doesn't get button4/5 in gtk2???
  //printf("event->type %d, event->button %d\n",event->type,event->button);
  if(event->type==GDK_BUTTON_PRESS && event->button==4)
  { // wheel scroll up
    printf("scroll up\n");
    GTK_TEXT(sbaw->pages[id].source_text)->vadj->value-=GTK_TEXT(sbaw->pages[id].source_text)->vadj->page_increment/4.0;
    if(GTK_TEXT(sbaw->pages[id].source_text)->vadj->value < GTK_TEXT(sbaw->pages[id].source_text)->vadj->lower)
      GTK_TEXT(sbaw->pages[id].source_text)->vadj->value = GTK_TEXT(sbaw->pages[id].source_text)->vadj->lower;
    gtk_adjustment_value_changed(GTK_TEXT(sbaw->pages[id].source_text)->vadj);
    return 1;
  }
  if(event->type==GDK_BUTTON_PRESS && event->button==5)
  { // wheel scroll down
    printf("scroll down\n");
    GTK_TEXT(sbaw->pages[id].source_text)->vadj->value+=GTK_TEXT(sbaw->pages[id].source_text)->vadj->page_increment/4.0;
    if(GTK_TEXT(sbaw->pages[id].source_text)->vadj->value > GTK_TEXT(sbaw->pages[id].source_text)->vadj->upper-GTK_TEXT(sbaw->pages[id].source_text)->vadj->page_increment)
      GTK_TEXT(sbaw->pages[id].source_text)->vadj->value = GTK_TEXT(sbaw->pages[id].source_text)->vadj->upper-GTK_TEXT(sbaw->pages[id].source_text)->vadj->page_increment;
    gtk_adjustment_value_changed(GTK_TEXT(sbaw->pages[id].source_text)->vadj);
    return 1;
  }
  return 0;
}

static gint text_adj_cb(GtkAdjustment *adj, GtkAdjustment *adj_to_update)
{
  // when sbaw->pages[id].source_text adjustment changes, we update the layout adj.

  // I assume that both adjustments count pixels

  assert(adj_to_update&&adj);

  if(adj_to_update && adj )
  {
    if (adj_to_update->upper >= adj->value )
    {
      //printf("%d: setting adjustment to %g old value %g\n",__LINE__,adj->value, adj_to_update->value);
      gtk_adjustment_set_value(adj_to_update, adj->value);
    }
  }

  return 0;
}


/*
This is handler for motion, button press and release for source_layout.
The GdkEventMotion and GdkEventButton are very similar so I (Ralf) use
the same for both!
This function is too complex, FIXME.
*/


static gint marker_cb(GtkWidget *w1,
                      GdkEventButton *event,
                      SourceBrowserAsm_Window *sbaw)
{
  static int button_pressed;
  static int button_pressed_y;
  static int button_pressed_x;
  static gdouble vadj_value=0.0;

  static int timeout_tag=-1;

  if(!sbaw || !sbaw->gp || !sbaw->gp->cpu)
    return 1;

  int id = gtk_notebook_get_current_page(GTK_NOTEBOOK(sbaw->notebook));

  vadj_value=GTK_TEXT(sbaw->pages[id].source_text)->vadj->value;

  switch(event->type) {

  case GDK_MOTION_NOTIFY:
    break;
  case GDK_BUTTON_PRESS:
    if(button_pressed==1)
      break;  // click number two(/three?) of a double click?
    button_pressed = 1;
    button_pressed_x = (int)event->x;  // and initial position of
    button_pressed_y = (int)event->y;  // possible drag action
    break;
  case GDK_2BUTTON_PRESS:
    if(event->button == 1) {
      int pos = (int)event->y - sbaw->layout_offset;
      BreakPointInfo *e = sbaw->getBPatPixel(id, pos);
      int line = e->getLine();
      sbaw->pma->toggle_break_at_line(sbaw->pages[id].pageindex_to_fileid ,line+1);
    }
    break;
  case GDK_BUTTON_RELEASE:
    button_pressed=0;
    if(timeout_tag!=-1)
    {
      gtk_timeout_remove(timeout_tag);
      timeout_tag=-1;
    }

    break;
  default:
    printf("Whoops? event type %d\n",event->type);
    break;
  }

  return 0;
}

/*
Adds a page to the notebook, and returns notebook-id for that page.
*/
int SourceBrowserAsm_Window::add_page(
  SourceBrowserAsm_Window *sbaw, int file_id)
{
  char str[256], *label_string;
  GtkWidget *hbox, *label, *vscrollbar;
  GtkStyle *style=0;

  int id;

  hbox = gtk_hbox_new(0,0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), PAGE_BORDER);

  FileContext *fc = sbaw->gp->cpu->files[file_id];

  strncpy(str,fc->name().c_str(),sizeof(str));

  label_string=str;

  find_char_and_skip(&label_string,'/');
  find_char_and_skip(&label_string,'\\');

  label=gtk_label_new(label_string);

  gtk_notebook_append_page(GTK_NOTEBOOK(sbaw->notebook),hbox,label);

  id=gtk_notebook_page_num(GTK_NOTEBOOK(sbaw->notebook),hbox);

  assert(id<SBAW_NRFILES && id >=0);
  sbaw->pages[id].pageindex_to_fileid = file_id;
  sbaw->pages[id].notebook_child=hbox;

  gtk_widget_show(hbox);

  // Create the Gray column to the left of the Source text.

  sbaw->pages[id].source_layout_adj = (GtkAdjustment*)gtk_adjustment_new(0.0,0.0,0.0,0.0,0.0,0.0);
  sbaw->pages[id].source_layout = gtk_layout_new(0,sbaw->pages[id].source_layout_adj);

  gtk_widget_set_events(sbaw->pages[id].source_layout,
    gtk_widget_get_events(sbaw->pages[id].source_layout)|
    GDK_BUTTON_PRESS_MASK |
    GDK_BUTTON_MOTION_MASK |
    GDK_BUTTON_RELEASE_MASK);
  gtk_widget_show(sbaw->pages[id].source_layout);

  gtk_widget_set_usize(GTK_WIDGET(sbaw->pages[id].source_layout),PIXMAP_SIZE*2,0);
  gtk_box_pack_start(GTK_BOX(hbox), sbaw->pages[id].source_layout,
    FALSE,FALSE, 0);

  // Create the Scroll bar.

  vscrollbar = gtk_vscrollbar_new(0);

  gtk_widget_show(vscrollbar);

  // Create the text object for holding the Source text

  sbaw->pages[id].source_text = gtk_text_new(0,GTK_RANGE(vscrollbar)->adjustment);

  gtk_text_set_word_wrap(GTK_TEXT(sbaw->pages[id].source_text),0);
  gtk_text_set_line_wrap(GTK_TEXT(sbaw->pages[id].source_text),0);
  gtk_widget_show(sbaw->pages[id].source_text);


  style=gtk_style_new();
  style->base[GTK_STATE_NORMAL].red=65535;
  style->base[GTK_STATE_NORMAL].green=65535;
  style->base[GTK_STATE_NORMAL].blue=65535;

  gtk_widget_set_style(GTK_WIDGET(sbaw->pages[id].source_text),style);

  gtk_signal_connect(GTK_OBJECT(sbaw->pages[id].source_text), "button_press_event",
    GTK_SIGNAL_FUNC(sigh_button_event), sbaw);
  gtk_box_pack_start_defaults(GTK_BOX(hbox), sbaw->pages[id].source_text);

  gtk_box_pack_start(GTK_BOX(hbox), vscrollbar,
    FALSE,FALSE, 0);
  gtk_signal_connect(GTK_OBJECT(GTK_TEXT(sbaw->pages[id].source_text)->vadj),
    "value_changed",GTK_SIGNAL_FUNC(text_adj_cb),sbaw->pages[id].source_layout_adj);

  gtk_signal_connect(GTK_OBJECT(sbaw->pages[id].source_layout),"motion-notify-event",
    GTK_SIGNAL_FUNC(marker_cb),sbaw);
  gtk_signal_connect(GTK_OBJECT(sbaw->pages[id].source_layout),"button_press_event",
    GTK_SIGNAL_FUNC(marker_cb),sbaw);
  gtk_signal_connect(GTK_OBJECT(sbaw->pages[id].source_layout),"button_release_event",
    GTK_SIGNAL_FUNC(marker_cb),sbaw);

  // display everything, so that gtk_notebook_get_current_page() works
  GTKWAIT;

  // We create pixmaps here, where the gtk_widget_get_style() call will
  // succeed. I tried putting this code in CreateSourceBrowserAsmWindow()
  // but then the window was not realized. And if I manually realized
  // it, then the call to gtk_window_set_default_size() was ignored.
  // Was that a bug in gtk? (gtk version 1.2.3)
  if(sbaw->pixmap_pc==0)
  {
    style = gtk_style_new();
    sbaw->pc_mask = 0;
    sbaw->bp_mask = 0;
    sbaw->canbp_mask = 0;
    sbaw->startp_mask = 0;
    sbaw->stopp_mask = 0;
    sbaw->pixmap_pc = gdk_pixmap_create_from_xpm_d(sbaw->window->window,
      &sbaw->pc_mask,
      &style->bg[GTK_STATE_NORMAL],
      (gchar**)pc_xpm);
    sbaw->pixmap_break = gdk_pixmap_create_from_xpm_d(sbaw->window->window,
      &sbaw->bp_mask,
      &style->bg[GTK_STATE_NORMAL],
      (gchar**)break_xpm);

    sbaw->pixmap_canbreak = gdk_pixmap_create_from_xpm_d(sbaw->window->window,
      &sbaw->canbp_mask,
      &style->bg[GTK_STATE_NORMAL],
      (gchar**)canbreak_xpm);

    sbaw->pixmap_profile_start = gdk_pixmap_create_from_xpm_d(sbaw->window->window,
      &sbaw->startp_mask,
      &style->bg[GTK_STATE_NORMAL],
      (gchar**)startp_xpm);
    sbaw->pixmap_profile_stop = gdk_pixmap_create_from_xpm_d(sbaw->window->window,
      &sbaw->stopp_mask,
      &style->bg[GTK_STATE_NORMAL],
      (gchar**)stopp_xpm);
  }
  sbaw->pages[id].source_pcwidget = gtk_pixmap_new(sbaw->pixmap_pc,sbaw->pc_mask);
  gtk_layout_put(GTK_LAYOUT(sbaw->pages[id].source_layout),
    sbaw->pages[id].source_pcwidget,PIXMAP_SIZE,0);
  gtk_widget_show(sbaw->pages[id].source_pcwidget);

  return id;

}

// Return true of there are instructions corresponding to the source line
int source_line_represents_code(Processor *cpu,
                                FileContext *fc,
                                unsigned int line)
{
  int address;
  address = cpu->pma->find_address_from_line(fc,line);

  return address>=0;
}

static int s_TotalTextLength = 0;
static CFormattedTextFragment * s_pLast = NULL;

static void InitCache(FileContext::Cache &FileCache) {
  s_TotalTextLength = 0;
  s_pLast = NULL;
}

static void AddCache(FileContext::Cache &FileCache, const char *pFragment,
                     int length,
                     GtkStyle *pStyle, GdkFont *font)
{
  if(s_pLast && s_pLast->m_text_style == pStyle) {
    if(length == -1) {
      s_pLast->m_length = -1;
      s_pLast->m_Fragment.append(pFragment);
    }
    else {
      s_pLast->m_length += length;
      s_pLast->m_Fragment.append(pFragment, length);
    }
  }
  else {
    s_pLast = new CFormattedTextFragment(pFragment, length, pStyle,
      font);
    FileCache.push_back(s_pLast);
  }

  s_TotalTextLength += (length == -1) ? strlen(pFragment) : length;

}
/*
Fills sbaw->pages[id].source_text with text from
file pointer sbaw->sbw.gui_obj.gp->p->files[file_id].file_ptr
*/
void SourceBrowserAsm_Window::SetText(int id, int file_id, FileContext *fc)
{
  bool instruction_done;
  char text_buffer[256];
  int cblock=0;

  int index;

  int line=0;
  FileContext::Cache &FileCache = fc->m_cache;

  // get a manageable pointer to the processor
  Processor *cpu = gp->cpu;
  GtkWidget *pSourceWindow = pages[id].source_text;
  gtk_text_freeze(GTK_TEXT(pSourceWindow));

  gtk_editable_delete_text(GTK_EDITABLE(pSourceWindow),0,-1);
  remove_all_points(this);

  // Check the type of file (ASM or C), and seperate the pattern matching
  // into set_text_asm() and set_text_c().
  // These functions fill the page with the colored source, and also fills
  // the sa_xlate_list[id] structure list with values, so that the pixmaps
  // are put on the right place.


  if(FileCache.size() == 0) {
    s_totallinesheight[id]=0;
    InitCache(FileCache);
    ParseSourceToFormattedText(
      id,
      s_totallinesheight[id],
      instruction_done,
      text_buffer,
      cblock,
      index,
      line,
      FileCache,
      cpu,
      pSourceWindow,
      fc,
      file_id  );
  }
  FileContext::Cache::iterator it;
  FileContext::Cache::iterator itEnd = FileCache.end();
  for(it = FileCache.begin(); it != itEnd; it++) {
    CFormattedTextFragment* pFragment = (CFormattedTextFragment*)*it;
    pFragment->SetText(GTK_TEXT(pSourceWindow));
  }
  DetermineBreakinfos(id);

  gtk_layout_set_size(GTK_LAYOUT(pages[id].source_layout),
    2*PIXMAP_SIZE,
    s_totallinesheight[id]+5*PIXMAP_SIZE);
  gtk_text_thaw(GTK_TEXT(pSourceWindow));

}

void SourceBrowserAsm_Window::ParseSourceToFormattedText(
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
  int file_id  )
{
  GList *iter = s_global_sa_xlate_list[id];

  /*
  for(iter=s_global_sa_xlate_list[id];iter!=0;)
  {
    GList *next=iter->next;
    free( (BreakPointInfo*)iter->data );
    g_list_remove(iter,iter->data);
    iter=next;
  }
  */
  while (iter) {
    free( (BreakPointInfo*)iter->data );
    iter = g_list_remove(iter,iter->data);
  }

  s_global_sa_xlate_list[id]=0;
  BreakPointInfo *entry;
  char *p;

  fc->rewind();

  while(fc->gets(text_buffer, 256))
  {
    char *end, *q;

    instruction_done=false; // to separate instruction from other text (symbols)

    index = s_TotalTextLength;

    p=text_buffer;

    if(file_id_to_source_mode[file_id]==ProgramMemoryAccess::ASM_MODE) {

      if(*p=='#' || !strncmp(p,"include",7))
      { // not a label
        q=p;
        q++;
        while(isalnum(*q) || *q=='_')
          q++;

        AddCache(FileCache, p, q-p,default_text_style,
          default_font);
        p=q;
        instruction_done=true; // well, variable misnamed
      }
      else if( (isalnum(*p) || *p=='_'))
      { // a label
        // locate end of label
        q=p;
        while(isalnum(*q) || *q=='_')
          q++;

        AddCache(FileCache, text_buffer, q-p,
          label_text_style, label_font);
        // advance the pointer p
        p=q;
      }
    }

    // 'end' is end of line
    end = text_buffer + strlen(text_buffer);

    // loop through the rest of the line
    while( p < end )
    {
      if(!source_line_represents_code(cpu,fc,line+1)) {
        AddCache(FileCache, p, -1,comment_text_style,
          instruction_font);
        break;
      }
      if(file_id_to_source_mode[file_id]==ProgramMemoryAccess::HLL_MODE)
      {
        AddCache(FileCache, p, -1,
          default_text_style, default_font);
        break;

      } else {
        // Asm mode.

        if( *p == ';')  { // comment
          comment_font      = gtk_style_get_font(comment_text_style);
          AddCache(FileCache, p, -1,
            comment_text_style, comment_font);
          break;
        }
        else if(isalpha(*p) || *p=='_')
        { // instruction, symbol or cblock
          q=p;
          while(isalnum(*q) || *q=='_')
            q++;
          if( ( !instruction_done && cblock==0) || !strncasecmp(p,"endc",4) )
          {  // instruction or cblock
            instruction_done=true;
            cblock=0;

            if(!strncasecmp(p,"cblock",6))
              cblock=1;

            AddCache(FileCache, p, q-p,
              instruction_text_style, instruction_font);
          }
          else
          { // symbol
            AddCache(FileCache, p, q-p,
              symbol_text_style, symbol_font);
          }
          p=q;
        }
        else if( isxdigit(*p))
        { // number
          q=p;
          if(*p=='0' && toupper(*(p+1))=='X')
            q+=2;
          while(isxdigit(*q))
            q++;

          AddCache(FileCache, p, q-p,
            number_text_style, number_font);
          p=q;
        }
        else
        { // default
          // FIXME, add a 'whitespace_text_style'
          // There is a small annoyance here. If the source
          // initially on a line have whitespace, followed by
          // a comment. Now if the comment have a smaller font
          // than the default font then the line will have line
          // spacing larger than nessesary.

          AddCache(FileCache, p, 1,
            default_text_style, default_font);
          p++;
        }
      }
    } //end of while( p < end )

    totallinesheight += CFormattedTextFragment::s_linedescent +
      CFormattedTextFragment::s_lineascent;

    // create an entry in sa_xlate_list for this source line.
    // 'this source line' is the one in 'buf' with line number
    // 'line' and index 'index' into text
    int     pos = totallinesheight -
      (CFormattedTextFragment::s_lineascent -
      CFormattedTextFragment::s_linedescent) -
      PIXMAP_SIZE/2 + PAGE_BORDER;

    entry= new BreakPointInfo(0, line,index,pos);
    s_global_sa_xlate_list[id]=g_list_append(s_global_sa_xlate_list[id],entry);
    line++;
  }

  // this made the end case of the search simpler once
  AddCache(FileCache, " ", 1,
    default_text_style, default_font);
}

void SourcePage::Close(void)
{
  if(notebook != NULL && notebook_child != NULL)
  {
    int num=gtk_notebook_page_num(GTK_NOTEBOOK(notebook),notebook_child);
    gtk_notebook_remove_page(GTK_NOTEBOOK(notebook),num);
    // JRH - looks like gtk_notebook_remove_page() is also
    //       deallocating notebook_chile.
    // gtk_widget_destroy(notebook_child);
    // this is all that is needed to destroy all child widgets
    // of notebook_child.
    notebook_child=0;
    source_layout_adj = 0;
    source_layout = 0;
    source_text = 0;
    pageindex_to_fileid = INVALID_VALUE;
    source_pcwidget = 0;
  }
}

void SourceBrowserAsm_Window::CloseSource(void)
{

  m_bLoadSource=0;
  m_bSourceLoaded = 0;
  if(!enabled)
    return;

  pixmap_pc=0;
  pixmap_break=0;
  pixmap_profile_start=0;
  pixmap_profile_stop=0;

  remove_all_points(this);

  layout_offset=-1;
  for(int i=0;i<SBAW_NRFILES;i++)
    pages[i].Close();
}

void SourceBrowserAsm_Window::NewSource(GUI_Processor *_gp)
{
  int i;
  int id;

  const char *file_name;
  int file_id;

  unsigned int address;

  if(!gp || !gp->cpu || !gp->cpu->pma)
    return;

  Processor * pProc = gp->cpu;
  if(!enabled)
  {
    m_bLoadSource=1;
    return;
  }

  if(!pma)
    pma = pProc->pma;

  assert(wt==WT_asm_source_window);

  CloseSource();

  m_bLoadSource=1;

  Dprintf(("NewSource\n"));

  /* Now create a cross-reference link that the
  * simulator can use to send information back to the gui
  */
  if(pProc->pc) {
    SourceXREF *cross_reference = new SourceXREF();

    cross_reference->parent_window_type =   WT_asm_source_window;
    cross_reference->parent_window = (gpointer) this;
    cross_reference->data = (gpointer) 0;

    pProc->pc->add_xref((gpointer) cross_reference);
    if(pProc->pc != pma->GetProgramCounter()) {
      pma->GetProgramCounter()->add_xref((gpointer) cross_reference);
    }
  }
  if(pProc->files.nsrc_files() != 0) {

    for(i=0;i<pProc->files.nsrc_files();i++) {
      FileContext *fc = pProc->files[i];
      file_name = fc->name().c_str();
      int iNameLength = strlen(file_name);

      if(strcmp(file_name+iNameLength-4,".lst")
        &&strcmp(file_name+iNameLength-4,".LST")
        &&strcmp(file_name+iNameLength-4,".cod")
        &&strcmp(file_name+iNameLength-4,".COD"))
      {
        if(!strcmp(file_name+iNameLength-2,".c")
          ||!strcmp(file_name+iNameLength-2,".C")
          ||!strcmp(file_name+iNameLength-4,".jal")
          ||!strcmp(file_name+iNameLength-4,".JAL")
          )
        {
          // These are HLL sources
          file_id_to_source_mode[i]=ProgramMemoryAccess::HLL_MODE;
          pma->set_hll_mode(ProgramMemoryAccess::HLL_MODE);
        }

        // FIXME, gpsim may change sometime making this fail
        file_id = i;

        // Make sure that the file is open
        fc->open("r");

        id = add_page(this,file_id);

        SetText(id,file_id, fc);

      } else {
        if(verbose)
          printf ("SourceBrowserAsm_new_source: skipping file: <%s>\n",
          file_name);
      }
    }

    m_bSourceLoaded = 1;

  }


  // Why is this needed? set_page() in SourceBrowserAsm_set_pc()
  // fails with widget_map() -> not visible
  GTKWAIT;

  address=pProc->pma->get_PC();
  if(address==INVALID_VALUE)
    puts("Warning, PC is invalid?");
  else
    SetPC(address);

  // update breakpoint widgets
  unsigned uPMMaxIndex = pProc->program_memory_size();
  for(unsigned int uPMIndex=0; uPMIndex < uPMMaxIndex; uPMIndex++)
    UpdateLine(pProc->map_pm_index2address(uPMIndex));

  GTKWAIT;

  Dprintf((" Source is loaded\n"));
}

static gint configure_event(GtkWidget *widget, GdkEventConfigure *e, gpointer data)
{
  if(widget->window==0)
    return 0;

  gdk_window_get_root_origin(widget->window,&dlg_x,&dlg_y);
  return 0;
}

static int load_fonts(SOURCE_WINDOW *sbaw)
{

  gtk_style_set_font(sbaw->comment_text_style,
    gdk_font_from_description(pango_font_description_from_string(sbaw->commentfont_string)));

  GdkFont *font = gdk_font_from_description(pango_font_description_from_string(sbaw->sourcefont_string));
  gtk_style_set_font(sbaw->default_text_style, font);
  gtk_style_set_font(sbaw->label_text_style, font);
  gtk_style_set_font(sbaw->symbol_text_style, font);
  gtk_style_set_font(sbaw->instruction_text_style, font);
  gtk_style_set_font(sbaw->number_text_style, font);

  if (gtk_style_get_font(sbaw->comment_text_style) == 0)
    return 0;
  if (gtk_style_get_font(sbaw->default_text_style) == 0)
    return 0;

  return 1;
}

/*************** Font selection dialog *********************/
class DialogFontSelect {
public:
  static gint  DialogRun(GtkWidget *w, gpointer user_data);

  static GtkWidget *m_pFontSelDialog;
};

GtkWidget * DialogFontSelect::m_pFontSelDialog = NULL;

gint DialogFontSelect::DialogRun(GtkWidget *w, gpointer user_data)
{
  GtkEntry *entry=GTK_ENTRY(user_data);
  const char *fontstring;
  gchar *fontname;

  if(m_pFontSelDialog==0)
  {

    m_pFontSelDialog=gtk_font_selection_dialog_new("Select font");

    fontstring=gtk_entry_get_text(entry);
  }
  gint result = gtk_dialog_run (GTK_DIALOG (m_pFontSelDialog));
  switch (result) {
  case GTK_RESPONSE_OK:
    fontname=gtk_font_selection_dialog_get_font_name(GTK_FONT_SELECTION_DIALOG(m_pFontSelDialog));
    gtk_widget_hide(m_pFontSelDialog);
    gtk_entry_set_text(entry,fontname);
    g_free(fontname);
    break;
  default:
      break;
  }
  gtk_widget_hide(m_pFontSelDialog);
  return result;
}

// To give access to reg and opcode windows
int font_dialog_browse(GtkWidget *w, gpointer user_data) {
  return DialogFontSelect::DialogRun(w, user_data);
}

/********************** Settings dialog ***************************/
static int settings_dialog(SOURCE_WINDOW *sbaw)
{
  static GtkWidget *dialog=0;
  GtkWidget *button;
  static int retval;
  GtkWidget *hbox;
  static GtkWidget *commentfontstringentry;
  static GtkWidget *sourcefontstringentry;
  GtkWidget *label;
  int fonts_ok=0;

  if(dialog==0)
  {
    dialog = gtk_dialog_new();
    gtk_window_set_title (GTK_WINDOW (dialog), "Source browser settings");
    gtk_signal_connect(GTK_OBJECT(dialog),
      "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
    gtk_signal_connect_object(GTK_OBJECT(dialog),
      "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(dialog));


    // Source font
    hbox = gtk_hbox_new(0,0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);
    gtk_widget_show(hbox);
    label=gtk_label_new("Font for source:");
    gtk_box_pack_start(GTK_BOX(hbox), label,
      FALSE,FALSE, 20);
    gtk_widget_show(label);
    sourcefontstringentry=gtk_entry_new();
    gtk_widget_set_size_request(sourcefontstringentry,
                                200, -1);
    gtk_box_pack_start(GTK_BOX(hbox), sourcefontstringentry,
      TRUE, TRUE, 0);
    gtk_widget_show(sourcefontstringentry);
    button = gtk_button_new_with_label("Browse...");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button,
      FALSE,FALSE,10);
    gtk_signal_connect(GTK_OBJECT(button),"clicked",
      GTK_SIGNAL_FUNC(DialogFontSelect::DialogRun),
      (gpointer)sourcefontstringentry);


    // Comment font
    hbox = gtk_hbox_new(0,0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),
      hbox,FALSE,FALSE,20);
    gtk_widget_show(hbox);
    label=gtk_label_new("Font for comments:");
    gtk_box_pack_start(GTK_BOX(hbox), label,
      FALSE,FALSE, 20);
    gtk_widget_show(label);
    commentfontstringentry=gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), commentfontstringentry,
      TRUE, TRUE, 0);
    gtk_widget_show(commentfontstringentry);
    button = gtk_button_new_with_label("Browse...");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button,
      FALSE,FALSE,10);
    gtk_signal_connect(GTK_OBJECT(button),"clicked",
      GTK_SIGNAL_FUNC(DialogFontSelect::DialogRun),
      (gpointer)commentfontstringentry);


    // OK button
    gtk_dialog_add_button(GTK_DIALOG(dialog), "OK", GTK_RESPONSE_OK);
  }

  gtk_entry_set_text(GTK_ENTRY(sourcefontstringentry), sbaw->sourcefont_string);
  gtk_entry_set_text(GTK_ENTRY(commentfontstringentry), sbaw->commentfont_string);

  gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
  gtk_widget_show_now(dialog);



  while(fonts_ok!=2)
  {
    char fontname[256];
#if GTK_MAJOR_VERSION >= 2
    PangoFontDescription *font;
#else
    GdkFont *font;
#endif

    gtk_dialog_run (GTK_DIALOG (dialog));
    fonts_ok=0;

    strcpy(fontname,gtk_entry_get_text(GTK_ENTRY(sourcefontstringentry)));
#if GTK_MAJOR_VERSION >= 2
    if((font=pango_font_description_from_string(fontname))==0)
#else
    if((font=gdk_fontset_load(fontname))==0)
#endif
    {
      if(gui_question("Sourcefont did not load!","Try again","Ignore/Cancel")==FALSE)
        break;
    }
    else
    {
#if GTK_MAJOR_VERSION >= 2
#else
      gdk_font_unref(font);
#endif
      strcpy(sbaw->sourcefont_string,gtk_entry_get_text(GTK_ENTRY(sourcefontstringentry)));
      config_set_string(sbaw->name(),"sourcefont",sbaw->sourcefont_string);
      fonts_ok++;
    }

    strcpy(fontname,gtk_entry_get_text(GTK_ENTRY(commentfontstringentry)));
#if GTK_MAJOR_VERSION >= 2
    if((font=pango_font_description_from_string(fontname))==0)
#else
    if((font=gdk_fontset_load(fontname))==0)
#endif
    {
      if(gui_question("Commentfont did not load!","Try again","Ignore/Cancel")==FALSE)
        break;
    }
    else
    {
#if GTK_MAJOR_VERSION >= 2
#else
      gdk_font_unref(font);
#endif
      strcpy(sbaw->commentfont_string,gtk_entry_get_text(GTK_ENTRY(commentfontstringentry)));
      config_set_string(sbaw->name(),"commentfont",sbaw->commentfont_string);
      fonts_ok++;
    }
  }

  load_fonts(sbaw);
  if(sbaw->m_bLoadSource)
    sbaw->NewSource(sbaw->gp);

  gtk_widget_hide(dialog);

  return retval;
}

/*********************** gui message dialog *************************/
static gboolean
message_close_cb(GtkWidget *widget, gpointer d)
{
  gtk_widget_hide(GTK_WIDGET(d));

  return FALSE;
}

int gui_message(const char *message)
{
  static GtkWidget *dialog=0;
  static GtkWidget *label;
  GtkWidget *button;
  GtkWidget *hbox;

  assert(message);

  if(dialog==0)
  {
    dialog = gtk_dialog_new();

    gtk_signal_connect(GTK_OBJECT(dialog),
      "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
    gtk_signal_connect_object(GTK_OBJECT(dialog),
      "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(dialog));

    hbox = gtk_hbox_new(0,0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);

    button = gtk_button_new_with_label("OK");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
      FALSE,FALSE,10);
    gtk_signal_connect(GTK_OBJECT(button),"clicked",
      GTK_SIGNAL_FUNC(message_close_cb),(gpointer)dialog);
    GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
    gtk_widget_grab_default(button);

    label=gtk_label_new(message);
    gtk_box_pack_start(GTK_BOX(hbox), label,
      FALSE,FALSE, 20);

    gtk_widget_show(hbox);
    gtk_widget_show(label);
  }
  else
  {
    gtk_label_set_text(GTK_LABEL(label),message);
  }

  gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
  gtk_widget_show_now(dialog);

  return 0;
}

/****************** gui question dialog **************************/
static void a_cb(GtkWidget *w, gpointer user_data)
{
  *(int*)user_data=TRUE;
}

static void b_cb(GtkWidget *w, gpointer user_data)
{
  *(int*)user_data=FALSE;
}

// modal dialog, asking a yes/no question
int gui_question(const char *question, const char *a, const char *b)
{
  static GtkWidget *dialog=0;
  static GtkWidget *label;
  static GtkWidget *abutton;
  static GtkWidget *bbutton;
  GtkWidget *hbox;
  static int retval=-1;

  if(dialog==0)
  {
    dialog = gtk_dialog_new();
    gtk_signal_connect(GTK_OBJECT(dialog),
      "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
    gtk_signal_connect_object(GTK_OBJECT(dialog),
      "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(dialog));

    hbox = gtk_hbox_new(0,0);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);

    abutton = gtk_button_new_with_label(a);
    gtk_widget_show(abutton);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), abutton,
      FALSE,FALSE,10);
    gtk_signal_connect(GTK_OBJECT(abutton),"clicked",
      GTK_SIGNAL_FUNC(a_cb),(gpointer)&retval);
    GTK_WIDGET_SET_FLAGS (abutton, GTK_CAN_DEFAULT);
    gtk_widget_grab_default(abutton);

    bbutton = gtk_button_new_with_label(b);
    gtk_widget_show(bbutton);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), bbutton,
      FALSE,FALSE,10);
    gtk_signal_connect(GTK_OBJECT(bbutton),"clicked",
      GTK_SIGNAL_FUNC(b_cb),(gpointer)&retval);

    label=gtk_label_new(question);
    gtk_box_pack_start(GTK_BOX(hbox), label,
      FALSE,FALSE, 20);

    gtk_widget_show(hbox);
    gtk_widget_show(label);
  }
  else
  {
    gtk_label_set_text(GTK_LABEL(label),question);
    gtk_label_set_text(GTK_LABEL(GTK_BIN(abutton)->child),a);
    gtk_label_set_text(GTK_LABEL(GTK_BIN(bbutton)->child),b);
  }

  gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
  gtk_widget_show_now(dialog);

  gtk_grab_add(dialog);
  while(retval==-1 && GTK_WIDGET_VISIBLE(dialog))
    gtk_main_iteration();
  gtk_grab_remove(dialog);

  gtk_widget_hide(dialog);

  //    puts(retval==1?"Yes":"No");

  return retval;
}


/*
A rather long function, simplify main loop. FIXME.
*/
void SourceBrowserAsm_Window::find_cb(
  GtkWidget *w, SourceBrowserAsm_Window *sbaw)
{

  const char *p;
  GList *l;

  int casesensitive;
  int direction;

  int last_matched=0;
  int k=0;

  int char1, char2;
  int j;  // index into search string
  int tlen;
  int id;

  SourceBrowserAsm_Window * pSrcWindow = popup_sbaw;
  if(!pSrcWindow->m_bSourceLoaded) return;

  id = gtk_notebook_get_current_page(GTK_NOTEBOOK(pSrcWindow->notebook));
  SourcePage & SrcPage = pSrcWindow->pages[id];

  if(id != searchdlg.lastid)
  { //  Changed notebook tab since last search reset search.
    searchdlg.lastid=id;
    searchdlg.found=0;
    searchdlg.looped=0;
    searchdlg.start=0;
    searchdlg.lastfound=0;
    searchdlg.i=0;
  }

  if(GTK_TOGGLE_BUTTON(searchdlg.case_button)->active)
    casesensitive=TRUE;
  else
    casesensitive=FALSE;

  if(GTK_TOGGLE_BUTTON(searchdlg.backwards_button)->active)
    direction=-1;
  else
    direction=1;

  p=gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(searchdlg.entry)->entry));

  if(*p=='\0')
    return;

  if(searchdlg.string==0 || strcmp(searchdlg.string,p))
  {  // not same string as last time
    // search list to prevent duplicates
    l=searchdlg.combo_strings;
    while(l)
    {
      if(!strcmp((char*)l->data,p))
      {
        // the string p already is in list
        // move it first?, FIXME

        searchdlg.string = (char*)l->data;
        break;
      }
      l=l->next;
    }
    if(l == 0)
    { // we didn't find string in history, create a new one
      searchdlg.string=(char*)malloc(strlen(p)+1);
      strcpy(searchdlg.string,p);
      searchdlg.combo_strings = g_list_prepend(searchdlg.combo_strings,searchdlg.string);
      gtk_combo_set_popdown_strings(GTK_COMBO(searchdlg.entry),searchdlg.combo_strings);
    }

    // initialize variables for a new search
    searchdlg.found=0;
    searchdlg.looped=0;
    searchdlg.i = pSrcWindow->getBPatPixel(id,0)->index;
    searchdlg.start = searchdlg.i; // remember where we started searching
  }

  tlen =gtk_text_get_length(GTK_TEXT(SrcPage.source_text));
  j=0;
  for(;searchdlg.i>=0 && searchdlg.i<tlen;searchdlg.i+=direction)
  {
    if(searchdlg.string[j]=='\0')
    {  // match! We found the string in text.
      int start_i, end_i;

      searchdlg.found++;

      start_i = k+ (direction==-1);      // comparing backwards means
      end_i = searchdlg.i+ (direction==-1); // we have to add 1

      if(start_i>end_i)
      {
        int temp=end_i;  // swap, so that k is the smaller
        end_i=start_i;
        start_i=temp;
      }
      assert(start_i<end_i);
      if(start_i==searchdlg.lastfound)
      { // we found the same position as last time
        // happens when searching backwards
        j=0;
        if(direction==1)
          searchdlg.i++; // skip this match
        else
          searchdlg.i--; // skip this match
        last_matched=0;
      }
      else
      {
        int pixel;
        float inc;

        searchdlg.lastfound=start_i;

        pixel = pSrcWindow->getBPatPixel(id,start_i)->pos + 12;
        inc = (float)GTK_ADJUSTMENT(GTK_TEXT(SrcPage.source_text)->vadj)->page_increment;
        gtk_adjustment_set_value(GTK_ADJUSTMENT( GTK_TEXT( SrcPage.source_text)->vadj),
          pixel-inc/2);
        //printf("%d: setting adjustment to %g\n",__LINE__,pixel-inc/2);

        gtk_editable_select_region(GTK_EDITABLE(SrcPage.source_text),start_i,end_i);
        return;
      }
    }
    if(searchdlg.looped && (searchdlg.start == searchdlg.i))
    {
      if(searchdlg.found==0)
      {
        gui_message("Not found");
        return;
      }
      else if(searchdlg.found==1)
      {
        gui_message("Just a single occurance in text");

        // so that the next next call marks text too, we do:
        searchdlg.found=0;
        searchdlg.looped=0;
        searchdlg.lastfound=-1;
        return;
      }
    }

    // get another character
    char1=GTK_TEXT_INDEX(GTK_TEXT(SrcPage.source_text),(unsigned)searchdlg.i);
    if(direction==1)
      char2=searchdlg.string[j];
    else
      char2=searchdlg.string[strlen(searchdlg.string)-1-j];
    //FIXME, many calls to strlen

    if(casesensitive==FALSE)
    {
      char1=toupper(char1); // FIXME, what about native letters?
      char2=toupper(char2);
    }

    if(char1!=char2)
    {                   // if these characters don't match
      j=0;            // set search index for string back to zero
      last_matched=0; // char in this loop didn't match
    }
    else
    {
      if(!last_matched)
      {
        k=searchdlg.i;     // remember first matching index for later
        last_matched=1; // char in this loop matched
      }
      j++;  // forward string index to compare next char
    }

  }
  // the string was not found in text between index 'search start' and
  // one end of text (index '0' or index 'tlen')

  // We ask user it he want to search from other end of file
  if(direction==1)
  {
    if(gui_question("End of file\ncontinue from start?","Yes","No")==(int)TRUE)
    {
      searchdlg.i=0;
      searchdlg.looped=1;
      find_cb(w,pSrcWindow);  // tail recursive, FIXME
      return;
    }
    else
      searchdlg.i=tlen-1;
  }
  else
  {
    if(gui_question("Top of file\ncontinue from end?","Yes", "No")==(int)TRUE)
    {
      searchdlg.i=tlen-1;
      searchdlg.looped=1;
      find_cb(w,pSrcWindow);  // tail recursive, FIXME
      return;
    }
    else
      searchdlg.i=0;
  }
}

void SourceBrowserAsm_Window::find_clear_cb(
  GtkWidget *w, SourceBrowserAsm_Window *sbaw)
{
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(searchdlg.entry)->entry),"");
}

void SourceBrowser_Window::set_pma(ProgramMemoryAccess *new_pma)
{
  pma = new_pma;

  if(window && pma) {

    SetTitle();
  }

  if(status_bar)
    status_bar->NewProcessor(gp, pma);
}

SourceBrowserAsm_Window::SourceBrowserAsm_Window(GUI_Processor *_gp, char* new_name=0)
{
  m_SourceWindowIndex = m_SourceWindowCount;
  m_SourceWindowCount++;
  menu = "<main>/Windows/Source";

  window = 0;
  gp = _gp;
  pma = 0;

  if(new_name)
    set_name(new_name);
  else
    set_name("source_browser");

  wc = WC_source;
  wt = WT_asm_source_window;

  status_bar = new StatusBar_Window();

  breakpoints.iter=0;
  notify_start_list.iter=0;
  notify_stop_list.iter=0;

  layout_offset=-1;


  pixmap_pc = 0; // these are created somewhere else
  pixmap_break=0;
  pixmap_profile_start=0;
  pixmap_profile_stop=0;


  m_bSourceLoaded = 0;

  m_bLoadSource=0;

  get_config();
  current_page = 0xffffffff;

  for(int i=0; i<SBAW_NRFILES; i++) {
    if(!bGlobalInitialized) {
      s_global_sa_xlate_list[i] = 0;
    }
    sa_xlate_list[i] = 0;
  }
  bGlobalInitialized = true;
  if(enabled)
    Build();

}

bool    SourceBrowserAsm_Window::bGlobalInitialized = false;
GList * SourceBrowserAsm_Window::s_global_sa_xlate_list[SBAW_NRFILES];
int     SourceBrowserAsm_Window::s_totallinesheight[SBAW_NRFILES];
int SourceBrowserAsm_Window::m_SourceWindowCount = 0;

//========================================================================
//
// SourceBrowserParent_Window
//
// Here is some experimental code that allows multiple source browser
// windows.

SourceBrowserParent_Window::SourceBrowserParent_Window(GUI_Processor *_gp)
: GUI_Object()
{

  gp = _gp;
  set_name("source_browser_parent");

  pma = 0;
  m_TabType = GTK_POS_BOTTOM;
  mpTagTable = gtk_text_tag_table_new();

  const char *sName = "source_config";
  char *fg=0;

  mLabel    = new TextStyle("Label",
    config_get_string(sName, "label_fg", &fg) ? fg : "orange",
    "white");

  fg=0;
  mMnemonic = new TextStyle("Mnemonic",
    config_get_string(sName, "mnemonic_fg", &fg) ? fg : "red",
    "white");
  fg=0;
  mSymbol   = new TextStyle("Symbols",
    config_get_string(sName, "symbol_fg", &fg) ? fg : "dark green",
    "white");
  fg=0;
  mComment  = new TextStyle("Comments",
    config_get_string(sName, "comment_fg", &fg) ? fg : "dim gray",
    "white");
  fg=0;
  mConstant = new TextStyle("Constants",
    config_get_string(sName, "constant_fg", &fg) ? fg : "blue",
    "white");
  fg=0;
  mDefault  = new TextStyle("Default",
    "black",
    "white");

  if (!config_get_variable(sName, "tab_position", &m_TabType))
    m_TabType = GTK_POS_LEFT;
  int b=1;
  config_get_variable(sName, "line_numbers", &b);
  margin().enableLineNumbers(b!=0);

  config_get_variable(sName, "addresses", &b);
  margin().enableAddresses(b!=0);

  config_get_variable(sName, "opcodes", &b);
  margin().enableOpcodes(b!=0);

  mBreakpointTag   = new TextStyle("BreakPoint","black", "red");
  mNoBreakpointTag = new TextStyle("NoBreakPoint","black", "white");
  mCurrentLineTag  = new TextStyle("CurrentLine","black", "light green");

  gtk_text_tag_table_add (mpTagTable, mLabel->tag());
  gtk_text_tag_table_add (mpTagTable, mMnemonic->tag());
  gtk_text_tag_table_add (mpTagTable, mSymbol->tag());
  gtk_text_tag_table_add (mpTagTable, mComment->tag());
  gtk_text_tag_table_add (mpTagTable, mConstant->tag());
  gtk_text_tag_table_add (mpTagTable, mDefault->tag());

  gtk_text_tag_table_add (mpTagTable, mBreakpointTag->tag());
  gtk_text_tag_table_add (mpTagTable, mNoBreakpointTag->tag());
  gtk_text_tag_table_add (mpTagTable, mCurrentLineTag->tag());

  m_FontDescription = 0;
  if (config_get_string(sName, "font", &fg))
    setFont(fg);
  else
    setFont("Serif 8");

  ppSourceBuffers = new SourceBuffer *[SBAW_NRFILES];
  for (int i=0; i<SBAW_NRFILES; i++)
    ppSourceBuffers[i] = 0;
  children.push_back(new SourceWindow(_gp,this,true));
}

SOURCE_WINDOW *SourceBrowserParent_Window::getChild(int n)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;
  sbaw_iterator = children.begin();

  return (sbaw_iterator != children.end()) ? *sbaw_iterator : 0;
}

void SourceBrowserParent_Window::Build(void)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->Build();

  UpdateMenuItem();

}

void SourceBrowserParent_Window::NewProcessor(GUI_Processor *gp)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;
  list <ProgramMemoryAccess *> :: iterator pma_iterator;

  sbaw_iterator = children.begin();
  pma_iterator = gp->cpu->pma_context.begin();

  CreateSourceBuffers(gp);

  int child = 1;
  SOURCE_WINDOW *sbaw=0;
  while( (sbaw_iterator != children.end()) ||
    (pma_iterator != gp->cpu->pma_context.end()))
  {
    char child_name[64];
    if(sbaw_iterator == children.end())
    {
      child++;
      sprintf(child_name,"source_browser%d",child);
      sbaw = new SOURCE_WINDOW(gp,this, true, child_name);
      children.push_back(sbaw);
    }
    else
      sbaw = *sbaw_iterator++;

    if(pma_iterator != gp->cpu->pma_context.end())
    {
      sbaw->set_pma(*pma_iterator);
      pma_iterator++;
    }
    else
    {
      sbaw->set_pma(gp->cpu->pma);
    }

  }
}

void SourceBrowserParent_Window::SelectAddress(int address)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->SelectAddress(address);
}

void SourceBrowserParent_Window::SelectAddress(Value *addrSym)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->SelectAddress(addrSym);
}

void SourceBrowserParent_Window::Update(void)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->Update();
}

void SourceBrowserParent_Window::UpdateLine(int address)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->UpdateLine(address);
}

void SourceBrowserParent_Window::SetPC(int address)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->SetPC(address);
}

void SourceBrowserParent_Window::CloseSource(void)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->CloseSource();
}

void SourceBrowserParent_Window::NewSource(GUI_Processor *gp)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  CreateSourceBuffers(gp);
  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->NewSource(gp);
}

void SourceBrowserParent_Window::ChangeView(int view_state)
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->ChangeView(view_state);
}

int SourceBrowserParent_Window::set_config()
{
  list <SOURCE_WINDOW *> :: iterator sbaw_iterator;

  for (sbaw_iterator = children.begin();
    sbaw_iterator != children.end();
    sbaw_iterator++)
    (*sbaw_iterator)->set_config();

  const char *sName = "source_config";
  char buff[100];
  config_set_string(sName,"mnemonic_fg",mMnemonic->mFG.get(buff, sizeof(buff)));
  config_set_string(sName,"label_fg",mLabel->mFG.get(buff, sizeof(buff)));
  config_set_string(sName,"symbol_fg",mSymbol->mFG.get(buff, sizeof(buff)));
  config_set_string(sName,"comment_fg",mComment->mFG.get(buff, sizeof(buff)));
  config_set_string(sName,"constant_fg",mConstant->mFG.get(buff, sizeof(buff)));

  config_set_string(sName,"font", getFont());

  config_set_variable(sName, "tab_position", getTabPosition());
  config_set_variable(sName, "line_numbers", margin().bLineNumbers());
  config_set_variable(sName, "addresses", margin().bAddresses());
  config_set_variable(sName, "opcodes", margin().bOpcodes());


  return 0;
}

//------------------------------------------------------------------------
// parseLine
//
// Added a line of text to the source buffer. Apply syntax highlighting.
//

void SourceBuffer::parseLine(const char *cP,
                             int parseStyle)
{

  GtkTextIter iEnd;
  GtkTextBuffer *pTextBuffer = m_buffer;
  gtk_text_buffer_get_end_iter (pTextBuffer, &iEnd);

  int offset = gtk_text_iter_get_offset (&iEnd);

  gtk_text_buffer_insert (pTextBuffer, &iEnd, cP, -1);

  if (parseStyle<0) {
    addTagRange(m_pParent->mComment,offset,offset + strlen(cP));
    return;
  }

  int i=0;
  int j=0;
  bool bHaveMnemonic = false;

  if (i != (j = isString(cP))) {
    addTagRange(m_pParent->mLabel,i+offset,j+offset);
    i=j;
  }

  while (!isEnd(cP[i])) {

    if ( (j=isWhiteSpace(&cP[i])) != 0) {
      i += j;
    } else if ( (j=isString(&cP[i])) != 0) {
      if (bHaveMnemonic)
        addTagRange(m_pParent->mSymbol,i+offset,i+j+offset);
      else
        addTagRange(m_pParent->mMnemonic,i+offset,i+j+offset);
      bHaveMnemonic = true;
      i += j;
    } else if ( (j=isNumber(&cP[i])) != 0) {
      addTagRange(m_pParent->mConstant,i+offset,i+j+offset);
      i += j;
    } else if ( (j=isComment(&cP[i])) != 0) {
      addTagRange(m_pParent->mComment,i+offset,i+j+offset);
      i += j;
      return;
    } else
      i++;
  }
}

//------------------------------------------------------------------------
SourcePageMargin &SourceBrowserParent_Window::margin()
{
  return m_margin;
}

//------------------------------------------------------------------------
void SourceBrowserParent_Window::setTabPosition(int tt)
{
  m_TabType = tt;
  Update();
}

//------------------------------------------------------------------------
void SourceBrowserParent_Window::setFont(const char *cpNewFont)
{
  if (cpNewFont) {
    g_free(m_FontDescription);
    m_FontDescription = g_strndup(cpNewFont,256);
    Update();
  }
}
const char *SourceBrowserParent_Window::getFont()
{
  return m_FontDescription;
}

//------------------------------------------------------------------------
// parseSource
void SourceBrowserParent_Window::parseSource(SourceBuffer *pBuffer,FileContext *pFC)
{
  pFC->rewind();

  char text_buffer[256];
  int line = 1;

  while(pFC->gets(text_buffer, sizeof(text_buffer))) {

    int address;
    // The syntax highlighting doesn't work on list files or hll files
    address = pFC->IsList()||pFC->IsHLL() ? -1 : gp->cpu->pma->find_address_from_line(pFC,line);

    // check if text_buffer in uft8 character set
    if (!g_utf8_validate(text_buffer, -1, NULL))
    {
	gsize bytes_read, bytes_written;
	gchar *new_buffer;
	GError *Gerror = NULL;

	// try to convert to uft8 using current locale
	// if we succeed, do normal processing on converted text
	if ((new_buffer = g_locale_to_utf8((const gchar *)text_buffer, 
		-1, &bytes_read, &bytes_written, &Gerror)))
	{
        	pBuffer->parseLine(new_buffer,address);
		g_free(new_buffer);
	}
	// Conversion based on locale did not work
	else
	{
	    // replace comment which may have unknown character set
	    if((new_buffer = strchr(text_buffer, ';')))
	    {
		*new_buffer = 0;
		strcat(text_buffer, "; comment stripped, characters from unknown locale\n");
	    }
	    // if still not OK replace entire line so line numbering still OK
    	    if (!g_utf8_validate(text_buffer, -1, NULL))
	    {
		strcpy(text_buffer, "; non-comment characters from unknow locale\n");
	    }
            pBuffer->parseLine(text_buffer,address);
	}
    }
    else
        pBuffer->parseLine(text_buffer,address);

    line++;
  }

}

//------------------------------------------------------------------------
void SourceBrowserParent_Window::CreateSourceBuffers(GUI_Processor *gp)
{
  Dprintf((" \n"));

  int i;

  const char *file_name;

  if(!gp || !gp->cpu || !gp->cpu->pma)
    return;
  Dprintf((" \n"));

  Processor * pProc = gp->cpu;
  if(!pma)
    pma = pProc->pma;

  CloseSource();

  Dprintf(("NewSource\n"));

  if(pProc->files.nsrc_files() != 0) {

    for(i=0;i<pProc->files.nsrc_files();i++) {
      FileContext *fc = pProc->files[i];
      file_name = fc->name().c_str();
      int iNameLength = strlen(file_name);

      if(strcmp(file_name+iNameLength-4,".cod")
        && strcmp(file_name+iNameLength-4,".COD")
        && (i < SBAW_NRFILES) )
        ppSourceBuffers[i] = new SourceBuffer(mpTagTable, fc, this);

      else {
        if(verbose)
          printf ("SourceBrowserAsm_new_source: skipping file: <%s>\n",
          file_name);
      }
    }

  }


  Dprintf((" Source is loaded\n"));

}


#endif // HAVE_GUI
