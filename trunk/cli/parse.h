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

  BinaryOperator*           BinaryOperator_P;
  Expression*               Expression_P;
  Integer*                  Integer_P;

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
# define	IGNORED	285
# define	SPANNING_LINES	286
# define	STRING	287
# define	INDIRECT	288
# define	END_OF_INPUT	289
# define	BIT_FLAG	290
# define	NUMERIC_OPTION	291
# define	STRING_OPTION	292
# define	CMD_SUBTYPE	293
# define	NUMBER	294
# define	FLOAT_NUMBER	295
# define	LITERAL_INT_T	296
# define	PLUS_T	297


extern YYSTYPE yylval;

#endif /* not BISON_PARSE_H */
