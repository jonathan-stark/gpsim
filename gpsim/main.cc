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



// Portions of this files are (C) by Ian King:

/* picdis.c  - pic disassembler         */
/* version 0.1                          */
/* (c) I.King 1994                      */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <iostream.h>
#include "../config.h"
#include "../cli/input.h"
#include "../src/gpsim_def.h"
#include "../src/interface.h"
#include "../src/fopen-path.h"

#ifdef HAVE_GUI
int use_gui=1;
#else
int use_gui=0;
#endif 
int quit_state;

extern "C" {
<<<<<<< main.cc
int gui_init (int argc, const char **argv);
=======
#include <popt.h>
int gui_init (int argc, char **argv);
>>>>>>> 1.7
void gui_main(void);
extern void exit_gpsim(void);

}


void initialize_gpsim(void);


int yyparse(void);
int parse_string(char *cmd_string);
extern void init_parser(void);
//extern void parser_cleanup(void);

extern int yydebug;
extern int quit_parse;
extern int abort_gpsim;

void gpsim_version(void)
{
  printf("%s\n", VERSION);
}

#define FILE_STRING_LENGTH 50

static char *startup_name = "";
static char *pic_name     = "";
static char *cod_name     = "";
static char *hex_name     = "";
static char *search_path  = "";

struct poptOption optionsTable[] = {
  //  { "help", 'h', 0, 0, 'h',
  //    "this help list" },
  { "processor", 'p', POPT_ARG_STRING, &pic_name, 0,
    "processor (e.g. -pp16c84 for the 'c84)","<processor name>" },
  { "command",   'c', POPT_ARG_STRING, &startup_name, 0,
    "startup command file",0 },
  { "symbol",    's', POPT_ARG_STRING, &cod_name, 0,
    ".cod symbol file",0 } ,
  { "", 'L',0,0,'L',
    "colon separated list of directories to search.", 0},
  { "version",'v',0,0,'v',
    "gpsim version",0},
  { "cli",'i',0,0,'i',
    "command line mode only",0},
  POPT_AUTOHELP
  { NULL, 0, 0, NULL, 0, 0 }
};

void 
helpme (const char *iam)
{
  printf ("\n\nuseage:\n%s [-h] [-p <device> [<hex_file>]] [-c <stc_file>]\n", iam);
  printf ("\t-h             : this help list\n");
  printf ("\t-p <device>    : processor (e.g. -pp16c84 for the 'c84)\n");
  printf ("\t<hex_file>     : input file in \"intelhex16\" format\n");
  printf ("\t-c <stc_file>  : startup command file\n");
  printf ("\t-s <cod_file>  : .cod symbol file\n");
  printf ("\t-L <path list> : colon separated list of directories to search.\n");
  printf ("\n\t-v             : gpsim version\n");
  printf ("\n Long options:\n\n");
  printf ("\t--cli          : command line mode only\n");
}
void usage(poptContext optCon, int exitcode, char *error, char *addl) 
{
  poptPrintUsage(optCon, stderr, 0);
  if (error) 
    fprintf(stderr, "%s: %s", error, addl);
  exit(exitcode);
}



void welcome(void)
{

  printf("\ngpsim - the GNUPIC simulator\nversion: %s\n", VERSION);
  printf("\n\ntype help for help\n");

}


void 
main (int argc, const char *argv[])
{

  FILE *inputfile = stdin, *startup=NULL;

  int i;
  int j;
  int c,usage=0;
  char command_str[256];
  char *b;
  int architecture;
  int option_index=0;
  poptContext optCon;   /* context for parsing command-line options */


  optCon = poptGetContext(NULL, argc, (const char **)argv, optionsTable, 0);
  poptSetOtherOptionHelp(optCon, "[-h] [-p <device> [<hex_file>]] [-c <stc_file>]");


  if (argc < 2) {
    poptPrintUsage(optCon, stderr, 0);
    exit(1);
  }

  welcome();

  //while ((c = getopt(argc, argv, "h?p:c:s:L:v")) != EOF) {
  while ((c = poptGetNextOpt(optCon)) >= 0) {
    switch (c) {

    default:
      printf("'%c' is an unrecognized option\n",c);
    case '?':
    case 'h':
      usage = 1;
      break;

      //case 'p':
      //strncpy(pic_name, optarg,FILE_STRING_LENGTH);
      //break;

      //case 'c':
      //strncpy(startup_name, optarg,FILE_STRING_LENGTH);
      //break;

      //case 's':
      //strncpy(cod_name, optarg,FILE_STRING_LENGTH);
      //break;

    case 'L':
	set_search_path (search_path);
      break;

    case 'v':
      printf("%s\n",VERSION);
      break;

    case 'i':
      use_gui = 0;
      printf("not using gui");
    }
    if (usage)
      break;
  }

  
  //if (optind < argc)
  //  strncpy(hex_name, argv[optind],FILE_STRING_LENGTH);
/*
  hex_name = poptGetArg(optCon);

  if((hex_name == NULL) || !(poptPeekArg(optCon) == NULL))
    usage(optCon, 1, "Specify a hex file", ".e.g., /dev/cua0");
*/
  if (usage) {
    helpme(argv[0]);
    exit (1);
  }


  initialize_gpsim();
  init_parser();
  initialize_readline();

  // initialize the gui
#ifdef HAVE_GUI
  if(use_gui)
    i = gui_init (argc,argv);
#endif


  initialization_is_complete();

  yydebug = 0;

  quit_parse = 0;
  abort_gpsim = 0;

  if(*pic_name)
    {
      strcpy(command_str, "processor ");
      strcat(command_str, pic_name);
      parse_string(command_str);
    }

  if(*hex_name)
    {
      strcpy(command_str, "load h ");
      strcat(command_str, hex_name);
      parse_string(command_str);
    }

  if(*cod_name)
    {
      strcpy(command_str, "load s ");
      strcat(command_str, cod_name);
      parse_string(command_str);
    }

  if(*startup_name)
    {
      strcpy(command_str, "load c ");
      strcat(command_str, startup_name);
      parse_string(command_str);
    }

  if(abort_gpsim)
    exit_gpsim();

  //    parser_cleanup();

  // Now enter the event loop and start processing user
  // commands.

#ifdef HAVE_GUI
  if(use_gui)
    gui_main();
  else
#endif
    do {
      init_parser();
      i = yyparse();

    } while(!quit_parse);



}
