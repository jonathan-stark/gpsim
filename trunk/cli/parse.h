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


extern YYSTYPE yylval;
