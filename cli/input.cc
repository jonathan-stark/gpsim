/*
   Copyright (C) 1998 T. Scott Dattalo

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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include <string>
#include <list>
#include <csignal>
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#include "fd2raw.h"
#else
#include <sys/file.h>
#include <unistd.h>
#endif

#ifdef HAVE_GUI
#include <glib.h>
#include <gdk/gdktypes.h>
#include <gdk/gdk.h>
#endif

#include "../src/gpsim_def.h"
#include "../src/gpsim_classes.h"
#include "../src/icd.h"
#include "../src/pic-processor.h"
#include "../src/breakpoints.h"
#include "command.h"
#include "input.h"
#include "cmd_macro.h"

#ifdef HAVE_LIBREADLINE
#define HAVE_READLINE
#endif

#ifdef HAVE_READLINE
/* See if we have namespace-clean readline or not */
#ifdef HAVE_RL_COMPLETION_MATCHES
#define HAVE_NSCLEAN_READLINE
#endif

extern "C" {
#include <readline/readline.h>
#include <readline/history.h>
}
#endif /* HAVE_READLINE */


extern const char *get_dir_delim(const char *path);
extern bool bUseGUI;

int yyparse(void);
void initialize_readline (void);

void exit_gpsim(void);
#ifdef HAVE_GUI
void quit_gui(void);
#endif

void redisplay_prompt(void);

char *gnu_readline (char *s, unsigned int force_readline);

int input_mode = 0;
int last_command_is_repeatable=0;
extern Macro *gCurrentMacro;

extern int quit_parse;

extern void start_server(void);
extern void stop_server(void);

/*
  temporary --- linked list input buffer
*/
class LLInput {
public:
  LLInput *next;
  void *data;
  Macro *macro;  // macro generating this text

  LLInput();
  LLInput(char *,Macro *);
  ~LLInput();
};

LLInput::LLInput()
  : next(0), data(0) 
{
}

LLInput::LLInput(char *s,Macro *m)
  : next(0), macro(m)
{
  data = strdup(s);
}

LLInput::~LLInput()
{
  if(data)
    free(data);
}

//************************************************************************
class LLStack : public LLInput
{
public:
  LLStack();

  void Push();
  void Pop();
  void Append(char *, Macro *);
  LLInput *GetNext();

  void print();
};

LLStack::LLStack()
{

}

static LLStack Stack;

//====================================================================================
//
// catch_control_c
//
//  
void catch_control_c(int sig)
{

  //if(simulation_mode != STOPPED)
  //  {
      cout << "<CTRL C> break\n";
      bp.halt();
  //  }
  //else {
  //  cout << "caught control c, but it doesn't seem gpsim was simulating\n";
  //  last_command_is_repeatable=0;
  //  redisplay_prompt();

  //}

}


void initialize_signals(void)
{
#ifndef _WIN32
  static struct sigaction action;

  action.sa_handler = catch_control_c;
  sigemptyset(&action.sa_mask);
  action.sa_flags=0;

  sigaction(SIGINT, &action, 0);
#endif

}

//==============================================================
// initialize_gpsim 
//
// Not much initialization is needed now. 
//

void initialize_gpsim(void)
{

  initialize_signals();
  start_server();
}


void LLStack::print(void)
{
#if 0
  LLInput *s = &Stack;

  cout << "Current state of input buffer:\n";
  int stack_number=0;
  while (s->data) {
    LLInput *h = (LLInput *)s->data;
    int depth =0;
    while(h) {
      
      cout << "   " <<stack_number <<':'<<depth << "  "<< (char *)  h->data;
      depth++;
      h = h->next;
    }
    stack_number++;
    s = s->next;
  }
  cout << "\n ---Leaving dump \n";
#endif
}

void LLStack::Push(void)
{
  LLInput *s = new LLInput();

  print();
  if(s) {
    s->next = next;
    s->data = data;
    next = s;
    data = 0;
  }
  
}

void LLStack::Pop()
{
  print();

  if(!next)
    return;

  data = next->data;
  next = next->next;

}

void LLStack::Append(char *s, Macro *m)
{
  
  LLInput *d = new LLInput(s,m);
  LLInput *h = (LLInput *)data;

  if(!h) {
    data = d;
    return;
  }

  /* go to the end of the list */
  while(h->next)
    h = h->next;

  /* add d to the end */

  h->next = d;

}


LLInput *LLStack::GetNext()
{
  if(!data) {
    if(!next)
      return 0;

    Pop();

    return GetNext();
  }

  LLInput *d = (LLInput *)data;

  // remove this item from the list  
  if(d) {
    data = (void *)d->next;

  }

  return d;
}

/*******************************************************
 */
void add_string_to_input_buffer(char *s, Macro *m=0)
{
  Stack.Append(s,m);
}

/*******************************************************
 */
void start_new_input_stream(void)
{
  Stack.Push();
}
/*******************************************************
 * start_parse
 * 
 * This routine will run a string through the command parser
 *this is useful if you want to execute a command but do not
 *wish to go through the readline stuff.
 */
int start_parse(void)
{
  int retval;

  init_parser();
  retval = yyparse();

  if(quit_parse)
    exit_gpsim();

  return retval;

}


int parse_string(char * str)
{
  add_string_to_input_buffer(str);
  return start_parse();
}

//========================================================================
//
void process_command_file(const char * file_name)
{

  FILE *cmd_file;
  char directory[256];
  const char *dir_path_end;

  if((verbose&4) && DEBUG_PARSER)
    cout << __FUNCTION__ <<"()\n";

  dir_path_end = get_dir_delim(file_name);
  if(dir_path_end)
  {
      strncpy(directory,file_name,dir_path_end-file_name);
      directory[dir_path_end-file_name]=0;
      printf("directory is \"%s\"\n",directory);
      chdir(directory);
      file_name=dir_path_end+1;
      printf("file_name is \"%s\"\n",file_name);
  }

  cmd_file = fopen(file_name,"r");

  if(cmd_file)
    {

      if((verbose) && DEBUG_PARSER)
	cout << "processing a command file\n";

      Stack.Push();

      char *s;
      char str[256];

      while( (s = fgets(str, 256, cmd_file)) != 0) 
       add_string_to_input_buffer(s);

      fclose(cmd_file);
    }

  Stack.print();

}

extern int load_symbol_file(Processor **, const char *);
//*********************************************

int gpsim_open(Processor *cpu, const char *file)
{
  if(!file)
    return 0;

  char *str = strrchr(file,'.');
  if(!str)
    return 0;

  str++;

  // Check for the acceptible file extensions.

  if(!strcmp(str,"hex") || !strcmp(str,"HEX")) {

    if(!cpu) {
      puts("gpsim_open::No processor has been selected!");
      return 0;
    }

    cpu->load_hex(file);
  }
  else if(!strcmp(str,"cod") || !strcmp(str,"COD"))
    load_symbol_file(&command::cpu, file);
  else if(!strcmp(str,"stc") || !strcmp(str,"STC")) {
    process_command_file(file);
    parse_string("\n");
  } else
  {
    cout << "Unknown file extension \"" << str <<"\" \n";
    return 0;
  }

  return 1;
}





//*********************************************
// gpsim_read
//
//  This function is called from the parser. It will either read
// a line of text from a command file or from stdin -- depending
// on the current source for data. Once a string is obtained, it
// is copied into a buffer that the parser has passed to us.

int
gpsim_read (char *buf, unsigned max_size)
{

  if((verbose&4) && DEBUG_PARSER)
    cout <<"gpsim_read\n";

  LLInput *d = Stack.GetNext();

  if(!d  || !d->data) {
    return 0;
  }

  gCurrentMacro = d->macro;

  char *cPstr = (char *) d->data;
  unsigned int count = strlen(cPstr);
  count = (count < max_size) ? count : max_size;

  strncpy(buf, cPstr, count);

  delete (d);

  return count;

}

void have_line(char *);

//**************************************************
void cli_main(void)
{
  do
    {
#ifdef HAVE_READLINE
      rl_callback_read_char ();
#else
      char line[256];

      fgets(line, sizeof(line),stdin);
      have_line(line);

#endif
    }
  while(!quit_parse);

}


/* **************************************************************** */
/*                                                                  */
/*                  Interface to Readline Completion                */
/*                                                                  */
/* **************************************************************** */

static char *_strndup(const char *s, int len)
{
#if defined(strndup)
  return strndup(s,len);
#else
  return strdup(s);
#endif
}

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *
command_generator (const char *text, int state)
{
  //static char *empty="";
  static int i = 0;
  const int cMaxStringLen = 64;

  /* If this is a new word to complete, initialize now.  */
  if (state == 0)
    i = 0;

  /* Return the next name which partially matches from the command list. */
  while( i<number_of_commands)
    {

      if(strstr(command_list[i]->name, text) == command_list[i]->name)
	return(_strndup(command_list[i++]->name, cMaxStringLen));

      i++;
    }

  // If no names matched, and this is the first item on a line (i.e. state==0)
  // then return a copy of the input text. (Note, it was emperically determined
  // that 'something' must be returned if there are no matches at all - 
  // otherwise readline crashes on windows.)
#ifdef _WIN32
  if(state == 0)
    return _strndup(text,cMaxStringLen);
#endif

  return 0;
}

/* Attempt to complete on the contents of TEXT.  START and END show the
   region of TEXT that contains the word to complete.  We can use the
   entire line in case we want to do some simple parsing.  Return the
   array of matches, or NULL if there aren't any. */
char **
gpsim_completion (const char *text, int start, int end)
{
  char **matches;
#ifdef _WIN32
  static char *empty="";
  matches = &empty;
#else
  matches = 0;
#endif

#ifdef HAVE_READLINE
  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
#ifdef HAVE_NSCLEAN_READLINE
    matches = rl_completion_matches (text, command_generator);
#else
    matches = completion_matches ((char *)text, (CPFunction *)command_generator);
#endif
#endif

  return (matches);
}


#ifdef HAVE_GUI

static GIOChannel *channel;

//============================================================================ 
//
// keypressed().
//
// When the user presses a key, this function will get called.
// If a simulation is running, then we'll pass the key to
// the stimulus engine.

static gboolean keypressed (GIOChannel *source, GIOCondition condition, gpointer data)
{

#ifdef HAVE_READLINE
    rl_callback_read_char ();
#endif

  return TRUE;
}

//============================================================================
//
// have_line
// When <cr> is pressed at the command line, the text string on the command
// is copied into a buffer and passed to the command parser.
//

void have_line(char *s)
{

  if(!s)
    return;

  static char last_line[256] = {0};

  if(strlen(s) == 0) {
    if(*last_line && last_command_is_repeatable)
      add_string_to_input_buffer(last_line);

  } else {
    // save a copy in the history buffer:
    strncpy(last_line,s,256);
#ifdef HAVE_READLINE
    add_history (s);
#endif
    add_string_to_input_buffer(s);
  }

  add_string_to_input_buffer("\n");

  start_parse();

  free(s);
}

#endif

/**********************************************************************
 **/
void exit_gpsim(void)
{
  if(use_icd)
    icd_disconnect();
  
#ifdef HAVE_GUI
  if(bUseGUI)
    quit_gui();
#endif

#ifdef HAVE_READLINE
  rl_callback_handler_remove();
  g_io_channel_unref(channel);
#endif

  stop_server();

  exit(0);
}

/* redisplay_prompt will redisplay the current data in the readline buffer.
   This function is used to restore a prompt that's been obliterated by diagnostic
   data.
*/

void redisplay_prompt(void)
{
#ifdef HAVE_READLINE
  rl_forced_update_display();
#endif
}

#if defined HAVE_READLINE && defined HAVE_GUI
static int gpsim_rl_getc(FILE *in)
{
  gchar buf[6];
  gsize bytes_read;

#if GLIB_MAJOR_VERSION >= 2
  g_io_channel_read_chars(channel, buf, 1, &bytes_read, NULL);
#else
  g_io_channel_read(channel, buf, 1, &bytes_read);
#endif
  /*
#if defined(_WIN32)
  if(buf[0] == 9)
    return 0x61;  // don't accept tabs in windows
#endif
  */
  return buf[0];
}
#endif

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void initialize_readline (void)
{
  const char *gpsim_prompt="gpsim> ";
  const char *gpsim_cli_prompt="**gpsim> ";
  const char *prompt = gpsim_cli_prompt;

#ifdef HAVE_READLINE

#ifdef _WIN32
  /* set console to raw mode */ 
  win32_fd_to_raw(fileno(stdin));
#endif

  rl_getc_function = gpsim_rl_getc;
  channel = g_io_channel_unix_new (fileno(stdin));

#ifdef _WIN32
  /* set console to raw mode */ 
  win32_set_is_readable(channel);
#endif

#ifdef HAVE_GUI
  if(bUseGUI)
    prompt = gpsim_prompt;
#endif

  g_io_add_watch (channel, G_IO_IN, keypressed, NULL);

  rl_callback_handler_install (prompt, have_line);

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function = gpsim_completion;

#endif //HAVE_READLINE
}

