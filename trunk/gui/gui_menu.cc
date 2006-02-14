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

extern GUI_Processor *gp;

static void 
do_quit_app(GtkWidget *widget) 
{
	exit_gpsim();
}


static void
show_message (char *title, char *message)
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
		 "gpsim homepage: http://www.dattalo.com/gnupic/gpsim.html\n"
		 "gpsimWin32: http://gpsim.sourceforge.net/gpsimWin32/gpsimWin32.html\n");

}


//========================================================================

// class ColorSelection 
//
// A class to keep track of the state of configurable colors.
//
class ColorSelection
{
public:
  ColorSelection(GdkColor *pC)
    : m_current(pC)
  {
    m_preferred=0;
    setPreferred(pC);
  }
  bool hasChanged()    { return gdk_color_equal(m_current,m_preferred) != TRUE;}
  GdkColor *getPreferred() { return m_preferred;}
  const char *getPreferred(char *);
  void setPreferred(GdkColor *cP_preferred) 
  { 
    if (m_preferred)
      gdk_color_free(m_preferred);
    m_preferred = gdk_color_copy(cP_preferred);
  }
protected:
  GdkColor *m_current;
  GdkColor *m_preferred;
};


// class ColorButton
//
// Creates a GtkColorButton and places it into a parent widget.
// When the color button is clicked and changed, the signal will
// call back into this class and keep track of the selected 
// color state.
class SourceBrowserPreferences;
class ColorButton : public ColorSelection
{
public:
  ColorButton (GtkWidget *pParent, 
	       GdkColor *pC, 
	       const char *label,
	       SourceBrowserPreferences *
	       );
  static void setColor_cb(GtkColorButton *widget,
			  ColorButton    *This);
  void apply();
  void addTagRange(int start_index, int end_index);
  void setBuffer(GtkTextBuffer *pBuffer)
  {
    m_buffer = pBuffer; 
    char cParray[20];
    m_tag = gtk_text_buffer_create_tag (pBuffer, m_label,
					"foreground",
					getPreferred(cParray),
					NULL);


  }
private:
  SourceBrowserPreferences *m_prefs;
  GtkTextTag *m_tag;
  const char *m_label;
  GtkTextBuffer *m_buffer;
};

class SourceBrowserPreferences
{
public:
  SourceBrowserPreferences(GtkWidget *pParent);

  void apply();
  void update();
  void parseLine(const char*);
  void parseLine(int opcode, const char*);
  void toggleBreak(int line);

private:
  void set_style_colors(const char *fg_color, const char *bg_color, GtkStyle **style);

  GtkTextView *m_view;
  GtkLayout   *m_layout;
  ColorButton *m_LabelColor;
  ColorButton *m_MnemonicColor;
  ColorButton *m_SymbolColor;
  ColorButton *m_CommentColor;
  ColorButton *m_ConstantColor;

  GtkStyle *symbol_text_style;       // for symbols in .asm display
  GtkStyle *label_text_style;        // for label in .asm display
  GtkStyle *instruction_text_style;  // for instruction in .asm display
  GtkStyle *number_text_style;       // for numbers in .asm display
  GtkStyle *comment_text_style;      // for comments in .asm display
  GtkStyle *linenumber_text_style;   // for line numbers and opcodes in .asm display
  GtkStyle *default_text_style;      // for everything else.

  GtkTextTag *m_BreakpointTag;
  GtkTextTag *m_NoBreakpointTag;

  GtkWidget   *m_SampleNotebook;
  GtkPositionType m_TabPosition;
};

//------------------------------------------------------------------------
class gpsimGuiPreferences
{
public:
  gpsimGuiPreferences();
  ~gpsimGuiPreferences();

  void apply();

  static void setup (gpointer             callback_data,
		     guint                callback_action,
		     GtkWidget           *widget);


private:
  SourceBrowserPreferences *m_SourceBrowser;

  static gint cancel_cb (gpsimGuiPreferences *Self);

  static gint apply_cb  (gpsimGuiPreferences *Self);

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
  printf (" cancel preferences %p \n",Self);
  delete Self;
  return TRUE;
}
gint gpsimGuiPreferences::apply_cb  (gpsimGuiPreferences *Self)
{
  printf (" apply preferences\n");
  Self->apply();
  delete Self;
  return TRUE;
}

//========================================================================
struct sTest
{
  const char *cName;
  int id;
};

static void setFont_cb (GtkWidget     *pFontButton,
			gpointer      callback_data)
{

  sTest *s = (sTest *) callback_data;
  if (s)
    printf("setFont_cb %s %d\n", s->cName, s->id);

  printf("Font %s\n", gtk_font_button_get_font_name (GTK_FONT_BUTTON(pFontButton)));
}

//------------------------------------------------------------------------
// ColorButton Constructor
ColorButton::ColorButton(GtkWidget *pParent, GdkColor *pColor, 
			 const char *colorName,SourceBrowserPreferences *prefs)
  : ColorSelection(pColor), m_prefs(prefs), m_label(colorName)
{
  GtkWidget *hbox        = gtk_hbox_new(0,0);
  gtk_box_pack_start (GTK_BOX (pParent), hbox, FALSE, TRUE, 0);

  GtkWidget *colorButton = gtk_color_button_new_with_color (pColor);
  gtk_color_button_set_title (GTK_COLOR_BUTTON(colorButton), colorName);
  gtk_box_pack_start (GTK_BOX(hbox),colorButton,FALSE, FALSE, 0);
  gtk_widget_show(colorButton);

  gtk_signal_connect (GTK_OBJECT(colorButton), 
		      "color-set", 
		      GTK_SIGNAL_FUNC(setColor_cb),
		      this);
  printf("create color button %s %p %p\n",colorName,colorButton,this);
  const int cBORDER = 10; // pixels
  GtkWidget *label       = gtk_label_new(colorName);
  gtk_box_pack_start (GTK_BOX(hbox),label,TRUE, TRUE, cBORDER);
  gtk_widget_show (label);

  gtk_widget_show (hbox);


}
//------------------------------------------------------------------------
void ColorButton::setColor_cb(GtkColorButton *widget,
			      ColorButton    *This)
{
  printf("setColor_cb  %p %p\n",widget,This);
  GdkColor newColor;
  gtk_color_button_get_color (widget, &newColor);
  This->setPreferred(&newColor);
  This->apply();
}
const char *ColorSelection::getPreferred(char *cParray)
{
  sprintf(cParray,"#%04X%04X%04X",
	  m_preferred->red, m_preferred->green, m_preferred->blue);
  return cParray;
}

bool isButtonEvent (GdkEventType type)
{
  return 
    type == GDK_BUTTON_PRESS ||
    type == GDK_2BUTTON_PRESS ||
    type == GDK_3BUTTON_PRESS ||
    type == GDK_BUTTON_RELEASE;

}
gboolean    TagEvent  (GtkTextTag *texttag,
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
// addTagRange(int start_index, int end_index)
//
// Each color button has an associated buffer and tag. 
// Addtag range applies the tag state to a range of text in the buffer
void ColorButton::addTagRange(int start_index, int end_index)
{
  GtkTextIter    start;
  GtkTextIter    end;
  gtk_text_buffer_get_iter_at_offset (m_buffer, &start, start_index);
  gtk_text_buffer_get_iter_at_offset (m_buffer, &end, end_index);

  gtk_text_buffer_apply_tag (m_buffer, m_tag, &start, &end);

  g_signal_connect (G_OBJECT (m_tag), "event",
		    GTK_SIGNAL_FUNC(TagEvent),
		    this);
  printf("Added TagRange event for %p\n",this);

}

void ColorButton::apply()
{
  char array[20];
  g_object_set(m_tag, "foreground" , getPreferred(array), NULL);
}
//------------------------------------------------------------------------

static void preferences_AddFontSelect(GtkWidget *pParent, const char *fontDescription,
				      const char *fontName  )
{
  GtkWidget *frame = gtk_frame_new ("Font");

  gtk_box_pack_start (GTK_BOX (pParent), frame, FALSE, TRUE, 0);
  GtkWidget *hbox        = gtk_hbox_new(0,0);
  //gtk_box_pack_start (GTK_BOX (pParent), hbox, FALSE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  //GtkWidget *fontButton = gtk_font_button_new_with_font (fontName);
  GtkWidget *fontButton = gtk_font_button_new ();
  gtk_font_button_set_title (GTK_FONT_BUTTON(fontButton), fontDescription);
  gtk_box_pack_start (GTK_BOX(hbox),fontButton,FALSE, FALSE, 0);
  gtk_widget_show(fontButton);

  sTest *s = new sTest();
  s->cName = fontDescription;
  s->id = 42;

  gtk_signal_connect (GTK_OBJECT(fontButton), 
		      "font-set", 
		      GTK_SIGNAL_FUNC(setFont_cb),
		      s);

  const int cBORDER = 10; // pixels
  GtkWidget *label       = gtk_label_new(fontName);
  gtk_box_pack_start (GTK_BOX(hbox),label,TRUE, TRUE, cBORDER);
  gtk_widget_show (label);

  gtk_widget_show (hbox);
}


//========================================================================
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

//#include "../xpms/break.xpm"
static char * break_xpm[] = {
"12 12 3 1",
" 	c None",
".	c gray",
"X	c white",
"   .......    ",
"  .........   ",
" ...........  ",
" ...XXXXX.... ",
" ...XXXXX.... ",
" ...XXXXX.... ",
" ...XXXXX.... ",
" ...XXXXX.... ",
" ...........  ",
"  .........   ",
"   .......    ",
"              "};
void SourceBrowserPreferences::toggleBreak(int line)
{

  //GtkWidget *widget = gtk_image_new_from_stock (GTK_STOCK_GO_FORWARD,
  //						  GTK_ICON_SIZE_BUTTON);
  //GtkWidget *widget = gtk_check_button_new();

  static GdkPixmap *pixmap_break = 0;
  static GdkBitmap *bp_mask = 0;

  if (pixmap_break == 0) {
    GtkStyle  *style;

    style=gtk_style_new();

    pixmap_break = gdk_pixmap_create_from_xpm_d(LocalWindow->window,
						&bp_mask,
						&style->bg[GTK_STATE_NORMAL],
						(gchar**)break_xpm);
    GtkWidget *widget = gtk_image_new_from_pixmap (pixmap_break,
						   bp_mask);
    /*
    GtkTextChildAnchor *anchor =
      gtk_text_buffer_create_child_anchor (buffer, &iEnd);

    gtk_text_view_add_child_at_anchor (m_view,
				       widget,
				       anchor);
    */

    gtk_widget_show_all (widget);
  }

  if (m_view) {
    GtkTextBuffer *buffer = gtk_text_view_get_buffer (m_view);
    GtkTextIter iBegin;
    GtkTextIter iEnd;

    gtk_text_buffer_get_end_iter (buffer, &iEnd);
    gtk_text_buffer_get_iter_at_line(buffer, &iBegin, line);
    #define STRLEN_OF_LINENUMBER_AND_OPCODE 10
    gtk_text_buffer_get_iter_at_line_offset(buffer, &iEnd, line, STRLEN_OF_LINENUMBER_AND_OPCODE);
    gtk_text_buffer_apply_tag (buffer, m_BreakpointTag, &iBegin, &iEnd);

  }
}


void SourceBrowserPreferences::parseLine(int opcode, const char *cP)
{
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (m_view);
  GtkTextIter iEnd;

  gtk_text_buffer_get_end_iter (buffer, &iEnd);

  char buf[64];
  int line_number = gtk_text_buffer_get_line_count(buffer);

  if (opcode > 0) 
    snprintf(buf, sizeof(buf), "%5d %04X ",line_number,opcode);
  else
    snprintf(buf, sizeof(buf), "%5d %s ",line_number,"    ");

  //gtk_text_buffer_insert_with_tags_by_name (buffer, &iEnd, buf, -1, "margin", NULL);
  gtk_text_buffer_insert (buffer, &iEnd, buf, -1);

  int offset = gtk_text_iter_get_offset (&iEnd);

  //gtk_text_buffer_insert_with_tags_by_name (buffer, &iEnd, cP, -1, "margin", NULL);
  gtk_text_buffer_insert (buffer, &iEnd, cP, -1);

  int i=0;
  int j=0;
  bool bHaveMnemonic = false;

  if (i != (j = isString(cP))) {
    //printf ("label %d:%d %d\n",i,j,offset);
    m_LabelColor->addTagRange(i+offset,j+offset);
    i=j;
  }
  while (!isEnd(cP[i])) {

    if ( (j=isWhiteSpace(&cP[i])) != 0) {
      //printf ("White space %d:%d\n",i,i+j);
      i += j;
    } else if ( (j=isString(&cP[i])) != 0) {
      if (bHaveMnemonic)
	m_SymbolColor->addTagRange(i+offset,i+j+offset);
      else
	m_MnemonicColor->addTagRange(i+offset,i+j+offset);
      bHaveMnemonic = true;
      i += j;
    } else if ( (j=isNumber(&cP[i])) != 0) {
      m_ConstantColor->addTagRange(i+offset,i+j+offset);
      i += j;
    } else if ( (j=isComment(&cP[i])) != 0) {
      m_CommentColor->addTagRange(i+offset,i+j+offset);
      i += j;
      return;
    } else 
      i++;
  }
}
void SourceBrowserPreferences::parseLine(const char *cP)
{
  parseLine(0, cP);
}
//========================================================================
SourceBrowserPreferences::SourceBrowserPreferences(GtkWidget *pParent)
{

  if (!gp && !gp->source_browser)
    return;
  SourceBrowserAsm_Window *sbaw = gp->source_browser->getChild(0);
  if (!sbaw)
    return;

  GtkWidget *hbox2 = gtk_hbox_new(0,0);
  gtk_box_pack_start (GTK_BOX (pParent), hbox2, FALSE, TRUE, 0);

  GtkWidget *vbox3 = gtk_vbox_new(0,0);
  gtk_box_pack_start (GTK_BOX (hbox2), vbox3, FALSE, TRUE, 0);

  {
    // Color Frame for Source Browser configuration
    GtkWidget *colorFrame = gtk_frame_new ("Colors");
    gtk_box_pack_start (GTK_BOX (hbox2), colorFrame, FALSE, TRUE, 0);

    GtkWidget *colorVbox = gtk_vbox_new(0,0);
    gtk_container_add (GTK_CONTAINER (colorFrame), colorVbox);

    set_style_colors("black", "white", &default_text_style);
    set_style_colors("dark green", "white", &symbol_text_style);
    set_style_colors("orange", "white", &label_text_style);
    set_style_colors("red", "white", &instruction_text_style);
    set_style_colors("blue", "white", &number_text_style);
    set_style_colors("dim gray", "white", &comment_text_style);

    m_LabelColor    = new ColorButton(GTK_WIDGET(colorVbox), 
				      &label_text_style->fg[GTK_STATE_NORMAL],
				      "Label", this);
    m_MnemonicColor = new ColorButton(GTK_WIDGET(colorVbox), 
				      &instruction_text_style->fg[GTK_STATE_NORMAL],
				      "Mnemonic", this);
    m_SymbolColor   = new ColorButton(GTK_WIDGET(colorVbox), 
				      &symbol_text_style->fg[GTK_STATE_NORMAL],
				      "Symbols", this);
    m_ConstantColor = new ColorButton(GTK_WIDGET(colorVbox),
				      &number_text_style->fg[GTK_STATE_NORMAL],
				      "Constants", this);
    m_CommentColor  = new ColorButton(GTK_WIDGET(colorVbox),
				      &comment_text_style->fg[GTK_STATE_NORMAL],
				      "Comments", this);



  }

  {
    // Tab Frame for the Source browser
    m_TabPosition = GTK_POS_TOP;
    GtkWidget *tabFrame = gtk_frame_new ("Tabs");
    gtk_box_pack_start (GTK_BOX (vbox3), tabFrame, FALSE, TRUE, 0);

    GtkWidget *radioUp    = gtk_radio_button_new_with_label (NULL,"up");
    GtkRadioButton *rb    = GTK_RADIO_BUTTON(radioUp);
    GtkWidget *radioLeft  = gtk_radio_button_new_with_label_from_widget (rb,"left");
    GtkWidget *radioDown  = gtk_radio_button_new_with_label_from_widget (rb,"down");
    GtkWidget *radioRight = gtk_radio_button_new_with_label_from_widget (rb,"right");
    GtkWidget *radioNone  = gtk_radio_button_new_with_label_from_widget (rb,"none");

    GtkWidget *tabVbox = gtk_vbox_new(0,0);
    gtk_container_add (GTK_CONTAINER (tabFrame), tabVbox);
    
    gtk_box_pack_start (GTK_BOX (tabVbox), radioUp,   FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (tabVbox), radioLeft, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (tabVbox), radioDown, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (tabVbox), radioRight,FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (tabVbox), radioNone, FALSE, TRUE, 0);

  }

  preferences_AddFontSelect(GTK_WIDGET(vbox3), "Mnemonic", "font");

  {

    GtkTextBuffer *buffer;
    PangoFontDescription *font_desc;

    m_view = GTK_TEXT_VIEW (gtk_text_view_new ());
    gtk_text_view_set_wrap_mode (m_view, GTK_WRAP_NONE);
    gtk_text_view_set_editable  (m_view, FALSE);

    /* Change default font throughout the widget */
    font_desc = pango_font_description_from_string ("Courier 12");
    gtk_widget_modify_font (GTK_WIDGET (m_view), font_desc);
    pango_font_description_free (font_desc);

    buffer = gtk_text_view_get_buffer (m_view);

    m_LabelColor->setBuffer(buffer);
    m_MnemonicColor->setBuffer(buffer);
    m_CommentColor->setBuffer(buffer);
    m_SymbolColor->setBuffer(buffer);
    m_ConstantColor->setBuffer(buffer);

    gtk_text_buffer_create_tag (buffer, "margin",
				"left_margin", 50,
				NULL);

    m_BreakpointTag = gtk_text_buffer_create_tag (buffer, "Breakpoint",
						  "foreground","black",
						  "background","red",
						  NULL);
    m_NoBreakpointTag = gtk_text_buffer_create_tag (buffer, "NoBreakpoint",
						    "foreground","black",
						    "background","white",
						    NULL);

    g_signal_connect (G_OBJECT (m_BreakpointTag), "event",
		      GTK_SIGNAL_FUNC(TagEvent),
		      0);
    g_signal_connect (G_OBJECT (m_NoBreakpointTag), "event",
		      GTK_SIGNAL_FUNC(TagEvent),
		      0);

    parseLine(0x1234,"Label: MOVF    Temp1,W ;Comment\n");
    parseLine(0xabcd,"       MOVLW   0x42    ;Comment\n");
    parseLine("; Line only with a comment\n");


    m_LabelColor->apply();
    m_MnemonicColor->apply();
    m_CommentColor->apply();
    m_SymbolColor->apply();
    m_ConstantColor->apply();



    GtkWidget *frame = gtk_frame_new ("Sample");
    gtk_box_pack_start (GTK_BOX (pParent), frame, FALSE, TRUE, 0);
    
    m_SampleNotebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos((GtkNotebook*)m_SampleNotebook,m_TabPosition);

    gtk_container_add (GTK_CONTAINER (frame), m_SampleNotebook);




    const int PAGE_BORDER = 3;
    const int PIXMAP_SIZE = 14;
    GtkWidget *hbox = gtk_hbox_new(0,0);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), PAGE_BORDER);

    GtkAdjustment *source_layout_adj = (GtkAdjustment*)gtk_adjustment_new(0.0,0.0,0.0,0.0,0.0,0.0);
    m_layout = GTK_LAYOUT(gtk_layout_new(0,source_layout_adj));
    gtk_widget_set_usize(GTK_WIDGET(m_layout),PIXMAP_SIZE*2,0);
    gtk_widget_show(GTK_WIDGET(m_layout));

    toggleBreak(2);

    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(m_layout),
		       FALSE,FALSE, 0);

    gtk_box_pack_start_defaults(GTK_BOX(hbox), GTK_WIDGET(m_view));

    gtk_widget_show(hbox);


    GtkWidget *label = gtk_label_new("file1.asm");
    gtk_notebook_append_page(GTK_NOTEBOOK(m_SampleNotebook),
			     hbox,
			     label);

    label = gtk_label_new("file2.asm");
    GtkWidget *emptyBox = gtk_hbox_new(0,0);
    gtk_notebook_append_page(GTK_NOTEBOOK(m_SampleNotebook),emptyBox,label);



  }


  gtk_widget_show_all(hbox2);
  gtk_widget_show_all(vbox3);


}

void SourceBrowserPreferences::apply()
{
  printf ("apply source browser preferences\n");
}

void SourceBrowserPreferences::set_style_colors(const char *fg_color, const char *bg_color, GtkStyle **style)
{
  GdkColor text_fg;
  GdkColor text_bg;

  gdk_color_parse(fg_color, &text_fg);
  gdk_color_parse(bg_color, &text_bg);
  *style = gtk_style_new();
  (*style)->base[GTK_STATE_NORMAL] = text_bg;
  (*style)->fg[GTK_STATE_NORMAL] = text_fg;

}

//========================================================================
gpsimGuiPreferences::gpsimGuiPreferences()
{
  GtkWidget *button;

  LocalWindow = gtk_dialog_new ();
  window = LocalWindow;
  gtk_widget_show (window);

  gtk_window_set_title (GTK_WINDOW (window), "Preferences ***EXPERIMENTAL***");
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);

  GtkWidget *vbox = GTK_DIALOG (window)->vbox;

  GtkWidget *notebook = gtk_notebook_new();
  gtk_notebook_set_tab_pos((GtkNotebook*)notebook,GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show(notebook);

  GtkWidget *label;
  GtkWidget *vbox2;


  // source browser preferences...
  
  vbox2 = gtk_vbox_new(0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 3);
  label = gtk_label_new("Source Browser");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox2,label);

  m_SourceBrowser = new SourceBrowserPreferences(vbox2);
  gtk_widget_show_all(vbox2);


  vbox2 = gtk_vbox_new(0,0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 3);
  label=gtk_label_new("RAM");
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),vbox2,label);
  gtk_widget_show(vbox2);

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
  gtk_grab_add (window);

  printf("Created preferences %p %p %s\n",this, window, gtk_widget_get_name(window));
}

gpsimGuiPreferences::~gpsimGuiPreferences()
{

  printf("Destroying preferences %p %p\n",this, window);

  gtk_widget_destroy (window);

  //delete m_SourceBrowser;
}

void gpsimGuiPreferences::apply()
{
  printf ("apply gui preferences\n");
}

//========================================================================
void
file_selection_hide_fileops (GtkWidget *widget,
			     GtkFileSelection *fs)
{
  gtk_file_selection_hide_fileop_buttons (fs);
}

extern int gui_message(char *message);

void
file_selection_ok (GtkWidget        *w,
		   GtkFileSelection *fs)
{

  const char *file;
  char msg[200];

  if(gp)
  {
    file=gtk_file_selection_get_filename (fs);
    if(!gpsim_open(gp->cpu, file, NULL))
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
  if(gp && menu_item) {
    
    int view_state =  GTK_CHECK_MENU_ITEM(menu_item)->active ? VIEW_SHOW : VIEW_HIDE;
			
    switch(callback_action) {
    case WT_opcode_source_window:
      gp->program_memory->ChangeView(view_state);
      break;
    case WT_asm_source_window:
      gp->source_browser->ChangeView(view_state);
      break;
    case WT_register_window:
      gp->regwin_ram->ChangeView(view_state);
      break;
    case WT_eeprom_window:
      gp->regwin_eeprom->ChangeView(view_state);
      break;
    case WT_watch_window:
      gp->watch_window->ChangeView(view_state);
      break;
    case WT_symbol_window:
      gp->symbol_window->ChangeView(view_state);
      break;
    case WT_breadboard_window:
      gp->breadboard_window->ChangeView(view_state);
      break;
    case WT_stack_window:
      gp->stack_window->ChangeView(view_state);
      break;
    case WT_trace_window:
      gp->trace_window->ChangeView(view_state);
      break;
    case WT_profile_window:
      gp->profile_window->ChangeView(view_state);
      break;
    case WT_stopwatch_window:
      gp->stopwatch_window->ChangeView(view_state);
      break;
    case WT_scope_window:
      //gp->scope_window->ChangeView(view_state);
      cout << " The Scope is disabled right now\n";
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
  if(gp && gp->cpu)
    gp->cpu->pma->run();
}

static void 
stopbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu)
    gp->cpu->pma->stop();
}
    
static void 
stepbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu) 
    gp->cpu->pma->step(1);
}
    
static void 
overbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu) 
    gp->cpu->pma->step_over();

}
    
static void 
finishbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu) 
    gp->cpu->pma->finish();
}

static void 
resetbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu)
    gp->cpu->reset(POR_RESET);
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

  if(gp && gp->cpu)
    gp->cpu->pma->stop();

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
  void Format(char *buf, int size)
  {
    double time_db = gp->cpu->get_InstPeriod() * get_cycles().value * 1e6;
    snprintf(buf,size, "%19.2f us",time_db);
  }
};

class TimeMilliSeconds : public TimeFormatter
{
public:
  TimeMilliSeconds(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"MilliSeconds") {}
  void Format(char *buf, int size)
  {
    double time_db = gp->cpu->get_InstPeriod() * get_cycles().value * 1e3;
    snprintf(buf,size, "%19.3f ms",time_db);
  }
};

class TimeSeconds : public TimeFormatter
{
public:
  TimeSeconds(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"Seconds") {}
  void Format(char *buf, int size)
  {
    double time_db = gp->cpu->get_InstPeriod() * get_cycles().value;
    snprintf(buf,size, "%19.3f Sec",time_db);
  }
};

class TimeHHMMSS : public TimeFormatter
{
public:
  TimeHHMMSS(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"HH:MM:SS.mmm") {}
  void Format(char *buf, int size)
  {
    double time_db = gp->cpu->get_InstPeriod() * get_cycles().value;
    double v=time_db;
    int hh=(int)(v/3600),mm,ss,cc;
    v-=hh*3600.0;
    mm=(int)(v/60);
    v-=mm*60.0;
    ss=(int)v;
    cc=(int)(v*100.0+0.5);
    snprintf(buf,size,"    %02d:%02d:%02d.%02d",hh,mm,ss,cc);
  }
};

class TimeCyclesHex : public TimeFormatter
{
public:
  TimeCyclesHex(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"Cycles (Hex)") {}
  void Format(char *buf, int size)
  {
    snprintf(buf,size,"0x%016" PRINTF_INT64_MODIFIER "x",get_cycles().value);
  }
};

class TimeCyclesDec : public TimeFormatter
{
public:
  TimeCyclesDec(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"Cycles (Dec)") {}
  void Format(char *buf, int size)
  {
    snprintf(buf,size,"%016" PRINTF_INT64_MODIFIER "d",get_cycles().value);
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

  //  { "/_Processor",     	 0,       0,               0, "<Branch>" },
  //  { "/_Processor/New",   0,       (GtkItemFactoryCallback)new_processor_dialog,       0 },
  //  { "/_Processor/Delete",0,       (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  //  { "/_Processor/Switch",0,       (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },


  //  { "/_Break",     	 0, 0,               0, "<Branch>" },
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

  { "/_Edit",     0, 0,       0, "<Branch>" },
  { "/Edit/Preferences",        0, (GtkItemFactoryCallback)gpsimGuiPreferences::setup, 0 },

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

  if(gp && gp->cpu) {

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
