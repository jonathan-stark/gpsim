#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

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
# define	SYMBOL	278
# define	TRACE	279
# define	gpsim_VERSION	280
# define	X	281
# define	ICD	282
# define	END_OF_COMMAND	283
# define	IGNORED	284
# define	SPANNING_LINES	285
# define	STRING	286
# define	INDIRECT	287
# define	END_OF_INPUT	288
# define	BIT_FLAG	289
# define	NUMERIC_OPTION	290
# define	STRING_OPTION	291
# define	NUMBER	292
# define	FLOAT_NUMBER	293


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
