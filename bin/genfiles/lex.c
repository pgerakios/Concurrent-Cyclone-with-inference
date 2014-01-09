#include <setjmp.h>
/* This is a C header file to be used by the output of the Cyclone to
   C translator.  The corresponding definitions are in file lib/runtime_*.c */
#ifndef _CYC_INCLUDE_H_
#define _CYC_INCLUDE_H_

/* Need one of these per thread (see runtime_stack.c). The runtime maintains 
   a stack that contains either _handler_cons structs or _RegionHandle structs.
   The tag is 0 for a handler_cons and 1 for a region handle.  */
struct _RuntimeStack {
  int tag; 
  struct _RuntimeStack *next;
  void (*cleanup)(struct _RuntimeStack *frame);
};

#ifndef offsetof
/* should be size_t, but int is fine. */
#define offsetof(t,n) ((int)(&(((t *)0)->n)))
#endif

/* Fat pointers */
struct _fat_ptr {
  unsigned char *curr; 
  unsigned char *base; 
  unsigned char *last_plus_one; 
};  

/* Discriminated Unions */
struct _xtunion_struct { char *tag; };

/* Regions */
struct _RegionPage
#ifdef CYC_REGION_PROFILE
{ unsigned total_bytes;
  unsigned free_bytes;
  /* MWH: wish we didn't have to include the stuff below ... */
  struct _RegionPage *next;
  char data[1];
}
#endif
; // abstract -- defined in runtime_memory.c
struct _RegionHandle {
  int is_xrgn; 
  struct _RuntimeStack s;
  struct _RegionPage *curr;
  char               *offset;
  char               *last_plus_one;
  struct _DynRegionHandle *sub_regions;
#ifdef CYC_REGION_PROFILE
  const char         *name;
#else
  unsigned used_bytes;
  unsigned wasted_bytes;
#endif
};
struct _DynRegionFrame {
  struct _RuntimeStack s;
  struct _DynRegionHandle *x;
};
// A dynamic region is just a region handle.  The wrapper struct is for type
// abstraction.
struct Cyc_Core_DynamicRegion {
  struct _RegionHandle h;
};

struct _RegionHandle _new_region(const char*);
void* _region_malloc(struct _RegionHandle*, unsigned);
void* _region_calloc(struct _RegionHandle*, unsigned , unsigned );
void   _free_region(struct _RegionHandle*);
struct _RegionHandle*_open_dynregion(struct _DynRegionFrame*,struct _DynRegionHandle*);
void   _pop_dynregion();

void _push_region(struct _RegionHandle *);
void _pop_region();


/* X-Regions*/
struct _XRegionHandle;
//parent_key can be found through the hashtable
struct _XTreeHandle { // stack allocated.

     struct _XRegionHandle *h; // heap allocated.
     struct _XTreeHandle *next; // next-entry in the hash-table

     struct _XTreeHandle *parent_key; // a linked-list parent 
     struct _XTreeHandle *sub_keys; // a linked-list of subkeys
     struct _XTreeHandle *next_key; // next sibling

	  struct _LocalCaps {
		  unsigned char rcnt; // local ref count 
		  unsigned char lcnt; // local lock count
		  unsigned short lcnt_other; // locks acquired
		 										// by ancestor regions. 
	  } caps;
};

struct _XRegionHandle *
	_new_xregion( struct _XRegionHandle *,
					  unsigned int,
					  struct _XTreeHandle *
			      );

void * _xregion_malloc(struct _XRegionHandle *,unsigned int ); 
void * _xregion_calloc(struct _XRegionHandle *,unsigned int ); 


/* run-time effects */
struct _EffectFrameCaps {
	unsigned char idx; //index -> formal_handles or inner_handles
	unsigned char rg; //region count
	unsigned char lk; //lock count
};

struct _EffectFrame 
{
  struct _RuntimeStack s;
  unsigned char *formal_handles; 
  unsigned char max_formal;
  void **inner_handles;
};

void _push_effect( struct _EffectFrame *,
						 const struct _EffectFrameCaps *,
						 const unsigned int 
					  );
void _spawn_with_effect(const struct _EffectFrameCaps *,
								 const unsigned int, 
						 		 void (*)(void *) ,
						 		 void *,
								 const unsigned char *,
								 const unsigned int,
								 const unsigned int,
								 const unsigned int
					  		  );

void _pop_effect();
void _peek_effect(  void **  );



/* Exceptions */
struct _handler_cons {
  struct _RuntimeStack s;
  jmp_buf handler;
};

void _push_handler(struct _handler_cons *);
void _npop_handler(int);
void _pop_handler();


#ifndef _throw
void* _throw_null_fn(const char*,unsigned);
void* _throw_arraybounds_fn(const char*,unsigned);
void* _throw_badalloc_fn(const char*,unsigned);
void* _throw_match_fn(const char*,unsigned);
void* _throw_fn(void*,const char*,unsigned);
void* _rethrow(void*);
#define _throw_null() (_throw_null_fn(__FILE__,__LINE__))
#define _throw_arraybounds() (_throw_arraybounds_fn(__FILE__,__LINE__))
#define _throw_badalloc() (_throw_badalloc_fn(__FILE__,__LINE__))
#define _throw_match() (_throw_match_fn(__FILE__,__LINE__))
#define _throw(e) (_throw_fn((e),__FILE__,__LINE__))
#endif

struct _xtunion_struct* Cyc_Core_get_exn_thrown();
/* Built-in Exceptions */
struct Cyc_Null_Exception_exn_struct { char *tag; };
struct Cyc_Array_bounds_exn_struct { char *tag; };
struct Cyc_Match_Exception_exn_struct { char *tag; };
struct Cyc_Bad_alloc_exn_struct { char *tag; };
extern char Cyc_Null_Exception[];
extern char Cyc_Array_bounds[];
extern char Cyc_Match_Exception[];
extern char Cyc_Bad_alloc[];

/* Built-in Run-time Checks and company */
#ifdef CYC_ANSI_OUTPUT
#define _INLINE  
#else
#define _INLINE inline
#endif

#ifdef NO_CYC_NULL_CHECKS
#define _check_null(ptr) (ptr)
#else
#define _check_null(ptr) \
  ({ void*_cks_null = (void*)(ptr); \
     if (!_cks_null) _throw_null(); \
     _cks_null; })
#endif

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index)\
   (((char*)ptr) + (elt_sz)*(index))
#ifdef NO_CYC_NULL_CHECKS
#define _check_known_subscript_null _check_known_subscript_notnull
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr);\
  int _index = (index);\
  if (!_cks_ptr) _throw_null(); \
  _cks_ptr + (elt_sz)*_index; })
#endif
#define _zero_arr_plus_fn(orig_x,orig_sz,orig_i,f,l) ((orig_x)+(orig_i))
#define _zero_arr_plus_char_fn _zero_arr_plus_fn
#define _zero_arr_plus_short_fn _zero_arr_plus_fn
#define _zero_arr_plus_int_fn _zero_arr_plus_fn
#define _zero_arr_plus_float_fn _zero_arr_plus_fn
#define _zero_arr_plus_double_fn _zero_arr_plus_fn
#define _zero_arr_plus_longdouble_fn _zero_arr_plus_fn
#define _zero_arr_plus_voidstar_fn _zero_arr_plus_fn
#else
#define _check_known_subscript_null(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (!_cks_ptr) _throw_null(); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })
#define _check_known_subscript_notnull(ptr,bound,elt_sz,index) ({ \
  char*_cks_ptr = (char*)(ptr); \
  unsigned _cks_index = (index); \
  if (_cks_index >= (bound)) _throw_arraybounds(); \
  _cks_ptr + (elt_sz)*_cks_index; })

/* _zero_arr_plus_*_fn(x,sz,i,filename,lineno) adds i to zero-terminated ptr
   x that has at least sz elements */
char* _zero_arr_plus_char_fn(char*,unsigned,int,const char*,unsigned);
short* _zero_arr_plus_short_fn(short*,unsigned,int,const char*,unsigned);
int* _zero_arr_plus_int_fn(int*,unsigned,int,const char*,unsigned);
float* _zero_arr_plus_float_fn(float*,unsigned,int,const char*,unsigned);
double* _zero_arr_plus_double_fn(double*,unsigned,int,const char*,unsigned);
long double* _zero_arr_plus_longdouble_fn(long double*,unsigned,int,const char*, unsigned);
void** _zero_arr_plus_voidstar_fn(void**,unsigned,int,const char*,unsigned);
#endif

/* _get_zero_arr_size_*(x,sz) returns the number of elements in a
   zero-terminated array that is NULL or has at least sz elements */
int _get_zero_arr_size_char(const char*,unsigned);
int _get_zero_arr_size_short(const short*,unsigned);
int _get_zero_arr_size_int(const int*,unsigned);
int _get_zero_arr_size_float(const float*,unsigned);
int _get_zero_arr_size_double(const double*,unsigned);
int _get_zero_arr_size_longdouble(const long double*,unsigned);
int _get_zero_arr_size_voidstar(const void**,unsigned);

/* _zero_arr_inplace_plus_*_fn(x,i,filename,lineno) sets
   zero-terminated pointer *x to *x + i */
char* _zero_arr_inplace_plus_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_short_fn(short**,int,const char*,unsigned);
int* _zero_arr_inplace_plus_int(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_longdouble_fn(long double**,int,const char*,unsigned);
void** _zero_arr_inplace_plus_voidstar_fn(void***,int,const char*,unsigned);
/* like the previous functions, but does post-addition (as in e++) */
char* _zero_arr_inplace_plus_post_char_fn(char**,int,const char*,unsigned);
short* _zero_arr_inplace_plus_post_short_fn(short**x,int,const char*,unsigned);
int* _zero_arr_inplace_plus_post_int_fn(int**,int,const char*,unsigned);
float* _zero_arr_inplace_plus_post_float_fn(float**,int,const char*,unsigned);
double* _zero_arr_inplace_plus_post_double_fn(double**,int,const char*,unsigned);
long double* _zero_arr_inplace_plus_post_longdouble_fn(long double**,int,const char *,unsigned);
void** _zero_arr_inplace_plus_post_voidstar_fn(void***,int,const char*,unsigned);
#define _zero_arr_plus_char(x,s,i) \
  (_zero_arr_plus_char_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_short(x,s,i) \
  (_zero_arr_plus_short_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_int(x,s,i) \
  (_zero_arr_plus_int_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_float(x,s,i) \
  (_zero_arr_plus_float_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_double(x,s,i) \
  (_zero_arr_plus_double_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_longdouble(x,s,i) \
  (_zero_arr_plus_longdouble_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_plus_voidstar(x,s,i) \
  (_zero_arr_plus_voidstar_fn(x,s,i,__FILE__,__LINE__))
#define _zero_arr_inplace_plus_char(x,i) \
  _zero_arr_inplace_plus_char_fn((char **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_short(x,i) \
  _zero_arr_inplace_plus_short_fn((short **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_int(x,i) \
  _zero_arr_inplace_plus_int_fn((int **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_float(x,i) \
  _zero_arr_inplace_plus_float_fn((float **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_double(x,i) \
  _zero_arr_inplace_plus_double_fn((double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_longdouble(x,i) \
  _zero_arr_inplace_plus_longdouble_fn((long double **)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_voidstar(x,i) \
  _zero_arr_inplace_plus_voidstar_fn((void ***)(x),i,__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_char(x,i) \
  _zero_arr_inplace_plus_post_char_fn((char **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_short(x,i) \
  _zero_arr_inplace_plus_post_short_fn((short **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_int(x,i) \
  _zero_arr_inplace_plus_post_int_fn((int **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_float(x,i) \
  _zero_arr_inplace_plus_post_float_fn((float **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_double(x,i) \
  _zero_arr_inplace_plus_post_double_fn((double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_longdouble(x,i) \
  _zero_arr_inplace_plus_post_longdouble_fn((long double **)(x),(i),__FILE__,__LINE__)
#define _zero_arr_inplace_plus_post_voidstar(x,i) \
  _zero_arr_inplace_plus_post_voidstar_fn((void***)(x),(i),__FILE__,__LINE__)

#ifdef NO_CYC_BOUNDS_CHECKS
#define _check_fat_subscript(arr,elt_sz,index) ((arr).curr + (elt_sz) * (index))
#define _untag_fat_ptr(arr,elt_sz,num_elts) ((arr).curr)
#else
#define _check_fat_subscript(arr,elt_sz,index) ({ \
  struct _fat_ptr _cus_arr = (arr); \
  unsigned char *_cus_ans = _cus_arr.curr + (elt_sz) * (index); \
  /* JGM: not needed! if (!_cus_arr.base) _throw_null();*/ \
  if (_cus_ans < _cus_arr.base || _cus_ans >= _cus_arr.last_plus_one) \
    _throw_arraybounds(); \
  _cus_ans; })
#define _untag_fat_ptr(arr,elt_sz,num_elts) ({ \
  struct _fat_ptr _arr = (arr); \
  unsigned char *_curr = _arr.curr; \
  if ((_curr < _arr.base || _curr + (elt_sz) * (num_elts) > _arr.last_plus_one) &&\
      _curr != (unsigned char *)0) \
    _throw_arraybounds(); \
  _curr; })
#endif

#define _tag_fat(tcurr,elt_sz,num_elts) ({ \
  struct _fat_ptr _ans; \
  unsigned _num_elts = (num_elts);\
  _ans.base = _ans.curr = (void*)(tcurr); \
  /* JGM: if we're tagging NULL, ignore num_elts */ \
  _ans.last_plus_one = _ans.base ? (_ans.base + (elt_sz) * _num_elts) : 0; \
  _ans; })

#define _get_fat_size(arr,elt_sz) \
  ({struct _fat_ptr _arr = (arr); \
    unsigned char *_arr_curr=_arr.curr; \
    unsigned char *_arr_last=_arr.last_plus_one; \
    (_arr_curr < _arr.base || _arr_curr >= _arr_last) ? 0 : \
    ((_arr_last - _arr_curr) / (elt_sz));})

#define _fat_ptr_plus(arr,elt_sz,change) ({ \
  struct _fat_ptr _ans = (arr); \
  int _change = (change);\
  _ans.curr += (elt_sz) * _change;\
  _ans; })
#define _fat_ptr_inplace_plus(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  _arr_ptr->curr += (elt_sz) * (change);\
  *_arr_ptr; })
#define _fat_ptr_inplace_plus_post(arr_ptr,elt_sz,change) ({ \
  struct _fat_ptr * _arr_ptr = (arr_ptr); \
  struct _fat_ptr _ans = *_arr_ptr; \
  _arr_ptr->curr += (elt_sz) * (change);\
  _ans; })

//Not a macro since initialization order matters. Defined in runtime_zeroterm.c.
struct _fat_ptr _fat_ptr_decrease_size(struct _fat_ptr,unsigned sz,unsigned numelts);

/* Allocation */
void* GC_malloc(int);
void* GC_malloc_atomic(int);
void* GC_calloc(unsigned,unsigned);
void* GC_calloc_atomic(unsigned,unsigned);
// bound the allocation size to be < MAX_ALLOC_SIZE. See macros below for usage.
#define MAX_MALLOC_SIZE (1 << 28)
void* _bounded_GC_malloc(int,const char*,int);
void* _bounded_GC_malloc_atomic(int,const char*,int);
void* _bounded_GC_calloc(unsigned,unsigned,const char*,int);
void* _bounded_GC_calloc_atomic(unsigned,unsigned,const char*,int);
/* these macros are overridden below ifdef CYC_REGION_PROFILE */
#ifndef CYC_REGION_PROFILE
#define _cycalloc(n) _bounded_GC_malloc(n,__FILE__,__LINE__)
#define _cycalloc_atomic(n) _bounded_GC_malloc_atomic(n,__FILE__,__LINE__)
#define _cyccalloc(n,s) _bounded_GC_calloc(n,s,__FILE__,__LINE__)
#define _cyccalloc_atomic(n,s) _bounded_GC_calloc_atomic(n,s,__FILE__,__LINE__)
#endif

static _INLINE unsigned int _check_times(unsigned x, unsigned y) {
  unsigned long long whole_ans = 
    ((unsigned long long) x)*((unsigned long long)y);
  unsigned word_ans = (unsigned)whole_ans;
  if(word_ans < whole_ans || word_ans > MAX_MALLOC_SIZE)
    _throw_badalloc();
  return word_ans;
}

#define _CYC_MAX_REGION_CONST 2
#define _CYC_MIN_ALIGNMENT (sizeof(double))

#ifdef CYC_REGION_PROFILE
extern int rgn_total_bytes;
#endif

static _INLINE void *_fast_region_malloc(struct _RegionHandle *r, unsigned orig_s) {  
  if (r > (struct _RegionHandle *)_CYC_MAX_REGION_CONST && r->curr != 0) { 
#ifdef CYC_NOALIGN
    unsigned s =  orig_s;
#else
    unsigned s =  (orig_s + _CYC_MIN_ALIGNMENT - 1) & (~(_CYC_MIN_ALIGNMENT -1)); 
#endif
    char *result; 
    result = r->offset; 
    if (s <= (r->last_plus_one - result)) {
      r->offset = result + s; 
#ifdef CYC_REGION_PROFILE
    r->curr->free_bytes = r->curr->free_bytes - s;
    rgn_total_bytes += s;
#endif
      return result;
    }
  } 
  return _region_malloc(r,orig_s); 
}

#ifdef CYC_REGION_PROFILE
/* see macros below for usage. defined in runtime_memory.c */
void* _profile_GC_malloc(int,const char*,const char*,int);
void* _profile_GC_malloc_atomic(int,const char*,const char*,int);
void* _profile_GC_calloc(unsigned,unsigned,const char*,const char*,int);
void* _profile_GC_calloc_atomic(unsigned,unsigned,const char*,const char*,int);
void* _profile_region_malloc(struct _RegionHandle*,unsigned,const char*,const char*,int);
void* _profile_region_calloc(struct _RegionHandle*,unsigned,unsigned,const char *,const char*,int);
struct _RegionHandle _profile_new_region(const char*,const char*,const char*,int);
void _profile_free_region(struct _RegionHandle*,const char*,const char*,int);
#ifndef RUNTIME_CYC
#define _new_region(n) _profile_new_region(n,__FILE__,__FUNCTION__,__LINE__)
#define _free_region(r) _profile_free_region(r,__FILE__,__FUNCTION__,__LINE__)
#define _region_malloc(rh,n) _profile_region_malloc(rh,n,__FILE__,__FUNCTION__,__LINE__)
#define _region_calloc(rh,n,t) _profile_region_calloc(rh,n,t,__FILE__,__FUNCTION__,__LINE__)
#  endif
#define _cycalloc(n) _profile_GC_malloc(n,__FILE__,__FUNCTION__,__LINE__)
#define _cycalloc_atomic(n) _profile_GC_malloc_atomic(n,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc(n,s) _profile_GC_calloc(n,s,__FILE__,__FUNCTION__,__LINE__)
#define _cyccalloc_atomic(n,s) _profile_GC_calloc_atomic(n,s,__FILE__,__FUNCTION__,__LINE__)
#endif
#endif
 struct Cyc_Core_Opt{void*v;};
# 95 "core.h"
struct _fat_ptr Cyc_Core_new_string(unsigned);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;
# 175
void Cyc_Core_ufree(void*ptr);struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};
# 231
struct Cyc_Core_NewDynamicRegion Cyc_Core__new_ukey(const char*file,const char*func,int lineno);
# 251 "core.h"
void Cyc_Core_free_ukey(struct Cyc_Core_DynamicRegion*k);
# 261
void*Cyc_Core_open_region(struct Cyc_Core_DynamicRegion*key,void*arg,void*(*body)(struct _RegionHandle*h,void*arg));struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73 "cycboot.h"
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 197 "cycboot.h"
int Cyc_sscanf(struct _fat_ptr,struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Lexing_Error[6U];struct Cyc_Lexing_Error_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Lexing_lexbuf{void(*refill_buff)(struct Cyc_Lexing_lexbuf*);void*refill_state;struct _fat_ptr lex_buffer;int lex_buffer_len;int lex_abs_pos;int lex_start_pos;int lex_curr_pos;int lex_last_pos;int lex_last_action;int lex_eof_reached;};struct Cyc_Lexing_function_lexbuf_state{int(*read_fun)(struct _fat_ptr,int,void*);void*read_fun_state;};struct Cyc_Lexing_lex_tables{struct _fat_ptr lex_base;struct _fat_ptr lex_backtrk;struct _fat_ptr lex_default;struct _fat_ptr lex_trans;struct _fat_ptr lex_check;};
# 82 "lexing.h"
struct _fat_ptr Cyc_Lexing_lexeme(struct Cyc_Lexing_lexbuf*);
char Cyc_Lexing_lexeme_char(struct Cyc_Lexing_lexbuf*,int);
int Cyc_Lexing_lexeme_start(struct Cyc_Lexing_lexbuf*);
int Cyc_Lexing_lexeme_end(struct Cyc_Lexing_lexbuf*);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178 "list.h"
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Set_Set;
# 51 "set.h"
struct Cyc_Set_Set*Cyc_Set_empty(int(*cmp)(void*,void*));
# 63
struct Cyc_Set_Set*Cyc_Set_insert(struct Cyc_Set_Set*s,void*elt);
# 100
int Cyc_Set_member(struct Cyc_Set_Set*s,void*elt);
# 127
void Cyc_Set_iter(void(*f)(void*),struct Cyc_Set_Set*s);extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
# 54
int Cyc_zstrptrcmp(struct _fat_ptr*,struct _fat_ptr*);
# 73
struct _fat_ptr Cyc_zstrncpy(struct _fat_ptr,struct _fat_ptr,unsigned long);
# 109 "string.h"
struct _fat_ptr Cyc_substring(struct _fat_ptr,int ofs,unsigned long n);struct Cyc_Xarray_Xarray{struct _fat_ptr elmts;int num_elmts;};
# 42 "xarray.h"
void*Cyc_Xarray_get(struct Cyc_Xarray_Xarray*,int);
# 51
struct Cyc_Xarray_Xarray*Cyc_Xarray_rcreate(struct _RegionHandle*,int,void*);
# 66
void Cyc_Xarray_add(struct Cyc_Xarray_Xarray*,void*);
# 69
int Cyc_Xarray_add_ind(struct Cyc_Xarray_Xarray*,void*);struct Cyc_Position_Error;struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 110 "dict.h"
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 111 "absyn.h"
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);
# 113
union Cyc_Absyn_Nmspace Cyc_Absyn_Abs_n(struct Cyc_List_List*ns,int C_scope);struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
# 184
enum Cyc_Absyn_DefExn{Cyc_Absyn_De_NullCheck =0U,Cyc_Absyn_De_BoundsCheck =1U,Cyc_Absyn_De_PatternCheck =2U,Cyc_Absyn_De_AllocCheck =3U};
# 194
enum Cyc_Absyn_Scope{Cyc_Absyn_Static =0U,Cyc_Absyn_Abstract =1U,Cyc_Absyn_Public =2U,Cyc_Absyn_Extern =3U,Cyc_Absyn_ExternC =4U,Cyc_Absyn_Register =5U};struct Cyc_Absyn_Tqual{int print_const: 1;int q_volatile: 1;int q_restrict: 1;int real_const: 1;unsigned loc;};
# 215
enum Cyc_Absyn_Size_of{Cyc_Absyn_Char_sz =0U,Cyc_Absyn_Short_sz =1U,Cyc_Absyn_Int_sz =2U,Cyc_Absyn_Long_sz =3U,Cyc_Absyn_LongLong_sz =4U};
enum Cyc_Absyn_Sign{Cyc_Absyn_Signed =0U,Cyc_Absyn_Unsigned =1U,Cyc_Absyn_None =2U};
enum Cyc_Absyn_AggrKind{Cyc_Absyn_StructA =0U,Cyc_Absyn_UnionA =1U};
# 220
enum Cyc_Absyn_AliasQual{Cyc_Absyn_Aliasable =0U,Cyc_Absyn_Unique =1U,Cyc_Absyn_Top =2U};
# 225
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};extern char Cyc_Parse_Exit[5U];struct Cyc_Parse_Exit_exn_struct{char*tag;};struct Cyc_FlatList{struct Cyc_FlatList*tl;void*hd[0U] __attribute__((aligned )) ;};struct Cyc_Type_specifier{int Signed_spec: 1;int Unsigned_spec: 1;int Short_spec: 1;int Long_spec: 1;int Long_Long_spec: 1;int Valid_type_spec: 1;void*Type_spec;unsigned loc;};struct Cyc_Declarator{struct _tuple0*id;struct Cyc_List_List*tms;};struct _tuple13{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;};struct _tuple12{struct _tuple12*tl;struct _tuple13 hd  __attribute__((aligned )) ;};
# 52 "parse.h"
enum Cyc_Storage_class{Cyc_Typedef_sc =0U,Cyc_Extern_sc =1U,Cyc_ExternC_sc =2U,Cyc_Static_sc =3U,Cyc_Auto_sc =4U,Cyc_Register_sc =5U,Cyc_Abstract_sc =6U};struct Cyc_Declaration_spec{enum Cyc_Storage_class*sc;struct Cyc_Absyn_Tqual tq;struct Cyc_Type_specifier type_specs;int is_inline;struct Cyc_List_List*attributes;};struct Cyc_Abstractdeclarator{struct Cyc_List_List*tms;};struct Cyc_Numelts_ptrqual_Pointer_qual_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Region_ptrqual_Pointer_qual_struct{int tag;void*f1;};struct Cyc_Thin_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Fat_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Zeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nozeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Notnull_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nullable_ptrqual_Pointer_qual_struct{int tag;};struct _union_YYSTYPE_Int_tok{int tag;union Cyc_Absyn_Cnst val;};struct _union_YYSTYPE_Char_tok{int tag;char val;};struct _union_YYSTYPE_String_tok{int tag;struct _fat_ptr val;};struct _union_YYSTYPE_Stringopt_tok{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_QualId_tok{int tag;struct _tuple0*val;};struct _tuple14{int f1;struct _fat_ptr f2;};struct _union_YYSTYPE_Asm_tok{int tag;struct _tuple14 val;};struct _union_YYSTYPE_Exp_tok{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_Stmt_tok{int tag;struct Cyc_Absyn_Stmt*val;};struct _tuple15{unsigned f1;void*f2;void*f3;};struct _union_YYSTYPE_YY1{int tag;struct _tuple15*val;};struct _union_YYSTYPE_YY2{int tag;void*val;};struct _union_YYSTYPE_YY3{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY4{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY5{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY6{int tag;enum Cyc_Absyn_Primop val;};struct _union_YYSTYPE_YY7{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_YY8{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY9{int tag;struct Cyc_Absyn_Pat*val;};struct _tuple16{struct Cyc_List_List*f1;int f2;};struct _union_YYSTYPE_YY10{int tag;struct _tuple16*val;};struct _union_YYSTYPE_YY11{int tag;struct Cyc_List_List*val;};struct _tuple17{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};struct _union_YYSTYPE_YY12{int tag;struct _tuple17*val;};struct _union_YYSTYPE_YY13{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY14{int tag;struct _tuple16*val;};struct _union_YYSTYPE_YY15{int tag;struct Cyc_Absyn_Fndecl*val;};struct _union_YYSTYPE_YY16{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY17{int tag;struct Cyc_Declaration_spec val;};struct _union_YYSTYPE_YY18{int tag;struct _tuple13 val;};struct _union_YYSTYPE_YY19{int tag;struct _tuple12*val;};struct _union_YYSTYPE_YY20{int tag;enum Cyc_Storage_class*val;};struct _union_YYSTYPE_YY21{int tag;struct Cyc_Type_specifier val;};struct _union_YYSTYPE_YY22{int tag;enum Cyc_Absyn_AggrKind val;};struct _union_YYSTYPE_YY23{int tag;struct Cyc_Absyn_Tqual val;};struct _union_YYSTYPE_YY24{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY25{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY26{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY27{int tag;struct Cyc_Declarator val;};struct _tuple18{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct _union_YYSTYPE_YY28{int tag;struct _tuple18*val;};struct _union_YYSTYPE_YY29{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY30{int tag;struct Cyc_Abstractdeclarator val;};struct _union_YYSTYPE_YY31{int tag;int val;};struct _union_YYSTYPE_YY32{int tag;enum Cyc_Absyn_Scope val;};struct _union_YYSTYPE_YY33{int tag;struct Cyc_Absyn_Datatypefield*val;};struct _union_YYSTYPE_YY34{int tag;struct Cyc_List_List*val;};struct _tuple19{struct Cyc_Absyn_Tqual f1;struct Cyc_Type_specifier f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY35{int tag;struct _tuple19 val;};struct _union_YYSTYPE_YY36{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY37{int tag;struct _tuple9*val;};struct _union_YYSTYPE_YY38{int tag;struct Cyc_List_List*val;};struct _tuple20{struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;};struct _union_YYSTYPE_YY39{int tag;struct _tuple20*val;};struct _union_YYSTYPE_YY40{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY41{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY42{int tag;void*val;};struct _union_YYSTYPE_YY43{int tag;struct Cyc_Absyn_Kind*val;};struct _union_YYSTYPE_YY44{int tag;void*val;};struct _union_YYSTYPE_YY45{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY46{int tag;void*val;};struct _union_YYSTYPE_YY47{int tag;struct Cyc_Absyn_Enumfield*val;};struct _union_YYSTYPE_YY48{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY49{int tag;void*val;};struct _union_YYSTYPE_YY50{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY51{int tag;void*val;};struct _union_YYSTYPE_YY52{int tag;struct Cyc_List_List*val;};struct _tuple21{struct Cyc_List_List*f1;unsigned f2;};struct _union_YYSTYPE_YY53{int tag;struct _tuple21*val;};struct _union_YYSTYPE_YY54{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY55{int tag;void*val;};struct _union_YYSTYPE_YY56{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY57{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_YY58{int tag;void*val;};struct _tuple22{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY59{int tag;struct _tuple22*val;};struct _union_YYSTYPE_YY60{int tag;struct _tuple8*val;};struct _union_YYSTYPE_YY61{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY62{int tag;struct Cyc_List_List*val;};struct _tuple23{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};struct _union_YYSTYPE_YY63{int tag;struct _tuple23*val;};struct _union_YYSTYPE_YY64{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY65{int tag;int val;};struct _union_YYSTYPE_YY66{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YYINITIALSVAL{int tag;int val;};union Cyc_YYSTYPE{struct _union_YYSTYPE_Int_tok Int_tok;struct _union_YYSTYPE_Char_tok Char_tok;struct _union_YYSTYPE_String_tok String_tok;struct _union_YYSTYPE_Stringopt_tok Stringopt_tok;struct _union_YYSTYPE_QualId_tok QualId_tok;struct _union_YYSTYPE_Asm_tok Asm_tok;struct _union_YYSTYPE_Exp_tok Exp_tok;struct _union_YYSTYPE_Stmt_tok Stmt_tok;struct _union_YYSTYPE_YY1 YY1;struct _union_YYSTYPE_YY2 YY2;struct _union_YYSTYPE_YY3 YY3;struct _union_YYSTYPE_YY4 YY4;struct _union_YYSTYPE_YY5 YY5;struct _union_YYSTYPE_YY6 YY6;struct _union_YYSTYPE_YY7 YY7;struct _union_YYSTYPE_YY8 YY8;struct _union_YYSTYPE_YY9 YY9;struct _union_YYSTYPE_YY10 YY10;struct _union_YYSTYPE_YY11 YY11;struct _union_YYSTYPE_YY12 YY12;struct _union_YYSTYPE_YY13 YY13;struct _union_YYSTYPE_YY14 YY14;struct _union_YYSTYPE_YY15 YY15;struct _union_YYSTYPE_YY16 YY16;struct _union_YYSTYPE_YY17 YY17;struct _union_YYSTYPE_YY18 YY18;struct _union_YYSTYPE_YY19 YY19;struct _union_YYSTYPE_YY20 YY20;struct _union_YYSTYPE_YY21 YY21;struct _union_YYSTYPE_YY22 YY22;struct _union_YYSTYPE_YY23 YY23;struct _union_YYSTYPE_YY24 YY24;struct _union_YYSTYPE_YY25 YY25;struct _union_YYSTYPE_YY26 YY26;struct _union_YYSTYPE_YY27 YY27;struct _union_YYSTYPE_YY28 YY28;struct _union_YYSTYPE_YY29 YY29;struct _union_YYSTYPE_YY30 YY30;struct _union_YYSTYPE_YY31 YY31;struct _union_YYSTYPE_YY32 YY32;struct _union_YYSTYPE_YY33 YY33;struct _union_YYSTYPE_YY34 YY34;struct _union_YYSTYPE_YY35 YY35;struct _union_YYSTYPE_YY36 YY36;struct _union_YYSTYPE_YY37 YY37;struct _union_YYSTYPE_YY38 YY38;struct _union_YYSTYPE_YY39 YY39;struct _union_YYSTYPE_YY40 YY40;struct _union_YYSTYPE_YY41 YY41;struct _union_YYSTYPE_YY42 YY42;struct _union_YYSTYPE_YY43 YY43;struct _union_YYSTYPE_YY44 YY44;struct _union_YYSTYPE_YY45 YY45;struct _union_YYSTYPE_YY46 YY46;struct _union_YYSTYPE_YY47 YY47;struct _union_YYSTYPE_YY48 YY48;struct _union_YYSTYPE_YY49 YY49;struct _union_YYSTYPE_YY50 YY50;struct _union_YYSTYPE_YY51 YY51;struct _union_YYSTYPE_YY52 YY52;struct _union_YYSTYPE_YY53 YY53;struct _union_YYSTYPE_YY54 YY54;struct _union_YYSTYPE_YY55 YY55;struct _union_YYSTYPE_YY56 YY56;struct _union_YYSTYPE_YY57 YY57;struct _union_YYSTYPE_YY58 YY58;struct _union_YYSTYPE_YY59 YY59;struct _union_YYSTYPE_YY60 YY60;struct _union_YYSTYPE_YY61 YY61;struct _union_YYSTYPE_YY62 YY62;struct _union_YYSTYPE_YY63 YY63;struct _union_YYSTYPE_YY64 YY64;struct _union_YYSTYPE_YY65 YY65;struct _union_YYSTYPE_YY66 YY66;struct _union_YYSTYPE_YYINITIALSVAL YYINITIALSVAL;};struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int last_line;int last_column;};
# 94 "parse_tab.h"
extern struct Cyc_Yyltype Cyc_yylloc;struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};struct Cyc_Binding_Namespace_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_Using_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_NSCtxt{struct Cyc_List_List*curr_ns;struct Cyc_List_List*availables;struct Cyc_Dict_Dict ns_data;};
# 46 "binding.h"
struct Cyc_Binding_NSCtxt*Cyc_Binding_mt_nsctxt(void*,void*(*mkdata)(void*));
void Cyc_Binding_enter_ns(struct Cyc_Binding_NSCtxt*,struct _fat_ptr*,void*,void*(*mkdata)(void*));
struct Cyc_List_List*Cyc_Binding_enter_using(unsigned loc,struct Cyc_Binding_NSCtxt*ctxt,struct _tuple0*usename);
void Cyc_Binding_leave_ns(struct Cyc_Binding_NSCtxt*ctxt);
void Cyc_Binding_leave_using(struct Cyc_Binding_NSCtxt*ctxt);
struct Cyc_List_List*Cyc_Binding_resolve_rel_ns(unsigned,struct Cyc_Binding_NSCtxt*,struct Cyc_List_List*);
# 35 "warn.h"
void Cyc_Warn_err(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 40
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct Cyc_Lex_Trie;struct _union_TrieChildren_Zero{int tag;int val;};struct _tuple24{int f1;struct Cyc_Lex_Trie*f2;};struct _union_TrieChildren_One{int tag;struct _tuple24 val;};struct _union_TrieChildren_Many{int tag;struct Cyc_Lex_Trie**val;};union Cyc_Lex_TrieChildren{struct _union_TrieChildren_Zero Zero;struct _union_TrieChildren_One One;struct _union_TrieChildren_Many Many;};
# 50 "lex.cyl"
union Cyc_Lex_TrieChildren Cyc_Lex_Zero(){return({union Cyc_Lex_TrieChildren _tmp18A;(_tmp18A.Zero).tag=1U,(_tmp18A.Zero).val=0;_tmp18A;});}union Cyc_Lex_TrieChildren Cyc_Lex_One(int i,struct Cyc_Lex_Trie*t){
# 52
return({union Cyc_Lex_TrieChildren _tmp18B;(_tmp18B.One).tag=2U,((_tmp18B.One).val).f1=i,((_tmp18B.One).val).f2=t;_tmp18B;});}
# 50
union Cyc_Lex_TrieChildren Cyc_Lex_Many(struct Cyc_Lex_Trie**ts){
# 55
return({union Cyc_Lex_TrieChildren _tmp18C;(_tmp18C.Many).tag=3U,(_tmp18C.Many).val=ts;_tmp18C;});}struct Cyc_Lex_Trie{union Cyc_Lex_TrieChildren children;int shared_str;};struct Cyc_Lex_DynTrie{struct Cyc_Core_DynamicRegion*dyn;struct Cyc_Lex_Trie*t;};struct Cyc_Lex_DynTrieSymbols{struct Cyc_Core_DynamicRegion*dyn;struct Cyc_Lex_Trie*t;struct Cyc_Xarray_Xarray*symbols;};
# 76
static int Cyc_Lex_num_kws=0;struct Cyc_Lex_KeyWordInfo{int token_index;int is_c_keyword;};
# 78
static struct _fat_ptr Cyc_Lex_kw_nums={(void*)0,(void*)0,(void*)(0 + 0)};
# 80
int Cyc_Lex_compile_for_boot_r=0;
int Cyc_Lex_expand_specials=0;
# 83
static int Cyc_Lex_in_extern_c=0;
static struct Cyc_List_List*Cyc_Lex_prev_extern_c=0;
void Cyc_Lex_enter_extern_c(){
struct Cyc_List_List*x=0;
({struct Cyc_List_List*_tmp3=x;struct Cyc_List_List*_tmp4=Cyc_Lex_prev_extern_c;x=_tmp4;Cyc_Lex_prev_extern_c=_tmp3;});
Cyc_Lex_prev_extern_c=({struct Cyc_List_List*_tmp5=_region_malloc(Cyc_Core_unique_region,sizeof(*_tmp5));_tmp5->hd=(void*)Cyc_Lex_in_extern_c,_tmp5->tl=x;_tmp5;});
Cyc_Lex_in_extern_c=1;}
# 91
void Cyc_Lex_leave_extern_c(){
struct Cyc_List_List*x=0;
({struct Cyc_List_List*_tmp7=x;struct Cyc_List_List*_tmp8=Cyc_Lex_prev_extern_c;x=_tmp8;Cyc_Lex_prev_extern_c=_tmp7;});
Cyc_Lex_in_extern_c=(int)((struct Cyc_List_List*)_check_null(x))->hd;
Cyc_Lex_prev_extern_c=x->tl;
({({void(*_tmp1A9)(struct Cyc_List_List*ptr)=({void(*_tmp9)(struct Cyc_List_List*ptr)=(void(*)(struct Cyc_List_List*ptr))Cyc_Core_ufree;_tmp9;});_tmp1A9(x);});});}
# 91
static struct Cyc_Lex_DynTrieSymbols*Cyc_Lex_ids_trie=0;
# 100
static struct Cyc_Lex_DynTrie*Cyc_Lex_typedefs_trie=0;
# 102
static int Cyc_Lex_comment_depth=0;
# 105
static union Cyc_Absyn_Cnst Cyc_Lex_token_int={.Int_c={5,{Cyc_Absyn_Signed,0}}};static char _tmpB[8U]="*bogus*";
static struct _fat_ptr Cyc_Lex_bogus_string={_tmpB,_tmpB,_tmpB + 8U};
static struct _tuple0 Cyc_Lex_token_id_pair={{.Loc_n={4,0}},& Cyc_Lex_bogus_string};
# 110
static char Cyc_Lex_token_char='\000';static char _tmpC[1U]="";
struct _fat_ptr Cyc_Lex_token_string={_tmpC,_tmpC,_tmpC + 1U};
struct _tuple0*Cyc_Lex_token_qvar=& Cyc_Lex_token_id_pair;
# 115
static int Cyc_Lex_runaway_start=0;
static int Cyc_Lex_paren_depth=0;
# 118
static void Cyc_Lex_err(struct _fat_ptr msg,struct Cyc_Lexing_lexbuf*lb){
unsigned s=(unsigned)({Cyc_Lexing_lexeme_start(lb);});
({void*_tmpD=0U;({unsigned _tmp1AB=s;struct _fat_ptr _tmp1AA=msg;Cyc_Warn_err(_tmp1AB,_tmp1AA,_tag_fat(_tmpD,sizeof(void*),0U));});});}
# 122
static void Cyc_Lex_runaway_err(struct _fat_ptr msg,struct Cyc_Lexing_lexbuf*lb){
unsigned s=(unsigned)Cyc_Lex_runaway_start;
({void*_tmpF=0U;({unsigned _tmp1AD=s;struct _fat_ptr _tmp1AC=msg;Cyc_Warn_err(_tmp1AD,_tmp1AC,_tag_fat(_tmpF,sizeof(void*),0U));});});}
# 126
static char Cyc_Lex_cnst2char(union Cyc_Absyn_Cnst cnst,struct Cyc_Lexing_lexbuf*lb){
union Cyc_Absyn_Cnst _tmp11=cnst;char _tmp12;long long _tmp13;int _tmp14;switch((_tmp11.Char_c).tag){case 5U: _LL1: _tmp14=((_tmp11.Int_c).val).f2;_LL2: {int i=_tmp14;
return(char)i;}case 6U: _LL3: _tmp13=((_tmp11.LongLong_c).val).f2;_LL4: {long long i=_tmp13;
return(char)i;}case 2U: _LL5: _tmp12=((_tmp11.Char_c).val).f2;_LL6: {char c=_tmp12;
return c;}default: _LL7: _LL8:
({({struct _fat_ptr _tmp1AE=({const char*_tmp15="couldn't convert constant to char!";_tag_fat(_tmp15,sizeof(char),35U);});Cyc_Lex_err(_tmp1AE,lb);});});return'\000';}_LL0:;}static char _tmp17[14U]="__attribute__";static char _tmp18[12U]="__attribute";static char _tmp19[9U]="abstract";static char _tmp1A[5U]="auto";static char _tmp1B[6U]="break";static char _tmp1C[18U]="__builtin_va_list";static char _tmp1D[7U]="calloc";static char _tmp1E[5U]="case";static char _tmp1F[6U]="catch";static char _tmp20[5U]="char";static char _tmp21[6U]="const";static char _tmp22[10U]="__const__";static char _tmp23[9U]="continue";static char _tmp24[17U]="cyclone_override";static char _tmp25[9U]="datatype";static char _tmp26[8U]="default";static char _tmp27[3U]="do";static char _tmp28[7U]="double";static char _tmp29[5U]="else";static char _tmp2A[5U]="enum";static char _tmp2B[7U]="export";static char _tmp2C[5U]="hide";static char _tmp2D[14U]="__extension__";static char _tmp2E[7U]="extern";static char _tmp2F[9U]="fallthru";static char _tmp30[6U]="float";static char _tmp31[4U]="for";static char _tmp32[5U]="goto";static char _tmp33[3U]="if";static char _tmp34[7U]="inline";static char _tmp35[11U]="__inline__";static char _tmp36[9U]="__inline";static char _tmp37[4U]="int";static char _tmp38[4U]="let";static char _tmp39[5U]="long";static char _tmp3A[7U]="malloc";static char _tmp3B[10U]="namespace";static char _tmp3C[4U]="new";static char _tmp3D[5U]="NULL";static char _tmp3E[8U]="numelts";static char _tmp3F[9U]="offsetof";static char _tmp40[20U]="__cyclone_port_on__";static char _tmp41[21U]="__cyclone_port_off__";static char _tmp42[19U]="__cyclone_pragma__";static char _tmp43[8U]="rcalloc";static char _tmp44[9U]="region_t";static char _tmp45[7U]="region";static char _tmp46[8U]="regions";static char _tmp47[9U]="register";static char _tmp48[9U]="restrict";static char _tmp49[7U]="return";static char _tmp4A[8U]="rmalloc";static char _tmp4B[15U]="rmalloc_inline";static char _tmp4C[5U]="rnew";static char _tmp4D[6U]="short";static char _tmp4E[7U]="signed";static char _tmp4F[11U]="__signed__";static char _tmp50[7U]="sizeof";static char _tmp51[7U]="static";static char _tmp52[7U]="struct";static char _tmp53[7U]="switch";static char _tmp54[9U]="tagcheck";static char _tmp55[6U]="tag_t";static char _tmp56[15U]="__tempest_on__";static char _tmp57[16U]="__tempest_off__";static char _tmp58[6U]="throw";static char _tmp59[4U]="try";static char _tmp5A[8U]="typedef";static char _tmp5B[7U]="typeof";static char _tmp5C[11U]="__typeof__";static char _tmp5D[6U]="union";static char _tmp5E[9U]="unsigned";static char _tmp5F[13U]="__unsigned__";static char _tmp60[6U]="using";static char _tmp61[8U]="valueof";static char _tmp62[10U]="valueof_t";static char _tmp63[5U]="void";static char _tmp64[9U]="volatile";static char _tmp65[13U]="__volatile__";static char _tmp66[4U]="asm";static char _tmp67[8U]="__asm__";static char _tmp68[6U]="while";static char _tmp69[6U]="spawn";struct _tuple25{struct _fat_ptr f1;short f2;int f3;};
# 126
static struct _tuple25 Cyc_Lex_rw_array[83U]={{{_tmp17,_tmp17,_tmp17 + 14U},384,1},{{_tmp18,_tmp18,_tmp18 + 12U},384,1},{{_tmp19,_tmp19,_tmp19 + 9U},305,0},{{_tmp1A,_tmp1A,_tmp1A + 5U},258,1},{{_tmp1B,_tmp1B,_tmp1B + 6U},290,1},{{_tmp1C,_tmp1C,_tmp1C + 18U},294,1},{{_tmp1D,_tmp1D,_tmp1D + 7U},314,0},{{_tmp1E,_tmp1E,_tmp1E + 5U},277,1},{{_tmp1F,_tmp1F,_tmp1F + 6U},300,1},{{_tmp20,_tmp20,_tmp20 + 5U},264,1},{{_tmp21,_tmp21,_tmp21 + 6U},272,1},{{_tmp22,_tmp22,_tmp22 + 10U},272,1},{{_tmp23,_tmp23,_tmp23 + 9U},289,1},{{_tmp24,_tmp24,_tmp24 + 17U},302,0},{{_tmp25,_tmp25,_tmp25 + 9U},309,0},{{_tmp26,_tmp26,_tmp26 + 8U},278,1},{{_tmp27,_tmp27,_tmp27 + 3U},286,1},{{_tmp28,_tmp28,_tmp28 + 7U},269,1},{{_tmp29,_tmp29,_tmp29 + 5U},283,1},{{_tmp2A,_tmp2A,_tmp2A + 5U},292,1},{{_tmp2B,_tmp2B,_tmp2B + 7U},301,0},{{_tmp2C,_tmp2C,_tmp2C + 5U},303,0},{{_tmp2D,_tmp2D,_tmp2D + 14U},295,1},{{_tmp2E,_tmp2E,_tmp2E + 7U},261,1},{{_tmp2F,_tmp2F,_tmp2F + 9U},306,0},{{_tmp30,_tmp30,_tmp30 + 6U},268,1},{{_tmp31,_tmp31,_tmp31 + 4U},287,1},{{_tmp32,_tmp32,_tmp32 + 5U},288,1},{{_tmp33,_tmp33,_tmp33 + 3U},282,1},{{_tmp34,_tmp34,_tmp34 + 7U},279,1},{{_tmp35,_tmp35,_tmp35 + 11U},279,1},{{_tmp36,_tmp36,_tmp36 + 9U},279,1},{{_tmp37,_tmp37,_tmp37 + 4U},266,1},{{_tmp38,_tmp38,_tmp38 + 4U},297,0},{{_tmp39,_tmp39,_tmp39 + 5U},267,1},{{_tmp3A,_tmp3A,_tmp3A + 7U},311,0},{{_tmp3B,_tmp3B,_tmp3B + 10U},308,0},{{_tmp3C,_tmp3C,_tmp3C + 4U},304,0},{{_tmp3D,_tmp3D,_tmp3D + 5U},296,0},{{_tmp3E,_tmp3E,_tmp3E + 8U},327,0},{{_tmp3F,_tmp3F,_tmp3F + 9U},281,1},{{_tmp40,_tmp40,_tmp40 + 20U},322,0},{{_tmp41,_tmp41,_tmp41 + 21U},323,0},{{_tmp42,_tmp42,_tmp42 + 19U},324,0},{{_tmp43,_tmp43,_tmp43 + 8U},315,0},{{_tmp44,_tmp44,_tmp44 + 9U},317,0},{{_tmp45,_tmp45,_tmp45 + 7U},319,0},{{_tmp46,_tmp46,_tmp46 + 8U},321,0},{{_tmp47,_tmp47,_tmp47 + 9U},259,1},{{_tmp48,_tmp48,_tmp48 + 9U},274,1},{{_tmp49,_tmp49,_tmp49 + 7U},291,1},{{_tmp4A,_tmp4A,_tmp4A + 8U},312,0},{{_tmp4B,_tmp4B,_tmp4B + 15U},313,0},{{_tmp4C,_tmp4C,_tmp4C + 5U},320,0},{{_tmp4D,_tmp4D,_tmp4D + 6U},265,1},{{_tmp4E,_tmp4E,_tmp4E + 7U},270,1},{{_tmp4F,_tmp4F,_tmp4F + 11U},270,1},{{_tmp50,_tmp50,_tmp50 + 7U},280,1},{{_tmp51,_tmp51,_tmp51 + 7U},260,1},{{_tmp52,_tmp52,_tmp52 + 7U},275,1},{{_tmp53,_tmp53,_tmp53 + 7U},284,1},{{_tmp54,_tmp54,_tmp54 + 9U},330,0},{{_tmp55,_tmp55,_tmp55 + 6U},318,0},{{_tmp56,_tmp56,_tmp56 + 15U},325,0},{{_tmp57,_tmp57,_tmp57 + 16U},326,0},{{_tmp58,_tmp58,_tmp58 + 6U},298,0},{{_tmp59,_tmp59,_tmp59 + 4U},299,0},{{_tmp5A,_tmp5A,_tmp5A + 8U},262,1},{{_tmp5B,_tmp5B,_tmp5B + 7U},293,1},{{_tmp5C,_tmp5C,_tmp5C + 11U},293,1},{{_tmp5D,_tmp5D,_tmp5D + 6U},276,1},{{_tmp5E,_tmp5E,_tmp5E + 9U},271,1},{{_tmp5F,_tmp5F,_tmp5F + 13U},271,1},{{_tmp60,_tmp60,_tmp60 + 6U},307,0},{{_tmp61,_tmp61,_tmp61 + 8U},328,0},{{_tmp62,_tmp62,_tmp62 + 10U},329,0},{{_tmp63,_tmp63,_tmp63 + 5U},263,1},{{_tmp64,_tmp64,_tmp64 + 9U},273,1},{{_tmp65,_tmp65,_tmp65 + 13U},273,1},{{_tmp66,_tmp66,_tmp66 + 4U},385,1},{{_tmp67,_tmp67,_tmp67 + 8U},385,1},{{_tmp68,_tmp68,_tmp68 + 6U},285,1},{{_tmp69,_tmp69,_tmp69 + 6U},310,0}};
# 224
static int Cyc_Lex_num_keywords(int include_cyclone_keywords){
int sum=0;
{unsigned i=0U;for(0;i < 83U;++ i){
if(include_cyclone_keywords ||(Cyc_Lex_rw_array[(int)i]).f3)
++ sum;}}
# 226
return sum;}
# 233
static struct Cyc_Lex_Trie*Cyc_Lex_empty_trie(struct _RegionHandle*d,int dummy){
return({struct Cyc_Lex_Trie*_tmp6B=_region_malloc(d,sizeof(*_tmp6B));({union Cyc_Lex_TrieChildren _tmp1AF=({Cyc_Lex_Zero();});_tmp6B->children=_tmp1AF;}),_tmp6B->shared_str=0;_tmp6B;});}
# 237
static int Cyc_Lex_trie_char(int c){
# 239
if(c >= 95)return c - 59;else{
if(c > 64)return c - 55;}
# 239
return c - 48;}
# 244
static struct Cyc_Lex_Trie*Cyc_Lex_trie_lookup(struct _RegionHandle*r,struct Cyc_Lex_Trie*t,struct _fat_ptr buff,int offset,unsigned len){
# 246
unsigned i=0U;
buff=_fat_ptr_plus(buff,sizeof(char),offset);
if(!(len < _get_fat_size(buff,sizeof(char))))
({void*_tmp6E=0U;({int(*_tmp1B1)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp70)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp70;});struct _fat_ptr _tmp1B0=({const char*_tmp6F="array bounds in trie_lookup";_tag_fat(_tmp6F,sizeof(char),28U);});_tmp1B1(_tmp1B0,_tag_fat(_tmp6E,sizeof(void*),0U));});});
# 248
while(i < len){
# 251
union Cyc_Lex_TrieChildren _stmttmp0=((struct Cyc_Lex_Trie*)_check_null(t))->children;union Cyc_Lex_TrieChildren _tmp71=_stmttmp0;struct Cyc_Lex_Trie*_tmp73;int _tmp72;struct Cyc_Lex_Trie**_tmp74;switch((_tmp71.One).tag){case 3U: _LL1: _tmp74=(_tmp71.Many).val;_LL2: {struct Cyc_Lex_Trie**arr=_tmp74;
# 254
int ch=({Cyc_Lex_trie_char((int)((const char*)buff.curr)[(int)i]);});
if(*((struct Cyc_Lex_Trie**)_check_known_subscript_notnull(arr,64U,sizeof(struct Cyc_Lex_Trie*),ch))== 0)
({struct Cyc_Lex_Trie*_tmp1B3=({struct Cyc_Lex_Trie*_tmp75=_region_malloc(r,sizeof(*_tmp75));({union Cyc_Lex_TrieChildren _tmp1B2=({Cyc_Lex_Zero();});_tmp75->children=_tmp1B2;}),_tmp75->shared_str=0;_tmp75;});arr[ch]=_tmp1B3;});
# 255
t=arr[ch];
# 258
++ i;
goto _LL0;}case 2U: _LL3: _tmp72=((_tmp71.One).val).f1;_tmp73=((_tmp71.One).val).f2;_LL4: {int one_ch=_tmp72;struct Cyc_Lex_Trie*one_trie=_tmp73;
# 262
if(one_ch == (int)((const char*)buff.curr)[(int)i])
t=one_trie;else{
# 265
struct Cyc_Lex_Trie**arr=({unsigned _tmp78=64U;struct Cyc_Lex_Trie**_tmp77=({struct _RegionHandle*_tmp1B4=r;_region_malloc(_tmp1B4,_check_times(_tmp78,sizeof(struct Cyc_Lex_Trie*)));});({{unsigned _tmp18D=64U;unsigned j;for(j=0;j < _tmp18D;++ j){_tmp77[j]=0;}}0;});_tmp77;});
({struct Cyc_Lex_Trie*_tmp1B6=one_trie;*((struct Cyc_Lex_Trie**)({typeof(arr )_tmp1B5=arr;_check_known_subscript_notnull(_tmp1B5,64U,sizeof(struct Cyc_Lex_Trie*),({Cyc_Lex_trie_char(one_ch);}));}))=_tmp1B6;});{
int ch=({Cyc_Lex_trie_char((int)((const char*)buff.curr)[(int)i]);});
({struct Cyc_Lex_Trie*_tmp1B8=({struct Cyc_Lex_Trie*_tmp76=_region_malloc(r,sizeof(*_tmp76));({union Cyc_Lex_TrieChildren _tmp1B7=({Cyc_Lex_Zero();});_tmp76->children=_tmp1B7;}),_tmp76->shared_str=0;_tmp76;});*((struct Cyc_Lex_Trie**)_check_known_subscript_notnull(arr,64U,sizeof(struct Cyc_Lex_Trie*),ch))=_tmp1B8;});
({union Cyc_Lex_TrieChildren _tmp1B9=({Cyc_Lex_Many(arr);});t->children=_tmp1B9;});
t=arr[ch];}}
# 272
++ i;
goto _LL0;}default: _LL5: _LL6:
# 276
 while(i < len){
struct Cyc_Lex_Trie*next=({struct Cyc_Lex_Trie*_tmp7C=_region_malloc(r,sizeof(*_tmp7C));({union Cyc_Lex_TrieChildren _tmp1BA=({Cyc_Lex_Zero();});_tmp7C->children=_tmp1BA;}),_tmp7C->shared_str=0;_tmp7C;});
({union Cyc_Lex_TrieChildren _tmp1BB=({union Cyc_Lex_TrieChildren(*_tmp79)(int i,struct Cyc_Lex_Trie*t)=Cyc_Lex_One;int _tmp7A=(int)*((const char*)_check_fat_subscript(buff,sizeof(char),(int)i ++));struct Cyc_Lex_Trie*_tmp7B=next;_tmp79(_tmp7A,_tmp7B);});t->children=_tmp1BB;});
t=next;}
# 281
return t;}_LL0:;}
# 284
return t;}struct _tuple26{struct Cyc_Lex_Trie*f1;struct Cyc_Xarray_Xarray*f2;struct _fat_ptr f3;int f4;int f5;};
# 287
static int Cyc_Lex_str_index_body(struct _RegionHandle*d,struct _tuple26*env){
# 290
struct _tuple26 _stmttmp1=*env;int _tmp82;int _tmp81;struct _fat_ptr _tmp80;struct Cyc_Xarray_Xarray*_tmp7F;struct Cyc_Lex_Trie*_tmp7E;_LL1: _tmp7E=_stmttmp1.f1;_tmp7F=_stmttmp1.f2;_tmp80=_stmttmp1.f3;_tmp81=_stmttmp1.f4;_tmp82=_stmttmp1.f5;_LL2: {struct Cyc_Lex_Trie*tree=_tmp7E;struct Cyc_Xarray_Xarray*symbols=_tmp7F;struct _fat_ptr buff=_tmp80;int offset=_tmp81;int len=_tmp82;
struct Cyc_Lex_Trie*t=({Cyc_Lex_trie_lookup(d,tree,buff,offset,(unsigned)len);});
# 293
if(((struct Cyc_Lex_Trie*)_check_null(t))->shared_str == 0){
struct _fat_ptr newstr=({Cyc_Core_new_string((unsigned)(len + 1));});
({({struct _fat_ptr _tmp1BD=_fat_ptr_decrease_size(newstr,sizeof(char),1U);struct _fat_ptr _tmp1BC=(struct _fat_ptr)_fat_ptr_plus(buff,sizeof(char),offset);Cyc_zstrncpy(_tmp1BD,_tmp1BC,(unsigned long)len);});});{
int ans=({({int(*_tmp1BF)(struct Cyc_Xarray_Xarray*,struct _fat_ptr*)=({int(*_tmp83)(struct Cyc_Xarray_Xarray*,struct _fat_ptr*)=(int(*)(struct Cyc_Xarray_Xarray*,struct _fat_ptr*))Cyc_Xarray_add_ind;_tmp83;});struct Cyc_Xarray_Xarray*_tmp1BE=symbols;_tmp1BF(_tmp1BE,({struct _fat_ptr*_tmp84=_cycalloc(sizeof(*_tmp84));*_tmp84=(struct _fat_ptr)newstr;_tmp84;}));});});
t->shared_str=ans;}}
# 293
return t->shared_str;}}
# 302
static int Cyc_Lex_str_index(struct _fat_ptr buff,int offset,int len){
struct Cyc_Lex_DynTrieSymbols*idt=0;
({struct Cyc_Lex_DynTrieSymbols*_tmp86=idt;struct Cyc_Lex_DynTrieSymbols*_tmp87=Cyc_Lex_ids_trie;idt=_tmp87;Cyc_Lex_ids_trie=_tmp86;});{
struct Cyc_Lex_DynTrieSymbols _stmttmp2=*((struct Cyc_Lex_DynTrieSymbols*)_check_null(idt));struct Cyc_Xarray_Xarray*_tmp8A;struct Cyc_Lex_Trie*_tmp89;struct Cyc_Core_DynamicRegion*_tmp88;_LL1: _tmp88=_stmttmp2.dyn;_tmp89=_stmttmp2.t;_tmp8A=_stmttmp2.symbols;_LL2: {struct Cyc_Core_DynamicRegion*dyn=_tmp88;struct Cyc_Lex_Trie*t=_tmp89;struct Cyc_Xarray_Xarray*symbols=_tmp8A;
struct _tuple26 env=({struct _tuple26 _tmp18F;_tmp18F.f1=t,_tmp18F.f2=symbols,_tmp18F.f3=buff,_tmp18F.f4=offset,_tmp18F.f5=len;_tmp18F;});
int res=({({int(*_tmp1C0)(struct Cyc_Core_DynamicRegion*key,struct _tuple26*arg,int(*body)(struct _RegionHandle*h,struct _tuple26*arg))=({int(*_tmp8D)(struct Cyc_Core_DynamicRegion*key,struct _tuple26*arg,int(*body)(struct _RegionHandle*h,struct _tuple26*arg))=(int(*)(struct Cyc_Core_DynamicRegion*key,struct _tuple26*arg,int(*body)(struct _RegionHandle*h,struct _tuple26*arg)))Cyc_Core_open_region;_tmp8D;});_tmp1C0(dyn,& env,Cyc_Lex_str_index_body);});});
({struct Cyc_Lex_DynTrieSymbols _tmp1C1=({struct Cyc_Lex_DynTrieSymbols _tmp18E;_tmp18E.dyn=dyn,_tmp18E.t=t,_tmp18E.symbols=symbols;_tmp18E;});*idt=_tmp1C1;});
({struct Cyc_Lex_DynTrieSymbols*_tmp8B=idt;struct Cyc_Lex_DynTrieSymbols*_tmp8C=Cyc_Lex_ids_trie;idt=_tmp8C;Cyc_Lex_ids_trie=_tmp8B;});
return res;}}}
# 313
static int Cyc_Lex_str_index_lbuf(struct Cyc_Lexing_lexbuf*lbuf){
return({Cyc_Lex_str_index((struct _fat_ptr)lbuf->lex_buffer,lbuf->lex_start_pos,lbuf->lex_curr_pos - lbuf->lex_start_pos);});}struct _tuple27{struct Cyc_Lex_Trie*f1;struct _fat_ptr f2;};
# 319
static int Cyc_Lex_insert_typedef_body(struct _RegionHandle*h,struct _tuple27*arg){
# 321
struct _tuple27 _stmttmp3=*arg;struct _fat_ptr _tmp91;struct Cyc_Lex_Trie*_tmp90;_LL1: _tmp90=_stmttmp3.f1;_tmp91=_stmttmp3.f2;_LL2: {struct Cyc_Lex_Trie*t=_tmp90;struct _fat_ptr s=_tmp91;
struct Cyc_Lex_Trie*t_node=({Cyc_Lex_trie_lookup(h,t,s,0,_get_fat_size(s,sizeof(char))- (unsigned)1);});
((struct Cyc_Lex_Trie*)_check_null(t_node))->shared_str=1;
return 0;}}
# 328
static void Cyc_Lex_insert_typedef(struct _fat_ptr*sptr){
struct _fat_ptr s=*sptr;
struct Cyc_Lex_DynTrie*tdefs=0;
({struct Cyc_Lex_DynTrie*_tmp93=tdefs;struct Cyc_Lex_DynTrie*_tmp94=Cyc_Lex_typedefs_trie;tdefs=_tmp94;Cyc_Lex_typedefs_trie=_tmp93;});{
struct Cyc_Lex_DynTrie _stmttmp4=*((struct Cyc_Lex_DynTrie*)_check_null(tdefs));struct Cyc_Lex_Trie*_tmp96;struct Cyc_Core_DynamicRegion*_tmp95;_LL1: _tmp95=_stmttmp4.dyn;_tmp96=_stmttmp4.t;_LL2: {struct Cyc_Core_DynamicRegion*dyn=_tmp95;struct Cyc_Lex_Trie*t=_tmp96;
struct _tuple27 env=({struct _tuple27 _tmp191;_tmp191.f1=t,_tmp191.f2=s;_tmp191;});
({({int(*_tmp1C2)(struct Cyc_Core_DynamicRegion*key,struct _tuple27*arg,int(*body)(struct _RegionHandle*h,struct _tuple27*arg))=({int(*_tmp97)(struct Cyc_Core_DynamicRegion*key,struct _tuple27*arg,int(*body)(struct _RegionHandle*h,struct _tuple27*arg))=(int(*)(struct Cyc_Core_DynamicRegion*key,struct _tuple27*arg,int(*body)(struct _RegionHandle*h,struct _tuple27*arg)))Cyc_Core_open_region;_tmp97;});_tmp1C2(dyn,& env,Cyc_Lex_insert_typedef_body);});});
({struct Cyc_Lex_DynTrie _tmp1C3=({struct Cyc_Lex_DynTrie _tmp190;_tmp190.dyn=dyn,_tmp190.t=t;_tmp190;});*tdefs=_tmp1C3;});
({struct Cyc_Lex_DynTrie*_tmp98=tdefs;struct Cyc_Lex_DynTrie*_tmp99=Cyc_Lex_typedefs_trie;tdefs=_tmp99;Cyc_Lex_typedefs_trie=_tmp98;});
return;}}}struct _tuple28{struct Cyc_Lex_Trie*f1;struct Cyc_Xarray_Xarray*f2;int f3;};
# 340
static struct _fat_ptr*Cyc_Lex_get_symbol_body(struct _RegionHandle*dyn,struct _tuple28*env){
struct _tuple28 _stmttmp5=*env;int _tmp9D;struct Cyc_Xarray_Xarray*_tmp9C;struct Cyc_Lex_Trie*_tmp9B;_LL1: _tmp9B=_stmttmp5.f1;_tmp9C=_stmttmp5.f2;_tmp9D=_stmttmp5.f3;_LL2: {struct Cyc_Lex_Trie*t=_tmp9B;struct Cyc_Xarray_Xarray*symbols=_tmp9C;int symbol_num=_tmp9D;
return({({struct _fat_ptr*(*_tmp1C5)(struct Cyc_Xarray_Xarray*,int)=({struct _fat_ptr*(*_tmp9E)(struct Cyc_Xarray_Xarray*,int)=(struct _fat_ptr*(*)(struct Cyc_Xarray_Xarray*,int))Cyc_Xarray_get;_tmp9E;});struct Cyc_Xarray_Xarray*_tmp1C4=symbols;_tmp1C5(_tmp1C4,symbol_num);});});}}
# 345
static struct _fat_ptr*Cyc_Lex_get_symbol(int symbol_num){
struct Cyc_Lex_DynTrieSymbols*idt=0;
({struct Cyc_Lex_DynTrieSymbols*_tmpA0=idt;struct Cyc_Lex_DynTrieSymbols*_tmpA1=Cyc_Lex_ids_trie;idt=_tmpA1;Cyc_Lex_ids_trie=_tmpA0;});{
struct Cyc_Lex_DynTrieSymbols _stmttmp6=*((struct Cyc_Lex_DynTrieSymbols*)_check_null(idt));struct Cyc_Xarray_Xarray*_tmpA4;struct Cyc_Lex_Trie*_tmpA3;struct Cyc_Core_DynamicRegion*_tmpA2;_LL1: _tmpA2=_stmttmp6.dyn;_tmpA3=_stmttmp6.t;_tmpA4=_stmttmp6.symbols;_LL2: {struct Cyc_Core_DynamicRegion*dyn=_tmpA2;struct Cyc_Lex_Trie*t=_tmpA3;struct Cyc_Xarray_Xarray*symbols=_tmpA4;
struct _tuple28 env=({struct _tuple28 _tmp193;_tmp193.f1=t,_tmp193.f2=symbols,_tmp193.f3=symbol_num;_tmp193;});
struct _fat_ptr*res=({({struct _fat_ptr*(*_tmp1C6)(struct Cyc_Core_DynamicRegion*key,struct _tuple28*arg,struct _fat_ptr*(*body)(struct _RegionHandle*h,struct _tuple28*arg))=({struct _fat_ptr*(*_tmpA7)(struct Cyc_Core_DynamicRegion*key,struct _tuple28*arg,struct _fat_ptr*(*body)(struct _RegionHandle*h,struct _tuple28*arg))=(struct _fat_ptr*(*)(struct Cyc_Core_DynamicRegion*key,struct _tuple28*arg,struct _fat_ptr*(*body)(struct _RegionHandle*h,struct _tuple28*arg)))Cyc_Core_open_region;_tmpA7;});_tmp1C6(dyn,& env,Cyc_Lex_get_symbol_body);});});
({struct Cyc_Lex_DynTrieSymbols _tmp1C7=({struct Cyc_Lex_DynTrieSymbols _tmp192;_tmp192.dyn=dyn,_tmp192.t=t,_tmp192.symbols=symbols;_tmp192;});*idt=_tmp1C7;});
({struct Cyc_Lex_DynTrieSymbols*_tmpA5=idt;struct Cyc_Lex_DynTrieSymbols*_tmpA6=Cyc_Lex_ids_trie;idt=_tmpA6;Cyc_Lex_ids_trie=_tmpA5;});
return res;}}}
# 364 "lex.cyl"
static int Cyc_Lex_int_of_char(char c){
if((int)'0' <= (int)c &&(int)c <= (int)'9')return(int)c - (int)'0';else{
if((int)'a' <= (int)c &&(int)c <= (int)'f')return(10 + (int)c)- (int)'a';else{
if((int)'A' <= (int)c &&(int)c <= (int)'F')return(10 + (int)c)- (int)'A';else{
(int)_throw(({struct Cyc_Core_Invalid_argument_exn_struct*_tmpAA=_cycalloc(sizeof(*_tmpAA));_tmpAA->tag=Cyc_Core_Invalid_argument,({struct _fat_ptr _tmp1C8=({const char*_tmpA9="string to integer conversion";_tag_fat(_tmpA9,sizeof(char),29U);});_tmpAA->f1=_tmp1C8;});_tmpAA;}));}}}}
# 372
static union Cyc_Absyn_Cnst Cyc_Lex_intconst(struct Cyc_Lexing_lexbuf*lbuf,int start,int end,int base){
enum Cyc_Absyn_Sign sn=2U;
start +=lbuf->lex_start_pos;{
struct _fat_ptr buff=_fat_ptr_plus(lbuf->lex_buffer,sizeof(char),start);
int end2=lbuf->lex_curr_pos - end;
int len=end2 - start;
enum Cyc_Absyn_Size_of size=2U;
int declared_size=0;
union Cyc_Absyn_Cnst res;
if(len >= 1 &&((int)*((char*)_check_fat_subscript(buff,sizeof(char),len - 1))== (int)'l' ||(int)*((char*)_check_fat_subscript(buff,sizeof(char),len - 1))== (int)'L')){
# 383
len -=1;
declared_size=1;
if(len >= 1 &&((int)*((char*)_check_fat_subscript(buff,sizeof(char),len - 1))== (int)'l' ||(int)*((char*)_check_fat_subscript(buff,sizeof(char),len - 1))== (int)'L')){
len -=1;
size=4U;}}
# 381
if(
# 390
len >= 1 &&((int)*((char*)_check_fat_subscript(buff,sizeof(char),len - 1))== (int)'u' ||(int)*((char*)_check_fat_subscript(buff,sizeof(char),len - 1))== (int)'U')){
len -=1;
sn=1U;}
# 381
if((int)sn == (int)1U){
# 395
unsigned long long n=(unsigned long long)0;
{int i=0;for(0;i < len;++ i){
n=({unsigned long long _tmp1C9=n * (unsigned long long)base;_tmp1C9 + (unsigned long long)({Cyc_Lex_int_of_char(*((char*)_check_fat_subscript(buff,sizeof(char),i)));});});}}
if(n > (unsigned long long)-1){
if(declared_size &&(int)size == (int)2U)
({({struct _fat_ptr _tmp1CA=({const char*_tmpAC="integer constant too large";_tag_fat(_tmpAC,sizeof(char),27U);});Cyc_Lex_err(_tmp1CA,lbuf);});});
# 399
size=4U;}
# 398
if((int)size == (int)2U)
# 404
res=({union Cyc_Absyn_Cnst _tmp194;(_tmp194.Int_c).tag=5U,((_tmp194.Int_c).val).f1=sn,((_tmp194.Int_c).val).f2=(int)((unsigned)n);_tmp194;});else{
# 406
res=({union Cyc_Absyn_Cnst _tmp195;(_tmp195.LongLong_c).tag=6U,((_tmp195.LongLong_c).val).f1=sn,((_tmp195.LongLong_c).val).f2=(long long)n;_tmp195;});}}else{
# 409
long long n=(long long)0;
{int i=0;for(0;i < len;++ i){
n=({long long _tmp1CB=n * (long long)base;_tmp1CB + (long long)({Cyc_Lex_int_of_char(*((char*)_check_fat_subscript(buff,sizeof(char),i)));});});}}{
unsigned long long x=(unsigned long long)n >> (unsigned long long)32;
if(x != (unsigned long long)-1 && x != (unsigned long long)0){
if(declared_size &&(int)size == (int)2U)
({({struct _fat_ptr _tmp1CC=({const char*_tmpAD="integer constant too large";_tag_fat(_tmpAD,sizeof(char),27U);});Cyc_Lex_err(_tmp1CC,lbuf);});});
# 414
size=4U;}
# 413
if((int)size == (int)2U)
# 419
res=({union Cyc_Absyn_Cnst _tmp196;(_tmp196.Int_c).tag=5U,((_tmp196.Int_c).val).f1=sn,((_tmp196.Int_c).val).f2=(int)n;_tmp196;});else{
# 421
res=({union Cyc_Absyn_Cnst _tmp197;(_tmp197.LongLong_c).tag=6U,((_tmp197.LongLong_c).val).f1=sn,((_tmp197.LongLong_c).val).f2=n;_tmp197;});}}}
# 424
return res;}}
# 372
char Cyc_Lex_string_buffer_v[11U]={'x','x','x','x','x','x','x','x','x','x','\000'};
# 430
struct _fat_ptr Cyc_Lex_string_buffer={(void*)Cyc_Lex_string_buffer_v,(void*)Cyc_Lex_string_buffer_v,(void*)(Cyc_Lex_string_buffer_v + 11U)};
int Cyc_Lex_string_pos=0;
void Cyc_Lex_store_string_char(char c){
int sz=(int)(_get_fat_size(Cyc_Lex_string_buffer,sizeof(char))- (unsigned)1);
if(Cyc_Lex_string_pos >= sz){
int newsz=sz;
while(Cyc_Lex_string_pos >= newsz){newsz=newsz * 2;}{
struct _fat_ptr str=({unsigned _tmpB0=(unsigned)newsz + 1U;char*_tmpAF=_cycalloc_atomic(_check_times(_tmpB0,sizeof(char)));({{unsigned _tmp198=(unsigned)newsz;unsigned i;for(i=0;i < _tmp198;++ i){i < (unsigned)sz?_tmpAF[i]=*((char*)_check_fat_subscript(Cyc_Lex_string_buffer,sizeof(char),(int)i)):(_tmpAF[i]='\000');}_tmpAF[_tmp198]=0;}0;});_tag_fat(_tmpAF,sizeof(char),_tmpB0);});
Cyc_Lex_string_buffer=str;}}
# 434
({struct _fat_ptr _tmpB1=_fat_ptr_plus(Cyc_Lex_string_buffer,sizeof(char),Cyc_Lex_string_pos);char _tmpB2=*((char*)_check_fat_subscript(_tmpB1,sizeof(char),0U));char _tmpB3=c;if(_get_fat_size(_tmpB1,sizeof(char))== 1U &&(_tmpB2 == 0 && _tmpB3 != 0))_throw_arraybounds();*((char*)_tmpB1.curr)=_tmpB3;});
# 441
++ Cyc_Lex_string_pos;}
# 443
void Cyc_Lex_store_string(struct _fat_ptr s){
int sz=(int)({Cyc_strlen((struct _fat_ptr)s);});
int i=0;for(0;i < sz;++ i){
({Cyc_Lex_store_string_char(*((const char*)_check_fat_subscript(s,sizeof(char),i)));});}}
# 443
struct _fat_ptr Cyc_Lex_get_stored_string(){
# 449
struct _fat_ptr str=({Cyc_substring((struct _fat_ptr)Cyc_Lex_string_buffer,0,(unsigned long)Cyc_Lex_string_pos);});
Cyc_Lex_string_pos=0;
return str;}struct Cyc_Lex_Ldecls{struct Cyc_Set_Set*typedefs;};
# 466 "lex.cyl"
static struct Cyc_Core_Opt*Cyc_Lex_lstate=0;
# 468
static struct Cyc_Lex_Ldecls*Cyc_Lex_mk_empty_ldecls(int ignore){
return({struct Cyc_Lex_Ldecls*_tmpB8=_cycalloc(sizeof(*_tmpB8));({struct Cyc_Set_Set*_tmp1CD=({({struct Cyc_Set_Set*(*_tmpB7)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Set_empty;_tmpB7;})(Cyc_zstrptrcmp);});_tmpB8->typedefs=_tmp1CD;});_tmpB8;});}
# 472
static void Cyc_Lex_typedef_init(){
Cyc_Lex_lstate=({struct Cyc_Core_Opt*_tmpBB=_cycalloc(sizeof(*_tmpBB));({struct Cyc_Binding_NSCtxt*_tmp1CE=({({struct Cyc_Binding_NSCtxt*(*_tmpBA)(int,struct Cyc_Lex_Ldecls*(*mkdata)(int))=(struct Cyc_Binding_NSCtxt*(*)(int,struct Cyc_Lex_Ldecls*(*mkdata)(int)))Cyc_Binding_mt_nsctxt;_tmpBA;})(1,Cyc_Lex_mk_empty_ldecls);});_tmpBB->v=_tmp1CE;});_tmpBB;});}
# 476
static void Cyc_Lex_recompute_typedefs(){
# 478
struct Cyc_Lex_DynTrie*tdefs=0;
({struct Cyc_Lex_DynTrie*_tmpBD=tdefs;struct Cyc_Lex_DynTrie*_tmpBE=Cyc_Lex_typedefs_trie;tdefs=_tmpBE;Cyc_Lex_typedefs_trie=_tmpBD;});{
struct Cyc_Lex_DynTrie _stmttmp7=*((struct Cyc_Lex_DynTrie*)_check_null(tdefs));struct Cyc_Lex_Trie*_tmpC0;struct Cyc_Core_DynamicRegion*_tmpBF;_LL1: _tmpBF=_stmttmp7.dyn;_tmpC0=_stmttmp7.t;_LL2: {struct Cyc_Core_DynamicRegion*dyn=_tmpBF;struct Cyc_Lex_Trie*t=_tmpC0;
({Cyc_Core_free_ukey(dyn);});{
struct Cyc_Core_NewDynamicRegion _stmttmp8=({Cyc_Core__new_ukey("internal-error","internal-error",0);});struct Cyc_Core_DynamicRegion*_tmpC1;_LL4: _tmpC1=_stmttmp8.key;_LL5: {struct Cyc_Core_DynamicRegion*dyn2=_tmpC1;
struct Cyc_Lex_Trie*t2=({({struct Cyc_Lex_Trie*(*_tmp1CF)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Lex_Trie*(*body)(struct _RegionHandle*h,int arg))=({struct Cyc_Lex_Trie*(*_tmpC9)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Lex_Trie*(*body)(struct _RegionHandle*h,int arg))=(struct Cyc_Lex_Trie*(*)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Lex_Trie*(*body)(struct _RegionHandle*h,int arg)))Cyc_Core_open_region;_tmpC9;});_tmp1CF(dyn2,0,Cyc_Lex_empty_trie);});});
({struct Cyc_Lex_DynTrie _tmp1D0=({struct Cyc_Lex_DynTrie _tmp199;_tmp199.dyn=dyn2,_tmp199.t=t2;_tmp199;});*tdefs=_tmp1D0;});
({struct Cyc_Lex_DynTrie*_tmpC2=Cyc_Lex_typedefs_trie;struct Cyc_Lex_DynTrie*_tmpC3=tdefs;Cyc_Lex_typedefs_trie=_tmpC3;tdefs=_tmpC2;});{
# 488
struct Cyc_List_List*as=((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v)->availables;for(0;as != 0;as=as->tl){
void*_stmttmp9=(void*)as->hd;void*_tmpC4=_stmttmp9;struct Cyc_List_List*_tmpC5;struct Cyc_List_List*_tmpC6;if(((struct Cyc_Binding_Using_Binding_NSDirective_struct*)_tmpC4)->tag == 1U){_LL7: _tmpC6=((struct Cyc_Binding_Using_Binding_NSDirective_struct*)_tmpC4)->f1;_LL8: {struct Cyc_List_List*ns=_tmpC6;
_tmpC5=ns;goto _LLA;}}else{_LL9: _tmpC5=((struct Cyc_Binding_Namespace_Binding_NSDirective_struct*)_tmpC4)->f1;_LLA: {struct Cyc_List_List*ns=_tmpC5;
# 492
struct Cyc_Lex_Ldecls*ts=({({struct Cyc_Lex_Ldecls*(*_tmp1D2)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({struct Cyc_Lex_Ldecls*(*_tmpC8)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(struct Cyc_Lex_Ldecls*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmpC8;});struct Cyc_Dict_Dict _tmp1D1=((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v)->ns_data;_tmp1D2(_tmp1D1,ns);});});
({({void(*_tmp1D3)(void(*f)(struct _fat_ptr*),struct Cyc_Set_Set*s)=({void(*_tmpC7)(void(*f)(struct _fat_ptr*),struct Cyc_Set_Set*s)=(void(*)(void(*f)(struct _fat_ptr*),struct Cyc_Set_Set*s))Cyc_Set_iter;_tmpC7;});_tmp1D3(Cyc_Lex_insert_typedef,ts->typedefs);});});
goto _LL6;}}_LL6:;}}}}}}}
# 500
static int Cyc_Lex_is_typedef_in_namespace(struct Cyc_List_List*ns,struct _fat_ptr*v){
struct Cyc_List_List*ans=({Cyc_Binding_resolve_rel_ns(0U,(struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v,ns);});
struct Cyc_Lex_Ldecls*ts=({({struct Cyc_Lex_Ldecls*(*_tmp1D5)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({struct Cyc_Lex_Ldecls*(*_tmpCC)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(struct Cyc_Lex_Ldecls*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmpCC;});struct Cyc_Dict_Dict _tmp1D4=((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v)->ns_data;_tmp1D5(_tmp1D4,ans);});});
return({({int(*_tmp1D7)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({int(*_tmpCB)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(int(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_member;_tmpCB;});struct Cyc_Set_Set*_tmp1D6=ts->typedefs;_tmp1D7(_tmp1D6,v);});});}struct _tuple29{struct Cyc_List_List*f1;struct Cyc_Lex_Trie*f2;struct _fat_ptr f3;};
# 506
static int Cyc_Lex_is_typedef_body(struct _RegionHandle*d,struct _tuple29*env){
# 509
struct _tuple29 _stmttmpA=*env;struct _fat_ptr _tmpD0;struct Cyc_Lex_Trie*_tmpCF;struct Cyc_List_List*_tmpCE;_LL1: _tmpCE=_stmttmpA.f1;_tmpCF=_stmttmpA.f2;_tmpD0=_stmttmpA.f3;_LL2: {struct Cyc_List_List*ns=_tmpCE;struct Cyc_Lex_Trie*t=_tmpCF;struct _fat_ptr s=_tmpD0;
int len=(int)(_get_fat_size(s,sizeof(char))- (unsigned)1);
{int i=0;for(0;i < len;++ i){
union Cyc_Lex_TrieChildren _stmttmpB=((struct Cyc_Lex_Trie*)_check_null(t))->children;union Cyc_Lex_TrieChildren _tmpD1=_stmttmpB;struct Cyc_Lex_Trie**_tmpD2;struct Cyc_Lex_Trie*_tmpD3;struct Cyc_Lex_Trie*_tmpD5;int _tmpD4;switch((_tmpD1.One).tag){case 1U: _LL4: _LL5:
 return 0;case 2U: _LL6: _tmpD4=((_tmpD1.One).val).f1;_tmpD5=((_tmpD1.One).val).f2;if(_tmpD4 != (int)*((const char*)_check_fat_subscript(s,sizeof(char),i))){_LL7: {int one_ch=_tmpD4;struct Cyc_Lex_Trie*one_trie=_tmpD5;
return 0;}}else{_LL8: _tmpD3=((_tmpD1.One).val).f2;_LL9: {struct Cyc_Lex_Trie*one_trie=_tmpD3;
t=one_trie;goto _LL3;}}default: _LLA: _tmpD2=(_tmpD1.Many).val;_LLB: {struct Cyc_Lex_Trie**arr=_tmpD2;
# 517
struct Cyc_Lex_Trie*next=*((struct Cyc_Lex_Trie**)({typeof(arr )_tmp1D8=arr;_check_known_subscript_notnull(_tmp1D8,64U,sizeof(struct Cyc_Lex_Trie*),({Cyc_Lex_trie_char((int)((const char*)s.curr)[i]);}));}));
if(next == 0)
return 0;
# 518
t=next;
# 521
goto _LL3;}}_LL3:;}}
# 523
return((struct Cyc_Lex_Trie*)_check_null(t))->shared_str;}}
# 525
static int Cyc_Lex_is_typedef(struct Cyc_List_List*ns,struct _fat_ptr*v){
if(ns != 0)
return({Cyc_Lex_is_typedef_in_namespace(ns,v);});{
# 526
struct _fat_ptr s=*v;
# 531
struct Cyc_Lex_DynTrie*tdefs=0;
({struct Cyc_Lex_DynTrie*_tmpD7=tdefs;struct Cyc_Lex_DynTrie*_tmpD8=Cyc_Lex_typedefs_trie;tdefs=_tmpD8;Cyc_Lex_typedefs_trie=_tmpD7;});{
struct Cyc_Lex_DynTrie _stmttmpC=*((struct Cyc_Lex_DynTrie*)_check_null(tdefs));struct Cyc_Lex_Trie*_tmpDA;struct Cyc_Core_DynamicRegion*_tmpD9;_LL1: _tmpD9=_stmttmpC.dyn;_tmpDA=_stmttmpC.t;_LL2: {struct Cyc_Core_DynamicRegion*dyn=_tmpD9;struct Cyc_Lex_Trie*t=_tmpDA;
struct _tuple29 env=({struct _tuple29 _tmp19B;_tmp19B.f1=ns,_tmp19B.f2=t,_tmp19B.f3=s;_tmp19B;});
int res=({({int(*_tmp1D9)(struct Cyc_Core_DynamicRegion*key,struct _tuple29*arg,int(*body)(struct _RegionHandle*h,struct _tuple29*arg))=({int(*_tmpDD)(struct Cyc_Core_DynamicRegion*key,struct _tuple29*arg,int(*body)(struct _RegionHandle*h,struct _tuple29*arg))=(int(*)(struct Cyc_Core_DynamicRegion*key,struct _tuple29*arg,int(*body)(struct _RegionHandle*h,struct _tuple29*arg)))Cyc_Core_open_region;_tmpDD;});_tmp1D9(dyn,& env,Cyc_Lex_is_typedef_body);});});
({struct Cyc_Lex_DynTrie _tmp1DA=({struct Cyc_Lex_DynTrie _tmp19A;_tmp19A.dyn=dyn,_tmp19A.t=t;_tmp19A;});*tdefs=_tmp1DA;});
({struct Cyc_Lex_DynTrie*_tmpDB=tdefs;struct Cyc_Lex_DynTrie*_tmpDC=Cyc_Lex_typedefs_trie;tdefs=_tmpDC;Cyc_Lex_typedefs_trie=_tmpDB;});
return res;}}}}
# 541
void Cyc_Lex_enter_namespace(struct _fat_ptr*s){
({({void(*_tmp1DC)(struct Cyc_Binding_NSCtxt*,struct _fat_ptr*,int,struct Cyc_Lex_Ldecls*(*mkdata)(int))=({void(*_tmpDF)(struct Cyc_Binding_NSCtxt*,struct _fat_ptr*,int,struct Cyc_Lex_Ldecls*(*mkdata)(int))=(void(*)(struct Cyc_Binding_NSCtxt*,struct _fat_ptr*,int,struct Cyc_Lex_Ldecls*(*mkdata)(int)))Cyc_Binding_enter_ns;_tmpDF;});struct Cyc_Binding_NSCtxt*_tmp1DB=(struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v;_tmp1DC(_tmp1DB,s,1,Cyc_Lex_mk_empty_ldecls);});});{
struct Cyc_Lex_Ldecls*ts=({({struct Cyc_Lex_Ldecls*(*_tmp1DE)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({struct Cyc_Lex_Ldecls*(*_tmpE1)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(struct Cyc_Lex_Ldecls*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmpE1;});struct Cyc_Dict_Dict _tmp1DD=((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v)->ns_data;_tmp1DE(_tmp1DD,((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v)->curr_ns);});});
# 546
({({void(*_tmp1DF)(void(*f)(struct _fat_ptr*),struct Cyc_Set_Set*s)=({void(*_tmpE0)(void(*f)(struct _fat_ptr*),struct Cyc_Set_Set*s)=(void(*)(void(*f)(struct _fat_ptr*),struct Cyc_Set_Set*s))Cyc_Set_iter;_tmpE0;});_tmp1DF(Cyc_Lex_insert_typedef,ts->typedefs);});});}}
# 548
void Cyc_Lex_leave_namespace(){
({Cyc_Binding_leave_ns((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v);});
({Cyc_Lex_recompute_typedefs();});}
# 552
void Cyc_Lex_enter_using(struct _tuple0*q){
struct Cyc_List_List*ns=({Cyc_Binding_enter_using(0U,(struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v,q);});
struct Cyc_Lex_Ldecls*ts=({({struct Cyc_Lex_Ldecls*(*_tmp1E1)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({struct Cyc_Lex_Ldecls*(*_tmpE5)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(struct Cyc_Lex_Ldecls*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmpE5;});struct Cyc_Dict_Dict _tmp1E0=((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v)->ns_data;_tmp1E1(_tmp1E0,ns);});});
# 557
({({void(*_tmp1E2)(void(*f)(struct _fat_ptr*),struct Cyc_Set_Set*s)=({void(*_tmpE4)(void(*f)(struct _fat_ptr*),struct Cyc_Set_Set*s)=(void(*)(void(*f)(struct _fat_ptr*),struct Cyc_Set_Set*s))Cyc_Set_iter;_tmpE4;});_tmp1E2(Cyc_Lex_insert_typedef,ts->typedefs);});});}
# 559
void Cyc_Lex_leave_using(){
({Cyc_Binding_leave_using((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v);});
# 562
({Cyc_Lex_recompute_typedefs();});}
# 565
void Cyc_Lex_register_typedef(struct _tuple0*q){
# 567
struct Cyc_Lex_Ldecls*ts=({({struct Cyc_Lex_Ldecls*(*_tmp1E4)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=({struct Cyc_Lex_Ldecls*(*_tmpE9)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k)=(struct Cyc_Lex_Ldecls*(*)(struct Cyc_Dict_Dict d,struct Cyc_List_List*k))Cyc_Dict_lookup;_tmpE9;});struct Cyc_Dict_Dict _tmp1E3=((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v)->ns_data;_tmp1E4(_tmp1E3,((struct Cyc_Binding_NSCtxt*)((struct Cyc_Core_Opt*)_check_null(Cyc_Lex_lstate))->v)->curr_ns);});});
({struct Cyc_Set_Set*_tmp1E7=({({struct Cyc_Set_Set*(*_tmp1E6)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({struct Cyc_Set_Set*(*_tmpE8)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_insert;_tmpE8;});struct Cyc_Set_Set*_tmp1E5=ts->typedefs;_tmp1E6(_tmp1E5,(*q).f2);});});ts->typedefs=_tmp1E7;});
# 572
({Cyc_Lex_insert_typedef((*q).f2);});}
# 575
static short Cyc_Lex_process_id(struct Cyc_Lexing_lexbuf*lbuf){
int symbol_num=({Cyc_Lex_str_index_lbuf(lbuf);});
# 579
if(symbol_num <= Cyc_Lex_num_kws){
# 581
if(!Cyc_Lex_in_extern_c ||(*((struct Cyc_Lex_KeyWordInfo*)_check_fat_subscript(Cyc_Lex_kw_nums,sizeof(struct Cyc_Lex_KeyWordInfo),symbol_num - 1))).is_c_keyword){
short res=(short)(*((struct Cyc_Lex_KeyWordInfo*)_check_fat_subscript(Cyc_Lex_kw_nums,sizeof(struct Cyc_Lex_KeyWordInfo),symbol_num - 1))).token_index;
return res;}}{
# 579
struct _fat_ptr*s=({Cyc_Lex_get_symbol(symbol_num);});
# 588
Cyc_Lex_token_string=*s;
# 591
if(({Cyc_Lex_is_typedef(0,s);}))
return 381;
# 591
return 373;}}
# 598
static short Cyc_Lex_process_qual_id(struct Cyc_Lexing_lexbuf*lbuf){
if(Cyc_Lex_in_extern_c)
({({struct _fat_ptr _tmp1E8=({const char*_tmpEC="qualified identifiers not allowed in C code";_tag_fat(_tmpEC,sizeof(char),44U);});Cyc_Lex_err(_tmp1E8,lbuf);});});{
# 599
int i=lbuf->lex_start_pos;
# 602
int end=lbuf->lex_curr_pos;
struct _fat_ptr s=lbuf->lex_buffer;
# 605
struct _fat_ptr*v=0;
struct Cyc_List_List*rev_vs=0;
# 608
while(i < end){
int start=i;
for(0;i < end &&(int)*((char*)_check_fat_subscript(s,sizeof(char),i))!= (int)':';++ i){
;}
if(start == i)
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmpEE=_cycalloc(sizeof(*_tmpEE));_tmpEE->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp1E9=({const char*_tmpED="bad namespace";_tag_fat(_tmpED,sizeof(char),14U);});_tmpEE->f1=_tmp1E9;});_tmpEE;}));{
# 612
int vlen=i - start;
# 615
if(v != 0)
rev_vs=({struct Cyc_List_List*_tmpEF=_cycalloc(sizeof(*_tmpEF));_tmpEF->hd=v,_tmpEF->tl=rev_vs;_tmpEF;});
# 615
v=({struct _fat_ptr*(*_tmpF0)(int symbol_num)=Cyc_Lex_get_symbol;int _tmpF1=({Cyc_Lex_str_index((struct _fat_ptr)s,start,vlen);});_tmpF0(_tmpF1);});
# 618
i +=2;}}
# 620
if(v == 0)
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmpF3=_cycalloc(sizeof(*_tmpF3));_tmpF3->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp1EA=({const char*_tmpF2="bad namespace";_tag_fat(_tmpF2,sizeof(char),14U);});_tmpF3->f1=_tmp1EA;});_tmpF3;}));{
# 620
struct Cyc_List_List*vs=({Cyc_List_imp_rev(rev_vs);});
# 625
if(vs != 0 &&({({struct _fat_ptr _tmp1EB=(struct _fat_ptr)*((struct _fat_ptr*)vs->hd);Cyc_strcmp(_tmp1EB,({const char*_tmpF4="Cyc";_tag_fat(_tmpF4,sizeof(char),4U);}));});})== 0){
vs=vs->tl;
Cyc_Lex_token_qvar=({struct _tuple0*_tmpF5=_cycalloc(sizeof(*_tmpF5));({union Cyc_Absyn_Nmspace _tmp1EC=({Cyc_Absyn_Abs_n(vs,0);});_tmpF5->f1=_tmp1EC;}),_tmpF5->f2=v;_tmpF5;});}else{
# 629
if(vs != 0 &&({({struct _fat_ptr _tmp1ED=(struct _fat_ptr)*((struct _fat_ptr*)vs->hd);Cyc_strcmp(_tmp1ED,({const char*_tmpF6="C";_tag_fat(_tmpF6,sizeof(char),2U);}));});})== 0){
vs=vs->tl;
Cyc_Lex_token_qvar=({struct _tuple0*_tmpF7=_cycalloc(sizeof(*_tmpF7));({union Cyc_Absyn_Nmspace _tmp1EE=({Cyc_Absyn_Abs_n(vs,1);});_tmpF7->f1=_tmp1EE;}),_tmpF7->f2=v;_tmpF7;});}else{
# 633
Cyc_Lex_token_qvar=({struct _tuple0*_tmpF8=_cycalloc(sizeof(*_tmpF8));({union Cyc_Absyn_Nmspace _tmp1EF=({Cyc_Absyn_Rel_n(vs);});_tmpF8->f1=_tmp1EF;}),_tmpF8->f2=v;_tmpF8;});}}
if(({Cyc_Lex_is_typedef(vs,v);}))
return 383;
# 634
return 382;}}}struct Cyc_Lex_PosInfo{struct Cyc_Lex_PosInfo*next;unsigned starting_line;struct _fat_ptr filename;struct _fat_ptr linenumpos;unsigned linenumpos_offset;};
# 652
static struct Cyc_Lex_PosInfo*Cyc_Lex_pos_info=0;
# 654
static int Cyc_Lex_linenumber=1;
# 657
static struct Cyc_Lex_PosInfo*Cyc_Lex_rnew_filepos(struct _RegionHandle*r,struct _fat_ptr filename,unsigned starting_line,struct Cyc_Lex_PosInfo*next){
# 661
struct _fat_ptr linenumpos=({unsigned _tmpFB=10U;_tag_fat(_region_calloc(Cyc_Core_unique_region,sizeof(unsigned),_tmpFB),sizeof(unsigned),_tmpFB);});
*((unsigned*)_check_fat_subscript(linenumpos,sizeof(unsigned),0))=(unsigned)Cyc_yylloc.first_line;
return({struct Cyc_Lex_PosInfo*_tmpFA=_region_malloc(r,sizeof(*_tmpFA));_tmpFA->next=next,_tmpFA->starting_line=starting_line,_tmpFA->filename=(struct _fat_ptr)filename,_tmpFA->linenumpos=linenumpos,_tmpFA->linenumpos_offset=1U;_tmpFA;});}
# 670
static void Cyc_Lex_inc_linenumber(){
if(Cyc_Lex_pos_info == 0)(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmpFE=_cycalloc(sizeof(*_tmpFE));_tmpFE->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp1F0=({const char*_tmpFD="empty position info!";_tag_fat(_tmpFD,sizeof(char),21U);});_tmpFE->f1=_tmp1F0;});_tmpFE;}));{struct Cyc_Lex_PosInfo*p=(struct Cyc_Lex_PosInfo*)_check_null(Cyc_Lex_pos_info);
# 673
struct _fat_ptr linenums=_tag_fat(0,0,0);
({struct _fat_ptr _tmpFF=p->linenumpos;struct _fat_ptr _tmp100=linenums;p->linenumpos=_tmp100;linenums=_tmpFF;});{
unsigned offset=p->linenumpos_offset;
unsigned n=_get_fat_size(linenums,sizeof(unsigned));
# 678
if(offset >= n){
# 680
struct _fat_ptr newlinenums=({unsigned _tmp104=n * (unsigned)2;_tag_fat(_region_calloc(Cyc_Core_unique_region,sizeof(unsigned),_tmp104),sizeof(unsigned),_tmp104);});
{unsigned i=0U;for(0;i < n;++ i){
*((unsigned*)_check_fat_subscript(newlinenums,sizeof(unsigned),(int)i))=((unsigned*)linenums.curr)[(int)i];}}
({struct _fat_ptr _tmp101=linenums;struct _fat_ptr _tmp102=newlinenums;linenums=_tmp102;newlinenums=_tmp101;});
# 685
({({void(*_tmp1F1)(unsigned*ptr)=({void(*_tmp103)(unsigned*ptr)=(void(*)(unsigned*ptr))Cyc_Core_ufree;_tmp103;});_tmp1F1((unsigned*)_untag_fat_ptr(newlinenums,sizeof(unsigned),1U));});});}
# 678
*((unsigned*)_check_fat_subscript(linenums,sizeof(unsigned),(int)offset))=(unsigned)Cyc_yylloc.first_line;
# 689
p->linenumpos_offset=offset + (unsigned)1;
++ Cyc_Lex_linenumber;
({struct _fat_ptr _tmp105=p->linenumpos;struct _fat_ptr _tmp106=linenums;p->linenumpos=_tmp106;linenums=_tmp105;});}}}
# 695
static void Cyc_Lex_process_directive(struct _fat_ptr line){
int i;
char buf[100U];
struct _fat_ptr filename=({const char*_tmp115="";_tag_fat(_tmp115,sizeof(char),1U);});
if(({struct Cyc_IntPtr_sa_ScanfArg_struct _tmp10A=({struct Cyc_IntPtr_sa_ScanfArg_struct _tmp19D;_tmp19D.tag=2U,_tmp19D.f1=& i;_tmp19D;});struct Cyc_CharPtr_sa_ScanfArg_struct _tmp10B=({struct Cyc_CharPtr_sa_ScanfArg_struct _tmp19C;_tmp19C.tag=7U,({struct _fat_ptr _tmp1F2=_tag_fat(buf,sizeof(char),100U);_tmp19C.f1=_tmp1F2;});_tmp19C;});void*_tmp108[2U];_tmp108[0]=& _tmp10A,_tmp108[1]=& _tmp10B;({struct _fat_ptr _tmp1F4=(struct _fat_ptr)line;struct _fat_ptr _tmp1F3=({const char*_tmp109="# %d \"%s";_tag_fat(_tmp109,sizeof(char),9U);});Cyc_sscanf(_tmp1F4,_tmp1F3,_tag_fat(_tmp108,sizeof(void*),2U));});})== 2){
if(Cyc_Lex_compile_for_boot_r){
# 703
int i=(int)(({Cyc_strlen(_tag_fat(buf,sizeof(char),100U));})- (unsigned long)1);
int last_slash=-1;
while(i >= 0){
if((int)*((char*)_check_known_subscript_notnull(buf,100U,sizeof(char),i))== (int)'/'){last_slash=i;break;}
-- i;}
# 709
filename=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp10E=({struct Cyc_String_pa_PrintArg_struct _tmp19E;_tmp19E.tag=0U,({struct _fat_ptr _tmp1F6=(struct _fat_ptr)((struct _fat_ptr)_fat_ptr_plus(({struct _fat_ptr _tmp1F5=_tag_fat(buf,sizeof(char),100U);_fat_ptr_plus(_tmp1F5,sizeof(char),last_slash);}),sizeof(char),1));_tmp19E.f1=_tmp1F6;});_tmp19E;});void*_tmp10C[1U];_tmp10C[0]=& _tmp10E;({struct _fat_ptr _tmp1F7=({const char*_tmp10D="\"%s";_tag_fat(_tmp10D,sizeof(char),4U);});Cyc_aprintf(_tmp1F7,_tag_fat(_tmp10C,sizeof(void*),1U));});});}else{
# 711
filename=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp111=({struct Cyc_String_pa_PrintArg_struct _tmp19F;_tmp19F.tag=0U,({struct _fat_ptr _tmp1F8=(struct _fat_ptr)_tag_fat(buf,sizeof(char),100U);_tmp19F.f1=_tmp1F8;});_tmp19F;});void*_tmp10F[1U];_tmp10F[0]=& _tmp111;({struct _fat_ptr _tmp1F9=({const char*_tmp110="\"%s";_tag_fat(_tmp110,sizeof(char),4U);});Cyc_aprintf(_tmp1F9,_tag_fat(_tmp10F,sizeof(void*),1U));});});}
if((Cyc_Lex_linenumber == i && Cyc_Lex_pos_info != 0)&&({Cyc_strcmp((struct _fat_ptr)((struct Cyc_Lex_PosInfo*)_check_null(Cyc_Lex_pos_info))->filename,(struct _fat_ptr)filename);})== 0)return;Cyc_Lex_linenumber=i;}else{
# 714
if(({struct Cyc_IntPtr_sa_ScanfArg_struct _tmp114=({struct Cyc_IntPtr_sa_ScanfArg_struct _tmp1A0;_tmp1A0.tag=2U,_tmp1A0.f1=& i;_tmp1A0;});void*_tmp112[1U];_tmp112[0]=& _tmp114;({struct _fat_ptr _tmp1FB=(struct _fat_ptr)line;struct _fat_ptr _tmp1FA=({const char*_tmp113="# %d";_tag_fat(_tmp113,sizeof(char),5U);});Cyc_sscanf(_tmp1FB,_tmp1FA,_tag_fat(_tmp112,sizeof(void*),1U));});})== 1){
if(Cyc_Lex_linenumber == i)return;Cyc_Lex_linenumber=i;
# 717
if(Cyc_Lex_pos_info != 0)filename=((struct Cyc_Lex_PosInfo*)_check_null(Cyc_Lex_pos_info))->filename;}else{
# 719
++ Cyc_Lex_linenumber;
return;}}
# 722
Cyc_Lex_pos_info=({Cyc_Lex_rnew_filepos(Cyc_Core_heap_region,filename,(unsigned)Cyc_Lex_linenumber,Cyc_Lex_pos_info);});}struct _tuple30{struct _fat_ptr f1;unsigned f2;};
# 727
struct _tuple30 Cyc_Lex_xlate_pos(unsigned char_offset){
{struct Cyc_Lex_PosInfo*p=Cyc_Lex_pos_info;for(0;p != 0;p=p->next){
unsigned first_char_offset=*((unsigned*)_check_fat_subscript(p->linenumpos,sizeof(unsigned),0));
if(char_offset < first_char_offset && p->next != 0)continue;{unsigned base=0U;
# 734
unsigned size=p->linenumpos_offset;
while(size > (unsigned)1){
int half=(int)(size / (unsigned)2);
int mid=(int)(base + (unsigned)half);
if(char_offset > *((unsigned*)_check_fat_subscript(p->linenumpos,sizeof(unsigned),mid))){
base=base + (unsigned)half;
size=size - (unsigned)half;}else{
# 742
size=(unsigned)half;}}
# 745
return({struct _tuple30 _tmp1A1;_tmp1A1.f1=p->filename,_tmp1A1.f2=p->starting_line + base;_tmp1A1;});}}}
# 747
return({struct _tuple30 _tmp1A2;({struct _fat_ptr _tmp1FC=_tag_fat(0,0,0);_tmp1A2.f1=_tmp1FC;}),_tmp1A2.f2=0U;_tmp1A2;});}
# 727
int Cyc_Lex_token(struct Cyc_Lexing_lexbuf*);
# 752
int Cyc_Lex_scan_charconst(struct Cyc_Lexing_lexbuf*);
int Cyc_Lex_strng(struct Cyc_Lexing_lexbuf*);
int Cyc_Lex_strng_next(struct Cyc_Lexing_lexbuf*);
int Cyc_Lex_wstrng(struct Cyc_Lexing_lexbuf*);
int Cyc_Lex_wstrng_next(struct Cyc_Lexing_lexbuf*);
int Cyc_Lex_comment(struct Cyc_Lexing_lexbuf*);
# 763
int Cyc_yylex(struct Cyc_Lexing_lexbuf*lbuf,union Cyc_YYSTYPE*yylval,struct Cyc_Yyltype*yyllocptr){
# 765
int ans=({Cyc_Lex_token(lbuf);});
({int _tmp1FE=({int _tmp1FD=({Cyc_Lexing_lexeme_start(lbuf);});yyllocptr->first_line=_tmp1FD;});Cyc_yylloc.first_line=_tmp1FE;});
({int _tmp200=({int _tmp1FF=({Cyc_Lexing_lexeme_end(lbuf);});yyllocptr->last_line=_tmp1FF;});Cyc_yylloc.last_line=_tmp200;});
{int _tmp118=ans;switch(_tmp118){case 380U: _LL1: _LL2:
 goto _LL4;case 378U: _LL3: _LL4:
 goto _LL6;case 379U: _LL5: _LL6:
 goto _LL8;case 375U: _LL7: _LL8:
 goto _LLA;case 376U: _LL9: _LLA:
 goto _LLC;case 373U: _LLB: _LLC:
 goto _LLE;case 381U: _LLD: _LLE:
({union Cyc_YYSTYPE _tmp201=({union Cyc_YYSTYPE _tmp1A3;(_tmp1A3.String_tok).tag=3U,(_tmp1A3.String_tok).val=Cyc_Lex_token_string;_tmp1A3;});*yylval=_tmp201;});goto _LL0;case 382U: _LLF: _LL10:
 goto _LL12;case 383U: _LL11: _LL12:
({union Cyc_YYSTYPE _tmp202=({union Cyc_YYSTYPE _tmp1A4;(_tmp1A4.QualId_tok).tag=5U,(_tmp1A4.QualId_tok).val=Cyc_Lex_token_qvar;_tmp1A4;});*yylval=_tmp202;});goto _LL0;case 374U: _LL13: _LL14:
({union Cyc_YYSTYPE _tmp203=({union Cyc_YYSTYPE _tmp1A5;(_tmp1A5.Int_tok).tag=1U,(_tmp1A5.Int_tok).val=Cyc_Lex_token_int;_tmp1A5;});*yylval=_tmp203;});goto _LL0;case 377U: _LL15: _LL16:
({union Cyc_YYSTYPE _tmp204=({union Cyc_YYSTYPE _tmp1A6;(_tmp1A6.Char_tok).tag=2U,(_tmp1A6.Char_tok).val=Cyc_Lex_token_char;_tmp1A6;});*yylval=_tmp204;});goto _LL0;default: _LL17: _LL18:
# 781
 goto _LL0;}_LL0:;}
# 783
return ans;}
# 790
const int Cyc_Lex_lex_base[319U]={0,113,119,120,125,126,127,131,- 6,4,12,2,- 3,- 1,- 2,115,- 4,121,- 1,131,- 5,209,217,240,272,132,- 4,- 3,- 2,5,2,133,- 17,138,- 1,351,- 18,6,- 12,- 11,280,- 13,- 10,- 7,- 8,- 9,424,447,295,- 14,154,- 17,7,- 1,- 15,- 16,8,- 1,502,303,575,650,367,- 16,- 74,178,- 54,9,2,- 56,137,30,107,117,31,115,101,377,150,727,770,135,138,32,141,336,840,929,98,1004,1062,110,- 73,- 38,- 44,1137,1212,- 39,- 57,- 58,1287,103,1345,1420,1495,117,94,80,90,104,94,105,122,107,102,106,104,121,113,122,1570,129,130,126,126,119,124,1645,- 19,130,121,1730,1805,140,147,150,1890,265,151,148,137,150,161,175,162,1965,167,169,2050,170,165,184,202,190,185,2125,- 21,200,201,203,212,196,2210,243,254,205,235,258,251,255,2285,275,276,271,279,2370,282,283,277,284,280,296,284,298,2445,294,292,303,304,2530,302,305,2605,- 20,333,334,338,341,326,2690,328,2765,330,335,353,345,341,352,361,352,361,2850,346,350,365,353,2925,- 15,- 30,409,- 42,- 27,421,- 29,- 47,- 41,- 50,425,- 51,3000,3029,518,- 26,500,410,411,- 25,403,737,3039,3069,3103,3143,502,429,3213,3251,724,430,431,- 22,423,434,- 23,426,725,451,452,- 24,445,726,456,482,- 23,492,- 55,582,- 36,- 54,10,438,3183,- 49,- 32,- 34,- 48,- 31,- 33,- 35,1,3291,2,446,809,470,490,492,494,495,496,540,541,542,3364,3448,- 71,- 65,- 64,- 63,- 62,- 61,- 60,- 59,- 66,- 69,- 70,831,543,- 67,- 68,- 72,- 43,- 40,- 37,593,- 52,11,- 28,602};
const int Cyc_Lex_lex_backtrk[319U]={- 1,- 1,- 1,- 1,- 1,- 1,- 1,6,- 1,5,3,4,- 1,- 1,- 1,2,- 1,2,- 1,5,- 1,2,- 1,2,2,2,- 1,- 1,- 1,1,3,15,- 1,15,- 1,18,- 1,1,- 1,- 1,13,- 1,- 1,- 1,- 1,- 1,- 1,14,13,- 1,15,- 1,1,- 1,- 1,- 1,14,- 1,17,12,- 1,13,12,- 1,- 1,52,- 1,53,73,- 1,73,73,73,73,73,73,73,73,73,22,24,73,73,73,73,73,0,0,73,73,73,73,- 1,- 1,- 1,2,0,- 1,- 1,- 1,0,- 1,- 1,1,1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,45,- 1,- 1,44,- 1,- 1,- 1,- 1,- 1,- 1,25,24,- 1,- 1,24,24,24,- 1,24,- 1,25,25,22,23,22,22,- 1,21,21,21,21,- 1,21,22,- 1,22,23,23,23,- 1,23,22,22,22,- 1,22,- 1,- 1,- 1,- 1,53,- 1,25,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,51,- 1,52};
const int Cyc_Lex_lex_default[319U]={64,54,26,31,26,15,7,7,0,- 1,- 1,- 1,0,0,0,25,0,25,0,- 1,0,- 1,- 1,- 1,- 1,25,0,0,0,- 1,- 1,50,0,50,0,- 1,0,- 1,0,0,- 1,0,0,0,0,0,- 1,- 1,- 1,0,50,0,- 1,0,0,0,- 1,0,- 1,- 1,- 1,- 1,- 1,0,0,- 1,0,- 1,- 1,0,314,- 1,- 1,279,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,- 1,- 1,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,- 1,- 1,- 1,- 1,- 1,- 1,128,0,- 1,- 1,36,39,- 1,- 1,- 1,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,45,- 1,- 1,- 1,- 1,- 1,- 1,156,0,- 1,- 1,- 1,- 1,- 1,51,- 1,- 1,- 1,- 1,- 1,- 1,- 1,42,- 1,- 1,- 1,- 1,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,43,- 1,- 1,- 1,- 1,20,- 1,- 1,193,0,- 1,- 1,- 1,- 1,- 1,63,- 1,38,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,44,- 1,- 1,- 1,- 1,217,0,0,- 1,0,0,- 1,0,0,0,0,- 1,0,- 1,- 1,- 1,0,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,0,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,0,- 1,0,266,0,0,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,0,0,0,0,- 1,- 1,0,0,0,0,0,0,314,0,- 1,0,- 1};
const int Cyc_Lex_lex_trans[3705U]={0,0,0,0,0,0,0,0,0,65,66,65,65,67,8,14,14,14,63,268,268,315,0,0,0,0,0,0,0,0,0,0,65,68,69,70,13,71,72,73,310,309,74,75,13,76,77,78,79,80,80,80,80,80,80,80,80,80,81,14,82,83,84,317,85,86,86,86,86,86,86,86,86,86,86,86,87,86,86,86,86,86,86,86,86,86,86,86,86,86,86,313,278,221,88,89,90,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,55,91,- 1,56,- 1,27,28,32,26,52,33,27,28,16,8,29,17,9,- 1,- 1,- 1,- 1,311,273,315,51,- 1,316,27,57,53,34,- 1,- 1,27,276,97,18,102,274,275,- 1,14,- 1,- 1,312,10,14,93,- 1,- 1,11,102,277,200,- 1,21,21,21,21,21,21,21,21,318,- 1,318,318,194,265,226,14,157,227,266,222,223,224,30,218,219,202,58,137,- 1,114,280,318,267,35,- 1,203,163,115,116,19,121,117,164,118,14,- 1,- 1,122,119,14,14,- 1,120,133,14,94,123,132,125,126,127,130,14,131,134,124,14,- 1,14,135,14,136,22,149,146,141,142,92,24,24,24,24,24,24,24,24,23,23,23,23,23,23,23,23,23,23,143,144,145,147,148,150,151,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,152,153,154,155,158,159,160,23,23,23,23,23,23,161,162,171,23,23,23,23,23,23,12,12,12,12,12,12,12,12,48,48,48,48,48,48,48,48,167,23,23,23,23,23,23,49,49,49,49,49,49,49,49,62,62,62,62,62,62,62,62,176,138,14,165,166,37,177,168,169,139,51,170,- 1,172,- 1,173,- 1,36,- 1,140,174,175,- 1,20,12,178,38,185,- 1,- 1,- 1,39,179,186,- 1,- 1,180,181,182,183,40,40,40,40,40,40,40,40,184,190,187,- 1,188,189,191,41,41,41,41,41,41,41,41,41,270,192,271,271,271,271,271,271,271,271,271,271,195,196,106,107,197,198,108,199,42,201,212,109,110,12,26,111,204,112,20,205,206,207,208,113,209,210,8,211,213,214,43,215,44,216,45,220,46,47,47,47,47,47,47,47,47,47,47,225,228,272,308,235,236,236,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,252,249,250,250,304,253,253,47,47,47,47,47,47,235,236,236,47,47,47,47,47,47,257,258,303,258,302,262,301,300,299,39,252,249,250,250,42,253,253,47,47,47,47,47,47,59,59,59,59,59,59,59,59,263,257,258,238,258,238,262,38,239,239,239,239,239,239,239,239,239,239,237,263,254,298,297,296,305,0,0,236,0,253,0,0,263,0,268,0,45,269,0,0,0,14,12,0,0,315,26,0,316,0,237,263,254,318,20,318,318,0,8,236,43,253,44,0,60,61,61,61,61,61,61,61,61,61,61,0,318,0,0,0,0,0,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,0,0,0,0,0,0,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,0,0,0,0,0,0,0,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,0,0,0,0,0,0,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,229,0,241,241,241,241,241,241,241,241,242,242,239,239,239,239,239,239,239,239,239,239,0,231,232,0,0,251,259,264,243,0,0,0,0,0,250,258,263,244,0,0,245,229,0,230,230,230,230,230,230,230,230,230,230,231,232,0,0,251,259,264,243,0,0,- 1,231,232,250,258,263,244,0,233,245,305,- 1,0,0,0,0,0,234,0,306,306,306,306,306,306,306,306,0,0,0,0,0,305,231,232,0,0,0,0,0,233,307,307,307,307,307,307,307,307,234,100,100,100,100,100,100,100,100,100,100,101,0,0,0,0,0,0,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,0,0,0,0,100,0,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,98,0,0,0,0,99,0,0,0,0,0,0,0,0,100,100,100,100,100,100,100,100,100,100,101,0,0,0,0,0,0,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,0,0,0,0,100,0,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,96,96,96,96,96,96,96,96,96,96,0,0,0,0,0,0,0,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,0,0,0,0,96,0,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,0,0,0,0,95,0,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,0,0,0,0,0,0,0,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,0,0,0,0,95,0,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,96,96,96,96,96,96,96,96,96,96,0,0,0,0,0,0,0,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,0,0,0,0,96,0,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,100,100,100,100,100,100,100,100,100,100,101,0,0,0,0,0,0,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,0,0,0,0,100,0,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,0,0,0,0,104,0,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,105,0,0,0,0,0,0,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,0,0,0,0,103,0,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,104,104,104,104,104,104,104,104,104,104,0,0,0,0,0,0,0,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,0,0,0,0,104,0,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,0,129,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,240,240,240,240,240,240,240,240,240,240,0,0,0,0,0,0,0,0,0,0,0,231,232,0,0,0,0,229,232,230,230,230,230,230,230,230,230,230,230,239,239,239,239,239,239,239,239,239,239,0,231,232,0,231,232,0,0,233,- 1,0,232,232,0,0,0,0,234,232,0,240,240,240,240,240,240,240,240,240,240,0,0,0,231,232,0,0,0,0,0,233,231,232,0,232,0,0,0,232,234,232,0,229,0,241,241,241,241,241,241,241,241,242,242,0,0,0,0,0,0,0,0,0,231,232,231,232,0,0,0,232,0,260,0,- 1,0,0,0,0,0,0,261,229,0,242,242,242,242,242,242,242,242,242,242,0,0,0,231,232,0,0,0,0,0,260,231,232,0,0,0,0,0,255,261,0,0,0,0,0,0,0,256,0,0,271,271,271,271,271,271,271,271,271,271,0,0,0,231,232,0,0,0,0,0,255,231,232,0,0,0,0,0,232,256,246,246,246,246,246,246,246,246,246,246,0,0,0,0,0,0,0,246,246,246,246,246,246,231,232,0,0,0,0,0,232,0,0,0,0,0,0,0,246,246,246,246,246,246,246,246,246,246,0,246,246,246,246,246,246,246,246,246,246,246,246,0,0,0,281,0,247,0,0,282,0,0,0,0,0,248,0,0,283,283,283,283,283,283,283,283,0,246,246,246,246,246,246,284,0,0,0,0,247,0,0,0,0,0,0,0,0,248,0,0,0,0,0,0,0,0,0,0,0,0,0,0,285,0,0,0,0,286,287,0,0,0,288,0,0,0,0,0,0,0,289,0,0,0,290,0,291,0,292,0,293,294,294,294,294,294,294,294,294,294,294,0,0,0,0,0,0,0,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,0,0,0,0,0,0,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,295,0,0,0,0,0,0,0,0,294,294,294,294,294,294,294,294,294,294,0,0,0,0,0,0,0,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,0,0,0,0,0,0,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const int Cyc_Lex_lex_check[3705U]={- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,0,9,29,37,52,56,67,269,316,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,0,30,0,0,0,279,281,0,0,11,0,0,0,0,0,0,0,0,0,0,0,0,0,0,10,0,0,0,68,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,71,74,83,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,15,1,73,2,2,3,17,2,3,4,4,5,6,4,5,6,7,25,31,7,72,76,70,33,15,70,2,1,2,3,17,73,4,75,88,5,101,76,76,50,19,25,31,72,6,19,91,33,7,6,105,75,107,7,19,19,19,19,19,19,19,19,65,50,65,65,108,78,81,19,110,81,78,82,82,82,4,84,84,106,1,111,15,113,73,65,78,3,17,106,109,114,115,5,112,116,109,117,19,25,31,112,118,19,19,33,119,121,19,91,122,123,124,125,126,129,19,130,133,122,19,50,19,134,19,135,19,138,139,140,141,0,21,21,21,21,21,21,21,21,22,22,22,22,22,22,22,22,22,22,142,143,144,146,147,149,150,22,22,22,22,22,22,23,23,23,23,23,23,23,23,23,23,151,152,153,154,157,158,159,23,23,23,23,23,23,160,161,165,22,22,22,22,22,22,24,24,24,24,24,24,24,24,40,40,40,40,40,40,40,40,166,23,23,23,23,23,23,48,48,48,48,48,48,48,48,59,59,59,59,59,59,59,59,163,137,35,164,164,35,163,167,168,137,1,169,15,171,73,172,2,3,17,137,173,174,4,5,6,177,35,176,7,25,31,35,178,176,70,33,179,180,181,182,35,35,35,35,35,35,35,35,183,185,186,50,187,188,190,35,62,62,62,62,62,62,62,62,77,191,77,77,77,77,77,77,77,77,77,77,194,195,85,85,196,197,85,198,35,200,202,85,85,35,35,85,203,85,35,204,205,206,207,85,208,209,35,210,212,213,35,214,35,215,35,219,35,46,46,46,46,46,46,46,46,46,46,222,227,270,282,234,235,237,46,46,46,46,46,46,47,47,47,47,47,47,47,47,47,47,244,248,249,251,284,252,254,47,47,47,47,47,47,234,235,237,46,46,46,46,46,46,256,257,285,259,286,261,287,288,289,58,244,248,249,251,58,252,254,47,47,47,47,47,47,58,58,58,58,58,58,58,58,262,256,257,231,259,231,261,58,231,231,231,231,231,231,231,231,231,231,233,264,243,290,291,292,307,- 1,- 1,233,- 1,243,- 1,- 1,262,- 1,266,- 1,58,266,- 1,- 1,- 1,58,58,- 1,- 1,314,58,- 1,314,- 1,233,264,243,318,58,318,318,- 1,58,233,58,243,58,- 1,58,60,60,60,60,60,60,60,60,60,60,- 1,318,- 1,- 1,- 1,- 1,- 1,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,- 1,- 1,- 1,- 1,- 1,- 1,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,60,61,61,61,61,61,61,61,61,61,61,- 1,- 1,- 1,- 1,- 1,- 1,- 1,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,- 1,- 1,- 1,- 1,- 1,- 1,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,61,79,- 1,79,79,79,79,79,79,79,79,79,79,238,238,238,238,238,238,238,238,238,238,- 1,79,79,- 1,- 1,247,255,260,79,- 1,- 1,- 1,- 1,- 1,247,255,260,79,- 1,- 1,79,80,- 1,80,80,80,80,80,80,80,80,80,80,79,79,- 1,- 1,247,255,260,79,- 1,- 1,266,80,80,247,255,260,79,- 1,80,79,283,314,- 1,- 1,- 1,- 1,- 1,80,- 1,283,283,283,283,283,283,283,283,- 1,- 1,- 1,- 1,- 1,306,80,80,- 1,- 1,- 1,- 1,- 1,80,306,306,306,306,306,306,306,306,80,86,86,86,86,86,86,86,86,86,86,86,- 1,- 1,- 1,- 1,- 1,- 1,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,- 1,- 1,- 1,- 1,86,- 1,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,86,87,- 1,- 1,- 1,- 1,87,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,87,87,87,87,87,87,87,87,87,87,87,- 1,- 1,- 1,- 1,- 1,- 1,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,- 1,- 1,- 1,- 1,87,- 1,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,87,89,89,89,89,89,89,89,89,89,89,- 1,- 1,- 1,- 1,- 1,- 1,- 1,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,- 1,- 1,- 1,- 1,89,- 1,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,89,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,- 1,- 1,- 1,- 1,90,- 1,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,90,95,95,95,95,95,95,95,95,95,95,- 1,- 1,- 1,- 1,- 1,- 1,- 1,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,- 1,- 1,- 1,- 1,95,- 1,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,95,96,96,96,96,96,96,96,96,96,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,- 1,- 1,- 1,- 1,96,- 1,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,96,100,100,100,100,100,100,100,100,100,100,100,- 1,- 1,- 1,- 1,- 1,- 1,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,- 1,- 1,- 1,- 1,100,- 1,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,- 1,- 1,- 1,- 1,102,- 1,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,102,103,103,103,103,103,103,103,103,103,103,103,- 1,- 1,- 1,- 1,- 1,- 1,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,- 1,- 1,- 1,- 1,103,- 1,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,103,104,104,104,104,104,104,104,104,104,104,- 1,- 1,- 1,- 1,- 1,- 1,- 1,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,- 1,- 1,- 1,- 1,104,- 1,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,104,120,120,120,120,120,120,120,120,120,120,- 1,- 1,- 1,- 1,- 1,- 1,- 1,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,- 1,- 1,- 1,- 1,120,- 1,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,120,127,127,127,127,127,127,127,127,127,127,- 1,- 1,- 1,- 1,- 1,- 1,- 1,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,- 1,- 1,- 1,- 1,127,- 1,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,131,131,131,131,131,131,131,131,131,131,- 1,- 1,- 1,- 1,- 1,- 1,- 1,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,- 1,- 1,- 1,- 1,131,120,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,131,132,132,132,132,132,132,132,132,132,132,- 1,- 1,- 1,- 1,- 1,- 1,- 1,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,- 1,- 1,- 1,- 1,132,127,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,132,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,136,136,136,136,136,136,136,136,136,136,- 1,- 1,- 1,- 1,- 1,- 1,- 1,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,- 1,- 1,- 1,- 1,136,131,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,136,145,145,145,145,145,145,145,145,145,145,- 1,- 1,- 1,- 1,- 1,- 1,- 1,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,- 1,- 1,- 1,- 1,145,132,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,145,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,148,148,148,148,148,148,148,148,148,148,- 1,- 1,- 1,- 1,- 1,- 1,- 1,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,- 1,- 1,- 1,- 1,148,136,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,148,155,155,155,155,155,155,155,155,155,155,- 1,- 1,- 1,- 1,- 1,- 1,- 1,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,- 1,- 1,- 1,- 1,155,145,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,155,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,162,162,162,162,162,162,162,162,162,162,- 1,- 1,- 1,- 1,- 1,- 1,- 1,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,- 1,- 1,- 1,- 1,162,148,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,162,170,170,170,170,170,170,170,170,170,170,- 1,- 1,- 1,- 1,- 1,- 1,- 1,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,- 1,- 1,- 1,- 1,170,155,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,170,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,175,175,175,175,175,175,175,175,175,175,- 1,- 1,- 1,- 1,- 1,- 1,- 1,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,- 1,- 1,- 1,- 1,175,162,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,175,184,184,184,184,184,184,184,184,184,184,- 1,- 1,- 1,- 1,- 1,- 1,- 1,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,- 1,- 1,- 1,- 1,184,170,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,184,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,189,189,189,189,189,189,189,189,189,189,- 1,- 1,- 1,- 1,- 1,- 1,- 1,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,- 1,- 1,- 1,- 1,189,175,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,189,192,192,192,192,192,192,192,192,192,192,- 1,- 1,- 1,- 1,- 1,- 1,- 1,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,- 1,- 1,- 1,- 1,192,184,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,192,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,199,199,199,199,199,199,199,199,199,199,- 1,- 1,- 1,- 1,- 1,- 1,- 1,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,- 1,- 1,- 1,- 1,199,189,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,199,201,201,201,201,201,201,201,201,201,201,- 1,- 1,- 1,- 1,- 1,- 1,- 1,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,- 1,- 1,- 1,- 1,201,192,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,201,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,211,211,211,211,211,211,211,211,211,211,- 1,- 1,- 1,- 1,- 1,- 1,- 1,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,- 1,- 1,- 1,- 1,211,199,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,211,216,216,216,216,216,216,216,216,216,216,- 1,- 1,- 1,- 1,- 1,- 1,- 1,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,- 1,- 1,- 1,- 1,216,201,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,216,229,229,229,229,229,229,229,229,229,229,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,229,229,- 1,- 1,- 1,- 1,230,229,230,230,230,230,230,230,230,230,230,230,239,239,239,239,239,239,239,239,239,239,- 1,230,230,- 1,229,229,- 1,- 1,230,211,- 1,229,239,- 1,- 1,- 1,- 1,230,239,- 1,240,240,240,240,240,240,240,240,240,240,- 1,- 1,- 1,230,230,- 1,- 1,- 1,- 1,- 1,230,240,240,- 1,239,- 1,- 1,- 1,240,230,239,- 1,241,- 1,241,241,241,241,241,241,241,241,241,241,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,240,240,241,241,- 1,- 1,- 1,240,- 1,241,- 1,216,- 1,- 1,- 1,- 1,- 1,- 1,241,242,- 1,242,242,242,242,242,242,242,242,242,242,- 1,- 1,- 1,241,241,- 1,- 1,- 1,- 1,- 1,241,242,242,- 1,- 1,- 1,- 1,- 1,242,241,- 1,- 1,- 1,- 1,- 1,- 1,- 1,242,- 1,- 1,271,271,271,271,271,271,271,271,271,271,- 1,- 1,- 1,242,242,- 1,- 1,- 1,- 1,- 1,242,271,271,- 1,- 1,- 1,- 1,- 1,271,242,245,245,245,245,245,245,245,245,245,245,- 1,- 1,- 1,- 1,- 1,- 1,- 1,245,245,245,245,245,245,271,271,- 1,- 1,- 1,- 1,- 1,271,- 1,- 1,- 1,- 1,- 1,- 1,- 1,246,246,246,246,246,246,246,246,246,246,- 1,245,245,245,245,245,245,246,246,246,246,246,246,- 1,- 1,- 1,280,- 1,246,- 1,- 1,280,- 1,- 1,- 1,- 1,- 1,246,- 1,- 1,280,280,280,280,280,280,280,280,- 1,246,246,246,246,246,246,280,- 1,- 1,- 1,- 1,246,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,246,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,280,- 1,- 1,- 1,- 1,280,280,- 1,- 1,- 1,280,- 1,- 1,- 1,- 1,- 1,- 1,- 1,280,- 1,- 1,- 1,280,- 1,280,- 1,280,- 1,280,293,293,293,293,293,293,293,293,293,293,- 1,- 1,- 1,- 1,- 1,- 1,- 1,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,- 1,- 1,- 1,- 1,- 1,- 1,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,293,294,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,294,294,294,294,294,294,294,294,294,294,- 1,- 1,- 1,- 1,- 1,- 1,- 1,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,- 1,- 1,- 1,- 1,- 1,- 1,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,294,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1};
int Cyc_Lex_lex_engine(int start_state,struct Cyc_Lexing_lexbuf*lbuf){
# 797
int state;int base;int backtrk;
int c;
state=start_state;
# 801
if(state >= 0){
({int _tmp205=lbuf->lex_start_pos=lbuf->lex_curr_pos;lbuf->lex_last_pos=_tmp205;});
lbuf->lex_last_action=- 1;}else{
# 805
state=(- state)- 1;}
# 807
while(1){
base=*((const int*)_check_known_subscript_notnull(Cyc_Lex_lex_base,319U,sizeof(int),state));
if(base < 0)return(- base)- 1;backtrk=Cyc_Lex_lex_backtrk[state];
# 811
if(backtrk >= 0){
lbuf->lex_last_pos=lbuf->lex_curr_pos;
lbuf->lex_last_action=backtrk;}
# 811
if(lbuf->lex_curr_pos >= lbuf->lex_buffer_len){
# 816
if(!lbuf->lex_eof_reached)
return(- state)- 1;else{
# 819
c=256;}}else{
# 821
c=(int)*((char*)_check_fat_subscript(lbuf->lex_buffer,sizeof(char),lbuf->lex_curr_pos ++));
if(c == -1)c=256;}
# 824
if(*((const int*)_check_known_subscript_notnull(Cyc_Lex_lex_check,3705U,sizeof(int),base + c))== state)
state=*((const int*)_check_known_subscript_notnull(Cyc_Lex_lex_trans,3705U,sizeof(int),base + c));else{
# 827
state=Cyc_Lex_lex_default[state];}
if(state < 0){
lbuf->lex_curr_pos=lbuf->lex_last_pos;
if(lbuf->lex_last_action == - 1)
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp11B=_cycalloc(sizeof(*_tmp11B));_tmp11B->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp206=({const char*_tmp11A="empty token";_tag_fat(_tmp11A,sizeof(char),12U);});_tmp11B->f1=_tmp206;});_tmp11B;}));else{
# 833
return lbuf->lex_last_action;}}else{
# 836
if(c == 256)lbuf->lex_eof_reached=0;}}}
# 840
int Cyc_Lex_token_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_Lex_lex_engine(lexstate,lexbuf);});
{int _tmp11D=lexstate;switch(_tmp11D){case 0U: _LL1: _LL2:
# 801 "lex.cyl"
 return(int)({Cyc_Lex_process_id(lexbuf);});case 1U: _LL3: _LL4:
# 804 "lex.cyl"
 return(int)({Cyc_Lex_process_qual_id(lexbuf);});case 2U: _LL5: _LL6:
# 807 "lex.cyl"
 Cyc_Lex_token_string=*({struct _fat_ptr*(*_tmp11E)(int symbol_num)=Cyc_Lex_get_symbol;int _tmp11F=({Cyc_Lex_str_index_lbuf(lexbuf);});_tmp11E(_tmp11F);});
return 380;case 3U: _LL7: _LL8:
# 814 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 347;case 4U: _LL9: _LLA:
# 815 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 334;case 5U: _LLB: _LLC:
# 816 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 346;case 6U: _LLD: _LLE:
# 817 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 345;case 7U: _LLF: _LL10:
# 818 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 348;case 8U: _LL11: _LL12:
# 819 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 344;case 9U: _LL13: _LL14:
# 820 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 331;case 10U: _LL15: _LL16:
# 821 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 332;case 11U: _LL17: _LL18:
# 822 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 333;case 12U: _LL19: _LL1A:
# 823 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 335;case 13U: _LL1B: _LL1C:
# 824 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 336;case 14U: _LL1D: _LL1E:
# 825 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 337;case 15U: _LL1F: _LL20:
# 826 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 338;case 16U: _LL21: _LL22:
# 827 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 339;case 17U: _LL23: _LL24:
# 828 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 342;case 18U: _LL25: _LL26:
# 829 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 340;case 19U: _LL27: _LL28:
# 830 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 341;case 20U: _LL29: _LL2A:
# 831 "lex.cyl"
 -- lexbuf->lex_curr_pos;return 343;case 21U: _LL2B: _LL2C:
# 835 "lex.cyl"
 Cyc_Lex_token_int=({Cyc_Lex_intconst(lexbuf,2,0,16);});
return 374;case 22U: _LL2D: _LL2E:
# 838
 Cyc_Lex_token_int=({Cyc_Lex_intconst(lexbuf,0,0,8);});
return 374;case 23U: _LL2F: _LL30:
# 844 "lex.cyl"
 Cyc_Lex_token_int=({Cyc_Lex_intconst(lexbuf,0,0,10);});
return 374;case 24U: _LL31: _LL32:
# 847
 Cyc_Lex_token_int=({Cyc_Lex_intconst(lexbuf,0,0,10);});
return 374;case 25U: _LL33: _LL34:
# 852 "lex.cyl"
 Cyc_Lex_token_string=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});
return 379;case 26U: _LL35: _LL36:
# 855
 return 356;case 27U: _LL37: _LL38:
# 856 "lex.cyl"
 return 357;case 28U: _LL39: _LL3A:
# 857 "lex.cyl"
 return 354;case 29U: _LL3B: _LL3C:
# 858 "lex.cyl"
 return 355;case 30U: _LL3D: _LL3E:
# 859 "lex.cyl"
 return 350;case 31U: _LL3F: _LL40:
# 860 "lex.cyl"
 return 351;case 32U: _LL41: _LL42:
# 861 "lex.cyl"
 return 363;case 33U: _LL43: _LL44:
# 862 "lex.cyl"
 return 364;case 34U: _LL45: _LL46:
# 863 "lex.cyl"
 return 360;case 35U: _LL47: _LL48:
# 864 "lex.cyl"
 return 361;case 36U: _LL49: _LL4A:
# 865 "lex.cyl"
 return 362;case 37U: _LL4B: _LL4C:
# 866 "lex.cyl"
 return 369;case 38U: _LL4D: _LL4E:
# 867 "lex.cyl"
 return 368;case 39U: _LL4F: _LL50:
# 868 "lex.cyl"
 return 367;case 40U: _LL51: _LL52:
# 869 "lex.cyl"
 return 365;case 41U: _LL53: _LL54:
# 870 "lex.cyl"
 return 366;case 42U: _LL55: _LL56:
# 871 "lex.cyl"
 return 358;case 43U: _LL57: _LL58:
# 872 "lex.cyl"
 return 359;case 44U: _LL59: _LL5A:
# 873 "lex.cyl"
 return 352;case 45U: _LL5B: _LL5C:
# 875
 return 353;case 46U: _LL5D: _LL5E:
# 876 "lex.cyl"
 return 371;case 47U: _LL5F: _LL60:
# 877 "lex.cyl"
 return 349;case 48U: _LL61: _LL62:
# 878 "lex.cyl"
 return 370;case 49U: _LL63: _LL64:
# 879 "lex.cyl"
 return 372;case 50U: _LL65: _LL66:
# 881
 return 316;case 51U: _LL67: _LL68:
# 884 "lex.cyl"
({void(*_tmp120)(struct _fat_ptr line)=Cyc_Lex_process_directive;struct _fat_ptr _tmp121=({Cyc_Lexing_lexeme(lexbuf);});_tmp120(_tmp121);});return({Cyc_Lex_token(lexbuf);});case 52U: _LL69: _LL6A:
# 885 "lex.cyl"
 return({Cyc_Lex_token(lexbuf);});case 53U: _LL6B: _LL6C:
# 886 "lex.cyl"
({Cyc_Lex_inc_linenumber();});return({Cyc_Lex_token(lexbuf);});case 54U: _LL6D: _LL6E:
# 887 "lex.cyl"
 Cyc_Lex_comment_depth=1;
Cyc_Lex_runaway_start=({Cyc_Lexing_lexeme_start(lexbuf);});
({Cyc_Lex_comment(lexbuf);});
return({Cyc_Lex_token(lexbuf);});case 55U: _LL6F: _LL70:
# 893 "lex.cyl"
 Cyc_Lex_string_pos=0;
Cyc_Lex_runaway_start=({Cyc_Lexing_lexeme_start(lexbuf);});
while(({Cyc_Lex_strng(lexbuf);})){
;}
Cyc_Lex_token_string=(struct _fat_ptr)({Cyc_Lex_get_stored_string();});
return 375;case 56U: _LL71: _LL72:
# 901 "lex.cyl"
 Cyc_Lex_string_pos=0;
Cyc_Lex_runaway_start=({Cyc_Lexing_lexeme_start(lexbuf);});
while(({Cyc_Lex_wstrng(lexbuf);})){
;}
Cyc_Lex_token_string=(struct _fat_ptr)({Cyc_Lex_get_stored_string();});
return 376;case 57U: _LL73: _LL74:
# 909 "lex.cyl"
 Cyc_Lex_string_pos=0;
Cyc_Lex_runaway_start=({Cyc_Lexing_lexeme_start(lexbuf);});
while(({Cyc_Lex_scan_charconst(lexbuf);})){
;}
Cyc_Lex_token_string=(struct _fat_ptr)({Cyc_Lex_get_stored_string();});
return 378;case 58U: _LL75: _LL76:
# 916
 Cyc_Lex_token_char='\a';return 377;case 59U: _LL77: _LL78:
# 917 "lex.cyl"
 Cyc_Lex_token_char='\b';return 377;case 60U: _LL79: _LL7A:
# 918 "lex.cyl"
 Cyc_Lex_token_char='\f';return 377;case 61U: _LL7B: _LL7C:
# 919 "lex.cyl"
 Cyc_Lex_token_char='\n';return 377;case 62U: _LL7D: _LL7E:
# 920 "lex.cyl"
 Cyc_Lex_token_char='\r';return 377;case 63U: _LL7F: _LL80:
# 921 "lex.cyl"
 Cyc_Lex_token_char='\t';return 377;case 64U: _LL81: _LL82:
# 922 "lex.cyl"
 Cyc_Lex_token_char='\v';return 377;case 65U: _LL83: _LL84:
# 923 "lex.cyl"
 Cyc_Lex_token_char='\\';return 377;case 66U: _LL85: _LL86:
# 924 "lex.cyl"
 Cyc_Lex_token_char='\'';return 377;case 67U: _LL87: _LL88:
# 925 "lex.cyl"
 Cyc_Lex_token_char='"';return 377;case 68U: _LL89: _LL8A:
# 926 "lex.cyl"
 Cyc_Lex_token_char='?';return 377;case 69U: _LL8B: _LL8C:
# 929 "lex.cyl"
 Cyc_Lex_token_char=({char(*_tmp122)(union Cyc_Absyn_Cnst cnst,struct Cyc_Lexing_lexbuf*lb)=Cyc_Lex_cnst2char;union Cyc_Absyn_Cnst _tmp123=({Cyc_Lex_intconst(lexbuf,2,1,8);});struct Cyc_Lexing_lexbuf*_tmp124=lexbuf;_tmp122(_tmp123,_tmp124);});
return 377;case 70U: _LL8D: _LL8E:
# 934 "lex.cyl"
 Cyc_Lex_token_char=({char(*_tmp125)(union Cyc_Absyn_Cnst cnst,struct Cyc_Lexing_lexbuf*lb)=Cyc_Lex_cnst2char;union Cyc_Absyn_Cnst _tmp126=({Cyc_Lex_intconst(lexbuf,3,1,16);});struct Cyc_Lexing_lexbuf*_tmp127=lexbuf;_tmp125(_tmp126,_tmp127);});
return 377;case 71U: _LL8F: _LL90:
# 938
 Cyc_Lex_token_char=({Cyc_Lexing_lexeme_char(lexbuf,1);});
return 377;case 72U: _LL91: _LL92:
# 942
 return - 1;case 73U: _LL93: _LL94:
# 944
 return(int)({Cyc_Lexing_lexeme_char(lexbuf,0);});default: _LL95: _LL96:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_Lex_token_rec(lexbuf,lexstate);});}_LL0:;}
# 948
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp129=_cycalloc(sizeof(*_tmp129));_tmp129->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp207=({const char*_tmp128="some action didn't return!";_tag_fat(_tmp128,sizeof(char),27U);});_tmp129->f1=_tmp207;});_tmp129;}));}
# 950
int Cyc_Lex_token(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_Lex_token_rec(lexbuf,0);});}
int Cyc_Lex_scan_charconst_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_Lex_lex_engine(lexstate,lexbuf);});
{int _tmp12C=lexstate;switch(_tmp12C){case 0U: _LL1: _LL2:
# 947 "lex.cyl"
 return 0;case 1U: _LL3: _LL4:
# 949
({Cyc_Lex_store_string_char('\a');});return 1;case 2U: _LL5: _LL6:
# 950 "lex.cyl"
({Cyc_Lex_store_string_char('\b');});return 1;case 3U: _LL7: _LL8:
# 951 "lex.cyl"
({Cyc_Lex_store_string_char('\f');});return 1;case 4U: _LL9: _LLA:
# 952 "lex.cyl"
({Cyc_Lex_store_string_char('\n');});return 1;case 5U: _LLB: _LLC:
# 953 "lex.cyl"
({Cyc_Lex_store_string_char('\r');});return 1;case 6U: _LLD: _LLE:
# 954 "lex.cyl"
({Cyc_Lex_store_string_char('\t');});return 1;case 7U: _LLF: _LL10:
# 955 "lex.cyl"
({Cyc_Lex_store_string_char('\v');});return 1;case 8U: _LL11: _LL12:
# 956 "lex.cyl"
({Cyc_Lex_store_string_char('\\');});return 1;case 9U: _LL13: _LL14:
# 957 "lex.cyl"
({Cyc_Lex_store_string_char('\'');});return 1;case 10U: _LL15: _LL16:
# 958 "lex.cyl"
({Cyc_Lex_store_string_char('"');});return 1;case 11U: _LL17: _LL18:
# 959 "lex.cyl"
({Cyc_Lex_store_string_char('?');});return 1;case 12U: _LL19: _LL1A:
# 962 "lex.cyl"
({void(*_tmp12D)(struct _fat_ptr s)=Cyc_Lex_store_string;struct _fat_ptr _tmp12E=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp12D(_tmp12E);});return 1;case 13U: _LL1B: _LL1C:
# 965 "lex.cyl"
({void(*_tmp12F)(struct _fat_ptr s)=Cyc_Lex_store_string;struct _fat_ptr _tmp130=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp12F(_tmp130);});return 1;case 14U: _LL1D: _LL1E:
# 967
({void(*_tmp131)(char c)=Cyc_Lex_store_string_char;char _tmp132=({Cyc_Lexing_lexeme_char(lexbuf,0);});_tmp131(_tmp132);});return 1;case 15U: _LL1F: _LL20:
# 969
({Cyc_Lex_inc_linenumber();});({({struct _fat_ptr _tmp208=({const char*_tmp133="wide character ends in newline";_tag_fat(_tmp133,sizeof(char),31U);});Cyc_Lex_runaway_err(_tmp208,lexbuf);});});return 0;case 16U: _LL21: _LL22:
# 970 "lex.cyl"
({({struct _fat_ptr _tmp209=({const char*_tmp134="unterminated wide character";_tag_fat(_tmp134,sizeof(char),28U);});Cyc_Lex_runaway_err(_tmp209,lexbuf);});});return 0;case 17U: _LL23: _LL24:
# 971 "lex.cyl"
({({struct _fat_ptr _tmp20A=({const char*_tmp135="bad character following backslash in wide character";_tag_fat(_tmp135,sizeof(char),52U);});Cyc_Lex_err(_tmp20A,lexbuf);});});return 1;default: _LL25: _LL26:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_Lex_scan_charconst_rec(lexbuf,lexstate);});}_LL0:;}
# 975
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp137=_cycalloc(sizeof(*_tmp137));_tmp137->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp20B=({const char*_tmp136="some action didn't return!";_tag_fat(_tmp136,sizeof(char),27U);});_tmp137->f1=_tmp20B;});_tmp137;}));}
# 977
int Cyc_Lex_scan_charconst(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_Lex_scan_charconst_rec(lexbuf,1);});}
int Cyc_Lex_strng_next_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_Lex_lex_engine(lexstate,lexbuf);});
{int _tmp13A=lexstate;switch(_tmp13A){case 0U: _LL1: _LL2:
# 976 "lex.cyl"
 return 1;case 1U: _LL3: _LL4:
# 977 "lex.cyl"
({Cyc_Lex_inc_linenumber();});return({Cyc_Lex_strng_next(lexbuf);});case 2U: _LL5: _LL6:
# 978 "lex.cyl"
 return({Cyc_Lex_strng_next(lexbuf);});case 3U: _LL7: _LL8:
# 980
 lexbuf->lex_curr_pos -=1;return 0;default: _LL9: _LLA:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_Lex_strng_next_rec(lexbuf,lexstate);});}_LL0:;}
# 984
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp13C=_cycalloc(sizeof(*_tmp13C));_tmp13C->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp20C=({const char*_tmp13B="some action didn't return!";_tag_fat(_tmp13B,sizeof(char),27U);});_tmp13C->f1=_tmp20C;});_tmp13C;}));}
# 986
int Cyc_Lex_strng_next(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_Lex_strng_next_rec(lexbuf,2);});}
int Cyc_Lex_strng_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_Lex_lex_engine(lexstate,lexbuf);});
{int _tmp13F=lexstate;switch(_tmp13F){case 0U: _LL1: _LL2:
# 984 "lex.cyl"
 return({Cyc_Lex_strng_next(lexbuf);});case 1U: _LL3: _LL4:
# 985 "lex.cyl"
({Cyc_Lex_inc_linenumber();});return 1;case 2U: _LL5: _LL6:
# 986 "lex.cyl"
({Cyc_Lex_store_string_char('\a');});return 1;case 3U: _LL7: _LL8:
# 987 "lex.cyl"
({Cyc_Lex_store_string_char('\b');});return 1;case 4U: _LL9: _LLA:
# 988 "lex.cyl"
({Cyc_Lex_store_string_char('\f');});return 1;case 5U: _LLB: _LLC:
# 989 "lex.cyl"
 if(Cyc_Lex_expand_specials){
({Cyc_Lex_store_string_char('\\');});
({Cyc_Lex_store_string_char('n');});}else{
# 993
({Cyc_Lex_store_string_char('\n');});}
return 1;case 6U: _LLD: _LLE:
# 996 "lex.cyl"
({Cyc_Lex_store_string_char('\r');});return 1;case 7U: _LLF: _LL10:
# 997 "lex.cyl"
 if(Cyc_Lex_expand_specials){
({Cyc_Lex_store_string_char('\\');});
({Cyc_Lex_store_string_char('t');});}else{
# 1001
({Cyc_Lex_store_string_char('\t');});}
return 1;case 8U: _LL11: _LL12:
# 1003 "lex.cyl"
({Cyc_Lex_store_string_char('\v');});return 1;case 9U: _LL13: _LL14:
# 1004 "lex.cyl"
({Cyc_Lex_store_string_char('\\');});return 1;case 10U: _LL15: _LL16:
# 1005 "lex.cyl"
({Cyc_Lex_store_string_char('\'');});return 1;case 11U: _LL17: _LL18:
# 1006 "lex.cyl"
({Cyc_Lex_store_string_char('"');});return 1;case 12U: _LL19: _LL1A:
# 1007 "lex.cyl"
({Cyc_Lex_store_string_char('?');});return 1;case 13U: _LL1B: _LL1C:
# 1010 "lex.cyl"
({void(*_tmp140)(char c)=Cyc_Lex_store_string_char;char _tmp141=({char(*_tmp142)(union Cyc_Absyn_Cnst cnst,struct Cyc_Lexing_lexbuf*lb)=Cyc_Lex_cnst2char;union Cyc_Absyn_Cnst _tmp143=({Cyc_Lex_intconst(lexbuf,1,0,8);});struct Cyc_Lexing_lexbuf*_tmp144=lexbuf;_tmp142(_tmp143,_tmp144);});_tmp140(_tmp141);});
return 1;case 14U: _LL1D: _LL1E:
# 1015 "lex.cyl"
({void(*_tmp145)(char c)=Cyc_Lex_store_string_char;char _tmp146=({char(*_tmp147)(union Cyc_Absyn_Cnst cnst,struct Cyc_Lexing_lexbuf*lb)=Cyc_Lex_cnst2char;union Cyc_Absyn_Cnst _tmp148=({Cyc_Lex_intconst(lexbuf,2,0,16);});struct Cyc_Lexing_lexbuf*_tmp149=lexbuf;_tmp147(_tmp148,_tmp149);});_tmp145(_tmp146);});
return 1;case 15U: _LL1F: _LL20:
# 1019
({void(*_tmp14A)(struct _fat_ptr s)=Cyc_Lex_store_string;struct _fat_ptr _tmp14B=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp14A(_tmp14B);});
return 1;case 16U: _LL21: _LL22:
# 1021 "lex.cyl"
({Cyc_Lex_inc_linenumber();});
({({struct _fat_ptr _tmp20D=({const char*_tmp14C="string ends in newline";_tag_fat(_tmp14C,sizeof(char),23U);});Cyc_Lex_runaway_err(_tmp20D,lexbuf);});});
return 0;case 17U: _LL23: _LL24:
# 1025 "lex.cyl"
({({struct _fat_ptr _tmp20E=({const char*_tmp14D="unterminated string";_tag_fat(_tmp14D,sizeof(char),20U);});Cyc_Lex_runaway_err(_tmp20E,lexbuf);});});
return 0;case 18U: _LL25: _LL26:
# 1028 "lex.cyl"
({({struct _fat_ptr _tmp20F=({const char*_tmp14E="bad character following backslash in string";_tag_fat(_tmp14E,sizeof(char),44U);});Cyc_Lex_err(_tmp20F,lexbuf);});});
return 1;default: _LL27: _LL28:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_Lex_strng_rec(lexbuf,lexstate);});}_LL0:;}
# 1033
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp150=_cycalloc(sizeof(*_tmp150));_tmp150->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp210=({const char*_tmp14F="some action didn't return!";_tag_fat(_tmp14F,sizeof(char),27U);});_tmp150->f1=_tmp210;});_tmp150;}));}
# 1035
int Cyc_Lex_strng(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_Lex_strng_rec(lexbuf,3);});}
int Cyc_Lex_wstrng_next_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_Lex_lex_engine(lexstate,lexbuf);});
{int _tmp153=lexstate;switch(_tmp153){case 0U: _LL1: _LL2:
# 1038 "lex.cyl"
({Cyc_Lex_store_string(({const char*_tmp154="\" L\"";_tag_fat(_tmp154,sizeof(char),5U);}));});return 1;case 1U: _LL3: _LL4:
# 1039 "lex.cyl"
({Cyc_Lex_inc_linenumber();});return({Cyc_Lex_wstrng_next(lexbuf);});case 2U: _LL5: _LL6:
# 1040 "lex.cyl"
 return({Cyc_Lex_wstrng_next(lexbuf);});case 3U: _LL7: _LL8:
# 1042
 lexbuf->lex_curr_pos -=1;return 0;default: _LL9: _LLA:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_Lex_wstrng_next_rec(lexbuf,lexstate);});}_LL0:;}
# 1046
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp156=_cycalloc(sizeof(*_tmp156));_tmp156->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp211=({const char*_tmp155="some action didn't return!";_tag_fat(_tmp155,sizeof(char),27U);});_tmp156->f1=_tmp211;});_tmp156;}));}
# 1048
int Cyc_Lex_wstrng_next(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_Lex_wstrng_next_rec(lexbuf,4);});}
int Cyc_Lex_wstrng_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_Lex_lex_engine(lexstate,lexbuf);});
{int _tmp159=lexstate;switch(_tmp159){case 0U: _LL1: _LL2:
# 1045 "lex.cyl"
 return({Cyc_Lex_wstrng_next(lexbuf);});case 1U: _LL3: _LL4:
# 1047
({Cyc_Lex_store_string_char('\\');});
({void(*_tmp15A)(char c)=Cyc_Lex_store_string_char;char _tmp15B=({Cyc_Lexing_lexeme_char(lexbuf,1);});_tmp15A(_tmp15B);});
return 1;case 2U: _LL5: _LL6:
# 1053 "lex.cyl"
({void(*_tmp15C)(struct _fat_ptr s)=Cyc_Lex_store_string;struct _fat_ptr _tmp15D=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp15C(_tmp15D);});
return 1;case 3U: _LL7: _LL8:
# 1055 "lex.cyl"
({Cyc_Lex_inc_linenumber();});
({({struct _fat_ptr _tmp212=({const char*_tmp15E="string ends in newline";_tag_fat(_tmp15E,sizeof(char),23U);});Cyc_Lex_runaway_err(_tmp212,lexbuf);});});
return 0;case 4U: _LL9: _LLA:
# 1058 "lex.cyl"
({({struct _fat_ptr _tmp213=({const char*_tmp15F="unterminated string";_tag_fat(_tmp15F,sizeof(char),20U);});Cyc_Lex_runaway_err(_tmp213,lexbuf);});});
return 0;case 5U: _LLB: _LLC:
# 1060 "lex.cyl"
({({struct _fat_ptr _tmp214=({const char*_tmp160="bad character following backslash in string";_tag_fat(_tmp160,sizeof(char),44U);});Cyc_Lex_err(_tmp214,lexbuf);});});
return 1;default: _LLD: _LLE:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_Lex_wstrng_rec(lexbuf,lexstate);});}_LL0:;}
# 1065
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp162=_cycalloc(sizeof(*_tmp162));_tmp162->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp215=({const char*_tmp161="some action didn't return!";_tag_fat(_tmp161,sizeof(char),27U);});_tmp162->f1=_tmp215;});_tmp162;}));}
# 1067
int Cyc_Lex_wstrng(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_Lex_wstrng_rec(lexbuf,5);});}
int Cyc_Lex_comment_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_Lex_lex_engine(lexstate,lexbuf);});
{int _tmp165=lexstate;switch(_tmp165){case 0U: _LL1: _LL2:
# 1063 "lex.cyl"
 ++ Cyc_Lex_comment_depth;return({Cyc_Lex_comment(lexbuf);});case 1U: _LL3: _LL4:
# 1064 "lex.cyl"
 -- Cyc_Lex_comment_depth;
if(Cyc_Lex_comment_depth > 0)
return({Cyc_Lex_comment(lexbuf);});
# 1065
return 0;case 2U: _LL5: _LL6:
# 1069 "lex.cyl"
({({struct _fat_ptr _tmp216=({const char*_tmp166="unterminated comment";_tag_fat(_tmp166,sizeof(char),21U);});Cyc_Lex_runaway_err(_tmp216,lexbuf);});});
return 0;case 3U: _LL7: _LL8:
# 1072 "lex.cyl"
 return({Cyc_Lex_comment(lexbuf);});case 4U: _LL9: _LLA:
# 1073 "lex.cyl"
 return({Cyc_Lex_comment(lexbuf);});case 5U: _LLB: _LLC:
# 1074 "lex.cyl"
({Cyc_Lex_inc_linenumber();});return({Cyc_Lex_comment(lexbuf);});case 6U: _LLD: _LLE:
# 1075 "lex.cyl"
 return({Cyc_Lex_comment(lexbuf);});default: _LLF: _LL10:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_Lex_comment_rec(lexbuf,lexstate);});}_LL0:;}
# 1079
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp168=_cycalloc(sizeof(*_tmp168));_tmp168->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp217=({const char*_tmp167="some action didn't return!";_tag_fat(_tmp167,sizeof(char),27U);});_tmp168->f1=_tmp217;});_tmp168;}));}
# 1081
int Cyc_Lex_comment(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_Lex_comment_rec(lexbuf,6);});}
# 1087 "lex.cyl"
void Cyc_Lex_pos_init(){
Cyc_Lex_linenumber=1;
Cyc_Lex_pos_info=0;}
# 1092
static struct Cyc_Xarray_Xarray*Cyc_Lex_empty_xarray(struct _RegionHandle*id_rgn,int dummy){
struct Cyc_Xarray_Xarray*symbols=({({struct Cyc_Xarray_Xarray*(*_tmp21A)(struct _RegionHandle*,int,struct _fat_ptr*)=({
struct Cyc_Xarray_Xarray*(*_tmp16D)(struct _RegionHandle*,int,struct _fat_ptr*)=(struct Cyc_Xarray_Xarray*(*)(struct _RegionHandle*,int,struct _fat_ptr*))Cyc_Xarray_rcreate;_tmp16D;});struct _RegionHandle*_tmp219=id_rgn;_tmp21A(_tmp219,101,({struct _fat_ptr*_tmp16F=_cycalloc(sizeof(*_tmp16F));({struct _fat_ptr _tmp218=(struct _fat_ptr)({const char*_tmp16E="";_tag_fat(_tmp16E,sizeof(char),1U);});*_tmp16F=_tmp218;});_tmp16F;}));});});
# 1096
({({void(*_tmp21B)(struct Cyc_Xarray_Xarray*,struct _fat_ptr*)=({void(*_tmp16C)(struct Cyc_Xarray_Xarray*,struct _fat_ptr*)=(void(*)(struct Cyc_Xarray_Xarray*,struct _fat_ptr*))Cyc_Xarray_add;_tmp16C;});_tmp21B(symbols,& Cyc_Lex_bogus_string);});});
return symbols;}
# 1100
void Cyc_Lex_lex_init(int include_cyclone_keywords){
# 1102
Cyc_Lex_in_extern_c=0;{
struct Cyc_List_List*x=0;
({struct Cyc_List_List*_tmp171=Cyc_Lex_prev_extern_c;struct Cyc_List_List*_tmp172=x;Cyc_Lex_prev_extern_c=_tmp172;x=_tmp171;});
while(x != 0){
struct Cyc_List_List*t=x->tl;
({({void(*_tmp21C)(struct Cyc_List_List*ptr)=({void(*_tmp173)(struct Cyc_List_List*ptr)=(void(*)(struct Cyc_List_List*ptr))Cyc_Core_ufree;_tmp173;});_tmp21C(x);});});
x=t;}
# 1111
if(Cyc_Lex_ids_trie != 0){
struct Cyc_Lex_DynTrieSymbols*idt=0;
({struct Cyc_Lex_DynTrieSymbols*_tmp174=idt;struct Cyc_Lex_DynTrieSymbols*_tmp175=Cyc_Lex_ids_trie;idt=_tmp175;Cyc_Lex_ids_trie=_tmp174;});{
struct Cyc_Lex_DynTrieSymbols _stmttmpD=*((struct Cyc_Lex_DynTrieSymbols*)_check_null(idt));struct Cyc_Core_DynamicRegion*_tmp176;_LL1: _tmp176=_stmttmpD.dyn;_LL2: {struct Cyc_Core_DynamicRegion*dyn=_tmp176;
({Cyc_Core_free_ukey(dyn);});
({({void(*_tmp21D)(struct Cyc_Lex_DynTrieSymbols*ptr)=({void(*_tmp177)(struct Cyc_Lex_DynTrieSymbols*ptr)=(void(*)(struct Cyc_Lex_DynTrieSymbols*ptr))Cyc_Core_ufree;_tmp177;});_tmp21D(idt);});});}}}
# 1111
if(Cyc_Lex_typedefs_trie != 0){
# 1119
struct Cyc_Lex_DynTrie*tdefs=0;
({struct Cyc_Lex_DynTrie*_tmp178=tdefs;struct Cyc_Lex_DynTrie*_tmp179=Cyc_Lex_typedefs_trie;tdefs=_tmp179;Cyc_Lex_typedefs_trie=_tmp178;});{
struct Cyc_Lex_DynTrie _stmttmpE=*((struct Cyc_Lex_DynTrie*)_check_null(tdefs));struct Cyc_Core_DynamicRegion*_tmp17A;_LL4: _tmp17A=_stmttmpE.dyn;_LL5: {struct Cyc_Core_DynamicRegion*dyn=_tmp17A;
({Cyc_Core_free_ukey(dyn);});
({({void(*_tmp21E)(struct Cyc_Lex_DynTrie*ptr)=({void(*_tmp17B)(struct Cyc_Lex_DynTrie*ptr)=(void(*)(struct Cyc_Lex_DynTrie*ptr))Cyc_Core_ufree;_tmp17B;});_tmp21E(tdefs);});});}}}{
# 1111
struct Cyc_Core_NewDynamicRegion _stmttmpF=({Cyc_Core__new_ukey("internal-error","internal-error",0);});struct Cyc_Core_DynamicRegion*_tmp17C;_LL7: _tmp17C=_stmttmpF.key;_LL8: {struct Cyc_Core_DynamicRegion*id_dyn=_tmp17C;
# 1127
struct Cyc_Lex_Trie*ts=({({struct Cyc_Lex_Trie*(*_tmp21F)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Lex_Trie*(*body)(struct _RegionHandle*h,int arg))=({struct Cyc_Lex_Trie*(*_tmp188)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Lex_Trie*(*body)(struct _RegionHandle*h,int arg))=(struct Cyc_Lex_Trie*(*)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Lex_Trie*(*body)(struct _RegionHandle*h,int arg)))Cyc_Core_open_region;_tmp188;});_tmp21F(id_dyn,0,Cyc_Lex_empty_trie);});});
struct Cyc_Xarray_Xarray*xa=({({struct Cyc_Xarray_Xarray*(*_tmp220)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Xarray_Xarray*(*body)(struct _RegionHandle*h,int arg))=({struct Cyc_Xarray_Xarray*(*_tmp187)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Xarray_Xarray*(*body)(struct _RegionHandle*h,int arg))=(struct Cyc_Xarray_Xarray*(*)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Xarray_Xarray*(*body)(struct _RegionHandle*h,int arg)))Cyc_Core_open_region;_tmp187;});_tmp220(id_dyn,0,Cyc_Lex_empty_xarray);});});
Cyc_Lex_ids_trie=({struct Cyc_Lex_DynTrieSymbols*_tmp17D=_region_malloc(Cyc_Core_unique_region,sizeof(*_tmp17D));_tmp17D->dyn=id_dyn,_tmp17D->t=ts,_tmp17D->symbols=xa;_tmp17D;});{
# 1131
struct Cyc_Core_NewDynamicRegion _stmttmp10=({Cyc_Core__new_ukey("internal-error","internal-error",0);});struct Cyc_Core_DynamicRegion*_tmp17E;_LLA: _tmp17E=_stmttmp10.key;_LLB: {struct Cyc_Core_DynamicRegion*typedefs_dyn=_tmp17E;
struct Cyc_Lex_Trie*t=({({struct Cyc_Lex_Trie*(*_tmp221)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Lex_Trie*(*body)(struct _RegionHandle*h,int arg))=({struct Cyc_Lex_Trie*(*_tmp186)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Lex_Trie*(*body)(struct _RegionHandle*h,int arg))=(struct Cyc_Lex_Trie*(*)(struct Cyc_Core_DynamicRegion*key,int arg,struct Cyc_Lex_Trie*(*body)(struct _RegionHandle*h,int arg)))Cyc_Core_open_region;_tmp186;});_tmp221(typedefs_dyn,0,Cyc_Lex_empty_trie);});});
Cyc_Lex_typedefs_trie=({struct Cyc_Lex_DynTrie*_tmp17F=_region_malloc(Cyc_Core_unique_region,sizeof(*_tmp17F));_tmp17F->dyn=typedefs_dyn,_tmp17F->t=t;_tmp17F;});
Cyc_Lex_num_kws=({Cyc_Lex_num_keywords(include_cyclone_keywords);});
Cyc_Lex_kw_nums=({unsigned _tmp181=(unsigned)Cyc_Lex_num_kws;struct Cyc_Lex_KeyWordInfo*_tmp180=_cycalloc(_check_times(_tmp181,sizeof(struct Cyc_Lex_KeyWordInfo)));({{unsigned _tmp1A7=(unsigned)Cyc_Lex_num_kws;unsigned i;for(i=0;i < _tmp1A7;++ i){(_tmp180[i]).token_index=0,(_tmp180[i]).is_c_keyword=0;}}0;});_tag_fat(_tmp180,sizeof(struct Cyc_Lex_KeyWordInfo),_tmp181);});{
unsigned i=0U;
unsigned rwsze=83U;
{unsigned j=0U;for(0;j < rwsze;++ j){
if(include_cyclone_keywords ||(Cyc_Lex_rw_array[(int)j]).f3){
struct _fat_ptr str=(Cyc_Lex_rw_array[(int)j]).f1;
({int(*_tmp182)(struct _fat_ptr buff,int offset,int len)=Cyc_Lex_str_index;struct _fat_ptr _tmp183=str;int _tmp184=0;int _tmp185=(int)({Cyc_strlen((struct _fat_ptr)str);});_tmp182(_tmp183,_tmp184,_tmp185);});
({struct Cyc_Lex_KeyWordInfo _tmp222=({struct Cyc_Lex_KeyWordInfo _tmp1A8;_tmp1A8.token_index=(int)(Cyc_Lex_rw_array[(int)j]).f2,_tmp1A8.is_c_keyword=(Cyc_Lex_rw_array[(int)j]).f3;_tmp1A8;});*((struct Cyc_Lex_KeyWordInfo*)_check_fat_subscript(Cyc_Lex_kw_nums,sizeof(struct Cyc_Lex_KeyWordInfo),(int)i))=_tmp222;});
++ i;}}}
# 1146
({Cyc_Lex_typedef_init();});
Cyc_Lex_comment_depth=0;}}}}}}}
