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

// Defined in ../src/pic-processors.cc
extern SIMULATION_MODES simulation_mode;

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
static bool using_readline=1;
int input_mode = 0;
int last_command_is_repeatable=0;

extern int quit_parse;

#ifndef _WIN32
extern void start_server(void);
#endif

/*
  temporary --- linked list input buffer
*/
class LLInput {
public:
  LLInput *next;
  void *data;
};

static LLInput Stack={0,0};

//====================================================================================
//
// catch_control_c
//
//  
void catch_control_c(int sig)
{

  if(simulation_mode != STOPPED)
    {
      cout << "<CTRL C> break\n";
      bp.halt();
    }
  else {
    cout << "caught control c, but it doesn't seem gpsim was simulating\n";
    last_command_is_repeatable=0;
    redisplay_prompt();

  }

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
// Not much initialization is needed now. However, the CORBA 
// calls needed later will change this...
//

void initialize_gpsim(void)
{

  initialize_signals();
#ifndef _WIN32
  start_server();
#endif
}


char *cmd_string_buf = 0;
FILE *cmd_file = 0;

static void LLDumpInputBuffer(void)
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

LLInput *newLLInput(void)
{
  LLInput *d = (LLInput *)(malloc(sizeof(LLInput)));
  if(d) {
    d->next = 0;
    d->data = 0;
  }
  return d;
}

static void LLPush(void)
{
  LLInput *s = newLLInput();

  LLDumpInputBuffer();
  if(s) {
    s->next = Stack.next;
    s->data = Stack.data;
    Stack.next = s;
    Stack.data = 0;
  }
  
}

static void LLPop(void)
{
  LLDumpInputBuffer();

  if(!Stack.next)
    return;

  Stack.data = Stack.next->data;
  Stack.next = Stack.next->next;

}

static void LLAppend(LLInput *h, LLInput *d)
{
  if(!h)
    return;

  /* go to the end of the list */
  while(h->next)
    h = h->next;

  /* add d to the end */

  h->next = d;

}


static LLInput *LLGetNext(void)
{
  if(!Stack.data) {
    if(!Stack.next)
      return 0;

    LLPop();

    return LLGetNext();
  }

  LLInput *d = (LLInput *)Stack.data;

  // remove this item from the list  
  if(d) {
    Stack.data = (void *)d->next;

  }

  return d;
}

/*******************************************************
 */
static void add_string_to_input_buffer(char *s)
{
  if(!s)
    return;

  LLInput *d = newLLInput();

  if(d) {

    d->data = (void *)strdup(s);       // free'd by gpsim_read.
    d->next = 0;

    if(!Stack.data) 
      Stack.data = d;
    else
      LLAppend((LLInput*)Stack.data, d);


  }
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
  static int save_readline_state = 0;
  int retval;

  //gdk_threads_enter();
  init_parser();
  retval = yyparse();

  //gdk_threads_leave();

  using_readline = (0 != save_readline_state);

  if(quit_parse) {
    free(cmd_string_buf);
    exit_gpsim();
  }

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

  int save_readline_state;
  FILE *save_cmd_file;
  char directory[256];
  const char *dir_path_end;

  if((verbose&4) && DEBUG_PARSER)
    cout << __FUNCTION__ <<"()\n";

  save_cmd_file = cmd_file;

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

  save_readline_state = using_readline;
  using_readline = 0;
  cmd_string_buf = 0;

  if(cmd_file)
    {

      if((verbose) && DEBUG_PARSER)
	cout << "processing a command file\n";

      LLPush();

      char *s;
      char str[256];

      while( (s = fgets(str, 256, cmd_file)) != 0) 
       add_string_to_input_buffer(s);

      //while(fread(str,1,256,cmd_file))
      //  add_string_to_input_buffer(str);

      fclose(cmd_file);
      cmd_file = save_cmd_file;
    }

  LLDumpInputBuffer();

  using_readline = (0 != save_readline_state);

}


extern int open_cod_file(pic_processor **, char *);

//*********************************************

int gpsim_open(Processor *cpu, const char *file)
{
  char *str;
  char command_str[256];
  char type =0;

  str = strrchr(file,'.');
  if(str==0)
  {
    //	puts("found no dot in file!");
    return 0;
  }
  str++;


  if(!strcmp(str,"hex"))
  {

    if(!cpu)
    {
      puts("gpsim_open::No processor has been selected!");
      return 0;
    }

    type = 'h';


  }
  else if(!strcmp(str,"cod"))
    type = 's';
  else if(!strcmp(str,"stc"))
    type = 'c';
  else
  {
    cout << "Unknown file extension \"" << str <<"\" \n";
    return 0;
  }

  snprintf(command_str, sizeof(command_str),
	   "load %c %s\n",type,file);
  parse_string(command_str);

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

  LLInput *d = LLGetNext();

  if(!d  || !d->data) {
    return 0;
  }
  char *cPstr = (char *) d->data;
  unsigned int count = strlen(cPstr);
  count = (count < max_size) ? count : max_size;

  strncpy(buf, cPstr, count);

  free (cPstr);
  free (d);

  return count;

}

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


/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *
command_generator (const char *text, int state)
{
  char  *n;
  static int i = 0;

  /* If this is a new word to complete, initialize now.  */

  if (state == 0)
    i = 0;

  /* Return the next name which partially matches from the command list. */
  while( i<number_of_commands)
    {

      if(strstr(command_list[i]->name, text) == command_list[i]->name)
	{
	  n = (char *) malloc(strlen(command_list[i]->name));
	  if(n == 0)
	    {
	      //fprintf (stderr, "malloc: Out of virtual memory!\n");
	      abort ();
	    }
	  strcpy(n,command_list[i]->name);
	  i++;
	  return(n);
	}
      i++;
    }

  /* If no names matched, then return NULL. */
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
  //char *command_generator (char *, int);

  matches = 0;

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
  if(simulation_mode == STOPPED) {
#ifdef HAVE_READLINE
    rl_callback_read_char ();
#endif
  } else { 

    // We're either running, sleeping, single stepping...
  }

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

  if(simulation_mode != STOPPED)
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

