
extern @tagged union YYSTYPE <`yy::R>
{
  cnst_t Int_tok;
  char Char_tok;
  string_t<`H> String_tok;
  opt_t<stringptr_t<`H,`H>,`H> Stringopt_tok;
  qvar_t QualId_tok;
  $(bool,string_t<`H>) Asm_tok;
  exp_t Exp_tok;
  stmt_t Stmt_tok;
  $(Position::seg_t,booltype_t, ptrbound_t)@`H YY1;
  ptrbound_t YY2;
  list_t<offsetof_field_t,`H> YY3;
  list_t<exp_t,`H> YY4;
  list_t<$(list_t<designator_t,`H>,exp_t)@`H,`H> YY5;
  primop_t YY6;
  opt_t<primop_t,`H> YY7;
  list_t<switch_clause_t,`H> YY8;
  pat_t YY9;
  $(list_t<pat_t,`H>,bool)@`H YY10;
  list_t<pat_t,`H> YY11;
  $(list_t<designator_t,`H>,pat_t)@`H YY12;
  list_t<$(list_t<designator_t,`H>,pat_t)@`H,`H> YY13;
  $(list_t<$(list_t<designator_t,`H>,pat_t)@`H,`H>,bool)@`H YY14;
  fndecl_t YY15;
  list_t<decl_t,`H> YY16;
  decl_spec_t YY17;
  $(declarator_t<`yy>,exp_opt_t) YY18;
  declarator_list_t<`yy> YY19;
  storage_class_t@`H YY20;
  type_specifier_t YY21;
  aggr_kind_t YY22;
  tqual_t YY23;
  list_t<aggrfield_t,`H> YY24;
  list_t<list_t<aggrfield_t,`H>,`H> YY25;
  list_t<type_modifier_t<`yy>,`yy> YY26;
  declarator_t<`yy> YY27;
  $(declarator_t<`yy>,exp_opt_t,exp_opt_t)@`yy YY28;
  list_t<$(declarator_t<`yy>,exp_opt_t,exp_opt_t)@`yy,`yy> YY29;
  abstractdeclarator_t<`yy> YY30;
  bool YY31;
  scope_t YY32;
  datatypefield_t YY33;
  list_t<datatypefield_t,`H> YY34;
  $(tqual_t,type_specifier_t,attributes_t) YY35;
  list_t<var_t,`H> YY36;
  $(var_opt_t,tqual_t,type_t)@`H YY37;
  list_t<$(var_opt_t,tqual_t,type_t)@`H,`H> YY38;
  $(list_t<$(var_opt_t,tqual_t,type_t)@`H,`H>, bool,vararg_info_t *`H,type_opt_t, list_t<$(type_t,type_t)@`H,`H>)@`H YY39;
  types_t YY40;
  list_t<designator_t,`H> YY41;
  designator_t YY42;
  kind_t YY43;
  type_t YY44;
  list_t<attribute_t,`H> YY45;
  attribute_t YY46;
  enumfield_t YY47;
  list_t<enumfield_t,`H> YY48;
  type_opt_t YY49;
  list_t<$(type_t,type_t)@`H,`H> YY50;
  booltype_t YY51;
  list_t<$(Position::seg_t,qvar_t,bool)@`H,`H> YY52;
  $(list_t<$(Position::seg_t,qvar_t,bool)@`H,`H>, seg_t)@`H YY53;
  list_t<qvar_t,`H> YY54;
  pointer_qual_t<`yy> YY55;
  pointer_quals_t<`yy> YY56;
  exp_opt_t YY57;
  raw_exp_t YY58;
  $(list_t<$(string_t<`H>, exp_t)@`H, `H>, list_t<$(string_t<`H>, exp_t)@`H, `H>, list_t<string_t<`H>@`H, `H>)@`H YY59;
  $(list_t<$(string_t<`H>, exp_t)@`H, `H>, list_t<string_t<`H>@`H, `H>)@`H YY60;
  list_t<string_t<`H>@`H, `H> YY61;
  list_t<$(string_t<`H>, exp_t)@`H, `H> YY62;
  $(string_t<`H>, exp_t)@`H YY63;
  throws_t YY64;
  reentrant_t YY65;
  list_t<type_t,`H> YY66;
  int  YYINITIALSVAL;
};

#ifndef YYLTYPE
extern struct Yyltype {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
};
typedef struct Yyltype yyltype;


#define YYLTYPE yyltype
extern YYLTYPE yynewloc();
extern yyltype yylloc;
#endif

#define	AUTO	258
#define	REGISTER	259
#define	STATIC	260
#define	EXTERN	261
#define	TYPEDEF	262
#define	VOID	263
#define	CHAR	264
#define	SHORT	265
#define	INT	266
#define	LONG	267
#define	FLOAT	268
#define	DOUBLE	269
#define	SIGNED	270
#define	UNSIGNED	271
#define	CONST	272
#define	VOLATILE	273
#define	RESTRICT	274
#define	STRUCT	275
#define	UNION	276
#define	CASE	277
#define	DEFAULT	278
#define	INLINE	279
#define	SIZEOF	280
#define	OFFSETOF	281
#define	IF	282
#define	ELSE	283
#define	SWITCH	284
#define	WHILE	285
#define	DO	286
#define	FOR	287
#define	GOTO	288
#define	CONTINUE	289
#define	BREAK	290
#define	RETURN	291
#define	ENUM	292
#define	TYPEOF	293
#define	BUILTIN_VA_LIST	294
#define	EXTENSION	295
#define	NULL_kw	296
#define	LET	297
#define	THROW	298
#define	TRY	299
#define	CATCH	300
#define	EXPORT	301
#define	OVERRIDE	302
#define	HIDE	303
#define	NEW	304
#define	ABSTRACT	305
#define	FALLTHRU	306
#define	CAP	307
#define	USING	308
#define	NAMESPACE	309
#define	DATATYPE	310
#define	SPAWN	311
#define	MALLOC	312
#define	RMALLOC	313
#define	RMALLOC_INLINE	314
#define	CALLOC	315
#define	RCALLOC	316
#define	SWAP	317
#define	REGION_T	318
#define	TAG_T	319
#define	REGION	320
#define	RNEW	321
#define	REGIONS	322
#define	PORTON	323
#define	PORTOFF	324
#define	PRAGMA	325
#define	TEMPESTON	326
#define	TEMPESTOFF	327
#define	NUMELTS	328
#define	VALUEOF	329
#define	VALUEOF_T	330
#define	TAGCHECK	331
#define	NUMELTS_QUAL	332
#define	THIN_QUAL	333
#define	FAT_QUAL	334
#define	NOTNULL_QUAL	335
#define	NULLABLE_QUAL	336
#define	REQUIRES_QUAL	337
#define	ENSURES_QUAL	338
#define	IEFFECT_QUAL	339
#define	OEFFECT_QUAL	340
#define	THROWS_QUAL	341
#define	NOTHROW_QUAL	342
#define	THROWSANY_QUAL	343
#define	REENTRANT_QUAL	344
#define	XRGN_QUAL	345
#define	REGION_QUAL	346
#define	NOZEROTERM_QUAL	347
#define	ZEROTERM_QUAL	348
#define	TAGGED_QUAL	349
#define	EXTENSIBLE_QUAL	350
#define	PTR_OP	351
#define	INC_OP	352
#define	DEC_OP	353
#define	LEFT_OP	354
#define	RIGHT_OP	355
#define	LE_OP	356
#define	GE_OP	357
#define	EQ_OP	358
#define	NE_OP	359
#define	AND_OP	360
#define	OR_OP	361
#define	MUL_ASSIGN	362
#define	DIV_ASSIGN	363
#define	MOD_ASSIGN	364
#define	ADD_ASSIGN	365
#define	SUB_ASSIGN	366
#define	LEFT_ASSIGN	367
#define	RIGHT_ASSIGN	368
#define	AND_ASSIGN	369
#define	XOR_ASSIGN	370
#define	OR_ASSIGN	371
#define	ELLIPSIS	372
#define	LEFT_RIGHT	373
#define	COLON_COLON	374
#define	IDENTIFIER	375
#define	INTEGER_CONSTANT	376
#define	STRING	377
#define	WSTRING	378
#define	CHARACTER_CONSTANT	379
#define	WCHARACTER_CONSTANT	380
#define	FLOATING_CONSTANT	381
#define	TYPE_VAR	382
#define	TYPEDEF_NAME	383
#define	QUAL_IDENTIFIER	384
#define	QUAL_TYPEDEF_NAME	385
#define	ATTRIBUTE	386
#define	ASM_TOK	387

