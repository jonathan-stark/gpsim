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
#include <iostream.h>
#include <iomanip.h>
#include <string>
#include <list>

#include "../cli/command.h"
#include "../cli/input.h"
#include "../src/gpsim_def.h"
#include "../src/interface.h"

extern "C" {
int gui_init (int argc, char **argv);
void gui_main(void);
void gpsim_interface_init(void);

}

extern int server_main (int argc, char *argv[]);

void initialize_gpsim(void);

unsigned int config_word;   //%%%FIX_ME%%%

int yyparse(void);
int parse_string(char *cmd_string);

extern int yydebug;
extern int quit_parse;

void gpsim_version(void)
{
  cout << GPSIM_VERSION << '\n';
}

void 
helpme (char *iam)
{
  printf ("\n\nuseage:\n%s [-h] [-p <device> [<hex_file>]] [-c <stc_file>]\n", iam);
  printf ("\t-h             : this help list\n");
  printf ("\t-p <device>    : processor (e.g. -pp16c84 for the 'c84)\n");
  printf ("\t<hex_file>     : input file in \"intelhex16\" format\n");
  printf ("\t-c <stc_file>  : startup command file\n");
  printf ("\t-s <cod_file>  : .cod symbol file\n\n");
  printf ("\t-v             : gpsim version\n");
}


void welcome(void)
{

  cout << "\ngpsim - the GNUPIC simulator\
\nversion: " << GPSIM_VERSION << "\n\ntype help for help\n";

}



void 
main (int argc, char *argv[])
{

  FILE *inputfile = stdin, *startup=NULL;
#define FILE_STRING_LENGTH 50
  char 
    startup_name[FILE_STRING_LENGTH] = "",
    pic_name[FILE_STRING_LENGTH]   = "",
    cod_name[FILE_STRING_LENGTH]   = "",
    hex_name[FILE_STRING_LENGTH]   = "";

  int i;
  int j;
  int c,usage=0;
  char command_str[256];
  char *b;
  int architecture;

  welcome();

  cout << hex ;
  cin >> hex;

#if 0
  i = 1;

  while(i < argc)
    {
      if (argv[i][0] == '-')
	switch (argv[i][1])
	  {
	  case 'P':
	  case 'p':
	    strncpy(pic_name, &argv[i][2],FILE_STRING_LENGTH);
	    break;

          case 'C':
          case 'c':
	    printf("%s  %s\n", &argv[i][0], &argv[i][2]);
	    strncpy (startup_name, &argv[i][2],FILE_STRING_LENGTH);
	    break;

          case 'S':
          case 's':
	    printf("%s  %s\n", &argv[i][0], &argv[i][2]);
	    strncpy (cod_name, &argv[i][2],FILE_STRING_LENGTH);
	    break;

	  case 'V':
	  case 'v':
	    printf("%s\n",GPSIM_VERSION);
	    break;

	  case 'H':
          case 'h':
	  default:
	    helpme (argv[0]);
	    exit (1);

	  }
      else
	{
	  strncpy(hex_name, argv[i],FILE_STRING_LENGTH);
	}

      i++;
    }
#endif

  while ((c = getopt(argc, argv, "h?p:c:s:v")) != EOF) {
    switch (c) {
    case '?':
    case 'h':
      usage = 1;
      break;
    case 'p':
      strncpy(pic_name, optarg,FILE_STRING_LENGTH);
      break;

    case 'c':
      strncpy(startup_name, optarg,FILE_STRING_LENGTH);
      break;

    case 's':
      strncpy(cod_name, optarg,FILE_STRING_LENGTH);
      break;

    case 'v':
      printf("%s\n",GPSIM_VERSION);
      break;

    }
    if (usage)
      break;
  }
  
  if (optind < argc)
    strncpy(hex_name, argv[optind],FILE_STRING_LENGTH);
  else
    usage = 1;


  if (usage) 
    helpme(argv[0]);

  initialize_gpsim();
  init_parser();
  initialize_readline();
  gpsim_interface_init();

  // initialize the gui
#ifdef HAVE_GUI
  i = gui_init (argc,argv);
#endif

  initialization_is_complete();


  yydebug = 0;

  quit_parse = 0;


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


  // Now enter the event loop and start processing user
  // commands.

#ifdef HAVE_GUI
  gui_main();
#else
  do {
    init_parser();
    i = yyparse();

  } while(!quit_parse);

#endif

}
