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


//extern void simulation_cleanup(void);

#include "../src/gpsim_def.h"
#include "../src/gpsim_classes.h"
#include "../src/icd.h"
#include "../config.h"

// Defined in ../src/pic-processors.cc
extern SIMULATION_MODES simulation_mode;

#ifdef HAVE_GUI
#include <unistd.h>
#include <glib.h>
#include <gdk/gdktypes.h>
#include <gdk/gdk.h>
#endif

#include <stdio.h>

#ifdef HAVE_LIBREADLINE
#define HAVE_READLINE
#endif

extern const char *get_dir_delim(const char *path);
extern bool bUseGUI;

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

#include <sys/types.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
//#include <slang/slang.h>
#include <string>
#include <list>
#include <csignal>
#include <iostream>

#include "command.h"
#include "input.h"
#include "../src/pic-processor.h"
#include "../src/breakpoints.h"

#define MAX_LINE_LENGTH 256  

int yyparse(void);
void initialize_readline (void);
//extern "C" {
void exit_gpsim(void);
#ifdef HAVE_GUI
  void quit_gui(void);

#endif

void redisplay_prompt(void);
//}

char *gnu_readline (char *s, unsigned int force_readline);
bool using_readline=1;
int input_mode = 0;
int allocs = 0;
int last_command_is_repeatable=0;
/* When non-zero, this global means the user is done using this program. */
int done = 0;

//const char *gpsim = "gpsim> ";  // Normal prompt
//const char *gpsim_cont = "> ";  // command continuation prompt

extern int quit_parse;

/* Command file reference counter. This global variable keeps track
 * of the nesting level of loaded command files */

int Gcmd_file_ref_count=0;



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

}


char *cmd_string_buf = 0;
FILE *cmd_file = 0;


/*******************************************************
 */
char * strip_cmd(char *buff)
{
  char *comment;

  if(!buff)
    return buff;

  // Strip leading spaces
  while(*buff == ' ') 
    *buff++;

  comment = strchr(buff, '#');

  // If there's a comment, remove it.
  if(comment) {
    *comment = '\0';
  }

  return buff;
}

/*******************************************************
 * parse_string
 * 
 * This routine will run a string through the command parser
 *this is useful if you want to execute a command but do not
 *wish to go through the readline stuff.
 */
int parse_string(char *cmd_string)
{
  static int save_readline_state = 0;
  static char last_line[256] = {0};

  int retval;

  save_readline_state = using_readline;
  using_readline = 0;

  if(!cmd_string)
    return 0;

  if(verbose & 2)
    printf("   %s: %s\n",__FUNCTION__,cmd_string);

  cmd_string = strip_cmd(cmd_string);

  cmd_string_buf = strdup(cmd_string); // free'd by gpsim_read


  if( strlen (cmd_string) == 0)
    {
      if(*last_line && last_command_is_repeatable)
	cmd_string_buf = strdup(last_line);

    }
  else
    {

#ifdef HAVE_READLINE
      if(strlen(cmd_string)) {
	add_history (cmd_string);

	strncpy(last_line,cmd_string,256);
      }
#endif
    }


  init_parser();
  retval = yyparse();

  using_readline = (0 != save_readline_state);

  if(quit_parse) {
    free(cmd_string_buf);
    exit_gpsim();
  }

  return retval;

}

char * gets_from_cmd_file(char **ss)
{
  char str[MAX_LINE_LENGTH];

  str[0] = 0;

  if((verbose&4) && DEBUG_PARSER)
    cout << __FUNCTION__ <<"()\n";

  if(cmd_file) {
    fgets(str, MAX_LINE_LENGTH, cmd_file);
    *ss= strdup(str); // free'd by gpsim_read
    allocs++;
    //cout << "allocs++" << allocs << '\n';
  }

  return *ss;

}

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

      quit_parse = 0;
      Gcmd_file_ref_count++;
      while( !quit_parse )
	{
	  //cout << "about to re-init the parser\n";
	  init_parser();
	  yyparse();
	}

      // decrement the reference counter if it's greater than 0.
      if(Gcmd_file_ref_count>0)
	Gcmd_file_ref_count--;

      fclose(cmd_file);
      cmd_file = save_cmd_file;
    }


  using_readline = (0 != save_readline_state);

}

static char *
get_user_input (void)
{
  char *retval = 0;
  static char buf[256];

  if((verbose&4) && DEBUG_PARSER)
    cout << __FUNCTION__ <<"() --- \n";

  if( !bUseGUI && using_readline) {
    //cout << "  1";
#ifdef HAVE_READLINE
    // If we're in cli-only mode and we're not processing a command file
    // then we use readline to get the commands
    retval = gnu_readline ( "**gpsim> ",1);

#else
    cout << "__gpsim> ";
    cout.flush();
    cin.getline(buf, sizeof(buf));
    if (cin.eof()) {
      cout << buf << endl;
      return 0;
    }
    return buf;
#endif

  } else {

    //cout << "  2";
    // We're either using the gui or we're parsing a command file.

    gets_from_cmd_file(&cmd_string_buf);
    retval = cmd_string_buf;

  }
  //cout << "  3 returning " << retval << endl;

  return retval;
}

extern int open_cod_file(pic_processor **, char *);

//*********************************************

int gpsim_open(Processor *cpu, const char *file)
{
  char *str;

  str = strrchr(file,'.');
  if(str==0)
  {
    //	puts("found no dot in file!");
    return 0;
  }
  str++;

  if(!cpu)
  {
    puts("gpsim_open::No processor has been selected!");
    return 0;
  }

  if(!strcmp(str,"hex"))
  {

    cpu->load_hex(file);
    
  }
  else if(!strcmp(str,"cod"))
  {

    int i;
    i=load_symbol_file(&cpu, file);

    if(i)
    {
      cout << "found a fatal error in the symbol file " << file <<'\n';
      return 0;
    }

    // FIXME: questionable
    command_list[0]->cpu=cpu;
    get_trace().switch_cpus(cpu);
      
  }
  else if(!strcmp(str,"stc"))
    process_command_file(file);
  else
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
  char *input_buf;
  static unsigned chars_left = 0;
  int status = 0;

//cout << __FUNCTION__ << endl;

  if((verbose&4) && DEBUG_PARSER)
    cout <<"gpsim_read\n";

  input_buf = get_user_input ();
  chars_left = input_buf ? (unsigned)strlen (input_buf) : 0;

  if (chars_left > 0)
    {
      buf[0] = '\0';

      chars_left = ( (chars_left < max_size) ? chars_left : max_size);

      strncpy (buf, input_buf, chars_left);

      // If the input string does not have a carriage return, then add one.
      if (buf[chars_left-1] != '\n')
	buf[chars_left++] = '\n';
      buf[chars_left] = 0;

      //cout << "chars_left > 0, copied cur_pos into buff\n";
      //cout << "buf[]  " << buf << '\n';
    }
    // If we are reading from a command file (and not stdin), then
    // the string that was read copied into a dynamically allocated
    // buffer that we need to delete.

  if(allocs) {
    allocs--;

    //cout << "freeing input_buf " <<input_buf <<'\n';
    //cout << "allocs--" << allocs << '\n';

    free (input_buf);
  }
  //status = len;
  //chars_left = 0;

  if((verbose&4) && DEBUG_PARSER)
    cout << "leaving gpsim_read\n";

  return chars_left;
}

#if 0
___main_input (void)
{

  COMMAND_MODES command_mode=NO_COMMAND;


  // if there is a startup file then read it and do what it says
  //  if(startup)
  //  process_command_file(startup);

  initialize_readline ();	/* Bind our completer. */

  //  initialize_signals();

  char last_line[256];
  last_line[0] = 0;

  int flag = 0;

  /* Loop reading and executing lines until the user quits. */
  while (!quit_gpsim)
    {
      char *line;
      
      if(command_mode == NO_COMMAND)
	{
	  //line = ::readline (gpsim);
	  line = ::readline ("gpsim> ");
	}
      else
	{
	  //line = ::readline (gpsim_cont);
	  line = ::readline (">");
	}

      if (!line)
	{
	  done = 1;		/* Encountered EOF at top level. */
	}
      else
	{
	  /* Remove leading and trailing whitespace from the line.
	     Then, if there is anything left, add it to the history list
	     and execute it. */
	  //stripwhite (line);

	  if (*line)
	    {
	      add_history (line);
	      //command_mode = execute_line (line);
	      strncpy(last_line,line,256);
	    }
	  else
	    {
	      //cout << "empty line\n";
	      //if(*last_line)
	      // command_mode = execute_line (last_line);
	    }
	}

      if (line)
	free (line);
      line = 0;

    }
  exit (0);
}

#endif

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
// When <cr> is press at the command line, the text string on the command
// is copied into a buffer and passed to the command parser.
//

void have_line(char *s)
{
#ifdef HAVE_READLINE
  if(simulation_mode != STOPPED)
    return;

  parse_string(s);
  free(s);
#endif
}

#endif


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

  //  simulation_cleanup();

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
  //cout << __FUNCTION__ << endl;
#ifdef HAVE_READLINE
#ifdef HAVE_GUI
  rl_getc_function = gpsim_rl_getc;
  channel = g_io_channel_unix_new (fileno(stdin));

  g_io_add_watch (channel, G_IO_IN, keypressed, NULL);

  rl_callback_handler_install ("gpsim> ", have_line);
#endif

  /* Allow conditional parsing of the ~/.inputrc file. */
  //  rl_readline_name = "gpsim";

  /* Tell the completer that we want a crack first. */
    rl_attempted_completion_function = gpsim_completion;
#else
  //char buf [256];

  //while(1) {
    //cout << "gpsim> ";
    //cout.flush();

    //cin.getline(buf, sizeof(buf));
    //if (cin.eof()) {
    //cout << buf << endl;
    //}

    //parse_string(buf);
    //}

#endif //HAVE_READLINE
}

#ifdef HAVE_READLINE
char *gnu_readline (char *s, unsigned int force_readline)
{
  static char last_line[256]={0};
  char *retval = 0;

  //cout << __FUNCTION__ << endl;

  if(using_readline || force_readline)
    {
      //cout << "about to do readline\n";

      char *tmp;

      if(input_mode == CONTINUING_LINE)
        //retval = ::readline (const_cast<char*>(gpsim_cont));
	retval = ::readline (">");
      else
	retval = ::readline (s);
      tmp = retval;

      /*
      if(tmp)
	{
	  cout << "tmp is not null and is " << strlen(tmp) << " chars long\n";
	}
      else
	{
	  cout << "tmp is null";
	}
      */

      if (!tmp)
	{
	  retval = (char *) malloc (2);
	  retval[0] = '\n';
	  retval[1] = '\0';
	}
      else
	{
	  if( strlen (tmp) == 0)
	    {
	      if(*last_line)
		{
		  retval = strdup(last_line);
		}
	      else
		{
		  retval = (char *) malloc (2);
		  retval[0] = '\n';
		  retval[1] = '\0';
		}
	    }
	  else
	    {
	      add_history (tmp);

	      strncpy(last_line,tmp,256);
	    }
	}
    }
  else
    {
      if(cmd_string_buf)
	{
	  //cout << "processing a command string\n";
	  retval = cmd_string_buf;
	}
      else
	{
	  cout << "read line error\n";
	  exit(1);
	}
    }

  return retval;
}
#endif  // HAVE_READLINE
