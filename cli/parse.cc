
/*  A Bison parser, made from parse.yy
 by  GNU Bison version 1.27
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	ATTACH	257
#define	BREAK	258
#define	CLEAR	259
#define	DISASSEMBLE	260
#define	DUMP	261
#define	HELP	262
#define	LOAD	263
#define	LIST	264
#define	NODE	265
#define	PROCESSOR	266
#define	QUIT	267
#define	RESET	268
#define	RUN	269
#define	SET	270
#define	STEP	271
#define	STIMULUS	272
#define	SYMBOL	273
#define	TRACE	274
#define	gpsim_VERSION	275
#define	X	276
#define	END_OF_COMMAND	277
#define	STRING	278
#define	INDIRECT	279
#define	END_OF_INPUT	280
#define	BIT_FLAG	281
#define	NUMERIC_OPTION	282
#define	STRING_OPTION	283
#define	NUMBER	284

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
#include "cmd_clear.h"
#include "cmd_disasm.h"
#include "cmd_dump.h"
#include "cmd_help.h"
#include "cmd_list.h"
#include "cmd_load.h"
#include "cmd_node.h"
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


void yyerror(char *message)
{
  printf("***ERROR: %s\n",message);
  //exit(1);
}

int yylex(void);
int quit_parse;

char_list *str_list_head;
char_list *str_list;

void free_char_list(char_list *);
 

#line 73 "parse.yy"
typedef union {
  guint32            i;
  guint64           li;
  char              *s;
  cmd_options      *co;
  cmd_options_num *con;
  cmd_options_str *cos;
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



#define	YYFINAL		94
#define	YYFLAG		-32768
#define	YYNTBASE	31

#define YYTRANSLATE(x) ((unsigned)(x) <= 284 ? yytranslate[x] : 59)

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
    27,    28,    29,    30
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     2,     4,     6,     8,    10,    12,    14,    16,    18,
    20,    22,    24,    26,    28,    30,    32,    34,    36,    38,
    40,    42,    45,    47,    50,    54,    59,    63,    68,    71,
    73,    76,    80,    82,    85,    87,    90,    92,    95,    98,
   102,   104,   107,   109,   112,   115,   119,   121,   123,   125,
   127,   130,   134,   137,   139,   142,   145,   147,   151,   153,
   156,   159,   162,   165,   167,   170,   175,   177,   180,   182,
   184,   187,   191,   194,   198,   201,   203,   205,   208,   211,
   213
};

static const short yyrhs[] = {    32,
     0,    33,     0,    34,     0,    35,     0,    36,     0,    37,
     0,    38,     0,    39,     0,    40,     0,    41,     0,    42,
     0,    43,     0,    44,     0,    45,     0,    46,     0,    47,
     0,    49,     0,    50,     0,    51,     0,    52,     0,    26,
     0,     3,    58,     0,     4,     0,     4,    55,     0,     4,
    55,    54,     0,     4,    55,    54,    30,     0,     4,    55,
    24,     0,     4,    55,    24,    30,     0,     5,    30,     0,
     6,     0,     6,    30,     0,     6,    30,    30,     0,     7,
     0,     7,    55,     0,     8,     0,     8,    24,     0,    10,
     0,    10,    53,     0,    10,    55,     0,     9,    55,    24,
     0,    11,     0,    11,    58,     0,    12,     0,    12,    55,
     0,    12,    24,     0,    12,    24,    24,     0,    13,     0,
    14,     0,    15,     0,    16,     0,    16,    55,     0,    16,
    55,    30,     0,    16,    56,     0,    17,     0,    17,    30,
     0,    17,    55,     0,    18,     0,    18,    48,    23,     0,
    55,     0,    48,    55,     0,    48,    56,     0,    48,    57,
     0,    48,    30,     0,    19,     0,    19,    24,     0,    19,
    24,    24,    30,     0,    20,     0,    20,    30,     0,    21,
     0,    22,     0,    22,    30,     0,    22,    54,    30,     0,
    22,    24,     0,    22,    24,    30,     0,    25,    54,     0,
    30,     0,    27,     0,    28,    30,     0,    29,    24,     0,
    24,     0,    58,    24,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   128,   129,   130,   131,   132,   133,   134,   135,   136,   137,
   138,   139,   140,   141,   142,   143,   144,   145,   146,   147,
   148,   156,   164,   166,   171,   173,   175,   177,   181,   185,
   187,   189,   193,   195,   200,   202,   206,   208,   210,   218,
   230,   234,   242,   244,   246,   248,   253,   260,   264,   268,
   272,   276,   280,   286,   288,   290,   294,   303,   310,   314,
   318,   322,   326,   332,   336,   340,   347,   351,   357,   363,
   367,   371,   375,   379,   390,   398,   405,   412,   422,   432,
   440
};
#endif


#if YYDEBUG != 0 || defined (YYERROR_VERBOSE)

static const char * const yytname[] = {   "$","error","$undefined.","ATTACH",
"BREAK","CLEAR","DISASSEMBLE","DUMP","HELP","LOAD","LIST","NODE","PROCESSOR",
"QUIT","RESET","RUN","SET","STEP","STIMULUS","SYMBOL","TRACE","gpsim_VERSION",
"X","END_OF_COMMAND","STRING","INDIRECT","END_OF_INPUT","BIT_FLAG","NUMERIC_OPTION",
"STRING_OPTION","NUMBER","cmd","attach_cmd","break_cmd","clear_cmd","disassemble_cmd",
"dump_cmd","help_cmd","list_cmd","load_cmd","node_cmd","processor_cmd","quit_cmd",
"reset_cmd","run_cmd","set_cmd","step_cmd","stimulus_cmd","stimulus_opt","symbol_cmd",
"trace_cmd","version_cmd","x_cmd","indirect","_register","bit_flag","numeric_option",
"string_option","string_list", NULL
};
#endif

static const short yyr1[] = {     0,
    31,    31,    31,    31,    31,    31,    31,    31,    31,    31,
    31,    31,    31,    31,    31,    31,    31,    31,    31,    31,
    31,    32,    33,    33,    33,    33,    33,    33,    34,    35,
    35,    35,    36,    36,    37,    37,    38,    38,    38,    39,
    40,    40,    41,    41,    41,    41,    42,    43,    44,    45,
    45,    45,    45,    46,    46,    46,    47,    47,    48,    48,
    48,    48,    48,    49,    49,    49,    50,    50,    51,    52,
    52,    52,    52,    52,    53,    54,    55,    56,    57,    58,
    58
};

static const short yyr2[] = {     0,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
     1,     2,     1,     2,     3,     4,     3,     4,     2,     1,
     2,     3,     1,     2,     1,     2,     1,     2,     2,     3,
     1,     2,     1,     2,     2,     3,     1,     1,     1,     1,
     2,     3,     2,     1,     2,     2,     1,     3,     1,     2,
     2,     2,     2,     1,     2,     4,     1,     2,     1,     1,
     2,     3,     2,     3,     2,     1,     1,     2,     2,     1,
     2
};

static const short yydefact[] = {     0,
     0,    23,     0,    30,    33,    35,     0,    37,    41,    43,
    47,    48,    49,    50,    54,    57,    64,    67,    69,    70,
    21,     1,     2,     3,     4,     5,     6,     7,     8,     9,
    10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    20,    80,    22,    77,    24,    29,    31,    34,    36,     0,
     0,    38,    39,    42,    45,    44,     0,    51,    53,    55,
    56,     0,    59,    65,    68,    73,    71,     0,    81,    27,
    76,    25,    32,    40,    75,    46,    78,    52,    58,     0,
    63,    60,    61,    62,     0,    74,    72,    28,    26,    79,
    66,     0,     0,     0
};

static const short yydefgoto[] = {    92,
    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    32,    33,    34,    35,    36,    37,    62,    38,    39,    40,
    41,    52,    68,    45,    59,    84,    43
};

static const short yypact[] = {     9,
   -16,     5,     4,    14,     5,    27,     5,   -21,   -16,    19,
-32768,-32768,-32768,    22,    18,     5,    28,    23,-32768,   -23,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,    30,-32768,    12,-32768,    25,-32768,-32768,    32,
    29,-32768,-32768,    30,    34,-32768,    31,    33,-32768,-32768,
-32768,    10,-32768,    36,-32768,    35,    37,    38,-32768,    39,
-32768,    40,-32768,-32768,-32768,-32768,-32768,-32768,-32768,    42,
-32768,-32768,-32768,-32768,    41,-32768,-32768,-32768,-32768,-32768,
-32768,    62,    64,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,    -4,    -5,    11,-32768,    63
};


#define	YYLAST		73


static const short yytable[] = {    48,
    66,    50,    53,    51,    56,    44,    67,    42,    58,    61,
    63,     1,     2,     3,     4,     5,     6,     7,     8,     9,
    10,    11,    12,    13,    14,    15,    16,    17,    18,    19,
    20,    44,    79,    46,    21,    70,    44,    57,    80,    81,
    72,    71,    55,    47,    44,    44,    75,    60,    44,    57,
    49,    64,    65,    69,    73,    74,    82,    76,    71,    85,
    77,    93,    78,    94,    86,    90,   -76,    87,    88,    89,
    91,    54,    83
};

static const short yycheck[] = {     5,
    24,     7,     8,    25,    10,    27,    30,    24,    14,    15,
    16,     3,     4,     5,     6,     7,     8,     9,    10,    11,
    12,    13,    14,    15,    16,    17,    18,    19,    20,    21,
    22,    27,    23,    30,    26,    24,    27,    28,    29,    30,
    45,    30,    24,    30,    27,    27,    51,    30,    27,    28,
    24,    24,    30,    24,    30,    24,    62,    24,    30,    24,
    30,     0,    30,     0,    30,    24,    30,    30,    30,    30,
    30,     9,    62
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

case 21:
#line 149 "parse.yy"
{
	     //cout << "bug? got an END_OF_INPUT\n";
       quit_parse = 1;
       YYABORT;
     ;
    break;}
case 22:
#line 157 "parse.yy"
{
	    //cout << "attach command with a string list\n";
	    attach.attach(str_list_head);
	    free_char_list(str_list_head);
	  ;
    break;}
case 23:
#line 165 "parse.yy"
{ c_break.list(); ;
    break;}
case 24:
#line 167 "parse.yy"
{ 
	    cmd_options *opt = yyvsp[0].co;
	    c_break.set_break(opt->value); 
	  ;
    break;}
case 25:
#line 172 "parse.yy"
{ c_break.set_break(yyvsp[-1].co->value,yyvsp[0].li); ;
    break;}
case 26:
#line 174 "parse.yy"
{ c_break.set_break(yyvsp[-2].co->value,yyvsp[-1].li,yyvsp[0].li); ;
    break;}
case 27:
#line 176 "parse.yy"
{ c_break.set_break(yyvsp[-1].co->value,yyvsp[0].s); ;
    break;}
case 28:
#line 178 "parse.yy"
{ c_break.set_break(yyvsp[-2].co->value,yyvsp[-1].s,yyvsp[0].li); ;
    break;}
case 29:
#line 182 "parse.yy"
{ clear.clear(yyvsp[0].li); ;
    break;}
case 30:
#line 186 "parse.yy"
{ disassemble.disassemble(-10, 5);
    break;}
case 31:
#line 188 "parse.yy"
{ disassemble.disassemble(0, yyvsp[0].li);
    break;}
case 32:
#line 190 "parse.yy"
{ disassemble.disassemble(-yyvsp[-1].li,yyvsp[0].li);
    break;}
case 33:
#line 194 "parse.yy"
{ dump.dump(2);;
    break;}
case 34:
#line 196 "parse.yy"
{ dump.dump(yyvsp[0].co->value);;
    break;}
case 35:
#line 201 "parse.yy"
{ help.help(); ;
    break;}
case 36:
#line 203 "parse.yy"
{ help.help(yyvsp[0].s); free(yyvsp[0].s); ;
    break;}
case 37:
#line 207 "parse.yy"
{ c_list.list();;
    break;}
case 38:
#line 209 "parse.yy"
{ printf("got a list with an indirect reference %d\n",yyvsp[0].li);;
    break;}
case 39:
#line 211 "parse.yy"
{ 
	    cmd_options *opt = yyvsp[0].co;
	    //cout << "  --- list with bit flag " << opt->name << '\n';
	    c_list.list(yyvsp[0].co);
	  ;
    break;}
case 40:
#line 219 "parse.yy"
{
	    c_load.load(yyvsp[-1].co->value,yyvsp[0].s);
	    //cout << "load completed\n\n";
	    if(quit_parse)
	      {
		quit_parse = 0;
		YYABORT;
	      }
	  ;
    break;}
case 41:
#line 231 "parse.yy"
{ 
	    c_node.list_nodes();
	  ;
    break;}
case 42:
#line 235 "parse.yy"
{
	    //cout << "node command with a string list\n";
	    c_node.add_nodes(str_list_head);
	    free_char_list(str_list_head);
          ;
    break;}
case 43:
#line 243 "parse.yy"
{ c_processor.processor(); ;
    break;}
case 44:
#line 245 "parse.yy"
{ c_processor.processor(yyvsp[0].co->value); ;
    break;}
case 45:
#line 247 "parse.yy"
{ c_processor.processor(yyvsp[0].s,NULL); ;
    break;}
case 46:
#line 249 "parse.yy"
{ c_processor.processor(yyvsp[-1].s,yyvsp[0].s); ;
    break;}
case 47:
#line 254 "parse.yy"
{ 
            printf("got a quit\n");
            quit_parse = 1;
          ;
    break;}
case 48:
#line 261 "parse.yy"
{ reset.reset(); ;
    break;}
case 49:
#line 265 "parse.yy"
{ c_run.run(); ;
    break;}
case 50:
#line 269 "parse.yy"
{ 
            c_set.set();
          ;
    break;}
case 51:
#line 273 "parse.yy"
{
            c_set.set(yyvsp[0].co->value,1);
          ;
    break;}
case 52:
#line 277 "parse.yy"
{
            c_set.set(yyvsp[-1].co->value,yyvsp[0].li);
          ;
    break;}
case 53:
#line 281 "parse.yy"
{
	    c_set.set(yyvsp[0].con);
	  ;
    break;}
case 54:
#line 287 "parse.yy"
{ step.step(1); ;
    break;}
case 55:
#line 289 "parse.yy"
{ step.step(yyvsp[0].li); ;
    break;}
case 56:
#line 291 "parse.yy"
{ step.over(); ;
    break;}
case 57:
#line 295 "parse.yy"
{
	    c_stimulus.stimulus();
	  ;
    break;}
case 58:
#line 304 "parse.yy"
{ 
	    printf(" end of stimulus command\n");
	    c_stimulus.end();
	  ;
    break;}
case 59:
#line 311 "parse.yy"
{
	    c_stimulus.stimulus(yyvsp[0].co->value);
	  ;
    break;}
case 60:
#line 315 "parse.yy"
{
	    c_stimulus.stimulus(yyvsp[0].co->value);
	  ;
    break;}
case 61:
#line 319 "parse.yy"
{
	    c_stimulus.stimulus(yyvsp[0].con);
	  ;
    break;}
case 62:
#line 323 "parse.yy"
{
	    c_stimulus.stimulus(yyvsp[0].cos);
	  ;
    break;}
case 63:
#line 327 "parse.yy"
{ 
	    c_stimulus.data_point(yyvsp[0].li);
	  ;
    break;}
case 64:
#line 333 "parse.yy"
{
	    c_symbol.dump_all();
	  ;
    break;}
case 65:
#line 337 "parse.yy"
{
	    c_symbol.dump_one(yyvsp[0].s);
	  ;
    break;}
case 66:
#line 341 "parse.yy"
{
	    c_symbol.add_one(yyvsp[-2].s,yyvsp[-1].s,yyvsp[0].li);
	  ;
    break;}
case 67:
#line 348 "parse.yy"
{
	    c_trace.trace();
	  ;
    break;}
case 68:
#line 352 "parse.yy"
{
	    c_trace.trace(yyvsp[0].li);
	  ;
    break;}
case 69:
#line 358 "parse.yy"
{
	    version.version();
	  ;
    break;}
case 70:
#line 364 "parse.yy"
{
	    c_x.x();
	  ;
    break;}
case 71:
#line 368 "parse.yy"
{
	    c_x.x(yyvsp[0].li);
	  ;
    break;}
case 72:
#line 372 "parse.yy"
{
	    c_x.x(yyvsp[-1].li,yyvsp[0].li);
	  ;
    break;}
case 73:
#line 376 "parse.yy"
{
	    c_x.x(yyvsp[0].s);
	  ;
    break;}
case 74:
#line 380 "parse.yy"
{
	    c_x.x(yyvsp[-1].s,yyvsp[0].li);
	  ;
    break;}
case 75:
#line 391 "parse.yy"
{
	  if(verbose)
            printf(" indirect register *%d",yyvsp[0].li);
	  yyval.li = yyvsp[0].li;
        ;
    break;}
case 76:
#line 399 "parse.yy"
{
	if(verbose)
         printf("  --- register %d\n", yyvsp[0].li);
       ;
    break;}
case 77:
#line 406 "parse.yy"
{
	 yyval.co = yyvsp[0].co;
	 //cout << "  --- bit_flag " << $$->name << '\n';
       ;
    break;}
case 78:
#line 413 "parse.yy"
{ 
	  //cout << $1->name;
	  yyval.con = new cmd_options_num;
	  yyval.con->co = yyvsp[-1].co;
	  yyval.con->n  = yyvsp[0].li;
	  //cout << "name " << $$->co->name << " value " << $$->n << " got a numeric option \n"; 
	;
    break;}
case 79:
#line 423 "parse.yy"
{ 
	  //cout << $1->name;
	  yyval.cos = new cmd_options_str;
	  yyval.cos->co  = yyvsp[-1].co;
	  yyval.cos->str = strdup(yyvsp[0].s);
	  //cout << " name " << $$->co->name << " value " << $$->str << " got a string option \n"; 
	;
    break;}
case 80:
#line 433 "parse.yy"
{
	  //cout << "got a string \n";
	  str_list = new(char_list);
	  str_list_head = str_list;
	  str_list->name = strdup(yyvsp[0].s);
	  str_list->next = NULL;
	;
    break;}
case 81:
#line 441 "parse.yy"
{
	  //cout << " -- have a list of strings \n";
	  str_list->next = new(char_list);
	  str_list = str_list->next;
	  str_list->name = strdup(yyvsp[0].s);
	  str_list->next = NULL;
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
#line 451 "parse.yy"


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

  attach.token_value = ATTACH;
  c_break.token_value = BREAK;
  clear.token_value = CLEAR;
  disassemble.token_value = DISASSEMBLE;
  dump.token_value = DUMP;
  help.token_value = HELP;
  c_list.token_value = LIST;
  c_load.token_value = LOAD;
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

}

void free_char_list(char_list *chl)
{
  char_list *old_node;

  while(chl)
    {

      old_node = chl;
      chl = chl->next;

      free(old_node->name);
      free(old_node);

    }
}
