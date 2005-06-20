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
#define GETCWD _getcwd

#else
#include <sys/file.h>
#include <unistd.h>

#if !defined(_MAX_PATH)
#define _MAX_PATH 1024
#endif

#define GETCWD getcwd
// temp fix to over come 23jan05 changes to configure script that prevent
// readline from being found on older systems.
#define HAVE_READLINE
#define HAVE_NSCLEAN_READLINE

#endif

#ifdef HAVE_GUI
#include <glib.h>
#include <gdk/gdktypes.h>
#include <gdk/gdk.h>
#endif

#include "../src/exports.h"
#include "../src/sim_context.h"
#include "../src/gpsim_def.h"
#include "../src/gpsim_classes.h"
#include "../src/icd.h"
#include "../src/pic-processor.h"
#include "../src/breakpoints.h"
#include "../src/cod.h"
#include "command.h"
#include "input.h"
#include "cmd_macro.h"
#include "cmd_run.h"

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
int parse_string(char * str);

//------------------------------------------------------------------------
// 
extern bool gUsingThreads(); // in ../src/interface.cc

void initialize_readline (void);

void exit_gpsim(void);
#ifdef HAVE_GUI
void quit_gui(void);
#endif

void redisplay_prompt(void);

char *gnu_readline (char *s, unsigned int force_readline);

int last_command_is_repeatable=0;
//extern Macro *gCurrentMacro;
extern void  scanPushMacroState(Macro *);

extern int quit_parse;

extern void start_server(void);
extern void stop_server(void);

//------------------------------------------------------------------------
// Command Handler - create an interface to the CLI
//------------------------------------------------------------------------
#include "../src/cmd_manager.h"
//class ISimConsole;
class CCliCommandHandler : public ICommandHandler
{
public:
  virtual char *GetName();
  virtual int Execute(const char * commandline, ISimConsole *out);
};

char *CCliCommandHandler::GetName()
{
  return "gpsimCLI";
}

int CCliCommandHandler::Execute(const char * commandline, ISimConsole *out)
{
  cout << "GCLICommandHandler::Execute:" << commandline << endl;
  
  return parse_string((char*)commandline);

}
static CCliCommandHandler sCliCommandHandler;

//------------------------------------------------------------------------
//------------------------------------------------------------------------


/*
  temporary --- linked list input buffer
*/

class LLInput {
public:

  LLInput();
  LLInput(char *,Macro *);
  ~LLInput();

  Macro *macro;  // macro generating this text
  char *data;
  LLInput *next_input;
};

LLInput::LLInput()
  : macro(0),data(0), next_input(0)
{
}

LLInput::LLInput(char *s,Macro *m)
  : macro(m), next_input(0)
{
  data = strdup(s);
}

LLInput::~LLInput()
{
  if(data)
    free(data);
}

//************************************************************************
class LLStack
{
public:
  LLStack();

  void Push();
  void Pop();
  void Append(char *, Macro *);
  LLInput *GetNext();

  void print();

  LLInput *LLdata;
  LLStack *next_stack;
};

LLStack::LLStack()
  : LLdata(0), next_stack(0)
  
{

}

static LLStack *Stack=0;

//========================================================================
//
// catch_control_c
//
//  
void catch_control_c(int sig)
{
#ifdef _WIN32
  if(CSimulationContext::GetContext()->GetActiveCPU()->simulation_mode
    == eSM_RUNNING) {
    CSimulationContext::GetContext()->GetBreakpoints().halt();
  }
  ::signal(SIGINT, catch_control_c);
#else
  // JRH - I'll let someone else try the above code under Linux.
  // The readline library appears to call ::signal() itself so
  // the call will not be needed here. I'm guessing that the CTRL->C
  // signal handling in readline did not work under Windows so
  // that is why it was originally ifdef'd out.
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
#endif
}

void initialize_threads(void)
{
#if GLIB_MAJOR_VERSION >= 2
  if( !g_thread_supported() )
  {
    g_thread_init(NULL);
    gdk_threads_init();
  }

#endif
}

void initialize_signals(void)
{
#ifdef _WIN32
  ::signal(SIGINT, catch_control_c);
#else
  static struct sigaction action;

  action.sa_handler = catch_control_c;
  sigemptyset(&action.sa_mask);
  action.sa_flags=0;

  sigaction(SIGINT, &action, 0);
#endif

}
void initialize_CLI()
{
  CCommandManager::GetManager().Register(&sCliCommandHandler);  
}
//==============================================================
// initialize_gpsim 
//
// Not much initialization is needed now. 
//

void initialize_gpsim(void)
{
  initialize_CLI();
  if(gUsingThreads()) 
    initialize_threads();
  initialize_signals();
  start_server();
}


void LLStack::print(void)
{
  if(verbose & 4) {
    LLStack *s = Stack;

    cout << "Current state of input buffer:\n";
    int stack_number=0;
    while (s) {
      LLInput *h = s->LLdata;
      int depth =0;
      while(h) {
      
	cout << "   " <<stack_number <<':'<<depth << "  "<<  h->data;
	depth++;
	h = h->next_input;
      }
      stack_number++;
      s = s->next_stack;
    }
    cout << "\n ---Leaving dump \n";
  }
}

void LLStack::Push(void)
{
  LLStack *s = new LLStack();

  s->next_stack = Stack;
  Stack = s;
    
  print();
}

void LLStack::Pop()
{
  if (Stack)
    Stack = Stack->next_stack;
  print();
}

void LLStack::Append(char *s, Macro *m)
{
  
  LLInput *d = new LLInput(s,m);

  LLInput *h = LLdata;

  if (h) {

    /* go to the end of the list */
    while(h->next_input)
      h = h->next_input;

    /* add d to the end */

    h->next_input = d;
    
  } else
    LLdata = d;


}


LLInput *LLStack::GetNext()
{

  if (Stack) {
    if (Stack->LLdata) {
      LLInput *d = Stack->LLdata;

      // remove this item from the list  
      if(d)
        Stack->LLdata = d->next_input;

      return d;
    }
    Pop();

    return GetNext();
  }

  return 0;
}

/*******************************************************
 */
void add_string_to_input_buffer(char *s, Macro *m=0)
{
  if(!Stack)
    Stack = new LLStack();
  Stack->Append(s,m);
}

/*******************************************************
 */
void start_new_input_stream(void)
{
  if(!Stack)
    Stack = new LLStack();
  else 
    Stack->Push();
}

/*******************************************************
 */
void clear_input_buffer(void)
{
  LLInput * pLine;
  if (Stack)
    while((pLine = Stack->GetNext()) != NULL)
      delete pLine;
}

/*******************************************************
 * start_parse
 * 
 * This routine will run a string through the command parser
 *this is useful if you want to execute a command but do not
 *wish to go through the readline stuff.
 */
extern int init_parser();

int start_parse(void)
{

  int retval = init_parser();

  if(quit_parse)
    exit_gpsim();

  return retval;
}


int parse_string(char * str)
{
  add_string_to_input_buffer(str);
  int iReturn = start_parse();
  if(iReturn == 1) {
    // If the str was a 'load c x.stc' file and the parsing
    // was aborted we need to remove the remaining
    // strings.
    clear_input_buffer();
  }
  return iReturn;
}

//========================================================================
//

static int check_old_command(char *s)
{
    char new_command[256];
    { // "module position <modulename> <xpos> <ypos>
    	char module_name[256];
      int xpos, ypos;
    	if(sscanf(s,"module position %s %d %d\n",module_name, &xpos, &ypos)==3)
      {
        cout<<"Found old style \"module position\" command"<<endl;
        sprintf(new_command,"%s.xpos=%d.0\n",module_name,xpos);
        add_string_to_input_buffer(new_command);
        cout<<"Translation: "<<new_command<<endl;
        sprintf(new_command,"%s.ypos=%d.0\n",module_name,ypos);
        add_string_to_input_buffer(new_command);
        cout<<"Translation: "<<new_command<<endl;
        return 1;
      }
    }
    return 0;
}

void process_command_file(const char * file_name)
{

  FILE *cmd_file;
  char directory[256];
  const char *dir_path_end;

  if(verbose&4)
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

      if(verbose)
          cout << "processing a command file\n";

      start_new_input_stream();
      //Stack.Push();

      char *s;
      char str[256];

      while( (s = fgets(str, 256, cmd_file)) != 0) 
      {
          if(!check_old_command(s))
              add_string_to_input_buffer(s);
      }

      fclose(cmd_file);
    }
  else
    {
      cout << "failed to open command file ";
      cout << file_name;
      cout << endl;
      char cw[_MAX_PATH];
      GETCWD(cw, _MAX_PATH);
      cout << "current working directory is ";
      cout << cw;
      cout << endl;

    }

  if(Stack)
    Stack->print();

}

/*********************************************
  Function: gpsim_open()

  JRH - gpsim_open() was returning different values
  to indicate a success. I have  made this function return
  1 for success and 0 for failure.

  Returns:  1   - success
            0   - failure
*/

int gpsim_open(Processor *cpu, const char *file, const char * pProcessorType)
{
  if(!file)
    return 0;

  // Check for the command file, file extension.
  if(IsFileExtension(file,"stc") || IsFileExtension(file,"STC")) {
    process_command_file(file);
    // A stc file could have any sequence of commands.
    // Just ignore the return value of parse_string().
    parse_string("\n");
    return 1;
  } else {
    // Assume a Program file
    return CSimulationContext::GetContext()->LoadProgram(
      file, pProcessorType);
  }
  return 0;
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


  LLInput *d = Stack ? Stack->GetNext() : 0;

  if (!d || !d->data) {
  if(verbose&4)
    cout <<"gpsim_read -- no more data\n";
    return 0;
  }

  scanPushMacroState(d->macro);
  //gCurrentMacro = d->macro;

  char *cPstr = d->data;
  unsigned int count = strlen(cPstr);
  count = (count < max_size) ? count : max_size;

  strncpy(buf, cPstr, count);

  if(verbose&4) {
    cout <<"gpsim_read returning " << count << ":" << cPstr << endl;
    if (d->macro) {
      cout << "   and it's a macro named:" << d->macro->name() << endl;
    }
  }
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

#if defined(_WIN32) || defined(WIN32)
	if (start) {
		char *empty = strdup("");
		matches = (char **) malloc(2 * (sizeof(&empty)));
		matches[0] = empty;
		matches[1] = 0;
	}
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
#endif

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

  CSimulationContext::GetContext()->GetContext()->Clear();
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
