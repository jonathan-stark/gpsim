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
#define	ABORT	257
#define	ATTACH	258
#define	BREAK	259
#define	BUS	260
#define	CLEAR	261
#define	DISASSEMBLE	262
#define	DUMP	263
#define	FREQUENCY	264
#define	HELP	265
#define	LOAD	266
#define	LIST	267
#define	NODE	268
#define	MODULE	269
#define	PROCESSOR	270
#define	QUIT	271
#define	RESET	272
#define	RUN	273
#define	SET	274
#define	STEP	275
#define	STIMULUS	276
#define	SYMBOL	277
#define	TRACE	278
#define	gpsim_VERSION	279
#define	X	280
#define	END_OF_COMMAND	281
#define	IGNORED	282
#define	SPANNING_LINES	283
#define	STRING	284
#define	INDIRECT	285
#define	END_OF_INPUT	286
#define	BIT_FLAG	287
#define	NUMERIC_OPTION	288
#define	STRING_OPTION	289
#define	NUMBER	290
#define	FLOAT_NUMBER	291


extern YYSTYPE yylval;
