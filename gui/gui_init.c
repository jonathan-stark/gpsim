

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>

#include <gdk/gdktypes.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

//#include <iostream.h>

#include <gtkextra/gtkcombobox.h>
#include <gtkextra/gtkbordercombo.h>
#include <gtkextra/gtkcolorcombo.h>
#include <gtkextra/gtksheet.h>
//#include <gtkextra/gtksheetentry.h>


#include "gui.h"

//
// 'styles' - 
//
GtkStyle *normal_style;
GtkStyle *current_line_number_style;
GtkStyle *breakpoint_line_number_style;

GdkColor item_has_changed_color;
GdkColor normal_fg_color;
GdkColor normal_bg_color;
GdkColor sfr_bg_color;
GdkColor breakpoint_color;
GdkColor alias_color;
GdkColor invalid_color;
GdkColor black_color;
GdkColor pm_has_changed_color;
GdkColor normal_pm_bg_color;
GdkColor high_output_color;
GdkColor low_output_color;

void gui_styles_init(void)
{
  GdkColor text_fg;
  GdkColor text_bg;
  
	GdkColormap *colormap = gdk_colormap_get_system();


  //
  // Initialize the styles used by the gui
  // The styles define the 'look-and-feel' for the text
  // output. For now they're hard coded. But since I've
  // picked the best colors and font, I don't think anyone
  // would ever want to change it :).
  //

  // normal style:
  //  black adobe text on a light cyan background

  gdk_color_parse("black", &text_fg);
  gdk_color_parse("light cyan", &text_bg);
  normal_fg_color = text_fg;
  normal_bg_color = text_bg;

  gdk_color_parse("cyan", &sfr_bg_color);
  
  normal_style = gtk_style_new ();
  normal_style->fg[GTK_STATE_NORMAL] = text_fg;
  normal_style->base[GTK_STATE_NORMAL] = text_bg;

  gdk_font_unref (normal_style->font);
  normal_style->font =
    //    gdk_font_load ("-adobe-courier-*-r-*-*-*-140-*-*-*-*-*-*");
    gdk_font_load ("-schumacher-clean-medium-r-normal-*-*-160-*-*-c-*-iso646.1991-irv");

  // Current line number style
  //   bold,black adobe text on a gray background

  text_bg.red   = 30000;
  text_bg.green = 30000;
  text_bg.blue  = 30000;

  current_line_number_style = gtk_style_new ();
  current_line_number_style->fg[GTK_STATE_NORMAL] = text_fg;
  current_line_number_style->base[GTK_STATE_NORMAL] = text_bg;

  gdk_font_unref (current_line_number_style->font);
  current_line_number_style->font =
    gdk_font_load ("-adobe-courier-bold-r-*-*-*-140-*-*-*-*-*-*");

  // Break point style
  //   bold,black adobe text on a red background

  gdk_color_parse("red", &text_bg);

  breakpoint_line_number_style = gtk_style_new ();
  breakpoint_line_number_style->fg[GTK_STATE_NORMAL] = text_fg;
  breakpoint_line_number_style->base[GTK_STATE_NORMAL] = text_bg;
  breakpoint_color = text_bg;

  gdk_font_unref (breakpoint_line_number_style->font);
  breakpoint_line_number_style->font =
    gdk_font_load ("-adobe-courier-bold-r-*-*-*-140-*-*-*-*-*-*");
      
  gdk_color_parse("blue", &item_has_changed_color);
  gdk_color_parse("light gray", &alias_color);
  invalid_color=normal_fg_color;

  gdk_color_parse("red",&high_output_color);
  gdk_color_parse("green",&low_output_color);
  
  g_assert(gdk_color_parse("black",&black_color)!=FALSE);
  
  gdk_color_parse("white",&normal_pm_bg_color);
  gdk_color_parse("light gray",&pm_has_changed_color);
  
	gdk_colormap_alloc_color(colormap, &normal_bg_color,FALSE,TRUE );
	gdk_colormap_alloc_color(colormap, &normal_fg_color,FALSE,TRUE );
	gdk_colormap_alloc_color(colormap, &item_has_changed_color,FALSE,TRUE);
	gdk_colormap_alloc_color(colormap, &breakpoint_color,FALSE,TRUE);
	gdk_colormap_alloc_color(colormap, &alias_color,FALSE,TRUE);
	gdk_colormap_alloc_color(colormap, &invalid_color,FALSE,TRUE);
	gdk_colormap_alloc_color(colormap, &sfr_bg_color,FALSE,TRUE);
	gdk_colormap_alloc_color(colormap, &high_output_color,FALSE,TRUE);
	gdk_colormap_alloc_color(colormap, &low_output_color,FALSE,TRUE);
	gdk_colormap_alloc_color(colormap, &black_color,FALSE,TRUE);
	gdk_colormap_alloc_color(colormap, &normal_pm_bg_color,FALSE,TRUE);
	gdk_colormap_alloc_color(colormap, &pm_has_changed_color,FALSE,TRUE);

	
	//puts("Initialized");

}
#endif // HAVE_GUI
