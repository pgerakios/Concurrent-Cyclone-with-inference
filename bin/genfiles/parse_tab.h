
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
  list_t<type_t,`H> YY64;
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
#define	USING	307
#define	NAMESPACE	308
#define	DATATYPE	309
#define	SPAWN	310
#define	MALLOC	311
#define	RMALLOC	312
#define	RMALLOC_INLINE	313
#define	CALLOC	314
#define	RCALLOC	315
#define	SWAP	316
#define	REGION_T	317
#define	TAG_T	318
#define	REGION	319
#define	RNEW	320
#define	REGIONS	321
#define	PORTON	322
#define	PORTOFF	323
#define	PRAGMA	324
#define	TEMPESTON	325
#define	TEMPESTOFF	326
#define	NUMELTS	327
#define	VALUEOF	328
#define	VALUEOF_T	329
#define	TAGCHECK	330
#define	NUMELTS_QUAL	331
#define	THIN_QUAL	332
#define	FAT_QUAL	333
#define	NOTNULL_QUAL	334
#define	NULLABLE_QUAL	335
#define	REQUIRES_QUAL	336
#define	ENSURES_QUAL	337
#define	IEFFECT_QUAL	338
#define	OEFFECT_QUAL	339
#define	REGION_QUAL	340
#define	NOZEROTERM_QUAL	341
#define	ZEROTERM_QUAL	342
#define	TAGGED_QUAL	343
#define	EXTENSIBLE_QUAL	344
#define	PTR_OP	345
#define	INC_OP	346
#define	DEC_OP	347
#define	LEFT_OP	348
#define	RIGHT_OP	349
#define	LE_OP	350
#define	GE_OP	351
#define	EQ_OP	352
#define	NE_OP	353
#define	AND_OP	354
#define	OR_OP	355
#define	MUL_ASSIGN	356
#define	DIV_ASSIGN	357
#define	MOD_ASSIGN	358
#define	ADD_ASSIGN	359
#define	SUB_ASSIGN	360
#define	LEFT_ASSIGN	361
#define	RIGHT_ASSIGN	362
#define	AND_ASSIGN	363
#define	XOR_ASSIGN	364
#define	OR_ASSIGN	365
#define	ELLIPSIS	366
#define	LEFT_RIGHT	367
#define	COLON_COLON	368
#define	IDENTIFIER	369
#define	INTEGER_CONSTANT	370
#define	STRING	371
#define	WSTRING	372
#define	CHARACTER_CONSTANT	373
#define	WCHARACTER_CONSTANT	374
#define	FLOATING_CONSTANT	375
#define	TYPE_VAR	376
#define	TYPEDEF_NAME	377
#define	QUAL_IDENTIFIER	378
#define	QUAL_TYPEDEF_NAME	379
#define	ATTRIBUTE	380
#define	ASM_TOK	381

