/* A Bison parser, made by GNU Bison 1.875d.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ABORT = 258,
     ATTACH = 259,
     BREAK = 260,
     BUS = 261,
     CLEAR = 262,
     DISASSEMBLE = 263,
     DUMP = 264,
     FREQUENCY = 265,
     HELP = 266,
     LOAD = 267,
     LOG = 268,
     LIST = 269,
     NODE = 270,
     MODULE = 271,
     PROCESSOR = 272,
     QUIT = 273,
     RESET = 274,
     RUN = 275,
     SET = 276,
     STEP = 277,
     STIMULUS = 278,
     STOPWATCH = 279,
     SYMBOL = 280,
     TRACE = 281,
     gpsim_VERSION = 282,
     X = 283,
     ICD = 284,
     END_OF_COMMAND = 285,
     STRING = 286,
     INDIRECT = 287,
     END_OF_INPUT = 288,
     BIT_FLAG = 289,
     EXPRESSION_OPTION = 290,
     NUMERIC_OPTION = 291,
     STRING_OPTION = 292,
     CMD_SUBTYPE = 293,
     NUMBER = 294,
     FLOAT_NUMBER = 295,
     LITERAL_INT_T = 296,
     LITERAL_BOOL_T = 297,
     LITERAL_FLOAT_T = 298,
     LITERAL_STRING_T = 299,
     COLON_T = 300,
     COMMENT_T = 301,
     EOLN_T = 302,
     PLUS_T = 303
   };
#endif
#define ABORT 258
#define ATTACH 259
#define BREAK 260
#define BUS 261
#define CLEAR 262
#define DISASSEMBLE 263
#define DUMP 264
#define FREQUENCY 265
#define HELP 266
#define LOAD 267
#define LOG 268
#define LIST 269
#define NODE 270
#define MODULE 271
#define PROCESSOR 272
#define QUIT 273
#define RESET 274
#define RUN 275
#define SET 276
#define STEP 277
#define STIMULUS 278
#define STOPWATCH 279
#define SYMBOL 280
#define TRACE 281
#define gpsim_VERSION 282
#define X 283
#define ICD 284
#define END_OF_COMMAND 285
#define STRING 286
#define INDIRECT 287
#define END_OF_INPUT 288
#define BIT_FLAG 289
#define EXPRESSION_OPTION 290
#define NUMERIC_OPTION 291
#define STRING_OPTION 292
#define CMD_SUBTYPE 293
#define NUMBER 294
#define FLOAT_NUMBER 295
#define LITERAL_INT_T 296
#define LITERAL_BOOL_T 297
#define LITERAL_FLOAT_T 298
#define LITERAL_STRING_T 299
#define COLON_T 300
#define COMMENT_T 301
#define EOLN_T 302
#define PLUS_T 303




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 100 "parse.yy"
typedef union YYSTYPE {
  guint32              i;
  guint64             li;
  float                f;
  char                *s;
  cmd_options        *co;
  cmd_options_num   *con;
  cmd_options_float *cof;
  cmd_options_str   *cos;
  cmd_options_expr  *coe;

  BinaryOperator*           BinaryOperator_P;
  Boolean*                  Boolean_P;
  Expression*               Expression_P;
  Float*                    Float_P;
  Integer*                  Integer_P;
  String*                   String_P;

  StringList_t             *StringList_P;
  ExprList_t               *ExprList_P;

} YYSTYPE;
/* Line 1285 of yacc.c.  */
#line 156 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



