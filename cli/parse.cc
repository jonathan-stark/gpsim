
/*  A Bison parser, made from parse.yy
 by  GNU Bison version 1.27
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	ATTACH	257
#define	BREAK	258
#define	BUS	259
#define	CLEAR	260
#define	DISASSEMBLE	261
#define	DUMP	262
#define	HELP	263
#define	LOAD	264
#define	LIST	265
#define	NODE	266
#define	MODULE	267
#define	PROCESSOR	268
#define	QUIT	269
#define	RESET	270
#define	RUN	271
#define	SET	272
#define	STEP	273
#define	STIMULUS	274
#define	SYMBOL	275
#define	TRACE	276
#define	gpsim_VERSION	277
#define	X	278
#define	END_OF_COMMAND	279
#define	IGNORED	280
#define	SPANNING_LINES	281
#define	STRING	282
#define	INDIRECT	283
#define	END_OF_INPUT	284
#define	BIT_FLAG	285
#define	NUMERIC_OPTION	286
#define	STRING_OPTION	287
#define	NUMBER	288
#define	FLOAT_NUMBER	289

#line 2 "parse.yy"

/* Parser for gpsim
   Copyright (C) 1999 Scott Dattalo

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
#include <iostream.h>
#include <iomanip.h>
#include <string>
#include <vector>
#include <glib.h>

#include "misc.h"
#include "command.h"

#include "cmd_attach.h"
#include "cmd_break.h"
#include "cmd_bus.h"
#include "cmd_clear.h"
#include "cmd_disasm.h"
#include "cmd_dump.h"
#include "cmd_help.h"
#include "cmd_list.h"
#include "cmd_load.h"
#include "cmd_node.h"
#include "cmd_module.h"
#include "cmd_processor.h"
#include "cmd_quit.h"
#include "cmd_reset.h"
#include "cmd_run.h"
#include "cmd_set.h"
#include "cmd_step.h"
#include "cmd_stimulus.h"
#include "cmd_symbol.h"
#include "cmd_trace.h"
#include "cmd_version.h"
#include "cmd_x.h"

#define YYERROR_VERBOSE
void yyerror(char *message)
{
  printf("***ERROR: %s\n",message);
  //exit(1);
}

int yylex(void);
int quit_parse;
int parser_warnings;
int parser_spanning_lines=0;

char_list *str_list_head;
char_list *str_list;

void free_char_list(char_list *);
 

#line 77 "parse.yy"
typedef union {
  guint32              i;
  guint64             li;
  float                f;
  char                *s;
  cmd_options        *co;
  cmd_options_num   *con;
  cmd_options_float *cof;
  cmd_options_str   *cos;
} YYSTYPE;
#ifndef YYDEBUG
#define YYDEBUG 1
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		114
#define	YYFLAG		-32768
#define	YYNTBASE	36

#define YYTRANSLATE(x) ((unsigned)(x) <= 289 ? yytranslate[x] : 69)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
    27,    28,    29,    30,    31,    32,    33,    34,    35
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     6,     8,    10,    12,    14,    16,    18,
    20,    22,    24,    26,    28,    30,    32,    34,    36,    38,
    40,    42,    44,    46,    48,    50,    52,    54,    57,    59,
    62,    66,    71,    75,    80,    82,    85,    88,    90,    93,
    97,    99,   102,   104,   107,   109,   112,   115,   119,   121,
   124,   126,   129,   132,   134,   137,   140,   144,   146,   148,
   150,   152,   155,   159,   162,   164,   167,   170,   172,   175,
   179,   182,   186,   189,   192,   196,   197,   200,   203,   206,
   209,   212,   215,   218,   221,   223,   226,   231,   233,   236,
   238,   240,   243,   247,   250,   254,   257,   259,   261,   264,
   267,   270,   272
};

static const short yyrhs[] = {    37,
     0,    39,     0,    40,     0,    41,     0,    42,     0,    43,
     0,    44,     0,    45,     0,    46,     0,    47,     0,    48,
     0,    49,     0,    50,     0,    51,     0,    52,     0,    53,
     0,    54,     0,    55,     0,    56,     0,    58,     0,    59,
     0,    60,     0,    61,     0,    38,     0,    30,     0,    26,
     0,    27,     0,     3,    68,     0,     4,     0,     4,    64,
     0,     4,    64,    63,     0,     4,    64,    63,    34,     0,
     4,    64,    28,     0,     4,    64,    28,    34,     0,     5,
     0,     5,    68,     0,     6,    34,     0,     7,     0,     7,
    34,     0,     7,    34,    34,     0,     8,     0,     8,    64,
     0,     9,     0,     9,    28,     0,    11,     0,    11,    62,
     0,    11,    64,     0,    10,    64,    28,     0,    12,     0,
    12,    68,     0,    13,     0,    13,    64,     0,    13,    67,
     0,    14,     0,    14,    64,     0,    14,    28,     0,    14,
    28,    28,     0,    15,     0,    16,     0,    17,     0,    18,
     0,    18,    64,     0,    18,    64,    34,     0,    18,    65,
     0,    19,     0,    19,    34,     0,    19,    64,     0,    20,
     0,    20,    34,     0,    20,    57,    27,     0,    20,    27,
     0,    20,    57,    26,     0,    20,    26,     0,    20,    35,
     0,    20,    57,    25,     0,     0,    57,    27,     0,    57,
    26,     0,    57,    64,     0,    57,    65,     0,    57,    66,
     0,    57,    67,     0,    57,    34,     0,    57,    35,     0,
    21,     0,    21,    28,     0,    21,    28,    28,    34,     0,
    22,     0,    22,    34,     0,    23,     0,    24,     0,    24,
    34,     0,    24,    63,    34,     0,    24,    28,     0,    24,
    28,    34,     0,    29,    63,     0,    34,     0,    31,     0,
    32,    34,     0,    32,    35,     0,    33,    28,     0,    28,
     0,    68,    28,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   141,   142,   143,   144,   145,   146,   147,   148,   149,   150,
   151,   152,   153,   154,   155,   156,   157,   158,   159,   160,
   161,   162,   163,   164,   165,   174,   190,   200,   211,   213,
   218,   220,   222,   224,   228,   232,   241,   245,   247,   249,
   253,   255,   260,   262,   266,   268,   270,   278,   291,   295,
   304,   309,   311,   321,   323,   325,   327,   335,   343,   347,
   351,   355,   359,   363,   369,   371,   373,   377,   384,   391,
   398,   405,   412,   420,   428,   438,   444,   451,   457,   463,
   469,   475,   481,   487,   495,   499,   503,   510,   514,   520,
   526,   530,   534,   538,   542,   553,   561,   568,   575,   586,
   597,   608,   617
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","ATTACH",
"BREAK","BUS","CLEAR","DISASSEMBLE","DUMP","HELP","LOAD","LIST","NODE","MODULE",
"PROCESSOR","QUIT","RESET","RUN","SET","STEP","STIMULUS","SYMBOL","TRACE","gpsim_VERSION",
"X","END_OF_COMMAND","IGNORED","SPANNING_LINES","STRING","INDIRECT","END_OF_INPUT",
"BIT_FLAG","NUMERIC_OPTION","STRING_OPTION","NUMBER","FLOAT_NUMBER","cmd","ignored",
"spanning_lines","attach_cmd","break_cmd","bus_cmd","clear_cmd","disassemble_cmd",
"dump_cmd","help_cmd","list_cmd","load_cmd","node_cmd","module_cmd","processor_cmd",
"quit_cmd","reset_cmd","run_cmd","set_cmd","step_cmd","stimulus_cmd","stimulus_opt",
"symbol_cmd","trace_cmd","version_cmd","x_cmd","indirect","_register","bit_flag",
"numeric_option","numeric_float_option","string_option","string_list", NULL
};
#endif

static const short yyr1[] = {     0,
    36,    36,    36,    36,    36,    36,    36,    36,    36,    36,
    36,    36,    36,    36,    36,    36,    36,    36,    36,    36,
    36,    36,    36,    36,    36,    37,    38,    39,    40,    40,
    40,    40,    40,    40,    41,    41,    42,    43,    43,    43,
    44,    44,    45,    45,    46,    46,    46,    47,    48,    48,
    49,    49,    49,    50,    50,    50,    50,    51,    52,    53,
    54,    54,    54,    54,    55,    55,    55,    56,    56,    56,
    56,    56,    56,    56,    56,    57,    57,    57,    57,    57,
    57,    57,    57,    57,    58,    58,    58,    59,    59,    60,
    61,    61,    61,    61,    61,    62,    63,    64,    65,    66,
    67,    68,    68
};

static const short yyr2[] = {     0,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     2,     1,     2,
     3,     4,     3,     4,     1,     2,     2,     1,     2,     3,
     1,     2,     1,     2,     1,     2,     2,     3,     1,     2,
     1,     2,     2,     1,     2,     2,     3,     1,     1,     1,
     1,     2,     3,     2,     1,     2,     2,     1,     2,     3,
     2,     3,     2,     2,     3,     0,     2,     2,     2,     2,
     2,     2,     2,     2,     1,     2,     4,     1,     2,     1,
     1,     2,     3,     2,     3,     2,     1,     1,     2,     2,
     2,     1,     2
};

static const short yydefact[] = {     0,
     0,    29,    35,     0,    38,    41,    43,     0,    45,    49,
    51,    54,    58,    59,    60,    61,    65,    76,    85,    88,
    90,    91,    26,    27,    25,     1,    24,     2,     3,     4,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
    15,    16,    17,    18,    19,    20,    21,    22,    23,   102,
    28,    98,    30,    36,    37,    39,    42,    44,     0,     0,
    46,    47,    50,     0,    52,    53,    56,    55,     0,    62,
    64,    66,    67,    73,    71,    69,    74,     0,    86,    89,
    94,    92,     0,   103,    33,    97,    31,    40,    48,    96,
   101,    57,    99,    63,    75,    78,    77,     0,    83,    84,
    79,    80,    81,    82,     0,    95,    93,    34,    32,   100,
    87,     0,     0,     0
};

static const short yydefgoto[] = {   112,
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
    78,    46,    47,    48,    49,    61,    83,    53,    71,   103,
    66,    51
};

static const short yypact[] = {    34,
   -24,    -9,   -24,    -3,    25,    -9,    40,    -9,     3,   -24,
    32,    -7,-32768,-32768,-32768,    35,    -1,     1,    41,    28,
-32768,    -8,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    45,-32768,    -5,    45,-32768,    42,-32768,-32768,    46,    43,
-32768,-32768,    45,    47,-32768,-32768,    50,-32768,    48,    49,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,   -18,    51,-32768,
    52,    53,    54,-32768,    55,-32768,    56,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,    80,    81,    36,-32768,-32768,
-32768,-32768,-32768,-32768,    57,-32768,-32768,-32768,-32768,-32768,
-32768,    84,    85,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,   -41,    -6,    14,-32768,
    16,    15
};


#define	YYLAST		94


static const short yytable[] = {    57,
   -68,    59,    62,    50,    65,    68,    95,    96,    97,    70,
    73,    87,    52,    98,    64,    99,   100,    54,    90,    81,
    67,    52,    85,    52,    63,    82,    74,    75,    86,    52,
    55,    60,    72,    52,    76,    77,     1,     2,     3,     4,
     5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
    15,    16,    17,    18,    19,    20,    21,    22,    56,    23,
    24,    80,    52,    25,    64,    52,    69,    58,    79,    93,
   110,   101,    84,    89,    91,    88,    86,    92,   105,   -72,
   -70,    93,    94,   113,   114,   106,   -97,   107,   108,   109,
   111,   102,     0,   104
};

static const short yycheck[] = {     6,
     0,     8,     9,    28,    11,    12,    25,    26,    27,    16,
    17,    53,    31,    32,    33,    34,    35,     3,    60,    28,
    28,    31,    28,    31,    10,    34,    26,    27,    34,    31,
    34,    29,    34,    31,    34,    35,     3,     4,     5,     6,
     7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,    34,    26,
    27,    34,    31,    30,    33,    31,    32,    28,    28,    34,
    35,    78,    28,    28,    28,    34,    34,    28,    28,     0,
     0,    34,    34,     0,     0,    34,    34,    34,    34,    34,
    34,    78,    -1,    78
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"
/* This file comes from bison-1.27.  */

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

#ifndef YYSTACK_USE_ALLOCA
#ifdef alloca
#define YYSTACK_USE_ALLOCA
#else /* alloca not defined */
#ifdef __GNUC__
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi) || (defined (__sun) && defined (__i386))
#define YYSTACK_USE_ALLOCA
#include <alloca.h>
#else /* not sparc */
/* We think this test detects Watcom and Microsoft C.  */
/* This used to test MSDOS, but that is a bad idea
   since that symbol is in the user namespace.  */
#if (defined (_MSDOS) || defined (_MSDOS_)) && !defined (__TURBOC__)
#if 0 /* No need for malloc.h, which pollutes the namespace;
	 instead, just don't use alloca.  */
#include <malloc.h>
#endif
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
/* I don't know what this was needed for, but it pollutes the namespace.
   So I turned it off.   rms, 2 May 1997.  */
/* #include <malloc.h>  */
 #pragma alloca
#define YYSTACK_USE_ALLOCA
#else /* not MSDOS, or __TURBOC__, or _AIX */
#if 0
#ifdef __hpux /* haible@ilog.fr says this works for HPUX 9.05 and up,
		 and on HPUX 10.  Eventually we can turn this on.  */
#define YYSTACK_USE_ALLOCA
#define alloca __builtin_alloca
#endif /* __hpux */
#endif
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc */
#endif /* not GNU C */
#endif /* alloca not defined */
#endif /* YYSTACK_USE_ALLOCA not defined */

#ifdef YYSTACK_USE_ALLOCA
#define YYSTACK_ALLOC alloca
#else
#define YYSTACK_ALLOC malloc
#endif

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	goto yyacceptlab
#define YYABORT 	goto yyabortlab
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, &yylloc, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval, &yylloc)
#endif
#else /* not YYLSP_NEEDED */
#ifdef YYLEX_PARAM
#define YYLEX		yylex(&yylval, YYLEX_PARAM)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif /* not YYLSP_NEEDED */
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Define __yy_memcpy.  Note that the size argument
   should be passed with type unsigned int, because that is what the non-GCC
   definitions require.  With GCC, __builtin_memcpy takes an arg
   of type size_t, but it can handle unsigned int.  */

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_memcpy(TO,FROM,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (to, from, count)
     char *to;
     char *from;
     unsigned int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_memcpy (char *to, char *from, unsigned int count)
{
  register char *t = to;
  register char *f = from;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 216 "/usr/lib/bison.simple"

/* The user can define YYPARSE_PARAM as the name of an argument to be passed
   into yyparse.  The argument should have type void *.
   It should actually point to an object.
   Grammar actions can access the variable by casting it
   to the proper pointer type.  */

#ifdef YYPARSE_PARAM
#ifdef __cplusplus
#define YYPARSE_PARAM_ARG void *YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else /* not __cplusplus */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL void *YYPARSE_PARAM;
#endif /* not __cplusplus */
#else /* not YYPARSE_PARAM */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif /* not YYPARSE_PARAM */

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
#ifdef YYPARSE_PARAM
int yyparse (void *);
#else
int yyparse (void);
#endif
#endif

int
yyparse(YYPARSE_PARAM_ARG)
     YYPARSE_PARAM_DECL
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;
  int yyfree_stacks = 0;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  if (yyfree_stacks)
	    {
	      free (yyss);
	      free (yyvs);
#ifdef YYLSP_NEEDED
	      free (yyls);
#endif
	    }
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
#ifndef YYSTACK_USE_ALLOCA
      yyfree_stacks = 1;
#endif
      yyss = (short *) YYSTACK_ALLOC (yystacksize * sizeof (*yyssp));
      __yy_memcpy ((char *)yyss, (char *)yyss1,
		   size * (unsigned int) sizeof (*yyssp));
      yyvs = (YYSTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yyvsp));
      __yy_memcpy ((char *)yyvs, (char *)yyvs1,
		   size * (unsigned int) sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) YYSTACK_ALLOC (yystacksize * sizeof (*yylsp));
      __yy_memcpy ((char *)yyls, (char *)yyls1,
		   size * (unsigned int) sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 25:
#line 166 "parse.yy"
{
       if((verbose&2) && DEBUG_PARSER)
         cout << "got an END_OF_INPUT\n";
       quit_parse = 1;
       YYABORT;
     ;
    break;}
case 26:
#line 175 "parse.yy"
{
            //if(parser_warnings || (verbose & 2 ))
            if((verbose & 2) && DEBUG_PARSER)
              cout << "parser is ignoring input\n";

            if(!parser_spanning_lines) {
              if((verbose & 2) && DEBUG_PARSER)
                cout << "  parser is aborting current input stream\n";

	      YYABORT;
            } else
		YYACCEPT;
          ;
    break;}
case 27:
#line 191 "parse.yy"
{
            if((verbose) && DEBUG_PARSER)
              cout << "parser is spanning lines\n";

            parser_spanning_lines = 1;
	    YYACCEPT;
          ;
    break;}
case 28:
#line 201 "parse.yy"
{
            if((verbose&2) && DEBUG_PARSER)
	      cout << "attach command with a string list\n";
	    attach.attach(str_list_head);
	    free_char_list(str_list_head);
            YYABORT;

	  ;
    break;}
case 29:
#line 212 "parse.yy"
{ c_break.list(); YYABORT;;
    break;}
case 30:
#line 214 "parse.yy"
{ 
	    cmd_options *opt = yyvsp[0].co;
	    c_break.set_break(opt->value); YYABORT;
	  ;
    break;}
case 31:
#line 219 "parse.yy"
{ c_break.set_break(yyvsp[-1].co->value,yyvsp[0].li); YYABORT;;
    break;}
case 32:
#line 221 "parse.yy"
{ c_break.set_break(yyvsp[-2].co->value,yyvsp[-1].li,yyvsp[0].li); YYABORT;;
    break;}
case 33:
#line 223 "parse.yy"
{ c_break.set_break(yyvsp[-1].co->value,yyvsp[0].s); YYABORT;;
    break;}
case 34:
#line 225 "parse.yy"
{ c_break.set_break(yyvsp[-2].co->value,yyvsp[-1].s,yyvsp[0].li); YYABORT;;
    break;}
case 35:
#line 229 "parse.yy"
{ 
	    c_bus.list_busses(); YYABORT;
	  ;
    break;}
case 36:
#line 233 "parse.yy"
{
	    //cout << "bus command with a string list\n";
	    c_bus.add_busses(str_list_head);
	    free_char_list(str_list_head);
            YYABORT;
          ;
    break;}
case 37:
#line 242 "parse.yy"
{ clear.clear(yyvsp[0].li); YYABORT;;
    break;}
case 38:
#line 246 "parse.yy"
{ disassemble.disassemble(-10, 5); YYABORT; ;
    break;}
case 39:
#line 248 "parse.yy"
{ disassemble.disassemble(0, yyvsp[0].li); YYABORT;;
    break;}
case 40:
#line 250 "parse.yy"
{ disassemble.disassemble(-yyvsp[-1].li,yyvsp[0].li); YYABORT;;
    break;}
case 41:
#line 254 "parse.yy"
{ dump.dump(2); YYABORT;;
    break;}
case 42:
#line 256 "parse.yy"
{ dump.dump(yyvsp[0].co->value); YYABORT;;
    break;}
case 43:
#line 261 "parse.yy"
{ help.help(); YYABORT;;
    break;}
case 44:
#line 263 "parse.yy"
{ help.help(yyvsp[0].s); free(yyvsp[0].s); YYABORT;;
    break;}
case 45:
#line 267 "parse.yy"
{ c_list.list(); YYABORT;;
    break;}
case 46:
#line 269 "parse.yy"
{ printf("got a list with an indirect reference %d\n",yyvsp[0].li);YYABORT;;
    break;}
case 47:
#line 271 "parse.yy"
{ 
	    cmd_options *opt = yyvsp[0].co;
	    //cout << "  --- list with bit flag " << opt->name << '\n';
	    c_list.list(yyvsp[0].co); YYABORT;
	  ;
    break;}
case 48:
#line 279 "parse.yy"
{
	    c_load.load(yyvsp[-1].co->value,yyvsp[0].s);
	    //cout << "load completed\n\n";
	    if(quit_parse)
	      {
		quit_parse = 0;
		YYABORT;
	      }
	     YYABORT;
	  ;
    break;}
case 49:
#line 292 "parse.yy"
{ 
	    c_node.list_nodes(); YYABORT;
	  ;
    break;}
case 50:
#line 296 "parse.yy"
{
	    //cout << "node command with a string list\n";
	    c_node.add_nodes(str_list_head);
	    free_char_list(str_list_head);
            YYABORT;
          ;
    break;}
case 51:
#line 305 "parse.yy"
{ 
            cout << "module command\n";
            c_module.module(); YYABORT;
          ;
    break;}
case 52:
#line 310 "parse.yy"
{ c_module.module(yyvsp[0].co->value); YYABORT;;
    break;}
case 53:
#line 312 "parse.yy"
{ 
            c_module.module(yyvsp[0].cos);
            delete yyvsp[0].cos;
            YYABORT;
          ;
    break;}
case 54:
#line 322 "parse.yy"
{ c_processor.processor(); YYABORT;;
    break;}
case 55:
#line 324 "parse.yy"
{ c_processor.processor(yyvsp[0].co->value); YYABORT;;
    break;}
case 56:
#line 326 "parse.yy"
{ c_processor.processor(yyvsp[0].s,NULL); YYABORT; ;
    break;}
case 57:
#line 328 "parse.yy"
{ 
            c_processor.processor(yyvsp[-1].s,yyvsp[0].s);
            YYABORT;
          ;
    break;}
case 58:
#line 336 "parse.yy"
{ 
            printf("got a quit\n");
            quit_parse = 1;
	    YYABORT;
          ;
    break;}
case 59:
#line 344 "parse.yy"
{ reset.reset(); YYABORT; ;
    break;}
case 60:
#line 348 "parse.yy"
{ c_run.run(); YYABORT;;
    break;}
case 61:
#line 352 "parse.yy"
{ 
            c_set.set(); YYABORT;
          ;
    break;}
case 62:
#line 356 "parse.yy"
{
            c_set.set(yyvsp[0].co->value,1); YYABORT;
          ;
    break;}
case 63:
#line 360 "parse.yy"
{
            c_set.set(yyvsp[-1].co->value,yyvsp[0].li);YYABORT;
          ;
    break;}
case 64:
#line 364 "parse.yy"
{
	    c_set.set(yyvsp[0].con); YYABORT;
	  ;
    break;}
case 65:
#line 370 "parse.yy"
{ step.step(1); YYABORT;;
    break;}
case 66:
#line 372 "parse.yy"
{ step.step(yyvsp[0].li); YYABORT;;
    break;}
case 67:
#line 374 "parse.yy"
{ step.over(); YYABORT;;
    break;}
case 68:
#line 378 "parse.yy"
{
            if(verbose)
              cout << "parser sees stimulus\n";
	    c_stimulus.stimulus();
	    YYABORT;
	  ;
    break;}
case 69:
#line 385 "parse.yy"
{ 
            if(verbose)
              cout << "parser sees stimulus with number: " << yyvsp[0].li << '\n';

	    c_stimulus.stimulus(yyvsp[0].li);
	  ;
    break;}
case 70:
#line 392 "parse.yy"
{ 
            if(verbose)
              cout << " stimulus cmd is spanning a line\n";

	    //YYACCEPT;
	  ;
    break;}
case 71:
#line 399 "parse.yy"
{ 
            if(verbose)
              cout << " stimulus cmd is spanning a line\n";

	    //YYACCEPT;
	  ;
    break;}
case 72:
#line 406 "parse.yy"
{ 
            if(verbose)
              cout << " stimulus cmd is ignoring stuff\n";

	    //YYACCEPT;
	  ;
    break;}
case 73:
#line 413 "parse.yy"
{ 
            if(verbose)
              cout << " stimulus cmd is ignoring stuff\n";

	    c_stimulus.stimulus();
	    YYABORT;
	  ;
    break;}
case 74:
#line 421 "parse.yy"
{ 
            if(verbose)
              cout << "parser sees stimulus with float number: " << yyvsp[0].f << '\n';

	    c_stimulus.stimulus(yyvsp[0].f);
	  ;
    break;}
case 75:
#line 429 "parse.yy"
{ 
            if(verbose)
	      cout << " end of stimulus command\n";
	    c_stimulus.end();
            parser_spanning_lines = 0;
	    YYACCEPT;
	  ;
    break;}
case 76:
#line 439 "parse.yy"
{
            if(verbose)
              cout << "parser sees stimulus(in _opt)\n"; // << $1->value << '\n';
	    //c_stimulus.stimulus($1->value);
	  ;
    break;}
case 77:
#line 445 "parse.yy"
{
            //if(verbose)
              //cout << "parser is ignoring spanned line in stimulus\n";
            parser_spanning_lines=1;
            //YYACCEPT;
          ;
    break;}
case 78:
#line 452 "parse.yy"
{
            //if(verbose)
              //cout << "parser is ignoring garbage in stimulus\n";
            //YYACCEPT;
          ;
    break;}
case 79:
#line 458 "parse.yy"
{
            if(verbose)
              cout << "parser sees stimulus with bit flag: " << yyvsp[0].co->value << '\n';
	    c_stimulus.stimulus(yyvsp[0].co->value);
	  ;
    break;}
case 80:
#line 464 "parse.yy"
{
            if(verbose)
              cout << "parser sees stimulus with numeric option\n";
	    c_stimulus.stimulus(yyvsp[0].con);
	  ;
    break;}
case 81:
#line 470 "parse.yy"
{
            if(verbose)
              cout << "parser sees stimulus with numeric float option\n";
	    c_stimulus.stimulus(yyvsp[0].cof);
	  ;
    break;}
case 82:
#line 476 "parse.yy"
{
            if(verbose)
              cout << "parser sees stimulus with string option\n";
	    c_stimulus.stimulus(yyvsp[0].cos);
	  ;
    break;}
case 83:
#line 482 "parse.yy"
{ 
            if(verbose)
              cout << "parser sees stimulus with number\n";
	    c_stimulus.data_point(yyvsp[0].li);
	  ;
    break;}
case 84:
#line 488 "parse.yy"
{ 
            if(verbose)
              cout << "parser sees stimulus with floating point number\n";
	    c_stimulus.data_point(yyvsp[0].f);
	  ;
    break;}
case 85:
#line 496 "parse.yy"
{
	    c_symbol.dump_all(); YYABORT;
	  ;
    break;}
case 86:
#line 500 "parse.yy"
{
	    c_symbol.dump_one(yyvsp[0].s); YYABORT;
	  ;
    break;}
case 87:
#line 504 "parse.yy"
{
	    c_symbol.add_one(yyvsp[-2].s,yyvsp[-1].s,yyvsp[0].li); YYABORT;
	  ;
    break;}
case 88:
#line 511 "parse.yy"
{
	    c_trace.trace(); YYABORT;
	  ;
    break;}
case 89:
#line 515 "parse.yy"
{
	    c_trace.trace(yyvsp[0].li); YYABORT;
	  ;
    break;}
case 90:
#line 521 "parse.yy"
{
	    version.version(); YYABORT;
	  ;
    break;}
case 91:
#line 527 "parse.yy"
{
	    c_x.x(); YYABORT;
	  ;
    break;}
case 92:
#line 531 "parse.yy"
{
	    c_x.x(yyvsp[0].li); YYABORT;
	  ;
    break;}
case 93:
#line 535 "parse.yy"
{
	    c_x.x(yyvsp[-1].li,yyvsp[0].li); YYABORT;
	  ;
    break;}
case 94:
#line 539 "parse.yy"
{
	    c_x.x(yyvsp[0].s); YYABORT;
	  ;
    break;}
case 95:
#line 543 "parse.yy"
{
	    c_x.x(yyvsp[-1].s,yyvsp[0].li); YYABORT;
	  ;
    break;}
case 96:
#line 554 "parse.yy"
{
	  if(verbose)
            printf(" indirect register *%d",yyvsp[0].li);
	  yyval.li = yyvsp[0].li;
        ;
    break;}
case 97:
#line 562 "parse.yy"
{
	if(verbose)
         printf("  --- register %d\n", yyvsp[0].li);
       ;
    break;}
case 98:
#line 569 "parse.yy"
{
	 yyval.co = yyvsp[0].co;
	 //cout << "  --- bit_flag " << $$->name << '\n';
       ;
    break;}
case 99:
#line 576 "parse.yy"
{ 
	  //cout << $1->name;
	  yyval.con = new cmd_options_num;
	  yyval.con->co = yyvsp[-1].co;
	  yyval.con->n  = yyvsp[0].li;
          if(verbose&2)
	    cout << "name " << yyval.con->co->name << " value " << yyval.con->n << " got a numeric option \n"; 
	;
    break;}
case 100:
#line 587 "parse.yy"
{ 
	  //cout << $1->name;
	  yyval.cof = new cmd_options_float;
	  yyval.cof->co = yyvsp[-1].co;
	  yyval.cof->f  = yyvsp[0].f;
          if(verbose&2)
	    cout << "name " << yyval.cof->co->name << " value " << yyval.cof->f << " got a numeric option \n"; 
	;
    break;}
case 101:
#line 598 "parse.yy"
{ 
	  //cout << $1->name;
	  yyval.cos = new cmd_options_str;
	  yyval.cos->co  = yyvsp[-1].co;
	  yyval.cos->str = strdup(yyvsp[0].s);
          if(verbose&2)
	    cout << " name " << yyval.cos->co->name << " value " << yyval.cos->str << " got a string option \n"; 
	;
    break;}
case 102:
#line 609 "parse.yy"
{
	  str_list = (char_list *) malloc(sizeof(char_list)); //new(char_list);
	  str_list_head = str_list;
	  str_list->name = strdup(yyvsp[0].s);
	  str_list->next = NULL;
	  if(verbose&2)
	    cout << "got a string. added " << str_list->name << '\n';
	;
    break;}
case 103:
#line 618 "parse.yy"
{
	  str_list->next = (char_list *) malloc(sizeof(char_list)); //new(char_list);
	  str_list = str_list->next;
	  str_list->name = strdup(yyvsp[0].s);
	  str_list->next = NULL;
	  if(verbose&2)
	    cout << " -- have a list of strings. added " << str_list->name << '\n';
	;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 542 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;

 yyacceptlab:
  /* YYACCEPT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 0;

 yyabortlab:
  /* YYABORT comes here.  */
  if (yyfree_stacks)
    {
      free (yyss);
      free (yyvs);
#ifdef YYLSP_NEEDED
      free (yyls);
#endif
    }
  return 1;
}
#line 629 "parse.yy"


       // parsing is over 

//--------------------------
// This initialization could be done by the compiler. However
// it requires two passes through because the token values are
// defined by the parser output (eg. y.tab.h) while at the same
// time the parser depends on the .h files in which these classes
// are defined.

void initialize_commands(void)
{
  static bool initialized = 0;

  if(initialized)
    return;

  if(verbose)
    cout << __FUNCTION__ << "()\n";

  attach.token_value = ATTACH;
  c_break.token_value = BREAK;
  c_bus.token_value = BUS;
  clear.token_value = CLEAR;
  disassemble.token_value = DISASSEMBLE;
  dump.token_value = DUMP;
  help.token_value = HELP;
  c_list.token_value = LIST;
  c_load.token_value = LOAD;
  c_module.token_value = MODULE;
  c_node.token_value = NODE;
  c_processor.token_value = PROCESSOR;
  quit.token_value = QUIT;
  reset.token_value = RESET;
  c_run.token_value = RUN;
  c_set.token_value = SET;
  step.token_value = STEP;
  c_stimulus.token_value = STIMULUS;
  c_symbol.token_value = SYMBOL;
  c_trace.token_value = TRACE;
  version.token_value = gpsim_VERSION;
  c_x.token_value = X;

  initialized = 1;

  parser_spanning_lines = 0;
  parser_warnings = 1; // display parser warnings.
}

void free_char_list(char_list *chl)
{
  char_list *old_node;

  while(chl)
    {

      old_node = chl;
      chl = chl->next;

      free (old_node->name);
      free (old_node);

    }

}
