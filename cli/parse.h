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
#define	LOG	267
#define	LIST	268
#define	NODE	269
#define	MODULE	270
#define	PROCESSOR	271
#define	QUIT	272
#define	RESET	273
#define	RUN	274
#define	SET	275
#define	STEP	276
#define	STIMULUS	277
#define	SYMBOL	278
#define	TRACE	279
#define	gpsim_VERSION	280
#define	X	281
#define	END_OF_COMMAND	282
#define	IGNORED	283
#define	SPANNING_LINES	284
#define	STRING	285
#define	INDIRECT	286
#define	END_OF_INPUT	287
#define	BIT_FLAG	288
#define	NUMERIC_OPTION	289
#define	STRING_OPTION	290
#define	NUMBER	291
#define	FLOAT_NUMBER	292


extern YYSTYPE yylval;
