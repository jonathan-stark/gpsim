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
#define	HELP	264
#define	LOAD	265
#define	LIST	266
#define	NODE	267
#define	MODULE	268
#define	PROCESSOR	269
#define	QUIT	270
#define	RESET	271
#define	RUN	272
#define	SET	273
#define	STEP	274
#define	STIMULUS	275
#define	SYMBOL	276
#define	TRACE	277
#define	gpsim_VERSION	278
#define	X	279
#define	END_OF_COMMAND	280
#define	IGNORED	281
#define	SPANNING_LINES	282
#define	STRING	283
#define	INDIRECT	284
#define	END_OF_INPUT	285
#define	BIT_FLAG	286
#define	NUMERIC_OPTION	287
#define	STRING_OPTION	288
#define	NUMBER	289
#define	FLOAT_NUMBER	290


extern YYSTYPE yylval;
