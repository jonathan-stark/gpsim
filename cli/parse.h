#ifndef BISON_PARSE_H
# define BISON_PARSE_H

#ifndef YYSTYPE
typedef union {
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

} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	ABORT	257
# define	ATTACH	258
# define	BREAK	259
# define	BUS	260
# define	CLEAR	261
# define	DISASSEMBLE	262
# define	DUMP	263
# define	FREQUENCY	264
# define	HELP	265
# define	LOAD	266
# define	LOG	267
# define	LIST	268
# define	NODE	269
# define	MODULE	270
# define	PROCESSOR	271
# define	QUIT	272
# define	RESET	273
# define	RUN	274
# define	SET	275
# define	STEP	276
# define	STIMULUS	277
# define	STOPWATCH	278
# define	SYMBOL	279
# define	TRACE	280
# define	gpsim_VERSION	281
# define	X	282
# define	ICD	283
# define	END_OF_COMMAND	284
# define	STRING	285
# define	INDIRECT	286
# define	END_OF_INPUT	287
# define	BIT_FLAG	288
# define	EXPRESSION_OPTION	289
# define	NUMERIC_OPTION	290
# define	STRING_OPTION	291
# define	CMD_SUBTYPE	292
# define	NUMBER	293
# define	FLOAT_NUMBER	294
# define	LITERAL_INT_T	295
# define	LITERAL_BOOL_T	296
# define	LITERAL_FLOAT_T	297
# define	LITERAL_STRING_T	298
# define	COLON_T	299
# define	COMMENT_T	300
# define	EOLN_T	301
# define	PLUS_T	302


extern YYSTYPE yylval;

#endif /* not BISON_PARSE_H */
