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
#define	IGNORED	278
#define	SPANNING_LINES	279
#define	STRING	280
#define	INDIRECT	281
#define	END_OF_INPUT	282
#define	BIT_FLAG	283
#define	NUMERIC_OPTION	284
#define	STRING_OPTION	285
#define	NUMBER	286
#define	FLOAT_NUMBER	287


extern YYSTYPE yylval;
