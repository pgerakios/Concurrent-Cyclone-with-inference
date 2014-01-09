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
struct _fat_ptr Cyc_Core_new_string(unsigned);struct _tuple0{void*f1;void*f2;};
# 108
void*Cyc_Core_fst(struct _tuple0*);
# 111
void*Cyc_Core_snd(struct _tuple0*);extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc___cycFILE;
# 51 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stdout;
# 53
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 79
int Cyc_fclose(struct Cyc___cycFILE*);
# 88
int Cyc_fflush(struct Cyc___cycFILE*);
# 98
struct Cyc___cycFILE*Cyc_fopen(const char*,const char*);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);
# 104
int Cyc_fputc(int,struct Cyc___cycFILE*);
# 106
int Cyc_fputs(const char*,struct Cyc___cycFILE*);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 157 "cycboot.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);
# 165
int puts(const char*);
# 167
int remove(const char*);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 272 "cycboot.h"
void Cyc_file_close(struct Cyc___cycFILE*);
# 318
int system(const char*);
void exit(int);extern char Cyc_Lexing_Error[6U];struct Cyc_Lexing_Error_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Lexing_lexbuf{void(*refill_buff)(struct Cyc_Lexing_lexbuf*);void*refill_state;struct _fat_ptr lex_buffer;int lex_buffer_len;int lex_abs_pos;int lex_start_pos;int lex_curr_pos;int lex_last_pos;int lex_last_action;int lex_eof_reached;};struct Cyc_Lexing_function_lexbuf_state{int(*read_fun)(struct _fat_ptr,int,void*);void*read_fun_state;};struct Cyc_Lexing_lex_tables{struct _fat_ptr lex_base;struct _fat_ptr lex_backtrk;struct _fat_ptr lex_default;struct _fat_ptr lex_trans;struct _fat_ptr lex_check;};
# 78 "lexing.h"
struct Cyc_Lexing_lexbuf*Cyc_Lexing_from_file(struct Cyc___cycFILE*);
# 84
int Cyc_Lexing_lexeme_start(struct Cyc_Lexing_lexbuf*);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 270
struct Cyc_List_List*Cyc_List_zip(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
# 62
struct _fat_ptr Cyc_strconcat(struct _fat_ptr,struct _fat_ptr);
# 64
struct _fat_ptr Cyc_strconcat_l(struct Cyc_List_List*);
# 66
struct _fat_ptr Cyc_str_sepstr(struct Cyc_List_List*,struct _fat_ptr);
# 109 "string.h"
struct _fat_ptr Cyc_substring(struct _fat_ptr,int ofs,unsigned long n);struct Cyc_Lineno_Pos{struct _fat_ptr logical_file;struct _fat_ptr line;int line_no;int col;};
# 32 "lineno.h"
void Cyc_Lineno_poss_of_abss(struct _fat_ptr filename,struct Cyc_List_List*places);
# 34 "filename.h"
struct _fat_ptr Cyc_Filename_chop_extension(struct _fat_ptr);extern char Cyc_Arg_Bad[4U];struct Cyc_Arg_Bad_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Arg_Error[6U];struct Cyc_Arg_Error_exn_struct{char*tag;};struct Cyc_Arg_Unit_spec_Arg_Spec_struct{int tag;void(*f1)();};struct Cyc_Arg_Flag_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_FlagString_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr,struct _fat_ptr);};struct Cyc_Arg_Set_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_Clear_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_String_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_Int_spec_Arg_Spec_struct{int tag;void(*f1)(int);};struct Cyc_Arg_Rest_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};
# 66 "arg.h"
void Cyc_Arg_usage(struct Cyc_List_List*,struct _fat_ptr);
# 71
void Cyc_Arg_parse(struct Cyc_List_List*specs,void(*anonfun)(struct _fat_ptr),int(*anonflagfun)(struct _fat_ptr),struct _fat_ptr errmsg,struct _fat_ptr args);
# 28 "position.h"
void Cyc_Position_reset_position(struct _fat_ptr);struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple1{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
# 184 "absyn.h"
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple1*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple1*datatype_name;struct _tuple1*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple2{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple2 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple3{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple3 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple1*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple4{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple4 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple5{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple6 val;};struct _tuple7{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple7 val;};struct _tuple8{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple8 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple9{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple9*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple10{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple10*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple9*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple11{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple11 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple11 f2;struct _tuple11 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple11 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple12{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple12*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 28 "parse.h"
struct Cyc_List_List*Cyc_Parse_parse_file(struct Cyc___cycFILE*f,struct _fat_ptr name);extern char Cyc_Parse_Exit[5U];struct Cyc_Parse_Exit_exn_struct{char*tag;};struct Cyc_FlatList{struct Cyc_FlatList*tl;void*hd[0U] __attribute__((aligned )) ;};struct Cyc_Type_specifier{int Signed_spec: 1;int Unsigned_spec: 1;int Short_spec: 1;int Long_spec: 1;int Long_Long_spec: 1;int Valid_type_spec: 1;void*Type_spec;unsigned loc;};struct Cyc_Declarator{struct _tuple1*id;struct Cyc_List_List*tms;};struct _tuple14{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;};struct _tuple13{struct _tuple13*tl;struct _tuple14 hd  __attribute__((aligned )) ;};
# 52
enum Cyc_Storage_class{Cyc_Typedef_sc =0U,Cyc_Extern_sc =1U,Cyc_ExternC_sc =2U,Cyc_Static_sc =3U,Cyc_Auto_sc =4U,Cyc_Register_sc =5U,Cyc_Abstract_sc =6U};struct Cyc_Declaration_spec{enum Cyc_Storage_class*sc;struct Cyc_Absyn_Tqual tq;struct Cyc_Type_specifier type_specs;int is_inline;struct Cyc_List_List*attributes;};struct Cyc_Abstractdeclarator{struct Cyc_List_List*tms;};struct Cyc_Numelts_ptrqual_Pointer_qual_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Region_ptrqual_Pointer_qual_struct{int tag;void*f1;};struct Cyc_Thin_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Fat_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Zeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nozeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Notnull_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nullable_ptrqual_Pointer_qual_struct{int tag;};struct _union_YYSTYPE_Int_tok{int tag;union Cyc_Absyn_Cnst val;};struct _union_YYSTYPE_Char_tok{int tag;char val;};struct _union_YYSTYPE_String_tok{int tag;struct _fat_ptr val;};struct _union_YYSTYPE_Stringopt_tok{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_QualId_tok{int tag;struct _tuple1*val;};struct _tuple15{int f1;struct _fat_ptr f2;};struct _union_YYSTYPE_Asm_tok{int tag;struct _tuple15 val;};struct _union_YYSTYPE_Exp_tok{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_Stmt_tok{int tag;struct Cyc_Absyn_Stmt*val;};struct _tuple16{unsigned f1;void*f2;void*f3;};struct _union_YYSTYPE_YY1{int tag;struct _tuple16*val;};struct _union_YYSTYPE_YY2{int tag;void*val;};struct _union_YYSTYPE_YY3{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY4{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY5{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY6{int tag;enum Cyc_Absyn_Primop val;};struct _union_YYSTYPE_YY7{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_YY8{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY9{int tag;struct Cyc_Absyn_Pat*val;};struct _tuple17{struct Cyc_List_List*f1;int f2;};struct _union_YYSTYPE_YY10{int tag;struct _tuple17*val;};struct _union_YYSTYPE_YY11{int tag;struct Cyc_List_List*val;};struct _tuple18{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};struct _union_YYSTYPE_YY12{int tag;struct _tuple18*val;};struct _union_YYSTYPE_YY13{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY14{int tag;struct _tuple17*val;};struct _union_YYSTYPE_YY15{int tag;struct Cyc_Absyn_Fndecl*val;};struct _union_YYSTYPE_YY16{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY17{int tag;struct Cyc_Declaration_spec val;};struct _union_YYSTYPE_YY18{int tag;struct _tuple14 val;};struct _union_YYSTYPE_YY19{int tag;struct _tuple13*val;};struct _union_YYSTYPE_YY20{int tag;enum Cyc_Storage_class*val;};struct _union_YYSTYPE_YY21{int tag;struct Cyc_Type_specifier val;};struct _union_YYSTYPE_YY22{int tag;enum Cyc_Absyn_AggrKind val;};struct _union_YYSTYPE_YY23{int tag;struct Cyc_Absyn_Tqual val;};struct _union_YYSTYPE_YY24{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY25{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY26{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY27{int tag;struct Cyc_Declarator val;};struct _tuple19{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct _union_YYSTYPE_YY28{int tag;struct _tuple19*val;};struct _union_YYSTYPE_YY29{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY30{int tag;struct Cyc_Abstractdeclarator val;};struct _union_YYSTYPE_YY31{int tag;int val;};struct _union_YYSTYPE_YY32{int tag;enum Cyc_Absyn_Scope val;};struct _union_YYSTYPE_YY33{int tag;struct Cyc_Absyn_Datatypefield*val;};struct _union_YYSTYPE_YY34{int tag;struct Cyc_List_List*val;};struct _tuple20{struct Cyc_Absyn_Tqual f1;struct Cyc_Type_specifier f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY35{int tag;struct _tuple20 val;};struct _union_YYSTYPE_YY36{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY37{int tag;struct _tuple10*val;};struct _union_YYSTYPE_YY38{int tag;struct Cyc_List_List*val;};struct _tuple21{struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;};struct _union_YYSTYPE_YY39{int tag;struct _tuple21*val;};struct _union_YYSTYPE_YY40{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY41{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY42{int tag;void*val;};struct _union_YYSTYPE_YY43{int tag;struct Cyc_Absyn_Kind*val;};struct _union_YYSTYPE_YY44{int tag;void*val;};struct _union_YYSTYPE_YY45{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY46{int tag;void*val;};struct _union_YYSTYPE_YY47{int tag;struct Cyc_Absyn_Enumfield*val;};struct _union_YYSTYPE_YY48{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY49{int tag;void*val;};struct _union_YYSTYPE_YY50{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY51{int tag;void*val;};struct _union_YYSTYPE_YY52{int tag;struct Cyc_List_List*val;};struct _tuple22{struct Cyc_List_List*f1;unsigned f2;};struct _union_YYSTYPE_YY53{int tag;struct _tuple22*val;};struct _union_YYSTYPE_YY54{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY55{int tag;void*val;};struct _union_YYSTYPE_YY56{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY57{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_YY58{int tag;void*val;};struct _tuple23{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY59{int tag;struct _tuple23*val;};struct _union_YYSTYPE_YY60{int tag;struct _tuple9*val;};struct _union_YYSTYPE_YY61{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY62{int tag;struct Cyc_List_List*val;};struct _tuple24{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};struct _union_YYSTYPE_YY63{int tag;struct _tuple24*val;};struct _union_YYSTYPE_YY64{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY65{int tag;int val;};struct _union_YYSTYPE_YY66{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YYINITIALSVAL{int tag;int val;};union Cyc_YYSTYPE{struct _union_YYSTYPE_Int_tok Int_tok;struct _union_YYSTYPE_Char_tok Char_tok;struct _union_YYSTYPE_String_tok String_tok;struct _union_YYSTYPE_Stringopt_tok Stringopt_tok;struct _union_YYSTYPE_QualId_tok QualId_tok;struct _union_YYSTYPE_Asm_tok Asm_tok;struct _union_YYSTYPE_Exp_tok Exp_tok;struct _union_YYSTYPE_Stmt_tok Stmt_tok;struct _union_YYSTYPE_YY1 YY1;struct _union_YYSTYPE_YY2 YY2;struct _union_YYSTYPE_YY3 YY3;struct _union_YYSTYPE_YY4 YY4;struct _union_YYSTYPE_YY5 YY5;struct _union_YYSTYPE_YY6 YY6;struct _union_YYSTYPE_YY7 YY7;struct _union_YYSTYPE_YY8 YY8;struct _union_YYSTYPE_YY9 YY9;struct _union_YYSTYPE_YY10 YY10;struct _union_YYSTYPE_YY11 YY11;struct _union_YYSTYPE_YY12 YY12;struct _union_YYSTYPE_YY13 YY13;struct _union_YYSTYPE_YY14 YY14;struct _union_YYSTYPE_YY15 YY15;struct _union_YYSTYPE_YY16 YY16;struct _union_YYSTYPE_YY17 YY17;struct _union_YYSTYPE_YY18 YY18;struct _union_YYSTYPE_YY19 YY19;struct _union_YYSTYPE_YY20 YY20;struct _union_YYSTYPE_YY21 YY21;struct _union_YYSTYPE_YY22 YY22;struct _union_YYSTYPE_YY23 YY23;struct _union_YYSTYPE_YY24 YY24;struct _union_YYSTYPE_YY25 YY25;struct _union_YYSTYPE_YY26 YY26;struct _union_YYSTYPE_YY27 YY27;struct _union_YYSTYPE_YY28 YY28;struct _union_YYSTYPE_YY29 YY29;struct _union_YYSTYPE_YY30 YY30;struct _union_YYSTYPE_YY31 YY31;struct _union_YYSTYPE_YY32 YY32;struct _union_YYSTYPE_YY33 YY33;struct _union_YYSTYPE_YY34 YY34;struct _union_YYSTYPE_YY35 YY35;struct _union_YYSTYPE_YY36 YY36;struct _union_YYSTYPE_YY37 YY37;struct _union_YYSTYPE_YY38 YY38;struct _union_YYSTYPE_YY39 YY39;struct _union_YYSTYPE_YY40 YY40;struct _union_YYSTYPE_YY41 YY41;struct _union_YYSTYPE_YY42 YY42;struct _union_YYSTYPE_YY43 YY43;struct _union_YYSTYPE_YY44 YY44;struct _union_YYSTYPE_YY45 YY45;struct _union_YYSTYPE_YY46 YY46;struct _union_YYSTYPE_YY47 YY47;struct _union_YYSTYPE_YY48 YY48;struct _union_YYSTYPE_YY49 YY49;struct _union_YYSTYPE_YY50 YY50;struct _union_YYSTYPE_YY51 YY51;struct _union_YYSTYPE_YY52 YY52;struct _union_YYSTYPE_YY53 YY53;struct _union_YYSTYPE_YY54 YY54;struct _union_YYSTYPE_YY55 YY55;struct _union_YYSTYPE_YY56 YY56;struct _union_YYSTYPE_YY57 YY57;struct _union_YYSTYPE_YY58 YY58;struct _union_YYSTYPE_YY59 YY59;struct _union_YYSTYPE_YY60 YY60;struct _union_YYSTYPE_YY61 YY61;struct _union_YYSTYPE_YY62 YY62;struct _union_YYSTYPE_YY63 YY63;struct _union_YYSTYPE_YY64 YY64;struct _union_YYSTYPE_YY65 YY65;struct _union_YYSTYPE_YY66 YY66;struct _union_YYSTYPE_YYINITIALSVAL YYINITIALSVAL;};struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int last_line;int last_column;};
# 39 "pp.h"
extern int Cyc_PP_tex_output;struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
# 53
struct _fat_ptr Cyc_PP_string_of_doc(struct Cyc_PP_Doc*d,int w);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 51 "absynpp.h"
extern int Cyc_Absynpp_print_for_cycdoc;
# 53
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);
# 55
extern struct Cyc_Absynpp_Params Cyc_Absynpp_tc_params_r;
# 59
struct Cyc_PP_Doc*Cyc_Absynpp_decl2doc(struct Cyc_Absyn_Decl*d);
# 29 "cycdoc.cyl"
void Cyc_Lex_lex_init(int use_cyclone_keywords);struct Cyc_Position_Segment{int start;int end;};struct Cyc_MatchDecl_Comment_struct{int tag;struct _fat_ptr f1;};struct Cyc_Standalone_Comment_struct{int tag;struct _fat_ptr f1;};struct _tuple25{int f1;void*f2;};
# 44
struct _tuple25*Cyc_token(struct Cyc_Lexing_lexbuf*lexbuf);
# 47
const int Cyc_lex_base[15U]={0,- 4,0,- 3,1,2,3,0,4,6,7,- 1,8,9,- 2};
const int Cyc_lex_backtrk[15U]={- 1,- 1,3,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1};
const int Cyc_lex_default[15U]={1,0,- 1,0,- 1,- 1,6,- 1,8,8,8,0,6,6,0};
const int Cyc_lex_trans[266U]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,6,0,0,0,0,0,0,0,4,5,7,12,9,2,10,10,13,13,0,11,- 1,14,- 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,3,0,0,- 1,- 1,0,- 1,- 1,- 1,- 1};
const int Cyc_lex_check[266U]={- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,7,- 1,5,- 1,- 1,- 1,- 1,- 1,- 1,- 1,2,4,5,6,8,0,9,10,12,13,- 1,9,10,12,13,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,6,8,- 1,9,10,12,13};
int Cyc_lex_engine(int start_state,struct Cyc_Lexing_lexbuf*lbuf){
# 54
int state;int base;int backtrk;
int c;
state=start_state;
# 58
if(state >= 0){
({int _tmpB1=lbuf->lex_start_pos=lbuf->lex_curr_pos;lbuf->lex_last_pos=_tmpB1;});
lbuf->lex_last_action=- 1;}else{
# 62
state=(- state)- 1;}
# 64
while(1){
base=*((const int*)_check_known_subscript_notnull(Cyc_lex_base,15U,sizeof(int),state));
if(base < 0)return(- base)- 1;backtrk=Cyc_lex_backtrk[state];
# 68
if(backtrk >= 0){
lbuf->lex_last_pos=lbuf->lex_curr_pos;
lbuf->lex_last_action=backtrk;}
# 68
if(lbuf->lex_curr_pos >= lbuf->lex_buffer_len){
# 73
if(!lbuf->lex_eof_reached)
return(- state)- 1;else{
# 76
c=256;}}else{
# 78
c=(int)*((char*)_check_fat_subscript(lbuf->lex_buffer,sizeof(char),lbuf->lex_curr_pos ++));
if(c == -1)c=256;}
# 81
if(*((const int*)_check_known_subscript_notnull(Cyc_lex_check,266U,sizeof(int),base + c))== state)
state=*((const int*)_check_known_subscript_notnull(Cyc_lex_trans,266U,sizeof(int),base + c));else{
# 84
state=Cyc_lex_default[state];}
if(state < 0){
lbuf->lex_curr_pos=lbuf->lex_last_pos;
if(lbuf->lex_last_action == - 1)
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp1=_cycalloc(sizeof(*_tmp1));_tmp1->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmpB2=({const char*_tmp0="empty token";_tag_fat(_tmp0,sizeof(char),12U);});_tmp1->f1=_tmpB2;});_tmp1;}));else{
# 90
return lbuf->lex_last_action;}}else{
# 93
if(c == 256)lbuf->lex_eof_reached=0;}}}
# 97
struct _tuple25*Cyc_token_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp3=lexstate;switch(_tmp3){case 0U: _LL1: _LL2: {
# 51 "cycdoc.cyl"
int start=lexbuf->lex_start_pos + 5;
int len=(lexbuf->lex_curr_pos - lexbuf->lex_start_pos)- 7;
return({struct _tuple25*_tmp5=_cycalloc(sizeof(*_tmp5));({int _tmpB5=({Cyc_Lexing_lexeme_start(lexbuf);});_tmp5->f1=_tmpB5;}),({
void*_tmpB4=(void*)({struct Cyc_Standalone_Comment_struct*_tmp4=_cycalloc(sizeof(*_tmp4));_tmp4->tag=1U,({struct _fat_ptr _tmpB3=({Cyc_substring((struct _fat_ptr)lexbuf->lex_buffer,start,(unsigned long)len);});_tmp4->f1=_tmpB3;});_tmp4;});_tmp5->f2=_tmpB4;});_tmp5;});}case 1U: _LL3: _LL4: {
# 56
int start=lexbuf->lex_start_pos + 4;
int len=(lexbuf->lex_curr_pos - lexbuf->lex_start_pos)- 6;
return({struct _tuple25*_tmp7=_cycalloc(sizeof(*_tmp7));({int _tmpB8=({Cyc_Lexing_lexeme_start(lexbuf);});_tmp7->f1=_tmpB8;}),({
void*_tmpB7=(void*)({struct Cyc_MatchDecl_Comment_struct*_tmp6=_cycalloc(sizeof(*_tmp6));_tmp6->tag=0U,({struct _fat_ptr _tmpB6=({Cyc_substring((struct _fat_ptr)lexbuf->lex_buffer,start,(unsigned long)len);});_tmp6->f1=_tmpB6;});_tmp6;});_tmp7->f2=_tmpB7;});_tmp7;});}case 2U: _LL5: _LL6:
# 60 "cycdoc.cyl"
 return 0;case 3U: _LL7: _LL8:
# 61 "cycdoc.cyl"
 return({Cyc_token(lexbuf);});default: _LL9: _LLA:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_token_rec(lexbuf,lexstate);});}_LL0:;}
# 65
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp9=_cycalloc(sizeof(*_tmp9));_tmp9->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmpB9=({const char*_tmp8="some action didn't return!";_tag_fat(_tmp8,sizeof(char),27U);});_tmp9->f1=_tmpB9;});_tmp9;}));}
# 67
struct _tuple25*Cyc_token(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_token_rec(lexbuf,0);});}struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 68 "cycdoc.cyl"
int Cyc_noexpand_r=0;
static void Cyc_dump_begin(){
# 71
({puts("%%HEVEA \\begin{latexonly}\n\\begin{list}{}{\\setlength\\itemsep{0pt}\\setlength\\topsep{0pt}\\setlength\\leftmargin{\\parindent}\\setlength\\itemindent{-\\leftmargin}}\\item[]\\colorbox{cycdochighlight}{\\ttfamily\\begin{minipage}[b]{\\textwidth}\n%%HEVEA \\end{latexonly}\n%%HEVEA \\begin{rawhtml}<dl><dt><table><tr><td bgcolor=\"c0d0ff\">\\end{rawhtml}\n\\begin{tabbing}");});}
# 85
static void Cyc_dump_middle(){
({puts("\\end{tabbing}\n%%HEVEA \\begin{latexonly}\n\\end{minipage}}\\\\\\strut\n%%HEVEA \\end{latexonly}\n%%HEVEA \\begin{rawhtml}</td></tr></table><dd>\\end{rawhtml}");});}
# 92
static void Cyc_dump_end(){
# 94
({puts("%%HEVEA \\begin{latexonly}\n\\end{list}\\smallskip\n%%HEVEA \\end{latexonly}\n%%HEVEA \\begin{rawhtml}</dl>\\end{rawhtml}");});}
# 103
static void Cyc_pr_comment(struct Cyc___cycFILE*outf,struct _fat_ptr s){
int depth=0;
int len=(int)({Cyc_strlen((struct _fat_ptr)s);});
int i=0;for(0;i < len;++ i){
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c != (int)'['){({Cyc_fputc((int)c,outf);});continue;}({Cyc_fputs("\\texttt{",outf);});
# 110
++ i;
++ depth;
for(0;i < len;++ i){
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)']'){
-- depth;
if(depth == 0){
({Cyc_fputc((int)'}',outf);});
break;}}else{
# 121
if((int)c == (int)'[')++ depth;}
# 114
({Cyc_fputc((int)c,outf);});}}}
# 103
static int Cyc_width=50;
# 129
static void Cyc_set_width(int w){
Cyc_width=w;}
# 129
static struct Cyc_List_List*Cyc_cycdoc_files=0;
# 133
static void Cyc_add_other(struct _fat_ptr s){
Cyc_cycdoc_files=({struct Cyc_List_List*_tmp12=_cycalloc(sizeof(*_tmp12));({struct _fat_ptr*_tmpBA=({struct _fat_ptr*_tmp11=_cycalloc(sizeof(*_tmp11));*_tmp11=s;_tmp11;});_tmp12->hd=_tmpBA;}),_tmp12->tl=Cyc_cycdoc_files;_tmp12;});}
# 136
static int Cyc_no_other(struct _fat_ptr s){
return 0;}
# 136
static struct Cyc_List_List*Cyc_cycargs=0;
# 140
static void Cyc_add_cycarg(struct _fat_ptr s){
Cyc_cycargs=({struct Cyc_List_List*_tmp16=_cycalloc(sizeof(*_tmp16));({struct _fat_ptr*_tmpBB=({struct _fat_ptr*_tmp15=_cycalloc(sizeof(*_tmp15));*_tmp15=s;_tmp15;});_tmp16->hd=_tmpBB;}),_tmp16->tl=Cyc_cycargs;_tmp16;});}static char _tmp18[8U]="cyclone";
# 140
static struct _fat_ptr Cyc_cyclone_file={_tmp18,_tmp18,_tmp18 + 8U};
# 144
static void Cyc_set_cyclone_file(struct _fat_ptr s){
Cyc_cyclone_file=s;}
# 149
static void Cyc_dumpdecl(struct Cyc_Absyn_Decl*d,struct _fat_ptr comment){
({Cyc_dump_begin();});
# 155
({struct Cyc_String_pa_PrintArg_struct _tmp1C=({struct Cyc_String_pa_PrintArg_struct _tmpAB;_tmpAB.tag=0U,({struct _fat_ptr _tmpBC=(struct _fat_ptr)((struct _fat_ptr)({struct _fat_ptr(*_tmp1D)(struct Cyc_PP_Doc*d,int w)=Cyc_PP_string_of_doc;struct Cyc_PP_Doc*_tmp1E=({Cyc_Absynpp_decl2doc(d);});int _tmp1F=50;_tmp1D(_tmp1E,_tmp1F);}));_tmpAB.f1=_tmpBC;});_tmpAB;});void*_tmp1A[1U];_tmp1A[0]=& _tmp1C;({struct _fat_ptr _tmpBD=({const char*_tmp1B="%s";_tag_fat(_tmp1B,sizeof(char),3U);});Cyc_printf(_tmpBD,_tag_fat(_tmp1A,sizeof(void*),1U));});});
({Cyc_dump_middle();});
({Cyc_pr_comment(Cyc_stdout,comment);});
({void*_tmp20=0U;({struct _fat_ptr _tmpBE=({const char*_tmp21="\n";_tag_fat(_tmp21,sizeof(char),2U);});Cyc_printf(_tmpBE,_tag_fat(_tmp20,sizeof(void*),0U));});});
({Cyc_dump_end();});}
# 163
static int Cyc_is_other_special(char c){
char _tmp23=c;switch(_tmp23){case 92U: _LL1: _LL2:
 goto _LL4;case 34U: _LL3: _LL4:
 goto _LL6;case 59U: _LL5: _LL6:
 goto _LL8;case 38U: _LL7: _LL8:
 goto _LLA;case 40U: _LL9: _LLA:
 goto _LLC;case 41U: _LLB: _LLC:
 goto _LLE;case 124U: _LLD: _LLE:
 goto _LL10;case 94U: _LLF: _LL10:
 goto _LL12;case 60U: _LL11: _LL12:
 goto _LL14;case 62U: _LL13: _LL14:
 goto _LL16;case 32U: _LL15: _LL16:
 goto _LL18;case 10U: _LL17: _LL18:
 goto _LL1A;case 9U: _LL19: _LL1A:
 return 1;default: _LL1B: _LL1C:
 return 0;}_LL0:;}
# 182
static struct _fat_ptr Cyc_sh_escape_string(struct _fat_ptr s){
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
# 186
int single_quotes=0;
int other_special=0;
{int i=0;for(0;(unsigned long)i < len;++ i){
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)'\'')++ single_quotes;else{
if(({Cyc_is_other_special(c);}))++ other_special;}}}
# 195
if(single_quotes == 0 && other_special == 0)
return s;
# 195
if(single_quotes == 0)
# 200
return(struct _fat_ptr)({struct _fat_ptr(*_tmp25)(struct Cyc_List_List*)=Cyc_strconcat_l;struct Cyc_List_List*_tmp26=({struct _fat_ptr*_tmp27[3U];({struct _fat_ptr*_tmpC3=({struct _fat_ptr*_tmp29=_cycalloc(sizeof(*_tmp29));({struct _fat_ptr _tmpC2=({const char*_tmp28="'";_tag_fat(_tmp28,sizeof(char),2U);});*_tmp29=_tmpC2;});_tmp29;});_tmp27[0]=_tmpC3;}),({struct _fat_ptr*_tmpC1=({struct _fat_ptr*_tmp2A=_cycalloc(sizeof(*_tmp2A));*_tmp2A=(struct _fat_ptr)s;_tmp2A;});_tmp27[1]=_tmpC1;}),({struct _fat_ptr*_tmpC0=({struct _fat_ptr*_tmp2C=_cycalloc(sizeof(*_tmp2C));({struct _fat_ptr _tmpBF=({const char*_tmp2B="'";_tag_fat(_tmp2B,sizeof(char),2U);});*_tmp2C=_tmpBF;});_tmp2C;});_tmp27[2]=_tmpC0;});Cyc_List_list(_tag_fat(_tmp27,sizeof(struct _fat_ptr*),3U));});_tmp25(_tmp26);});{
# 195
unsigned long len2=(len + (unsigned long)single_quotes)+ (unsigned long)other_special;
# 204
struct _fat_ptr s2=({unsigned _tmp34=(len2 + (unsigned long)1)+ 1U;char*_tmp33=_cycalloc_atomic(_check_times(_tmp34,sizeof(char)));({{unsigned _tmpAC=len2 + (unsigned long)1;unsigned i;for(i=0;i < _tmpAC;++ i){_tmp33[i]='\000';}_tmp33[_tmpAC]=0;}0;});_tag_fat(_tmp33,sizeof(char),_tmp34);});
int i=0;
int j=0;
for(0;(unsigned long)i < len;++ i){
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)'\'' ||({Cyc_is_other_special(c);}))
({struct _fat_ptr _tmp2D=_fat_ptr_plus(s2,sizeof(char),j ++);char _tmp2E=*((char*)_check_fat_subscript(_tmp2D,sizeof(char),0U));char _tmp2F='\\';if(_get_fat_size(_tmp2D,sizeof(char))== 1U &&(_tmp2E == 0 && _tmp2F != 0))_throw_arraybounds();*((char*)_tmp2D.curr)=_tmp2F;});
# 209
({struct _fat_ptr _tmp30=_fat_ptr_plus(s2,sizeof(char),j ++);char _tmp31=*((char*)_check_fat_subscript(_tmp30,sizeof(char),0U));char _tmp32=c;if(_get_fat_size(_tmp30,sizeof(char))== 1U &&(_tmp31 == 0 && _tmp32 != 0))_throw_arraybounds();*((char*)_tmp30.curr)=_tmp32;});}
# 213
return(struct _fat_ptr)s2;}}
# 215
static struct _fat_ptr*Cyc_sh_escape_stringptr(struct _fat_ptr*sp){
return({struct _fat_ptr*_tmp36=_cycalloc(sizeof(*_tmp36));({struct _fat_ptr _tmpC4=({Cyc_sh_escape_string(*sp);});*_tmp36=_tmpC4;});_tmp36;});}
# 223
static struct Cyc_Lineno_Pos*Cyc_new_pos(){
return({struct Cyc_Lineno_Pos*_tmp39=_cycalloc(sizeof(*_tmp39));({struct _fat_ptr _tmpC6=({const char*_tmp38="";_tag_fat(_tmp38,sizeof(char),1U);});_tmp39->logical_file=_tmpC6;}),({struct _fat_ptr _tmpC5=({Cyc_Core_new_string(0U);});_tmp39->line=_tmpC5;}),_tmp39->line_no=0,_tmp39->col=0;_tmp39;});}struct _tuple26{int f1;struct Cyc_Lineno_Pos*f2;};
# 226
static struct _tuple26*Cyc_start2pos(int x){
return({struct _tuple26*_tmp3B=_cycalloc(sizeof(*_tmp3B));_tmp3B->f1=x,({struct Cyc_Lineno_Pos*_tmpC7=({Cyc_new_pos();});_tmp3B->f2=_tmpC7;});_tmp3B;});}
# 231
static int Cyc_decl2start(struct Cyc_Absyn_Decl*d){
return(int)d->loc;}struct _tuple27{struct Cyc_Lineno_Pos*f1;void*f2;};
# 237
static struct Cyc_List_List*Cyc_this_file(struct _fat_ptr file,struct Cyc_List_List*x){
struct Cyc_List_List*result=0;
for(0;x != 0;x=x->tl){
if(({Cyc_strcmp((struct _fat_ptr)((*((struct _tuple27*)x->hd)).f1)->logical_file,(struct _fat_ptr)file);})== 0)
result=({struct Cyc_List_List*_tmp3E=_cycalloc(sizeof(*_tmp3E));_tmp3E->hd=(struct _tuple27*)x->hd,_tmp3E->tl=result;_tmp3E;});}
# 243
result=({Cyc_List_imp_rev(result);});
return result;}
# 248
static int Cyc_lineno(struct Cyc_Lineno_Pos*p){
return p->line_no;}
# 252
static struct Cyc_List_List*Cyc_flatten_decls(struct Cyc_List_List*decls){
struct Cyc_List_List*result=0;
while(decls != 0){
void*_stmttmp0=((struct Cyc_Absyn_Decl*)decls->hd)->r;void*_tmp41=_stmttmp0;struct Cyc_List_List*_tmp42;struct Cyc_List_List*_tmp43;struct Cyc_List_List*_tmp44;struct Cyc_List_List*_tmp45;switch(*((int*)_tmp41)){case 9U: _LL1: _tmp45=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp41)->f2;_LL2: {struct Cyc_List_List*tdl=_tmp45;
_tmp44=tdl;goto _LL4;}case 10U: _LL3: _tmp44=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp41)->f2;_LL4: {struct Cyc_List_List*tdl=_tmp44;
_tmp43=tdl;goto _LL6;}case 11U: _LL5: _tmp43=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp41)->f1;_LL6: {struct Cyc_List_List*tdl=_tmp43;
_tmp42=tdl;goto _LL8;}case 12U: _LL7: _tmp42=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp41)->f1;_LL8: {struct Cyc_List_List*tdl=_tmp42;
# 260
decls=({Cyc_List_append(tdl,decls->tl);});
goto _LL0;}case 5U: _LL9: _LLA:
 goto _LLC;case 0U: _LLB: _LLC:
 goto _LLE;case 6U: _LLD: _LLE:
 goto _LL10;case 7U: _LLF: _LL10:
 goto _LL12;case 8U: _LL11: _LL12:
 goto _LL14;case 1U: _LL13: _LL14:
 goto _LL16;case 2U: _LL15: _LL16:
 goto _LL18;case 3U: _LL17: _LL18:
 goto _LL1A;case 13U: _LL19: _LL1A:
 goto _LL1C;case 14U: _LL1B: _LL1C:
 goto _LL1E;case 15U: _LL1D: _LL1E:
 goto _LL20;case 16U: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
# 276
 result=({struct Cyc_List_List*_tmp46=_cycalloc(sizeof(*_tmp46));_tmp46->hd=(struct Cyc_Absyn_Decl*)decls->hd,_tmp46->tl=result;_tmp46;});
decls=decls->tl;
goto _LL0;}_LL0:;}
# 281
return({Cyc_List_imp_rev(result);});}struct _tuple28{int f1;struct Cyc_Absyn_Decl*f2;};struct _tuple29{struct Cyc_Lineno_Pos*f1;struct Cyc_Absyn_Decl*f2;};
# 285
static void Cyc_process_file(struct _fat_ptr filename){
struct _fat_ptr basename=({Cyc_Filename_chop_extension(filename);});
# 288
const char*preprocfile=(const char*)_check_null(_untag_fat_ptr(({({struct _fat_ptr _tmpC8=(struct _fat_ptr)basename;Cyc_strconcat(_tmpC8,({const char*_tmp8C=".cyp";_tag_fat(_tmp8C,sizeof(char),5U);}));});}),sizeof(char),1U));
struct _fat_ptr cycargs_string=({struct _fat_ptr(*_tmp81)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp82=({struct Cyc_List_List*_tmp89=_cycalloc(sizeof(*_tmp89));
({struct _fat_ptr*_tmpCB=({struct _fat_ptr*_tmp84=_cycalloc(sizeof(*_tmp84));({struct _fat_ptr _tmpCA=(struct _fat_ptr)({const char*_tmp83="";_tag_fat(_tmp83,sizeof(char),1U);});*_tmp84=_tmpCA;});_tmp84;});_tmp89->hd=_tmpCB;}),({
struct Cyc_List_List*_tmpC9=({struct Cyc_List_List*(*_tmp85)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp86)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_map;_tmp86;});struct _fat_ptr*(*_tmp87)(struct _fat_ptr*sp)=Cyc_sh_escape_stringptr;struct Cyc_List_List*_tmp88=({Cyc_List_rev(Cyc_cycargs);});_tmp85(_tmp87,_tmp88);});_tmp89->tl=_tmpC9;});_tmp89;});struct _fat_ptr _tmp8A=({const char*_tmp8B=" ";_tag_fat(_tmp8B,sizeof(char),2U);});_tmp81(_tmp82,_tmp8A);});
# 293
const char*cmd=(const char*)_check_null(_untag_fat_ptr(({struct Cyc_String_pa_PrintArg_struct _tmp7C=({struct Cyc_String_pa_PrintArg_struct _tmpB0;_tmpB0.tag=0U,_tmpB0.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_cyclone_file);_tmpB0;});struct Cyc_String_pa_PrintArg_struct _tmp7D=({struct Cyc_String_pa_PrintArg_struct _tmpAF;_tmpAF.tag=0U,_tmpAF.f1=(struct _fat_ptr)((struct _fat_ptr)cycargs_string);_tmpAF;});struct Cyc_String_pa_PrintArg_struct _tmp7E=({struct Cyc_String_pa_PrintArg_struct _tmpAE;_tmpAE.tag=0U,({struct _fat_ptr _tmpCC=(struct _fat_ptr)((struct _fat_ptr)({Cyc_sh_escape_string(({const char*_tmp80=preprocfile;_tag_fat(_tmp80,sizeof(char),_get_zero_arr_size_char((void*)_tmp80,1U));}));}));_tmpAE.f1=_tmpCC;});_tmpAE;});struct Cyc_String_pa_PrintArg_struct _tmp7F=({struct Cyc_String_pa_PrintArg_struct _tmpAD;_tmpAD.tag=0U,({struct _fat_ptr _tmpCD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_sh_escape_string(filename);}));_tmpAD.f1=_tmpCD;});_tmpAD;});void*_tmp7A[4U];_tmp7A[0]=& _tmp7C,_tmp7A[1]=& _tmp7D,_tmp7A[2]=& _tmp7E,_tmp7A[3]=& _tmp7F;({struct _fat_ptr _tmpCE=({const char*_tmp7B="%s %s -E -o %s -x cyc %s";_tag_fat(_tmp7B,sizeof(char),25U);});Cyc_aprintf(_tmpCE,_tag_fat(_tmp7A,sizeof(void*),4U));});}),sizeof(char),1U));
# 298
if(({system(cmd);})!= 0){
({void*_tmp48=0U;({struct Cyc___cycFILE*_tmpD0=Cyc_stderr;struct _fat_ptr _tmpCF=({const char*_tmp49="\nError: preprocessing\n";_tag_fat(_tmp49,sizeof(char),23U);});Cyc_fprintf(_tmpD0,_tmpCF,_tag_fat(_tmp48,sizeof(void*),0U));});});
return;}
# 298
({Cyc_Position_reset_position(({const char*_tmp4A=preprocfile;_tag_fat(_tmp4A,sizeof(char),_get_zero_arr_size_char((void*)_tmp4A,1U));}));});{
# 304
struct Cyc___cycFILE*in_file=(struct Cyc___cycFILE*)_check_null(({Cyc_fopen(preprocfile,"r");}));
({Cyc_Lex_lex_init(1);});{
struct Cyc_List_List*decls=({({struct Cyc___cycFILE*_tmpD1=in_file;Cyc_Parse_parse_file(_tmpD1,({const char*_tmp79=preprocfile;_tag_fat(_tmp79,sizeof(char),_get_zero_arr_size_char((void*)_tmp79,1U));}));});});
({Cyc_Lex_lex_init(1);});
({Cyc_file_close(in_file);});
decls=({Cyc_flatten_decls(decls);});{
# 311
struct Cyc_List_List*poss=({struct Cyc_List_List*(*_tmp74)(struct _tuple26*(*f)(int),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp75)(struct _tuple26*(*f)(int),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple26*(*f)(int),struct Cyc_List_List*x))Cyc_List_map;_tmp75;});struct _tuple26*(*_tmp76)(int x)=Cyc_start2pos;struct Cyc_List_List*_tmp77=({({struct Cyc_List_List*(*_tmpD2)(int(*f)(struct Cyc_Absyn_Decl*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp78)(int(*f)(struct Cyc_Absyn_Decl*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*f)(struct Cyc_Absyn_Decl*),struct Cyc_List_List*x))Cyc_List_map;_tmp78;});_tmpD2(Cyc_decl2start,decls);});});_tmp74(_tmp76,_tmp77);});
({({struct _fat_ptr _tmpD3=({const char*_tmp4B=preprocfile;_tag_fat(_tmp4B,sizeof(char),_get_zero_arr_size_char((void*)_tmp4B,1U));});Cyc_Lineno_poss_of_abss(_tmpD3,poss);});});
({remove(preprocfile);});{
struct Cyc_List_List*pos_decls=({struct Cyc_List_List*(*_tmp6F)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_zip;struct Cyc_List_List*_tmp70=({({struct Cyc_List_List*(*_tmpD5)(struct Cyc_Lineno_Pos*(*f)(struct _tuple26*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp71)(struct Cyc_Lineno_Pos*(*f)(struct _tuple26*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Lineno_Pos*(*f)(struct _tuple26*),struct Cyc_List_List*x))Cyc_List_map;_tmp71;});struct Cyc_Lineno_Pos*(*_tmpD4)(struct _tuple26*)=({struct Cyc_Lineno_Pos*(*_tmp72)(struct _tuple26*)=(struct Cyc_Lineno_Pos*(*)(struct _tuple26*))Cyc_Core_snd;_tmp72;});_tmpD5(_tmpD4,poss);});});struct Cyc_List_List*_tmp73=decls;_tmp6F(_tmp70,_tmp73);});
# 316
pos_decls=({Cyc_this_file(filename,pos_decls);});{
struct Cyc_List_List*lineno_decls=({struct Cyc_List_List*(*_tmp64)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_zip;struct Cyc_List_List*_tmp65=({struct Cyc_List_List*(*_tmp66)(int(*f)(struct Cyc_Lineno_Pos*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp67)(int(*f)(struct Cyc_Lineno_Pos*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*f)(struct Cyc_Lineno_Pos*),struct Cyc_List_List*x))Cyc_List_map;_tmp67;});int(*_tmp68)(struct Cyc_Lineno_Pos*p)=Cyc_lineno;struct Cyc_List_List*_tmp69=({({struct Cyc_List_List*(*_tmpD7)(struct Cyc_Lineno_Pos*(*f)(struct _tuple29*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp6A)(struct Cyc_Lineno_Pos*(*f)(struct _tuple29*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Lineno_Pos*(*f)(struct _tuple29*),struct Cyc_List_List*x))Cyc_List_map;_tmp6A;});struct Cyc_Lineno_Pos*(*_tmpD6)(struct _tuple29*)=({struct Cyc_Lineno_Pos*(*_tmp6B)(struct _tuple29*)=(struct Cyc_Lineno_Pos*(*)(struct _tuple29*))Cyc_Core_fst;_tmp6B;});_tmpD7(_tmpD6,pos_decls);});});_tmp66(_tmp68,_tmp69);});struct Cyc_List_List*_tmp6C=({({struct Cyc_List_List*(*_tmpD9)(struct Cyc_Absyn_Decl*(*f)(struct _tuple29*),struct Cyc_List_List*x)=({
struct Cyc_List_List*(*_tmp6D)(struct Cyc_Absyn_Decl*(*f)(struct _tuple29*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Decl*(*f)(struct _tuple29*),struct Cyc_List_List*x))Cyc_List_map;_tmp6D;});struct Cyc_Absyn_Decl*(*_tmpD8)(struct _tuple29*)=({struct Cyc_Absyn_Decl*(*_tmp6E)(struct _tuple29*)=(struct Cyc_Absyn_Decl*(*)(struct _tuple29*))Cyc_Core_snd;_tmp6E;});_tmpD9(_tmpD8,pos_decls);});});_tmp64(_tmp65,_tmp6C);});
# 321
struct Cyc___cycFILE*f=(struct Cyc___cycFILE*)_check_null(({Cyc_fopen((const char*)_check_null(_untag_fat_ptr(filename,sizeof(char),1U)),"r");}));
struct Cyc_Lexing_lexbuf*lb=({Cyc_Lexing_from_file(f);});
struct Cyc_List_List*comments=0;
struct Cyc_List_List*indices=0;
struct _tuple25*tok;
while((tok=({Cyc_token(lb);}))!= 0){
struct _tuple25 _stmttmp1=*((struct _tuple25*)_check_null(tok));void*_tmp4D;int _tmp4C;_LL1: _tmp4C=_stmttmp1.f1;_tmp4D=_stmttmp1.f2;_LL2: {int index=_tmp4C;void*comment=_tmp4D;
comments=({struct Cyc_List_List*_tmp4E=_cycalloc(sizeof(*_tmp4E));_tmp4E->hd=comment,_tmp4E->tl=comments;_tmp4E;});
indices=({struct Cyc_List_List*_tmp4F=_cycalloc(sizeof(*_tmp4F));_tmp4F->hd=(void*)index,_tmp4F->tl=indices;_tmp4F;});}}
# 331
({Cyc_fclose(f);});
comments=({Cyc_List_imp_rev(comments);});
indices=({Cyc_List_imp_rev(indices);});{
# 335
struct Cyc_List_List*poss=({({struct Cyc_List_List*(*_tmpDA)(struct _tuple26*(*f)(int),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp63)(struct _tuple26*(*f)(int),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple26*(*f)(int),struct Cyc_List_List*x))Cyc_List_map;_tmp63;});_tmpDA(Cyc_start2pos,indices);});});
({Cyc_Lineno_poss_of_abss(filename,poss);});{
struct Cyc_List_List*lineno_comments=({struct Cyc_List_List*(*_tmp5A)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_zip;struct Cyc_List_List*_tmp5B=({struct Cyc_List_List*(*_tmp5C)(int(*f)(struct Cyc_Lineno_Pos*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp5D)(int(*f)(struct Cyc_Lineno_Pos*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(int(*f)(struct Cyc_Lineno_Pos*),struct Cyc_List_List*x))Cyc_List_map;_tmp5D;});int(*_tmp5E)(struct Cyc_Lineno_Pos*p)=Cyc_lineno;struct Cyc_List_List*_tmp5F=({({struct Cyc_List_List*(*_tmpDC)(struct Cyc_Lineno_Pos*(*f)(struct _tuple26*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp60)(struct Cyc_Lineno_Pos*(*f)(struct _tuple26*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Lineno_Pos*(*f)(struct _tuple26*),struct Cyc_List_List*x))Cyc_List_map;_tmp60;});struct Cyc_Lineno_Pos*(*_tmpDB)(struct _tuple26*)=({struct Cyc_Lineno_Pos*(*_tmp61)(struct _tuple26*)=(struct Cyc_Lineno_Pos*(*)(struct _tuple26*))Cyc_Core_snd;_tmp61;});_tmpDC(_tmpDB,poss);});});_tmp5C(_tmp5E,_tmp5F);});struct Cyc_List_List*_tmp62=comments;_tmp5A(_tmp5B,_tmp62);});
# 342
while(lineno_decls != 0 && lineno_comments != 0){
struct _tuple28*_stmttmp2=(struct _tuple28*)lineno_decls->hd;struct Cyc_Absyn_Decl*_tmp51;int _tmp50;_LL4: _tmp50=_stmttmp2->f1;_tmp51=_stmttmp2->f2;_LL5: {int dline=_tmp50;struct Cyc_Absyn_Decl*decl=_tmp51;
struct _tuple25*_stmttmp3=(struct _tuple25*)lineno_comments->hd;void*_tmp53;int _tmp52;_LL7: _tmp52=_stmttmp3->f1;_tmp53=_stmttmp3->f2;_LL8: {int cline=_tmp52;void*comment=_tmp53;
void*_tmp54=comment;struct _fat_ptr _tmp55;struct _fat_ptr _tmp56;if(((struct Cyc_Standalone_Comment_struct*)_tmp54)->tag == 1U){_LLA: _tmp56=((struct Cyc_Standalone_Comment_struct*)_tmp54)->f1;_LLB: {struct _fat_ptr cmt=_tmp56;
# 347
({Cyc_pr_comment(Cyc_stdout,(struct _fat_ptr)cmt);});
({void*_tmp57=0U;({struct _fat_ptr _tmpDD=({const char*_tmp58="\n";_tag_fat(_tmp58,sizeof(char),2U);});Cyc_printf(_tmpDD,_tag_fat(_tmp57,sizeof(void*),0U));});});
lineno_comments=lineno_comments->tl;
goto _LL9;}}else{_LLC: _tmp55=((struct Cyc_MatchDecl_Comment_struct*)_tmp54)->f1;_LLD: {struct _fat_ptr cmt=_tmp55;
# 352
if(cline < dline){
# 354
lineno_comments=lineno_comments->tl;
continue;}
# 352
if(lineno_decls->tl != 0){
# 358
struct _tuple28*_stmttmp4=(struct _tuple28*)((struct Cyc_List_List*)_check_null(lineno_decls->tl))->hd;int _tmp59;_LLF: _tmp59=_stmttmp4->f1;_LL10: {int dline2=_tmp59;
if(dline2 < cline){
# 361
lineno_decls=lineno_decls->tl;
continue;}}}
# 352
({Cyc_dumpdecl(decl,(struct _fat_ptr)cmt);});
# 368
({Cyc_fflush(Cyc_stdout);});
lineno_decls=lineno_decls->tl;
lineno_comments=lineno_comments->tl;
goto _LL9;}}_LL9:;}}}}}}}}}}}
# 377
void GC_blacklist_warn_clear();struct _tuple30{struct _fat_ptr f1;int f2;struct _fat_ptr f3;void*f4;struct _fat_ptr f5;};
# 379
int Cyc_main(int argc,struct _fat_ptr argv){
# 384
({GC_blacklist_warn_clear();});{
# 386
struct Cyc_List_List*options=({struct _tuple30*_tmp90[5U];({
struct _tuple30*_tmpF6=({struct _tuple30*_tmp95=_cycalloc(sizeof(*_tmp95));({struct _fat_ptr _tmpF5=({const char*_tmp91="-cyclone";_tag_fat(_tmp91,sizeof(char),9U);});_tmp95->f1=_tmpF5;}),_tmp95->f2=0,({struct _fat_ptr _tmpF4=({const char*_tmp92=" <file>";_tag_fat(_tmp92,sizeof(char),8U);});_tmp95->f3=_tmpF4;}),({
void*_tmpF3=(void*)({struct Cyc_Arg_String_spec_Arg_Spec_struct*_tmp93=_cycalloc(sizeof(*_tmp93));_tmp93->tag=5U,_tmp93->f1=Cyc_set_cyclone_file;_tmp93;});_tmp95->f4=_tmpF3;}),({
struct _fat_ptr _tmpF2=({const char*_tmp94="Use <file> as the cyclone compiler";_tag_fat(_tmp94,sizeof(char),35U);});_tmp95->f5=_tmpF2;});_tmp95;});
# 387
_tmp90[0]=_tmpF6;}),({
# 390
struct _tuple30*_tmpF1=({struct _tuple30*_tmp9A=_cycalloc(sizeof(*_tmp9A));({struct _fat_ptr _tmpF0=({const char*_tmp96="-w";_tag_fat(_tmp96,sizeof(char),3U);});_tmp9A->f1=_tmpF0;}),_tmp9A->f2=0,({struct _fat_ptr _tmpEF=({const char*_tmp97=" <width>";_tag_fat(_tmp97,sizeof(char),9U);});_tmp9A->f3=_tmpEF;}),({
void*_tmpEE=(void*)({struct Cyc_Arg_Int_spec_Arg_Spec_struct*_tmp98=_cycalloc(sizeof(*_tmp98));_tmp98->tag=6U,_tmp98->f1=Cyc_set_width;_tmp98;});_tmp9A->f4=_tmpEE;}),({
struct _fat_ptr _tmpED=({const char*_tmp99="Use <width> as the max width for printing declarations";_tag_fat(_tmp99,sizeof(char),55U);});_tmp9A->f5=_tmpED;});_tmp9A;});
# 390
_tmp90[1]=_tmpF1;}),({
# 393
struct _tuple30*_tmpEC=({struct _tuple30*_tmp9F=_cycalloc(sizeof(*_tmp9F));({struct _fat_ptr _tmpEB=({const char*_tmp9B="-D";_tag_fat(_tmp9B,sizeof(char),3U);});_tmp9F->f1=_tmpEB;}),_tmp9F->f2=1,({struct _fat_ptr _tmpEA=({const char*_tmp9C="<name>[=<value>]";_tag_fat(_tmp9C,sizeof(char),17U);});_tmp9F->f3=_tmpEA;}),({
void*_tmpE9=(void*)({struct Cyc_Arg_Flag_spec_Arg_Spec_struct*_tmp9D=_cycalloc(sizeof(*_tmp9D));_tmp9D->tag=1U,_tmp9D->f1=Cyc_add_cycarg;_tmp9D;});_tmp9F->f4=_tmpE9;}),({
struct _fat_ptr _tmpE8=({const char*_tmp9E="Pass definition to preprocessor";_tag_fat(_tmp9E,sizeof(char),32U);});_tmp9F->f5=_tmpE8;});_tmp9F;});
# 393
_tmp90[2]=_tmpEC;}),({
# 396
struct _tuple30*_tmpE7=({struct _tuple30*_tmpA4=_cycalloc(sizeof(*_tmpA4));({struct _fat_ptr _tmpE6=({const char*_tmpA0="-I";_tag_fat(_tmpA0,sizeof(char),3U);});_tmpA4->f1=_tmpE6;}),_tmpA4->f2=1,({struct _fat_ptr _tmpE5=({const char*_tmpA1="<dir>";_tag_fat(_tmpA1,sizeof(char),6U);});_tmpA4->f3=_tmpE5;}),({
void*_tmpE4=(void*)({struct Cyc_Arg_Flag_spec_Arg_Spec_struct*_tmpA2=_cycalloc(sizeof(*_tmpA2));_tmpA2->tag=1U,_tmpA2->f1=Cyc_add_cycarg;_tmpA2;});_tmpA4->f4=_tmpE4;}),({
struct _fat_ptr _tmpE3=({const char*_tmpA3="Add to the list of directories to search for include files";_tag_fat(_tmpA3,sizeof(char),59U);});_tmpA4->f5=_tmpE3;});_tmpA4;});
# 396
_tmp90[3]=_tmpE7;}),({
# 399
struct _tuple30*_tmpE2=({struct _tuple30*_tmpA9=_cycalloc(sizeof(*_tmpA9));({struct _fat_ptr _tmpE1=({const char*_tmpA5="-B";_tag_fat(_tmpA5,sizeof(char),3U);});_tmpA9->f1=_tmpE1;}),_tmpA9->f2=1,({struct _fat_ptr _tmpE0=({const char*_tmpA6="<file>";_tag_fat(_tmpA6,sizeof(char),7U);});_tmpA9->f3=_tmpE0;}),({
void*_tmpDF=(void*)({struct Cyc_Arg_Flag_spec_Arg_Spec_struct*_tmpA7=_cycalloc(sizeof(*_tmpA7));_tmpA7->tag=1U,_tmpA7->f1=Cyc_add_cycarg;_tmpA7;});_tmpA9->f4=_tmpDF;}),({
struct _fat_ptr _tmpDE=({const char*_tmpA8="Add to the list of directories to search for compiler files";_tag_fat(_tmpA8,sizeof(char),60U);});_tmpA9->f5=_tmpDE;});_tmpA9;});
# 399
_tmp90[4]=_tmpE2;});Cyc_List_list(_tag_fat(_tmp90,sizeof(struct _tuple30*),5U));});
# 404
({({struct Cyc_List_List*_tmpF8=options;struct _fat_ptr _tmpF7=({const char*_tmp8E="Options:";_tag_fat(_tmp8E,sizeof(char),9U);});Cyc_Arg_parse(_tmpF8,Cyc_add_other,Cyc_no_other,_tmpF7,argv);});});
# 406
if(Cyc_cycdoc_files == 0){
({({struct Cyc_List_List*_tmpF9=options;Cyc_Arg_usage(_tmpF9,({const char*_tmp8F="Usage: cycdoc [options] file1 file2 ...\nOptions:";_tag_fat(_tmp8F,sizeof(char),49U);}));});});
# 410
({exit(1);});}
# 406
Cyc_PP_tex_output=1;
# 417
({Cyc_Absynpp_set_params(& Cyc_Absynpp_tc_params_r);});
Cyc_Absynpp_print_for_cycdoc=1;
# 420
{struct Cyc_List_List*l=({Cyc_List_rev(Cyc_cycdoc_files);});for(0;l != 0;l=l->tl){
({Cyc_process_file(*((struct _fat_ptr*)l->hd));});}}
# 424
return 0;}}
