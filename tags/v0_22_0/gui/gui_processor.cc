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
#include "gui_breadboard.h"
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

#include "gui_processor.h"

void create_dispatcher (void);

GUI_Processor::GUI_Processor(void)
{

  cpu = 0;
  m_pGUIRamRegisters = 0;
  m_pGUIEEPromRegisters = 0;

  create_dispatcher();

  regwin_ram =  new  RAM_RegisterWindow(this);
  regwin_eeprom = new  EEPROM_RegisterWindow(this);
  program_memory = new  SourceBrowserOpcode_Window(this);
  source_browser = new  SourceBrowserParent_Window(this);
  symbol_window = new  Symbol_Window(this);
  watch_window = new  Watch_Window(this);
  stack_window = new  Stack_Window(this);
  breadboard_window = new  Breadboard_Window(this);
  trace_window = new  Trace_Window(this);
  profile_window = new  Profile_Window(this);
  stopwatch_window = new  StopWatch_Window(this);
  scope_window = new  Scope_Window(this);
}

void GUI_Processor::SetCPU(Processor *new_cpu) {
  cpu = new_cpu;
  if(m_pGUIRamRegisters) {
    delete m_pGUIRamRegisters;
  }
  m_pGUIRamRegisters = new GUIRegisterList(&new_cpu->rma);
  if(m_pGUIEEPromRegisters) {
    delete m_pGUIEEPromRegisters;
  }
  m_pGUIEEPromRegisters = new GUIRegisterList(&new_cpu->ema);
}

#endif // HAVE_GUI
