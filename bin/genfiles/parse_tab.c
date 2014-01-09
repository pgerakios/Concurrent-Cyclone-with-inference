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

# 6 "parse_tab.cyc"
 void(*Cyc_beforedie)()=0;struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc___cycFILE;
# 53 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stderr;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Lexing_Error[6U];struct Cyc_Lexing_Error_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Lexing_lexbuf{void(*refill_buff)(struct Cyc_Lexing_lexbuf*);void*refill_state;struct _fat_ptr lex_buffer;int lex_buffer_len;int lex_abs_pos;int lex_start_pos;int lex_curr_pos;int lex_last_pos;int lex_last_action;int lex_eof_reached;};struct Cyc_Lexing_function_lexbuf_state{int(*read_fun)(struct _fat_ptr,int,void*);void*read_fun_state;};struct Cyc_Lexing_lex_tables{struct _fat_ptr lex_base;struct _fat_ptr lex_backtrk;struct _fat_ptr lex_default;struct _fat_ptr lex_trans;struct _fat_ptr lex_check;};
# 78 "lexing.h"
struct Cyc_Lexing_lexbuf*Cyc_Lexing_from_file(struct Cyc___cycFILE*);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 61
int Cyc_List_length(struct Cyc_List_List*x);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);
# 83
struct Cyc_List_List*Cyc_List_map_c(void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 135
void Cyc_List_iter_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*x);
# 153
void*Cyc_List_fold_right(void*(*f)(void*,void*),struct Cyc_List_List*x,void*accum);
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 195
struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
# 52
int Cyc_zstrcmp(struct _fat_ptr,struct _fat_ptr);
# 54
int Cyc_zstrptrcmp(struct _fat_ptr*,struct _fat_ptr*);
# 60
struct _fat_ptr Cyc_strcat(struct _fat_ptr dest,struct _fat_ptr src);
# 71
struct _fat_ptr Cyc_strcpy(struct _fat_ptr dest,struct _fat_ptr src);struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};
# 110 "absyn.h"
extern union Cyc_Absyn_Nmspace Cyc_Absyn_Loc_n;
union Cyc_Absyn_Nmspace Cyc_Absyn_Rel_n(struct Cyc_List_List*);struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};
# 346
union Cyc_Absyn_DatatypeInfo Cyc_Absyn_UnknownDatatype(struct Cyc_Absyn_UnknownDatatypeInfo);struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};
# 359
union Cyc_Absyn_DatatypeFieldInfo Cyc_Absyn_UnknownDatatypefield(struct Cyc_Absyn_UnknownDatatypeFieldInfo);struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};
# 366
union Cyc_Absyn_AggrInfo Cyc_Absyn_UnknownAggr(enum Cyc_Absyn_AggrKind,struct _tuple0*,struct Cyc_Core_Opt*);struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};
# 522
extern struct Cyc_Absyn_Const_att_Absyn_Attribute_struct Cyc_Absyn_Const_att_val;struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 771 "absyn.h"
extern struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct Cyc_Absyn_Wild_p_val;
extern struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct Cyc_Absyn_Null_p_val;struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};
# 930
extern struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct Cyc_Absyn_Porton_d_val;
extern struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct Cyc_Absyn_Portoff_d_val;
extern struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct Cyc_Absyn_Tempeston_d_val;
extern struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct Cyc_Absyn_Tempestoff_d_val;struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 959
int Cyc_Absyn_is_qvar_qualified(struct _tuple0*);
# 963
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
struct Cyc_Absyn_Tqual Cyc_Absyn_combine_tqual(struct Cyc_Absyn_Tqual,struct Cyc_Absyn_Tqual);
# 968
void*Cyc_Absyn_compress_kb(void*);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 981
void*Cyc_Absyn_wildtyp(struct Cyc_Core_Opt*);
void*Cyc_Absyn_int_type(enum Cyc_Absyn_Sign,enum Cyc_Absyn_Size_of);
# 984
extern void*Cyc_Absyn_char_type;extern void*Cyc_Absyn_uint_type;
# 986
extern void*Cyc_Absyn_sint_type;
# 988
extern void*Cyc_Absyn_float_type;extern void*Cyc_Absyn_double_type;extern void*Cyc_Absyn_long_double_type;
# 991
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_unique_rgn_type;extern void*Cyc_Absyn_refcnt_rgn_type;
# 995
extern void*Cyc_Absyn_true_type;extern void*Cyc_Absyn_false_type;
# 997
extern void*Cyc_Absyn_void_type;void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);void*Cyc_Absyn_tag_type(void*);void*Cyc_Absyn_rgn_handle_type(void*);void*Cyc_Absyn_valueof_type(struct Cyc_Absyn_Exp*);void*Cyc_Absyn_typeof_type(struct Cyc_Absyn_Exp*);void*Cyc_Absyn_access_eff(void*);void*Cyc_Absyn_join_eff(struct Cyc_List_List*);void*Cyc_Absyn_regionsof_eff(void*);void*Cyc_Absyn_enum_type(struct _tuple0*n,struct Cyc_Absyn_Enumdecl*d);void*Cyc_Absyn_anon_enum_type(struct Cyc_List_List*);void*Cyc_Absyn_builtin_type(struct _fat_ptr s,struct Cyc_Absyn_Kind*k);void*Cyc_Absyn_typedef_type(struct _tuple0*,struct Cyc_List_List*,struct Cyc_Absyn_Typedefdecl*,void*);
# 1022
extern void*Cyc_Absyn_fat_bound_type;
void*Cyc_Absyn_thin_bounds_type(void*);
void*Cyc_Absyn_thin_bounds_exp(struct Cyc_Absyn_Exp*);
# 1026
void*Cyc_Absyn_bounds_one();
# 1028
void*Cyc_Absyn_pointer_type(struct Cyc_Absyn_PtrInfo);
# 1050
void*Cyc_Absyn_array_type(void*elt_type,struct Cyc_Absyn_Tqual tq,struct Cyc_Absyn_Exp*num_elts,void*zero_term,unsigned ztloc);
# 1053
void*Cyc_Absyn_datatype_type(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*args);
void*Cyc_Absyn_datatype_field_type(union Cyc_Absyn_DatatypeFieldInfo,struct Cyc_List_List*args);
void*Cyc_Absyn_aggr_type(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args);
# 1058
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_New_exp(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,unsigned);
# 1061
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_null_exp(unsigned);
# 1064
struct Cyc_Absyn_Exp*Cyc_Absyn_true_exp(unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_false_exp(unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_int_exp(enum Cyc_Absyn_Sign,int,unsigned);
# 1068
struct Cyc_Absyn_Exp*Cyc_Absyn_uint_exp(unsigned,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_char_exp(char,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_wchar_exp(struct _fat_ptr,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_float_exp(struct _fat_ptr,int,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_string_exp(struct _fat_ptr,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_wstring_exp(struct _fat_ptr,unsigned);
# 1076
struct Cyc_Absyn_Exp*Cyc_Absyn_unknownid_exp(struct _tuple0*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_pragma_exp(struct _fat_ptr,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_primop_exp(enum Cyc_Absyn_Primop,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_prim1_exp(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_prim2_exp(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_swap_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1085
struct Cyc_Absyn_Exp*Cyc_Absyn_eq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_neq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_gt_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_lt_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_gte_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_lte_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_assignop_exp(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned);
# 1093
struct Cyc_Absyn_Exp*Cyc_Absyn_increment_exp(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_conditional_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_and_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_or_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_unknowncall_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
# 1100
struct Cyc_Absyn_Exp*Cyc_Absyn_throw_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1102
struct Cyc_Absyn_Exp*Cyc_Absyn_noinstantiate_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_instantiate_exp(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_cast_exp(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_address_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*t,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeofexp_exp(struct Cyc_Absyn_Exp*e,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_offsetof_exp(void*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrmember_exp(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrarrow_exp(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_tuple_exp(struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,unsigned);
# 1117
struct Cyc_Absyn_Exp*Cyc_Absyn_valueof_exp(void*,unsigned);
# 1120
struct Cyc_Absyn_Exp*Cyc_Absyn_extension_exp(struct Cyc_Absyn_Exp*,unsigned);
# 1127
struct Cyc_Absyn_Stmt*Cyc_Absyn_new_stmt(void*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_skip_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
# 1132
struct Cyc_Absyn_Stmt*Cyc_Absyn_return_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_ifthenelse_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_while_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_break_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_continue_stmt(unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_for_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_switch_stmt(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_fallthru_stmt(struct Cyc_List_List*,unsigned);
# 1143
struct Cyc_Absyn_Stmt*Cyc_Absyn_do_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_goto_stmt(struct _fat_ptr*,unsigned);
# 1146
struct Cyc_Absyn_Stmt*Cyc_Absyn_trycatch_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_List_List*,unsigned);
# 1149
struct Cyc_Absyn_Pat*Cyc_Absyn_new_pat(void*,unsigned);
struct Cyc_Absyn_Pat*Cyc_Absyn_exp_pat(struct Cyc_Absyn_Exp*);
# 1153
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*,unsigned);
struct Cyc_Absyn_Decl*Cyc_Absyn_let_decl(struct Cyc_Absyn_Pat*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Decl*Cyc_Absyn_letv_decl(struct Cyc_List_List*,unsigned);
struct Cyc_Absyn_Decl*Cyc_Absyn_region_decl(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*open_exp,unsigned);
# 1158
struct Cyc_Absyn_Vardecl*Cyc_Absyn_new_vardecl(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init);
# 1160
struct Cyc_Absyn_AggrdeclImpl*Cyc_Absyn_aggrdecl_impl(struct Cyc_List_List*exists,struct Cyc_List_List*po,struct Cyc_List_List*fs,int tagged);
# 1167
struct Cyc_Absyn_TypeDecl*Cyc_Absyn_aggr_tdecl(enum Cyc_Absyn_AggrKind,enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Absyn_AggrdeclImpl*,struct Cyc_List_List*,unsigned);
# 1174
struct Cyc_Absyn_Decl*Cyc_Absyn_datatype_decl(enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Core_Opt*fs,int is_extensible,unsigned);
# 1177
struct Cyc_Absyn_TypeDecl*Cyc_Absyn_datatype_tdecl(enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Core_Opt*fs,int is_extensible,unsigned);
# 1181
void*Cyc_Absyn_function_type(struct Cyc_List_List*tvs,void*eff_typ,struct Cyc_Absyn_Tqual ret_tqual,void*ret_type,struct Cyc_List_List*args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,struct Cyc_List_List*rgn_po,struct Cyc_List_List*,struct Cyc_Absyn_Exp*requires_clause,struct Cyc_Absyn_Exp*ensures_clause,struct Cyc_List_List*,struct Cyc_List_List*,struct Cyc_List_List*,int);
# 1215
int Cyc_Absyn_fntype_att(void*);
# 1237
extern int Cyc_Absyn_porting_c_code;
# 1264
void*Cyc_Absyn_parse_nullary_att(unsigned loc,struct _fat_ptr s);
void*Cyc_Absyn_parse_unary_att(unsigned sloc,struct _fat_ptr s,unsigned eloc,struct Cyc_Absyn_Exp*e);
# 1267
void*Cyc_Absyn_parse_format_att(unsigned loc,unsigned s2loc,struct _fat_ptr s1,struct _fat_ptr s2,unsigned u1,unsigned u2);
# 1286
void*Cyc_Absyn_xrgn_var_type(struct Cyc_Absyn_Tvar*tv);
# 1328
struct Cyc_List_List*Cyc_Absyn_parse_rgneffects(unsigned loc,void*t);
# 1373
struct Cyc_List_List*Cyc_Absyn_throwsany();
# 1375
struct Cyc_List_List*Cyc_Absyn_nothrow();
# 29 "warn.h"
void Cyc_Warn_warn(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 33
void Cyc_Warn_verr(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 35
void Cyc_Warn_err(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 48 "tcutil.h"
int Cyc_Tcutil_is_array_type(void*);
# 97
void*Cyc_Tcutil_copy_type(void*);
# 110
void*Cyc_Tcutil_compress(void*);
# 138
extern struct Cyc_Absyn_Kind Cyc_Tcutil_xrk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_mk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ek;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ik;
# 148
extern struct Cyc_Absyn_Kind Cyc_Tcutil_trk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tbk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_tmk;
# 153
extern struct Cyc_Absyn_Kind Cyc_Tcutil_urk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_uak;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ubk;
extern struct Cyc_Absyn_Kind Cyc_Tcutil_umk;
# 158
extern struct Cyc_Core_Opt Cyc_Tcutil_rko;
# 160
extern struct Cyc_Core_Opt Cyc_Tcutil_bko;
# 162
extern struct Cyc_Core_Opt Cyc_Tcutil_iko;
# 167
extern struct Cyc_Core_Opt Cyc_Tcutil_trko;
# 177
struct Cyc_Core_Opt*Cyc_Tcutil_kind_to_opt(struct Cyc_Absyn_Kind*k);
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);
# 261
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 303
void*Cyc_Tcutil_promote_array(void*t,void*rgn,int convert_tag);
# 313
void*Cyc_Tcutil_any_bool(struct Cyc_List_List*);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 66 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_cnst2string(union Cyc_Absyn_Cnst);
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_stmt2string(struct Cyc_Absyn_Stmt*);
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);
# 64 "parse.y"
extern void(*Cyc_beforedie)();
# 70
void Cyc_Lex_register_typedef(struct _tuple0*s);
void Cyc_Lex_enter_namespace(struct _fat_ptr*);
void Cyc_Lex_leave_namespace();
void Cyc_Lex_enter_using(struct _tuple0*);
void Cyc_Lex_leave_using();
void Cyc_Lex_enter_extern_c();
void Cyc_Lex_leave_extern_c();
extern struct _tuple0*Cyc_Lex_token_qvar;
extern struct _fat_ptr Cyc_Lex_token_string;
# 96 "parse.y"
int Cyc_Parse_parsing_tempest=0;struct Cyc_Parse_FlatList{struct Cyc_Parse_FlatList*tl;void*hd[0U] __attribute__((aligned )) ;};
# 102
struct Cyc_Parse_FlatList*Cyc_Parse_flat_imp_rev(struct Cyc_Parse_FlatList*x){
if(x == 0)return x;else{
# 105
struct Cyc_Parse_FlatList*first=x;
struct Cyc_Parse_FlatList*second=x->tl;
x->tl=0;
while(second != 0){
struct Cyc_Parse_FlatList*temp=second->tl;
second->tl=first;
first=second;
second=temp;}
# 114
return first;}}
# 102
int Cyc_Parse_no_register=0;char Cyc_Parse_Exit[5U]="Exit";struct Cyc_Parse_Exit_exn_struct{char*tag;};struct Cyc_Parse_Type_specifier{int Signed_spec: 1;int Unsigned_spec: 1;int Short_spec: 1;int Long_spec: 1;int Long_Long_spec: 1;int Valid_type_spec: 1;void*Type_spec;unsigned loc;};
# 136
enum Cyc_Parse_Storage_class{Cyc_Parse_Typedef_sc =0U,Cyc_Parse_Extern_sc =1U,Cyc_Parse_ExternC_sc =2U,Cyc_Parse_Static_sc =3U,Cyc_Parse_Auto_sc =4U,Cyc_Parse_Register_sc =5U,Cyc_Parse_Abstract_sc =6U};struct Cyc_Parse_Declaration_spec{enum Cyc_Parse_Storage_class*sc;struct Cyc_Absyn_Tqual tq;struct Cyc_Parse_Type_specifier type_specs;int is_inline;struct Cyc_List_List*attributes;};struct Cyc_Parse_Declarator{struct _tuple0*id;unsigned varloc;struct Cyc_List_List*tms;};struct _tuple12{struct _tuple12*tl;struct Cyc_Parse_Declarator hd  __attribute__((aligned )) ;};struct _tuple13{struct Cyc_Parse_Declarator f1;struct Cyc_Absyn_Exp*f2;};struct _tuple14{struct _tuple14*tl;struct _tuple13 hd  __attribute__((aligned )) ;};struct Cyc_Parse_Numelts_ptrqual_Parse_Pointer_qual_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Parse_Region_ptrqual_Parse_Pointer_qual_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Parse_Thin_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Fat_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Zeroterm_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Nozeroterm_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Notnull_ptrqual_Parse_Pointer_qual_struct{int tag;};struct Cyc_Parse_Nullable_ptrqual_Parse_Pointer_qual_struct{int tag;};
# 173
static void Cyc_Parse_decl_split(struct _RegionHandle*r,struct _tuple14*ds,struct _tuple12**decls,struct Cyc_List_List**es){
# 176
struct _tuple12*declarators=0;
struct Cyc_List_List*exprs=0;
for(0;ds != 0;ds=ds->tl){
struct _tuple13 _stmttmp0=ds->hd;struct Cyc_Absyn_Exp*_tmp2;struct Cyc_Parse_Declarator _tmp1;_LL1: _tmp1=_stmttmp0.f1;_tmp2=_stmttmp0.f2;_LL2: {struct Cyc_Parse_Declarator d=_tmp1;struct Cyc_Absyn_Exp*e=_tmp2;
declarators=({struct _tuple12*_tmp3=_region_malloc(r,sizeof(*_tmp3));_tmp3->tl=declarators,_tmp3->hd=d;_tmp3;});
exprs=({struct Cyc_List_List*_tmp4=_region_malloc(r,sizeof(*_tmp4));_tmp4->hd=e,_tmp4->tl=exprs;_tmp4;});}}
# 183
({struct Cyc_List_List*_tmp1027=({Cyc_List_imp_rev(exprs);});*es=_tmp1027;});
({struct _tuple12*_tmp1029=({({struct _tuple12*(*_tmp1028)(struct _tuple12*x)=({struct _tuple12*(*_tmp5)(struct _tuple12*x)=(struct _tuple12*(*)(struct _tuple12*x))Cyc_Parse_flat_imp_rev;_tmp5;});_tmp1028(declarators);});});*decls=_tmp1029;});}struct Cyc_Parse_Abstractdeclarator{struct Cyc_List_List*tms;};
# 193
static void*Cyc_Parse_collapse_type_specifiers(struct Cyc_Parse_Type_specifier ts,unsigned loc);struct _tuple15{struct Cyc_Absyn_Tqual f1;void*f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;};
static struct _tuple15 Cyc_Parse_apply_tms(struct Cyc_Absyn_Tqual,void*,struct Cyc_List_List*,struct Cyc_List_List*);
# 199
static struct Cyc_List_List*Cyc_Parse_parse_result=0;
# 201
static void*Cyc_Parse_parse_abort(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap){
# 203
({Cyc_Warn_verr(loc,fmt,ap);});
(int)_throw(({struct Cyc_Parse_Exit_exn_struct*_tmp7=_cycalloc(sizeof(*_tmp7));_tmp7->tag=Cyc_Parse_Exit;_tmp7;}));}
# 207
static void*Cyc_Parse_type_name_to_type(struct _tuple9*tqt,unsigned loc){
# 209
void*_tmpA;struct Cyc_Absyn_Tqual _tmp9;_LL1: _tmp9=tqt->f2;_tmpA=tqt->f3;_LL2: {struct Cyc_Absyn_Tqual tq=_tmp9;void*t=_tmpA;
if((tq.print_const || tq.q_volatile)|| tq.q_restrict){
if(tq.loc != (unsigned)0)loc=tq.loc;({void*_tmpB=0U;({unsigned _tmp102B=loc;struct _fat_ptr _tmp102A=({const char*_tmpC="qualifier on type is ignored";_tag_fat(_tmpC,sizeof(char),29U);});Cyc_Warn_warn(_tmp102B,_tmp102A,_tag_fat(_tmpB,sizeof(void*),0U));});});}
# 210
return t;}}
# 241 "parse.y"
static void*Cyc_Parse_make_pointer_mod(struct _RegionHandle*r,struct Cyc_Absyn_PtrLoc*loc,void*nullable,void*bound,void*eff,struct Cyc_List_List*pqs,struct Cyc_Absyn_Tqual tqs){
# 246
void*zeroterm=({Cyc_Tcutil_any_bool(0);});
for(0;pqs != 0;pqs=pqs->tl){
void*_stmttmp1=(void*)pqs->hd;void*_tmpE=_stmttmp1;struct Cyc_List_List*_tmpF;struct Cyc_Absyn_Exp*_tmp10;switch(*((int*)_tmpE)){case 4U: _LL1: _LL2:
# 250
 zeroterm=Cyc_Absyn_true_type;goto _LL0;case 5U: _LL3: _LL4:
 zeroterm=Cyc_Absyn_false_type;goto _LL0;case 7U: _LL5: _LL6:
 nullable=Cyc_Absyn_true_type;goto _LL0;case 6U: _LL7: _LL8:
 nullable=Cyc_Absyn_false_type;goto _LL0;case 3U: _LL9: _LLA:
 bound=Cyc_Absyn_fat_bound_type;goto _LL0;case 2U: _LLB: _LLC:
 bound=({Cyc_Absyn_bounds_one();});goto _LL0;case 0U: _LLD: _tmp10=((struct Cyc_Parse_Numelts_ptrqual_Parse_Pointer_qual_struct*)_tmpE)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmp10;
bound=({Cyc_Absyn_thin_bounds_exp(e);});goto _LL0;}default: _LLF: _tmpF=((struct Cyc_Parse_Region_ptrqual_Parse_Pointer_qual_struct*)_tmpE)->f1;_LL10: {struct Cyc_List_List*ts=_tmpF;
eff=({Cyc_Absyn_join_eff(ts);});goto _LL0;}}_LL0:;}
# 259
return(void*)({struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*_tmp11=_region_malloc(r,sizeof(*_tmp11));_tmp11->tag=2U,(_tmp11->f1).rgn=eff,(_tmp11->f1).nullable=nullable,(_tmp11->f1).bounds=bound,(_tmp11->f1).zero_term=zeroterm,(_tmp11->f1).ptrloc=loc,_tmp11->f2=tqs;_tmp11;});}
# 241
struct _tuple0*Cyc_Parse_gensym_enum(){
# 270
static int enum_counter=0;
return({struct _tuple0*_tmp17=_cycalloc(sizeof(*_tmp17));({union Cyc_Absyn_Nmspace _tmp102F=({Cyc_Absyn_Rel_n(0);});_tmp17->f1=_tmp102F;}),({
struct _fat_ptr*_tmp102E=({struct _fat_ptr*_tmp16=_cycalloc(sizeof(*_tmp16));({struct _fat_ptr _tmp102D=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp15=({struct Cyc_Int_pa_PrintArg_struct _tmpF82;_tmpF82.tag=1U,_tmpF82.f1=(unsigned long)enum_counter ++;_tmpF82;});void*_tmp13[1U];_tmp13[0]=& _tmp15;({struct _fat_ptr _tmp102C=({const char*_tmp14="__anonymous_enum_%d__";_tag_fat(_tmp14,sizeof(char),22U);});Cyc_aprintf(_tmp102C,_tag_fat(_tmp13,sizeof(void*),1U));});});*_tmp16=_tmp102D;});_tmp16;});_tmp17->f2=_tmp102E;});_tmp17;});}struct _tuple16{unsigned f1;struct _tuple0*f2;struct Cyc_Absyn_Tqual f3;void*f4;struct Cyc_List_List*f5;struct Cyc_List_List*f6;};struct _tuple17{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple18{struct _tuple16*f1;struct _tuple17*f2;};
# 275
static struct Cyc_Absyn_Aggrfield*Cyc_Parse_make_aggr_field(unsigned loc,struct _tuple18*field_info){
# 280
struct Cyc_Absyn_Exp*_tmp20;struct Cyc_Absyn_Exp*_tmp1F;struct Cyc_List_List*_tmp1E;struct Cyc_List_List*_tmp1D;void*_tmp1C;struct Cyc_Absyn_Tqual _tmp1B;struct _tuple0*_tmp1A;unsigned _tmp19;_LL1: _tmp19=(field_info->f1)->f1;_tmp1A=(field_info->f1)->f2;_tmp1B=(field_info->f1)->f3;_tmp1C=(field_info->f1)->f4;_tmp1D=(field_info->f1)->f5;_tmp1E=(field_info->f1)->f6;_tmp1F=(field_info->f2)->f1;_tmp20=(field_info->f2)->f2;_LL2: {unsigned varloc=_tmp19;struct _tuple0*qid=_tmp1A;struct Cyc_Absyn_Tqual tq=_tmp1B;void*t=_tmp1C;struct Cyc_List_List*tvs=_tmp1D;struct Cyc_List_List*atts=_tmp1E;struct Cyc_Absyn_Exp*widthopt=_tmp1F;struct Cyc_Absyn_Exp*reqopt=_tmp20;
if(tvs != 0)
({void*_tmp21=0U;({unsigned _tmp1031=loc;struct _fat_ptr _tmp1030=({const char*_tmp22="bad type params in struct field";_tag_fat(_tmp22,sizeof(char),32U);});Cyc_Warn_err(_tmp1031,_tmp1030,_tag_fat(_tmp21,sizeof(void*),0U));});});
# 281
if(({Cyc_Absyn_is_qvar_qualified(qid);}))
# 284
({void*_tmp23=0U;({unsigned _tmp1033=loc;struct _fat_ptr _tmp1032=({const char*_tmp24="struct or union field cannot be qualified with a namespace";_tag_fat(_tmp24,sizeof(char),59U);});Cyc_Warn_err(_tmp1033,_tmp1032,_tag_fat(_tmp23,sizeof(void*),0U));});});
# 281
return({struct Cyc_Absyn_Aggrfield*_tmp25=_cycalloc(sizeof(*_tmp25));
# 285
_tmp25->name=(*qid).f2,_tmp25->tq=tq,_tmp25->type=t,_tmp25->width=widthopt,_tmp25->attributes=atts,_tmp25->requires_clause=reqopt;_tmp25;});}}
# 290
static struct Cyc_Parse_Type_specifier Cyc_Parse_empty_spec(unsigned loc){
return({struct Cyc_Parse_Type_specifier _tmpF83;_tmpF83.Signed_spec=0,_tmpF83.Unsigned_spec=0,_tmpF83.Short_spec=0,_tmpF83.Long_spec=0,_tmpF83.Long_Long_spec=0,_tmpF83.Valid_type_spec=0,_tmpF83.Type_spec=Cyc_Absyn_sint_type,_tmpF83.loc=loc;_tmpF83;});}
# 301
static struct Cyc_Parse_Type_specifier Cyc_Parse_type_spec(void*t,unsigned loc){
struct Cyc_Parse_Type_specifier s=({Cyc_Parse_empty_spec(loc);});
s.Type_spec=t;
s.Valid_type_spec=1;
return s;}
# 307
static struct Cyc_Parse_Type_specifier Cyc_Parse_signed_spec(unsigned loc){
struct Cyc_Parse_Type_specifier s=({Cyc_Parse_empty_spec(loc);});
s.Signed_spec=1;
return s;}
# 312
static struct Cyc_Parse_Type_specifier Cyc_Parse_unsigned_spec(unsigned loc){
struct Cyc_Parse_Type_specifier s=({Cyc_Parse_empty_spec(loc);});
s.Unsigned_spec=1;
return s;}
# 317
static struct Cyc_Parse_Type_specifier Cyc_Parse_short_spec(unsigned loc){
struct Cyc_Parse_Type_specifier s=({Cyc_Parse_empty_spec(loc);});
s.Short_spec=1;
return s;}
# 322
static struct Cyc_Parse_Type_specifier Cyc_Parse_long_spec(unsigned loc){
struct Cyc_Parse_Type_specifier s=({Cyc_Parse_empty_spec(loc);});
s.Long_spec=1;
return s;}
# 329
static void*Cyc_Parse_array2ptr(void*t,int argposn){
# 331
return({Cyc_Tcutil_is_array_type(t);})?({void*(*_tmp2D)(void*t,void*rgn,int convert_tag)=Cyc_Tcutil_promote_array;void*_tmp2E=t;void*_tmp2F=
argposn?({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,0);}): Cyc_Absyn_heap_rgn_type;int _tmp30=0;_tmp2D(_tmp2E,_tmp2F,_tmp30);}): t;}struct _tuple19{struct _fat_ptr*f1;void*f2;};
# 344 "parse.y"
static struct Cyc_List_List*Cyc_Parse_get_arg_tags(struct Cyc_List_List*x){
struct Cyc_List_List*res=0;
for(0;x != 0;x=x->tl){
struct _tuple9*_stmttmp2=(struct _tuple9*)x->hd;struct _tuple9*_tmp32=_stmttmp2;void**_tmp34;struct _fat_ptr _tmp33;void*_tmp36;struct _fat_ptr*_tmp35;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple9*)_tmp32)->f3)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple9*)_tmp32)->f3)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple9*)_tmp32)->f3)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple9*)_tmp32)->f3)->f2)->tl == 0){_LL1: _tmp35=_tmp32->f1;_tmp36=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp32->f3)->f2)->hd;if(_tmp35 != 0){_LL2: {struct _fat_ptr*v=_tmp35;void*i=_tmp36;
# 349
{void*_tmp37=i;void**_tmp38;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp37)->tag == 1U){_LL8: _tmp38=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp37)->f2;_LL9: {void**z=_tmp38;
# 353
struct _fat_ptr*nm=({struct _fat_ptr*_tmp3E=_cycalloc(sizeof(*_tmp3E));({struct _fat_ptr _tmp1035=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp3D=({struct Cyc_String_pa_PrintArg_struct _tmpF84;_tmpF84.tag=0U,_tmpF84.f1=(struct _fat_ptr)((struct _fat_ptr)*v);_tmpF84;});void*_tmp3B[1U];_tmp3B[0]=& _tmp3D;({struct _fat_ptr _tmp1034=({const char*_tmp3C="`%s";_tag_fat(_tmp3C,sizeof(char),4U);});Cyc_aprintf(_tmp1034,_tag_fat(_tmp3B,sizeof(void*),1U));});});*_tmp3E=_tmp1035;});_tmp3E;});
({void*_tmp1037=({Cyc_Absyn_var_type(({struct Cyc_Absyn_Tvar*_tmp3A=_cycalloc(sizeof(*_tmp3A));_tmp3A->name=nm,_tmp3A->identity=- 1,({void*_tmp1036=(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmp39=_cycalloc(sizeof(*_tmp39));_tmp39->tag=0U,_tmp39->f1=& Cyc_Tcutil_ik;_tmp39;});_tmp3A->kind=_tmp1036;});_tmp3A;}));});*z=_tmp1037;});
goto _LL7;}}else{_LLA: _LLB:
 goto _LL7;}_LL7:;}
# 358
res=({struct Cyc_List_List*_tmp40=_cycalloc(sizeof(*_tmp40));({struct _tuple19*_tmp1038=({struct _tuple19*_tmp3F=_cycalloc(sizeof(*_tmp3F));_tmp3F->f1=v,_tmp3F->f2=i;_tmp3F;});_tmp40->hd=_tmp1038;}),_tmp40->tl=res;_tmp40;});goto _LL0;}}else{if(((struct _tuple9*)_tmp32)->f1 != 0)goto _LL5;else{goto _LL5;}}}else{if(((struct _tuple9*)_tmp32)->f1 != 0)goto _LL5;else{goto _LL5;}}}else{if(((struct _tuple9*)_tmp32)->f1 != 0)goto _LL5;else{goto _LL5;}}}else{if(((struct _tuple9*)_tmp32)->f1 != 0){if(((struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple9*)_tmp32)->f3)->f1)->tag == 3U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple9*)_tmp32)->f3)->f2 != 0){if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple9*)_tmp32)->f3)->f2)->hd)->tag == 1U){if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)((struct _tuple9*)_tmp32)->f3)->f2)->tl == 0){_LL3: _tmp33=*_tmp32->f1;_tmp34=(void**)&((struct Cyc_Absyn_Evar_Absyn_Type_struct*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp32->f3)->f2)->hd)->f2;_LL4: {struct _fat_ptr v=_tmp33;void**z=_tmp34;
# 362
struct _fat_ptr*nm=({struct _fat_ptr*_tmp46=_cycalloc(sizeof(*_tmp46));({struct _fat_ptr _tmp103A=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp45=({struct Cyc_String_pa_PrintArg_struct _tmpF85;_tmpF85.tag=0U,_tmpF85.f1=(struct _fat_ptr)((struct _fat_ptr)v);_tmpF85;});void*_tmp43[1U];_tmp43[0]=& _tmp45;({struct _fat_ptr _tmp1039=({const char*_tmp44="`%s";_tag_fat(_tmp44,sizeof(char),4U);});Cyc_aprintf(_tmp1039,_tag_fat(_tmp43,sizeof(void*),1U));});});*_tmp46=_tmp103A;});_tmp46;});
({void*_tmp103C=({Cyc_Absyn_var_type(({struct Cyc_Absyn_Tvar*_tmp42=_cycalloc(sizeof(*_tmp42));_tmp42->name=nm,_tmp42->identity=- 1,({void*_tmp103B=(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmp41=_cycalloc(sizeof(*_tmp41));_tmp41->tag=0U,_tmp41->f1=& Cyc_Tcutil_rk;_tmp41;});_tmp42->kind=_tmp103B;});_tmp42;}));});*z=_tmp103C;});
goto _LL0;}}else{goto _LL5;}}else{goto _LL5;}}else{goto _LL5;}}else{goto _LL5;}}else{goto _LL5;}}}else{if(((struct _tuple9*)_tmp32)->f1 != 0)goto _LL5;else{_LL5: _LL6:
 goto _LL0;}}_LL0:;}
# 368
return res;}
# 372
static struct Cyc_List_List*Cyc_Parse_get_aggrfield_tags(struct Cyc_List_List*x){
struct Cyc_List_List*res=0;
for(0;x != 0;x=x->tl){
void*_stmttmp3=((struct Cyc_Absyn_Aggrfield*)x->hd)->type;void*_tmp48=_stmttmp3;void*_tmp49;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48)->tag == 0U){if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48)->f2)->tl == 0){_LL1: _tmp49=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp48)->f2)->hd;_LL2: {void*i=_tmp49;
# 377
res=({struct Cyc_List_List*_tmp4B=_cycalloc(sizeof(*_tmp4B));({struct _tuple19*_tmp103D=({struct _tuple19*_tmp4A=_cycalloc(sizeof(*_tmp4A));_tmp4A->f1=((struct Cyc_Absyn_Aggrfield*)x->hd)->name,_tmp4A->f2=i;_tmp4A;});_tmp4B->hd=_tmp103D;}),_tmp4B->tl=res;_tmp4B;});goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 381
return res;}
# 385
static struct Cyc_Absyn_Exp*Cyc_Parse_substitute_tags_exp(struct Cyc_List_List*tags,struct Cyc_Absyn_Exp*e){
{void*_stmttmp4=e->r;void*_tmp4D=_stmttmp4;struct _fat_ptr*_tmp4E;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp4D)->tag == 1U){if(((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp4D)->f1)->tag == 0U){if(((((struct _tuple0*)((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp4D)->f1)->f1)->f1).Rel_n).tag == 1){if(((((struct _tuple0*)((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp4D)->f1)->f1)->f1).Rel_n).val == 0){_LL1: _tmp4E=(((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp4D)->f1)->f1)->f2;_LL2: {struct _fat_ptr*y=_tmp4E;
# 388
{struct Cyc_List_List*ts=tags;for(0;ts != 0;ts=ts->tl){
struct _tuple19*_stmttmp5=(struct _tuple19*)ts->hd;void*_tmp50;struct _fat_ptr*_tmp4F;_LL6: _tmp4F=_stmttmp5->f1;_tmp50=_stmttmp5->f2;_LL7: {struct _fat_ptr*x=_tmp4F;void*i=_tmp50;
if(({Cyc_strptrcmp(x,y);})== 0)
return({struct Cyc_Absyn_Exp*(*_tmp51)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp52=(void*)({struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*_tmp53=_cycalloc(sizeof(*_tmp53));_tmp53->tag=40U,({void*_tmp103E=({Cyc_Tcutil_copy_type(i);});_tmp53->f1=_tmp103E;});_tmp53;});unsigned _tmp54=e->loc;_tmp51(_tmp52,_tmp54);});}}}
# 393
goto _LL0;}}else{goto _LL3;}}else{goto _LL3;}}else{goto _LL3;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 396
return e;}
# 401
static void*Cyc_Parse_substitute_tags(struct Cyc_List_List*tags,void*t){
{void*_tmp56=t;struct Cyc_Absyn_Exp*_tmp57;void*_tmp58;struct Cyc_Absyn_PtrLoc*_tmp5F;void*_tmp5E;void*_tmp5D;void*_tmp5C;void*_tmp5B;struct Cyc_Absyn_Tqual _tmp5A;void*_tmp59;unsigned _tmp64;void*_tmp63;struct Cyc_Absyn_Exp*_tmp62;struct Cyc_Absyn_Tqual _tmp61;void*_tmp60;switch(*((int*)_tmp56)){case 4U: _LL1: _tmp60=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp56)->f1).elt_type;_tmp61=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp56)->f1).tq;_tmp62=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp56)->f1).num_elts;_tmp63=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp56)->f1).zero_term;_tmp64=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp56)->f1).zt_loc;_LL2: {void*et=_tmp60;struct Cyc_Absyn_Tqual tq=_tmp61;struct Cyc_Absyn_Exp*nelts=_tmp62;void*zt=_tmp63;unsigned ztloc=_tmp64;
# 404
struct Cyc_Absyn_Exp*nelts2=nelts;
if(nelts != 0)
nelts2=({Cyc_Parse_substitute_tags_exp(tags,nelts);});{
# 405
void*et2=({Cyc_Parse_substitute_tags(tags,et);});
# 409
if(nelts != nelts2 || et != et2)
return({Cyc_Absyn_array_type(et2,tq,nelts2,zt,ztloc);});
# 409
goto _LL0;}}case 3U: _LL3: _tmp59=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).elt_type;_tmp5A=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).elt_tq;_tmp5B=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).ptr_atts).rgn;_tmp5C=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).ptr_atts).nullable;_tmp5D=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).ptr_atts).bounds;_tmp5E=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).ptr_atts).zero_term;_tmp5F=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp56)->f1).ptr_atts).ptrloc;_LL4: {void*et=_tmp59;struct Cyc_Absyn_Tqual tq=_tmp5A;void*r=_tmp5B;void*n=_tmp5C;void*b=_tmp5D;void*zt=_tmp5E;struct Cyc_Absyn_PtrLoc*pl=_tmp5F;
# 413
void*et2=({Cyc_Parse_substitute_tags(tags,et);});
void*b2=({Cyc_Parse_substitute_tags(tags,b);});
if(et2 != et || b2 != b)
return({Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmpF86;_tmpF86.elt_type=et2,_tmpF86.elt_tq=tq,(_tmpF86.ptr_atts).rgn=r,(_tmpF86.ptr_atts).nullable=n,(_tmpF86.ptr_atts).bounds=b2,(_tmpF86.ptr_atts).zero_term=zt,(_tmpF86.ptr_atts).ptrloc=pl;_tmpF86;}));});
# 415
goto _LL0;}case 0U: if(((struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56)->f1)->tag == 15U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56)->f2)->tl == 0){_LL5: _tmp58=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp56)->f2)->hd;_LL6: {void*t=_tmp58;
# 419
void*t2=({Cyc_Parse_substitute_tags(tags,t);});
if(t != t2)return({Cyc_Absyn_thin_bounds_type(t2);});goto _LL0;}}else{goto _LL9;}}else{goto _LL9;}}else{goto _LL9;}case 9U: _LL7: _tmp57=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp56)->f1;_LL8: {struct Cyc_Absyn_Exp*e=_tmp57;
# 423
struct Cyc_Absyn_Exp*e2=({Cyc_Parse_substitute_tags_exp(tags,e);});
if(e2 != e)return({Cyc_Absyn_valueof_type(e2);});goto _LL0;}default: _LL9: _LLA:
# 429
 goto _LL0;}_LL0:;}
# 431
return t;}
# 436
static void Cyc_Parse_substitute_aggrfield_tags(struct Cyc_List_List*tags,struct Cyc_Absyn_Aggrfield*x){
({void*_tmp103F=({Cyc_Parse_substitute_tags(tags,x->type);});x->type=_tmp103F;});}struct _tuple20{struct Cyc_Absyn_Tqual f1;void*f2;};
# 444
static struct _tuple20*Cyc_Parse_get_tqual_typ(unsigned loc,struct _tuple9*t){
return({struct _tuple20*_tmp67=_cycalloc(sizeof(*_tmp67));_tmp67->f1=(*t).f2,_tmp67->f2=(*t).f3;_tmp67;});}
# 448
static int Cyc_Parse_is_typeparam(void*tm){
void*_tmp69=tm;if(((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmp69)->tag == 4U){_LL1: _LL2:
 return 1;}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 457
static void*Cyc_Parse_id2type(struct _fat_ptr s,void*k){
if(({({struct _fat_ptr _tmp1040=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp1040,({const char*_tmp6B="`H";_tag_fat(_tmp6B,sizeof(char),3U);}));});})== 0)
return Cyc_Absyn_heap_rgn_type;else{
if(({({struct _fat_ptr _tmp1041=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp1041,({const char*_tmp6C="`U";_tag_fat(_tmp6C,sizeof(char),3U);}));});})== 0)
return Cyc_Absyn_unique_rgn_type;else{
if(({({struct _fat_ptr _tmp1042=(struct _fat_ptr)s;Cyc_zstrcmp(_tmp1042,({const char*_tmp6D="`RC";_tag_fat(_tmp6D,sizeof(char),4U);}));});})== 0)
return Cyc_Absyn_refcnt_rgn_type;else{
# 465
return({Cyc_Absyn_var_type(({struct Cyc_Absyn_Tvar*_tmp6F=_cycalloc(sizeof(*_tmp6F));({struct _fat_ptr*_tmp1043=({struct _fat_ptr*_tmp6E=_cycalloc(sizeof(*_tmp6E));*_tmp6E=s;_tmp6E;});_tmp6F->name=_tmp1043;}),_tmp6F->identity=- 1,_tmp6F->kind=k;_tmp6F;}));});}}}}
# 472
static struct Cyc_Absyn_Tvar*Cyc_Parse_typ2tvar(unsigned loc,void*t){
void*_tmp71=t;struct Cyc_Absyn_Tvar*_tmp72;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp71)->tag == 2U){_LL1: _tmp72=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp71)->f1;_LL2: {struct Cyc_Absyn_Tvar*pr=_tmp72;
return pr;}}else{_LL3: _LL4:
({void*_tmp73=0U;({int(*_tmp1046)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp75)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmp75;});unsigned _tmp1045=loc;struct _fat_ptr _tmp1044=({const char*_tmp74="expecting a list of type variables, not types";_tag_fat(_tmp74,sizeof(char),46U);});_tmp1046(_tmp1045,_tmp1044,_tag_fat(_tmp73,sizeof(void*),0U));});});}_LL0:;}
# 480
static void Cyc_Parse_set_vartyp_kind(void*t,struct Cyc_Absyn_Kind*k,int leq){
void*_stmttmp6=({Cyc_Tcutil_compress(t);});void*_tmp77=_stmttmp6;void**_tmp78;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp77)->tag == 2U){_LL1: _tmp78=(void**)&(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp77)->f1)->kind;_LL2: {void**cptr=_tmp78;
# 483
void*_stmttmp7=({Cyc_Absyn_compress_kb(*cptr);});void*_tmp79=_stmttmp7;if(((struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*)_tmp79)->tag == 1U){_LL6: _LL7:
# 485
 if(!leq)({void*_tmp1047=({Cyc_Tcutil_kind_to_bound(k);});*cptr=_tmp1047;});else{
({void*_tmp1048=(void*)({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp7A=_cycalloc(sizeof(*_tmp7A));_tmp7A->tag=2U,_tmp7A->f1=0,_tmp7A->f2=k;_tmp7A;});*cptr=_tmp1048;});}
return;}else{_LL8: _LL9:
 return;}_LL5:;}}else{_LL3: _LL4:
# 490
 return;}_LL0:;}
# 496
static struct Cyc_List_List*Cyc_Parse_oldstyle2newstyle(struct _RegionHandle*yy,struct Cyc_List_List*tms,struct Cyc_List_List*tds,unsigned loc){
# 501
if(tds == 0)return tms;if(tms == 0)
# 506
return 0;{
# 501
void*_stmttmp8=(void*)tms->hd;void*_tmp7C=_stmttmp8;void*_tmp7D;if(((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp7C)->tag == 3U){_LL1: _tmp7D=(void*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp7C)->f1;_LL2: {void*args=_tmp7D;
# 511
if(tms->tl == 0 ||
({Cyc_Parse_is_typeparam((void*)((struct Cyc_List_List*)_check_null(tms->tl))->hd);})&&((struct Cyc_List_List*)_check_null(tms->tl))->tl == 0){
# 514
void*_tmp7E=args;struct Cyc_List_List*_tmp7F;if(((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmp7E)->tag == 1U){_LL6: _LL7:
# 516
({void*_tmp80=0U;({unsigned _tmp104A=loc;struct _fat_ptr _tmp1049=({const char*_tmp81="function declaration with both new- and old-style parameter declarations; ignoring old-style";_tag_fat(_tmp81,sizeof(char),93U);});Cyc_Warn_warn(_tmp104A,_tmp1049,_tag_fat(_tmp80,sizeof(void*),0U));});});
# 518
return tms;}else{_LL8: _tmp7F=((struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*)_tmp7E)->f1;_LL9: {struct Cyc_List_List*ids=_tmp7F;
# 520
if(({int _tmp104B=({Cyc_List_length(ids);});_tmp104B != ({Cyc_List_length(tds);});}))
({void*_tmp82=0U;({int(*_tmp104E)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp84)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmp84;});unsigned _tmp104D=loc;struct _fat_ptr _tmp104C=({const char*_tmp83="wrong number of parameter declarations in old-style function declaration";_tag_fat(_tmp83,sizeof(char),73U);});_tmp104E(_tmp104D,_tmp104C,_tag_fat(_tmp82,sizeof(void*),0U));});});{
# 520
struct Cyc_List_List*rev_new_params=0;
# 525
for(0;ids != 0;ids=ids->tl){
struct Cyc_List_List*tds2=tds;
for(0;tds2 != 0;tds2=tds2->tl){
struct Cyc_Absyn_Decl*x=(struct Cyc_Absyn_Decl*)tds2->hd;
void*_stmttmp9=x->r;void*_tmp85=_stmttmp9;struct Cyc_Absyn_Vardecl*_tmp86;if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp85)->tag == 0U){_LLB: _tmp86=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp85)->f1;_LLC: {struct Cyc_Absyn_Vardecl*vd=_tmp86;
# 531
if(({Cyc_zstrptrcmp((*vd->name).f2,(struct _fat_ptr*)ids->hd);})!= 0)
continue;
# 531
if(vd->initializer != 0)
# 534
({void*_tmp87=0U;({int(*_tmp1051)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp89)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmp89;});unsigned _tmp1050=x->loc;struct _fat_ptr _tmp104F=({const char*_tmp88="initializer found in parameter declaration";_tag_fat(_tmp88,sizeof(char),43U);});_tmp1051(_tmp1050,_tmp104F,_tag_fat(_tmp87,sizeof(void*),0U));});});
# 531
if(({Cyc_Absyn_is_qvar_qualified(vd->name);}))
# 536
({void*_tmp8A=0U;({int(*_tmp1054)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp8C)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmp8C;});unsigned _tmp1053=x->loc;struct _fat_ptr _tmp1052=({const char*_tmp8B="namespaces forbidden in parameter declarations";_tag_fat(_tmp8B,sizeof(char),47U);});_tmp1054(_tmp1053,_tmp1052,_tag_fat(_tmp8A,sizeof(void*),0U));});});
# 531
rev_new_params=({struct Cyc_List_List*_tmp8E=_cycalloc(sizeof(*_tmp8E));
# 538
({struct _tuple9*_tmp1055=({struct _tuple9*_tmp8D=_cycalloc(sizeof(*_tmp8D));_tmp8D->f1=(*vd->name).f2,_tmp8D->f2=vd->tq,_tmp8D->f3=vd->type;_tmp8D;});_tmp8E->hd=_tmp1055;}),_tmp8E->tl=rev_new_params;_tmp8E;});
# 540
goto L;}}else{_LLD: _LLE:
({void*_tmp8F=0U;({int(*_tmp1058)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp91)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmp91;});unsigned _tmp1057=x->loc;struct _fat_ptr _tmp1056=({const char*_tmp90="nonvariable declaration in parameter type";_tag_fat(_tmp90,sizeof(char),42U);});_tmp1058(_tmp1057,_tmp1056,_tag_fat(_tmp8F,sizeof(void*),0U));});});}_LLA:;}
# 544
L: if(tds2 == 0)
({struct Cyc_String_pa_PrintArg_struct _tmp95=({struct Cyc_String_pa_PrintArg_struct _tmpF87;_tmpF87.tag=0U,_tmpF87.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)ids->hd));_tmpF87;});void*_tmp92[1U];_tmp92[0]=& _tmp95;({int(*_tmp105B)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp94)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmp94;});unsigned _tmp105A=loc;struct _fat_ptr _tmp1059=({const char*_tmp93="%s is not given a type";_tag_fat(_tmp93,sizeof(char),23U);});_tmp105B(_tmp105A,_tmp1059,_tag_fat(_tmp92,sizeof(void*),1U));});});}
# 547
return({struct Cyc_List_List*_tmp98=_region_malloc(yy,sizeof(*_tmp98));
({void*_tmp105E=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp97=_region_malloc(yy,sizeof(*_tmp97));_tmp97->tag=3U,({void*_tmp105D=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp96=_region_malloc(yy,sizeof(*_tmp96));_tmp96->tag=1U,({struct Cyc_List_List*_tmp105C=({Cyc_List_imp_rev(rev_new_params);});_tmp96->f1=_tmp105C;}),_tmp96->f2=0,_tmp96->f3=0,_tmp96->f4=0,_tmp96->f5=0,_tmp96->f6=0,_tmp96->f7=0,_tmp96->f8=0,_tmp96->f9=0,_tmp96->f10=0,_tmp96->f11=0;_tmp96;});_tmp97->f1=_tmp105D;});_tmp97;});_tmp98->hd=_tmp105E;}),_tmp98->tl=0;_tmp98;});}}}_LL5:;}
# 511
goto _LL4;}}else{_LL3: _LL4:
# 555
 return({struct Cyc_List_List*_tmp99=_region_malloc(yy,sizeof(*_tmp99));_tmp99->hd=(void*)tms->hd,({struct Cyc_List_List*_tmp105F=({Cyc_Parse_oldstyle2newstyle(yy,tms->tl,tds,loc);});_tmp99->tl=_tmp105F;});_tmp99;});}_LL0:;}}
# 562
static struct Cyc_Absyn_Fndecl*Cyc_Parse_make_function(struct _RegionHandle*yy,struct Cyc_Parse_Declaration_spec*dso,struct Cyc_Parse_Declarator d,struct Cyc_List_List*tds,struct Cyc_Absyn_Stmt*body,unsigned loc){
# 566
if(tds != 0)
d=({struct Cyc_Parse_Declarator _tmpF88;_tmpF88.id=d.id,_tmpF88.varloc=d.varloc,({struct Cyc_List_List*_tmp1060=({Cyc_Parse_oldstyle2newstyle(yy,d.tms,tds,loc);});_tmpF88.tms=_tmp1060;});_tmpF88;});{
# 566
enum Cyc_Absyn_Scope sc=2U;
# 570
struct Cyc_Parse_Type_specifier tss=({Cyc_Parse_empty_spec(loc);});
struct Cyc_Absyn_Tqual tq=({Cyc_Absyn_empty_tqual(0U);});
int is_inline=0;
struct Cyc_List_List*atts=0;
# 575
if(dso != 0){
tss=dso->type_specs;
tq=dso->tq;
is_inline=dso->is_inline;
atts=dso->attributes;
# 581
if(dso->sc != 0){
enum Cyc_Parse_Storage_class _stmttmpA=*((enum Cyc_Parse_Storage_class*)_check_null(dso->sc));enum Cyc_Parse_Storage_class _tmp9B=_stmttmpA;switch(_tmp9B){case Cyc_Parse_Extern_sc: _LL1: _LL2:
 sc=3U;goto _LL0;case Cyc_Parse_Static_sc: _LL3: _LL4:
 sc=0U;goto _LL0;default: _LL5: _LL6:
({void*_tmp9C=0U;({unsigned _tmp1062=loc;struct _fat_ptr _tmp1061=({const char*_tmp9D="bad storage class on function";_tag_fat(_tmp9D,sizeof(char),30U);});Cyc_Warn_err(_tmp1062,_tmp1061,_tag_fat(_tmp9C,sizeof(void*),0U));});});goto _LL0;}_LL0:;}}{
# 575
void*t=({Cyc_Parse_collapse_type_specifiers(tss,loc);});
# 589
struct _tuple15 _stmttmpB=({Cyc_Parse_apply_tms(tq,t,atts,d.tms);});struct Cyc_List_List*_tmpA1;struct Cyc_List_List*_tmpA0;void*_tmp9F;struct Cyc_Absyn_Tqual _tmp9E;_LL8: _tmp9E=_stmttmpB.f1;_tmp9F=_stmttmpB.f2;_tmpA0=_stmttmpB.f3;_tmpA1=_stmttmpB.f4;_LL9: {struct Cyc_Absyn_Tqual fn_tqual=_tmp9E;void*fn_type=_tmp9F;struct Cyc_List_List*x=_tmpA0;struct Cyc_List_List*out_atts=_tmpA1;
# 593
if(x != 0)
# 596
({void*_tmpA2=0U;({unsigned _tmp1064=loc;struct _fat_ptr _tmp1063=({const char*_tmpA3="bad type params, ignoring";_tag_fat(_tmpA3,sizeof(char),26U);});Cyc_Warn_warn(_tmp1064,_tmp1063,_tag_fat(_tmpA2,sizeof(void*),0U));});});{
# 593
void*_tmpA4=fn_type;struct Cyc_Absyn_FnInfo _tmpA5;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA4)->tag == 5U){_LLB: _tmpA5=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmpA4)->f1;_LLC: {struct Cyc_Absyn_FnInfo i=_tmpA5;
# 600
{struct Cyc_List_List*args2=i.args;for(0;args2 != 0;args2=args2->tl){
if((*((struct _tuple9*)args2->hd)).f1 == 0){
({void*_tmpA6=0U;({unsigned _tmp1066=loc;struct _fat_ptr _tmp1065=({const char*_tmpA7="missing argument variable in function prototype";_tag_fat(_tmpA7,sizeof(char),48U);});Cyc_Warn_err(_tmp1066,_tmp1065,_tag_fat(_tmpA6,sizeof(void*),0U));});});
({struct _fat_ptr*_tmp1068=({struct _fat_ptr*_tmpA9=_cycalloc(sizeof(*_tmpA9));({struct _fat_ptr _tmp1067=({const char*_tmpA8="?";_tag_fat(_tmpA8,sizeof(char),2U);});*_tmpA9=_tmp1067;});_tmpA9;});(*((struct _tuple9*)args2->hd)).f1=_tmp1068;});}}}
# 600
({struct Cyc_List_List*_tmp1069=({Cyc_List_append(i.attributes,out_atts);});i.attributes=_tmp1069;});
# 608
return({struct Cyc_Absyn_Fndecl*_tmpAA=_cycalloc(sizeof(*_tmpAA));_tmpAA->sc=sc,_tmpAA->is_inline=is_inline,_tmpAA->name=d.id,_tmpAA->body=body,_tmpAA->i=i,_tmpAA->cached_type=0,_tmpAA->param_vardecls=0,_tmpAA->fn_vardecl=0;_tmpAA;});}}else{_LLD: _LLE:
# 611
({void*_tmpAB=0U;({int(*_tmp106C)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpAD)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmpAD;});unsigned _tmp106B=loc;struct _fat_ptr _tmp106A=({const char*_tmpAC="declarator is not a function prototype";_tag_fat(_tmpAC,sizeof(char),39U);});_tmp106C(_tmp106B,_tmp106A,_tag_fat(_tmpAB,sizeof(void*),0U));});});}_LLA:;}}}}}static char _tmpAF[76U]="at most one type may appear within a type specifier \n\t(missing ';' or ','?)";
# 562
static struct _fat_ptr Cyc_Parse_msg1={_tmpAF,_tmpAF,_tmpAF + 76U};static char _tmpB0[87U]="const or volatile may appear only once within a type specifier \n\t(missing ';' or ','?)";
# 617
static struct _fat_ptr Cyc_Parse_msg2={_tmpB0,_tmpB0,_tmpB0 + 87U};static char _tmpB1[74U]="type specifier includes more than one declaration \n\t(missing ';' or ','?)";
# 619
static struct _fat_ptr Cyc_Parse_msg3={_tmpB1,_tmpB1,_tmpB1 + 74U};static char _tmpB2[84U]="sign specifier may appear only once within a type specifier \n\t(missing ';' or ','?)";
# 621
static struct _fat_ptr Cyc_Parse_msg4={_tmpB2,_tmpB2,_tmpB2 + 84U};
# 628
static struct Cyc_Parse_Type_specifier Cyc_Parse_combine_specifiers(unsigned loc,struct Cyc_Parse_Type_specifier s1,struct Cyc_Parse_Type_specifier s2){
# 631
if(s1.Signed_spec && s2.Signed_spec)
({void*_tmpB3=0U;({unsigned _tmp106E=loc;struct _fat_ptr _tmp106D=Cyc_Parse_msg4;Cyc_Warn_warn(_tmp106E,_tmp106D,_tag_fat(_tmpB3,sizeof(void*),0U));});});
# 631
s1.Signed_spec |=s2.Signed_spec;
# 634
if(s1.Unsigned_spec && s2.Unsigned_spec)
({void*_tmpB4=0U;({unsigned _tmp1070=loc;struct _fat_ptr _tmp106F=Cyc_Parse_msg4;Cyc_Warn_warn(_tmp1070,_tmp106F,_tag_fat(_tmpB4,sizeof(void*),0U));});});
# 634
s1.Unsigned_spec |=s2.Unsigned_spec;
# 637
if(s1.Short_spec && s2.Short_spec)
({void*_tmpB5=0U;({unsigned _tmp1072=loc;struct _fat_ptr _tmp1071=Cyc_Parse_msg4;Cyc_Warn_warn(_tmp1072,_tmp1071,_tag_fat(_tmpB5,sizeof(void*),0U));});});
# 637
s1.Short_spec |=s2.Short_spec;
# 640
if((s1.Long_Long_spec && s2.Long_Long_spec ||
 s1.Long_Long_spec && s2.Long_spec)||
 s2.Long_Long_spec && s1.Long_spec)
({void*_tmpB6=0U;({unsigned _tmp1074=loc;struct _fat_ptr _tmp1073=Cyc_Parse_msg4;Cyc_Warn_warn(_tmp1074,_tmp1073,_tag_fat(_tmpB6,sizeof(void*),0U));});});
# 640
s1.Long_Long_spec=
# 645
(s1.Long_Long_spec || s2.Long_Long_spec)|| s1.Long_spec && s2.Long_spec;
s1.Long_spec=!s1.Long_Long_spec &&(s1.Long_spec || s2.Long_spec);
if(s1.Valid_type_spec && s2.Valid_type_spec)
({void*_tmpB7=0U;({unsigned _tmp1076=loc;struct _fat_ptr _tmp1075=Cyc_Parse_msg1;Cyc_Warn_err(_tmp1076,_tmp1075,_tag_fat(_tmpB7,sizeof(void*),0U));});});else{
if(s2.Valid_type_spec){
s1.Type_spec=s2.Type_spec;
s1.Valid_type_spec=1;}}
# 647
return s1;}
# 659
static void*Cyc_Parse_collapse_type_specifiers(struct Cyc_Parse_Type_specifier ts,unsigned loc){
# 662
int seen_type=ts.Valid_type_spec;
int seen_sign=ts.Signed_spec || ts.Unsigned_spec;
int seen_size=(ts.Short_spec || ts.Long_spec)|| ts.Long_Long_spec;
void*t=seen_type?ts.Type_spec: Cyc_Absyn_void_type;
enum Cyc_Absyn_Size_of sz=2U;
enum Cyc_Absyn_Sign sgn=0U;
# 669
if(ts.Signed_spec && ts.Unsigned_spec)
({void*_tmpB9=0U;({unsigned _tmp1078=loc;struct _fat_ptr _tmp1077=Cyc_Parse_msg4;Cyc_Warn_err(_tmp1078,_tmp1077,_tag_fat(_tmpB9,sizeof(void*),0U));});});
# 669
if(ts.Unsigned_spec)
# 671
sgn=1U;
# 669
if(
# 672
ts.Short_spec &&(ts.Long_spec || ts.Long_Long_spec)||
 ts.Long_spec && ts.Long_Long_spec)
({void*_tmpBA=0U;({unsigned _tmp107A=loc;struct _fat_ptr _tmp1079=Cyc_Parse_msg4;Cyc_Warn_err(_tmp107A,_tmp1079,_tag_fat(_tmpBA,sizeof(void*),0U));});});
# 669
if(ts.Short_spec)
# 675
sz=1U;
# 669
if(ts.Long_spec)
# 676
sz=3U;
# 669
if(ts.Long_Long_spec)
# 677
sz=4U;
# 669
if(!seen_type){
# 682
if(!seen_sign && !seen_size)
({void*_tmpBB=0U;({unsigned _tmp107C=loc;struct _fat_ptr _tmp107B=({const char*_tmpBC="missing type within specifier";_tag_fat(_tmpBC,sizeof(char),30U);});Cyc_Warn_warn(_tmp107C,_tmp107B,_tag_fat(_tmpBB,sizeof(void*),0U));});});
# 682
t=({Cyc_Absyn_int_type(sgn,sz);});}else{
# 686
if(seen_sign){
void*_tmpBD=t;enum Cyc_Absyn_Size_of _tmpBF;enum Cyc_Absyn_Sign _tmpBE;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpBD)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpBD)->f1)->tag == 1U){_LL1: _tmpBE=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpBD)->f1)->f1;_tmpBF=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpBD)->f1)->f2;_LL2: {enum Cyc_Absyn_Sign sgn2=_tmpBE;enum Cyc_Absyn_Size_of sz2=_tmpBF;
# 689
if((int)sgn2 != (int)sgn)
t=({Cyc_Absyn_int_type(sgn,sz2);});
# 689
goto _LL0;}}else{goto _LL3;}}else{_LL3: _LL4:
# 692
({void*_tmpC0=0U;({unsigned _tmp107E=loc;struct _fat_ptr _tmp107D=({const char*_tmpC1="sign specification on non-integral type";_tag_fat(_tmpC1,sizeof(char),40U);});Cyc_Warn_err(_tmp107E,_tmp107D,_tag_fat(_tmpC0,sizeof(void*),0U));});});goto _LL0;}_LL0:;}
# 686
if(seen_size){
# 695
void*_tmpC2=t;enum Cyc_Absyn_Size_of _tmpC4;enum Cyc_Absyn_Sign _tmpC3;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC2)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC2)->f1)){case 1U: _LL6: _tmpC3=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC2)->f1)->f1;_tmpC4=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpC2)->f1)->f2;_LL7: {enum Cyc_Absyn_Sign sgn2=_tmpC3;enum Cyc_Absyn_Size_of sz2=_tmpC4;
# 697
if((int)sz2 != (int)sz)
t=({Cyc_Absyn_int_type(sgn2,sz);});
# 697
goto _LL5;}case 2U: _LL8: _LL9:
# 701
 t=Cyc_Absyn_long_double_type;goto _LL5;default: goto _LLA;}else{_LLA: _LLB:
({void*_tmpC5=0U;({unsigned _tmp1080=loc;struct _fat_ptr _tmp107F=({const char*_tmpC6="size qualifier on non-integral type";_tag_fat(_tmpC6,sizeof(char),36U);});Cyc_Warn_err(_tmp1080,_tmp107F,_tag_fat(_tmpC5,sizeof(void*),0U));});});goto _LL5;}_LL5:;}}
# 705
return t;}
# 709
static struct Cyc_List_List*Cyc_Parse_apply_tmss(struct _RegionHandle*r,struct Cyc_Absyn_Tqual tq,void*t,struct _tuple12*ds,struct Cyc_List_List*shared_atts){
# 712
if(ds == 0)return 0;{struct Cyc_Parse_Declarator d=ds->hd;
# 714
struct _tuple0*q=d.id;
unsigned varloc=d.varloc;
struct _tuple15 _stmttmpC=({Cyc_Parse_apply_tms(tq,t,shared_atts,d.tms);});struct Cyc_List_List*_tmpCB;struct Cyc_List_List*_tmpCA;void*_tmpC9;struct Cyc_Absyn_Tqual _tmpC8;_LL1: _tmpC8=_stmttmpC.f1;_tmpC9=_stmttmpC.f2;_tmpCA=_stmttmpC.f3;_tmpCB=_stmttmpC.f4;_LL2: {struct Cyc_Absyn_Tqual tq2=_tmpC8;void*new_typ=_tmpC9;struct Cyc_List_List*tvs=_tmpCA;struct Cyc_List_List*atts=_tmpCB;
# 719
if(ds->tl == 0)
return({struct Cyc_List_List*_tmpCD=_region_malloc(r,sizeof(*_tmpCD));({struct _tuple16*_tmp1081=({struct _tuple16*_tmpCC=_region_malloc(r,sizeof(*_tmpCC));_tmpCC->f1=varloc,_tmpCC->f2=q,_tmpCC->f3=tq2,_tmpCC->f4=new_typ,_tmpCC->f5=tvs,_tmpCC->f6=atts;_tmpCC;});_tmpCD->hd=_tmp1081;}),_tmpCD->tl=0;_tmpCD;});else{
# 722
return({struct Cyc_List_List*_tmpD5=_region_malloc(r,sizeof(*_tmpD5));({struct _tuple16*_tmp1083=({struct _tuple16*_tmpCE=_region_malloc(r,sizeof(*_tmpCE));_tmpCE->f1=varloc,_tmpCE->f2=q,_tmpCE->f3=tq2,_tmpCE->f4=new_typ,_tmpCE->f5=tvs,_tmpCE->f6=atts;_tmpCE;});_tmpD5->hd=_tmp1083;}),({
struct Cyc_List_List*_tmp1082=({struct Cyc_List_List*(*_tmpCF)(struct _RegionHandle*r,struct Cyc_Absyn_Tqual tq,void*t,struct _tuple12*ds,struct Cyc_List_List*shared_atts)=Cyc_Parse_apply_tmss;struct _RegionHandle*_tmpD0=r;struct Cyc_Absyn_Tqual _tmpD1=tq;void*_tmpD2=({Cyc_Tcutil_copy_type(t);});struct _tuple12*_tmpD3=ds->tl;struct Cyc_List_List*_tmpD4=shared_atts;_tmpCF(_tmpD0,_tmpD1,_tmpD2,_tmpD3,_tmpD4);});_tmpD5->tl=_tmp1082;});_tmpD5;});}}}}
# 727
static struct _tuple15 Cyc_Parse_apply_tms(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_List_List*atts,struct Cyc_List_List*tms){
# 729
if(tms == 0)return({struct _tuple15 _tmpF89;_tmpF89.f1=tq,_tmpF89.f2=t,_tmpF89.f3=0,_tmpF89.f4=atts;_tmpF89;});{void*_stmttmpD=(void*)tms->hd;void*_tmpD7=_stmttmpD;struct Cyc_List_List*_tmpD9;unsigned _tmpD8;struct Cyc_Absyn_Tqual _tmpDB;struct Cyc_Absyn_PtrAtts _tmpDA;unsigned _tmpDD;struct Cyc_List_List*_tmpDC;void*_tmpDE;unsigned _tmpE1;void*_tmpE0;struct Cyc_Absyn_Exp*_tmpDF;unsigned _tmpE3;void*_tmpE2;switch(*((int*)_tmpD7)){case 0U: _LL1: _tmpE2=(void*)((struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*)_tmpD7)->f1;_tmpE3=((struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*)_tmpD7)->f2;_LL2: {void*zeroterm=_tmpE2;unsigned ztloc=_tmpE3;
# 732
return({struct _tuple15(*_tmpE4)(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_List_List*atts,struct Cyc_List_List*tms)=Cyc_Parse_apply_tms;struct Cyc_Absyn_Tqual _tmpE5=({Cyc_Absyn_empty_tqual(0U);});void*_tmpE6=({Cyc_Absyn_array_type(t,tq,0,zeroterm,ztloc);});struct Cyc_List_List*_tmpE7=atts;struct Cyc_List_List*_tmpE8=tms->tl;_tmpE4(_tmpE5,_tmpE6,_tmpE7,_tmpE8);});}case 1U: _LL3: _tmpDF=((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmpD7)->f1;_tmpE0=(void*)((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmpD7)->f2;_tmpE1=((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmpD7)->f3;_LL4: {struct Cyc_Absyn_Exp*e=_tmpDF;void*zeroterm=_tmpE0;unsigned ztloc=_tmpE1;
# 735
return({struct _tuple15(*_tmpE9)(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_List_List*atts,struct Cyc_List_List*tms)=Cyc_Parse_apply_tms;struct Cyc_Absyn_Tqual _tmpEA=({Cyc_Absyn_empty_tqual(0U);});void*_tmpEB=({Cyc_Absyn_array_type(t,tq,e,zeroterm,ztloc);});struct Cyc_List_List*_tmpEC=atts;struct Cyc_List_List*_tmpED=tms->tl;_tmpE9(_tmpEA,_tmpEB,_tmpEC,_tmpED);});}case 3U: _LL5: _tmpDE=(void*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmpD7)->f1;_LL6: {void*args=_tmpDE;
# 738
void*_tmpEE=args;unsigned _tmpEF;int _tmpFA;struct Cyc_List_List*_tmpF9;struct Cyc_List_List*_tmpF8;struct Cyc_List_List*_tmpF7;struct Cyc_Absyn_Exp*_tmpF6;struct Cyc_Absyn_Exp*_tmpF5;struct Cyc_List_List*_tmpF4;void*_tmpF3;struct Cyc_Absyn_VarargInfo*_tmpF2;int _tmpF1;struct Cyc_List_List*_tmpF0;if(((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->tag == 1U){_LLE: _tmpF0=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f1;_tmpF1=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f2;_tmpF2=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f3;_tmpF3=(void*)((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f4;_tmpF4=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f5;_tmpF5=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f6;_tmpF6=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f7;_tmpF7=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f8;_tmpF8=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f9;_tmpF9=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f10;_tmpFA=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)_tmpEE)->f11;_LLF: {struct Cyc_List_List*args2=_tmpF0;int c_vararg=_tmpF1;struct Cyc_Absyn_VarargInfo*cyc_vararg=_tmpF2;void*eff=_tmpF3;struct Cyc_List_List*rgn_po=_tmpF4;struct Cyc_Absyn_Exp*req=_tmpF5;struct Cyc_Absyn_Exp*ens=_tmpF6;struct Cyc_List_List*ieff=_tmpF7;struct Cyc_List_List*oeff=_tmpF8;struct Cyc_List_List*throws=_tmpF9;int reentrant=_tmpFA;
# 741
struct Cyc_List_List*typvars=0;
# 743
struct Cyc_List_List*fn_atts=0;struct Cyc_List_List*new_atts=0;
{struct Cyc_List_List*as=atts;for(0;as != 0;as=as->tl){
# 746
if(({Cyc_Absyn_fntype_att((void*)as->hd);}))
fn_atts=({struct Cyc_List_List*_tmpFB=_cycalloc(sizeof(*_tmpFB));_tmpFB->hd=(void*)as->hd,_tmpFB->tl=fn_atts;_tmpFB;});else{
# 749
new_atts=({struct Cyc_List_List*_tmpFC=_cycalloc(sizeof(*_tmpFC));_tmpFC->hd=(void*)as->hd,_tmpFC->tl=new_atts;_tmpFC;});}}}
# 752
if(tms->tl != 0){
# 754
void*_stmttmpE=(void*)((struct Cyc_List_List*)_check_null(tms->tl))->hd;void*_tmpFD=_stmttmpE;struct Cyc_List_List*_tmpFE;if(((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmpFD)->tag == 4U){_LL13: _tmpFE=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmpFD)->f1;_LL14: {struct Cyc_List_List*ts=_tmpFE;
# 757
typvars=ts;
tms=tms->tl;
goto _LL12;}}else{_LL15: _LL16:
 goto _LL12;}_LL12:;}
# 752
if(
# 764
((((!c_vararg && cyc_vararg == 0)&& args2 != 0)&& args2->tl == 0)&&(*((struct _tuple9*)args2->hd)).f1 == 0)&&(*((struct _tuple9*)args2->hd)).f3 == Cyc_Absyn_void_type)
# 769
args2=0;{
# 752
struct Cyc_List_List*tags=({Cyc_Parse_get_arg_tags(args2);});
# 774
if(tags != 0)
t=({Cyc_Parse_substitute_tags(tags,t);});
# 774
t=({Cyc_Parse_array2ptr(t,0);});
# 779
{struct Cyc_List_List*a=args2;for(0;a != 0;a=a->tl){
struct _tuple9*_stmttmpF=(struct _tuple9*)a->hd;void**_tmp101;struct Cyc_Absyn_Tqual _tmp100;struct _fat_ptr*_tmpFF;_LL18: _tmpFF=_stmttmpF->f1;_tmp100=_stmttmpF->f2;_tmp101=(void**)& _stmttmpF->f3;_LL19: {struct _fat_ptr*vopt=_tmpFF;struct Cyc_Absyn_Tqual tq=_tmp100;void**t=_tmp101;
if(tags != 0)
({void*_tmp1084=({Cyc_Parse_substitute_tags(tags,*t);});*t=_tmp1084;});
# 781
({void*_tmp1085=({Cyc_Parse_array2ptr(*t,1);});*t=_tmp1085;});}}}
# 791
return({struct _tuple15(*_tmp102)(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_List_List*atts,struct Cyc_List_List*tms)=Cyc_Parse_apply_tms;struct Cyc_Absyn_Tqual _tmp103=({Cyc_Absyn_empty_tqual(tq.loc);});void*_tmp104=({Cyc_Absyn_function_type(typvars,eff,tq,t,args2,c_vararg,cyc_vararg,rgn_po,fn_atts,req,ens,ieff,oeff,throws,reentrant);});struct Cyc_List_List*_tmp105=new_atts;struct Cyc_List_List*_tmp106=((struct Cyc_List_List*)_check_null(tms))->tl;_tmp102(_tmp103,_tmp104,_tmp105,_tmp106);});}}}else{_LL10: _tmpEF=((struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*)_tmpEE)->f2;_LL11: {unsigned loc=_tmpEF;
# 800
({void*_tmp107=0U;({int(*_tmp1088)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp109)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmp109;});unsigned _tmp1087=loc;struct _fat_ptr _tmp1086=({const char*_tmp108="function declaration without parameter types";_tag_fat(_tmp108,sizeof(char),45U);});_tmp1088(_tmp1087,_tmp1086,_tag_fat(_tmp107,sizeof(void*),0U));});});}}_LLD:;}case 4U: _LL7: _tmpDC=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmpD7)->f1;_tmpDD=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmpD7)->f2;_LL8: {struct Cyc_List_List*ts=_tmpDC;unsigned loc=_tmpDD;
# 807
if(tms->tl == 0)
return({struct _tuple15 _tmpF8A;_tmpF8A.f1=tq,_tmpF8A.f2=t,_tmpF8A.f3=ts,_tmpF8A.f4=atts;_tmpF8A;});
# 807
({void*_tmp10A=0U;({int(*_tmp108B)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp10C)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmp10C;});unsigned _tmp108A=loc;struct _fat_ptr _tmp1089=({const char*_tmp10B="type parameters must appear before function arguments in declarator";_tag_fat(_tmp10B,sizeof(char),68U);});_tmp108B(_tmp108A,_tmp1089,_tag_fat(_tmp10A,sizeof(void*),0U));});});}case 2U: _LL9: _tmpDA=((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmpD7)->f1;_tmpDB=((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmpD7)->f2;_LLA: {struct Cyc_Absyn_PtrAtts ptratts=_tmpDA;struct Cyc_Absyn_Tqual tq2=_tmpDB;
# 815
return({struct _tuple15(*_tmp10D)(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_List_List*atts,struct Cyc_List_List*tms)=Cyc_Parse_apply_tms;struct Cyc_Absyn_Tqual _tmp10E=tq2;void*_tmp10F=({Cyc_Absyn_pointer_type(({struct Cyc_Absyn_PtrInfo _tmpF8B;_tmpF8B.elt_type=t,_tmpF8B.elt_tq=tq,_tmpF8B.ptr_atts=ptratts;_tmpF8B;}));});struct Cyc_List_List*_tmp110=atts;struct Cyc_List_List*_tmp111=tms->tl;_tmp10D(_tmp10E,_tmp10F,_tmp110,_tmp111);});}default: _LLB: _tmpD8=((struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_tmpD7)->f1;_tmpD9=((struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_tmpD7)->f2;_LLC: {unsigned loc=_tmpD8;struct Cyc_List_List*atts2=_tmpD9;
# 820
return({struct _tuple15(*_tmp112)(struct Cyc_Absyn_Tqual tq,void*t,struct Cyc_List_List*atts,struct Cyc_List_List*tms)=Cyc_Parse_apply_tms;struct Cyc_Absyn_Tqual _tmp113=tq;void*_tmp114=t;struct Cyc_List_List*_tmp115=({Cyc_List_append(atts,atts2);});struct Cyc_List_List*_tmp116=tms->tl;_tmp112(_tmp113,_tmp114,_tmp115,_tmp116);});}}_LL0:;}}
# 727
void*Cyc_Parse_speclist2typ(struct Cyc_Parse_Type_specifier tss,unsigned loc){
# 827
return({Cyc_Parse_collapse_type_specifiers(tss,loc);});}
# 835
static struct Cyc_Absyn_Decl*Cyc_Parse_v_typ_to_typedef(unsigned loc,struct _tuple16*t){
struct Cyc_List_List*_tmp11E;struct Cyc_List_List*_tmp11D;void*_tmp11C;struct Cyc_Absyn_Tqual _tmp11B;struct _tuple0*_tmp11A;unsigned _tmp119;_LL1: _tmp119=t->f1;_tmp11A=t->f2;_tmp11B=t->f3;_tmp11C=t->f4;_tmp11D=t->f5;_tmp11E=t->f6;_LL2: {unsigned varloc=_tmp119;struct _tuple0*x=_tmp11A;struct Cyc_Absyn_Tqual tq=_tmp11B;void*typ=_tmp11C;struct Cyc_List_List*tvs=_tmp11D;struct Cyc_List_List*atts=_tmp11E;
# 838
({Cyc_Lex_register_typedef(x);});{
# 840
struct Cyc_Core_Opt*kind;
void*type;
{void*_tmp11F=typ;struct Cyc_Core_Opt*_tmp120;if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp11F)->tag == 1U){_LL4: _tmp120=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp11F)->f1;_LL5: {struct Cyc_Core_Opt*kopt=_tmp120;
# 844
type=0;
if(kopt == 0)kind=& Cyc_Tcutil_bko;else{
kind=kopt;}
goto _LL3;}}else{_LL6: _LL7:
 kind=0;type=typ;goto _LL3;}_LL3:;}
# 850
return({({void*_tmp108D=(void*)({struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*_tmp122=_cycalloc(sizeof(*_tmp122));_tmp122->tag=8U,({struct Cyc_Absyn_Typedefdecl*_tmp108C=({struct Cyc_Absyn_Typedefdecl*_tmp121=_cycalloc(sizeof(*_tmp121));_tmp121->name=x,_tmp121->tvs=tvs,_tmp121->kind=kind,_tmp121->defn=type,_tmp121->atts=atts,_tmp121->tq=tq,_tmp121->extern_c=0;_tmp121;});_tmp122->f1=_tmp108C;});_tmp122;});Cyc_Absyn_new_decl(_tmp108D,loc);});});}}}
# 857
static struct Cyc_Absyn_Stmt*Cyc_Parse_flatten_decl(struct Cyc_Absyn_Decl*d,struct Cyc_Absyn_Stmt*s){
return({({void*_tmp108E=(void*)({struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*_tmp124=_cycalloc(sizeof(*_tmp124));_tmp124->tag=12U,_tmp124->f1=d,_tmp124->f2=s;_tmp124;});Cyc_Absyn_new_stmt(_tmp108E,d->loc);});});}
# 863
static struct Cyc_Absyn_Stmt*Cyc_Parse_flatten_declarations(struct Cyc_List_List*ds,struct Cyc_Absyn_Stmt*s){
return({({struct Cyc_Absyn_Stmt*(*_tmp1090)(struct Cyc_Absyn_Stmt*(*f)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum)=({struct Cyc_Absyn_Stmt*(*_tmp126)(struct Cyc_Absyn_Stmt*(*f)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum)=(struct Cyc_Absyn_Stmt*(*)(struct Cyc_Absyn_Stmt*(*f)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*),struct Cyc_List_List*x,struct Cyc_Absyn_Stmt*accum))Cyc_List_fold_right;_tmp126;});struct Cyc_List_List*_tmp108F=ds;_tmp1090(Cyc_Parse_flatten_decl,_tmp108F,s);});});}
# 872
static struct Cyc_List_List*Cyc_Parse_make_declarations(struct Cyc_Parse_Declaration_spec ds,struct _tuple14*ids,unsigned tqual_loc,unsigned loc){
# 876
struct _RegionHandle _tmp128=_new_region("mkrgn");struct _RegionHandle*mkrgn=& _tmp128;_push_region(mkrgn);
{struct Cyc_List_List*_tmp12B;struct Cyc_Parse_Type_specifier _tmp12A;struct Cyc_Absyn_Tqual _tmp129;_LL1: _tmp129=ds.tq;_tmp12A=ds.type_specs;_tmp12B=ds.attributes;_LL2: {struct Cyc_Absyn_Tqual tq=_tmp129;struct Cyc_Parse_Type_specifier tss=_tmp12A;struct Cyc_List_List*atts=_tmp12B;
if(tq.loc == (unsigned)0)tq.loc=tqual_loc;if(ds.is_inline)
# 880
({void*_tmp12C=0U;({unsigned _tmp1092=loc;struct _fat_ptr _tmp1091=({const char*_tmp12D="inline qualifier on non-function definition";_tag_fat(_tmp12D,sizeof(char),44U);});Cyc_Warn_warn(_tmp1092,_tmp1091,_tag_fat(_tmp12C,sizeof(void*),0U));});});{
# 878
enum Cyc_Absyn_Scope s=2U;
# 883
int istypedef=0;
if(ds.sc != 0){
enum Cyc_Parse_Storage_class _stmttmp10=*ds.sc;enum Cyc_Parse_Storage_class _tmp12E=_stmttmp10;switch(_tmp12E){case Cyc_Parse_Typedef_sc: _LL4: _LL5:
 istypedef=1;goto _LL3;case Cyc_Parse_Extern_sc: _LL6: _LL7:
 s=3U;goto _LL3;case Cyc_Parse_ExternC_sc: _LL8: _LL9:
 s=4U;goto _LL3;case Cyc_Parse_Static_sc: _LLA: _LLB:
 s=0U;goto _LL3;case Cyc_Parse_Auto_sc: _LLC: _LLD:
 s=2U;goto _LL3;case Cyc_Parse_Register_sc: _LLE: _LLF:
 if(Cyc_Parse_no_register)s=2U;else{s=5U;}goto _LL3;case Cyc_Parse_Abstract_sc: _LL10: _LL11:
 goto _LL13;default: _LL12: _LL13:
 s=1U;goto _LL3;}_LL3:;}{
# 884
struct _tuple12*declarators=0;
# 900
struct Cyc_List_List*exprs=0;
({Cyc_Parse_decl_split(mkrgn,ids,& declarators,& exprs);});{
# 903
int exps_empty=1;
{struct Cyc_List_List*es=exprs;for(0;es != 0;es=es->tl){
if((struct Cyc_Absyn_Exp*)es->hd != 0){
exps_empty=0;
break;}}}{
# 904
void*base_type=({Cyc_Parse_collapse_type_specifiers(tss,loc);});
# 912
if(declarators == 0){
# 915
void*_tmp12F=base_type;struct Cyc_List_List*_tmp130;struct _tuple0*_tmp131;struct Cyc_List_List*_tmp134;int _tmp133;struct _tuple0*_tmp132;struct Cyc_Absyn_Datatypedecl**_tmp135;struct Cyc_List_List*_tmp138;struct _tuple0*_tmp137;enum Cyc_Absyn_AggrKind _tmp136;struct Cyc_Absyn_Datatypedecl*_tmp139;struct Cyc_Absyn_Enumdecl*_tmp13A;struct Cyc_Absyn_Aggrdecl*_tmp13B;switch(*((int*)_tmp12F)){case 10U: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp12F)->f1)->r)){case 0U: _LL15: _tmp13B=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp12F)->f1)->r)->f1;_LL16: {struct Cyc_Absyn_Aggrdecl*ad=_tmp13B;
# 917
({struct Cyc_List_List*_tmp1093=({Cyc_List_append(ad->attributes,atts);});ad->attributes=_tmp1093;});
ad->sc=s;{
struct Cyc_List_List*_tmp13E=({struct Cyc_List_List*_tmp13D=_cycalloc(sizeof(*_tmp13D));({struct Cyc_Absyn_Decl*_tmp1095=({({void*_tmp1094=(void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp13C=_cycalloc(sizeof(*_tmp13C));_tmp13C->tag=5U,_tmp13C->f1=ad;_tmp13C;});Cyc_Absyn_new_decl(_tmp1094,loc);});});_tmp13D->hd=_tmp1095;}),_tmp13D->tl=0;_tmp13D;});_npop_handler(0U);return _tmp13E;}}case 1U: _LL17: _tmp13A=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp12F)->f1)->r)->f1;_LL18: {struct Cyc_Absyn_Enumdecl*ed=_tmp13A;
# 921
if(atts != 0)({void*_tmp13F=0U;({unsigned _tmp1097=loc;struct _fat_ptr _tmp1096=({const char*_tmp140="attributes on enum not supported";_tag_fat(_tmp140,sizeof(char),33U);});Cyc_Warn_err(_tmp1097,_tmp1096,_tag_fat(_tmp13F,sizeof(void*),0U));});});ed->sc=s;{
# 923
struct Cyc_List_List*_tmp143=({struct Cyc_List_List*_tmp142=_cycalloc(sizeof(*_tmp142));({struct Cyc_Absyn_Decl*_tmp1099=({({void*_tmp1098=(void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp141=_cycalloc(sizeof(*_tmp141));_tmp141->tag=7U,_tmp141->f1=ed;_tmp141;});Cyc_Absyn_new_decl(_tmp1098,loc);});});_tmp142->hd=_tmp1099;}),_tmp142->tl=0;_tmp142;});_npop_handler(0U);return _tmp143;}}default: _LL19: _tmp139=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp12F)->f1)->r)->f1;_LL1A: {struct Cyc_Absyn_Datatypedecl*dd=_tmp139;
# 925
if(atts != 0)({void*_tmp144=0U;({unsigned _tmp109B=loc;struct _fat_ptr _tmp109A=({const char*_tmp145="attributes on datatypes not supported";_tag_fat(_tmp145,sizeof(char),38U);});Cyc_Warn_err(_tmp109B,_tmp109A,_tag_fat(_tmp144,sizeof(void*),0U));});});dd->sc=s;{
# 927
struct Cyc_List_List*_tmp148=({struct Cyc_List_List*_tmp147=_cycalloc(sizeof(*_tmp147));({struct Cyc_Absyn_Decl*_tmp109D=({({void*_tmp109C=(void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp146=_cycalloc(sizeof(*_tmp146));_tmp146->tag=6U,_tmp146->f1=dd;_tmp146;});Cyc_Absyn_new_decl(_tmp109C,loc);});});_tmp147->hd=_tmp109D;}),_tmp147->tl=0;_tmp147;});_npop_handler(0U);return _tmp148;}}}case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)){case 22U: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)->f1).UnknownAggr).tag == 1){_LL1B: _tmp136=(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)->f1).UnknownAggr).val).f1;_tmp137=(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)->f1).UnknownAggr).val).f2;_tmp138=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f2;_LL1C: {enum Cyc_Absyn_AggrKind k=_tmp136;struct _tuple0*n=_tmp137;struct Cyc_List_List*ts=_tmp138;
# 929
struct Cyc_List_List*ts2=({({struct Cyc_List_List*(*_tmp109F)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp14F)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp14F;});unsigned _tmp109E=loc;_tmp109F(Cyc_Parse_typ2tvar,_tmp109E,ts);});});
struct Cyc_Absyn_Aggrdecl*ad=({struct Cyc_Absyn_Aggrdecl*_tmp14E=_cycalloc(sizeof(*_tmp14E));_tmp14E->kind=k,_tmp14E->sc=s,_tmp14E->name=n,_tmp14E->tvs=ts2,_tmp14E->impl=0,_tmp14E->attributes=0,_tmp14E->expected_mem_kind=0;_tmp14E;});
if(atts != 0)({void*_tmp149=0U;({unsigned _tmp10A1=loc;struct _fat_ptr _tmp10A0=({const char*_tmp14A="bad attributes on type declaration";_tag_fat(_tmp14A,sizeof(char),35U);});Cyc_Warn_err(_tmp10A1,_tmp10A0,_tag_fat(_tmp149,sizeof(void*),0U));});});{struct Cyc_List_List*_tmp14D=({struct Cyc_List_List*_tmp14C=_cycalloc(sizeof(*_tmp14C));
({struct Cyc_Absyn_Decl*_tmp10A3=({({void*_tmp10A2=(void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp14B=_cycalloc(sizeof(*_tmp14B));_tmp14B->tag=5U,_tmp14B->f1=ad;_tmp14B;});Cyc_Absyn_new_decl(_tmp10A2,loc);});});_tmp14C->hd=_tmp10A3;}),_tmp14C->tl=0;_tmp14C;});_npop_handler(0U);return _tmp14D;}}}else{goto _LL25;}case 20U: if(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)->f1).KnownDatatype).tag == 2){_LL1D: _tmp135=((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)->f1).KnownDatatype).val;_LL1E: {struct Cyc_Absyn_Datatypedecl**tudp=_tmp135;
# 934
if(atts != 0)({void*_tmp150=0U;({unsigned _tmp10A5=loc;struct _fat_ptr _tmp10A4=({const char*_tmp151="bad attributes on datatype";_tag_fat(_tmp151,sizeof(char),27U);});Cyc_Warn_err(_tmp10A5,_tmp10A4,_tag_fat(_tmp150,sizeof(void*),0U));});});{struct Cyc_List_List*_tmp154=({struct Cyc_List_List*_tmp153=_cycalloc(sizeof(*_tmp153));
({struct Cyc_Absyn_Decl*_tmp10A7=({({void*_tmp10A6=(void*)({struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*_tmp152=_cycalloc(sizeof(*_tmp152));_tmp152->tag=6U,_tmp152->f1=*tudp;_tmp152;});Cyc_Absyn_new_decl(_tmp10A6,loc);});});_tmp153->hd=_tmp10A7;}),_tmp153->tl=0;_tmp153;});_npop_handler(0U);return _tmp154;}}}else{_LL1F: _tmp132=(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)->f1).UnknownDatatype).val).name;_tmp133=(((((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)->f1).UnknownDatatype).val).is_extensible;_tmp134=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f2;_LL20: {struct _tuple0*n=_tmp132;int isx=_tmp133;struct Cyc_List_List*ts=_tmp134;
# 937
struct Cyc_List_List*ts2=({({struct Cyc_List_List*(*_tmp10A9)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp159)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp159;});unsigned _tmp10A8=loc;_tmp10A9(Cyc_Parse_typ2tvar,_tmp10A8,ts);});});
struct Cyc_Absyn_Decl*tud=({Cyc_Absyn_datatype_decl(s,n,ts2,0,isx,loc);});
if(atts != 0)({void*_tmp155=0U;({unsigned _tmp10AB=loc;struct _fat_ptr _tmp10AA=({const char*_tmp156="bad attributes on datatype";_tag_fat(_tmp156,sizeof(char),27U);});Cyc_Warn_err(_tmp10AB,_tmp10AA,_tag_fat(_tmp155,sizeof(void*),0U));});});{struct Cyc_List_List*_tmp158=({struct Cyc_List_List*_tmp157=_cycalloc(sizeof(*_tmp157));
_tmp157->hd=tud,_tmp157->tl=0;_tmp157;});_npop_handler(0U);return _tmp158;}}}case 17U: _LL21: _tmp131=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)->f1;_LL22: {struct _tuple0*n=_tmp131;
# 942
struct Cyc_Absyn_Enumdecl*ed=({struct Cyc_Absyn_Enumdecl*_tmp160=_cycalloc(sizeof(*_tmp160));_tmp160->sc=s,_tmp160->name=n,_tmp160->fields=0;_tmp160;});
if(atts != 0)({void*_tmp15A=0U;({unsigned _tmp10AD=loc;struct _fat_ptr _tmp10AC=({const char*_tmp15B="bad attributes on enum";_tag_fat(_tmp15B,sizeof(char),23U);});Cyc_Warn_err(_tmp10AD,_tmp10AC,_tag_fat(_tmp15A,sizeof(void*),0U));});});{struct Cyc_List_List*_tmp15F=({struct Cyc_List_List*_tmp15E=_cycalloc(sizeof(*_tmp15E));
({struct Cyc_Absyn_Decl*_tmp10AF=({struct Cyc_Absyn_Decl*_tmp15D=_cycalloc(sizeof(*_tmp15D));({void*_tmp10AE=(void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp15C=_cycalloc(sizeof(*_tmp15C));_tmp15C->tag=7U,_tmp15C->f1=ed;_tmp15C;});_tmp15D->r=_tmp10AE;}),_tmp15D->loc=loc;_tmp15D;});_tmp15E->hd=_tmp10AF;}),_tmp15E->tl=0;_tmp15E;});_npop_handler(0U);return _tmp15F;}}case 18U: _LL23: _tmp130=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp12F)->f1)->f1;_LL24: {struct Cyc_List_List*fs=_tmp130;
# 948
struct Cyc_Absyn_Enumdecl*ed=({struct Cyc_Absyn_Enumdecl*_tmp168=_cycalloc(sizeof(*_tmp168));_tmp168->sc=s,({struct _tuple0*_tmp10B1=({Cyc_Parse_gensym_enum();});_tmp168->name=_tmp10B1;}),({struct Cyc_Core_Opt*_tmp10B0=({struct Cyc_Core_Opt*_tmp167=_cycalloc(sizeof(*_tmp167));_tmp167->v=fs;_tmp167;});_tmp168->fields=_tmp10B0;});_tmp168;});
if(atts != 0)({void*_tmp161=0U;({unsigned _tmp10B3=loc;struct _fat_ptr _tmp10B2=({const char*_tmp162="bad attributes on enum";_tag_fat(_tmp162,sizeof(char),23U);});Cyc_Warn_err(_tmp10B3,_tmp10B2,_tag_fat(_tmp161,sizeof(void*),0U));});});{struct Cyc_List_List*_tmp166=({struct Cyc_List_List*_tmp165=_cycalloc(sizeof(*_tmp165));
({struct Cyc_Absyn_Decl*_tmp10B5=({struct Cyc_Absyn_Decl*_tmp164=_cycalloc(sizeof(*_tmp164));({void*_tmp10B4=(void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp163=_cycalloc(sizeof(*_tmp163));_tmp163->tag=7U,_tmp163->f1=ed;_tmp163;});_tmp164->r=_tmp10B4;}),_tmp164->loc=loc;_tmp164;});_tmp165->hd=_tmp10B5;}),_tmp165->tl=0;_tmp165;});_npop_handler(0U);return _tmp166;}}default: goto _LL25;}default: _LL25: _LL26:
({void*_tmp169=0U;({unsigned _tmp10B7=loc;struct _fat_ptr _tmp10B6=({const char*_tmp16A="missing declarator";_tag_fat(_tmp16A,sizeof(char),19U);});Cyc_Warn_err(_tmp10B7,_tmp10B6,_tag_fat(_tmp169,sizeof(void*),0U));});});{struct Cyc_List_List*_tmp16B=0;_npop_handler(0U);return _tmp16B;}}_LL14:;}else{
# 955
struct Cyc_List_List*fields=({Cyc_Parse_apply_tmss(mkrgn,tq,base_type,declarators,atts);});
if(istypedef){
# 960
if(!exps_empty)
({void*_tmp16C=0U;({unsigned _tmp10B9=loc;struct _fat_ptr _tmp10B8=({const char*_tmp16D="initializer in typedef declaration";_tag_fat(_tmp16D,sizeof(char),35U);});Cyc_Warn_err(_tmp10B9,_tmp10B8,_tag_fat(_tmp16C,sizeof(void*),0U));});});{
# 960
struct Cyc_List_List*decls=({({struct Cyc_List_List*(*_tmp10BB)(struct Cyc_Absyn_Decl*(*f)(unsigned,struct _tuple16*),unsigned env,struct Cyc_List_List*x)=({
# 962
struct Cyc_List_List*(*_tmp16F)(struct Cyc_Absyn_Decl*(*f)(unsigned,struct _tuple16*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Decl*(*f)(unsigned,struct _tuple16*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp16F;});unsigned _tmp10BA=loc;_tmp10BB(Cyc_Parse_v_typ_to_typedef,_tmp10BA,fields);});});
struct Cyc_List_List*_tmp16E=decls;_npop_handler(0U);return _tmp16E;}}else{
# 966
struct Cyc_List_List*decls=0;
{struct Cyc_List_List*ds=fields;for(0;ds != 0;(ds=ds->tl,exprs=((struct Cyc_List_List*)_check_null(exprs))->tl)){
struct _tuple16*_stmttmp11=(struct _tuple16*)ds->hd;struct Cyc_List_List*_tmp175;struct Cyc_List_List*_tmp174;void*_tmp173;struct Cyc_Absyn_Tqual _tmp172;struct _tuple0*_tmp171;unsigned _tmp170;_LL28: _tmp170=_stmttmp11->f1;_tmp171=_stmttmp11->f2;_tmp172=_stmttmp11->f3;_tmp173=_stmttmp11->f4;_tmp174=_stmttmp11->f5;_tmp175=_stmttmp11->f6;_LL29: {unsigned varloc=_tmp170;struct _tuple0*x=_tmp171;struct Cyc_Absyn_Tqual tq2=_tmp172;void*t2=_tmp173;struct Cyc_List_List*tvs2=_tmp174;struct Cyc_List_List*atts2=_tmp175;
if(tvs2 != 0)
({void*_tmp176=0U;({unsigned _tmp10BD=loc;struct _fat_ptr _tmp10BC=({const char*_tmp177="bad type params, ignoring";_tag_fat(_tmp177,sizeof(char),26U);});Cyc_Warn_warn(_tmp10BD,_tmp10BC,_tag_fat(_tmp176,sizeof(void*),0U));});});
# 969
if(exprs == 0)
# 972
({void*_tmp178=0U;({int(*_tmp10C0)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp17A)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Parse_parse_abort;_tmp17A;});unsigned _tmp10BF=loc;struct _fat_ptr _tmp10BE=({const char*_tmp179="unexpected NULL in parse!";_tag_fat(_tmp179,sizeof(char),26U);});_tmp10C0(_tmp10BF,_tmp10BE,_tag_fat(_tmp178,sizeof(void*),0U));});});{
# 969
struct Cyc_Absyn_Exp*eopt=(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(exprs))->hd;
# 974
struct Cyc_Absyn_Vardecl*vd=({Cyc_Absyn_new_vardecl(varloc,x,t2,eopt);});
vd->tq=tq2;
vd->sc=s;
vd->attributes=atts2;{
struct Cyc_Absyn_Decl*d=({struct Cyc_Absyn_Decl*_tmp17D=_cycalloc(sizeof(*_tmp17D));({void*_tmp10C1=(void*)({struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*_tmp17C=_cycalloc(sizeof(*_tmp17C));_tmp17C->tag=0U,_tmp17C->f1=vd;_tmp17C;});_tmp17D->r=_tmp10C1;}),_tmp17D->loc=loc;_tmp17D;});
decls=({struct Cyc_List_List*_tmp17B=_cycalloc(sizeof(*_tmp17B));_tmp17B->hd=d,_tmp17B->tl=decls;_tmp17B;});}}}}}{
# 981
struct Cyc_List_List*_tmp17E=({Cyc_List_imp_rev(decls);});_npop_handler(0U);return _tmp17E;}}}}}}}}}
# 877
;_pop_region();}
# 987
static struct Cyc_Absyn_Kind*Cyc_Parse_id_to_kind(struct _fat_ptr s,unsigned loc){
if(({Cyc_strlen((struct _fat_ptr)s);})== (unsigned long)1 ||({Cyc_strlen((struct _fat_ptr)s);})== (unsigned long)2){
char _stmttmp12=*((const char*)_check_fat_subscript(s,sizeof(char),0));char _tmp180=_stmttmp12;switch(_tmp180){case 65U: _LL1: _LL2:
 return& Cyc_Tcutil_ak;case 77U: _LL3: _LL4:
 return& Cyc_Tcutil_mk;case 66U: _LL5: _LL6:
 return& Cyc_Tcutil_bk;case 82U: _LL7: _LL8:
 return& Cyc_Tcutil_rk;case 88U: _LL9: _LLA:
 return& Cyc_Tcutil_xrk;case 69U: _LLB: _LLC:
 return& Cyc_Tcutil_ek;case 73U: _LLD: _LLE:
 return& Cyc_Tcutil_ik;case 85U: _LLF: _LL10:
# 998
{char _stmttmp13=*((const char*)_check_fat_subscript(s,sizeof(char),1));char _tmp181=_stmttmp13;switch(_tmp181){case 82U: _LL16: _LL17:
 return& Cyc_Tcutil_urk;case 65U: _LL18: _LL19:
 return& Cyc_Tcutil_uak;case 77U: _LL1A: _LL1B:
 return& Cyc_Tcutil_umk;case 66U: _LL1C: _LL1D:
 return& Cyc_Tcutil_ubk;default: _LL1E: _LL1F:
 goto _LL15;}_LL15:;}
# 1005
goto _LL0;case 84U: _LL11: _LL12:
# 1007
{char _stmttmp14=*((const char*)_check_fat_subscript(s,sizeof(char),1));char _tmp182=_stmttmp14;switch(_tmp182){case 82U: _LL21: _LL22:
 return& Cyc_Tcutil_trk;case 65U: _LL23: _LL24:
 return& Cyc_Tcutil_tak;case 77U: _LL25: _LL26:
 return& Cyc_Tcutil_tmk;case 66U: _LL27: _LL28:
 return& Cyc_Tcutil_tbk;default: _LL29: _LL2A:
 goto _LL20;}_LL20:;}
# 1014
goto _LL0;default: _LL13: _LL14:
 goto _LL0;}_LL0:;}
# 988
({struct Cyc_String_pa_PrintArg_struct _tmp185=({struct Cyc_String_pa_PrintArg_struct _tmpF8D;_tmpF8D.tag=0U,_tmpF8D.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmpF8D;});struct Cyc_Int_pa_PrintArg_struct _tmp186=({struct Cyc_Int_pa_PrintArg_struct _tmpF8C;_tmpF8C.tag=1U,({
# 1017
unsigned long _tmp10C2=(unsigned long)((int)({Cyc_strlen((struct _fat_ptr)s);}));_tmpF8C.f1=_tmp10C2;});_tmpF8C;});void*_tmp183[2U];_tmp183[0]=& _tmp185,_tmp183[1]=& _tmp186;({unsigned _tmp10C4=loc;struct _fat_ptr _tmp10C3=({const char*_tmp184="bad kind: %s; strlen=%d";_tag_fat(_tmp184,sizeof(char),24U);});Cyc_Warn_err(_tmp10C4,_tmp10C3,_tag_fat(_tmp183,sizeof(void*),2U));});});
return& Cyc_Tcutil_bk;}
# 1022
static struct _fat_ptr Cyc_Parse_exp2string(unsigned loc,struct Cyc_Absyn_Exp*e){
void*_stmttmp15=e->r;void*_tmp188=_stmttmp15;struct _fat_ptr _tmp189;if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp188)->tag == 0U){if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp188)->f1).String_c).tag == 8){_LL1: _tmp189=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp188)->f1).String_c).val;_LL2: {struct _fat_ptr s=_tmp189;
return s;}}else{goto _LL3;}}else{_LL3: _LL4:
# 1026
({void*_tmp18A=0U;({unsigned _tmp10C6=loc;struct _fat_ptr _tmp10C5=({const char*_tmp18B="expecting string constant";_tag_fat(_tmp18B,sizeof(char),26U);});Cyc_Warn_err(_tmp10C6,_tmp10C5,_tag_fat(_tmp18A,sizeof(void*),0U));});});
return _tag_fat(0,0,0);}_LL0:;}
# 1032
static unsigned Cyc_Parse_cnst2uint(unsigned loc,union Cyc_Absyn_Cnst x){
union Cyc_Absyn_Cnst _tmp18D=x;long long _tmp18E;char _tmp18F;int _tmp190;switch((_tmp18D.LongLong_c).tag){case 5U: _LL1: _tmp190=((_tmp18D.Int_c).val).f2;_LL2: {int i=_tmp190;
return(unsigned)i;}case 2U: _LL3: _tmp18F=((_tmp18D.Char_c).val).f2;_LL4: {char c=_tmp18F;
return(unsigned)c;}case 6U: _LL5: _tmp18E=((_tmp18D.LongLong_c).val).f2;_LL6: {long long x=_tmp18E;
# 1037
unsigned long long y=(unsigned long long)x;
if(y > (unsigned long long)-1)
({void*_tmp191=0U;({unsigned _tmp10C8=loc;struct _fat_ptr _tmp10C7=({const char*_tmp192="integer constant too large";_tag_fat(_tmp192,sizeof(char),27U);});Cyc_Warn_err(_tmp10C8,_tmp10C7,_tag_fat(_tmp191,sizeof(void*),0U));});});
# 1038
return(unsigned)x;}default: _LL7: _LL8:
# 1042
({struct Cyc_String_pa_PrintArg_struct _tmp195=({struct Cyc_String_pa_PrintArg_struct _tmpF8E;_tmpF8E.tag=0U,({struct _fat_ptr _tmp10C9=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_cnst2string(x);}));_tmpF8E.f1=_tmp10C9;});_tmpF8E;});void*_tmp193[1U];_tmp193[0]=& _tmp195;({unsigned _tmp10CB=loc;struct _fat_ptr _tmp10CA=({const char*_tmp194="expected integer constant but found %s";_tag_fat(_tmp194,sizeof(char),39U);});Cyc_Warn_err(_tmp10CB,_tmp10CA,_tag_fat(_tmp193,sizeof(void*),1U));});});
return 0U;}_LL0:;}
# 1048
static struct Cyc_Absyn_Exp*Cyc_Parse_pat2exp(struct Cyc_Absyn_Pat*p){
void*_stmttmp16=p->r;void*_tmp197=_stmttmp16;struct Cyc_Absyn_Exp*_tmp198;struct Cyc_List_List*_tmp19A;struct _tuple0*_tmp199;int _tmp19C;struct _fat_ptr _tmp19B;char _tmp19D;int _tmp19F;enum Cyc_Absyn_Sign _tmp19E;struct Cyc_Absyn_Pat*_tmp1A0;struct Cyc_Absyn_Vardecl*_tmp1A1;struct _tuple0*_tmp1A2;switch(*((int*)_tmp197)){case 15U: _LL1: _tmp1A2=((struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct*)_tmp197)->f1;_LL2: {struct _tuple0*x=_tmp1A2;
return({Cyc_Absyn_unknownid_exp(x,p->loc);});}case 3U: if(((struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*)((struct Cyc_Absyn_Pat*)((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp197)->f2)->r)->tag == 0U){_LL3: _tmp1A1=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp197)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp1A1;
# 1052
return({struct Cyc_Absyn_Exp*(*_tmp1A3)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmp1A4=({Cyc_Absyn_unknownid_exp(vd->name,p->loc);});unsigned _tmp1A5=p->loc;_tmp1A3(_tmp1A4,_tmp1A5);});}}else{goto _LL13;}case 6U: _LL5: _tmp1A0=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp197)->f1;_LL6: {struct Cyc_Absyn_Pat*p2=_tmp1A0;
return({struct Cyc_Absyn_Exp*(*_tmp1A6)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_address_exp;struct Cyc_Absyn_Exp*_tmp1A7=({Cyc_Parse_pat2exp(p2);});unsigned _tmp1A8=p->loc;_tmp1A6(_tmp1A7,_tmp1A8);});}case 9U: _LL7: _LL8:
 return({Cyc_Absyn_null_exp(p->loc);});case 10U: _LL9: _tmp19E=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp197)->f1;_tmp19F=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp197)->f2;_LLA: {enum Cyc_Absyn_Sign s=_tmp19E;int i=_tmp19F;
return({Cyc_Absyn_int_exp(s,i,p->loc);});}case 11U: _LLB: _tmp19D=((struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*)_tmp197)->f1;_LLC: {char c=_tmp19D;
return({Cyc_Absyn_char_exp(c,p->loc);});}case 12U: _LLD: _tmp19B=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp197)->f1;_tmp19C=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp197)->f2;_LLE: {struct _fat_ptr s=_tmp19B;int i=_tmp19C;
return({Cyc_Absyn_float_exp(s,i,p->loc);});}case 16U: if(((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp197)->f3 == 0){_LLF: _tmp199=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp197)->f1;_tmp19A=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp197)->f2;_LL10: {struct _tuple0*x=_tmp199;struct Cyc_List_List*ps=_tmp19A;
# 1059
struct Cyc_Absyn_Exp*e1=({Cyc_Absyn_unknownid_exp(x,p->loc);});
struct Cyc_List_List*es=({({struct Cyc_List_List*(*_tmp10CC)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp1A9)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Exp*(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*x))Cyc_List_map;_tmp1A9;});_tmp10CC(Cyc_Parse_pat2exp,ps);});});
return({Cyc_Absyn_unknowncall_exp(e1,es,p->loc);});}}else{goto _LL13;}case 17U: _LL11: _tmp198=((struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*)_tmp197)->f1;_LL12: {struct Cyc_Absyn_Exp*e=_tmp198;
return e;}default: _LL13: _LL14:
# 1064
({void*_tmp1AA=0U;({unsigned _tmp10CE=p->loc;struct _fat_ptr _tmp10CD=({const char*_tmp1AB="cannot mix patterns and expressions in case";_tag_fat(_tmp1AB,sizeof(char),44U);});Cyc_Warn_err(_tmp10CE,_tmp10CD,_tag_fat(_tmp1AA,sizeof(void*),0U));});});
return({Cyc_Absyn_null_exp(p->loc);});}_LL0:;}struct _union_YYSTYPE_Int_tok{int tag;union Cyc_Absyn_Cnst val;};struct _union_YYSTYPE_Char_tok{int tag;char val;};struct _union_YYSTYPE_String_tok{int tag;struct _fat_ptr val;};struct _union_YYSTYPE_Stringopt_tok{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_QualId_tok{int tag;struct _tuple0*val;};struct _tuple21{int f1;struct _fat_ptr f2;};struct _union_YYSTYPE_Asm_tok{int tag;struct _tuple21 val;};struct _union_YYSTYPE_Exp_tok{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_Stmt_tok{int tag;struct Cyc_Absyn_Stmt*val;};struct _tuple22{unsigned f1;void*f2;void*f3;};struct _union_YYSTYPE_YY1{int tag;struct _tuple22*val;};struct _union_YYSTYPE_YY2{int tag;void*val;};struct _union_YYSTYPE_YY3{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY4{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY5{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY6{int tag;enum Cyc_Absyn_Primop val;};struct _union_YYSTYPE_YY7{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_YY8{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY9{int tag;struct Cyc_Absyn_Pat*val;};struct _tuple23{struct Cyc_List_List*f1;int f2;};struct _union_YYSTYPE_YY10{int tag;struct _tuple23*val;};struct _union_YYSTYPE_YY11{int tag;struct Cyc_List_List*val;};struct _tuple24{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};struct _union_YYSTYPE_YY12{int tag;struct _tuple24*val;};struct _union_YYSTYPE_YY13{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY14{int tag;struct _tuple23*val;};struct _union_YYSTYPE_YY15{int tag;struct Cyc_Absyn_Fndecl*val;};struct _union_YYSTYPE_YY16{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY17{int tag;struct Cyc_Parse_Declaration_spec val;};struct _union_YYSTYPE_YY18{int tag;struct _tuple13 val;};struct _union_YYSTYPE_YY19{int tag;struct _tuple14*val;};struct _union_YYSTYPE_YY20{int tag;enum Cyc_Parse_Storage_class*val;};struct _union_YYSTYPE_YY21{int tag;struct Cyc_Parse_Type_specifier val;};struct _union_YYSTYPE_YY22{int tag;enum Cyc_Absyn_AggrKind val;};struct _union_YYSTYPE_YY23{int tag;struct Cyc_Absyn_Tqual val;};struct _union_YYSTYPE_YY24{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY25{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY26{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY27{int tag;struct Cyc_Parse_Declarator val;};struct _tuple25{struct Cyc_Parse_Declarator f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct _union_YYSTYPE_YY28{int tag;struct _tuple25*val;};struct _union_YYSTYPE_YY29{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY30{int tag;struct Cyc_Parse_Abstractdeclarator val;};struct _union_YYSTYPE_YY31{int tag;int val;};struct _union_YYSTYPE_YY32{int tag;enum Cyc_Absyn_Scope val;};struct _union_YYSTYPE_YY33{int tag;struct Cyc_Absyn_Datatypefield*val;};struct _union_YYSTYPE_YY34{int tag;struct Cyc_List_List*val;};struct _tuple26{struct Cyc_Absyn_Tqual f1;struct Cyc_Parse_Type_specifier f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY35{int tag;struct _tuple26 val;};struct _union_YYSTYPE_YY36{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY37{int tag;struct _tuple9*val;};struct _union_YYSTYPE_YY38{int tag;struct Cyc_List_List*val;};struct _tuple27{struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;};struct _union_YYSTYPE_YY39{int tag;struct _tuple27*val;};struct _union_YYSTYPE_YY40{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY41{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY42{int tag;void*val;};struct _union_YYSTYPE_YY43{int tag;struct Cyc_Absyn_Kind*val;};struct _union_YYSTYPE_YY44{int tag;void*val;};struct _union_YYSTYPE_YY45{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY46{int tag;void*val;};struct _union_YYSTYPE_YY47{int tag;struct Cyc_Absyn_Enumfield*val;};struct _union_YYSTYPE_YY48{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY49{int tag;void*val;};struct _union_YYSTYPE_YY50{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY51{int tag;void*val;};struct _union_YYSTYPE_YY52{int tag;struct Cyc_List_List*val;};struct _tuple28{struct Cyc_List_List*f1;unsigned f2;};struct _union_YYSTYPE_YY53{int tag;struct _tuple28*val;};struct _union_YYSTYPE_YY54{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY55{int tag;void*val;};struct _union_YYSTYPE_YY56{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY57{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_YY58{int tag;void*val;};struct _tuple29{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY59{int tag;struct _tuple29*val;};struct _union_YYSTYPE_YY60{int tag;struct _tuple8*val;};struct _union_YYSTYPE_YY61{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY62{int tag;struct Cyc_List_List*val;};struct _tuple30{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};struct _union_YYSTYPE_YY63{int tag;struct _tuple30*val;};struct _union_YYSTYPE_YY64{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY65{int tag;int val;};struct _union_YYSTYPE_YY66{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YYINITIALSVAL{int tag;int val;};union Cyc_YYSTYPE{struct _union_YYSTYPE_Int_tok Int_tok;struct _union_YYSTYPE_Char_tok Char_tok;struct _union_YYSTYPE_String_tok String_tok;struct _union_YYSTYPE_Stringopt_tok Stringopt_tok;struct _union_YYSTYPE_QualId_tok QualId_tok;struct _union_YYSTYPE_Asm_tok Asm_tok;struct _union_YYSTYPE_Exp_tok Exp_tok;struct _union_YYSTYPE_Stmt_tok Stmt_tok;struct _union_YYSTYPE_YY1 YY1;struct _union_YYSTYPE_YY2 YY2;struct _union_YYSTYPE_YY3 YY3;struct _union_YYSTYPE_YY4 YY4;struct _union_YYSTYPE_YY5 YY5;struct _union_YYSTYPE_YY6 YY6;struct _union_YYSTYPE_YY7 YY7;struct _union_YYSTYPE_YY8 YY8;struct _union_YYSTYPE_YY9 YY9;struct _union_YYSTYPE_YY10 YY10;struct _union_YYSTYPE_YY11 YY11;struct _union_YYSTYPE_YY12 YY12;struct _union_YYSTYPE_YY13 YY13;struct _union_YYSTYPE_YY14 YY14;struct _union_YYSTYPE_YY15 YY15;struct _union_YYSTYPE_YY16 YY16;struct _union_YYSTYPE_YY17 YY17;struct _union_YYSTYPE_YY18 YY18;struct _union_YYSTYPE_YY19 YY19;struct _union_YYSTYPE_YY20 YY20;struct _union_YYSTYPE_YY21 YY21;struct _union_YYSTYPE_YY22 YY22;struct _union_YYSTYPE_YY23 YY23;struct _union_YYSTYPE_YY24 YY24;struct _union_YYSTYPE_YY25 YY25;struct _union_YYSTYPE_YY26 YY26;struct _union_YYSTYPE_YY27 YY27;struct _union_YYSTYPE_YY28 YY28;struct _union_YYSTYPE_YY29 YY29;struct _union_YYSTYPE_YY30 YY30;struct _union_YYSTYPE_YY31 YY31;struct _union_YYSTYPE_YY32 YY32;struct _union_YYSTYPE_YY33 YY33;struct _union_YYSTYPE_YY34 YY34;struct _union_YYSTYPE_YY35 YY35;struct _union_YYSTYPE_YY36 YY36;struct _union_YYSTYPE_YY37 YY37;struct _union_YYSTYPE_YY38 YY38;struct _union_YYSTYPE_YY39 YY39;struct _union_YYSTYPE_YY40 YY40;struct _union_YYSTYPE_YY41 YY41;struct _union_YYSTYPE_YY42 YY42;struct _union_YYSTYPE_YY43 YY43;struct _union_YYSTYPE_YY44 YY44;struct _union_YYSTYPE_YY45 YY45;struct _union_YYSTYPE_YY46 YY46;struct _union_YYSTYPE_YY47 YY47;struct _union_YYSTYPE_YY48 YY48;struct _union_YYSTYPE_YY49 YY49;struct _union_YYSTYPE_YY50 YY50;struct _union_YYSTYPE_YY51 YY51;struct _union_YYSTYPE_YY52 YY52;struct _union_YYSTYPE_YY53 YY53;struct _union_YYSTYPE_YY54 YY54;struct _union_YYSTYPE_YY55 YY55;struct _union_YYSTYPE_YY56 YY56;struct _union_YYSTYPE_YY57 YY57;struct _union_YYSTYPE_YY58 YY58;struct _union_YYSTYPE_YY59 YY59;struct _union_YYSTYPE_YY60 YY60;struct _union_YYSTYPE_YY61 YY61;struct _union_YYSTYPE_YY62 YY62;struct _union_YYSTYPE_YY63 YY63;struct _union_YYSTYPE_YY64 YY64;struct _union_YYSTYPE_YY65 YY65;struct _union_YYSTYPE_YY66 YY66;struct _union_YYSTYPE_YYINITIALSVAL YYINITIALSVAL;};
# 1152
static void Cyc_yythrowfail(struct _fat_ptr s){
(int)_throw(({struct Cyc_Core_Failure_exn_struct*_tmp1AD=_cycalloc(sizeof(*_tmp1AD));_tmp1AD->tag=Cyc_Core_Failure,_tmp1AD->f1=s;_tmp1AD;}));}static char _tmp1B1[7U]="cnst_t";
# 1123 "parse.y"
static union Cyc_Absyn_Cnst Cyc_yyget_Int_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1B1,_tmp1B1,_tmp1B1 + 7U};
union Cyc_YYSTYPE*_tmp1AF=yy1;union Cyc_Absyn_Cnst _tmp1B0;if((((union Cyc_YYSTYPE*)_tmp1AF)->Int_tok).tag == 1){_LL1: _tmp1B0=(_tmp1AF->Int_tok).val;_LL2: {union Cyc_Absyn_Cnst yy=_tmp1B0;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1130
static union Cyc_YYSTYPE Cyc_Int_tok(union Cyc_Absyn_Cnst yy1){return({union Cyc_YYSTYPE _tmpF8F;(_tmpF8F.Int_tok).tag=1U,(_tmpF8F.Int_tok).val=yy1;_tmpF8F;});}static char _tmp1B6[5U]="char";
# 1124 "parse.y"
static char Cyc_yyget_Char_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1B6,_tmp1B6,_tmp1B6 + 5U};
union Cyc_YYSTYPE*_tmp1B4=yy1;char _tmp1B5;if((((union Cyc_YYSTYPE*)_tmp1B4)->Char_tok).tag == 2){_LL1: _tmp1B5=(_tmp1B4->Char_tok).val;_LL2: {char yy=_tmp1B5;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1131
static union Cyc_YYSTYPE Cyc_Char_tok(char yy1){return({union Cyc_YYSTYPE _tmpF90;(_tmpF90.Char_tok).tag=2U,(_tmpF90.Char_tok).val=yy1;_tmpF90;});}static char _tmp1BB[13U]="string_t<`H>";
# 1125 "parse.y"
static struct _fat_ptr Cyc_yyget_String_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1BB,_tmp1BB,_tmp1BB + 13U};
union Cyc_YYSTYPE*_tmp1B9=yy1;struct _fat_ptr _tmp1BA;if((((union Cyc_YYSTYPE*)_tmp1B9)->String_tok).tag == 3){_LL1: _tmp1BA=(_tmp1B9->String_tok).val;_LL2: {struct _fat_ptr yy=_tmp1BA;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1132
static union Cyc_YYSTYPE Cyc_String_tok(struct _fat_ptr yy1){return({union Cyc_YYSTYPE _tmpF91;(_tmpF91.String_tok).tag=3U,(_tmpF91.String_tok).val=yy1;_tmpF91;});}static char _tmp1C0[45U]="$(Position::seg_t,booltype_t, ptrbound_t)@`H";
# 1128 "parse.y"
static struct _tuple22*Cyc_yyget_YY1(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1C0,_tmp1C0,_tmp1C0 + 45U};
union Cyc_YYSTYPE*_tmp1BE=yy1;struct _tuple22*_tmp1BF;if((((union Cyc_YYSTYPE*)_tmp1BE)->YY1).tag == 9){_LL1: _tmp1BF=(_tmp1BE->YY1).val;_LL2: {struct _tuple22*yy=_tmp1BF;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1135
static union Cyc_YYSTYPE Cyc_YY1(struct _tuple22*yy1){return({union Cyc_YYSTYPE _tmpF92;(_tmpF92.YY1).tag=9U,(_tmpF92.YY1).val=yy1;_tmpF92;});}static char _tmp1C5[11U]="ptrbound_t";
# 1129 "parse.y"
static void*Cyc_yyget_YY2(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1C5,_tmp1C5,_tmp1C5 + 11U};
union Cyc_YYSTYPE*_tmp1C3=yy1;void*_tmp1C4;if((((union Cyc_YYSTYPE*)_tmp1C3)->YY2).tag == 10){_LL1: _tmp1C4=(_tmp1C3->YY2).val;_LL2: {void*yy=_tmp1C4;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1136
static union Cyc_YYSTYPE Cyc_YY2(void*yy1){return({union Cyc_YYSTYPE _tmpF93;(_tmpF93.YY2).tag=10U,(_tmpF93.YY2).val=yy1;_tmpF93;});}static char _tmp1CA[28U]="list_t<offsetof_field_t,`H>";
# 1130 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY3(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1CA,_tmp1CA,_tmp1CA + 28U};
union Cyc_YYSTYPE*_tmp1C8=yy1;struct Cyc_List_List*_tmp1C9;if((((union Cyc_YYSTYPE*)_tmp1C8)->YY3).tag == 11){_LL1: _tmp1C9=(_tmp1C8->YY3).val;_LL2: {struct Cyc_List_List*yy=_tmp1C9;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1137
static union Cyc_YYSTYPE Cyc_YY3(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpF94;(_tmpF94.YY3).tag=11U,(_tmpF94.YY3).val=yy1;_tmpF94;});}static char _tmp1CF[6U]="exp_t";
# 1131 "parse.y"
static struct Cyc_Absyn_Exp*Cyc_yyget_Exp_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1CF,_tmp1CF,_tmp1CF + 6U};
union Cyc_YYSTYPE*_tmp1CD=yy1;struct Cyc_Absyn_Exp*_tmp1CE;if((((union Cyc_YYSTYPE*)_tmp1CD)->Exp_tok).tag == 7){_LL1: _tmp1CE=(_tmp1CD->Exp_tok).val;_LL2: {struct Cyc_Absyn_Exp*yy=_tmp1CE;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1138
static union Cyc_YYSTYPE Cyc_Exp_tok(struct Cyc_Absyn_Exp*yy1){return({union Cyc_YYSTYPE _tmpF95;(_tmpF95.Exp_tok).tag=7U,(_tmpF95.Exp_tok).val=yy1;_tmpF95;});}static char _tmp1D4[17U]="list_t<exp_t,`H>";
static struct Cyc_List_List*Cyc_yyget_YY4(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1D4,_tmp1D4,_tmp1D4 + 17U};
union Cyc_YYSTYPE*_tmp1D2=yy1;struct Cyc_List_List*_tmp1D3;if((((union Cyc_YYSTYPE*)_tmp1D2)->YY4).tag == 12){_LL1: _tmp1D3=(_tmp1D2->YY4).val;_LL2: {struct Cyc_List_List*yy=_tmp1D3;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1146
static union Cyc_YYSTYPE Cyc_YY4(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpF96;(_tmpF96.YY4).tag=12U,(_tmpF96.YY4).val=yy1;_tmpF96;});}static char _tmp1D9[47U]="list_t<$(list_t<designator_t,`H>,exp_t)@`H,`H>";
# 1140 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY5(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1D9,_tmp1D9,_tmp1D9 + 47U};
union Cyc_YYSTYPE*_tmp1D7=yy1;struct Cyc_List_List*_tmp1D8;if((((union Cyc_YYSTYPE*)_tmp1D7)->YY5).tag == 13){_LL1: _tmp1D8=(_tmp1D7->YY5).val;_LL2: {struct Cyc_List_List*yy=_tmp1D8;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1147
static union Cyc_YYSTYPE Cyc_YY5(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpF97;(_tmpF97.YY5).tag=13U,(_tmpF97.YY5).val=yy1;_tmpF97;});}static char _tmp1DE[9U]="primop_t";
# 1141 "parse.y"
static enum Cyc_Absyn_Primop Cyc_yyget_YY6(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1DE,_tmp1DE,_tmp1DE + 9U};
union Cyc_YYSTYPE*_tmp1DC=yy1;enum Cyc_Absyn_Primop _tmp1DD;if((((union Cyc_YYSTYPE*)_tmp1DC)->YY6).tag == 14){_LL1: _tmp1DD=(_tmp1DC->YY6).val;_LL2: {enum Cyc_Absyn_Primop yy=_tmp1DD;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1148
static union Cyc_YYSTYPE Cyc_YY6(enum Cyc_Absyn_Primop yy1){return({union Cyc_YYSTYPE _tmpF98;(_tmpF98.YY6).tag=14U,(_tmpF98.YY6).val=yy1;_tmpF98;});}static char _tmp1E3[19U]="opt_t<primop_t,`H>";
# 1142 "parse.y"
static struct Cyc_Core_Opt*Cyc_yyget_YY7(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1E3,_tmp1E3,_tmp1E3 + 19U};
union Cyc_YYSTYPE*_tmp1E1=yy1;struct Cyc_Core_Opt*_tmp1E2;if((((union Cyc_YYSTYPE*)_tmp1E1)->YY7).tag == 15){_LL1: _tmp1E2=(_tmp1E1->YY7).val;_LL2: {struct Cyc_Core_Opt*yy=_tmp1E2;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1149
static union Cyc_YYSTYPE Cyc_YY7(struct Cyc_Core_Opt*yy1){return({union Cyc_YYSTYPE _tmpF99;(_tmpF99.YY7).tag=15U,(_tmpF99.YY7).val=yy1;_tmpF99;});}static char _tmp1E8[7U]="qvar_t";
# 1143 "parse.y"
static struct _tuple0*Cyc_yyget_QualId_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1E8,_tmp1E8,_tmp1E8 + 7U};
union Cyc_YYSTYPE*_tmp1E6=yy1;struct _tuple0*_tmp1E7;if((((union Cyc_YYSTYPE*)_tmp1E6)->QualId_tok).tag == 5){_LL1: _tmp1E7=(_tmp1E6->QualId_tok).val;_LL2: {struct _tuple0*yy=_tmp1E7;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1150
static union Cyc_YYSTYPE Cyc_QualId_tok(struct _tuple0*yy1){return({union Cyc_YYSTYPE _tmpF9A;(_tmpF9A.QualId_tok).tag=5U,(_tmpF9A.QualId_tok).val=yy1;_tmpF9A;});}static char _tmp1ED[7U]="stmt_t";
# 1146 "parse.y"
static struct Cyc_Absyn_Stmt*Cyc_yyget_Stmt_tok(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1ED,_tmp1ED,_tmp1ED + 7U};
union Cyc_YYSTYPE*_tmp1EB=yy1;struct Cyc_Absyn_Stmt*_tmp1EC;if((((union Cyc_YYSTYPE*)_tmp1EB)->Stmt_tok).tag == 8){_LL1: _tmp1EC=(_tmp1EB->Stmt_tok).val;_LL2: {struct Cyc_Absyn_Stmt*yy=_tmp1EC;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1153
static union Cyc_YYSTYPE Cyc_Stmt_tok(struct Cyc_Absyn_Stmt*yy1){return({union Cyc_YYSTYPE _tmpF9B;(_tmpF9B.Stmt_tok).tag=8U,(_tmpF9B.Stmt_tok).val=yy1;_tmpF9B;});}static char _tmp1F2[27U]="list_t<switch_clause_t,`H>";
# 1150 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY8(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1F2,_tmp1F2,_tmp1F2 + 27U};
union Cyc_YYSTYPE*_tmp1F0=yy1;struct Cyc_List_List*_tmp1F1;if((((union Cyc_YYSTYPE*)_tmp1F0)->YY8).tag == 16){_LL1: _tmp1F1=(_tmp1F0->YY8).val;_LL2: {struct Cyc_List_List*yy=_tmp1F1;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1157
static union Cyc_YYSTYPE Cyc_YY8(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpF9C;(_tmpF9C.YY8).tag=16U,(_tmpF9C.YY8).val=yy1;_tmpF9C;});}static char _tmp1F7[6U]="pat_t";
# 1151 "parse.y"
static struct Cyc_Absyn_Pat*Cyc_yyget_YY9(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1F7,_tmp1F7,_tmp1F7 + 6U};
union Cyc_YYSTYPE*_tmp1F5=yy1;struct Cyc_Absyn_Pat*_tmp1F6;if((((union Cyc_YYSTYPE*)_tmp1F5)->YY9).tag == 17){_LL1: _tmp1F6=(_tmp1F5->YY9).val;_LL2: {struct Cyc_Absyn_Pat*yy=_tmp1F6;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1158
static union Cyc_YYSTYPE Cyc_YY9(struct Cyc_Absyn_Pat*yy1){return({union Cyc_YYSTYPE _tmpF9D;(_tmpF9D.YY9).tag=17U,(_tmpF9D.YY9).val=yy1;_tmpF9D;});}static char _tmp1FC[28U]="$(list_t<pat_t,`H>,bool)@`H";
# 1156 "parse.y"
static struct _tuple23*Cyc_yyget_YY10(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp1FC,_tmp1FC,_tmp1FC + 28U};
union Cyc_YYSTYPE*_tmp1FA=yy1;struct _tuple23*_tmp1FB;if((((union Cyc_YYSTYPE*)_tmp1FA)->YY10).tag == 18){_LL1: _tmp1FB=(_tmp1FA->YY10).val;_LL2: {struct _tuple23*yy=_tmp1FB;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1163
static union Cyc_YYSTYPE Cyc_YY10(struct _tuple23*yy1){return({union Cyc_YYSTYPE _tmpF9E;(_tmpF9E.YY10).tag=18U,(_tmpF9E.YY10).val=yy1;_tmpF9E;});}static char _tmp201[17U]="list_t<pat_t,`H>";
# 1157 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY11(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp201,_tmp201,_tmp201 + 17U};
union Cyc_YYSTYPE*_tmp1FF=yy1;struct Cyc_List_List*_tmp200;if((((union Cyc_YYSTYPE*)_tmp1FF)->YY11).tag == 19){_LL1: _tmp200=(_tmp1FF->YY11).val;_LL2: {struct Cyc_List_List*yy=_tmp200;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1164
static union Cyc_YYSTYPE Cyc_YY11(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpF9F;(_tmpF9F.YY11).tag=19U,(_tmpF9F.YY11).val=yy1;_tmpF9F;});}static char _tmp206[36U]="$(list_t<designator_t,`H>,pat_t)@`H";
# 1158 "parse.y"
static struct _tuple24*Cyc_yyget_YY12(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp206,_tmp206,_tmp206 + 36U};
union Cyc_YYSTYPE*_tmp204=yy1;struct _tuple24*_tmp205;if((((union Cyc_YYSTYPE*)_tmp204)->YY12).tag == 20){_LL1: _tmp205=(_tmp204->YY12).val;_LL2: {struct _tuple24*yy=_tmp205;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1165
static union Cyc_YYSTYPE Cyc_YY12(struct _tuple24*yy1){return({union Cyc_YYSTYPE _tmpFA0;(_tmpFA0.YY12).tag=20U,(_tmpFA0.YY12).val=yy1;_tmpFA0;});}static char _tmp20B[47U]="list_t<$(list_t<designator_t,`H>,pat_t)@`H,`H>";
# 1159 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY13(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp20B,_tmp20B,_tmp20B + 47U};
union Cyc_YYSTYPE*_tmp209=yy1;struct Cyc_List_List*_tmp20A;if((((union Cyc_YYSTYPE*)_tmp209)->YY13).tag == 21){_LL1: _tmp20A=(_tmp209->YY13).val;_LL2: {struct Cyc_List_List*yy=_tmp20A;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1166
static union Cyc_YYSTYPE Cyc_YY13(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFA1;(_tmpFA1.YY13).tag=21U,(_tmpFA1.YY13).val=yy1;_tmpFA1;});}static char _tmp210[58U]="$(list_t<$(list_t<designator_t,`H>,pat_t)@`H,`H>,bool)@`H";
# 1160 "parse.y"
static struct _tuple23*Cyc_yyget_YY14(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp210,_tmp210,_tmp210 + 58U};
union Cyc_YYSTYPE*_tmp20E=yy1;struct _tuple23*_tmp20F;if((((union Cyc_YYSTYPE*)_tmp20E)->YY14).tag == 22){_LL1: _tmp20F=(_tmp20E->YY14).val;_LL2: {struct _tuple23*yy=_tmp20F;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1167
static union Cyc_YYSTYPE Cyc_YY14(struct _tuple23*yy1){return({union Cyc_YYSTYPE _tmpFA2;(_tmpFA2.YY14).tag=22U,(_tmpFA2.YY14).val=yy1;_tmpFA2;});}static char _tmp215[9U]="fndecl_t";
# 1161 "parse.y"
static struct Cyc_Absyn_Fndecl*Cyc_yyget_YY15(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp215,_tmp215,_tmp215 + 9U};
union Cyc_YYSTYPE*_tmp213=yy1;struct Cyc_Absyn_Fndecl*_tmp214;if((((union Cyc_YYSTYPE*)_tmp213)->YY15).tag == 23){_LL1: _tmp214=(_tmp213->YY15).val;_LL2: {struct Cyc_Absyn_Fndecl*yy=_tmp214;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1168
static union Cyc_YYSTYPE Cyc_YY15(struct Cyc_Absyn_Fndecl*yy1){return({union Cyc_YYSTYPE _tmpFA3;(_tmpFA3.YY15).tag=23U,(_tmpFA3.YY15).val=yy1;_tmpFA3;});}static char _tmp21A[18U]="list_t<decl_t,`H>";
# 1162 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY16(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp21A,_tmp21A,_tmp21A + 18U};
union Cyc_YYSTYPE*_tmp218=yy1;struct Cyc_List_List*_tmp219;if((((union Cyc_YYSTYPE*)_tmp218)->YY16).tag == 24){_LL1: _tmp219=(_tmp218->YY16).val;_LL2: {struct Cyc_List_List*yy=_tmp219;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1169
static union Cyc_YYSTYPE Cyc_YY16(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFA4;(_tmpFA4.YY16).tag=24U,(_tmpFA4.YY16).val=yy1;_tmpFA4;});}static char _tmp21F[12U]="decl_spec_t";
# 1165 "parse.y"
static struct Cyc_Parse_Declaration_spec Cyc_yyget_YY17(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp21F,_tmp21F,_tmp21F + 12U};
union Cyc_YYSTYPE*_tmp21D=yy1;struct Cyc_Parse_Declaration_spec _tmp21E;if((((union Cyc_YYSTYPE*)_tmp21D)->YY17).tag == 25){_LL1: _tmp21E=(_tmp21D->YY17).val;_LL2: {struct Cyc_Parse_Declaration_spec yy=_tmp21E;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1172
static union Cyc_YYSTYPE Cyc_YY17(struct Cyc_Parse_Declaration_spec yy1){return({union Cyc_YYSTYPE _tmpFA5;(_tmpFA5.YY17).tag=25U,(_tmpFA5.YY17).val=yy1;_tmpFA5;});}static char _tmp224[31U]="$(declarator_t<`yy>,exp_opt_t)";
# 1166 "parse.y"
static struct _tuple13 Cyc_yyget_YY18(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp224,_tmp224,_tmp224 + 31U};
union Cyc_YYSTYPE*_tmp222=yy1;struct _tuple13 _tmp223;if((((union Cyc_YYSTYPE*)_tmp222)->YY18).tag == 26){_LL1: _tmp223=(_tmp222->YY18).val;_LL2: {struct _tuple13 yy=_tmp223;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1173
static union Cyc_YYSTYPE Cyc_YY18(struct _tuple13 yy1){return({union Cyc_YYSTYPE _tmpFA6;(_tmpFA6.YY18).tag=26U,(_tmpFA6.YY18).val=yy1;_tmpFA6;});}static char _tmp229[23U]="declarator_list_t<`yy>";
# 1167 "parse.y"
static struct _tuple14*Cyc_yyget_YY19(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp229,_tmp229,_tmp229 + 23U};
union Cyc_YYSTYPE*_tmp227=yy1;struct _tuple14*_tmp228;if((((union Cyc_YYSTYPE*)_tmp227)->YY19).tag == 27){_LL1: _tmp228=(_tmp227->YY19).val;_LL2: {struct _tuple14*yy=_tmp228;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1174
static union Cyc_YYSTYPE Cyc_YY19(struct _tuple14*yy1){return({union Cyc_YYSTYPE _tmpFA7;(_tmpFA7.YY19).tag=27U,(_tmpFA7.YY19).val=yy1;_tmpFA7;});}static char _tmp22E[19U]="storage_class_t@`H";
# 1168 "parse.y"
static enum Cyc_Parse_Storage_class*Cyc_yyget_YY20(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp22E,_tmp22E,_tmp22E + 19U};
union Cyc_YYSTYPE*_tmp22C=yy1;enum Cyc_Parse_Storage_class*_tmp22D;if((((union Cyc_YYSTYPE*)_tmp22C)->YY20).tag == 28){_LL1: _tmp22D=(_tmp22C->YY20).val;_LL2: {enum Cyc_Parse_Storage_class*yy=_tmp22D;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1175
static union Cyc_YYSTYPE Cyc_YY20(enum Cyc_Parse_Storage_class*yy1){return({union Cyc_YYSTYPE _tmpFA8;(_tmpFA8.YY20).tag=28U,(_tmpFA8.YY20).val=yy1;_tmpFA8;});}static char _tmp233[17U]="type_specifier_t";
# 1169 "parse.y"
static struct Cyc_Parse_Type_specifier Cyc_yyget_YY21(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp233,_tmp233,_tmp233 + 17U};
union Cyc_YYSTYPE*_tmp231=yy1;struct Cyc_Parse_Type_specifier _tmp232;if((((union Cyc_YYSTYPE*)_tmp231)->YY21).tag == 29){_LL1: _tmp232=(_tmp231->YY21).val;_LL2: {struct Cyc_Parse_Type_specifier yy=_tmp232;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1176
static union Cyc_YYSTYPE Cyc_YY21(struct Cyc_Parse_Type_specifier yy1){return({union Cyc_YYSTYPE _tmpFA9;(_tmpFA9.YY21).tag=29U,(_tmpFA9.YY21).val=yy1;_tmpFA9;});}static char _tmp238[12U]="aggr_kind_t";
# 1171 "parse.y"
static enum Cyc_Absyn_AggrKind Cyc_yyget_YY22(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp238,_tmp238,_tmp238 + 12U};
union Cyc_YYSTYPE*_tmp236=yy1;enum Cyc_Absyn_AggrKind _tmp237;if((((union Cyc_YYSTYPE*)_tmp236)->YY22).tag == 30){_LL1: _tmp237=(_tmp236->YY22).val;_LL2: {enum Cyc_Absyn_AggrKind yy=_tmp237;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1178
static union Cyc_YYSTYPE Cyc_YY22(enum Cyc_Absyn_AggrKind yy1){return({union Cyc_YYSTYPE _tmpFAA;(_tmpFAA.YY22).tag=30U,(_tmpFAA.YY22).val=yy1;_tmpFAA;});}static char _tmp23D[8U]="tqual_t";
# 1172 "parse.y"
static struct Cyc_Absyn_Tqual Cyc_yyget_YY23(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp23D,_tmp23D,_tmp23D + 8U};
union Cyc_YYSTYPE*_tmp23B=yy1;struct Cyc_Absyn_Tqual _tmp23C;if((((union Cyc_YYSTYPE*)_tmp23B)->YY23).tag == 31){_LL1: _tmp23C=(_tmp23B->YY23).val;_LL2: {struct Cyc_Absyn_Tqual yy=_tmp23C;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1179
static union Cyc_YYSTYPE Cyc_YY23(struct Cyc_Absyn_Tqual yy1){return({union Cyc_YYSTYPE _tmpFAB;(_tmpFAB.YY23).tag=31U,(_tmpFAB.YY23).val=yy1;_tmpFAB;});}static char _tmp242[23U]="list_t<aggrfield_t,`H>";
# 1173 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY24(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp242,_tmp242,_tmp242 + 23U};
union Cyc_YYSTYPE*_tmp240=yy1;struct Cyc_List_List*_tmp241;if((((union Cyc_YYSTYPE*)_tmp240)->YY24).tag == 32){_LL1: _tmp241=(_tmp240->YY24).val;_LL2: {struct Cyc_List_List*yy=_tmp241;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1180
static union Cyc_YYSTYPE Cyc_YY24(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFAC;(_tmpFAC.YY24).tag=32U,(_tmpFAC.YY24).val=yy1;_tmpFAC;});}static char _tmp247[34U]="list_t<list_t<aggrfield_t,`H>,`H>";
# 1174 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY25(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp247,_tmp247,_tmp247 + 34U};
union Cyc_YYSTYPE*_tmp245=yy1;struct Cyc_List_List*_tmp246;if((((union Cyc_YYSTYPE*)_tmp245)->YY25).tag == 33){_LL1: _tmp246=(_tmp245->YY25).val;_LL2: {struct Cyc_List_List*yy=_tmp246;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1181
static union Cyc_YYSTYPE Cyc_YY25(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFAD;(_tmpFAD.YY25).tag=33U,(_tmpFAD.YY25).val=yy1;_tmpFAD;});}static char _tmp24C[33U]="list_t<type_modifier_t<`yy>,`yy>";
# 1175 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY26(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp24C,_tmp24C,_tmp24C + 33U};
union Cyc_YYSTYPE*_tmp24A=yy1;struct Cyc_List_List*_tmp24B;if((((union Cyc_YYSTYPE*)_tmp24A)->YY26).tag == 34){_LL1: _tmp24B=(_tmp24A->YY26).val;_LL2: {struct Cyc_List_List*yy=_tmp24B;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1182
static union Cyc_YYSTYPE Cyc_YY26(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFAE;(_tmpFAE.YY26).tag=34U,(_tmpFAE.YY26).val=yy1;_tmpFAE;});}static char _tmp251[18U]="declarator_t<`yy>";
# 1176 "parse.y"
static struct Cyc_Parse_Declarator Cyc_yyget_YY27(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp251,_tmp251,_tmp251 + 18U};
union Cyc_YYSTYPE*_tmp24F=yy1;struct Cyc_Parse_Declarator _tmp250;if((((union Cyc_YYSTYPE*)_tmp24F)->YY27).tag == 35){_LL1: _tmp250=(_tmp24F->YY27).val;_LL2: {struct Cyc_Parse_Declarator yy=_tmp250;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1183
static union Cyc_YYSTYPE Cyc_YY27(struct Cyc_Parse_Declarator yy1){return({union Cyc_YYSTYPE _tmpFAF;(_tmpFAF.YY27).tag=35U,(_tmpFAF.YY27).val=yy1;_tmpFAF;});}static char _tmp256[45U]="$(declarator_t<`yy>,exp_opt_t,exp_opt_t)@`yy";
# 1177 "parse.y"
static struct _tuple25*Cyc_yyget_YY28(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp256,_tmp256,_tmp256 + 45U};
union Cyc_YYSTYPE*_tmp254=yy1;struct _tuple25*_tmp255;if((((union Cyc_YYSTYPE*)_tmp254)->YY28).tag == 36){_LL1: _tmp255=(_tmp254->YY28).val;_LL2: {struct _tuple25*yy=_tmp255;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1184
static union Cyc_YYSTYPE Cyc_YY28(struct _tuple25*yy1){return({union Cyc_YYSTYPE _tmpFB0;(_tmpFB0.YY28).tag=36U,(_tmpFB0.YY28).val=yy1;_tmpFB0;});}static char _tmp25B[57U]="list_t<$(declarator_t<`yy>,exp_opt_t,exp_opt_t)@`yy,`yy>";
# 1178 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY29(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp25B,_tmp25B,_tmp25B + 57U};
union Cyc_YYSTYPE*_tmp259=yy1;struct Cyc_List_List*_tmp25A;if((((union Cyc_YYSTYPE*)_tmp259)->YY29).tag == 37){_LL1: _tmp25A=(_tmp259->YY29).val;_LL2: {struct Cyc_List_List*yy=_tmp25A;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1185
static union Cyc_YYSTYPE Cyc_YY29(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFB1;(_tmpFB1.YY29).tag=37U,(_tmpFB1.YY29).val=yy1;_tmpFB1;});}static char _tmp260[26U]="abstractdeclarator_t<`yy>";
# 1179 "parse.y"
static struct Cyc_Parse_Abstractdeclarator Cyc_yyget_YY30(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp260,_tmp260,_tmp260 + 26U};
union Cyc_YYSTYPE*_tmp25E=yy1;struct Cyc_Parse_Abstractdeclarator _tmp25F;if((((union Cyc_YYSTYPE*)_tmp25E)->YY30).tag == 38){_LL1: _tmp25F=(_tmp25E->YY30).val;_LL2: {struct Cyc_Parse_Abstractdeclarator yy=_tmp25F;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1186
static union Cyc_YYSTYPE Cyc_YY30(struct Cyc_Parse_Abstractdeclarator yy1){return({union Cyc_YYSTYPE _tmpFB2;(_tmpFB2.YY30).tag=38U,(_tmpFB2.YY30).val=yy1;_tmpFB2;});}static char _tmp265[5U]="bool";
# 1180 "parse.y"
static int Cyc_yyget_YY31(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp265,_tmp265,_tmp265 + 5U};
union Cyc_YYSTYPE*_tmp263=yy1;int _tmp264;if((((union Cyc_YYSTYPE*)_tmp263)->YY31).tag == 39){_LL1: _tmp264=(_tmp263->YY31).val;_LL2: {int yy=_tmp264;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1187
static union Cyc_YYSTYPE Cyc_YY31(int yy1){return({union Cyc_YYSTYPE _tmpFB3;(_tmpFB3.YY31).tag=39U,(_tmpFB3.YY31).val=yy1;_tmpFB3;});}static char _tmp26A[8U]="scope_t";
# 1181 "parse.y"
static enum Cyc_Absyn_Scope Cyc_yyget_YY32(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp26A,_tmp26A,_tmp26A + 8U};
union Cyc_YYSTYPE*_tmp268=yy1;enum Cyc_Absyn_Scope _tmp269;if((((union Cyc_YYSTYPE*)_tmp268)->YY32).tag == 40){_LL1: _tmp269=(_tmp268->YY32).val;_LL2: {enum Cyc_Absyn_Scope yy=_tmp269;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1188
static union Cyc_YYSTYPE Cyc_YY32(enum Cyc_Absyn_Scope yy1){return({union Cyc_YYSTYPE _tmpFB4;(_tmpFB4.YY32).tag=40U,(_tmpFB4.YY32).val=yy1;_tmpFB4;});}static char _tmp26F[16U]="datatypefield_t";
# 1182 "parse.y"
static struct Cyc_Absyn_Datatypefield*Cyc_yyget_YY33(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp26F,_tmp26F,_tmp26F + 16U};
union Cyc_YYSTYPE*_tmp26D=yy1;struct Cyc_Absyn_Datatypefield*_tmp26E;if((((union Cyc_YYSTYPE*)_tmp26D)->YY33).tag == 41){_LL1: _tmp26E=(_tmp26D->YY33).val;_LL2: {struct Cyc_Absyn_Datatypefield*yy=_tmp26E;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1189
static union Cyc_YYSTYPE Cyc_YY33(struct Cyc_Absyn_Datatypefield*yy1){return({union Cyc_YYSTYPE _tmpFB5;(_tmpFB5.YY33).tag=41U,(_tmpFB5.YY33).val=yy1;_tmpFB5;});}static char _tmp274[27U]="list_t<datatypefield_t,`H>";
# 1183 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY34(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp274,_tmp274,_tmp274 + 27U};
union Cyc_YYSTYPE*_tmp272=yy1;struct Cyc_List_List*_tmp273;if((((union Cyc_YYSTYPE*)_tmp272)->YY34).tag == 42){_LL1: _tmp273=(_tmp272->YY34).val;_LL2: {struct Cyc_List_List*yy=_tmp273;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1190
static union Cyc_YYSTYPE Cyc_YY34(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFB6;(_tmpFB6.YY34).tag=42U,(_tmpFB6.YY34).val=yy1;_tmpFB6;});}static char _tmp279[41U]="$(tqual_t,type_specifier_t,attributes_t)";
# 1184 "parse.y"
static struct _tuple26 Cyc_yyget_YY35(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp279,_tmp279,_tmp279 + 41U};
union Cyc_YYSTYPE*_tmp277=yy1;struct _tuple26 _tmp278;if((((union Cyc_YYSTYPE*)_tmp277)->YY35).tag == 43){_LL1: _tmp278=(_tmp277->YY35).val;_LL2: {struct _tuple26 yy=_tmp278;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1191
static union Cyc_YYSTYPE Cyc_YY35(struct _tuple26 yy1){return({union Cyc_YYSTYPE _tmpFB7;(_tmpFB7.YY35).tag=43U,(_tmpFB7.YY35).val=yy1;_tmpFB7;});}static char _tmp27E[17U]="list_t<var_t,`H>";
# 1185 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY36(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp27E,_tmp27E,_tmp27E + 17U};
union Cyc_YYSTYPE*_tmp27C=yy1;struct Cyc_List_List*_tmp27D;if((((union Cyc_YYSTYPE*)_tmp27C)->YY36).tag == 44){_LL1: _tmp27D=(_tmp27C->YY36).val;_LL2: {struct Cyc_List_List*yy=_tmp27D;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1192
static union Cyc_YYSTYPE Cyc_YY36(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFB8;(_tmpFB8.YY36).tag=44U,(_tmpFB8.YY36).val=yy1;_tmpFB8;});}static char _tmp283[31U]="$(var_opt_t,tqual_t,type_t)@`H";
# 1186 "parse.y"
static struct _tuple9*Cyc_yyget_YY37(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp283,_tmp283,_tmp283 + 31U};
union Cyc_YYSTYPE*_tmp281=yy1;struct _tuple9*_tmp282;if((((union Cyc_YYSTYPE*)_tmp281)->YY37).tag == 45){_LL1: _tmp282=(_tmp281->YY37).val;_LL2: {struct _tuple9*yy=_tmp282;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1193
static union Cyc_YYSTYPE Cyc_YY37(struct _tuple9*yy1){return({union Cyc_YYSTYPE _tmpFB9;(_tmpFB9.YY37).tag=45U,(_tmpFB9.YY37).val=yy1;_tmpFB9;});}static char _tmp288[42U]="list_t<$(var_opt_t,tqual_t,type_t)@`H,`H>";
# 1187 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY38(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp288,_tmp288,_tmp288 + 42U};
union Cyc_YYSTYPE*_tmp286=yy1;struct Cyc_List_List*_tmp287;if((((union Cyc_YYSTYPE*)_tmp286)->YY38).tag == 46){_LL1: _tmp287=(_tmp286->YY38).val;_LL2: {struct Cyc_List_List*yy=_tmp287;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1194
static union Cyc_YYSTYPE Cyc_YY38(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFBA;(_tmpFBA.YY38).tag=46U,(_tmpFBA.YY38).val=yy1;_tmpFBA;});}static char _tmp28D[115U]="$(list_t<$(var_opt_t,tqual_t,type_t)@`H,`H>, bool,vararg_info_t *`H,type_opt_t, list_t<$(type_t,type_t)@`H,`H>)@`H";
# 1188 "parse.y"
static struct _tuple27*Cyc_yyget_YY39(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp28D,_tmp28D,_tmp28D + 115U};
union Cyc_YYSTYPE*_tmp28B=yy1;struct _tuple27*_tmp28C;if((((union Cyc_YYSTYPE*)_tmp28B)->YY39).tag == 47){_LL1: _tmp28C=(_tmp28B->YY39).val;_LL2: {struct _tuple27*yy=_tmp28C;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1195
static union Cyc_YYSTYPE Cyc_YY39(struct _tuple27*yy1){return({union Cyc_YYSTYPE _tmpFBB;(_tmpFBB.YY39).tag=47U,(_tmpFBB.YY39).val=yy1;_tmpFBB;});}static char _tmp292[8U]="types_t";
# 1189 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY40(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp292,_tmp292,_tmp292 + 8U};
union Cyc_YYSTYPE*_tmp290=yy1;struct Cyc_List_List*_tmp291;if((((union Cyc_YYSTYPE*)_tmp290)->YY40).tag == 48){_LL1: _tmp291=(_tmp290->YY40).val;_LL2: {struct Cyc_List_List*yy=_tmp291;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1196
static union Cyc_YYSTYPE Cyc_YY40(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFBC;(_tmpFBC.YY40).tag=48U,(_tmpFBC.YY40).val=yy1;_tmpFBC;});}static char _tmp297[24U]="list_t<designator_t,`H>";
# 1191 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY41(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp297,_tmp297,_tmp297 + 24U};
union Cyc_YYSTYPE*_tmp295=yy1;struct Cyc_List_List*_tmp296;if((((union Cyc_YYSTYPE*)_tmp295)->YY41).tag == 49){_LL1: _tmp296=(_tmp295->YY41).val;_LL2: {struct Cyc_List_List*yy=_tmp296;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1198
static union Cyc_YYSTYPE Cyc_YY41(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFBD;(_tmpFBD.YY41).tag=49U,(_tmpFBD.YY41).val=yy1;_tmpFBD;});}static char _tmp29C[13U]="designator_t";
# 1192 "parse.y"
static void*Cyc_yyget_YY42(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp29C,_tmp29C,_tmp29C + 13U};
union Cyc_YYSTYPE*_tmp29A=yy1;void*_tmp29B;if((((union Cyc_YYSTYPE*)_tmp29A)->YY42).tag == 50){_LL1: _tmp29B=(_tmp29A->YY42).val;_LL2: {void*yy=_tmp29B;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1199
static union Cyc_YYSTYPE Cyc_YY42(void*yy1){return({union Cyc_YYSTYPE _tmpFBE;(_tmpFBE.YY42).tag=50U,(_tmpFBE.YY42).val=yy1;_tmpFBE;});}static char _tmp2A1[7U]="kind_t";
# 1193 "parse.y"
static struct Cyc_Absyn_Kind*Cyc_yyget_YY43(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2A1,_tmp2A1,_tmp2A1 + 7U};
union Cyc_YYSTYPE*_tmp29F=yy1;struct Cyc_Absyn_Kind*_tmp2A0;if((((union Cyc_YYSTYPE*)_tmp29F)->YY43).tag == 51){_LL1: _tmp2A0=(_tmp29F->YY43).val;_LL2: {struct Cyc_Absyn_Kind*yy=_tmp2A0;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1200
static union Cyc_YYSTYPE Cyc_YY43(struct Cyc_Absyn_Kind*yy1){return({union Cyc_YYSTYPE _tmpFBF;(_tmpFBF.YY43).tag=51U,(_tmpFBF.YY43).val=yy1;_tmpFBF;});}static char _tmp2A6[7U]="type_t";
# 1194 "parse.y"
static void*Cyc_yyget_YY44(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2A6,_tmp2A6,_tmp2A6 + 7U};
union Cyc_YYSTYPE*_tmp2A4=yy1;void*_tmp2A5;if((((union Cyc_YYSTYPE*)_tmp2A4)->YY44).tag == 52){_LL1: _tmp2A5=(_tmp2A4->YY44).val;_LL2: {void*yy=_tmp2A5;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1201
static union Cyc_YYSTYPE Cyc_YY44(void*yy1){return({union Cyc_YYSTYPE _tmpFC0;(_tmpFC0.YY44).tag=52U,(_tmpFC0.YY44).val=yy1;_tmpFC0;});}static char _tmp2AB[23U]="list_t<attribute_t,`H>";
# 1195 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY45(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2AB,_tmp2AB,_tmp2AB + 23U};
union Cyc_YYSTYPE*_tmp2A9=yy1;struct Cyc_List_List*_tmp2AA;if((((union Cyc_YYSTYPE*)_tmp2A9)->YY45).tag == 53){_LL1: _tmp2AA=(_tmp2A9->YY45).val;_LL2: {struct Cyc_List_List*yy=_tmp2AA;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1202
static union Cyc_YYSTYPE Cyc_YY45(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFC1;(_tmpFC1.YY45).tag=53U,(_tmpFC1.YY45).val=yy1;_tmpFC1;});}static char _tmp2B0[12U]="attribute_t";
# 1196 "parse.y"
static void*Cyc_yyget_YY46(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2B0,_tmp2B0,_tmp2B0 + 12U};
union Cyc_YYSTYPE*_tmp2AE=yy1;void*_tmp2AF;if((((union Cyc_YYSTYPE*)_tmp2AE)->YY46).tag == 54){_LL1: _tmp2AF=(_tmp2AE->YY46).val;_LL2: {void*yy=_tmp2AF;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1203
static union Cyc_YYSTYPE Cyc_YY46(void*yy1){return({union Cyc_YYSTYPE _tmpFC2;(_tmpFC2.YY46).tag=54U,(_tmpFC2.YY46).val=yy1;_tmpFC2;});}static char _tmp2B5[12U]="enumfield_t";
# 1197 "parse.y"
static struct Cyc_Absyn_Enumfield*Cyc_yyget_YY47(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2B5,_tmp2B5,_tmp2B5 + 12U};
union Cyc_YYSTYPE*_tmp2B3=yy1;struct Cyc_Absyn_Enumfield*_tmp2B4;if((((union Cyc_YYSTYPE*)_tmp2B3)->YY47).tag == 55){_LL1: _tmp2B4=(_tmp2B3->YY47).val;_LL2: {struct Cyc_Absyn_Enumfield*yy=_tmp2B4;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1204
static union Cyc_YYSTYPE Cyc_YY47(struct Cyc_Absyn_Enumfield*yy1){return({union Cyc_YYSTYPE _tmpFC3;(_tmpFC3.YY47).tag=55U,(_tmpFC3.YY47).val=yy1;_tmpFC3;});}static char _tmp2BA[23U]="list_t<enumfield_t,`H>";
# 1198 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY48(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2BA,_tmp2BA,_tmp2BA + 23U};
union Cyc_YYSTYPE*_tmp2B8=yy1;struct Cyc_List_List*_tmp2B9;if((((union Cyc_YYSTYPE*)_tmp2B8)->YY48).tag == 56){_LL1: _tmp2B9=(_tmp2B8->YY48).val;_LL2: {struct Cyc_List_List*yy=_tmp2B9;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1205
static union Cyc_YYSTYPE Cyc_YY48(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFC4;(_tmpFC4.YY48).tag=56U,(_tmpFC4.YY48).val=yy1;_tmpFC4;});}static char _tmp2BF[11U]="type_opt_t";
# 1199 "parse.y"
static void*Cyc_yyget_YY49(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2BF,_tmp2BF,_tmp2BF + 11U};
union Cyc_YYSTYPE*_tmp2BD=yy1;void*_tmp2BE;if((((union Cyc_YYSTYPE*)_tmp2BD)->YY49).tag == 57){_LL1: _tmp2BE=(_tmp2BD->YY49).val;_LL2: {void*yy=_tmp2BE;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1206
static union Cyc_YYSTYPE Cyc_YY49(void*yy1){return({union Cyc_YYSTYPE _tmpFC5;(_tmpFC5.YY49).tag=57U,(_tmpFC5.YY49).val=yy1;_tmpFC5;});}static char _tmp2C4[31U]="list_t<$(type_t,type_t)@`H,`H>";
# 1200 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY50(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2C4,_tmp2C4,_tmp2C4 + 31U};
union Cyc_YYSTYPE*_tmp2C2=yy1;struct Cyc_List_List*_tmp2C3;if((((union Cyc_YYSTYPE*)_tmp2C2)->YY50).tag == 58){_LL1: _tmp2C3=(_tmp2C2->YY50).val;_LL2: {struct Cyc_List_List*yy=_tmp2C3;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1207
static union Cyc_YYSTYPE Cyc_YY50(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFC6;(_tmpFC6.YY50).tag=58U,(_tmpFC6.YY50).val=yy1;_tmpFC6;});}static char _tmp2C9[11U]="booltype_t";
# 1201 "parse.y"
static void*Cyc_yyget_YY51(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2C9,_tmp2C9,_tmp2C9 + 11U};
union Cyc_YYSTYPE*_tmp2C7=yy1;void*_tmp2C8;if((((union Cyc_YYSTYPE*)_tmp2C7)->YY51).tag == 59){_LL1: _tmp2C8=(_tmp2C7->YY51).val;_LL2: {void*yy=_tmp2C8;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1208
static union Cyc_YYSTYPE Cyc_YY51(void*yy1){return({union Cyc_YYSTYPE _tmpFC7;(_tmpFC7.YY51).tag=59U,(_tmpFC7.YY51).val=yy1;_tmpFC7;});}static char _tmp2CE[45U]="list_t<$(Position::seg_t,qvar_t,bool)@`H,`H>";
# 1202 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY52(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2CE,_tmp2CE,_tmp2CE + 45U};
union Cyc_YYSTYPE*_tmp2CC=yy1;struct Cyc_List_List*_tmp2CD;if((((union Cyc_YYSTYPE*)_tmp2CC)->YY52).tag == 60){_LL1: _tmp2CD=(_tmp2CC->YY52).val;_LL2: {struct Cyc_List_List*yy=_tmp2CD;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1209
static union Cyc_YYSTYPE Cyc_YY52(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFC8;(_tmpFC8.YY52).tag=60U,(_tmpFC8.YY52).val=yy1;_tmpFC8;});}static char _tmp2D3[58U]="$(list_t<$(Position::seg_t,qvar_t,bool)@`H,`H>, seg_t)@`H";
# 1203 "parse.y"
static struct _tuple28*Cyc_yyget_YY53(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2D3,_tmp2D3,_tmp2D3 + 58U};
union Cyc_YYSTYPE*_tmp2D1=yy1;struct _tuple28*_tmp2D2;if((((union Cyc_YYSTYPE*)_tmp2D1)->YY53).tag == 61){_LL1: _tmp2D2=(_tmp2D1->YY53).val;_LL2: {struct _tuple28*yy=_tmp2D2;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1210
static union Cyc_YYSTYPE Cyc_YY53(struct _tuple28*yy1){return({union Cyc_YYSTYPE _tmpFC9;(_tmpFC9.YY53).tag=61U,(_tmpFC9.YY53).val=yy1;_tmpFC9;});}static char _tmp2D8[18U]="list_t<qvar_t,`H>";
# 1204 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY54(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2D8,_tmp2D8,_tmp2D8 + 18U};
union Cyc_YYSTYPE*_tmp2D6=yy1;struct Cyc_List_List*_tmp2D7;if((((union Cyc_YYSTYPE*)_tmp2D6)->YY54).tag == 62){_LL1: _tmp2D7=(_tmp2D6->YY54).val;_LL2: {struct Cyc_List_List*yy=_tmp2D7;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1211
static union Cyc_YYSTYPE Cyc_YY54(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFCA;(_tmpFCA.YY54).tag=62U,(_tmpFCA.YY54).val=yy1;_tmpFCA;});}static char _tmp2DD[20U]="pointer_qual_t<`yy>";
# 1205 "parse.y"
static void*Cyc_yyget_YY55(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2DD,_tmp2DD,_tmp2DD + 20U};
union Cyc_YYSTYPE*_tmp2DB=yy1;void*_tmp2DC;if((((union Cyc_YYSTYPE*)_tmp2DB)->YY55).tag == 63){_LL1: _tmp2DC=(_tmp2DB->YY55).val;_LL2: {void*yy=_tmp2DC;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1212
static union Cyc_YYSTYPE Cyc_YY55(void*yy1){return({union Cyc_YYSTYPE _tmpFCB;(_tmpFCB.YY55).tag=63U,(_tmpFCB.YY55).val=yy1;_tmpFCB;});}static char _tmp2E2[21U]="pointer_quals_t<`yy>";
# 1206 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY56(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2E2,_tmp2E2,_tmp2E2 + 21U};
union Cyc_YYSTYPE*_tmp2E0=yy1;struct Cyc_List_List*_tmp2E1;if((((union Cyc_YYSTYPE*)_tmp2E0)->YY56).tag == 64){_LL1: _tmp2E1=(_tmp2E0->YY56).val;_LL2: {struct Cyc_List_List*yy=_tmp2E1;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1213
static union Cyc_YYSTYPE Cyc_YY56(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFCC;(_tmpFCC.YY56).tag=64U,(_tmpFCC.YY56).val=yy1;_tmpFCC;});}static char _tmp2E7[10U]="exp_opt_t";
# 1207 "parse.y"
static struct Cyc_Absyn_Exp*Cyc_yyget_YY57(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2E7,_tmp2E7,_tmp2E7 + 10U};
union Cyc_YYSTYPE*_tmp2E5=yy1;struct Cyc_Absyn_Exp*_tmp2E6;if((((union Cyc_YYSTYPE*)_tmp2E5)->YY57).tag == 65){_LL1: _tmp2E6=(_tmp2E5->YY57).val;_LL2: {struct Cyc_Absyn_Exp*yy=_tmp2E6;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1214
static union Cyc_YYSTYPE Cyc_YY57(struct Cyc_Absyn_Exp*yy1){return({union Cyc_YYSTYPE _tmpFCD;(_tmpFCD.YY57).tag=65U,(_tmpFCD.YY57).val=yy1;_tmpFCD;});}static char _tmp2EC[10U]="raw_exp_t";
# 1208 "parse.y"
static void*Cyc_yyget_YY58(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2EC,_tmp2EC,_tmp2EC + 10U};
union Cyc_YYSTYPE*_tmp2EA=yy1;void*_tmp2EB;if((((union Cyc_YYSTYPE*)_tmp2EA)->YY58).tag == 66){_LL1: _tmp2EB=(_tmp2EA->YY58).val;_LL2: {void*yy=_tmp2EB;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1215
static union Cyc_YYSTYPE Cyc_YY58(void*yy1){return({union Cyc_YYSTYPE _tmpFCE;(_tmpFCE.YY58).tag=66U,(_tmpFCE.YY58).val=yy1;_tmpFCE;});}static char _tmp2F1[112U]="$(list_t<$(string_t<`H>, exp_t)@`H, `H>, list_t<$(string_t<`H>, exp_t)@`H, `H>, list_t<string_t<`H>@`H, `H>)@`H";
# 1210 "parse.y"
static struct _tuple29*Cyc_yyget_YY59(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2F1,_tmp2F1,_tmp2F1 + 112U};
union Cyc_YYSTYPE*_tmp2EF=yy1;struct _tuple29*_tmp2F0;if((((union Cyc_YYSTYPE*)_tmp2EF)->YY59).tag == 67){_LL1: _tmp2F0=(_tmp2EF->YY59).val;_LL2: {struct _tuple29*yy=_tmp2F0;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1217
static union Cyc_YYSTYPE Cyc_YY59(struct _tuple29*yy1){return({union Cyc_YYSTYPE _tmpFCF;(_tmpFCF.YY59).tag=67U,(_tmpFCF.YY59).val=yy1;_tmpFCF;});}static char _tmp2F6[73U]="$(list_t<$(string_t<`H>, exp_t)@`H, `H>, list_t<string_t<`H>@`H, `H>)@`H";
# 1211 "parse.y"
static struct _tuple8*Cyc_yyget_YY60(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2F6,_tmp2F6,_tmp2F6 + 73U};
union Cyc_YYSTYPE*_tmp2F4=yy1;struct _tuple8*_tmp2F5;if((((union Cyc_YYSTYPE*)_tmp2F4)->YY60).tag == 68){_LL1: _tmp2F5=(_tmp2F4->YY60).val;_LL2: {struct _tuple8*yy=_tmp2F5;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1218
static union Cyc_YYSTYPE Cyc_YY60(struct _tuple8*yy1){return({union Cyc_YYSTYPE _tmpFD0;(_tmpFD0.YY60).tag=68U,(_tmpFD0.YY60).val=yy1;_tmpFD0;});}static char _tmp2FB[28U]="list_t<string_t<`H>@`H, `H>";
# 1212 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY61(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp2FB,_tmp2FB,_tmp2FB + 28U};
union Cyc_YYSTYPE*_tmp2F9=yy1;struct Cyc_List_List*_tmp2FA;if((((union Cyc_YYSTYPE*)_tmp2F9)->YY61).tag == 69){_LL1: _tmp2FA=(_tmp2F9->YY61).val;_LL2: {struct Cyc_List_List*yy=_tmp2FA;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1219
static union Cyc_YYSTYPE Cyc_YY61(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFD1;(_tmpFD1.YY61).tag=69U,(_tmpFD1.YY61).val=yy1;_tmpFD1;});}static char _tmp300[38U]="list_t<$(string_t<`H>, exp_t)@`H, `H>";
# 1213 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY62(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp300,_tmp300,_tmp300 + 38U};
union Cyc_YYSTYPE*_tmp2FE=yy1;struct Cyc_List_List*_tmp2FF;if((((union Cyc_YYSTYPE*)_tmp2FE)->YY62).tag == 70){_LL1: _tmp2FF=(_tmp2FE->YY62).val;_LL2: {struct Cyc_List_List*yy=_tmp2FF;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1220
static union Cyc_YYSTYPE Cyc_YY62(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFD2;(_tmpFD2.YY62).tag=70U,(_tmpFD2.YY62).val=yy1;_tmpFD2;});}static char _tmp305[26U]="$(string_t<`H>, exp_t)@`H";
# 1214 "parse.y"
static struct _tuple30*Cyc_yyget_YY63(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp305,_tmp305,_tmp305 + 26U};
union Cyc_YYSTYPE*_tmp303=yy1;struct _tuple30*_tmp304;if((((union Cyc_YYSTYPE*)_tmp303)->YY63).tag == 71){_LL1: _tmp304=(_tmp303->YY63).val;_LL2: {struct _tuple30*yy=_tmp304;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1221
static union Cyc_YYSTYPE Cyc_YY63(struct _tuple30*yy1){return({union Cyc_YYSTYPE _tmpFD3;(_tmpFD3.YY63).tag=71U,(_tmpFD3.YY63).val=yy1;_tmpFD3;});}static char _tmp30A[9U]="throws_t";
# 1216 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY64(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp30A,_tmp30A,_tmp30A + 9U};
union Cyc_YYSTYPE*_tmp308=yy1;struct Cyc_List_List*_tmp309;if((((union Cyc_YYSTYPE*)_tmp308)->YY64).tag == 72){_LL1: _tmp309=(_tmp308->YY64).val;_LL2: {struct Cyc_List_List*yy=_tmp309;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1223
static union Cyc_YYSTYPE Cyc_YY64(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFD4;(_tmpFD4.YY64).tag=72U,(_tmpFD4.YY64).val=yy1;_tmpFD4;});}static char _tmp30F[12U]="reentrant_t";
# 1217 "parse.y"
static int Cyc_yyget_YY65(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp30F,_tmp30F,_tmp30F + 12U};
union Cyc_YYSTYPE*_tmp30D=yy1;int _tmp30E;if((((union Cyc_YYSTYPE*)_tmp30D)->YY65).tag == 73){_LL1: _tmp30E=(_tmp30D->YY65).val;_LL2: {int yy=_tmp30E;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1224
static union Cyc_YYSTYPE Cyc_YY65(int yy1){return({union Cyc_YYSTYPE _tmpFD5;(_tmpFD5.YY65).tag=73U,(_tmpFD5.YY65).val=yy1;_tmpFD5;});}static char _tmp314[18U]="list_t<type_t,`H>";
# 1218 "parse.y"
static struct Cyc_List_List*Cyc_yyget_YY66(union Cyc_YYSTYPE*yy1){
static struct _fat_ptr s={_tmp314,_tmp314,_tmp314 + 18U};
union Cyc_YYSTYPE*_tmp312=yy1;struct Cyc_List_List*_tmp313;if((((union Cyc_YYSTYPE*)_tmp312)->YY66).tag == 74){_LL1: _tmp313=(_tmp312->YY66).val;_LL2: {struct Cyc_List_List*yy=_tmp313;
return yy;}}else{_LL3: _LL4:
 if(Cyc_beforedie != 0)({((void(*)())_check_null(Cyc_beforedie))();});({Cyc_yythrowfail(s);});}_LL0:;}
# 1225
static union Cyc_YYSTYPE Cyc_YY66(struct Cyc_List_List*yy1){return({union Cyc_YYSTYPE _tmpFD6;(_tmpFD6.YY66).tag=74U,(_tmpFD6.YY66).val=yy1;_tmpFD6;});}struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int last_line;int last_column;};
# 1241
struct Cyc_Yyltype Cyc_yynewloc(){
return({struct Cyc_Yyltype _tmpFD7;_tmpFD7.timestamp=0,_tmpFD7.first_line=0,_tmpFD7.first_column=0,_tmpFD7.last_line=0,_tmpFD7.last_column=0;_tmpFD7;});}
# 1241
struct Cyc_Yyltype Cyc_yylloc={0,0,0,0,0};
# 1255 "parse.y"
static short Cyc_yytranslate[386U]={0,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,157,2,2,143,155,152,2,139,140,135,149,134,153,145,154,2,2,2,2,2,2,2,2,2,2,144,131,137,136,138,148,141,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,146,2,147,151,142,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,132,150,133,156,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,128,129,130};static char _tmp318[2U]="$";static char _tmp319[6U]="error";static char _tmp31A[12U]="$undefined.";static char _tmp31B[5U]="AUTO";static char _tmp31C[9U]="REGISTER";static char _tmp31D[7U]="STATIC";static char _tmp31E[7U]="EXTERN";static char _tmp31F[8U]="TYPEDEF";static char _tmp320[5U]="VOID";static char _tmp321[5U]="CHAR";static char _tmp322[6U]="SHORT";static char _tmp323[4U]="INT";static char _tmp324[5U]="LONG";static char _tmp325[6U]="FLOAT";static char _tmp326[7U]="DOUBLE";static char _tmp327[7U]="SIGNED";static char _tmp328[9U]="UNSIGNED";static char _tmp329[6U]="CONST";static char _tmp32A[9U]="VOLATILE";static char _tmp32B[9U]="RESTRICT";static char _tmp32C[7U]="STRUCT";static char _tmp32D[6U]="UNION";static char _tmp32E[5U]="CASE";static char _tmp32F[8U]="DEFAULT";static char _tmp330[7U]="INLINE";static char _tmp331[7U]="SIZEOF";static char _tmp332[9U]="OFFSETOF";static char _tmp333[3U]="IF";static char _tmp334[5U]="ELSE";static char _tmp335[7U]="SWITCH";static char _tmp336[6U]="WHILE";static char _tmp337[3U]="DO";static char _tmp338[4U]="FOR";static char _tmp339[5U]="GOTO";static char _tmp33A[9U]="CONTINUE";static char _tmp33B[6U]="BREAK";static char _tmp33C[7U]="RETURN";static char _tmp33D[5U]="ENUM";static char _tmp33E[7U]="TYPEOF";static char _tmp33F[16U]="BUILTIN_VA_LIST";static char _tmp340[10U]="EXTENSION";static char _tmp341[8U]="NULL_kw";static char _tmp342[4U]="LET";static char _tmp343[6U]="THROW";static char _tmp344[4U]="TRY";static char _tmp345[6U]="CATCH";static char _tmp346[7U]="EXPORT";static char _tmp347[9U]="OVERRIDE";static char _tmp348[5U]="HIDE";static char _tmp349[4U]="NEW";static char _tmp34A[9U]="ABSTRACT";static char _tmp34B[9U]="FALLTHRU";static char _tmp34C[6U]="USING";static char _tmp34D[10U]="NAMESPACE";static char _tmp34E[9U]="DATATYPE";static char _tmp34F[6U]="SPAWN";static char _tmp350[7U]="MALLOC";static char _tmp351[8U]="RMALLOC";static char _tmp352[15U]="RMALLOC_INLINE";static char _tmp353[7U]="CALLOC";static char _tmp354[8U]="RCALLOC";static char _tmp355[5U]="SWAP";static char _tmp356[9U]="REGION_T";static char _tmp357[6U]="TAG_T";static char _tmp358[7U]="REGION";static char _tmp359[5U]="RNEW";static char _tmp35A[8U]="REGIONS";static char _tmp35B[7U]="PORTON";static char _tmp35C[8U]="PORTOFF";static char _tmp35D[7U]="PRAGMA";static char _tmp35E[10U]="TEMPESTON";static char _tmp35F[11U]="TEMPESTOFF";static char _tmp360[8U]="NUMELTS";static char _tmp361[8U]="VALUEOF";static char _tmp362[10U]="VALUEOF_T";static char _tmp363[9U]="TAGCHECK";static char _tmp364[13U]="NUMELTS_QUAL";static char _tmp365[10U]="THIN_QUAL";static char _tmp366[9U]="FAT_QUAL";static char _tmp367[13U]="NOTNULL_QUAL";static char _tmp368[14U]="NULLABLE_QUAL";static char _tmp369[14U]="REQUIRES_QUAL";static char _tmp36A[13U]="ENSURES_QUAL";static char _tmp36B[13U]="IEFFECT_QUAL";static char _tmp36C[13U]="OEFFECT_QUAL";static char _tmp36D[12U]="THROWS_QUAL";static char _tmp36E[13U]="NOTHROW_QUAL";static char _tmp36F[15U]="THROWSANY_QUAL";static char _tmp370[15U]="REENTRANT_QUAL";static char _tmp371[12U]="REGION_QUAL";static char _tmp372[16U]="NOZEROTERM_QUAL";static char _tmp373[14U]="ZEROTERM_QUAL";static char _tmp374[12U]="TAGGED_QUAL";static char _tmp375[16U]="EXTENSIBLE_QUAL";static char _tmp376[7U]="PTR_OP";static char _tmp377[7U]="INC_OP";static char _tmp378[7U]="DEC_OP";static char _tmp379[8U]="LEFT_OP";static char _tmp37A[9U]="RIGHT_OP";static char _tmp37B[6U]="LE_OP";static char _tmp37C[6U]="GE_OP";static char _tmp37D[6U]="EQ_OP";static char _tmp37E[6U]="NE_OP";static char _tmp37F[7U]="AND_OP";static char _tmp380[6U]="OR_OP";static char _tmp381[11U]="MUL_ASSIGN";static char _tmp382[11U]="DIV_ASSIGN";static char _tmp383[11U]="MOD_ASSIGN";static char _tmp384[11U]="ADD_ASSIGN";static char _tmp385[11U]="SUB_ASSIGN";static char _tmp386[12U]="LEFT_ASSIGN";static char _tmp387[13U]="RIGHT_ASSIGN";static char _tmp388[11U]="AND_ASSIGN";static char _tmp389[11U]="XOR_ASSIGN";static char _tmp38A[10U]="OR_ASSIGN";static char _tmp38B[9U]="ELLIPSIS";static char _tmp38C[11U]="LEFT_RIGHT";static char _tmp38D[12U]="COLON_COLON";static char _tmp38E[11U]="IDENTIFIER";static char _tmp38F[17U]="INTEGER_CONSTANT";static char _tmp390[7U]="STRING";static char _tmp391[8U]="WSTRING";static char _tmp392[19U]="CHARACTER_CONSTANT";static char _tmp393[20U]="WCHARACTER_CONSTANT";static char _tmp394[18U]="FLOATING_CONSTANT";static char _tmp395[9U]="TYPE_VAR";static char _tmp396[13U]="TYPEDEF_NAME";static char _tmp397[16U]="QUAL_IDENTIFIER";static char _tmp398[18U]="QUAL_TYPEDEF_NAME";static char _tmp399[10U]="ATTRIBUTE";static char _tmp39A[8U]="ASM_TOK";static char _tmp39B[4U]="';'";static char _tmp39C[4U]="'{'";static char _tmp39D[4U]="'}'";static char _tmp39E[4U]="','";static char _tmp39F[4U]="'*'";static char _tmp3A0[4U]="'='";static char _tmp3A1[4U]="'<'";static char _tmp3A2[4U]="'>'";static char _tmp3A3[4U]="'('";static char _tmp3A4[4U]="')'";static char _tmp3A5[4U]="'@'";static char _tmp3A6[4U]="'_'";static char _tmp3A7[4U]="'$'";static char _tmp3A8[4U]="':'";static char _tmp3A9[4U]="'.'";static char _tmp3AA[4U]="'['";static char _tmp3AB[4U]="']'";static char _tmp3AC[4U]="'?'";static char _tmp3AD[4U]="'+'";static char _tmp3AE[4U]="'|'";static char _tmp3AF[4U]="'^'";static char _tmp3B0[4U]="'&'";static char _tmp3B1[4U]="'-'";static char _tmp3B2[4U]="'/'";static char _tmp3B3[4U]="'%'";static char _tmp3B4[4U]="'~'";static char _tmp3B5[4U]="'!'";static char _tmp3B6[5U]="prog";static char _tmp3B7[17U]="translation_unit";static char _tmp3B8[18U]="tempest_on_action";static char _tmp3B9[19U]="tempest_off_action";static char _tmp3BA[16U]="extern_c_action";static char _tmp3BB[13U]="end_extern_c";static char _tmp3BC[14U]="hide_list_opt";static char _tmp3BD[17U]="hide_list_values";static char _tmp3BE[16U]="export_list_opt";static char _tmp3BF[12U]="export_list";static char _tmp3C0[19U]="export_list_values";static char _tmp3C1[13U]="override_opt";static char _tmp3C2[21U]="external_declaration";static char _tmp3C3[15U]="optional_comma";static char _tmp3C4[20U]="function_definition";static char _tmp3C5[21U]="function_definition2";static char _tmp3C6[13U]="using_action";static char _tmp3C7[15U]="unusing_action";static char _tmp3C8[17U]="namespace_action";static char _tmp3C9[19U]="unnamespace_action";static char _tmp3CA[12U]="declaration";static char _tmp3CB[17U]="declaration_list";static char _tmp3CC[23U]="declaration_specifiers";static char _tmp3CD[24U]="storage_class_specifier";static char _tmp3CE[15U]="attributes_opt";static char _tmp3CF[11U]="attributes";static char _tmp3D0[15U]="attribute_list";static char _tmp3D1[10U]="attribute";static char _tmp3D2[15U]="type_specifier";static char _tmp3D3[25U]="type_specifier_notypedef";static char _tmp3D4[5U]="kind";static char _tmp3D5[15U]="type_qualifier";static char _tmp3D6[15U]="enum_specifier";static char _tmp3D7[11U]="enum_field";static char _tmp3D8[22U]="enum_declaration_list";static char _tmp3D9[26U]="struct_or_union_specifier";static char _tmp3DA[16U]="type_params_opt";static char _tmp3DB[16U]="struct_or_union";static char _tmp3DC[24U]="struct_declaration_list";static char _tmp3DD[25U]="struct_declaration_list0";static char _tmp3DE[21U]="init_declarator_list";static char _tmp3DF[22U]="init_declarator_list0";static char _tmp3E0[16U]="init_declarator";static char _tmp3E1[19U]="struct_declaration";static char _tmp3E2[25U]="specifier_qualifier_list";static char _tmp3E3[35U]="notypedef_specifier_qualifier_list";static char _tmp3E4[23U]="struct_declarator_list";static char _tmp3E5[24U]="struct_declarator_list0";static char _tmp3E6[18U]="struct_declarator";static char _tmp3E7[20U]="requires_clause_opt";static char _tmp3E8[9U]="eff_list";static char _tmp3E9[19U]="ieffect_clause_opt";static char _tmp3EA[19U]="oeffect_clause_opt";static char _tmp3EB[18U]="throws_clause_opt";static char _tmp3EC[21U]="reentrant_clause_opt";static char _tmp3ED[19U]="ensures_clause_opt";static char _tmp3EE[19U]="datatype_specifier";static char _tmp3EF[14U]="qual_datatype";static char _tmp3F0[19U]="datatypefield_list";static char _tmp3F1[20U]="datatypefield_scope";static char _tmp3F2[14U]="datatypefield";static char _tmp3F3[11U]="declarator";static char _tmp3F4[23U]="declarator_withtypedef";static char _tmp3F5[18U]="direct_declarator";static char _tmp3F6[30U]="direct_declarator_withtypedef";static char _tmp3F7[8U]="pointer";static char _tmp3F8[12U]="one_pointer";static char _tmp3F9[14U]="pointer_quals";static char _tmp3FA[13U]="pointer_qual";static char _tmp3FB[23U]="pointer_null_and_bound";static char _tmp3FC[14U]="pointer_bound";static char _tmp3FD[18U]="zeroterm_qual_opt";static char _tmp3FE[8U]="eff_set";static char _tmp3FF[8U]="eff_opt";static char _tmp400[11U]="tqual_list";static char _tmp401[20U]="parameter_type_list";static char _tmp402[9U]="type_var";static char _tmp403[16U]="optional_effect";static char _tmp404[19U]="optional_rgn_order";static char _tmp405[10U]="rgn_order";static char _tmp406[16U]="optional_inject";static char _tmp407[11U]="effect_set";static char _tmp408[14U]="atomic_effect";static char _tmp409[11U]="region_set";static char _tmp40A[15U]="parameter_list";static char _tmp40B[22U]="parameter_declaration";static char _tmp40C[16U]="identifier_list";static char _tmp40D[17U]="identifier_list0";static char _tmp40E[12U]="initializer";static char _tmp40F[18U]="array_initializer";static char _tmp410[17U]="initializer_list";static char _tmp411[12U]="designation";static char _tmp412[16U]="designator_list";static char _tmp413[11U]="designator";static char _tmp414[10U]="type_name";static char _tmp415[14U]="any_type_name";static char _tmp416[15U]="type_name_list";static char _tmp417[20U]="abstract_declarator";static char _tmp418[27U]="direct_abstract_declarator";static char _tmp419[10U]="statement";static char _tmp41A[18U]="labeled_statement";static char _tmp41B[21U]="expression_statement";static char _tmp41C[19U]="compound_statement";static char _tmp41D[16U]="block_item_list";static char _tmp41E[20U]="selection_statement";static char _tmp41F[15U]="switch_clauses";static char _tmp420[20U]="iteration_statement";static char _tmp421[15U]="jump_statement";static char _tmp422[12U]="exp_pattern";static char _tmp423[20U]="conditional_pattern";static char _tmp424[19U]="logical_or_pattern";static char _tmp425[20U]="logical_and_pattern";static char _tmp426[21U]="inclusive_or_pattern";static char _tmp427[21U]="exclusive_or_pattern";static char _tmp428[12U]="and_pattern";static char _tmp429[17U]="equality_pattern";static char _tmp42A[19U]="relational_pattern";static char _tmp42B[14U]="shift_pattern";static char _tmp42C[17U]="additive_pattern";static char _tmp42D[23U]="multiplicative_pattern";static char _tmp42E[13U]="cast_pattern";static char _tmp42F[14U]="unary_pattern";static char _tmp430[16U]="postfix_pattern";static char _tmp431[16U]="primary_pattern";static char _tmp432[8U]="pattern";static char _tmp433[19U]="tuple_pattern_list";static char _tmp434[20U]="tuple_pattern_list0";static char _tmp435[14U]="field_pattern";static char _tmp436[19U]="field_pattern_list";static char _tmp437[20U]="field_pattern_list0";static char _tmp438[11U]="expression";static char _tmp439[22U]="assignment_expression";static char _tmp43A[20U]="assignment_operator";static char _tmp43B[23U]="conditional_expression";static char _tmp43C[20U]="constant_expression";static char _tmp43D[22U]="logical_or_expression";static char _tmp43E[23U]="logical_and_expression";static char _tmp43F[24U]="inclusive_or_expression";static char _tmp440[24U]="exclusive_or_expression";static char _tmp441[15U]="and_expression";static char _tmp442[20U]="equality_expression";static char _tmp443[22U]="relational_expression";static char _tmp444[17U]="shift_expression";static char _tmp445[20U]="additive_expression";static char _tmp446[26U]="multiplicative_expression";static char _tmp447[16U]="cast_expression";static char _tmp448[17U]="unary_expression";static char _tmp449[9U]="asm_expr";static char _tmp44A[13U]="volatile_opt";static char _tmp44B[12U]="asm_out_opt";static char _tmp44C[12U]="asm_outlist";static char _tmp44D[11U]="asm_in_opt";static char _tmp44E[11U]="asm_inlist";static char _tmp44F[11U]="asm_io_elt";static char _tmp450[16U]="asm_clobber_opt";static char _tmp451[17U]="asm_clobber_list";static char _tmp452[15U]="unary_operator";static char _tmp453[19U]="postfix_expression";static char _tmp454[17U]="field_expression";static char _tmp455[19U]="primary_expression";static char _tmp456[25U]="argument_expression_list";static char _tmp457[26U]="argument_expression_list0";static char _tmp458[9U]="constant";static char _tmp459[20U]="qual_opt_identifier";static char _tmp45A[17U]="qual_opt_typedef";static char _tmp45B[18U]="struct_union_name";static char _tmp45C[11U]="field_name";static char _tmp45D[12U]="right_angle";
# 1638 "parse.y"
static struct _fat_ptr Cyc_yytname[326U]={{_tmp318,_tmp318,_tmp318 + 2U},{_tmp319,_tmp319,_tmp319 + 6U},{_tmp31A,_tmp31A,_tmp31A + 12U},{_tmp31B,_tmp31B,_tmp31B + 5U},{_tmp31C,_tmp31C,_tmp31C + 9U},{_tmp31D,_tmp31D,_tmp31D + 7U},{_tmp31E,_tmp31E,_tmp31E + 7U},{_tmp31F,_tmp31F,_tmp31F + 8U},{_tmp320,_tmp320,_tmp320 + 5U},{_tmp321,_tmp321,_tmp321 + 5U},{_tmp322,_tmp322,_tmp322 + 6U},{_tmp323,_tmp323,_tmp323 + 4U},{_tmp324,_tmp324,_tmp324 + 5U},{_tmp325,_tmp325,_tmp325 + 6U},{_tmp326,_tmp326,_tmp326 + 7U},{_tmp327,_tmp327,_tmp327 + 7U},{_tmp328,_tmp328,_tmp328 + 9U},{_tmp329,_tmp329,_tmp329 + 6U},{_tmp32A,_tmp32A,_tmp32A + 9U},{_tmp32B,_tmp32B,_tmp32B + 9U},{_tmp32C,_tmp32C,_tmp32C + 7U},{_tmp32D,_tmp32D,_tmp32D + 6U},{_tmp32E,_tmp32E,_tmp32E + 5U},{_tmp32F,_tmp32F,_tmp32F + 8U},{_tmp330,_tmp330,_tmp330 + 7U},{_tmp331,_tmp331,_tmp331 + 7U},{_tmp332,_tmp332,_tmp332 + 9U},{_tmp333,_tmp333,_tmp333 + 3U},{_tmp334,_tmp334,_tmp334 + 5U},{_tmp335,_tmp335,_tmp335 + 7U},{_tmp336,_tmp336,_tmp336 + 6U},{_tmp337,_tmp337,_tmp337 + 3U},{_tmp338,_tmp338,_tmp338 + 4U},{_tmp339,_tmp339,_tmp339 + 5U},{_tmp33A,_tmp33A,_tmp33A + 9U},{_tmp33B,_tmp33B,_tmp33B + 6U},{_tmp33C,_tmp33C,_tmp33C + 7U},{_tmp33D,_tmp33D,_tmp33D + 5U},{_tmp33E,_tmp33E,_tmp33E + 7U},{_tmp33F,_tmp33F,_tmp33F + 16U},{_tmp340,_tmp340,_tmp340 + 10U},{_tmp341,_tmp341,_tmp341 + 8U},{_tmp342,_tmp342,_tmp342 + 4U},{_tmp343,_tmp343,_tmp343 + 6U},{_tmp344,_tmp344,_tmp344 + 4U},{_tmp345,_tmp345,_tmp345 + 6U},{_tmp346,_tmp346,_tmp346 + 7U},{_tmp347,_tmp347,_tmp347 + 9U},{_tmp348,_tmp348,_tmp348 + 5U},{_tmp349,_tmp349,_tmp349 + 4U},{_tmp34A,_tmp34A,_tmp34A + 9U},{_tmp34B,_tmp34B,_tmp34B + 9U},{_tmp34C,_tmp34C,_tmp34C + 6U},{_tmp34D,_tmp34D,_tmp34D + 10U},{_tmp34E,_tmp34E,_tmp34E + 9U},{_tmp34F,_tmp34F,_tmp34F + 6U},{_tmp350,_tmp350,_tmp350 + 7U},{_tmp351,_tmp351,_tmp351 + 8U},{_tmp352,_tmp352,_tmp352 + 15U},{_tmp353,_tmp353,_tmp353 + 7U},{_tmp354,_tmp354,_tmp354 + 8U},{_tmp355,_tmp355,_tmp355 + 5U},{_tmp356,_tmp356,_tmp356 + 9U},{_tmp357,_tmp357,_tmp357 + 6U},{_tmp358,_tmp358,_tmp358 + 7U},{_tmp359,_tmp359,_tmp359 + 5U},{_tmp35A,_tmp35A,_tmp35A + 8U},{_tmp35B,_tmp35B,_tmp35B + 7U},{_tmp35C,_tmp35C,_tmp35C + 8U},{_tmp35D,_tmp35D,_tmp35D + 7U},{_tmp35E,_tmp35E,_tmp35E + 10U},{_tmp35F,_tmp35F,_tmp35F + 11U},{_tmp360,_tmp360,_tmp360 + 8U},{_tmp361,_tmp361,_tmp361 + 8U},{_tmp362,_tmp362,_tmp362 + 10U},{_tmp363,_tmp363,_tmp363 + 9U},{_tmp364,_tmp364,_tmp364 + 13U},{_tmp365,_tmp365,_tmp365 + 10U},{_tmp366,_tmp366,_tmp366 + 9U},{_tmp367,_tmp367,_tmp367 + 13U},{_tmp368,_tmp368,_tmp368 + 14U},{_tmp369,_tmp369,_tmp369 + 14U},{_tmp36A,_tmp36A,_tmp36A + 13U},{_tmp36B,_tmp36B,_tmp36B + 13U},{_tmp36C,_tmp36C,_tmp36C + 13U},{_tmp36D,_tmp36D,_tmp36D + 12U},{_tmp36E,_tmp36E,_tmp36E + 13U},{_tmp36F,_tmp36F,_tmp36F + 15U},{_tmp370,_tmp370,_tmp370 + 15U},{_tmp371,_tmp371,_tmp371 + 12U},{_tmp372,_tmp372,_tmp372 + 16U},{_tmp373,_tmp373,_tmp373 + 14U},{_tmp374,_tmp374,_tmp374 + 12U},{_tmp375,_tmp375,_tmp375 + 16U},{_tmp376,_tmp376,_tmp376 + 7U},{_tmp377,_tmp377,_tmp377 + 7U},{_tmp378,_tmp378,_tmp378 + 7U},{_tmp379,_tmp379,_tmp379 + 8U},{_tmp37A,_tmp37A,_tmp37A + 9U},{_tmp37B,_tmp37B,_tmp37B + 6U},{_tmp37C,_tmp37C,_tmp37C + 6U},{_tmp37D,_tmp37D,_tmp37D + 6U},{_tmp37E,_tmp37E,_tmp37E + 6U},{_tmp37F,_tmp37F,_tmp37F + 7U},{_tmp380,_tmp380,_tmp380 + 6U},{_tmp381,_tmp381,_tmp381 + 11U},{_tmp382,_tmp382,_tmp382 + 11U},{_tmp383,_tmp383,_tmp383 + 11U},{_tmp384,_tmp384,_tmp384 + 11U},{_tmp385,_tmp385,_tmp385 + 11U},{_tmp386,_tmp386,_tmp386 + 12U},{_tmp387,_tmp387,_tmp387 + 13U},{_tmp388,_tmp388,_tmp388 + 11U},{_tmp389,_tmp389,_tmp389 + 11U},{_tmp38A,_tmp38A,_tmp38A + 10U},{_tmp38B,_tmp38B,_tmp38B + 9U},{_tmp38C,_tmp38C,_tmp38C + 11U},{_tmp38D,_tmp38D,_tmp38D + 12U},{_tmp38E,_tmp38E,_tmp38E + 11U},{_tmp38F,_tmp38F,_tmp38F + 17U},{_tmp390,_tmp390,_tmp390 + 7U},{_tmp391,_tmp391,_tmp391 + 8U},{_tmp392,_tmp392,_tmp392 + 19U},{_tmp393,_tmp393,_tmp393 + 20U},{_tmp394,_tmp394,_tmp394 + 18U},{_tmp395,_tmp395,_tmp395 + 9U},{_tmp396,_tmp396,_tmp396 + 13U},{_tmp397,_tmp397,_tmp397 + 16U},{_tmp398,_tmp398,_tmp398 + 18U},{_tmp399,_tmp399,_tmp399 + 10U},{_tmp39A,_tmp39A,_tmp39A + 8U},{_tmp39B,_tmp39B,_tmp39B + 4U},{_tmp39C,_tmp39C,_tmp39C + 4U},{_tmp39D,_tmp39D,_tmp39D + 4U},{_tmp39E,_tmp39E,_tmp39E + 4U},{_tmp39F,_tmp39F,_tmp39F + 4U},{_tmp3A0,_tmp3A0,_tmp3A0 + 4U},{_tmp3A1,_tmp3A1,_tmp3A1 + 4U},{_tmp3A2,_tmp3A2,_tmp3A2 + 4U},{_tmp3A3,_tmp3A3,_tmp3A3 + 4U},{_tmp3A4,_tmp3A4,_tmp3A4 + 4U},{_tmp3A5,_tmp3A5,_tmp3A5 + 4U},{_tmp3A6,_tmp3A6,_tmp3A6 + 4U},{_tmp3A7,_tmp3A7,_tmp3A7 + 4U},{_tmp3A8,_tmp3A8,_tmp3A8 + 4U},{_tmp3A9,_tmp3A9,_tmp3A9 + 4U},{_tmp3AA,_tmp3AA,_tmp3AA + 4U},{_tmp3AB,_tmp3AB,_tmp3AB + 4U},{_tmp3AC,_tmp3AC,_tmp3AC + 4U},{_tmp3AD,_tmp3AD,_tmp3AD + 4U},{_tmp3AE,_tmp3AE,_tmp3AE + 4U},{_tmp3AF,_tmp3AF,_tmp3AF + 4U},{_tmp3B0,_tmp3B0,_tmp3B0 + 4U},{_tmp3B1,_tmp3B1,_tmp3B1 + 4U},{_tmp3B2,_tmp3B2,_tmp3B2 + 4U},{_tmp3B3,_tmp3B3,_tmp3B3 + 4U},{_tmp3B4,_tmp3B4,_tmp3B4 + 4U},{_tmp3B5,_tmp3B5,_tmp3B5 + 4U},{_tmp3B6,_tmp3B6,_tmp3B6 + 5U},{_tmp3B7,_tmp3B7,_tmp3B7 + 17U},{_tmp3B8,_tmp3B8,_tmp3B8 + 18U},{_tmp3B9,_tmp3B9,_tmp3B9 + 19U},{_tmp3BA,_tmp3BA,_tmp3BA + 16U},{_tmp3BB,_tmp3BB,_tmp3BB + 13U},{_tmp3BC,_tmp3BC,_tmp3BC + 14U},{_tmp3BD,_tmp3BD,_tmp3BD + 17U},{_tmp3BE,_tmp3BE,_tmp3BE + 16U},{_tmp3BF,_tmp3BF,_tmp3BF + 12U},{_tmp3C0,_tmp3C0,_tmp3C0 + 19U},{_tmp3C1,_tmp3C1,_tmp3C1 + 13U},{_tmp3C2,_tmp3C2,_tmp3C2 + 21U},{_tmp3C3,_tmp3C3,_tmp3C3 + 15U},{_tmp3C4,_tmp3C4,_tmp3C4 + 20U},{_tmp3C5,_tmp3C5,_tmp3C5 + 21U},{_tmp3C6,_tmp3C6,_tmp3C6 + 13U},{_tmp3C7,_tmp3C7,_tmp3C7 + 15U},{_tmp3C8,_tmp3C8,_tmp3C8 + 17U},{_tmp3C9,_tmp3C9,_tmp3C9 + 19U},{_tmp3CA,_tmp3CA,_tmp3CA + 12U},{_tmp3CB,_tmp3CB,_tmp3CB + 17U},{_tmp3CC,_tmp3CC,_tmp3CC + 23U},{_tmp3CD,_tmp3CD,_tmp3CD + 24U},{_tmp3CE,_tmp3CE,_tmp3CE + 15U},{_tmp3CF,_tmp3CF,_tmp3CF + 11U},{_tmp3D0,_tmp3D0,_tmp3D0 + 15U},{_tmp3D1,_tmp3D1,_tmp3D1 + 10U},{_tmp3D2,_tmp3D2,_tmp3D2 + 15U},{_tmp3D3,_tmp3D3,_tmp3D3 + 25U},{_tmp3D4,_tmp3D4,_tmp3D4 + 5U},{_tmp3D5,_tmp3D5,_tmp3D5 + 15U},{_tmp3D6,_tmp3D6,_tmp3D6 + 15U},{_tmp3D7,_tmp3D7,_tmp3D7 + 11U},{_tmp3D8,_tmp3D8,_tmp3D8 + 22U},{_tmp3D9,_tmp3D9,_tmp3D9 + 26U},{_tmp3DA,_tmp3DA,_tmp3DA + 16U},{_tmp3DB,_tmp3DB,_tmp3DB + 16U},{_tmp3DC,_tmp3DC,_tmp3DC + 24U},{_tmp3DD,_tmp3DD,_tmp3DD + 25U},{_tmp3DE,_tmp3DE,_tmp3DE + 21U},{_tmp3DF,_tmp3DF,_tmp3DF + 22U},{_tmp3E0,_tmp3E0,_tmp3E0 + 16U},{_tmp3E1,_tmp3E1,_tmp3E1 + 19U},{_tmp3E2,_tmp3E2,_tmp3E2 + 25U},{_tmp3E3,_tmp3E3,_tmp3E3 + 35U},{_tmp3E4,_tmp3E4,_tmp3E4 + 23U},{_tmp3E5,_tmp3E5,_tmp3E5 + 24U},{_tmp3E6,_tmp3E6,_tmp3E6 + 18U},{_tmp3E7,_tmp3E7,_tmp3E7 + 20U},{_tmp3E8,_tmp3E8,_tmp3E8 + 9U},{_tmp3E9,_tmp3E9,_tmp3E9 + 19U},{_tmp3EA,_tmp3EA,_tmp3EA + 19U},{_tmp3EB,_tmp3EB,_tmp3EB + 18U},{_tmp3EC,_tmp3EC,_tmp3EC + 21U},{_tmp3ED,_tmp3ED,_tmp3ED + 19U},{_tmp3EE,_tmp3EE,_tmp3EE + 19U},{_tmp3EF,_tmp3EF,_tmp3EF + 14U},{_tmp3F0,_tmp3F0,_tmp3F0 + 19U},{_tmp3F1,_tmp3F1,_tmp3F1 + 20U},{_tmp3F2,_tmp3F2,_tmp3F2 + 14U},{_tmp3F3,_tmp3F3,_tmp3F3 + 11U},{_tmp3F4,_tmp3F4,_tmp3F4 + 23U},{_tmp3F5,_tmp3F5,_tmp3F5 + 18U},{_tmp3F6,_tmp3F6,_tmp3F6 + 30U},{_tmp3F7,_tmp3F7,_tmp3F7 + 8U},{_tmp3F8,_tmp3F8,_tmp3F8 + 12U},{_tmp3F9,_tmp3F9,_tmp3F9 + 14U},{_tmp3FA,_tmp3FA,_tmp3FA + 13U},{_tmp3FB,_tmp3FB,_tmp3FB + 23U},{_tmp3FC,_tmp3FC,_tmp3FC + 14U},{_tmp3FD,_tmp3FD,_tmp3FD + 18U},{_tmp3FE,_tmp3FE,_tmp3FE + 8U},{_tmp3FF,_tmp3FF,_tmp3FF + 8U},{_tmp400,_tmp400,_tmp400 + 11U},{_tmp401,_tmp401,_tmp401 + 20U},{_tmp402,_tmp402,_tmp402 + 9U},{_tmp403,_tmp403,_tmp403 + 16U},{_tmp404,_tmp404,_tmp404 + 19U},{_tmp405,_tmp405,_tmp405 + 10U},{_tmp406,_tmp406,_tmp406 + 16U},{_tmp407,_tmp407,_tmp407 + 11U},{_tmp408,_tmp408,_tmp408 + 14U},{_tmp409,_tmp409,_tmp409 + 11U},{_tmp40A,_tmp40A,_tmp40A + 15U},{_tmp40B,_tmp40B,_tmp40B + 22U},{_tmp40C,_tmp40C,_tmp40C + 16U},{_tmp40D,_tmp40D,_tmp40D + 17U},{_tmp40E,_tmp40E,_tmp40E + 12U},{_tmp40F,_tmp40F,_tmp40F + 18U},{_tmp410,_tmp410,_tmp410 + 17U},{_tmp411,_tmp411,_tmp411 + 12U},{_tmp412,_tmp412,_tmp412 + 16U},{_tmp413,_tmp413,_tmp413 + 11U},{_tmp414,_tmp414,_tmp414 + 10U},{_tmp415,_tmp415,_tmp415 + 14U},{_tmp416,_tmp416,_tmp416 + 15U},{_tmp417,_tmp417,_tmp417 + 20U},{_tmp418,_tmp418,_tmp418 + 27U},{_tmp419,_tmp419,_tmp419 + 10U},{_tmp41A,_tmp41A,_tmp41A + 18U},{_tmp41B,_tmp41B,_tmp41B + 21U},{_tmp41C,_tmp41C,_tmp41C + 19U},{_tmp41D,_tmp41D,_tmp41D + 16U},{_tmp41E,_tmp41E,_tmp41E + 20U},{_tmp41F,_tmp41F,_tmp41F + 15U},{_tmp420,_tmp420,_tmp420 + 20U},{_tmp421,_tmp421,_tmp421 + 15U},{_tmp422,_tmp422,_tmp422 + 12U},{_tmp423,_tmp423,_tmp423 + 20U},{_tmp424,_tmp424,_tmp424 + 19U},{_tmp425,_tmp425,_tmp425 + 20U},{_tmp426,_tmp426,_tmp426 + 21U},{_tmp427,_tmp427,_tmp427 + 21U},{_tmp428,_tmp428,_tmp428 + 12U},{_tmp429,_tmp429,_tmp429 + 17U},{_tmp42A,_tmp42A,_tmp42A + 19U},{_tmp42B,_tmp42B,_tmp42B + 14U},{_tmp42C,_tmp42C,_tmp42C + 17U},{_tmp42D,_tmp42D,_tmp42D + 23U},{_tmp42E,_tmp42E,_tmp42E + 13U},{_tmp42F,_tmp42F,_tmp42F + 14U},{_tmp430,_tmp430,_tmp430 + 16U},{_tmp431,_tmp431,_tmp431 + 16U},{_tmp432,_tmp432,_tmp432 + 8U},{_tmp433,_tmp433,_tmp433 + 19U},{_tmp434,_tmp434,_tmp434 + 20U},{_tmp435,_tmp435,_tmp435 + 14U},{_tmp436,_tmp436,_tmp436 + 19U},{_tmp437,_tmp437,_tmp437 + 20U},{_tmp438,_tmp438,_tmp438 + 11U},{_tmp439,_tmp439,_tmp439 + 22U},{_tmp43A,_tmp43A,_tmp43A + 20U},{_tmp43B,_tmp43B,_tmp43B + 23U},{_tmp43C,_tmp43C,_tmp43C + 20U},{_tmp43D,_tmp43D,_tmp43D + 22U},{_tmp43E,_tmp43E,_tmp43E + 23U},{_tmp43F,_tmp43F,_tmp43F + 24U},{_tmp440,_tmp440,_tmp440 + 24U},{_tmp441,_tmp441,_tmp441 + 15U},{_tmp442,_tmp442,_tmp442 + 20U},{_tmp443,_tmp443,_tmp443 + 22U},{_tmp444,_tmp444,_tmp444 + 17U},{_tmp445,_tmp445,_tmp445 + 20U},{_tmp446,_tmp446,_tmp446 + 26U},{_tmp447,_tmp447,_tmp447 + 16U},{_tmp448,_tmp448,_tmp448 + 17U},{_tmp449,_tmp449,_tmp449 + 9U},{_tmp44A,_tmp44A,_tmp44A + 13U},{_tmp44B,_tmp44B,_tmp44B + 12U},{_tmp44C,_tmp44C,_tmp44C + 12U},{_tmp44D,_tmp44D,_tmp44D + 11U},{_tmp44E,_tmp44E,_tmp44E + 11U},{_tmp44F,_tmp44F,_tmp44F + 11U},{_tmp450,_tmp450,_tmp450 + 16U},{_tmp451,_tmp451,_tmp451 + 17U},{_tmp452,_tmp452,_tmp452 + 15U},{_tmp453,_tmp453,_tmp453 + 19U},{_tmp454,_tmp454,_tmp454 + 17U},{_tmp455,_tmp455,_tmp455 + 19U},{_tmp456,_tmp456,_tmp456 + 25U},{_tmp457,_tmp457,_tmp457 + 26U},{_tmp458,_tmp458,_tmp458 + 9U},{_tmp459,_tmp459,_tmp459 + 20U},{_tmp45A,_tmp45A,_tmp45A + 17U},{_tmp45B,_tmp45B,_tmp45B + 18U},{_tmp45C,_tmp45C,_tmp45C + 11U},{_tmp45D,_tmp45D,_tmp45D + 12U}};
# 1698
static short Cyc_yyr1[577U]={0,158,159,159,159,159,159,159,159,159,159,159,159,160,161,162,163,164,164,165,165,165,166,166,167,167,167,168,168,168,169,169,170,170,170,171,171,172,172,172,172,173,173,174,175,176,177,178,178,178,178,178,178,178,178,179,179,180,180,180,180,180,180,180,180,180,180,180,181,181,181,181,181,181,181,182,182,183,184,184,185,185,185,185,186,186,187,187,187,187,187,187,187,187,187,187,187,187,187,187,187,187,187,187,187,187,187,187,187,188,189,189,189,190,190,190,191,191,192,192,192,193,193,193,193,193,194,194,195,195,196,196,197,197,198,199,199,200,200,201,202,202,202,202,202,202,203,203,203,203,203,203,204,205,205,206,206,206,206,207,207,208,208,209,209,210,210,211,211,211,211,212,212,213,213,214,214,214,215,215,216,216,216,216,217,217,217,218,218,219,219,220,220,221,221,221,221,221,221,221,221,221,221,222,222,222,222,222,222,222,222,222,222,222,223,223,224,225,225,226,226,226,226,226,226,226,226,227,227,227,228,228,229,229,229,230,230,231,231,231,232,232,233,233,233,233,234,234,235,235,236,236,237,237,238,238,239,239,240,240,240,240,241,241,242,242,243,243,243,244,245,245,246,246,247,247,247,247,247,248,248,248,248,249,249,250,250,251,251,252,252,253,253,253,253,253,254,254,255,255,255,256,256,256,256,256,256,256,256,256,256,256,257,257,257,257,257,257,258,259,259,260,260,261,261,261,261,261,261,261,261,262,262,262,262,262,262,263,263,263,263,263,263,264,264,264,264,264,264,264,264,264,264,264,264,264,264,265,265,265,265,265,265,265,265,266,267,267,268,268,269,269,270,270,271,271,272,272,273,273,273,274,274,274,274,274,275,275,275,276,276,276,277,277,277,277,278,278,279,279,279,279,279,279,280,281,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,282,283,283,283,284,284,285,285,286,286,286,287,287,288,288,289,289,289,290,290,290,290,290,290,290,290,290,290,290,291,291,291,291,291,291,291,291,292,293,293,294,294,295,295,296,296,297,297,298,298,298,299,299,299,299,299,300,300,300,301,301,301,302,302,302,302,303,303,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,304,305,306,306,307,307,307,308,308,309,309,309,310,310,311,312,312,312,313,313,314,314,314,315,315,315,315,315,315,315,315,315,315,315,316,316,316,316,317,317,317,317,317,317,317,317,317,317,317,318,319,319,320,320,320,320,320,321,321,322,322,323,323,324,324,325,325};
# 1759
static short Cyc_yyr2[577U]={0,1,2,3,5,3,5,8,3,3,3,3,0,1,1,2,1,0,4,1,2,3,0,1,4,3,4,1,2,3,0,4,1,1,1,1,0,3,4,4,5,3,4,2,1,2,1,2,3,5,3,6,3,8,5,1,2,1,2,2,1,2,1,2,1,2,1,2,1,1,1,1,2,1,1,0,1,6,1,3,1,1,4,8,1,2,1,1,1,1,1,1,1,1,1,1,1,4,1,1,1,1,3,4,4,1,4,1,4,1,1,1,1,5,2,4,1,3,1,2,3,4,9,8,4,3,0,3,1,1,0,1,1,2,1,1,3,1,3,3,1,2,1,2,1,2,1,2,1,2,1,2,1,1,3,2,2,0,3,4,0,3,5,4,0,4,0,4,1,1,0,1,0,4,0,6,3,5,1,2,1,2,3,3,0,1,1,2,5,1,2,1,2,1,3,4,4,5,10,11,4,4,2,1,1,3,4,4,5,10,11,4,4,2,1,2,5,0,2,4,4,1,1,1,1,1,1,2,2,1,0,3,0,1,1,1,3,0,1,1,0,2,3,5,5,7,1,3,0,2,0,2,3,5,0,1,1,3,2,3,4,1,1,3,1,3,2,1,2,1,1,3,1,1,2,3,4,8,8,1,2,3,4,2,2,1,2,3,2,1,2,1,2,3,4,3,1,3,1,1,2,3,3,4,4,5,10,9,11,10,4,2,1,1,1,1,1,1,3,1,2,2,3,1,2,3,4,1,2,1,2,5,7,7,5,8,6,0,4,4,5,6,7,5,7,6,7,7,8,7,8,8,9,6,7,7,8,3,2,2,2,3,2,4,5,1,1,5,1,3,1,3,1,3,1,3,1,3,1,3,3,1,3,3,3,3,1,3,3,1,3,3,1,3,3,3,1,4,1,2,2,4,2,6,1,1,1,3,1,1,3,6,6,4,4,5,4,2,2,2,4,4,4,1,3,1,1,3,1,2,1,3,1,1,3,1,3,1,3,3,1,1,1,1,1,1,1,1,1,1,1,1,5,2,2,2,5,5,5,1,1,3,1,3,1,3,1,3,1,3,1,3,3,1,3,3,3,3,1,3,3,1,3,3,1,3,3,3,1,4,1,2,2,2,2,2,2,4,2,6,4,6,6,9,11,4,6,6,4,2,2,5,0,1,0,2,3,1,3,0,2,3,1,3,4,0,1,2,1,3,1,1,1,1,4,3,4,3,3,2,2,5,6,7,1,1,3,3,1,4,1,1,1,3,2,5,4,5,5,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
# 1820
static short Cyc_yydefact[1198U]={0,34,68,69,70,71,73,86,87,88,89,90,91,92,93,94,110,111,112,128,129,64,0,0,98,0,0,74,0,0,178,105,107,0,0,0,13,14,0,0,0,567,246,569,568,570,0,230,0,230,101,0,229,1,0,0,0,0,32,0,0,33,0,57,66,60,84,62,95,96,0,99,0,0,189,0,214,217,100,193,126,72,71,65,0,114,0,59,566,0,567,562,563,564,565,0,126,0,0,406,0,0,0,269,0,408,409,43,45,0,0,0,0,0,0,0,0,179,0,0,0,227,0,0,228,0,0,0,0,0,2,0,0,0,0,47,0,134,135,137,58,67,61,63,130,571,572,126,126,0,55,0,0,36,0,248,0,202,190,215,0,221,222,225,226,0,224,223,237,217,0,85,72,118,0,116,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,551,552,512,0,0,0,0,0,532,530,531,0,435,437,451,460,462,464,466,468,470,473,478,481,484,488,0,490,533,550,548,567,418,0,0,0,0,419,0,0,417,50,0,0,126,0,0,0,144,140,142,289,291,0,0,52,0,0,0,8,9,0,126,573,574,247,109,0,0,0,194,102,267,0,264,10,11,0,3,0,5,0,48,0,0,0,36,0,131,132,157,125,0,176,0,0,0,0,0,0,0,0,0,0,0,0,567,319,321,0,329,323,0,327,312,313,314,0,315,316,317,0,56,36,137,35,37,296,0,254,270,0,0,250,248,0,232,0,0,0,239,238,75,235,218,0,119,115,0,0,0,498,0,0,510,453,488,0,454,455,0,0,0,0,0,0,0,0,0,0,0,491,492,513,509,0,494,0,0,0,0,495,493,0,97,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,441,442,443,444,445,446,447,448,449,450,440,0,496,0,539,540,0,0,0,554,0,126,410,0,0,0,432,567,574,0,0,0,0,285,428,433,0,430,0,0,407,425,426,0,423,271,0,0,0,0,292,0,262,145,150,146,148,141,143,248,0,298,290,299,576,575,0,104,106,0,0,0,108,124,81,80,0,78,231,195,248,266,191,298,268,203,204,0,103,16,30,44,0,46,0,136,138,273,272,36,38,121,133,0,0,0,152,153,160,0,126,126,184,0,0,0,0,0,567,0,0,0,358,359,360,0,0,362,0,0,0,330,324,137,328,322,320,39,0,201,255,0,0,0,261,249,256,160,0,0,0,250,200,234,233,196,232,0,0,240,76,0,127,120,459,117,113,0,0,0,0,567,274,279,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,553,560,0,559,436,461,0,463,465,467,469,471,472,476,477,474,475,479,480,482,483,485,486,487,439,438,538,535,0,537,0,0,0,421,422,0,288,0,429,283,286,416,0,284,420,413,0,49,0,414,0,293,0,151,147,149,0,250,0,232,0,300,0,248,0,311,295,0,54,0,126,0,0,0,144,0,126,0,248,0,213,192,265,0,22,4,6,40,0,156,139,157,0,0,155,250,177,186,185,0,0,180,0,0,0,337,0,0,0,0,0,0,357,361,0,0,0,325,318,0,41,297,248,0,258,0,0,174,251,0,160,254,242,197,219,220,240,216,236,497,0,0,0,275,0,280,0,500,0,0,0,0,0,549,505,508,0,0,514,0,0,489,556,0,0,536,534,0,0,0,0,287,431,434,424,427,415,294,263,160,0,301,302,232,0,0,250,232,0,0,51,250,567,0,77,79,0,205,0,0,250,0,232,0,0,0,17,23,154,0,158,130,175,187,181,184,0,0,0,0,0,0,0,0,0,0,0,0,0,337,363,0,326,42,250,0,259,257,0,164,0,174,250,0,241,545,0,544,0,276,281,0,458,0,0,0,0,456,457,538,537,519,0,558,541,0,561,452,555,557,0,411,412,174,160,304,310,160,0,303,232,0,130,0,82,206,212,160,0,211,207,232,0,0,0,0,0,0,0,183,182,331,337,0,0,0,0,0,0,365,366,368,370,372,374,376,378,381,386,389,392,396,398,404,405,0,0,334,343,0,0,0,0,0,0,0,0,0,0,364,244,260,0,0,166,252,164,243,248,499,0,0,282,501,502,0,0,507,506,0,525,519,515,517,511,542,0,164,174,174,160,305,53,0,0,174,160,208,31,25,0,0,27,0,7,159,123,0,0,0,337,0,402,0,0,399,337,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,400,337,0,345,0,0,0,353,0,0,0,0,0,0,336,0,0,0,170,0,166,250,547,546,0,0,0,0,526,525,522,520,0,516,543,166,164,164,174,122,0,164,174,26,24,28,0,0,19,188,332,333,0,0,0,0,337,339,369,0,371,373,375,377,379,380,384,385,382,383,387,388,390,391,393,394,395,0,338,344,346,347,0,355,354,0,349,0,0,0,173,0,0,0,0,168,169,172,253,170,245,0,0,0,0,0,528,527,0,521,518,170,166,166,164,0,166,164,29,18,20,0,335,401,0,397,340,0,337,348,356,350,351,0,0,0,163,0,184,171,198,172,278,277,503,0,524,0,523,172,170,170,166,83,170,166,21,0,367,337,341,352,161,0,165,0,199,0,529,307,172,172,170,172,170,403,342,0,167,504,306,309,172,209,172,162,308,210,0,0,0};
# 1943
static short Cyc_yydefgoto[168U]={1195,53,54,55,56,490,885,1054,796,797,971,674,57,321,58,305,59,492,60,494,61,151,62,63,559,243,476,477,244,66,260,245,68,173,174,69,171,70,282,283,136,137,138,284,246,457,505,506,507,684,1099,934,1025,1104,1147,829,71,72,689,690,691,73,508,74,482,75,76,168,169,77,121,555,336,337,727,646,78,647,549,718,541,545,546,451,329,269,102,103,573,497,574,431,432,433,247,322,323,648,463,308,309,310,311,312,313,811,314,315,898,899,900,901,902,903,904,905,906,907,908,909,910,911,912,913,434,443,444,435,436,437,316,207,411,208,565,209,210,211,212,213,214,215,216,217,218,219,220,369,370,852,951,952,1036,953,1038,1114,221,222,836,223,592,593,224,225,80,972,438,467};
# 1963
static short Cyc_yypact[1198U]={3004,- -32768,- -32768,- -32768,- -32768,- 27,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,3538,481,15,- -32768,3538,2985,- -32768,232,18,- -32768,65,128,- 34,109,144,- -32768,- -32768,160,336,339,- -32768,213,- -32768,- -32768,- -32768,277,304,715,304,341,354,- -32768,- -32768,383,391,392,2858,- -32768,440,503,- -32768,895,3538,3538,3538,- -32768,3538,- -32768,- -32768,651,- -32768,232,3296,205,79,369,1113,- -32768,- -32768,404,424,461,- -32768,232,457,6284,- -32768,- -32768,3229,344,- -32768,- -32768,- -32768,- -32768,468,404,500,6284,- -32768,484,3229,508,523,525,- -32768,88,- -32768,- -32768,3841,3841,132,544,2858,2858,6284,527,- -32768,95,538,6284,- -32768,766,561,- -32768,95,4263,2858,2858,3150,- -32768,2858,3150,2858,3150,- -32768,577,579,- -32768,1301,- -32768,- -32768,- -32768,- -32768,4263,- -32768,- -32768,404,198,1780,- -32768,3296,895,591,3841,3771,4877,- -32768,205,- -32768,606,- -32768,- -32768,- -32768,- -32768,617,- -32768,- -32768,70,1113,3841,- -32768,- -32768,632,643,645,232,6592,679,6633,6284,6432,687,707,730,744,749,753,756,760,764,769,774,6633,6633,- -32768,- -32768,806,6739,2550,785,6739,6739,- -32768,- -32768,- -32768,49,- -32768,- -32768,20,725,698,775,781,587,189,738,352,146,- -32768,1292,6739,80,- 54,- -32768,745,- 7,- -32768,3229,151,819,1096,835,313,1393,- -32768,- -32768,840,6284,404,1393,825,4007,4263,4333,4263,244,- -32768,54,54,- -32768,847,6284,829,- -32768,- -32768,408,404,- -32768,- -32768,- -32768,- -32768,39,833,830,- -32768,- -32768,811,436,- -32768,- -32768,- -32768,838,- -32768,842,- -32768,845,- -32768,766,4996,3296,591,846,4263,- -32768,922,836,232,849,848,206,850,4388,870,867,880,884,5038,2400,4388,243,874,- -32768,- -32768,892,1935,1935,895,1935,- -32768,- -32768,- -32768,899,- -32768,- -32768,- -32768,423,- -32768,591,897,- -32768,- -32768,888,73,923,- -32768,- 20,902,901,463,907,680,904,6284,928,- -32768,- -32768,925,909,- -32768,73,232,- -32768,6284,927,2550,- -32768,4263,2550,- -32768,- -32768,- -32768,4507,- -32768,958,6284,6284,6284,6284,6284,6284,6284,947,6284,4263,983,- -32768,- -32768,- -32768,- -32768,929,- -32768,1935,933,464,6284,- -32768,- -32768,6284,- -32768,6739,6284,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6284,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,6284,- -32768,95,- -32768,- -32768,5157,95,6284,- -32768,930,404,- -32768,931,936,937,- -32768,185,468,95,6284,3229,323,- -32768,- -32768,- -32768,943,945,940,3229,- -32768,- -32768,- -32768,941,946,- -32768,566,1096,948,3841,- -32768,944,951,- -32768,4333,4333,4333,- -32768,- -32768,3608,5199,269,- -32768,361,- -32768,- -32768,- 20,- -32768,- -32768,953,665,972,- -32768,961,- -32768,956,968,962,- -32768,- -32768,3392,- -32768,636,121,- -32768,- -32768,- -32768,4263,- -32768,- -32768,1064,- -32768,2858,- -32768,2858,- -32768,- -32768,- -32768,- -32768,591,- -32768,- -32768,- -32768,1153,6284,982,980,- -32768,- 3,268,404,404,853,6284,6284,977,987,6284,976,1093,2245,993,- -32768,- -32768,- -32768,666,1080,- -32768,5318,2090,2700,- -32768,- -32768,1301,- -32768,- -32768,- -32768,- -32768,3841,- -32768,- -32768,4263,988,4100,- -32768,- -32768,979,1049,- 20,991,4193,901,- -32768,- -32768,- -32768,- -32768,680,994,996,103,- -32768,928,- -32768,- -32768,- -32768,- -32768,- -32768,999,1006,1002,1025,1000,- -32768,- -32768,728,4996,1007,1010,1012,1017,1021,1024,467,1023,1026,1027,175,1041,1031,6473,- -32768,- -32768,1034,1037,- -32768,725,- 53,698,775,781,587,189,189,738,738,738,738,352,352,146,146,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,1035,- -32768,22,3841,4835,4263,- -32768,4263,- -32768,1029,- -32768,- -32768,- -32768,- -32768,1332,- -32768,- -32768,- -32768,1424,- -32768,1045,- -32768,264,- -32768,4263,- -32768,- -32768,- -32768,1040,901,1044,680,1047,361,3841,3937,5360,- -32768,- -32768,6284,- -32768,1050,404,6326,1048,39,3678,1055,404,3841,3771,5479,- -32768,636,- -32768,1066,1150,- -32768,- -32768,- -32768,1219,- -32768,- -32768,922,1061,6284,- -32768,901,- -32768,- -32768,- -32768,1072,232,673,485,498,6284,864,524,4388,1067,5521,5640,685,- -32768,- -32768,1075,1077,1069,1935,- -32768,3296,- -32768,888,1082,3841,- -32768,1084,- 20,1139,- -32768,1086,1049,258,- -32768,- -32768,- -32768,- -32768,103,- -32768,- -32768,1094,418,1094,1088,- -32768,4549,- -32768,6739,- -32768,6284,6284,1202,6284,6432,- -32768,- -32768,- -32768,95,95,1085,1090,4671,- -32768,- -32768,6284,6284,- -32768,- -32768,73,777,1114,1115,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,1049,1097,- -32768,- -32768,680,73,1100,901,680,1087,526,- -32768,901,1102,1103,- -32768,- -32768,1104,- -32768,73,1106,901,1107,680,1105,3150,1117,1203,- -32768,- -32768,6284,- -32768,4263,- -32768,1122,96,853,4388,1121,1116,1030,1118,1130,4388,6284,5682,689,5801,691,5843,864,- -32768,1133,- -32768,- -32768,901,312,- -32768,- -32768,1126,1184,1143,1139,901,4263,- -32768,- -32768,292,- -32768,6284,- -32768,- -32768,4996,958,1132,1135,1131,1151,- -32768,958,1137,1144,- 4,1146,- -32768,- -32768,786,- -32768,- -32768,- -32768,- -32768,4835,- -32768,- -32768,1139,1049,- -32768,- -32768,1049,1147,- -32768,680,1158,4263,1171,- -32768,- -32768,- -32768,1049,1155,- -32768,- -32768,680,1160,593,1164,2858,1157,1166,4263,- -32768,- -32768,1272,864,1170,6780,1185,2700,6739,1159,- -32768,46,- -32768,1220,1176,770,834,216,843,406,289,- -32768,- -32768,- -32768,- -32768,1224,6739,1935,- -32768,- -32768,565,4388,570,5962,4388,590,6004,6123,709,1195,- -32768,- -32768,- -32768,6284,1190,1246,1197,1184,- -32768,1082,- -32768,442,71,- -32768,- -32768,- -32768,4263,1308,- -32768,- -32768,1196,- 2,168,- -32768,- -32768,- -32768,- -32768,4713,1184,1139,1139,1049,- -32768,- -32768,1209,1214,1139,1049,- -32768,- -32768,- -32768,1226,1228,721,527,- -32768,- -32768,- -32768,599,4388,1233,864,2550,- -32768,4263,1212,- -32768,1625,6739,6284,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6739,6284,- -32768,864,1238,- -32768,4388,4388,603,- -32768,4388,4388,610,4388,612,6165,- -32768,1231,1240,1235,563,- 20,1246,901,- -32768,- -32768,2700,1237,1239,6284,1259,184,- -32768,- -32768,1260,- -32768,- -32768,1246,1184,1184,1139,- -32768,1264,1184,1139,- -32768,- -32768,- -32768,527,1251,726,- -32768,- -32768,- -32768,1254,1248,1255,6739,864,- -32768,725,237,698,775,775,587,189,189,738,738,738,738,352,352,146,146,- -32768,- -32768,- -32768,276,- -32768,- -32768,- -32768,- -32768,4388,- -32768,- -32768,4388,- -32768,4388,4388,613,- -32768,3841,614,1240,1252,- -32768,- -32768,1302,- -32768,563,- -32768,1262,814,1256,4263,634,- -32768,1258,1260,- -32768,- -32768,563,1246,1246,1184,1268,1246,1184,- -32768,- -32768,- -32768,527,- -32768,1094,418,- -32768,- -32768,6284,1625,- -32768,- -32768,- -32768,- -32768,4388,822,1277,- -32768,650,853,- -32768,- -32768,1302,- -32768,- -32768,- -32768,1270,- -32768,1291,- -32768,1302,563,563,1246,- -32768,563,1246,- -32768,357,- -32768,864,- -32768,- -32768,- -32768,3841,- -32768,1273,- -32768,1274,- -32768,- -32768,1302,1302,563,1302,563,- -32768,- -32768,828,- -32768,- -32768,- -32768,- -32768,1302,- -32768,1302,- -32768,- -32768,- -32768,1412,1415,- -32768};
# 2086
static short Cyc_yypgoto[168U]={- -32768,233,- -32768,- -32768,- -32768,- -32768,- -32768,288,- -32768,- -32768,364,- -32768,- -32768,- 241,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- 63,- 136,- 14,- -32768,- -32768,0,758,- -32768,123,- 169,1294,33,- -32768,- -32768,- 132,- -32768,252,1383,- 729,- -32768,- -32768,- -32768,1145,1142,548,410,- -32768,- -32768,750,3,332,- 675,- 732,- 980,- 706,- 198,- -32768,- -32768,- 782,- -32768,- -32768,131,- 148,1363,- 394,134,- -32768,1271,- -32768,- -32768,1390,- 332,- 297,- -32768,716,- 121,- 126,- 125,- 406,419,731,733,- 449,- 472,- 124,- 456,- 150,- -32768,- 250,- 154,- 577,- 286,- -32768,1014,387,- 71,- 156,- 8,- 298,5,- -32768,- -32768,- 47,- 290,- -32768,- 380,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- -32768,- 6,1213,- -32768,831,1013,- -32768,460,516,- -32768,- 152,- 387,- 173,- 376,- 373,- 370,1073,- 361,- 326,- 281,- 324,- 315,- 166,608,- -32768,- -32768,- -32768,- -32768,510,- -32768,- 880,426,- -32768,659,1108,338,- -32768,- 375,- -32768,450,451,- 69,- 64,- 106,- 94};
# 2110
static short Cyc_yytable[6938U]={64,146,268,280,595,330,147,83,354,597,150,87,261,598,340,531,532,656,534,261,104,64,889,890,600,64,153,353,350,496,328,672,371,67,327,376,377,558,248,249,500,618,338,627,344,758,542,146,122,140,141,142,257,143,67,412,474,64,67,152,601,602,419,64,64,64,575,64,607,608,1037,715,887,64,157,455,150,537,682,609,610,378,588,227,111,712,306,420,317,671,67,754,281,81,672,235,67,67,67,719,67,687,688,112,318,42,67,603,604,605,606,228,543,64,64,671,949,679,949,481,16,17,18,65,380,152,1148,64,64,64,229,64,64,64,64,307,108,152,1156,64,950,683,1035,963,65,722,67,67,65,64,987,64,464,706,86,468,378,475,157,1117,67,67,67,651,67,67,67,67,381,756,768,464,67,855,413,414,415,1177,1178,123,65,1180,67,378,67,651,65,65,65,379,65,564,465,139,988,42,65,41,486,1189,544,1191,109,466,551,378,44,538,338,563,159,465,335,258,- 184,1031,486,317,48,416,239,259,422,- 184,723,417,418,240,442,539,611,612,613,499,442,1155,65,65,462,41,113,770,306,306,454,306,562,43,44,45,65,65,65,264,65,65,65,65,677,484,480,1027,65,250,728,110,152,460,251,746,414,415,65,252,65,114,423,456,564,801,64,396,1042,319,87,455,455,455,388,389,130,307,307,424,307,1106,800,519,64,115,397,398,1039,228,526,64,64,616,64,306,1118,619,950,67,416,995,996,772,1115,808,747,418,229,626,41,735,390,391,1035,- 573,118,67,665,44,46,170,575,560,67,67,544,67,154,287,155,514,254,255,231,515,41,156,564,997,998,665,19,20,307,44,270,271,272,1172,273,274,275,276,1119,1120,868,378,64,1123,527,872,540,938,640,47,461,1134,528,459,878,49,41,1157,1158,326,460,1161,52,117,43,44,45,709,- 248,286,288,483,- 248,65,767,67,268,503,459,319,378,666,886,466,486,460,119,822,930,509,1135,65,544,751,1003,628,937,1179,65,65,1181,65,939,634,486,338,120,940,533,928,486,865,1173,1004,1005,869,1159,378,841,1162,1176,79,931,440,454,454,454,700,125,629,880,466,228,655,575,757,707,711,429,430,150,1187,1188,85,1190,- 270,105,106,- 270,107,664,229,670,1193,840,1194,710,456,456,456,46,447,64,126,64,65,774,1182,652,79,653,394,940,678,47,395,152,654,79,473,49,788,979,79,127,152,786,52,791,152,64,145,128,148,129,67,79,67,776,64,786,564,775,64,708,175,258,835,961,105,106,170,378,790,259,1022,206,789,472,967,717,105,106,67,536,1001,- 15,378,233,1002,258,1029,67,842,79,79,67,145,259,848,487,131,132,79,841,256,488,719,79,79,79,172,79,79,79,79,373,824,847,176,544,942,726,461,326,666,832,550,378,41,1059,378,857,79,590,230,1064,742,44,486,840,1065,486,84,483,65,1067,65,232,378,1068,1069,1107,234,837,806,1008,175,1085,452,765,1070,378,936,133,134,263,509,807,236,849,850,825,65,306,41,317,564,1101,1102,1103,655,65,43,44,45,65,237,378,374,378,238,823,858,812,957,871,1071,1072,253,841,670,332,622,267,1077,1078,262,105,106,866,105,106,1133,105,106,1079,1080,386,387,105,106,285,307,876,152,1063,637,446,378,378,265,708,267,378,1009,840,64,277,64,1011,41,470,278,1073,1074,1075,1076,485,43,44,45,320,831,378,675,969,676,970,79,1015,985,567,487,568,569,485,378,511,1056,67,516,67,1089,378,333,378,378,1142,1007,1092,585,1094,1140,1143,1167,334,525,79,726,1043,1044,685,686,977,46,341,1048,378,41,553,554,863,667,1153,668,342,43,44,45,564,343,669,144,1142,346,1183,349,351,351,1171,453,175,458,64,498,658,703,509,378,378,366,367,914,804,374,351,805,374,351,351,891,509,1028,146,509,818,918,347,378,922,582,925,378,368,378,355,67,382,351,65,285,65,41,1030,392,393,1081,1082,1083,1020,596,44,378,46,1166,356,1121,383,557,47,1124,1052,306,48,1053,49,1127,687,688,1128,733,734,52,643,644,645,958,498,357,959,576,577,578,579,580,581,421,620,584,965,105,106,358,41,64,809,810,359,105,106,591,360,44,594,361,1132,105,106,362,544,47,307,363,146,48,349,49,364,1055,859,860,781,365,52,614,64,65,67,955,956,991,992,306,375,1010,384,615,1014,41,452,485,591,385,485,993,994,43,44,45,999,1000,1141,79,425,79,47,1150,378,67,480,351,49,439,485,1169,538,460,445,52,485,1192,538,1045,449,469,478,471,510,1049,479,489,307,692,693,491,650,696,493,501,701,512,1165,1057,146,521,64,513,351,517,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,267,65,520,759,522,760,41,1184,523,1087,1088,529,67,1090,1091,44,1093,88,837,135,882,267,452,47,530,535,279,48,267,49,466,351,65,41,540,547,52,591,548,591,552,43,44,45,556,189,42,46,894,895,47,561,146,566,503,380,49,1055,583,504,621,587,623,52,88,306,589,624,625,631,641,267,632,636,635,105,106,633,642,105,106,639,267,659,498,657,660,1136,661,663,1137,267,1138,1139,41,91,195,196,92,93,94,662,65,44,673,351,680,681,485,694,779,974,695,697,307,348,698,702,704,200,713,716,485,682,720,485,89,724,64,725,88,498,729,730,803,731,732,- 573,1168,738,736,226,91,737,739,92,93,94,740,95,44,741,815,817,748,96,743,749,97,744,745,67,896,778,753,99,100,752,755,761,783,766,897,769,780,101,203,771,793,204,205,784,160,161,162,163,164,773,787,795,351,794,89,799,267,165,166,167,802,813,819,820,821,591,426,453,326,427,91,267,826,92,93,94,828,428,44,830,838,750,845,96,851,853,97,861,862,870,98,873,864,99,100,867,429,430,874,875,79,877,879,101,883,498,884,881,892,843,844,893,846,65,105,106,888,916,917,929,932,498,933,935,856,945,41,943,919,921,944,924,947,927,43,44,45,46,984,948,946,954,960,47,962,964,351,503,968,49,966,973,975,941,976,978,52,980,986,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,989,983,21,990,1006,1021,1023,1024,1026,1032,1033,145,1034,79,41,22,23,24,25,1046,26,351,43,44,45,1047,285,351,27,1062,399,47,30,233,498,503,1050,49,1051,351,31,32,33,1058,52,1060,1086,1061,1097,1098,88,1100,38,498,1110,1111,1113,949,267,1013,1122,1126,1017,1019,1129,1130,1131,1146,1145,1154,39,40,1149,1151,400,401,402,403,404,405,406,407,408,409,351,1160,1170,1174,1175,1196,1185,1186,1197,1163,1125,1108,266,285,785,116,495,145,502,42,43,410,45,46,798,1144,149,88,89,267,279,158,124,339,374,834,50,51,1105,630,762,1066,827,427,91,833,448,92,93,94,599,428,44,638,1040,1116,763,96,88,1084,97,915,1164,0,98,498,586,99,100,0,429,430,0,1096,0,0,0,101,0,0,0,0,0,0,1109,0,0,1112,0,89,0,1152,0,0,0,982,0,145,351,0,0,441,0,0,226,91,0,0,92,93,94,0,95,44,0,0,351,0,96,0,89,97,0,0,0,98,0,0,99,100,0,0,764,351,0,226,91,0,101,92,93,94,0,95,44,0,0,0,0,96,0,0,97,0,0,0,98,0,0,99,100,0,0,0,0,0,0,0,0,101,0,0,145,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,351,0,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,351,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,809,810,21,177,178,289,0,290,291,292,293,294,295,296,297,22,23,24,298,88,26,180,299,351,0,0,0,181,27,300,0,0,30,182,183,184,185,186,187,0,31,32,33,188,0,0,0,189,0,0,190,191,38,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,351,301,91,195,196,92,93,94,42,43,44,45,46,197,302,149,0,0,198,0,0,0,199,0,0,50,304,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,289,0,290,291,292,293,294,295,296,297,22,23,24,298,88,26,180,299,0,0,0,0,181,27,300,0,0,30,182,183,184,185,186,187,0,31,32,33,188,0,0,0,189,0,0,190,191,38,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,301,91,195,196,92,93,94,42,43,44,45,46,197,302,149,303,0,198,0,0,0,199,0,0,50,304,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,289,0,290,291,292,293,294,295,296,297,22,23,24,298,88,26,180,299,0,0,0,0,181,27,300,0,0,30,182,183,184,185,186,187,0,31,32,33,188,0,0,0,189,0,0,190,191,38,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,301,91,195,196,92,93,94,42,43,44,45,46,197,302,149,0,0,198,0,0,0,199,0,0,50,304,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,289,0,290,291,292,293,294,295,296,297,22,23,24,298,88,26,180,299,0,0,0,0,181,27,300,0,0,30,182,183,184,185,186,187,0,31,32,33,188,0,0,0,189,0,0,190,191,38,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,518,91,195,196,92,93,94,42,43,44,45,46,197,302,149,0,0,198,0,0,0,199,0,0,50,304,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,0,0,0,0,0,0,0,0,0,0,22,23,24,298,88,26,180,0,0,0,0,0,181,27,0,0,0,30,182,183,184,185,186,187,0,31,32,33,188,0,0,0,189,0,0,190,191,38,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,42,43,44,45,46,197,699,0,0,0,198,0,0,0,199,0,0,50,304,0,0,0,0,0,201,0,0,202,203,0,0,204,205,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,177,178,0,0,0,0,0,0,0,0,0,0,22,23,24,298,88,0,0,0,0,0,0,0,0,27,0,0,0,30,0,183,184,185,186,187,0,31,32,0,0,0,0,0,189,0,0,190,191,38,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,42,43,44,45,46,197,0,0,0,0,198,0,0,0,348,0,0,50,304,0,0,0,0,0,201,0,0,202,203,0,0,204,205,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,177,178,0,0,0,0,0,0,0,0,0,0,22,23,24,179,88,0,180,0,0,0,0,0,181,0,0,0,0,30,182,183,184,185,186,187,0,31,32,0,188,0,0,0,189,0,0,190,191,38,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,42,43,44,45,46,197,0,372,0,0,198,0,0,0,199,0,0,50,304,0,0,0,0,0,201,0,0,202,203,0,0,204,205,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,177,178,0,0,0,0,0,0,0,0,0,0,22,23,24,179,88,0,180,0,0,0,0,0,181,0,0,0,0,30,182,183,184,185,186,187,0,31,32,0,188,0,0,0,189,0,0,190,191,38,192,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,91,195,196,92,93,94,42,43,44,45,46,197,0,0,0,0,198,0,0,0,199,0,0,50,304,0,0,0,0,0,201,0,0,202,203,0,0,204,205,- 12,1,0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,25,0,26,0,0,0,0,0,0,0,27,0,28,29,30,0,0,0,0,0,0,0,31,32,33,0,0,34,35,0,36,37,0,0,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,0,0,0,0,0,0,42,43,44,45,46,0,0,0,- 12,0,47,0,0,0,48,0,49,50,51,0,0,- 12,1,52,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,88,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,25,0,26,0,0,0,0,0,0,0,27,0,28,29,30,0,0,0,0,0,0,0,31,32,33,0,0,34,35,0,36,37,0,0,38,0,0,0,0,0,0,0,0,0,89,0,0,0,0,0,0,0,39,40,0,0,0,0,0,90,91,0,0,92,93,94,0,95,44,0,0,0,0,96,0,0,97,0,41,0,98,0,0,99,100,42,43,44,45,46,0,0,0,101,0,47,0,0,0,48,0,49,50,51,0,0,0,1,52,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,25,0,26,0,0,0,0,0,0,0,27,0,28,29,30,0,0,0,0,0,0,0,31,32,33,0,0,34,35,0,36,37,0,0,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,41,0,88,0,0,0,0,42,43,44,45,46,0,0,0,- 12,0,47,0,0,0,48,0,49,50,51,0,0,0,0,52,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,89,22,23,24,25,0,26,0,0,0,0,0,0,0,27,226,91,0,30,92,93,94,0,95,44,0,31,32,33,96,0,0,97,0,0,0,98,0,38,99,100,0,0,0,0,0,0,0,0,101,0,0,0,0,0,0,39,40,0,0,0,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,42,43,0,45,46,0,0,149,22,23,24,0,0,0,0,0,0,50,51,0,0,0,0,0,0,30,0,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,324,0,0,41,0,0,0,0,0,0,42,43,44,45,46,0,326,0,0,0,47,0,0,0,480,0,49,50,51,0,0,460,0,52,2,3,4,82,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,21,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,25,0,0,0,0,0,0,0,0,0,27,0,0,0,30,0,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,39,40,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,42,43,0,45,46,0,0,31,32,0,0,0,0,0,0,0,0,50,51,38,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,39,40,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,324,0,0,0,0,0,0,0,0,30,42,43,0,45,46,0,326,31,32,0,47,0,0,0,459,0,49,50,51,38,0,460,0,52,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,41,0,0,0,0,0,0,42,43,44,45,46,22,23,24,0,0,47,0,0,0,503,0,49,50,51,0,0,0,30,52,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,39,40,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,324,0,0,325,0,0,0,0,0,30,42,43,0,45,46,0,326,31,32,0,0,241,0,0,0,0,0,50,51,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,0,0,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,42,43,0,45,46,0,0,242,22,23,24,0,0,0,0,0,0,50,51,0,0,0,0,0,0,30,0,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,39,40,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,324,0,0,0,0,0,0,0,0,30,42,43,0,45,46,0,326,31,32,0,0,0,0,0,0,0,0,50,51,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,42,43,0,45,46,22,23,24,450,0,0,0,0,0,0,0,0,50,51,0,0,0,30,0,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,39,40,0,0,0,0,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,0,0,0,0,0,0,0,0,0,0,42,43,0,45,46,22,23,24,714,0,0,0,0,0,0,0,0,50,51,0,0,0,30,0,0,0,0,0,0,0,31,32,0,0,0,0,0,0,0,0,0,0,38,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,39,40,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,721,0,0,0,0,0,0,0,0,30,42,43,0,45,46,0,0,31,32,0,0,0,0,0,0,0,0,50,51,38,0,0,0,7,8,9,10,11,12,13,14,15,16,17,18,19,20,39,40,0,0,0,0,0,0,0,0,0,0,0,0,0,22,23,24,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,42,43,0,45,46,0,0,31,32,0,0,0,0,0,0,0,0,50,51,38,0,0,0,0,0,177,178,289,0,290,291,292,293,294,295,296,297,39,40,0,179,88,0,180,299,0,0,0,0,181,0,300,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,42,0,190,191,46,192,0,0,0,0,0,0,0,0,0,0,0,50,51,0,0,0,0,0,0,193,194,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,518,91,195,196,92,93,94,0,0,44,0,0,197,302,149,0,0,198,0,0,0,199,0,0,0,200,177,178,0,0,0,201,0,570,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,571,91,195,196,92,93,94,0,259,44,0,0,197,0,352,572,0,198,0,193,194,199,0,0,0,200,0,429,430,0,0,201,0,0,202,203,0,0,204,205,0,0,571,91,195,196,92,93,94,0,259,44,0,0,197,0,352,839,0,198,0,0,0,199,0,0,0,200,0,429,430,177,178,201,0,0,202,203,0,0,204,205,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,571,91,195,196,92,93,94,0,259,44,0,0,197,0,352,854,0,198,0,193,194,199,0,0,0,200,0,429,430,0,0,201,0,0,202,203,0,0,204,205,0,0,571,91,195,196,92,93,94,0,259,44,0,0,197,0,352,1041,0,198,0,0,0,199,0,0,0,200,0,429,430,177,178,201,0,0,202,203,0,0,204,205,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,571,91,195,196,92,93,94,0,259,44,0,0,197,0,352,0,0,198,0,193,194,199,0,0,0,200,0,429,430,0,0,201,0,0,202,203,0,0,204,205,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,0,0,0,200,177,178,0,331,0,201,0,0,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,41,91,195,196,92,93,94,0,0,44,0,0,197,0,352,0,0,198,0,193,194,199,0,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,524,0,0,0,198,0,0,0,199,0,0,0,200,177,178,0,0,0,201,0,0,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,193,194,199,617,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,0,0,0,200,177,178,0,649,0,201,0,0,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,193,194,199,705,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,0,0,0,200,177,178,0,777,0,201,0,0,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,193,194,199,0,0,0,200,0,0,0,792,0,201,0,0,202,203,0,0,204,205,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,814,0,0,0,198,0,0,0,199,0,0,0,200,177,178,0,0,0,201,0,0,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,41,91,195,196,92,93,94,0,0,44,0,0,197,816,0,0,0,198,0,193,194,199,0,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,920,0,0,200,177,178,0,0,0,201,0,0,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,193,194,199,923,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,926,0,0,0,198,0,0,0,199,0,0,0,200,177,178,0,0,0,201,0,0,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,193,194,199,1012,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,1016,0,0,200,177,178,0,0,0,201,0,0,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,193,194,199,1018,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,199,1095,0,0,200,177,178,0,0,0,201,0,0,202,203,0,0,204,205,0,179,88,0,180,0,0,0,0,0,181,0,0,0,0,0,182,183,184,185,186,187,0,0,0,0,188,0,177,178,189,0,0,190,191,0,192,0,0,0,0,0,0,179,88,0,180,0,0,0,0,0,181,0,0,0,193,194,182,183,184,185,186,187,0,0,0,0,188,0,0,0,189,0,0,190,191,0,192,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,193,194,199,0,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,0,782,91,195,196,92,93,94,0,0,44,0,0,197,177,178,0,0,198,0,0,0,199,0,0,0,200,0,0,179,88,0,201,0,0,202,203,0,0,204,205,0,0,0,0,183,184,185,186,187,0,0,0,0,0,177,178,0,189,0,0,190,191,0,192,0,0,0,0,0,179,88,0,0,0,0,0,0,0,0,0,0,0,0,193,194,183,184,185,186,187,0,0,0,0,0,0,0,0,189,0,0,190,191,0,192,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,352,0,0,198,193,194,0,199,0,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,750,0,0,198,0,0,0,199,0,0,0,200,177,178,0,0,0,201,0,0,202,203,0,0,204,205,0,179,88,0,0,0,0,0,0,0,0,0,0,0,0,0,0,183,184,185,186,187,0,0,0,0,0,177,178,0,189,0,0,190,191,0,192,0,0,0,0,0,179,88,0,0,0,0,0,0,0,0,0,0,0,0,193,194,183,184,185,186,187,0,0,0,0,0,0,0,0,189,0,0,190,191,0,192,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,193,194,0,345,0,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,41,91,195,196,92,93,94,0,0,44,0,0,197,177,178,0,0,198,0,0,0,348,0,0,0,200,0,0,179,88,0,201,0,0,202,203,0,0,204,205,0,0,0,0,183,184,185,186,187,0,0,0,0,0,177,178,0,189,0,0,190,191,0,192,0,0,0,0,0,179,88,0,0,0,0,0,0,0,0,0,0,0,0,193,194,183,184,185,186,187,0,0,0,0,0,0,0,0,189,0,0,190,191,0,192,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,193,194,0,199,0,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205,0,41,91,195,196,92,93,94,0,0,44,0,0,197,0,0,0,0,198,0,0,0,981,0,0,0,200,0,0,0,0,0,201,0,0,202,203,0,0,204,205};
# 2807
static short Cyc_yycheck[6938U]={0,70,126,139,380,155,70,21,181,382,73,25,118,383,170,305,306,466,308,125,26,21,804,805,385,25,73,181,180,279,155,487,198,0,155,201,202,334,109,110,281,416,168,430,176,622,66,116,48,63,64,65,116,67,21,221,17,57,25,73,386,387,116,63,64,65,352,67,392,393,950,543,801,73,74,244,139,318,81,394,395,134,372,89,118,541,149,141,151,483,57,144,139,120,550,101,63,64,65,548,67,5,6,137,151,125,73,388,389,390,391,118,132,113,114,509,120,504,120,267,17,18,19,0,104,139,1106,127,128,129,137,131,132,133,134,149,118,151,1118,139,144,144,144,872,21,551,113,114,25,149,104,151,98,528,139,249,134,118,158,1039,127,128,129,461,131,132,133,134,148,147,642,98,139,750,94,95,96,1157,1158,48,57,1161,149,134,151,483,63,64,65,140,67,343,138,62,148,125,73,118,267,1179,326,1181,137,149,329,134,127,134,334,341,76,138,142,118,118,144,285,280,139,139,132,126,228,127,556,145,146,139,234,323,396,397,398,280,240,1115,113,114,246,118,131,647,305,306,244,308,340,126,127,128,127,128,129,122,131,132,133,134,499,267,139,936,139,131,561,137,280,146,136,94,95,96,149,141,151,131,125,244,430,685,280,135,957,152,298,454,455,456,99,100,57,305,306,142,308,1027,683,292,298,139,154,155,134,118,299,305,306,413,308,372,1042,417,144,280,139,99,100,649,134,694,145,146,137,429,118,575,137,138,144,144,117,298,480,127,129,137,622,337,305,306,466,308,137,145,139,139,113,114,96,143,118,146,504,137,138,503,20,21,372,127,127,128,129,1145,131,132,133,134,1043,1044,776,134,372,1048,131,781,118,833,449,135,246,144,139,139,790,141,118,1119,1120,131,146,1123,148,54,126,127,128,533,140,147,148,267,144,280,140,372,530,139,139,278,134,480,799,149,483,146,139,707,824,285,144,298,548,589,135,431,832,1159,305,306,1162,308,140,439,503,561,132,145,307,819,509,773,1148,154,155,777,1121,134,734,1124,1156,0,140,140,454,455,456,520,117,136,792,149,118,463,750,621,529,538,145,146,533,1177,1178,22,1180,131,26,26,134,28,480,137,482,1189,734,1191,533,454,455,456,129,239,492,139,494,372,652,140,137,48,139,149,145,503,135,153,520,146,57,257,141,667,892,62,131,529,664,148,668,533,520,70,131,72,132,492,75,494,653,529,678,683,653,533,529,84,118,119,870,89,89,137,134,668,126,932,86,668,140,881,547,101,101,520,131,149,132,134,98,153,118,119,529,736,113,114,533,116,126,742,134,131,132,122,860,115,140,1026,127,128,129,120,131,132,133,134,199,712,742,132,716,841,559,459,131,664,721,134,134,118,980,134,754,152,140,137,986,140,127,678,860,987,681,132,480,492,989,494,118,134,990,991,1028,139,730,140,916,176,1008,242,636,992,134,831,131,132,120,503,140,131,746,747,713,520,707,118,709,799,85,86,87,651,529,126,127,128,533,134,134,199,134,136,709,757,140,863,140,993,994,125,956,671,156,421,126,999,1000,139,228,228,774,231,231,1063,234,234,1001,1002,101,102,240,240,144,707,788,709,986,131,238,134,134,140,697,155,134,140,956,707,131,709,140,118,252,134,995,996,997,998,267,126,127,128,131,720,134,492,133,494,135,278,140,897,345,134,347,348,285,134,287,140,707,290,709,140,134,139,134,134,134,915,140,364,140,140,140,1135,139,297,307,726,958,959,510,511,888,129,134,965,134,118,90,91,769,137,140,139,133,126,127,128,932,136,146,132,134,177,1166,179,180,181,140,243,341,245,794,279,131,131,664,134,134,193,194,809,131,345,198,134,348,201,202,806,678,938,883,681,131,812,139,134,131,361,131,134,18,134,139,794,103,221,707,283,709,118,940,97,98,1003,1004,1005,131,381,127,134,129,1135,139,1045,150,333,135,1049,131,916,139,134,141,131,5,6,134,133,134,148,454,455,456,864,352,139,867,355,356,357,358,359,360,132,418,363,877,431,431,139,118,885,22,23,139,439,439,375,139,127,378,139,1062,447,447,139,1026,135,916,139,973,139,298,141,139,973,133,134,660,139,148,399,916,794,885,133,134,151,152,986,139,920,151,411,923,118,543,480,416,152,483,101,102,126,127,128,97,98,1098,492,125,494,135,133,134,916,139,343,141,118,503,133,134,146,118,148,509,133,134,960,139,118,133,138,132,966,140,133,986,513,514,133,460,517,133,133,520,132,1134,978,1053,118,986,139,380,139,382,383,384,385,386,387,388,389,390,391,392,393,394,395,396,397,398,459,885,139,623,131,625,118,1170,131,1011,1012,144,986,1015,1016,127,1018,41,1131,131,794,480,642,135,139,133,136,139,487,141,149,430,916,118,118,140,148,528,144,530,140,126,127,128,147,69,125,129,25,26,135,149,1128,133,139,104,141,1128,118,144,137,139,138,148,41,1135,140,138,138,133,133,530,134,134,140,632,632,144,134,636,636,140,541,118,575,139,132,1089,139,134,1092,550,1094,1095,118,119,120,121,122,123,124,140,986,127,47,504,131,134,664,139,657,885,132,144,1135,139,30,131,45,143,139,149,678,81,140,681,103,140,1135,140,41,622,140,134,690,140,118,144,1140,134,140,118,119,140,134,122,123,124,134,126,127,134,699,700,120,132,140,133,135,140,140,1135,139,654,134,142,143,140,140,147,661,133,149,140,131,152,153,140,669,156,157,140,76,77,78,79,80,147,140,46,589,132,103,139,653,89,90,91,133,139,132,131,140,694,115,664,131,118,119,668,133,122,123,124,82,126,127,138,137,132,25,132,144,140,135,118,118,147,139,134,140,142,143,140,145,146,140,140,794,140,140,152,132,734,48,147,132,738,739,140,741,1135,809,809,139,144,133,131,139,750,83,125,753,139,118,140,813,814,140,816,140,818,126,127,128,129,896,140,134,140,140,135,131,119,683,139,133,141,140,132,140,838,133,28,148,132,144,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,103,139,24,150,103,133,139,84,134,945,25,883,139,885,118,37,38,39,40,133,42,736,126,127,128,134,801,742,50,140,61,135,54,896,841,139,133,141,133,754,62,63,64,133,148,981,131,983,140,132,41,139,74,860,140,139,120,120,833,922,119,133,925,926,133,140,134,88,139,134,92,93,133,140,105,106,107,108,109,110,111,112,113,114,799,140,132,140,120,0,140,140,0,1128,1053,1031,125,872,663,39,278,973,283,125,126,136,128,129,681,1100,132,41,103,888,136,75,49,169,981,726,142,143,1026,432,115,988,716,118,119,721,240,122,123,124,384,126,127,447,951,1036,632,132,41,1006,135,809,1131,- 1,139,956,365,142,143,- 1,145,146,- 1,1020,- 1,- 1,- 1,152,- 1,- 1,- 1,- 1,- 1,- 1,1031,- 1,- 1,1034,- 1,103,- 1,1111,- 1,- 1,- 1,894,- 1,1053,897,- 1,- 1,115,- 1,- 1,118,119,- 1,- 1,122,123,124,- 1,126,127,- 1,- 1,915,- 1,132,- 1,103,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,115,932,- 1,118,119,- 1,152,122,123,124,- 1,126,127,- 1,- 1,- 1,- 1,132,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,152,- 1,- 1,1128,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,987,- 1,989,990,991,992,993,994,995,996,997,998,999,1000,1001,1002,1003,1004,1005,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,1062,- 1,- 1,- 1,49,50,51,- 1,- 1,54,55,56,57,58,59,60,- 1,62,63,64,65,- 1,- 1,- 1,69,- 1,- 1,72,73,74,75,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,95,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,1134,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,- 1,- 1,- 1,- 1,49,50,51,- 1,- 1,54,55,56,57,58,59,60,- 1,62,63,64,65,- 1,- 1,- 1,69,- 1,- 1,72,73,74,75,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,95,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,133,- 1,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,- 1,- 1,- 1,- 1,49,50,51,- 1,- 1,54,55,56,57,58,59,60,- 1,62,63,64,65,- 1,- 1,- 1,69,- 1,- 1,72,73,74,75,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,95,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,27,- 1,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,- 1,- 1,- 1,- 1,49,50,51,- 1,- 1,54,55,56,57,58,59,60,- 1,62,63,64,65,- 1,- 1,- 1,69,- 1,- 1,72,73,74,75,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,95,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,119,120,121,122,123,124,125,126,127,128,129,130,131,132,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,41,42,43,- 1,- 1,- 1,- 1,- 1,49,50,- 1,- 1,- 1,54,55,56,57,58,59,60,- 1,62,63,64,65,- 1,- 1,- 1,69,- 1,- 1,72,73,74,75,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,95,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,119,120,121,122,123,124,125,126,127,128,129,130,131,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,- 1,- 1,54,- 1,56,57,58,59,60,- 1,62,63,- 1,- 1,- 1,- 1,- 1,69,- 1,- 1,72,73,74,75,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,95,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,119,120,121,122,123,124,125,126,127,128,129,130,- 1,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,54,55,56,57,58,59,60,- 1,62,63,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,74,75,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,95,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,119,120,121,122,123,124,125,126,127,128,129,130,- 1,132,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,25,26,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,54,55,56,57,58,59,60,- 1,62,63,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,74,75,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,95,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,119,120,121,122,123,124,125,126,127,128,129,130,- 1,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,0,1,- 1,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,- 1,42,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,52,53,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,63,64,- 1,- 1,67,68,- 1,70,71,- 1,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,- 1,- 1,- 1,- 1,- 1,- 1,125,126,127,128,129,- 1,- 1,- 1,133,- 1,135,- 1,- 1,- 1,139,- 1,141,142,143,- 1,- 1,0,1,148,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,41,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,- 1,42,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,52,53,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,63,64,- 1,- 1,67,68,- 1,70,71,- 1,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,103,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,- 1,- 1,- 1,- 1,118,119,- 1,- 1,122,123,124,- 1,126,127,- 1,- 1,- 1,- 1,132,- 1,- 1,135,- 1,118,- 1,139,- 1,- 1,142,143,125,126,127,128,129,- 1,- 1,- 1,152,- 1,135,- 1,- 1,- 1,139,- 1,141,142,143,- 1,- 1,- 1,1,148,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,- 1,42,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,52,53,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,63,64,- 1,- 1,67,68,- 1,70,71,- 1,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,- 1,41,- 1,- 1,- 1,- 1,125,126,127,128,129,- 1,- 1,- 1,133,- 1,135,- 1,- 1,- 1,139,- 1,141,142,143,- 1,- 1,- 1,- 1,148,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,103,37,38,39,40,- 1,42,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,118,119,- 1,54,122,123,124,- 1,126,127,- 1,62,63,64,132,- 1,- 1,135,- 1,- 1,- 1,139,- 1,74,142,143,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,152,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,125,126,- 1,128,129,- 1,- 1,132,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,115,- 1,- 1,118,- 1,- 1,- 1,- 1,- 1,- 1,125,126,127,128,129,- 1,131,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,141,142,143,- 1,- 1,146,- 1,148,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,24,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,40,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,50,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,74,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,125,126,- 1,128,129,- 1,- 1,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,142,143,74,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,115,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,125,126,- 1,128,129,- 1,131,62,63,- 1,135,- 1,- 1,- 1,139,- 1,141,142,143,74,- 1,146,- 1,148,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,118,- 1,- 1,- 1,- 1,- 1,- 1,125,126,127,128,129,37,38,39,- 1,- 1,135,- 1,- 1,- 1,139,- 1,141,142,143,- 1,- 1,- 1,54,148,- 1,- 1,- 1,- 1,- 1,- 1,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,74,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,115,- 1,- 1,118,- 1,- 1,- 1,- 1,- 1,54,125,126,- 1,128,129,- 1,131,62,63,- 1,- 1,66,- 1,- 1,- 1,- 1,- 1,142,143,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,125,126,- 1,128,129,- 1,- 1,132,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,74,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,115,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,125,126,- 1,128,129,- 1,131,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,142,143,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,125,126,- 1,128,129,37,38,39,133,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,142,143,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,74,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,125,126,- 1,128,129,37,38,39,133,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,142,143,- 1,- 1,- 1,54,- 1,- 1,- 1,- 1,- 1,- 1,- 1,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,74,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,115,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,125,126,- 1,128,129,- 1,- 1,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,142,143,74,- 1,- 1,- 1,8,9,10,11,12,13,14,15,16,17,18,19,20,21,92,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,37,38,39,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,54,125,126,- 1,128,129,- 1,- 1,62,63,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,142,143,74,- 1,- 1,- 1,- 1,- 1,25,26,27,- 1,29,30,31,32,33,34,35,36,92,93,- 1,40,41,- 1,43,44,- 1,- 1,- 1,- 1,49,- 1,51,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,125,- 1,72,73,129,75,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,142,143,- 1,- 1,- 1,- 1,- 1,- 1,95,96,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,131,132,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,25,26,- 1,- 1,- 1,149,- 1,32,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,126,127,- 1,- 1,130,- 1,132,133,- 1,135,- 1,95,96,139,- 1,- 1,- 1,143,- 1,145,146,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,126,127,- 1,- 1,130,- 1,132,133,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,- 1,145,146,25,26,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,126,127,- 1,- 1,130,- 1,132,133,- 1,135,- 1,95,96,139,- 1,- 1,- 1,143,- 1,145,146,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,126,127,- 1,- 1,130,- 1,132,133,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,- 1,145,146,25,26,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,126,127,- 1,- 1,130,- 1,132,- 1,- 1,135,- 1,95,96,139,- 1,- 1,- 1,143,- 1,145,146,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,25,26,- 1,147,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,132,- 1,- 1,135,- 1,95,96,139,- 1,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,131,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,25,26,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,95,96,139,140,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,25,26,- 1,147,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,95,96,139,140,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,25,26,- 1,147,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,95,96,139,- 1,- 1,- 1,143,- 1,- 1,- 1,147,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,131,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,25,26,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,131,- 1,- 1,- 1,135,- 1,95,96,139,- 1,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,- 1,- 1,139,140,- 1,- 1,143,25,26,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,95,96,139,140,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,131,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,25,26,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,95,96,139,140,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,- 1,- 1,139,140,- 1,- 1,143,25,26,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,95,96,139,140,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,- 1,- 1,139,140,- 1,- 1,143,25,26,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,- 1,- 1,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,25,26,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,- 1,40,41,- 1,43,- 1,- 1,- 1,- 1,- 1,49,- 1,- 1,- 1,95,96,55,56,57,58,59,60,- 1,- 1,- 1,- 1,65,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,95,96,139,- 1,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,25,26,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,- 1,- 1,40,41,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,- 1,- 1,56,57,58,59,60,- 1,- 1,- 1,- 1,- 1,25,26,- 1,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,40,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,95,96,56,57,58,59,60,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,132,- 1,- 1,135,95,96,- 1,139,- 1,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,132,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,25,26,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,40,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,56,57,58,59,60,- 1,- 1,- 1,- 1,- 1,25,26,- 1,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,40,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,95,96,56,57,58,59,60,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,95,96,- 1,139,- 1,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,25,26,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,- 1,- 1,40,41,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,- 1,- 1,- 1,56,57,58,59,60,- 1,- 1,- 1,- 1,- 1,25,26,- 1,69,- 1,- 1,72,73,- 1,75,- 1,- 1,- 1,- 1,- 1,40,41,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,95,96,56,57,58,59,60,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,69,- 1,- 1,72,73,- 1,75,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,95,96,- 1,139,- 1,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157,- 1,118,119,120,121,122,123,124,- 1,- 1,127,- 1,- 1,130,- 1,- 1,- 1,- 1,135,- 1,- 1,- 1,139,- 1,- 1,- 1,143,- 1,- 1,- 1,- 1,- 1,149,- 1,- 1,152,153,- 1,- 1,156,157};char Cyc_Yystack_overflow[17U]="Yystack_overflow";struct Cyc_Yystack_overflow_exn_struct{char*tag;int f1;};
# 45 "cycbison.simple"
struct Cyc_Yystack_overflow_exn_struct Cyc_Yystack_overflow_val={Cyc_Yystack_overflow,0};
# 72 "cycbison.simple"
void Cyc_yyerror(struct _fat_ptr,int state,int token);
# 82 "cycbison.simple"
int Cyc_yylex(struct Cyc_Lexing_lexbuf*,union Cyc_YYSTYPE*yylval_ptr,struct Cyc_Yyltype*yylloc);struct Cyc_Yystacktype{union Cyc_YYSTYPE v;struct Cyc_Yyltype l;};struct _tuple31{unsigned f1;struct _tuple0*f2;int f3;};struct _tuple32{void*f1;void*f2;};struct _tuple33{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 145 "cycbison.simple"
int Cyc_yyparse(struct _RegionHandle*yyr,struct Cyc_Lexing_lexbuf*yylex_buf){
# 148
struct _RegionHandle _tmp45E=_new_region("yyregion");struct _RegionHandle*yyregion=& _tmp45E;_push_region(yyregion);
{int yystate;
int yyn=0;
int yyerrstatus;
int yychar1=0;
# 154
int yychar;
union Cyc_YYSTYPE yylval=({union Cyc_YYSTYPE _tmp101E;(_tmp101E.YYINITIALSVAL).tag=75U,(_tmp101E.YYINITIALSVAL).val=0;_tmp101E;});
int yynerrs;
# 158
struct Cyc_Yyltype yylloc;
# 162
int yyssp_offset;
# 164
struct _fat_ptr yyss=({unsigned _tmpF57=200U;_tag_fat(_region_calloc(yyregion,sizeof(short),_tmpF57),sizeof(short),_tmpF57);});
# 166
int yyvsp_offset;
# 168
struct _fat_ptr yyvs=
_tag_fat(({unsigned _tmpF56=200U;struct Cyc_Yystacktype*_tmpF55=({struct _RegionHandle*_tmp10CF=yyregion;_region_malloc(_tmp10CF,_check_times(_tmpF56,sizeof(struct Cyc_Yystacktype)));});({{unsigned _tmp101D=200U;unsigned i;for(i=0;i < _tmp101D;++ i){(_tmpF55[i]).v=yylval,({struct Cyc_Yyltype _tmp10D0=({Cyc_yynewloc();});(_tmpF55[i]).l=_tmp10D0;});}}0;});_tmpF55;}),sizeof(struct Cyc_Yystacktype),200U);
# 174
struct Cyc_Yystacktype*yyyvsp;
# 177
int yystacksize=200;
# 179
union Cyc_YYSTYPE yyval=yylval;
# 183
int yylen;
# 190
yystate=0;
yyerrstatus=0;
yynerrs=0;
yychar=-2;
# 200
yyssp_offset=-1;
yyvsp_offset=0;
# 206
yynewstate:
# 208
*((short*)_check_fat_subscript(yyss,sizeof(short),++ yyssp_offset))=(short)yystate;
# 210
if(yyssp_offset >= (yystacksize - 1)- 12){
# 212
if(yystacksize >= 10000){
({({struct _fat_ptr _tmp10D2=({const char*_tmp45F="parser stack overflow";_tag_fat(_tmp45F,sizeof(char),22U);});int _tmp10D1=yystate;Cyc_yyerror(_tmp10D2,_tmp10D1,yychar);});});
(int)_throw(& Cyc_Yystack_overflow_val);}
# 212
yystacksize *=2;
# 217
if(yystacksize > 10000)
yystacksize=10000;{
# 217
struct _fat_ptr yyss1=({unsigned _tmp463=(unsigned)yystacksize;short*_tmp462=({struct _RegionHandle*_tmp10D3=yyregion;_region_malloc(_tmp10D3,_check_times(_tmp463,sizeof(short)));});({{unsigned _tmpFD9=(unsigned)yystacksize;unsigned i;for(i=0;i < _tmpFD9;++ i){
# 220
i <= (unsigned)yyssp_offset?_tmp462[i]=*((short*)_check_fat_subscript(yyss,sizeof(short),(int)i)):(_tmp462[i]=0);}}0;});_tag_fat(_tmp462,sizeof(short),_tmp463);});
# 222
struct _fat_ptr yyvs1=({unsigned _tmp461=(unsigned)yystacksize;struct Cyc_Yystacktype*_tmp460=({struct _RegionHandle*_tmp10D4=yyregion;_region_malloc(_tmp10D4,_check_times(_tmp461,sizeof(struct Cyc_Yystacktype)));});({{unsigned _tmpFD8=(unsigned)yystacksize;unsigned i;for(i=0;i < _tmpFD8;++ i){
# 224
i <= (unsigned)yyssp_offset?_tmp460[i]=*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),(int)i)):(_tmp460[i]=*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),0)));}}0;});_tag_fat(_tmp460,sizeof(struct Cyc_Yystacktype),_tmp461);});
# 230
yyss=yyss1;
yyvs=yyvs1;}}
# 210
goto yybackup;
# 242
yybackup:
# 254 "cycbison.simple"
 yyn=(int)*((short*)_check_known_subscript_notnull(Cyc_yypact,1198U,sizeof(short),yystate));
if(yyn == -32768)goto yydefault;if(yychar == -2)
# 267
yychar=({Cyc_yylex(yylex_buf,& yylval,& yylloc);});
# 255
if(yychar <= 0){
# 273
yychar1=0;
yychar=0;}else{
# 282
yychar1=yychar > 0 && yychar <= 385?(int)*((short*)_check_known_subscript_notnull(Cyc_yytranslate,386U,sizeof(short),yychar)): 326;}
# 299 "cycbison.simple"
yyn +=yychar1;
if((yyn < 0 || yyn > 6937)||(int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,6938U,sizeof(short),yyn))!= yychar1)goto yydefault;yyn=(int)Cyc_yytable[yyn];
# 309
if(yyn < 0){
# 311
if(yyn == -32768)goto yyerrlab;yyn=- yyn;
# 313
goto yyreduce;}else{
# 315
if(yyn == 0)goto yyerrlab;}
# 309
if(yyn == 1197){
# 318
int _tmp464=0;_npop_handler(0U);return _tmp464;}
# 309
if(yychar != 0)
# 329 "cycbison.simple"
yychar=-2;
# 309 "cycbison.simple"
({struct Cyc_Yystacktype _tmp10D5=({struct Cyc_Yystacktype _tmpFDA;_tmpFDA.v=yylval,_tmpFDA.l=yylloc;_tmpFDA;});*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),++ yyvsp_offset))=_tmp10D5;});
# 338 "cycbison.simple"
if(yyerrstatus != 0)-- yyerrstatus;yystate=yyn;
# 341
goto yynewstate;
# 344
yydefault:
# 346
 yyn=(int)Cyc_yydefact[yystate];
if(yyn == 0)goto yyerrlab;yyreduce:
# 353
 yylen=(int)*((short*)_check_known_subscript_notnull(Cyc_yyr2,577U,sizeof(short),yyn));
yyyvsp=(struct Cyc_Yystacktype*)_check_null(_untag_fat_ptr(_fat_ptr_plus(yyvs,sizeof(struct Cyc_Yystacktype),(yyvsp_offset + 1)- yylen),sizeof(struct Cyc_Yystacktype),12U));
if(yylen > 0)
yyval=(yyyvsp[0]).v;
# 355
{int _tmp465=yyn;switch(_tmp465){case 1U: _LL1: _LL2:
# 1225 "parse.y"
 yyval=(yyyvsp[0]).v;
Cyc_Parse_parse_result=({Cyc_yyget_YY16(&(yyyvsp[0]).v);});
# 1228
goto _LL0;case 2U: _LL3: _LL4:
# 1231 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp466)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp467=({struct Cyc_List_List*(*_tmp468)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp469=({Cyc_yyget_YY16(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp46A=({Cyc_yyget_YY16(&(yyyvsp[1]).v);});_tmp468(_tmp469,_tmp46A);});_tmp466(_tmp467);});
goto _LL0;case 3U: _LL5: _LL6:
# 1235 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp46B)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp46C=({struct Cyc_List_List*_tmp46F=_cycalloc(sizeof(*_tmp46F));({struct Cyc_Absyn_Decl*_tmp10DA=({struct Cyc_Absyn_Decl*_tmp46E=_cycalloc(sizeof(*_tmp46E));({void*_tmp10D9=(void*)({struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_tmp46D=_cycalloc(sizeof(*_tmp46D));_tmp46D->tag=10U,({struct _tuple0*_tmp10D8=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp46D->f1=_tmp10D8;}),({struct Cyc_List_List*_tmp10D7=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp46D->f2=_tmp10D7;});_tmp46D;});_tmp46E->r=_tmp10D9;}),({unsigned _tmp10D6=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp46E->loc=_tmp10D6;});_tmp46E;});_tmp46F->hd=_tmp10DA;}),_tmp46F->tl=0;_tmp46F;});_tmp46B(_tmp46C);});
({Cyc_Lex_leave_using();});
# 1238
goto _LL0;case 4U: _LL7: _LL8:
# 1239 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp470)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp471=({struct Cyc_List_List*_tmp474=_cycalloc(sizeof(*_tmp474));({struct Cyc_Absyn_Decl*_tmp10E0=({struct Cyc_Absyn_Decl*_tmp473=_cycalloc(sizeof(*_tmp473));({void*_tmp10DF=(void*)({struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*_tmp472=_cycalloc(sizeof(*_tmp472));_tmp472->tag=10U,({struct _tuple0*_tmp10DE=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp472->f1=_tmp10DE;}),({struct Cyc_List_List*_tmp10DD=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp472->f2=_tmp10DD;});_tmp472;});_tmp473->r=_tmp10DF;}),({unsigned _tmp10DC=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp473->loc=_tmp10DC;});_tmp473;});_tmp474->hd=_tmp10E0;}),({struct Cyc_List_List*_tmp10DB=({Cyc_yyget_YY16(&(yyyvsp[4]).v);});_tmp474->tl=_tmp10DB;});_tmp474;});_tmp470(_tmp471);});
goto _LL0;case 5U: _LL9: _LLA:
# 1242
 yyval=({union Cyc_YYSTYPE(*_tmp475)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp476=({struct Cyc_List_List*_tmp47A=_cycalloc(sizeof(*_tmp47A));({struct Cyc_Absyn_Decl*_tmp10E6=({struct Cyc_Absyn_Decl*_tmp479=_cycalloc(sizeof(*_tmp479));({void*_tmp10E5=(void*)({struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_tmp478=_cycalloc(sizeof(*_tmp478));_tmp478->tag=9U,({struct _fat_ptr*_tmp10E4=({struct _fat_ptr*_tmp477=_cycalloc(sizeof(*_tmp477));({struct _fat_ptr _tmp10E3=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmp477=_tmp10E3;});_tmp477;});_tmp478->f1=_tmp10E4;}),({struct Cyc_List_List*_tmp10E2=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp478->f2=_tmp10E2;});_tmp478;});_tmp479->r=_tmp10E5;}),({unsigned _tmp10E1=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp479->loc=_tmp10E1;});_tmp479;});_tmp47A->hd=_tmp10E6;}),_tmp47A->tl=0;_tmp47A;});_tmp475(_tmp476);});
({Cyc_Lex_leave_namespace();});
# 1245
goto _LL0;case 6U: _LLB: _LLC:
# 1246 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp47B)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp47C=({struct Cyc_List_List*_tmp480=_cycalloc(sizeof(*_tmp480));({struct Cyc_Absyn_Decl*_tmp10ED=({struct Cyc_Absyn_Decl*_tmp47F=_cycalloc(sizeof(*_tmp47F));({void*_tmp10EC=(void*)({struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*_tmp47E=_cycalloc(sizeof(*_tmp47E));_tmp47E->tag=9U,({struct _fat_ptr*_tmp10EB=({struct _fat_ptr*_tmp47D=_cycalloc(sizeof(*_tmp47D));({struct _fat_ptr _tmp10EA=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmp47D=_tmp10EA;});_tmp47D;});_tmp47E->f1=_tmp10EB;}),({struct Cyc_List_List*_tmp10E9=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp47E->f2=_tmp10E9;});_tmp47E;});_tmp47F->r=_tmp10EC;}),({unsigned _tmp10E8=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp47F->loc=_tmp10E8;});_tmp47F;});_tmp480->hd=_tmp10ED;}),({struct Cyc_List_List*_tmp10E7=({Cyc_yyget_YY16(&(yyyvsp[4]).v);});_tmp480->tl=_tmp10E7;});_tmp480;});_tmp47B(_tmp47C);});
goto _LL0;case 7U: _LLD: _LLE: {
# 1248 "parse.y"
int is_c_include=({Cyc_yyget_YY31(&(yyyvsp[0]).v);});
struct Cyc_List_List*cycdecls=({Cyc_yyget_YY16(&(yyyvsp[4]).v);});
struct _tuple28*_stmttmp17=({Cyc_yyget_YY53(&(yyyvsp[5]).v);});unsigned _tmp482;struct Cyc_List_List*_tmp481;_LL480: _tmp481=_stmttmp17->f1;_tmp482=_stmttmp17->f2;_LL481: {struct Cyc_List_List*exs=_tmp481;unsigned wc=_tmp482;
struct Cyc_List_List*hides=({Cyc_yyget_YY54(&(yyyvsp[6]).v);});
if(exs != 0 && hides != 0)
({void*_tmp483=0U;({unsigned _tmp10EF=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct _fat_ptr _tmp10EE=({const char*_tmp484="hide list can only be used with export { * }, or no export block";_tag_fat(_tmp484,sizeof(char),65U);});Cyc_Warn_err(_tmp10EF,_tmp10EE,_tag_fat(_tmp483,sizeof(void*),0U));});});
# 1252
if(
# 1255
(unsigned)hides && !((int)wc))
wc=(unsigned)((yyyvsp[6]).l).first_line;
# 1252
if(!is_c_include){
# 1259
if(exs != 0 || cycdecls != 0){
({void*_tmp485=0U;({unsigned _tmp10F1=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct _fat_ptr _tmp10F0=({const char*_tmp486="expecting \"C include\"";_tag_fat(_tmp486,sizeof(char),22U);});Cyc_Warn_err(_tmp10F1,_tmp10F0,_tag_fat(_tmp485,sizeof(void*),0U));});});
yyval=({union Cyc_YYSTYPE(*_tmp487)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp488=({struct Cyc_List_List*_tmp48C=_cycalloc(sizeof(*_tmp48C));({struct Cyc_Absyn_Decl*_tmp10F7=({struct Cyc_Absyn_Decl*_tmp48B=_cycalloc(sizeof(*_tmp48B));({void*_tmp10F6=(void*)({struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*_tmp48A=_cycalloc(sizeof(*_tmp48A));_tmp48A->tag=12U,({struct Cyc_List_List*_tmp10F5=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp48A->f1=_tmp10F5;}),_tmp48A->f2=cycdecls,_tmp48A->f3=exs,({struct _tuple11*_tmp10F4=({struct _tuple11*_tmp489=_cycalloc(sizeof(*_tmp489));_tmp489->f1=wc,_tmp489->f2=hides;_tmp489;});_tmp48A->f4=_tmp10F4;});_tmp48A;});_tmp48B->r=_tmp10F6;}),({unsigned _tmp10F3=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp48B->loc=_tmp10F3;});_tmp48B;});_tmp48C->hd=_tmp10F7;}),({struct Cyc_List_List*_tmp10F2=({Cyc_yyget_YY16(&(yyyvsp[7]).v);});_tmp48C->tl=_tmp10F2;});_tmp48C;});_tmp487(_tmp488);});}else{
# 1264
yyval=({union Cyc_YYSTYPE(*_tmp48D)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp48E=({struct Cyc_List_List*_tmp491=_cycalloc(sizeof(*_tmp491));({struct Cyc_Absyn_Decl*_tmp10FC=({struct Cyc_Absyn_Decl*_tmp490=_cycalloc(sizeof(*_tmp490));({void*_tmp10FB=(void*)({struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*_tmp48F=_cycalloc(sizeof(*_tmp48F));_tmp48F->tag=11U,({struct Cyc_List_List*_tmp10FA=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp48F->f1=_tmp10FA;});_tmp48F;});_tmp490->r=_tmp10FB;}),({unsigned _tmp10F9=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp490->loc=_tmp10F9;});_tmp490;});_tmp491->hd=_tmp10FC;}),({struct Cyc_List_List*_tmp10F8=({Cyc_yyget_YY16(&(yyyvsp[7]).v);});_tmp491->tl=_tmp10F8;});_tmp491;});_tmp48D(_tmp48E);});}}else{
# 1268
yyval=({union Cyc_YYSTYPE(*_tmp492)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp493=({struct Cyc_List_List*_tmp497=_cycalloc(sizeof(*_tmp497));({struct Cyc_Absyn_Decl*_tmp1102=({struct Cyc_Absyn_Decl*_tmp496=_cycalloc(sizeof(*_tmp496));({void*_tmp1101=(void*)({struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*_tmp495=_cycalloc(sizeof(*_tmp495));_tmp495->tag=12U,({struct Cyc_List_List*_tmp1100=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp495->f1=_tmp1100;}),_tmp495->f2=cycdecls,_tmp495->f3=exs,({struct _tuple11*_tmp10FF=({struct _tuple11*_tmp494=_cycalloc(sizeof(*_tmp494));_tmp494->f1=wc,_tmp494->f2=hides;_tmp494;});_tmp495->f4=_tmp10FF;});_tmp495;});_tmp496->r=_tmp1101;}),({unsigned _tmp10FE=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp496->loc=_tmp10FE;});_tmp496;});_tmp497->hd=_tmp1102;}),({struct Cyc_List_List*_tmp10FD=({Cyc_yyget_YY16(&(yyyvsp[7]).v);});_tmp497->tl=_tmp10FD;});_tmp497;});_tmp492(_tmp493);});}
# 1271
goto _LL0;}}case 8U: _LLF: _LL10:
# 1272 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp498)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp499=({struct Cyc_List_List*_tmp49B=_cycalloc(sizeof(*_tmp49B));({struct Cyc_Absyn_Decl*_tmp1104=({struct Cyc_Absyn_Decl*_tmp49A=_cycalloc(sizeof(*_tmp49A));_tmp49A->r=(void*)& Cyc_Absyn_Porton_d_val,_tmp49A->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp49A;});_tmp49B->hd=_tmp1104;}),({struct Cyc_List_List*_tmp1103=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp49B->tl=_tmp1103;});_tmp49B;});_tmp498(_tmp499);});
goto _LL0;case 9U: _LL11: _LL12:
# 1274 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp49C)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp49D=({struct Cyc_List_List*_tmp49F=_cycalloc(sizeof(*_tmp49F));({struct Cyc_Absyn_Decl*_tmp1106=({struct Cyc_Absyn_Decl*_tmp49E=_cycalloc(sizeof(*_tmp49E));_tmp49E->r=(void*)& Cyc_Absyn_Portoff_d_val,_tmp49E->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp49E;});_tmp49F->hd=_tmp1106;}),({struct Cyc_List_List*_tmp1105=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp49F->tl=_tmp1105;});_tmp49F;});_tmp49C(_tmp49D);});
goto _LL0;case 10U: _LL13: _LL14:
# 1276 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4A0)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp4A1=({struct Cyc_List_List*_tmp4A3=_cycalloc(sizeof(*_tmp4A3));({struct Cyc_Absyn_Decl*_tmp1108=({struct Cyc_Absyn_Decl*_tmp4A2=_cycalloc(sizeof(*_tmp4A2));_tmp4A2->r=(void*)& Cyc_Absyn_Tempeston_d_val,_tmp4A2->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp4A2;});_tmp4A3->hd=_tmp1108;}),({struct Cyc_List_List*_tmp1107=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp4A3->tl=_tmp1107;});_tmp4A3;});_tmp4A0(_tmp4A1);});
goto _LL0;case 11U: _LL15: _LL16:
# 1278 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4A4)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp4A5=({struct Cyc_List_List*_tmp4A7=_cycalloc(sizeof(*_tmp4A7));({struct Cyc_Absyn_Decl*_tmp110A=({struct Cyc_Absyn_Decl*_tmp4A6=_cycalloc(sizeof(*_tmp4A6));_tmp4A6->r=(void*)& Cyc_Absyn_Tempestoff_d_val,_tmp4A6->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp4A6;});_tmp4A7->hd=_tmp110A;}),({struct Cyc_List_List*_tmp1109=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});_tmp4A7->tl=_tmp1109;});_tmp4A7;});_tmp4A4(_tmp4A5);});
goto _LL0;case 12U: _LL17: _LL18:
# 1279 "parse.y"
 yyval=({Cyc_YY16(0);});
goto _LL0;case 13U: _LL19: _LL1A:
# 1284 "parse.y"
 Cyc_Parse_parsing_tempest=1;
goto _LL0;case 14U: _LL1B: _LL1C:
# 1289 "parse.y"
 Cyc_Parse_parsing_tempest=0;
goto _LL0;case 15U: _LL1D: _LL1E: {
# 1294 "parse.y"
struct _fat_ptr two=({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});
({Cyc_Lex_enter_extern_c();});
if(({({struct _fat_ptr _tmp110B=(struct _fat_ptr)two;Cyc_strcmp(_tmp110B,({const char*_tmp4A8="C";_tag_fat(_tmp4A8,sizeof(char),2U);}));});})== 0)
yyval=({Cyc_YY31(0);});else{
if(({({struct _fat_ptr _tmp110C=(struct _fat_ptr)two;Cyc_strcmp(_tmp110C,({const char*_tmp4A9="C include";_tag_fat(_tmp4A9,sizeof(char),10U);}));});})== 0)
yyval=({Cyc_YY31(1);});else{
# 1301
({void*_tmp4AA=0U;({unsigned _tmp110E=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct _fat_ptr _tmp110D=({const char*_tmp4AB="expecting \"C\" or \"C include\"";_tag_fat(_tmp4AB,sizeof(char),29U);});Cyc_Warn_err(_tmp110E,_tmp110D,_tag_fat(_tmp4AA,sizeof(void*),0U));});});
yyval=({Cyc_YY31(1);});}}
# 1305
goto _LL0;}case 16U: _LL1F: _LL20:
# 1308 "parse.y"
({Cyc_Lex_leave_extern_c();});
goto _LL0;case 17U: _LL21: _LL22:
# 1312 "parse.y"
 yyval=({Cyc_YY54(0);});
goto _LL0;case 18U: _LL23: _LL24:
# 1313 "parse.y"
 yyval=(yyyvsp[2]).v;
goto _LL0;case 19U: _LL25: _LL26:
# 1317 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4AC)(struct Cyc_List_List*yy1)=Cyc_YY54;struct Cyc_List_List*_tmp4AD=({struct Cyc_List_List*_tmp4AE=_cycalloc(sizeof(*_tmp4AE));({struct _tuple0*_tmp110F=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp4AE->hd=_tmp110F;}),_tmp4AE->tl=0;_tmp4AE;});_tmp4AC(_tmp4AD);});
goto _LL0;case 20U: _LL27: _LL28:
# 1318 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4AF)(struct Cyc_List_List*yy1)=Cyc_YY54;struct Cyc_List_List*_tmp4B0=({struct Cyc_List_List*_tmp4B1=_cycalloc(sizeof(*_tmp4B1));({struct _tuple0*_tmp1110=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp4B1->hd=_tmp1110;}),_tmp4B1->tl=0;_tmp4B1;});_tmp4AF(_tmp4B0);});
goto _LL0;case 21U: _LL29: _LL2A:
# 1320 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4B2)(struct Cyc_List_List*yy1)=Cyc_YY54;struct Cyc_List_List*_tmp4B3=({struct Cyc_List_List*_tmp4B4=_cycalloc(sizeof(*_tmp4B4));({struct _tuple0*_tmp1112=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp4B4->hd=_tmp1112;}),({struct Cyc_List_List*_tmp1111=({Cyc_yyget_YY54(&(yyyvsp[2]).v);});_tmp4B4->tl=_tmp1111;});_tmp4B4;});_tmp4B2(_tmp4B3);});
goto _LL0;case 22U: _LL2B: _LL2C:
# 1324 "parse.y"
 yyval=({Cyc_YY53(({struct _tuple28*_tmp4B5=_cycalloc(sizeof(*_tmp4B5));_tmp4B5->f1=0,_tmp4B5->f2=0U;_tmp4B5;}));});
goto _LL0;case 23U: _LL2D: _LL2E:
# 1325 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 24U: _LL2F: _LL30:
# 1329 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4B6)(struct _tuple28*yy1)=Cyc_YY53;struct _tuple28*_tmp4B7=({struct _tuple28*_tmp4B8=_cycalloc(sizeof(*_tmp4B8));({struct Cyc_List_List*_tmp1113=({Cyc_yyget_YY52(&(yyyvsp[2]).v);});_tmp4B8->f1=_tmp1113;}),_tmp4B8->f2=0U;_tmp4B8;});_tmp4B6(_tmp4B7);});
goto _LL0;case 25U: _LL31: _LL32:
# 1330 "parse.y"
 yyval=({Cyc_YY53(({struct _tuple28*_tmp4B9=_cycalloc(sizeof(*_tmp4B9));_tmp4B9->f1=0,_tmp4B9->f2=0U;_tmp4B9;}));});
goto _LL0;case 26U: _LL33: _LL34:
# 1331 "parse.y"
 yyval=({Cyc_YY53(({struct _tuple28*_tmp4BA=_cycalloc(sizeof(*_tmp4BA));_tmp4BA->f1=0,_tmp4BA->f2=(unsigned)((yyyvsp[0]).l).first_line;_tmp4BA;}));});
goto _LL0;case 27U: _LL35: _LL36:
# 1335 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4BB)(struct Cyc_List_List*yy1)=Cyc_YY52;struct Cyc_List_List*_tmp4BC=({struct Cyc_List_List*_tmp4BE=_cycalloc(sizeof(*_tmp4BE));({struct _tuple31*_tmp1115=({struct _tuple31*_tmp4BD=_cycalloc(sizeof(*_tmp4BD));_tmp4BD->f1=(unsigned)((yyyvsp[0]).l).first_line,({struct _tuple0*_tmp1114=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp4BD->f2=_tmp1114;}),_tmp4BD->f3=0;_tmp4BD;});_tmp4BE->hd=_tmp1115;}),_tmp4BE->tl=0;_tmp4BE;});_tmp4BB(_tmp4BC);});
goto _LL0;case 28U: _LL37: _LL38:
# 1336 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4BF)(struct Cyc_List_List*yy1)=Cyc_YY52;struct Cyc_List_List*_tmp4C0=({struct Cyc_List_List*_tmp4C2=_cycalloc(sizeof(*_tmp4C2));({struct _tuple31*_tmp1117=({struct _tuple31*_tmp4C1=_cycalloc(sizeof(*_tmp4C1));_tmp4C1->f1=(unsigned)((yyyvsp[0]).l).first_line,({struct _tuple0*_tmp1116=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp4C1->f2=_tmp1116;}),_tmp4C1->f3=0;_tmp4C1;});_tmp4C2->hd=_tmp1117;}),_tmp4C2->tl=0;_tmp4C2;});_tmp4BF(_tmp4C0);});
goto _LL0;case 29U: _LL39: _LL3A:
# 1338 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4C3)(struct Cyc_List_List*yy1)=Cyc_YY52;struct Cyc_List_List*_tmp4C4=({struct Cyc_List_List*_tmp4C6=_cycalloc(sizeof(*_tmp4C6));({struct _tuple31*_tmp111A=({struct _tuple31*_tmp4C5=_cycalloc(sizeof(*_tmp4C5));_tmp4C5->f1=(unsigned)((yyyvsp[0]).l).first_line,({struct _tuple0*_tmp1119=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp4C5->f2=_tmp1119;}),_tmp4C5->f3=0;_tmp4C5;});_tmp4C6->hd=_tmp111A;}),({struct Cyc_List_List*_tmp1118=({Cyc_yyget_YY52(&(yyyvsp[2]).v);});_tmp4C6->tl=_tmp1118;});_tmp4C6;});_tmp4C3(_tmp4C4);});
goto _LL0;case 30U: _LL3B: _LL3C:
# 1342 "parse.y"
 yyval=({Cyc_YY16(0);});
goto _LL0;case 31U: _LL3D: _LL3E:
# 1343 "parse.y"
 yyval=(yyyvsp[2]).v;
goto _LL0;case 32U: _LL3F: _LL40: {
# 1348 "parse.y"
struct Cyc_Absyn_Decl*d=({struct Cyc_Absyn_Decl*(*_tmp4C8)(void*,unsigned)=Cyc_Absyn_new_decl;void*_tmp4C9=(void*)({struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmp4CA=_cycalloc(sizeof(*_tmp4CA));_tmp4CA->tag=1U,({struct Cyc_Absyn_Fndecl*_tmp111B=({Cyc_yyget_YY15(&(yyyvsp[0]).v);});_tmp4CA->f1=_tmp111B;});_tmp4CA;});unsigned _tmp4CB=(unsigned)((yyyvsp[0]).l).first_line;_tmp4C8(_tmp4C9,_tmp4CB);});
yyval=({Cyc_YY16(({struct Cyc_List_List*_tmp4C7=_cycalloc(sizeof(*_tmp4C7));_tmp4C7->hd=d,_tmp4C7->tl=0;_tmp4C7;}));});
goto _LL0;}case 33U: _LL41: _LL42:
# 1350 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 34U: _LL43: _LL44:
# 1351 "parse.y"
 yyval=({Cyc_YY16(0);});
goto _LL0;case 37U: _LL45: _LL46:
# 1360 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4CC)(struct Cyc_Absyn_Fndecl*yy1)=Cyc_YY15;struct Cyc_Absyn_Fndecl*_tmp4CD=({struct Cyc_Absyn_Fndecl*(*_tmp4CE)(struct _RegionHandle*yy,struct Cyc_Parse_Declaration_spec*dso,struct Cyc_Parse_Declarator d,struct Cyc_List_List*tds,struct Cyc_Absyn_Stmt*body,unsigned loc)=Cyc_Parse_make_function;struct _RegionHandle*_tmp4CF=yyr;struct Cyc_Parse_Declaration_spec*_tmp4D0=0;struct Cyc_Parse_Declarator _tmp4D1=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp4D2=0;struct Cyc_Absyn_Stmt*_tmp4D3=({Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);});unsigned _tmp4D4=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp4CE(_tmp4CF,_tmp4D0,_tmp4D1,_tmp4D2,_tmp4D3,_tmp4D4);});_tmp4CC(_tmp4CD);});
goto _LL0;case 38U: _LL47: _LL48: {
# 1362 "parse.y"
struct Cyc_Parse_Declaration_spec d=({Cyc_yyget_YY17(&(yyyvsp[0]).v);});
((yyyvsp[0]).l).first_line=((yyyvsp[1]).l).first_line;
yyval=({union Cyc_YYSTYPE(*_tmp4D5)(struct Cyc_Absyn_Fndecl*yy1)=Cyc_YY15;struct Cyc_Absyn_Fndecl*_tmp4D6=({struct Cyc_Absyn_Fndecl*(*_tmp4D7)(struct _RegionHandle*yy,struct Cyc_Parse_Declaration_spec*dso,struct Cyc_Parse_Declarator d,struct Cyc_List_List*tds,struct Cyc_Absyn_Stmt*body,unsigned loc)=Cyc_Parse_make_function;struct _RegionHandle*_tmp4D8=yyr;struct Cyc_Parse_Declaration_spec*_tmp4D9=& d;struct Cyc_Parse_Declarator _tmp4DA=({Cyc_yyget_YY27(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmp4DB=0;struct Cyc_Absyn_Stmt*_tmp4DC=({Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);});unsigned _tmp4DD=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp4D7(_tmp4D8,_tmp4D9,_tmp4DA,_tmp4DB,_tmp4DC,_tmp4DD);});_tmp4D5(_tmp4D6);});
goto _LL0;}case 39U: _LL49: _LL4A:
# 1376 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp4DE)(struct Cyc_Absyn_Fndecl*yy1)=Cyc_YY15;struct Cyc_Absyn_Fndecl*_tmp4DF=({struct Cyc_Absyn_Fndecl*(*_tmp4E0)(struct _RegionHandle*yy,struct Cyc_Parse_Declaration_spec*dso,struct Cyc_Parse_Declarator d,struct Cyc_List_List*tds,struct Cyc_Absyn_Stmt*body,unsigned loc)=Cyc_Parse_make_function;struct _RegionHandle*_tmp4E1=yyr;struct Cyc_Parse_Declaration_spec*_tmp4E2=0;struct Cyc_Parse_Declarator _tmp4E3=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp4E4=({Cyc_yyget_YY16(&(yyyvsp[1]).v);});struct Cyc_Absyn_Stmt*_tmp4E5=({Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);});unsigned _tmp4E6=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp4E0(_tmp4E1,_tmp4E2,_tmp4E3,_tmp4E4,_tmp4E5,_tmp4E6);});_tmp4DE(_tmp4DF);});
goto _LL0;case 40U: _LL4B: _LL4C: {
# 1378 "parse.y"
struct Cyc_Parse_Declaration_spec d=({Cyc_yyget_YY17(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp4E7)(struct Cyc_Absyn_Fndecl*yy1)=Cyc_YY15;struct Cyc_Absyn_Fndecl*_tmp4E8=({struct Cyc_Absyn_Fndecl*(*_tmp4E9)(struct _RegionHandle*yy,struct Cyc_Parse_Declaration_spec*dso,struct Cyc_Parse_Declarator d,struct Cyc_List_List*tds,struct Cyc_Absyn_Stmt*body,unsigned loc)=Cyc_Parse_make_function;struct _RegionHandle*_tmp4EA=yyr;struct Cyc_Parse_Declaration_spec*_tmp4EB=& d;struct Cyc_Parse_Declarator _tmp4EC=({Cyc_yyget_YY27(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmp4ED=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});struct Cyc_Absyn_Stmt*_tmp4EE=({Cyc_yyget_Stmt_tok(&(yyyvsp[3]).v);});unsigned _tmp4EF=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp4E9(_tmp4EA,_tmp4EB,_tmp4EC,_tmp4ED,_tmp4EE,_tmp4EF);});_tmp4E7(_tmp4E8);});
goto _LL0;}case 41U: _LL4D: _LL4E: {
# 1386 "parse.y"
struct Cyc_Parse_Declaration_spec d=({Cyc_yyget_YY17(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp4F0)(struct Cyc_Absyn_Fndecl*yy1)=Cyc_YY15;struct Cyc_Absyn_Fndecl*_tmp4F1=({struct Cyc_Absyn_Fndecl*(*_tmp4F2)(struct _RegionHandle*yy,struct Cyc_Parse_Declaration_spec*dso,struct Cyc_Parse_Declarator d,struct Cyc_List_List*tds,struct Cyc_Absyn_Stmt*body,unsigned loc)=Cyc_Parse_make_function;struct _RegionHandle*_tmp4F3=yyr;struct Cyc_Parse_Declaration_spec*_tmp4F4=& d;struct Cyc_Parse_Declarator _tmp4F5=({Cyc_yyget_YY27(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmp4F6=0;struct Cyc_Absyn_Stmt*_tmp4F7=({Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);});unsigned _tmp4F8=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp4F2(_tmp4F3,_tmp4F4,_tmp4F5,_tmp4F6,_tmp4F7,_tmp4F8);});_tmp4F0(_tmp4F1);});
goto _LL0;}case 42U: _LL4F: _LL50: {
# 1389 "parse.y"
struct Cyc_Parse_Declaration_spec d=({Cyc_yyget_YY17(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp4F9)(struct Cyc_Absyn_Fndecl*yy1)=Cyc_YY15;struct Cyc_Absyn_Fndecl*_tmp4FA=({struct Cyc_Absyn_Fndecl*(*_tmp4FB)(struct _RegionHandle*yy,struct Cyc_Parse_Declaration_spec*dso,struct Cyc_Parse_Declarator d,struct Cyc_List_List*tds,struct Cyc_Absyn_Stmt*body,unsigned loc)=Cyc_Parse_make_function;struct _RegionHandle*_tmp4FC=yyr;struct Cyc_Parse_Declaration_spec*_tmp4FD=& d;struct Cyc_Parse_Declarator _tmp4FE=({Cyc_yyget_YY27(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmp4FF=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});struct Cyc_Absyn_Stmt*_tmp500=({Cyc_yyget_Stmt_tok(&(yyyvsp[3]).v);});unsigned _tmp501=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp4FB(_tmp4FC,_tmp4FD,_tmp4FE,_tmp4FF,_tmp500,_tmp501);});_tmp4F9(_tmp4FA);});
goto _LL0;}case 43U: _LL51: _LL52:
# 1394 "parse.y"
({void(*_tmp502)(struct _tuple0*)=Cyc_Lex_enter_using;struct _tuple0*_tmp503=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});_tmp502(_tmp503);});yyval=(yyyvsp[1]).v;
goto _LL0;case 44U: _LL53: _LL54:
# 1397
({Cyc_Lex_leave_using();});
goto _LL0;case 45U: _LL55: _LL56:
# 1400
({void(*_tmp504)(struct _fat_ptr*)=Cyc_Lex_enter_namespace;struct _fat_ptr*_tmp505=({struct _fat_ptr*_tmp506=_cycalloc(sizeof(*_tmp506));({struct _fat_ptr _tmp111C=({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});*_tmp506=_tmp111C;});_tmp506;});_tmp504(_tmp505);});yyval=(yyyvsp[1]).v;
goto _LL0;case 46U: _LL57: _LL58:
# 1403
({Cyc_Lex_leave_namespace();});
goto _LL0;case 47U: _LL59: _LL5A: {
# 1409 "parse.y"
int location=((yyyvsp[0]).l).first_line;
yyval=({union Cyc_YYSTYPE(*_tmp507)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp508=({struct Cyc_List_List*(*_tmp509)(struct Cyc_Parse_Declaration_spec ds,struct _tuple14*ids,unsigned tqual_loc,unsigned loc)=Cyc_Parse_make_declarations;struct Cyc_Parse_Declaration_spec _tmp50A=({Cyc_yyget_YY17(&(yyyvsp[0]).v);});struct _tuple14*_tmp50B=0;unsigned _tmp50C=(unsigned)location;unsigned _tmp50D=(unsigned)location;_tmp509(_tmp50A,_tmp50B,_tmp50C,_tmp50D);});_tmp507(_tmp508);});
goto _LL0;}case 48U: _LL5B: _LL5C: {
# 1412 "parse.y"
int _tmp50E=({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});int location=_tmp50E;
yyval=({union Cyc_YYSTYPE(*_tmp50F)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp510=({struct Cyc_List_List*(*_tmp511)(struct Cyc_Parse_Declaration_spec ds,struct _tuple14*ids,unsigned tqual_loc,unsigned loc)=Cyc_Parse_make_declarations;struct Cyc_Parse_Declaration_spec _tmp512=({Cyc_yyget_YY17(&(yyyvsp[0]).v);});struct _tuple14*_tmp513=({Cyc_yyget_YY19(&(yyyvsp[1]).v);});unsigned _tmp514=(unsigned)((yyyvsp[0]).l).first_line;unsigned _tmp515=(unsigned)location;_tmp511(_tmp512,_tmp513,_tmp514,_tmp515);});_tmp50F(_tmp510);});
goto _LL0;}case 49U: _LL5D: _LL5E:
# 1416
 yyval=({union Cyc_YYSTYPE(*_tmp516)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp517=({struct Cyc_List_List*_tmp51C=_cycalloc(sizeof(*_tmp51C));({struct Cyc_Absyn_Decl*_tmp111D=({struct Cyc_Absyn_Decl*(*_tmp518)(struct Cyc_Absyn_Pat*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_let_decl;struct Cyc_Absyn_Pat*_tmp519=({Cyc_yyget_YY9(&(yyyvsp[1]).v);});struct Cyc_Absyn_Exp*_tmp51A=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});unsigned _tmp51B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp518(_tmp519,_tmp51A,_tmp51B);});_tmp51C->hd=_tmp111D;}),_tmp51C->tl=0;_tmp51C;});_tmp516(_tmp517);});
goto _LL0;case 50U: _LL5F: _LL60: {
# 1418 "parse.y"
struct Cyc_List_List*vds=0;
{struct Cyc_List_List*ids=({Cyc_yyget_YY36(&(yyyvsp[1]).v);});for(0;ids != 0;ids=ids->tl){
struct _fat_ptr*id=(struct _fat_ptr*)ids->hd;
struct _tuple0*qv=({struct _tuple0*_tmp523=_cycalloc(sizeof(*_tmp523));({union Cyc_Absyn_Nmspace _tmp111E=({Cyc_Absyn_Rel_n(0);});_tmp523->f1=_tmp111E;}),_tmp523->f2=id;_tmp523;});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmp51E)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp51F=0U;struct _tuple0*_tmp520=qv;void*_tmp521=({Cyc_Absyn_wildtyp(0);});struct Cyc_Absyn_Exp*_tmp522=0;_tmp51E(_tmp51F,_tmp520,_tmp521,_tmp522);});
vds=({struct Cyc_List_List*_tmp51D=_cycalloc(sizeof(*_tmp51D));_tmp51D->hd=vd,_tmp51D->tl=vds;_tmp51D;});}}
# 1425
vds=({Cyc_List_imp_rev(vds);});
yyval=({union Cyc_YYSTYPE(*_tmp524)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp525=({struct Cyc_List_List*_tmp529=_cycalloc(sizeof(*_tmp529));({struct Cyc_Absyn_Decl*_tmp111F=({struct Cyc_Absyn_Decl*(*_tmp526)(struct Cyc_List_List*,unsigned)=Cyc_Absyn_letv_decl;struct Cyc_List_List*_tmp527=vds;unsigned _tmp528=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp526(_tmp527,_tmp528);});_tmp529->hd=_tmp111F;}),_tmp529->tl=0;_tmp529;});_tmp524(_tmp525);});
# 1428
goto _LL0;}case 51U: _LL61: _LL62: {
# 1431 "parse.y"
struct _fat_ptr three=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});
# 1433
if(({({struct _fat_ptr _tmp1120=(struct _fat_ptr)three;Cyc_zstrcmp(_tmp1120,({const char*_tmp52A="`H";_tag_fat(_tmp52A,sizeof(char),3U);}));});})== 0)
({void*_tmp52B=0U;({unsigned _tmp1122=(unsigned)((yyyvsp[2]).l).first_line;struct _fat_ptr _tmp1121=({const char*_tmp52C="bad occurrence of heap region";_tag_fat(_tmp52C,sizeof(char),30U);});Cyc_Warn_err(_tmp1122,_tmp1121,_tag_fat(_tmp52B,sizeof(void*),0U));});});
# 1433
if(({({struct _fat_ptr _tmp1123=(struct _fat_ptr)three;Cyc_zstrcmp(_tmp1123,({const char*_tmp52D="`U";_tag_fat(_tmp52D,sizeof(char),3U);}));});})== 0)
# 1436
({void*_tmp52E=0U;({unsigned _tmp1125=(unsigned)((yyyvsp[2]).l).first_line;struct _fat_ptr _tmp1124=({const char*_tmp52F="bad occurrence of unique region";_tag_fat(_tmp52F,sizeof(char),32U);});Cyc_Warn_err(_tmp1125,_tmp1124,_tag_fat(_tmp52E,sizeof(void*),0U));});});{
# 1433
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp540=_cycalloc(sizeof(*_tmp540));
# 1437
({struct _fat_ptr*_tmp1127=({struct _fat_ptr*_tmp53F=_cycalloc(sizeof(*_tmp53F));*_tmp53F=three;_tmp53F;});_tmp540->name=_tmp1127;}),_tmp540->identity=- 1,({void*_tmp1126=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);});_tmp540->kind=_tmp1126;});_tmp540;});
void*t=({Cyc_Absyn_var_type(tv);});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmp538)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp539=(unsigned)((yyyvsp[4]).l).first_line;struct _tuple0*_tmp53A=({struct _tuple0*_tmp53C=_cycalloc(sizeof(*_tmp53C));_tmp53C->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp1129=({struct _fat_ptr*_tmp53B=_cycalloc(sizeof(*_tmp53B));({struct _fat_ptr _tmp1128=({Cyc_yyget_String_tok(&(yyyvsp[4]).v);});*_tmp53B=_tmp1128;});_tmp53B;});_tmp53C->f2=_tmp1129;});_tmp53C;});void*_tmp53D=({Cyc_Absyn_rgn_handle_type(t);});struct Cyc_Absyn_Exp*_tmp53E=0;_tmp538(_tmp539,_tmp53A,_tmp53D,_tmp53E);});
yyval=({union Cyc_YYSTYPE(*_tmp530)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp531=({struct Cyc_List_List*_tmp537=_cycalloc(sizeof(*_tmp537));({struct Cyc_Absyn_Decl*_tmp112A=({struct Cyc_Absyn_Decl*(*_tmp532)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*open_exp,unsigned)=Cyc_Absyn_region_decl;struct Cyc_Absyn_Tvar*_tmp533=tv;struct Cyc_Absyn_Vardecl*_tmp534=vd;struct Cyc_Absyn_Exp*_tmp535=0;unsigned _tmp536=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp532(_tmp533,_tmp534,_tmp535,_tmp536);});_tmp537->hd=_tmp112A;}),_tmp537->tl=0;_tmp537;});_tmp530(_tmp531);});
# 1442
goto _LL0;}}case 52U: _LL63: _LL64: {
# 1444
struct _fat_ptr two=({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});
if(({({struct _fat_ptr _tmp112B=(struct _fat_ptr)two;Cyc_zstrcmp(_tmp112B,({const char*_tmp541="H";_tag_fat(_tmp541,sizeof(char),2U);}));});})== 0)
({void*_tmp542=0U;({unsigned _tmp112D=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp112C=({const char*_tmp543="bad occurrence of heap region `H";_tag_fat(_tmp543,sizeof(char),33U);});Cyc_Warn_err(_tmp112D,_tmp112C,_tag_fat(_tmp542,sizeof(void*),0U));});});
# 1445
if(({({struct _fat_ptr _tmp112E=(struct _fat_ptr)two;Cyc_zstrcmp(_tmp112E,({const char*_tmp544="U";_tag_fat(_tmp544,sizeof(char),2U);}));});})== 0)
# 1448
({void*_tmp545=0U;({unsigned _tmp1130=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp112F=({const char*_tmp546="bad occurrence of unique region `U";_tag_fat(_tmp546,sizeof(char),35U);});Cyc_Warn_err(_tmp1130,_tmp112F,_tag_fat(_tmp545,sizeof(void*),0U));});});{
# 1445
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp55A=_cycalloc(sizeof(*_tmp55A));
# 1449
({struct _fat_ptr*_tmp1134=({struct _fat_ptr*_tmp559=_cycalloc(sizeof(*_tmp559));({struct _fat_ptr _tmp1133=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp558=({struct Cyc_String_pa_PrintArg_struct _tmpFDB;_tmpFDB.tag=0U,_tmpFDB.f1=(struct _fat_ptr)((struct _fat_ptr)two);_tmpFDB;});void*_tmp556[1U];_tmp556[0]=& _tmp558;({struct _fat_ptr _tmp1132=({const char*_tmp557="`%s";_tag_fat(_tmp557,sizeof(char),4U);});Cyc_aprintf(_tmp1132,_tag_fat(_tmp556,sizeof(void*),1U));});});*_tmp559=_tmp1133;});_tmp559;});_tmp55A->name=_tmp1134;}),_tmp55A->identity=- 1,({
void*_tmp1131=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);});_tmp55A->kind=_tmp1131;});_tmp55A;});
void*t=({Cyc_Absyn_var_type(tv);});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmp54F)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp550=(unsigned)((yyyvsp[1]).l).first_line;struct _tuple0*_tmp551=({struct _tuple0*_tmp553=_cycalloc(sizeof(*_tmp553));_tmp553->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp1135=({struct _fat_ptr*_tmp552=_cycalloc(sizeof(*_tmp552));*_tmp552=two;_tmp552;});_tmp553->f2=_tmp1135;});_tmp553;});void*_tmp554=({Cyc_Absyn_rgn_handle_type(t);});struct Cyc_Absyn_Exp*_tmp555=0;_tmp54F(_tmp550,_tmp551,_tmp554,_tmp555);});
yyval=({union Cyc_YYSTYPE(*_tmp547)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp548=({struct Cyc_List_List*_tmp54E=_cycalloc(sizeof(*_tmp54E));({struct Cyc_Absyn_Decl*_tmp1136=({struct Cyc_Absyn_Decl*(*_tmp549)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*open_exp,unsigned)=Cyc_Absyn_region_decl;struct Cyc_Absyn_Tvar*_tmp54A=tv;struct Cyc_Absyn_Vardecl*_tmp54B=vd;struct Cyc_Absyn_Exp*_tmp54C=0;unsigned _tmp54D=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp549(_tmp54A,_tmp54B,_tmp54C,_tmp54D);});_tmp54E->hd=_tmp1136;}),_tmp54E->tl=0;_tmp54E;});_tmp547(_tmp548);});
# 1455
goto _LL0;}}case 53U: _LL65: _LL66: {
# 1457
struct _fat_ptr two=({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});
struct _fat_ptr four=({Cyc_yyget_String_tok(&(yyyvsp[3]).v);});
struct Cyc_Absyn_Exp*six=({Cyc_yyget_Exp_tok(&(yyyvsp[5]).v);});
int ba=({({struct _fat_ptr _tmp1137=(struct _fat_ptr)four;Cyc_strcmp(_tmp1137,({const char*_tmp586="open";_tag_fat(_tmp586,sizeof(char),5U);}));});})== 0;
int bb=({({struct _fat_ptr _tmp1138=(struct _fat_ptr)four;Cyc_strcmp(_tmp1138,({const char*_tmp585="xopen";_tag_fat(_tmp585,sizeof(char),6U);}));});})== 0;
if(!ba && !bb)
({void*_tmp55B=0U;({unsigned _tmp113A=(unsigned)((yyyvsp[3]).l).first_line;struct _fat_ptr _tmp1139=({const char*_tmp55C="expecting `open' or `xopen'";_tag_fat(_tmp55C,sizeof(char),28U);});Cyc_Warn_err(_tmp113A,_tmp1139,_tag_fat(_tmp55B,sizeof(void*),0U));});});
# 1462
if(ba){
# 1465
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp570=_cycalloc(sizeof(*_tmp570));({struct _fat_ptr*_tmp113E=({struct _fat_ptr*_tmp56F=_cycalloc(sizeof(*_tmp56F));({struct _fat_ptr _tmp113D=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp56E=({struct Cyc_String_pa_PrintArg_struct _tmpFDC;_tmpFDC.tag=0U,_tmpFDC.f1=(struct _fat_ptr)((struct _fat_ptr)two);_tmpFDC;});void*_tmp56C[1U];_tmp56C[0]=& _tmp56E;({struct _fat_ptr _tmp113C=({const char*_tmp56D="`%s";_tag_fat(_tmp56D,sizeof(char),4U);});Cyc_aprintf(_tmp113C,_tag_fat(_tmp56C,sizeof(void*),1U));});});*_tmp56F=_tmp113D;});_tmp56F;});_tmp570->name=_tmp113E;}),_tmp570->identity=- 1,({
void*_tmp113B=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);});_tmp570->kind=_tmp113B;});_tmp570;});
void*t=({Cyc_Absyn_var_type(tv);});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmp565)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp566=(unsigned)((yyyvsp[2]).l).first_line;struct _tuple0*_tmp567=({struct _tuple0*_tmp569=_cycalloc(sizeof(*_tmp569));_tmp569->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp113F=({struct _fat_ptr*_tmp568=_cycalloc(sizeof(*_tmp568));*_tmp568=two;_tmp568;});_tmp569->f2=_tmp113F;});_tmp569;});void*_tmp56A=({Cyc_Absyn_rgn_handle_type(t);});struct Cyc_Absyn_Exp*_tmp56B=0;_tmp565(_tmp566,_tmp567,_tmp56A,_tmp56B);});
yyval=({union Cyc_YYSTYPE(*_tmp55D)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp55E=({struct Cyc_List_List*_tmp564=_cycalloc(sizeof(*_tmp564));({struct Cyc_Absyn_Decl*_tmp1140=({struct Cyc_Absyn_Decl*(*_tmp55F)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*open_exp,unsigned)=Cyc_Absyn_region_decl;struct Cyc_Absyn_Tvar*_tmp560=tv;struct Cyc_Absyn_Vardecl*_tmp561=vd;struct Cyc_Absyn_Exp*_tmp562=six;unsigned _tmp563=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp55F(_tmp560,_tmp561,_tmp562,_tmp563);});_tmp564->hd=_tmp1140;}),_tmp564->tl=0;_tmp564;});_tmp55D(_tmp55E);});}else{
# 1471
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp584=_cycalloc(sizeof(*_tmp584));({struct _fat_ptr*_tmp1144=({struct _fat_ptr*_tmp583=_cycalloc(sizeof(*_tmp583));({struct _fat_ptr _tmp1143=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp582=({struct Cyc_String_pa_PrintArg_struct _tmpFDD;_tmpFDD.tag=0U,_tmpFDD.f1=(struct _fat_ptr)((struct _fat_ptr)two);_tmpFDD;});void*_tmp580[1U];_tmp580[0]=& _tmp582;({struct _fat_ptr _tmp1142=({const char*_tmp581="`%s";_tag_fat(_tmp581,sizeof(char),4U);});Cyc_aprintf(_tmp1142,_tag_fat(_tmp580,sizeof(void*),1U));});});*_tmp583=_tmp1143;});_tmp583;});_tmp584->name=_tmp1144;}),_tmp584->identity=- 2,({
void*_tmp1141=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_rk);});_tmp584->kind=_tmp1141;});_tmp584;});
void*t=({Cyc_Absyn_var_type(tv);});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmp579)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp57A=(unsigned)((yyyvsp[2]).l).first_line;struct _tuple0*_tmp57B=({struct _tuple0*_tmp57D=_cycalloc(sizeof(*_tmp57D));_tmp57D->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp1145=({struct _fat_ptr*_tmp57C=_cycalloc(sizeof(*_tmp57C));*_tmp57C=two;_tmp57C;});_tmp57D->f2=_tmp1145;});_tmp57D;});void*_tmp57E=({Cyc_Absyn_rgn_handle_type(t);});struct Cyc_Absyn_Exp*_tmp57F=0;_tmp579(_tmp57A,_tmp57B,_tmp57E,_tmp57F);});
yyval=({union Cyc_YYSTYPE(*_tmp571)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp572=({struct Cyc_List_List*_tmp578=_cycalloc(sizeof(*_tmp578));({struct Cyc_Absyn_Decl*_tmp1146=({struct Cyc_Absyn_Decl*(*_tmp573)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*open_exp,unsigned)=Cyc_Absyn_region_decl;struct Cyc_Absyn_Tvar*_tmp574=tv;struct Cyc_Absyn_Vardecl*_tmp575=vd;struct Cyc_Absyn_Exp*_tmp576=six;unsigned _tmp577=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp573(_tmp574,_tmp575,_tmp576,_tmp577);});_tmp578->hd=_tmp1146;}),_tmp578->tl=0;_tmp578;});_tmp571(_tmp572);});}
# 1478
goto _LL0;}case 54U: _LL67: _LL68: {
# 1480
struct _fat_ptr two=({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});
struct Cyc_Absyn_Exp*four=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmp59A=_cycalloc(sizeof(*_tmp59A));({struct _fat_ptr*_tmp114A=({struct _fat_ptr*_tmp599=_cycalloc(sizeof(*_tmp599));({struct _fat_ptr _tmp1149=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp598=({struct Cyc_String_pa_PrintArg_struct _tmpFDE;_tmpFDE.tag=0U,_tmpFDE.f1=(struct _fat_ptr)((struct _fat_ptr)two);_tmpFDE;});void*_tmp596[1U];_tmp596[0]=& _tmp598;({struct _fat_ptr _tmp1148=({const char*_tmp597="`%s";_tag_fat(_tmp597,sizeof(char),4U);});Cyc_aprintf(_tmp1148,_tag_fat(_tmp596,sizeof(void*),1U));});});*_tmp599=_tmp1149;});_tmp599;});_tmp59A->name=_tmp114A;}),_tmp59A->identity=- 1,({
void*_tmp1147=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_xrk);});_tmp59A->kind=_tmp1147;});_tmp59A;});
void*t=({Cyc_Absyn_xrgn_var_type(tv);});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmp58F)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp590=(unsigned)((yyyvsp[2]).l).first_line;struct _tuple0*_tmp591=({struct _tuple0*_tmp593=_cycalloc(sizeof(*_tmp593));
_tmp593->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp114B=({struct _fat_ptr*_tmp592=_cycalloc(sizeof(*_tmp592));*_tmp592=two;_tmp592;});_tmp593->f2=_tmp114B;});_tmp593;});void*_tmp594=({Cyc_Absyn_rgn_handle_type(t);});struct Cyc_Absyn_Exp*_tmp595=0;_tmp58F(_tmp590,_tmp591,_tmp594,_tmp595);});
# 1493
yyval=({union Cyc_YYSTYPE(*_tmp587)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp588=({struct Cyc_List_List*_tmp58E=_cycalloc(sizeof(*_tmp58E));({struct Cyc_Absyn_Decl*_tmp114C=({struct Cyc_Absyn_Decl*(*_tmp589)(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*open_exp,unsigned)=Cyc_Absyn_region_decl;struct Cyc_Absyn_Tvar*_tmp58A=tv;struct Cyc_Absyn_Vardecl*_tmp58B=vd;struct Cyc_Absyn_Exp*_tmp58C=four;unsigned _tmp58D=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp589(_tmp58A,_tmp58B,_tmp58C,_tmp58D);});_tmp58E->hd=_tmp114C;}),_tmp58E->tl=0;_tmp58E;});_tmp587(_tmp588);});
# 1495
goto _LL0;}case 55U: _LL69: _LL6A:
# 1499 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 56U: _LL6B: _LL6C:
# 1501 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp59B)(struct Cyc_List_List*yy1)=Cyc_YY16;struct Cyc_List_List*_tmp59C=({struct Cyc_List_List*(*_tmp59D)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp59E=({Cyc_yyget_YY16(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp59F=({Cyc_yyget_YY16(&(yyyvsp[1]).v);});_tmp59D(_tmp59E,_tmp59F);});_tmp59B(_tmp59C);});
goto _LL0;case 57U: _LL6D: _LL6E:
# 1507 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5A0)(struct Cyc_Parse_Declaration_spec yy1)=Cyc_YY17;struct Cyc_Parse_Declaration_spec _tmp5A1=({struct Cyc_Parse_Declaration_spec _tmpFDF;({enum Cyc_Parse_Storage_class*_tmp114F=({Cyc_yyget_YY20(&(yyyvsp[0]).v);});_tmpFDF.sc=_tmp114F;}),({struct Cyc_Absyn_Tqual _tmp114E=({Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);});_tmpFDF.tq=_tmp114E;}),({
struct Cyc_Parse_Type_specifier _tmp114D=({Cyc_Parse_empty_spec(0U);});_tmpFDF.type_specs=_tmp114D;}),_tmpFDF.is_inline=0,_tmpFDF.attributes=0;_tmpFDF;});_tmp5A0(_tmp5A1);});
goto _LL0;case 58U: _LL6F: _LL70: {
# 1510 "parse.y"
struct Cyc_Parse_Declaration_spec two=({Cyc_yyget_YY17(&(yyyvsp[1]).v);});
if(two.sc != 0)
({void*_tmp5A2=0U;({unsigned _tmp1151=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct _fat_ptr _tmp1150=({const char*_tmp5A3="Only one storage class is allowed in a declaration (missing ';' or ','?)";_tag_fat(_tmp5A3,sizeof(char),73U);});Cyc_Warn_warn(_tmp1151,_tmp1150,_tag_fat(_tmp5A2,sizeof(void*),0U));});});
# 1511
yyval=({union Cyc_YYSTYPE(*_tmp5A4)(struct Cyc_Parse_Declaration_spec yy1)=Cyc_YY17;struct Cyc_Parse_Declaration_spec _tmp5A5=({struct Cyc_Parse_Declaration_spec _tmpFE0;({
# 1514
enum Cyc_Parse_Storage_class*_tmp1152=({Cyc_yyget_YY20(&(yyyvsp[0]).v);});_tmpFE0.sc=_tmp1152;}),_tmpFE0.tq=two.tq,_tmpFE0.type_specs=two.type_specs,_tmpFE0.is_inline=two.is_inline,_tmpFE0.attributes=two.attributes;_tmpFE0;});_tmp5A4(_tmp5A5);});
# 1518
goto _LL0;}case 59U: _LL71: _LL72:
# 1519 "parse.y"
({void*_tmp5A6=0U;({unsigned _tmp1154=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct _fat_ptr _tmp1153=({const char*_tmp5A7="__extension__ keyword ignored in declaration";_tag_fat(_tmp5A7,sizeof(char),45U);});Cyc_Warn_warn(_tmp1154,_tmp1153,_tag_fat(_tmp5A6,sizeof(void*),0U));});});
yyval=(yyyvsp[1]).v;
# 1522
goto _LL0;case 60U: _LL73: _LL74:
# 1523 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5A8)(struct Cyc_Parse_Declaration_spec yy1)=Cyc_YY17;struct Cyc_Parse_Declaration_spec _tmp5A9=({struct Cyc_Parse_Declaration_spec _tmpFE1;_tmpFE1.sc=0,({struct Cyc_Absyn_Tqual _tmp1156=({Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);});_tmpFE1.tq=_tmp1156;}),({
struct Cyc_Parse_Type_specifier _tmp1155=({Cyc_yyget_YY21(&(yyyvsp[0]).v);});_tmpFE1.type_specs=_tmp1155;}),_tmpFE1.is_inline=0,_tmpFE1.attributes=0;_tmpFE1;});_tmp5A8(_tmp5A9);});
goto _LL0;case 61U: _LL75: _LL76: {
# 1526 "parse.y"
struct Cyc_Parse_Declaration_spec two=({Cyc_yyget_YY17(&(yyyvsp[1]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp5AA)(struct Cyc_Parse_Declaration_spec yy1)=Cyc_YY17;struct Cyc_Parse_Declaration_spec _tmp5AB=({struct Cyc_Parse_Declaration_spec _tmpFE2;_tmpFE2.sc=two.sc,_tmpFE2.tq=two.tq,({
struct Cyc_Parse_Type_specifier _tmp1157=({struct Cyc_Parse_Type_specifier(*_tmp5AC)(unsigned loc,struct Cyc_Parse_Type_specifier s1,struct Cyc_Parse_Type_specifier s2)=Cyc_Parse_combine_specifiers;unsigned _tmp5AD=(unsigned)((yyyvsp[0]).l).first_line;struct Cyc_Parse_Type_specifier _tmp5AE=two.type_specs;struct Cyc_Parse_Type_specifier _tmp5AF=({Cyc_yyget_YY21(&(yyyvsp[0]).v);});_tmp5AC(_tmp5AD,_tmp5AE,_tmp5AF);});_tmpFE2.type_specs=_tmp1157;}),_tmpFE2.is_inline=two.is_inline,_tmpFE2.attributes=two.attributes;_tmpFE2;});_tmp5AA(_tmp5AB);});
# 1532
goto _LL0;}case 62U: _LL77: _LL78:
# 1533 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5B0)(struct Cyc_Parse_Declaration_spec yy1)=Cyc_YY17;struct Cyc_Parse_Declaration_spec _tmp5B1=({struct Cyc_Parse_Declaration_spec _tmpFE3;_tmpFE3.sc=0,({struct Cyc_Absyn_Tqual _tmp1159=({Cyc_yyget_YY23(&(yyyvsp[0]).v);});_tmpFE3.tq=_tmp1159;}),({struct Cyc_Parse_Type_specifier _tmp1158=({Cyc_Parse_empty_spec(0U);});_tmpFE3.type_specs=_tmp1158;}),_tmpFE3.is_inline=0,_tmpFE3.attributes=0;_tmpFE3;});_tmp5B0(_tmp5B1);});
goto _LL0;case 63U: _LL79: _LL7A: {
# 1535 "parse.y"
struct Cyc_Parse_Declaration_spec two=({Cyc_yyget_YY17(&(yyyvsp[1]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp5B2)(struct Cyc_Parse_Declaration_spec yy1)=Cyc_YY17;struct Cyc_Parse_Declaration_spec _tmp5B3=({struct Cyc_Parse_Declaration_spec _tmpFE4;_tmpFE4.sc=two.sc,({struct Cyc_Absyn_Tqual _tmp115A=({struct Cyc_Absyn_Tqual(*_tmp5B4)(struct Cyc_Absyn_Tqual,struct Cyc_Absyn_Tqual)=Cyc_Absyn_combine_tqual;struct Cyc_Absyn_Tqual _tmp5B5=({Cyc_yyget_YY23(&(yyyvsp[0]).v);});struct Cyc_Absyn_Tqual _tmp5B6=two.tq;_tmp5B4(_tmp5B5,_tmp5B6);});_tmpFE4.tq=_tmp115A;}),_tmpFE4.type_specs=two.type_specs,_tmpFE4.is_inline=two.is_inline,_tmpFE4.attributes=two.attributes;_tmpFE4;});_tmp5B2(_tmp5B3);});
# 1540
goto _LL0;}case 64U: _LL7B: _LL7C:
# 1541 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5B7)(struct Cyc_Parse_Declaration_spec yy1)=Cyc_YY17;struct Cyc_Parse_Declaration_spec _tmp5B8=({struct Cyc_Parse_Declaration_spec _tmpFE5;_tmpFE5.sc=0,({struct Cyc_Absyn_Tqual _tmp115C=({Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);});_tmpFE5.tq=_tmp115C;}),({
struct Cyc_Parse_Type_specifier _tmp115B=({Cyc_Parse_empty_spec(0U);});_tmpFE5.type_specs=_tmp115B;}),_tmpFE5.is_inline=1,_tmpFE5.attributes=0;_tmpFE5;});_tmp5B7(_tmp5B8);});
goto _LL0;case 65U: _LL7D: _LL7E: {
# 1544 "parse.y"
struct Cyc_Parse_Declaration_spec two=({Cyc_yyget_YY17(&(yyyvsp[1]).v);});
yyval=({Cyc_YY17(({struct Cyc_Parse_Declaration_spec _tmpFE6;_tmpFE6.sc=two.sc,_tmpFE6.tq=two.tq,_tmpFE6.type_specs=two.type_specs,_tmpFE6.is_inline=1,_tmpFE6.attributes=two.attributes;_tmpFE6;}));});
# 1548
goto _LL0;}case 66U: _LL7F: _LL80:
# 1549 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5B9)(struct Cyc_Parse_Declaration_spec yy1)=Cyc_YY17;struct Cyc_Parse_Declaration_spec _tmp5BA=({struct Cyc_Parse_Declaration_spec _tmpFE7;_tmpFE7.sc=0,({struct Cyc_Absyn_Tqual _tmp115F=({Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);});_tmpFE7.tq=_tmp115F;}),({
struct Cyc_Parse_Type_specifier _tmp115E=({Cyc_Parse_empty_spec(0U);});_tmpFE7.type_specs=_tmp115E;}),_tmpFE7.is_inline=0,({struct Cyc_List_List*_tmp115D=({Cyc_yyget_YY45(&(yyyvsp[0]).v);});_tmpFE7.attributes=_tmp115D;});_tmpFE7;});_tmp5B9(_tmp5BA);});
goto _LL0;case 67U: _LL81: _LL82: {
# 1552 "parse.y"
struct Cyc_Parse_Declaration_spec two=({Cyc_yyget_YY17(&(yyyvsp[1]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp5BB)(struct Cyc_Parse_Declaration_spec yy1)=Cyc_YY17;struct Cyc_Parse_Declaration_spec _tmp5BC=({struct Cyc_Parse_Declaration_spec _tmpFE8;_tmpFE8.sc=two.sc,_tmpFE8.tq=two.tq,_tmpFE8.type_specs=two.type_specs,_tmpFE8.is_inline=two.is_inline,({
# 1555
struct Cyc_List_List*_tmp1160=({struct Cyc_List_List*(*_tmp5BD)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp5BE=({Cyc_yyget_YY45(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp5BF=two.attributes;_tmp5BD(_tmp5BE,_tmp5BF);});_tmpFE8.attributes=_tmp1160;});_tmpFE8;});_tmp5BB(_tmp5BC);});
goto _LL0;}case 68U: _LL83: _LL84: {
# 1559 "parse.y"
static enum Cyc_Parse_Storage_class auto_sc=Cyc_Parse_Auto_sc;
yyval=({Cyc_YY20(& auto_sc);});
goto _LL0;}case 69U: _LL85: _LL86: {
# 1561 "parse.y"
static enum Cyc_Parse_Storage_class register_sc=Cyc_Parse_Register_sc;
yyval=({Cyc_YY20(& register_sc);});
goto _LL0;}case 70U: _LL87: _LL88: {
# 1563 "parse.y"
static enum Cyc_Parse_Storage_class static_sc=Cyc_Parse_Static_sc;
yyval=({Cyc_YY20(& static_sc);});
goto _LL0;}case 71U: _LL89: _LL8A: {
# 1565 "parse.y"
static enum Cyc_Parse_Storage_class extern_sc=Cyc_Parse_Extern_sc;
yyval=({Cyc_YY20(& extern_sc);});
goto _LL0;}case 72U: _LL8B: _LL8C: {
# 1568 "parse.y"
static enum Cyc_Parse_Storage_class externC_sc=Cyc_Parse_ExternC_sc;
if(({int(*_tmp5C0)(struct _fat_ptr s1,struct _fat_ptr s2)=Cyc_strcmp;struct _fat_ptr _tmp5C1=(struct _fat_ptr)({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});struct _fat_ptr _tmp5C2=({const char*_tmp5C3="C";_tag_fat(_tmp5C3,sizeof(char),2U);});_tmp5C0(_tmp5C1,_tmp5C2);})!= 0)
({void*_tmp5C4=0U;({unsigned _tmp1162=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct _fat_ptr _tmp1161=({const char*_tmp5C5="only extern or extern \"C\" is allowed";_tag_fat(_tmp5C5,sizeof(char),37U);});Cyc_Warn_err(_tmp1162,_tmp1161,_tag_fat(_tmp5C4,sizeof(void*),0U));});});
# 1569
yyval=({Cyc_YY20(& externC_sc);});
# 1573
goto _LL0;}case 73U: _LL8D: _LL8E: {
# 1573 "parse.y"
static enum Cyc_Parse_Storage_class typedef_sc=Cyc_Parse_Typedef_sc;
yyval=({Cyc_YY20(& typedef_sc);});
goto _LL0;}case 74U: _LL8F: _LL90: {
# 1576 "parse.y"
static enum Cyc_Parse_Storage_class abstract_sc=Cyc_Parse_Abstract_sc;
yyval=({Cyc_YY20(& abstract_sc);});
goto _LL0;}case 75U: _LL91: _LL92:
# 1582 "parse.y"
 yyval=({Cyc_YY45(0);});
goto _LL0;case 76U: _LL93: _LL94:
# 1583 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 77U: _LL95: _LL96:
# 1588 "parse.y"
 yyval=(yyyvsp[3]).v;
goto _LL0;case 78U: _LL97: _LL98:
# 1592 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5C6)(struct Cyc_List_List*yy1)=Cyc_YY45;struct Cyc_List_List*_tmp5C7=({struct Cyc_List_List*_tmp5C8=_cycalloc(sizeof(*_tmp5C8));({void*_tmp1163=({Cyc_yyget_YY46(&(yyyvsp[0]).v);});_tmp5C8->hd=_tmp1163;}),_tmp5C8->tl=0;_tmp5C8;});_tmp5C6(_tmp5C7);});
goto _LL0;case 79U: _LL99: _LL9A:
# 1593 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5C9)(struct Cyc_List_List*yy1)=Cyc_YY45;struct Cyc_List_List*_tmp5CA=({struct Cyc_List_List*_tmp5CB=_cycalloc(sizeof(*_tmp5CB));({void*_tmp1165=({Cyc_yyget_YY46(&(yyyvsp[0]).v);});_tmp5CB->hd=_tmp1165;}),({struct Cyc_List_List*_tmp1164=({Cyc_yyget_YY45(&(yyyvsp[2]).v);});_tmp5CB->tl=_tmp1164;});_tmp5CB;});_tmp5C9(_tmp5CA);});
goto _LL0;case 80U: _LL9B: _LL9C:
# 1597 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5CC)(void*yy1)=Cyc_YY46;void*_tmp5CD=({void*(*_tmp5CE)(unsigned loc,struct _fat_ptr s)=Cyc_Absyn_parse_nullary_att;unsigned _tmp5CF=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp5D0=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});_tmp5CE(_tmp5CF,_tmp5D0);});_tmp5CC(_tmp5CD);});
goto _LL0;case 81U: _LL9D: _LL9E:
# 1598 "parse.y"
 yyval=({Cyc_YY46((void*)& Cyc_Absyn_Const_att_val);});
goto _LL0;case 82U: _LL9F: _LLA0:
# 1600 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5D1)(void*yy1)=Cyc_YY46;void*_tmp5D2=({void*(*_tmp5D3)(unsigned sloc,struct _fat_ptr s,unsigned eloc,struct Cyc_Absyn_Exp*e)=Cyc_Absyn_parse_unary_att;unsigned _tmp5D4=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp5D5=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});unsigned _tmp5D6=(unsigned)((yyyvsp[2]).l).first_line;struct Cyc_Absyn_Exp*_tmp5D7=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp5D3(_tmp5D4,_tmp5D5,_tmp5D6,_tmp5D7);});_tmp5D1(_tmp5D2);});
goto _LL0;case 83U: _LLA1: _LLA2:
# 1602 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5D8)(void*yy1)=Cyc_YY46;void*_tmp5D9=({void*(*_tmp5DA)(unsigned loc,unsigned s2loc,struct _fat_ptr s1,struct _fat_ptr s2,unsigned u1,unsigned u2)=Cyc_Absyn_parse_format_att;unsigned _tmp5DB=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});unsigned _tmp5DC=(unsigned)((yyyvsp[2]).l).first_line;struct _fat_ptr _tmp5DD=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});struct _fat_ptr _tmp5DE=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});unsigned _tmp5DF=({unsigned(*_tmp5E0)(unsigned loc,union Cyc_Absyn_Cnst x)=Cyc_Parse_cnst2uint;unsigned _tmp5E1=(unsigned)((yyyvsp[4]).l).first_line;union Cyc_Absyn_Cnst _tmp5E2=({Cyc_yyget_Int_tok(&(yyyvsp[4]).v);});_tmp5E0(_tmp5E1,_tmp5E2);});unsigned _tmp5E3=({unsigned(*_tmp5E4)(unsigned loc,union Cyc_Absyn_Cnst x)=Cyc_Parse_cnst2uint;unsigned _tmp5E5=(unsigned)((yyyvsp[6]).l).first_line;union Cyc_Absyn_Cnst _tmp5E6=({Cyc_yyget_Int_tok(&(yyyvsp[6]).v);});_tmp5E4(_tmp5E5,_tmp5E6);});_tmp5DA(_tmp5DB,_tmp5DC,_tmp5DD,_tmp5DE,_tmp5DF,_tmp5E3);});_tmp5D8(_tmp5D9);});
# 1605
goto _LL0;case 84U: _LLA3: _LLA4:
# 1682 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 85U: _LLA5: _LLA6:
# 1684 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5E7)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp5E8=({struct Cyc_Parse_Type_specifier(*_tmp5E9)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp5EA=({void*(*_tmp5EB)(struct _tuple0*,struct Cyc_List_List*,struct Cyc_Absyn_Typedefdecl*,void*)=Cyc_Absyn_typedef_type;struct _tuple0*_tmp5EC=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp5ED=({Cyc_yyget_YY40(&(yyyvsp[1]).v);});struct Cyc_Absyn_Typedefdecl*_tmp5EE=0;void*_tmp5EF=0;_tmp5EB(_tmp5EC,_tmp5ED,_tmp5EE,_tmp5EF);});unsigned _tmp5F0=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp5E9(_tmp5EA,_tmp5F0);});_tmp5E7(_tmp5E8);});
goto _LL0;case 86U: _LLA7: _LLA8:
# 1688 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5F1)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp5F2=({Cyc_Parse_type_spec(Cyc_Absyn_void_type,(unsigned)((yyyvsp[0]).l).first_line);});_tmp5F1(_tmp5F2);});
goto _LL0;case 87U: _LLA9: _LLAA:
# 1689 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5F3)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp5F4=({Cyc_Parse_type_spec(Cyc_Absyn_char_type,(unsigned)((yyyvsp[0]).l).first_line);});_tmp5F3(_tmp5F4);});
goto _LL0;case 88U: _LLAB: _LLAC:
# 1690 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5F5)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp5F6=({Cyc_Parse_short_spec((unsigned)((yyyvsp[0]).l).first_line);});_tmp5F5(_tmp5F6);});
goto _LL0;case 89U: _LLAD: _LLAE:
# 1691 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5F7)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp5F8=({Cyc_Parse_type_spec(Cyc_Absyn_sint_type,(unsigned)((yyyvsp[0]).l).first_line);});_tmp5F7(_tmp5F8);});
goto _LL0;case 90U: _LLAF: _LLB0:
# 1692 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5F9)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp5FA=({Cyc_Parse_long_spec((unsigned)((yyyvsp[0]).l).first_line);});_tmp5F9(_tmp5FA);});
goto _LL0;case 91U: _LLB1: _LLB2:
# 1693 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5FB)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp5FC=({Cyc_Parse_type_spec(Cyc_Absyn_float_type,(unsigned)((yyyvsp[0]).l).first_line);});_tmp5FB(_tmp5FC);});
goto _LL0;case 92U: _LLB3: _LLB4:
# 1694 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5FD)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp5FE=({Cyc_Parse_type_spec(Cyc_Absyn_double_type,(unsigned)((yyyvsp[0]).l).first_line);});_tmp5FD(_tmp5FE);});
goto _LL0;case 93U: _LLB5: _LLB6:
# 1695 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp5FF)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp600=({Cyc_Parse_signed_spec((unsigned)((yyyvsp[0]).l).first_line);});_tmp5FF(_tmp600);});
goto _LL0;case 94U: _LLB7: _LLB8:
# 1696 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp601)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp602=({Cyc_Parse_unsigned_spec((unsigned)((yyyvsp[0]).l).first_line);});_tmp601(_tmp602);});
goto _LL0;case 95U: _LLB9: _LLBA:
# 1697 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 96U: _LLBB: _LLBC:
# 1698 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 97U: _LLBD: _LLBE:
# 1701
 yyval=({union Cyc_YYSTYPE(*_tmp603)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp604=({struct Cyc_Parse_Type_specifier(*_tmp605)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp606=({void*(*_tmp607)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_typeof_type;struct Cyc_Absyn_Exp*_tmp608=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp607(_tmp608);});unsigned _tmp609=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp605(_tmp606,_tmp609);});_tmp603(_tmp604);});
goto _LL0;case 98U: _LLBF: _LLC0:
# 1703 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp60A)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp60B=({struct Cyc_Parse_Type_specifier(*_tmp60C)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp60D=({Cyc_Absyn_builtin_type(({const char*_tmp60E="__builtin_va_list";_tag_fat(_tmp60E,sizeof(char),18U);}),& Cyc_Tcutil_bk);});unsigned _tmp60F=(unsigned)((yyyvsp[0]).l).first_line;_tmp60C(_tmp60D,_tmp60F);});_tmp60A(_tmp60B);});
goto _LL0;case 99U: _LLC1: _LLC2:
# 1705 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 100U: _LLC3: _LLC4:
# 1707 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp610)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp611=({struct Cyc_Parse_Type_specifier(*_tmp612)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp613=({Cyc_yyget_YY44(&(yyyvsp[0]).v);});unsigned _tmp614=(unsigned)((yyyvsp[0]).l).first_line;_tmp612(_tmp613,_tmp614);});_tmp610(_tmp611);});
goto _LL0;case 101U: _LLC5: _LLC6:
# 1709 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp615)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp616=({struct Cyc_Parse_Type_specifier(*_tmp617)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp618=({Cyc_Absyn_new_evar(0,0);});unsigned _tmp619=(unsigned)((yyyvsp[0]).l).first_line;_tmp617(_tmp618,_tmp619);});_tmp615(_tmp616);});
goto _LL0;case 102U: _LLC7: _LLC8:
# 1711 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp61A)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp61B=({struct Cyc_Parse_Type_specifier(*_tmp61C)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp61D=({void*(*_tmp61E)(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv)=Cyc_Absyn_new_evar;struct Cyc_Core_Opt*_tmp61F=({struct Cyc_Core_Opt*(*_tmp620)(struct Cyc_Absyn_Kind*k)=Cyc_Tcutil_kind_to_opt;struct Cyc_Absyn_Kind*_tmp621=({Cyc_yyget_YY43(&(yyyvsp[2]).v);});_tmp620(_tmp621);});struct Cyc_Core_Opt*_tmp622=0;_tmp61E(_tmp61F,_tmp622);});unsigned _tmp623=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp61C(_tmp61D,_tmp623);});_tmp61A(_tmp61B);});
goto _LL0;case 103U: _LLC9: _LLCA:
# 1713 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp624)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp625=({struct Cyc_Parse_Type_specifier(*_tmp626)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp627=(void*)({struct Cyc_Absyn_TupleType_Absyn_Type_struct*_tmp62F=_cycalloc(sizeof(*_tmp62F));_tmp62F->tag=6U,({struct Cyc_List_List*_tmp1166=({struct Cyc_List_List*(*_tmp628)(struct _tuple20*(*f)(unsigned,struct _tuple9*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp629)(struct _tuple20*(*f)(unsigned,struct _tuple9*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple20*(*f)(unsigned,struct _tuple9*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp629;});struct _tuple20*(*_tmp62A)(unsigned loc,struct _tuple9*t)=Cyc_Parse_get_tqual_typ;unsigned _tmp62B=(unsigned)((yyyvsp[2]).l).first_line;struct Cyc_List_List*_tmp62C=({struct Cyc_List_List*(*_tmp62D)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp62E=({Cyc_yyget_YY38(&(yyyvsp[2]).v);});_tmp62D(_tmp62E);});_tmp628(_tmp62A,_tmp62B,_tmp62C);});_tmp62F->f1=_tmp1166;});_tmp62F;});unsigned _tmp630=(unsigned)({
# 1715
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp626(_tmp627,_tmp630);});_tmp624(_tmp625);});
goto _LL0;case 104U: _LLCB: _LLCC:
# 1717 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp631)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp632=({struct Cyc_Parse_Type_specifier(*_tmp633)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp634=({void*(*_tmp635)(void*)=Cyc_Absyn_rgn_handle_type;void*_tmp636=({Cyc_yyget_YY44(&(yyyvsp[2]).v);});_tmp635(_tmp636);});unsigned _tmp637=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp633(_tmp634,_tmp637);});_tmp631(_tmp632);});
goto _LL0;case 105U: _LLCD: _LLCE:
# 1719 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp638)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp639=({struct Cyc_Parse_Type_specifier(*_tmp63A)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp63B=({void*(*_tmp63C)(void*)=Cyc_Absyn_rgn_handle_type;void*_tmp63D=({Cyc_Absyn_new_evar(& Cyc_Tcutil_rko,0);});_tmp63C(_tmp63D);});unsigned _tmp63E=(unsigned)((yyyvsp[0]).l).first_line;_tmp63A(_tmp63B,_tmp63E);});_tmp638(_tmp639);});
# 1721
goto _LL0;case 106U: _LLCF: _LLD0:
# 1722 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp63F)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp640=({struct Cyc_Parse_Type_specifier(*_tmp641)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp642=({void*(*_tmp643)(void*)=Cyc_Absyn_tag_type;void*_tmp644=({Cyc_yyget_YY44(&(yyyvsp[2]).v);});_tmp643(_tmp644);});unsigned _tmp645=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp641(_tmp642,_tmp645);});_tmp63F(_tmp640);});
goto _LL0;case 107U: _LLD1: _LLD2:
# 1724 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp646)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp647=({struct Cyc_Parse_Type_specifier(*_tmp648)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp649=({void*(*_tmp64A)(void*)=Cyc_Absyn_tag_type;void*_tmp64B=({Cyc_Absyn_new_evar(& Cyc_Tcutil_iko,0);});_tmp64A(_tmp64B);});unsigned _tmp64C=(unsigned)((yyyvsp[0]).l).first_line;_tmp648(_tmp649,_tmp64C);});_tmp646(_tmp647);});
goto _LL0;case 108U: _LLD3: _LLD4:
# 1726 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp64D)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp64E=({struct Cyc_Parse_Type_specifier(*_tmp64F)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp650=({void*(*_tmp651)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_valueof_type;struct Cyc_Absyn_Exp*_tmp652=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp651(_tmp652);});unsigned _tmp653=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp64F(_tmp650,_tmp653);});_tmp64D(_tmp64E);});
goto _LL0;case 109U: _LLD5: _LLD6:
# 1732 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp654)(struct Cyc_Absyn_Kind*yy1)=Cyc_YY43;struct Cyc_Absyn_Kind*_tmp655=({struct Cyc_Absyn_Kind*(*_tmp656)(struct _fat_ptr s,unsigned loc)=Cyc_Parse_id_to_kind;struct _fat_ptr _tmp657=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});unsigned _tmp658=(unsigned)((yyyvsp[0]).l).first_line;_tmp656(_tmp657,_tmp658);});_tmp654(_tmp655);});
goto _LL0;case 110U: _LLD7: _LLD8: {
# 1736 "parse.y"
unsigned loc=(unsigned)(Cyc_Absyn_porting_c_code?((yyyvsp[0]).l).first_line:(int)0U);
yyval=({Cyc_YY23(({struct Cyc_Absyn_Tqual _tmpFE9;_tmpFE9.print_const=1,_tmpFE9.q_volatile=0,_tmpFE9.q_restrict=0,_tmpFE9.real_const=1,_tmpFE9.loc=loc;_tmpFE9;}));});
goto _LL0;}case 111U: _LLD9: _LLDA:
# 1738 "parse.y"
 yyval=({Cyc_YY23(({struct Cyc_Absyn_Tqual _tmpFEA;_tmpFEA.print_const=0,_tmpFEA.q_volatile=1,_tmpFEA.q_restrict=0,_tmpFEA.real_const=0,_tmpFEA.loc=0U;_tmpFEA;}));});
goto _LL0;case 112U: _LLDB: _LLDC:
# 1739 "parse.y"
 yyval=({Cyc_YY23(({struct Cyc_Absyn_Tqual _tmpFEB;_tmpFEB.print_const=0,_tmpFEB.q_volatile=0,_tmpFEB.q_restrict=1,_tmpFEB.real_const=0,_tmpFEB.loc=0U;_tmpFEB;}));});
goto _LL0;case 113U: _LLDD: _LLDE: {
# 1745 "parse.y"
struct Cyc_Absyn_TypeDecl*_tmp659=({struct Cyc_Absyn_TypeDecl*_tmp663=_cycalloc(sizeof(*_tmp663));({void*_tmp116C=(void*)({struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*_tmp662=_cycalloc(sizeof(*_tmp662));_tmp662->tag=1U,({struct Cyc_Absyn_Enumdecl*_tmp116B=({struct Cyc_Absyn_Enumdecl*_tmp661=_cycalloc(sizeof(*_tmp661));_tmp661->sc=Cyc_Absyn_Public,({struct _tuple0*_tmp116A=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});_tmp661->name=_tmp116A;}),({struct Cyc_Core_Opt*_tmp1169=({struct Cyc_Core_Opt*_tmp660=_cycalloc(sizeof(*_tmp660));({struct Cyc_List_List*_tmp1168=({Cyc_yyget_YY48(&(yyyvsp[3]).v);});_tmp660->v=_tmp1168;});_tmp660;});_tmp661->fields=_tmp1169;});_tmp661;});_tmp662->f1=_tmp116B;});_tmp662;});_tmp663->r=_tmp116C;}),({
unsigned _tmp1167=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp663->loc=_tmp1167;});_tmp663;});
# 1745
struct Cyc_Absyn_TypeDecl*ed=_tmp659;
# 1747
yyval=({union Cyc_YYSTYPE(*_tmp65A)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp65B=({struct Cyc_Parse_Type_specifier(*_tmp65C)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp65D=(void*)({struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp65E=_cycalloc(sizeof(*_tmp65E));_tmp65E->tag=10U,_tmp65E->f1=ed,_tmp65E->f2=0;_tmp65E;});unsigned _tmp65F=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp65C(_tmp65D,_tmp65F);});_tmp65A(_tmp65B);});
# 1749
goto _LL0;}case 114U: _LLDF: _LLE0:
# 1750 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp664)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp665=({struct Cyc_Parse_Type_specifier(*_tmp666)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp667=({void*(*_tmp668)(struct _tuple0*n,struct Cyc_Absyn_Enumdecl*d)=Cyc_Absyn_enum_type;struct _tuple0*_tmp669=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});struct Cyc_Absyn_Enumdecl*_tmp66A=0;_tmp668(_tmp669,_tmp66A);});unsigned _tmp66B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp666(_tmp667,_tmp66B);});_tmp664(_tmp665);});
goto _LL0;case 115U: _LLE1: _LLE2:
# 1752 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp66C)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp66D=({struct Cyc_Parse_Type_specifier(*_tmp66E)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp66F=({void*(*_tmp670)(struct Cyc_List_List*)=Cyc_Absyn_anon_enum_type;struct Cyc_List_List*_tmp671=({Cyc_yyget_YY48(&(yyyvsp[2]).v);});_tmp670(_tmp671);});unsigned _tmp672=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp66E(_tmp66F,_tmp672);});_tmp66C(_tmp66D);});
goto _LL0;case 116U: _LLE3: _LLE4:
# 1758 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp673)(struct Cyc_Absyn_Enumfield*yy1)=Cyc_YY47;struct Cyc_Absyn_Enumfield*_tmp674=({struct Cyc_Absyn_Enumfield*_tmp675=_cycalloc(sizeof(*_tmp675));({struct _tuple0*_tmp116D=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp675->name=_tmp116D;}),_tmp675->tag=0,_tmp675->loc=(unsigned)((yyyvsp[0]).l).first_line;_tmp675;});_tmp673(_tmp674);});
goto _LL0;case 117U: _LLE5: _LLE6:
# 1760 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp676)(struct Cyc_Absyn_Enumfield*yy1)=Cyc_YY47;struct Cyc_Absyn_Enumfield*_tmp677=({struct Cyc_Absyn_Enumfield*_tmp678=_cycalloc(sizeof(*_tmp678));({struct _tuple0*_tmp1170=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp678->name=_tmp1170;}),({struct Cyc_Absyn_Exp*_tmp116F=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp678->tag=_tmp116F;}),({unsigned _tmp116E=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp678->loc=_tmp116E;});_tmp678;});_tmp676(_tmp677);});
goto _LL0;case 118U: _LLE7: _LLE8:
# 1764 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp679)(struct Cyc_List_List*yy1)=Cyc_YY48;struct Cyc_List_List*_tmp67A=({struct Cyc_List_List*_tmp67B=_cycalloc(sizeof(*_tmp67B));({struct Cyc_Absyn_Enumfield*_tmp1171=({Cyc_yyget_YY47(&(yyyvsp[0]).v);});_tmp67B->hd=_tmp1171;}),_tmp67B->tl=0;_tmp67B;});_tmp679(_tmp67A);});
goto _LL0;case 119U: _LLE9: _LLEA:
# 1765 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp67C)(struct Cyc_List_List*yy1)=Cyc_YY48;struct Cyc_List_List*_tmp67D=({struct Cyc_List_List*_tmp67E=_cycalloc(sizeof(*_tmp67E));({struct Cyc_Absyn_Enumfield*_tmp1172=({Cyc_yyget_YY47(&(yyyvsp[0]).v);});_tmp67E->hd=_tmp1172;}),_tmp67E->tl=0;_tmp67E;});_tmp67C(_tmp67D);});
goto _LL0;case 120U: _LLEB: _LLEC:
# 1766 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp67F)(struct Cyc_List_List*yy1)=Cyc_YY48;struct Cyc_List_List*_tmp680=({struct Cyc_List_List*_tmp681=_cycalloc(sizeof(*_tmp681));({struct Cyc_Absyn_Enumfield*_tmp1174=({Cyc_yyget_YY47(&(yyyvsp[0]).v);});_tmp681->hd=_tmp1174;}),({struct Cyc_List_List*_tmp1173=({Cyc_yyget_YY48(&(yyyvsp[2]).v);});_tmp681->tl=_tmp1173;});_tmp681;});_tmp67F(_tmp680);});
goto _LL0;case 121U: _LLED: _LLEE:
# 1772 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp682)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp683=({struct Cyc_Parse_Type_specifier(*_tmp684)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp685=(void*)({struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*_tmp686=_cycalloc(sizeof(*_tmp686));_tmp686->tag=7U,({enum Cyc_Absyn_AggrKind _tmp1176=({Cyc_yyget_YY22(&(yyyvsp[0]).v);});_tmp686->f1=_tmp1176;}),({struct Cyc_List_List*_tmp1175=({Cyc_yyget_YY24(&(yyyvsp[2]).v);});_tmp686->f2=_tmp1175;});_tmp686;});unsigned _tmp687=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp684(_tmp685,_tmp687);});_tmp682(_tmp683);});
goto _LL0;case 122U: _LLEF: _LLF0: {
# 1778 "parse.y"
struct Cyc_List_List*ts=({struct Cyc_List_List*(*_tmp6A1)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp6A2)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp6A2;});struct Cyc_Absyn_Tvar*(*_tmp6A3)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmp6A4=(unsigned)((yyyvsp[3]).l).first_line;struct Cyc_List_List*_tmp6A5=({Cyc_yyget_YY40(&(yyyvsp[3]).v);});_tmp6A1(_tmp6A3,_tmp6A4,_tmp6A5);});
struct Cyc_List_List*exist_ts=({struct Cyc_List_List*(*_tmp69C)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp69D)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp69D;});struct Cyc_Absyn_Tvar*(*_tmp69E)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmp69F=(unsigned)((yyyvsp[5]).l).first_line;struct Cyc_List_List*_tmp6A0=({Cyc_yyget_YY40(&(yyyvsp[5]).v);});_tmp69C(_tmp69E,_tmp69F,_tmp6A0);});
struct Cyc_Absyn_TypeDecl*_tmp688=({struct Cyc_Absyn_TypeDecl*(*_tmp68F)(enum Cyc_Absyn_AggrKind,enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Absyn_AggrdeclImpl*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_aggr_tdecl;enum Cyc_Absyn_AggrKind _tmp690=({Cyc_yyget_YY22(&(yyyvsp[1]).v);});enum Cyc_Absyn_Scope _tmp691=Cyc_Absyn_Public;struct _tuple0*_tmp692=({Cyc_yyget_QualId_tok(&(yyyvsp[2]).v);});struct Cyc_List_List*_tmp693=ts;struct Cyc_Absyn_AggrdeclImpl*_tmp694=({struct Cyc_Absyn_AggrdeclImpl*(*_tmp695)(struct Cyc_List_List*exists,struct Cyc_List_List*po,struct Cyc_List_List*fs,int tagged)=Cyc_Absyn_aggrdecl_impl;struct Cyc_List_List*_tmp696=exist_ts;struct Cyc_List_List*_tmp697=({Cyc_yyget_YY50(&(yyyvsp[6]).v);});struct Cyc_List_List*_tmp698=({Cyc_yyget_YY24(&(yyyvsp[7]).v);});int _tmp699=1;_tmp695(_tmp696,_tmp697,_tmp698,_tmp699);});struct Cyc_List_List*_tmp69A=0;unsigned _tmp69B=(unsigned)({
# 1782
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp68F(_tmp690,_tmp691,_tmp692,_tmp693,_tmp694,_tmp69A,_tmp69B);});
# 1780
struct Cyc_Absyn_TypeDecl*td=_tmp688;
# 1783
yyval=({union Cyc_YYSTYPE(*_tmp689)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp68A=({struct Cyc_Parse_Type_specifier(*_tmp68B)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp68C=(void*)({struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp68D=_cycalloc(sizeof(*_tmp68D));_tmp68D->tag=10U,_tmp68D->f1=td,_tmp68D->f2=0;_tmp68D;});unsigned _tmp68E=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp68B(_tmp68C,_tmp68E);});_tmp689(_tmp68A);});
# 1785
goto _LL0;}case 123U: _LLF1: _LLF2: {
# 1789 "parse.y"
struct Cyc_List_List*ts=({struct Cyc_List_List*(*_tmp6BF)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp6C0)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp6C0;});struct Cyc_Absyn_Tvar*(*_tmp6C1)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmp6C2=(unsigned)((yyyvsp[2]).l).first_line;struct Cyc_List_List*_tmp6C3=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp6BF(_tmp6C1,_tmp6C2,_tmp6C3);});
struct Cyc_List_List*exist_ts=({struct Cyc_List_List*(*_tmp6BA)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp6BB)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp6BB;});struct Cyc_Absyn_Tvar*(*_tmp6BC)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmp6BD=(unsigned)((yyyvsp[4]).l).first_line;struct Cyc_List_List*_tmp6BE=({Cyc_yyget_YY40(&(yyyvsp[4]).v);});_tmp6BA(_tmp6BC,_tmp6BD,_tmp6BE);});
struct Cyc_Absyn_TypeDecl*_tmp6A6=({struct Cyc_Absyn_TypeDecl*(*_tmp6AD)(enum Cyc_Absyn_AggrKind,enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Absyn_AggrdeclImpl*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_aggr_tdecl;enum Cyc_Absyn_AggrKind _tmp6AE=({Cyc_yyget_YY22(&(yyyvsp[0]).v);});enum Cyc_Absyn_Scope _tmp6AF=Cyc_Absyn_Public;struct _tuple0*_tmp6B0=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmp6B1=ts;struct Cyc_Absyn_AggrdeclImpl*_tmp6B2=({struct Cyc_Absyn_AggrdeclImpl*(*_tmp6B3)(struct Cyc_List_List*exists,struct Cyc_List_List*po,struct Cyc_List_List*fs,int tagged)=Cyc_Absyn_aggrdecl_impl;struct Cyc_List_List*_tmp6B4=exist_ts;struct Cyc_List_List*_tmp6B5=({Cyc_yyget_YY50(&(yyyvsp[5]).v);});struct Cyc_List_List*_tmp6B6=({Cyc_yyget_YY24(&(yyyvsp[6]).v);});int _tmp6B7=0;_tmp6B3(_tmp6B4,_tmp6B5,_tmp6B6,_tmp6B7);});struct Cyc_List_List*_tmp6B8=0;unsigned _tmp6B9=(unsigned)({
# 1793
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp6AD(_tmp6AE,_tmp6AF,_tmp6B0,_tmp6B1,_tmp6B2,_tmp6B8,_tmp6B9);});
# 1791
struct Cyc_Absyn_TypeDecl*td=_tmp6A6;
# 1794
yyval=({union Cyc_YYSTYPE(*_tmp6A7)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp6A8=({struct Cyc_Parse_Type_specifier(*_tmp6A9)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp6AA=(void*)({struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp6AB=_cycalloc(sizeof(*_tmp6AB));_tmp6AB->tag=10U,_tmp6AB->f1=td,_tmp6AB->f2=0;_tmp6AB;});unsigned _tmp6AC=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp6A9(_tmp6AA,_tmp6AC);});_tmp6A7(_tmp6A8);});
# 1796
goto _LL0;}case 124U: _LLF3: _LLF4:
# 1797 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6C4)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp6C5=({struct Cyc_Parse_Type_specifier(*_tmp6C6)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp6C7=({void*(*_tmp6C8)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmp6C9=({union Cyc_Absyn_AggrInfo(*_tmp6CA)(enum Cyc_Absyn_AggrKind,struct _tuple0*,struct Cyc_Core_Opt*)=Cyc_Absyn_UnknownAggr;enum Cyc_Absyn_AggrKind _tmp6CB=({Cyc_yyget_YY22(&(yyyvsp[1]).v);});struct _tuple0*_tmp6CC=({Cyc_yyget_QualId_tok(&(yyyvsp[2]).v);});struct Cyc_Core_Opt*_tmp6CD=({struct Cyc_Core_Opt*_tmp6CE=_cycalloc(sizeof(*_tmp6CE));_tmp6CE->v=(void*)1;_tmp6CE;});_tmp6CA(_tmp6CB,_tmp6CC,_tmp6CD);});struct Cyc_List_List*_tmp6CF=({Cyc_yyget_YY40(&(yyyvsp[3]).v);});_tmp6C8(_tmp6C9,_tmp6CF);});unsigned _tmp6D0=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp6C6(_tmp6C7,_tmp6D0);});_tmp6C4(_tmp6C5);});
# 1800
goto _LL0;case 125U: _LLF5: _LLF6:
# 1801 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6D1)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp6D2=({struct Cyc_Parse_Type_specifier(*_tmp6D3)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp6D4=({void*(*_tmp6D5)(union Cyc_Absyn_AggrInfo,struct Cyc_List_List*args)=Cyc_Absyn_aggr_type;union Cyc_Absyn_AggrInfo _tmp6D6=({union Cyc_Absyn_AggrInfo(*_tmp6D7)(enum Cyc_Absyn_AggrKind,struct _tuple0*,struct Cyc_Core_Opt*)=Cyc_Absyn_UnknownAggr;enum Cyc_Absyn_AggrKind _tmp6D8=({Cyc_yyget_YY22(&(yyyvsp[0]).v);});struct _tuple0*_tmp6D9=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});struct Cyc_Core_Opt*_tmp6DA=0;_tmp6D7(_tmp6D8,_tmp6D9,_tmp6DA);});struct Cyc_List_List*_tmp6DB=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp6D5(_tmp6D6,_tmp6DB);});unsigned _tmp6DC=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp6D3(_tmp6D4,_tmp6DC);});_tmp6D1(_tmp6D2);});
goto _LL0;case 126U: _LLF7: _LLF8:
# 1806 "parse.y"
 yyval=({Cyc_YY40(0);});
goto _LL0;case 127U: _LLF9: _LLFA:
# 1808 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6DD)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp6DE=({struct Cyc_List_List*(*_tmp6DF)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp6E0=({Cyc_yyget_YY40(&(yyyvsp[1]).v);});_tmp6DF(_tmp6E0);});_tmp6DD(_tmp6DE);});
goto _LL0;case 128U: _LLFB: _LLFC:
# 1812 "parse.y"
 yyval=({Cyc_YY22(Cyc_Absyn_StructA);});
goto _LL0;case 129U: _LLFD: _LLFE:
# 1813 "parse.y"
 yyval=({Cyc_YY22(Cyc_Absyn_UnionA);});
goto _LL0;case 130U: _LLFF: _LL100:
# 1818 "parse.y"
 yyval=({Cyc_YY24(0);});
goto _LL0;case 131U: _LL101: _LL102: {
# 1820 "parse.y"
struct Cyc_List_List*decls=0;
{struct Cyc_List_List*x=({Cyc_yyget_YY25(&(yyyvsp[0]).v);});for(0;x != 0;x=x->tl){
decls=({Cyc_List_imp_append((struct Cyc_List_List*)x->hd,decls);});}}{
# 1824
struct Cyc_List_List*tags=({Cyc_Parse_get_aggrfield_tags(decls);});
if(tags != 0)
({({void(*_tmp1178)(void(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*env,struct Cyc_List_List*x)=({void(*_tmp6E1)(void(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*env,struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_List_List*,struct Cyc_Absyn_Aggrfield*),struct Cyc_List_List*env,struct Cyc_List_List*x))Cyc_List_iter_c;_tmp6E1;});struct Cyc_List_List*_tmp1177=tags;_tmp1178(Cyc_Parse_substitute_aggrfield_tags,_tmp1177,decls);});});
# 1825
yyval=({Cyc_YY24(decls);});
# 1829
goto _LL0;}}case 132U: _LL103: _LL104:
# 1834 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6E2)(struct Cyc_List_List*yy1)=Cyc_YY25;struct Cyc_List_List*_tmp6E3=({struct Cyc_List_List*_tmp6E4=_cycalloc(sizeof(*_tmp6E4));({struct Cyc_List_List*_tmp1179=({Cyc_yyget_YY24(&(yyyvsp[0]).v);});_tmp6E4->hd=_tmp1179;}),_tmp6E4->tl=0;_tmp6E4;});_tmp6E2(_tmp6E3);});
goto _LL0;case 133U: _LL105: _LL106:
# 1836 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6E5)(struct Cyc_List_List*yy1)=Cyc_YY25;struct Cyc_List_List*_tmp6E6=({struct Cyc_List_List*_tmp6E7=_cycalloc(sizeof(*_tmp6E7));({struct Cyc_List_List*_tmp117B=({Cyc_yyget_YY24(&(yyyvsp[1]).v);});_tmp6E7->hd=_tmp117B;}),({struct Cyc_List_List*_tmp117A=({Cyc_yyget_YY25(&(yyyvsp[0]).v);});_tmp6E7->tl=_tmp117A;});_tmp6E7;});_tmp6E5(_tmp6E6);});
goto _LL0;case 134U: _LL107: _LL108:
# 1840 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6E8)(struct _tuple14*yy1)=Cyc_YY19;struct _tuple14*_tmp6E9=({struct _tuple14*(*_tmp6EA)(struct _tuple14*x)=({struct _tuple14*(*_tmp6EB)(struct _tuple14*x)=(struct _tuple14*(*)(struct _tuple14*x))Cyc_Parse_flat_imp_rev;_tmp6EB;});struct _tuple14*_tmp6EC=({Cyc_yyget_YY19(&(yyyvsp[0]).v);});_tmp6EA(_tmp6EC);});_tmp6E8(_tmp6E9);});
goto _LL0;case 135U: _LL109: _LL10A:
# 1846 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6ED)(struct _tuple14*yy1)=Cyc_YY19;struct _tuple14*_tmp6EE=({struct _tuple14*_tmp6EF=_region_malloc(yyr,sizeof(*_tmp6EF));_tmp6EF->tl=0,({struct _tuple13 _tmp117C=({Cyc_yyget_YY18(&(yyyvsp[0]).v);});_tmp6EF->hd=_tmp117C;});_tmp6EF;});_tmp6ED(_tmp6EE);});
goto _LL0;case 136U: _LL10B: _LL10C:
# 1848 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6F0)(struct _tuple14*yy1)=Cyc_YY19;struct _tuple14*_tmp6F1=({struct _tuple14*_tmp6F2=_region_malloc(yyr,sizeof(*_tmp6F2));({struct _tuple14*_tmp117E=({Cyc_yyget_YY19(&(yyyvsp[0]).v);});_tmp6F2->tl=_tmp117E;}),({struct _tuple13 _tmp117D=({Cyc_yyget_YY18(&(yyyvsp[2]).v);});_tmp6F2->hd=_tmp117D;});_tmp6F2;});_tmp6F0(_tmp6F1);});
goto _LL0;case 137U: _LL10D: _LL10E:
# 1853 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6F3)(struct _tuple13 yy1)=Cyc_YY18;struct _tuple13 _tmp6F4=({struct _tuple13 _tmpFEC;({struct Cyc_Parse_Declarator _tmp117F=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});_tmpFEC.f1=_tmp117F;}),_tmpFEC.f2=0;_tmpFEC;});_tmp6F3(_tmp6F4);});
goto _LL0;case 138U: _LL10F: _LL110:
# 1855 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp6F5)(struct _tuple13 yy1)=Cyc_YY18;struct _tuple13 _tmp6F6=({struct _tuple13 _tmpFED;({struct Cyc_Parse_Declarator _tmp1181=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});_tmpFED.f1=_tmp1181;}),({struct Cyc_Absyn_Exp*_tmp1180=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmpFED.f2=_tmp1180;});_tmpFED;});_tmp6F5(_tmp6F6);});
goto _LL0;case 139U: _LL111: _LL112: {
# 1861 "parse.y"
struct _RegionHandle _tmp6F7=_new_region("temp");struct _RegionHandle*temp=& _tmp6F7;_push_region(temp);
{struct _tuple26 _stmttmp18=({Cyc_yyget_YY35(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp6FA;struct Cyc_Parse_Type_specifier _tmp6F9;struct Cyc_Absyn_Tqual _tmp6F8;_LL483: _tmp6F8=_stmttmp18.f1;_tmp6F9=_stmttmp18.f2;_tmp6FA=_stmttmp18.f3;_LL484: {struct Cyc_Absyn_Tqual tq=_tmp6F8;struct Cyc_Parse_Type_specifier tspecs=_tmp6F9;struct Cyc_List_List*atts=_tmp6FA;
if(tq.loc == (unsigned)0)tq.loc=(unsigned)((yyyvsp[0]).l).first_line;{struct _tuple12*decls=0;
# 1865
struct Cyc_List_List*widths_and_reqs=0;
{struct Cyc_List_List*x=({Cyc_yyget_YY29(&(yyyvsp[1]).v);});for(0;x != 0;x=x->tl){
struct _tuple25*_stmttmp19=(struct _tuple25*)x->hd;struct Cyc_Absyn_Exp*_tmp6FD;struct Cyc_Absyn_Exp*_tmp6FC;struct Cyc_Parse_Declarator _tmp6FB;_LL486: _tmp6FB=_stmttmp19->f1;_tmp6FC=_stmttmp19->f2;_tmp6FD=_stmttmp19->f3;_LL487: {struct Cyc_Parse_Declarator d=_tmp6FB;struct Cyc_Absyn_Exp*wd=_tmp6FC;struct Cyc_Absyn_Exp*wh=_tmp6FD;
decls=({struct _tuple12*_tmp6FE=_region_malloc(temp,sizeof(*_tmp6FE));_tmp6FE->tl=decls,_tmp6FE->hd=d;_tmp6FE;});
widths_and_reqs=({struct Cyc_List_List*_tmp700=_region_malloc(temp,sizeof(*_tmp700));
({struct _tuple17*_tmp1182=({struct _tuple17*_tmp6FF=_region_malloc(temp,sizeof(*_tmp6FF));_tmp6FF->f1=wd,_tmp6FF->f2=wh;_tmp6FF;});_tmp700->hd=_tmp1182;}),_tmp700->tl=widths_and_reqs;_tmp700;});}}}
# 1872
decls=({({struct _tuple12*(*_tmp1183)(struct _tuple12*x)=({struct _tuple12*(*_tmp701)(struct _tuple12*x)=(struct _tuple12*(*)(struct _tuple12*x))Cyc_Parse_flat_imp_rev;_tmp701;});_tmp1183(decls);});});
widths_and_reqs=({Cyc_List_imp_rev(widths_and_reqs);});{
void*t=({Cyc_Parse_speclist2typ(tspecs,(unsigned)((yyyvsp[0]).l).first_line);});
struct Cyc_List_List*info=({struct Cyc_List_List*(*_tmp709)(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_rzip;struct _RegionHandle*_tmp70A=temp;struct _RegionHandle*_tmp70B=temp;struct Cyc_List_List*_tmp70C=({Cyc_Parse_apply_tmss(temp,tq,t,decls,atts);});struct Cyc_List_List*_tmp70D=widths_and_reqs;_tmp709(_tmp70A,_tmp70B,_tmp70C,_tmp70D);});
# 1878
yyval=({union Cyc_YYSTYPE(*_tmp702)(struct Cyc_List_List*yy1)=Cyc_YY24;struct Cyc_List_List*_tmp703=({struct Cyc_List_List*(*_tmp704)(struct Cyc_Absyn_Aggrfield*(*f)(unsigned,struct _tuple18*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp705)(struct Cyc_Absyn_Aggrfield*(*f)(unsigned,struct _tuple18*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Aggrfield*(*f)(unsigned,struct _tuple18*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp705;});struct Cyc_Absyn_Aggrfield*(*_tmp706)(unsigned loc,struct _tuple18*field_info)=Cyc_Parse_make_aggr_field;unsigned _tmp707=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct Cyc_List_List*_tmp708=info;_tmp704(_tmp706,_tmp707,_tmp708);});_tmp702(_tmp703);});
# 1880
_npop_handler(0U);goto _LL0;}}}}
# 1862
;_pop_region();}case 140U: _LL113: _LL114:
# 1888 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp70E)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp70F=({struct _tuple26 _tmpFEE;({struct Cyc_Absyn_Tqual _tmp1185=({Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);});_tmpFEE.f1=_tmp1185;}),({struct Cyc_Parse_Type_specifier _tmp1184=({Cyc_yyget_YY21(&(yyyvsp[0]).v);});_tmpFEE.f2=_tmp1184;}),_tmpFEE.f3=0;_tmpFEE;});_tmp70E(_tmp70F);});
goto _LL0;case 141U: _LL115: _LL116: {
# 1890 "parse.y"
struct _tuple26 two=({Cyc_yyget_YY35(&(yyyvsp[1]).v);});yyval=({union Cyc_YYSTYPE(*_tmp710)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp711=({struct _tuple26 _tmpFEF;_tmpFEF.f1=two.f1,({struct Cyc_Parse_Type_specifier _tmp1186=({struct Cyc_Parse_Type_specifier(*_tmp712)(unsigned loc,struct Cyc_Parse_Type_specifier s1,struct Cyc_Parse_Type_specifier s2)=Cyc_Parse_combine_specifiers;unsigned _tmp713=(unsigned)((yyyvsp[0]).l).first_line;struct Cyc_Parse_Type_specifier _tmp714=({Cyc_yyget_YY21(&(yyyvsp[0]).v);});struct Cyc_Parse_Type_specifier _tmp715=two.f2;_tmp712(_tmp713,_tmp714,_tmp715);});_tmpFEF.f2=_tmp1186;}),_tmpFEF.f3=two.f3;_tmpFEF;});_tmp710(_tmp711);});
goto _LL0;}case 142U: _LL117: _LL118:
# 1892 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp716)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp717=({struct _tuple26 _tmpFF0;({struct Cyc_Absyn_Tqual _tmp1188=({Cyc_yyget_YY23(&(yyyvsp[0]).v);});_tmpFF0.f1=_tmp1188;}),({struct Cyc_Parse_Type_specifier _tmp1187=({Cyc_Parse_empty_spec(0U);});_tmpFF0.f2=_tmp1187;}),_tmpFF0.f3=0;_tmpFF0;});_tmp716(_tmp717);});
goto _LL0;case 143U: _LL119: _LL11A: {
# 1894 "parse.y"
struct _tuple26 two=({Cyc_yyget_YY35(&(yyyvsp[1]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp718)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp719=({struct _tuple26 _tmpFF1;({struct Cyc_Absyn_Tqual _tmp1189=({struct Cyc_Absyn_Tqual(*_tmp71A)(struct Cyc_Absyn_Tqual,struct Cyc_Absyn_Tqual)=Cyc_Absyn_combine_tqual;struct Cyc_Absyn_Tqual _tmp71B=({Cyc_yyget_YY23(&(yyyvsp[0]).v);});struct Cyc_Absyn_Tqual _tmp71C=two.f1;_tmp71A(_tmp71B,_tmp71C);});_tmpFF1.f1=_tmp1189;}),_tmpFF1.f2=two.f2,_tmpFF1.f3=two.f3;_tmpFF1;});_tmp718(_tmp719);});
goto _LL0;}case 144U: _LL11B: _LL11C:
# 1897 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp71D)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp71E=({struct _tuple26 _tmpFF2;({struct Cyc_Absyn_Tqual _tmp118C=({Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);});_tmpFF2.f1=_tmp118C;}),({struct Cyc_Parse_Type_specifier _tmp118B=({Cyc_Parse_empty_spec(0U);});_tmpFF2.f2=_tmp118B;}),({struct Cyc_List_List*_tmp118A=({Cyc_yyget_YY45(&(yyyvsp[0]).v);});_tmpFF2.f3=_tmp118A;});_tmpFF2;});_tmp71D(_tmp71E);});
goto _LL0;case 145U: _LL11D: _LL11E: {
# 1899 "parse.y"
struct _tuple26 two=({Cyc_yyget_YY35(&(yyyvsp[1]).v);});yyval=({union Cyc_YYSTYPE(*_tmp71F)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp720=({struct _tuple26 _tmpFF3;_tmpFF3.f1=two.f1,_tmpFF3.f2=two.f2,({struct Cyc_List_List*_tmp118D=({struct Cyc_List_List*(*_tmp721)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmp722=({Cyc_yyget_YY45(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp723=two.f3;_tmp721(_tmp722,_tmp723);});_tmpFF3.f3=_tmp118D;});_tmpFF3;});_tmp71F(_tmp720);});
goto _LL0;}case 146U: _LL11F: _LL120:
# 1905 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp724)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp725=({struct _tuple26 _tmpFF4;({struct Cyc_Absyn_Tqual _tmp118F=({Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);});_tmpFF4.f1=_tmp118F;}),({struct Cyc_Parse_Type_specifier _tmp118E=({Cyc_yyget_YY21(&(yyyvsp[0]).v);});_tmpFF4.f2=_tmp118E;}),_tmpFF4.f3=0;_tmpFF4;});_tmp724(_tmp725);});
goto _LL0;case 147U: _LL121: _LL122: {
# 1907 "parse.y"
struct _tuple26 two=({Cyc_yyget_YY35(&(yyyvsp[1]).v);});yyval=({union Cyc_YYSTYPE(*_tmp726)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp727=({struct _tuple26 _tmpFF5;_tmpFF5.f1=two.f1,({struct Cyc_Parse_Type_specifier _tmp1190=({struct Cyc_Parse_Type_specifier(*_tmp728)(unsigned loc,struct Cyc_Parse_Type_specifier s1,struct Cyc_Parse_Type_specifier s2)=Cyc_Parse_combine_specifiers;unsigned _tmp729=(unsigned)((yyyvsp[0]).l).first_line;struct Cyc_Parse_Type_specifier _tmp72A=({Cyc_yyget_YY21(&(yyyvsp[0]).v);});struct Cyc_Parse_Type_specifier _tmp72B=two.f2;_tmp728(_tmp729,_tmp72A,_tmp72B);});_tmpFF5.f2=_tmp1190;}),_tmpFF5.f3=two.f3;_tmpFF5;});_tmp726(_tmp727);});
goto _LL0;}case 148U: _LL123: _LL124:
# 1909 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp72C)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp72D=({struct _tuple26 _tmpFF6;({struct Cyc_Absyn_Tqual _tmp1192=({Cyc_yyget_YY23(&(yyyvsp[0]).v);});_tmpFF6.f1=_tmp1192;}),({struct Cyc_Parse_Type_specifier _tmp1191=({Cyc_Parse_empty_spec(0U);});_tmpFF6.f2=_tmp1191;}),_tmpFF6.f3=0;_tmpFF6;});_tmp72C(_tmp72D);});
goto _LL0;case 149U: _LL125: _LL126: {
# 1911 "parse.y"
struct _tuple26 two=({Cyc_yyget_YY35(&(yyyvsp[1]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp72E)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp72F=({struct _tuple26 _tmpFF7;({struct Cyc_Absyn_Tqual _tmp1193=({struct Cyc_Absyn_Tqual(*_tmp730)(struct Cyc_Absyn_Tqual,struct Cyc_Absyn_Tqual)=Cyc_Absyn_combine_tqual;struct Cyc_Absyn_Tqual _tmp731=({Cyc_yyget_YY23(&(yyyvsp[0]).v);});struct Cyc_Absyn_Tqual _tmp732=two.f1;_tmp730(_tmp731,_tmp732);});_tmpFF7.f1=_tmp1193;}),_tmpFF7.f2=two.f2,_tmpFF7.f3=two.f3;_tmpFF7;});_tmp72E(_tmp72F);});
goto _LL0;}case 150U: _LL127: _LL128:
# 1914 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp733)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp734=({struct _tuple26 _tmpFF8;({struct Cyc_Absyn_Tqual _tmp1196=({Cyc_Absyn_empty_tqual((unsigned)((yyyvsp[0]).l).first_line);});_tmpFF8.f1=_tmp1196;}),({struct Cyc_Parse_Type_specifier _tmp1195=({Cyc_Parse_empty_spec(0U);});_tmpFF8.f2=_tmp1195;}),({struct Cyc_List_List*_tmp1194=({Cyc_yyget_YY45(&(yyyvsp[0]).v);});_tmpFF8.f3=_tmp1194;});_tmpFF8;});_tmp733(_tmp734);});
goto _LL0;case 151U: _LL129: _LL12A: {
# 1916 "parse.y"
struct _tuple26 two=({Cyc_yyget_YY35(&(yyyvsp[1]).v);});yyval=({union Cyc_YYSTYPE(*_tmp735)(struct _tuple26 yy1)=Cyc_YY35;struct _tuple26 _tmp736=({struct _tuple26 _tmpFF9;_tmpFF9.f1=two.f1,_tmpFF9.f2=two.f2,({struct Cyc_List_List*_tmp1197=({struct Cyc_List_List*(*_tmp737)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_append;struct Cyc_List_List*_tmp738=({Cyc_yyget_YY45(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp739=two.f3;_tmp737(_tmp738,_tmp739);});_tmpFF9.f3=_tmp1197;});_tmpFF9;});_tmp735(_tmp736);});
goto _LL0;}case 152U: _LL12B: _LL12C:
# 1920 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp73A)(struct Cyc_List_List*yy1)=Cyc_YY29;struct Cyc_List_List*_tmp73B=({struct Cyc_List_List*(*_tmp73C)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp73D=({Cyc_yyget_YY29(&(yyyvsp[0]).v);});_tmp73C(_tmp73D);});_tmp73A(_tmp73B);});
goto _LL0;case 153U: _LL12D: _LL12E:
# 1926 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp73E)(struct Cyc_List_List*yy1)=Cyc_YY29;struct Cyc_List_List*_tmp73F=({struct Cyc_List_List*_tmp740=_region_malloc(yyr,sizeof(*_tmp740));({struct _tuple25*_tmp1198=({Cyc_yyget_YY28(&(yyyvsp[0]).v);});_tmp740->hd=_tmp1198;}),_tmp740->tl=0;_tmp740;});_tmp73E(_tmp73F);});
goto _LL0;case 154U: _LL12F: _LL130:
# 1928 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp741)(struct Cyc_List_List*yy1)=Cyc_YY29;struct Cyc_List_List*_tmp742=({struct Cyc_List_List*_tmp743=_region_malloc(yyr,sizeof(*_tmp743));({struct _tuple25*_tmp119A=({Cyc_yyget_YY28(&(yyyvsp[2]).v);});_tmp743->hd=_tmp119A;}),({struct Cyc_List_List*_tmp1199=({Cyc_yyget_YY29(&(yyyvsp[0]).v);});_tmp743->tl=_tmp1199;});_tmp743;});_tmp741(_tmp742);});
goto _LL0;case 155U: _LL131: _LL132:
# 1933 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp744)(struct _tuple25*yy1)=Cyc_YY28;struct _tuple25*_tmp745=({struct _tuple25*_tmp746=_region_malloc(yyr,sizeof(*_tmp746));({struct Cyc_Parse_Declarator _tmp119C=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});_tmp746->f1=_tmp119C;}),_tmp746->f2=0,({struct Cyc_Absyn_Exp*_tmp119B=({Cyc_yyget_YY57(&(yyyvsp[1]).v);});_tmp746->f3=_tmp119B;});_tmp746;});_tmp744(_tmp745);});
goto _LL0;case 156U: _LL133: _LL134:
# 1937 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp747)(struct _tuple25*yy1)=Cyc_YY28;struct _tuple25*_tmp748=({struct _tuple25*_tmp74C=_region_malloc(yyr,sizeof(*_tmp74C));({struct _tuple0*_tmp11A1=({struct _tuple0*_tmp74B=_cycalloc(sizeof(*_tmp74B));({union Cyc_Absyn_Nmspace _tmp11A0=({Cyc_Absyn_Rel_n(0);});_tmp74B->f1=_tmp11A0;}),({struct _fat_ptr*_tmp119F=({struct _fat_ptr*_tmp74A=_cycalloc(sizeof(*_tmp74A));({struct _fat_ptr _tmp119E=({const char*_tmp749="";_tag_fat(_tmp749,sizeof(char),1U);});*_tmp74A=_tmp119E;});_tmp74A;});_tmp74B->f2=_tmp119F;});_tmp74B;});(_tmp74C->f1).id=_tmp11A1;}),(_tmp74C->f1).varloc=0U,(_tmp74C->f1).tms=0,({struct Cyc_Absyn_Exp*_tmp119D=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});_tmp74C->f2=_tmp119D;}),_tmp74C->f3=0;_tmp74C;});_tmp747(_tmp748);});
# 1939
goto _LL0;case 157U: _LL135: _LL136:
# 1942 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp74D)(struct _tuple25*yy1)=Cyc_YY28;struct _tuple25*_tmp74E=({struct _tuple25*_tmp752=_region_malloc(yyr,sizeof(*_tmp752));({struct _tuple0*_tmp11A5=({struct _tuple0*_tmp751=_cycalloc(sizeof(*_tmp751));({union Cyc_Absyn_Nmspace _tmp11A4=({Cyc_Absyn_Rel_n(0);});_tmp751->f1=_tmp11A4;}),({struct _fat_ptr*_tmp11A3=({struct _fat_ptr*_tmp750=_cycalloc(sizeof(*_tmp750));({struct _fat_ptr _tmp11A2=({const char*_tmp74F="";_tag_fat(_tmp74F,sizeof(char),1U);});*_tmp750=_tmp11A2;});_tmp750;});_tmp751->f2=_tmp11A3;});_tmp751;});(_tmp752->f1).id=_tmp11A5;}),(_tmp752->f1).varloc=0U,(_tmp752->f1).tms=0,_tmp752->f2=0,_tmp752->f3=0;_tmp752;});_tmp74D(_tmp74E);});
# 1944
goto _LL0;case 158U: _LL137: _LL138:
# 1945 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp753)(struct _tuple25*yy1)=Cyc_YY28;struct _tuple25*_tmp754=({struct _tuple25*_tmp755=_region_malloc(yyr,sizeof(*_tmp755));({struct Cyc_Parse_Declarator _tmp11A7=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});_tmp755->f1=_tmp11A7;}),({struct Cyc_Absyn_Exp*_tmp11A6=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp755->f2=_tmp11A6;}),_tmp755->f3=0;_tmp755;});_tmp753(_tmp754);});
goto _LL0;case 159U: _LL139: _LL13A:
# 1949 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp756)(struct Cyc_Absyn_Exp*yy1)=Cyc_YY57;struct Cyc_Absyn_Exp*_tmp757=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp756(_tmp757);});
goto _LL0;case 160U: _LL13B: _LL13C:
# 1950 "parse.y"
 yyval=({Cyc_YY57(0);});
goto _LL0;case 161U: _LL13D: _LL13E:
# 1957 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp758)(struct Cyc_List_List*yy1)=Cyc_YY66;struct Cyc_List_List*_tmp759=({struct Cyc_List_List*_tmp75C=_cycalloc(sizeof(*_tmp75C));({void*_tmp11A8=({void*(*_tmp75A)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp75B=({Cyc_yyget_YY40(&(yyyvsp[1]).v);});_tmp75A(_tmp75B);});_tmp75C->hd=_tmp11A8;}),_tmp75C->tl=0;_tmp75C;});_tmp758(_tmp759);});
# 1959
goto _LL0;case 162U: _LL13F: _LL140:
# 1962
 yyval=({union Cyc_YYSTYPE(*_tmp75D)(struct Cyc_List_List*yy1)=Cyc_YY66;struct Cyc_List_List*_tmp75E=({struct Cyc_List_List*_tmp761=_cycalloc(sizeof(*_tmp761));({void*_tmp11AA=({void*(*_tmp75F)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp760=({Cyc_yyget_YY40(&(yyyvsp[3]).v);});_tmp75F(_tmp760);});_tmp761->hd=_tmp11AA;}),({struct Cyc_List_List*_tmp11A9=({Cyc_yyget_YY66(&(yyyvsp[0]).v);});_tmp761->tl=_tmp11A9;});_tmp761;});_tmp75D(_tmp75E);});
# 1964
goto _LL0;case 163U: _LL141: _LL142:
# 1969 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp762)(void*yy1)=Cyc_YY49;void*_tmp763=({void*(*_tmp764)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp765=({Cyc_yyget_YY66(&(yyyvsp[2]).v);});_tmp764(_tmp765);});_tmp762(_tmp763);});
# 1971
goto _LL0;case 164U: _LL143: _LL144:
# 1971 "parse.y"
 yyval=({Cyc_YY49(0);});
goto _LL0;case 165U: _LL145: _LL146:
# 1977 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp766)(void*yy1)=Cyc_YY49;void*_tmp767=({void*(*_tmp768)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp769=({Cyc_yyget_YY66(&(yyyvsp[2]).v);});_tmp768(_tmp769);});_tmp766(_tmp767);});
# 1979
goto _LL0;case 166U: _LL147: _LL148:
# 1979 "parse.y"
 yyval=({Cyc_YY49(0);});
goto _LL0;case 167U: _LL149: _LL14A:
# 1986 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp76A)(struct Cyc_List_List*yy1)=Cyc_YY64;struct Cyc_List_List*_tmp76B=({Cyc_yyget_YY34(&(yyyvsp[2]).v);});_tmp76A(_tmp76B);});
# 1988
goto _LL0;case 168U: _LL14B: _LL14C:
# 1990 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp76C)(struct Cyc_List_List*yy1)=Cyc_YY64;struct Cyc_List_List*_tmp76D=({Cyc_Absyn_nothrow();});_tmp76C(_tmp76D);});
# 1992
goto _LL0;case 169U: _LL14D: _LL14E:
# 1994 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp76E)(struct Cyc_List_List*yy1)=Cyc_YY64;struct Cyc_List_List*_tmp76F=({Cyc_Absyn_throwsany();});_tmp76E(_tmp76F);});
# 1996
goto _LL0;case 170U: _LL14F: _LL150:
# 1996 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp770)(struct Cyc_List_List*yy1)=Cyc_YY64;struct Cyc_List_List*_tmp771=({Cyc_Absyn_throwsany();});_tmp770(_tmp771);});
goto _LL0;case 171U: _LL151: _LL152:
# 2002 "parse.y"
 yyval=({Cyc_YY65(1);});
# 2004
goto _LL0;case 172U: _LL153: _LL154:
# 2004 "parse.y"
 yyval=({Cyc_YY65(0);});
goto _LL0;case 173U: _LL155: _LL156:
# 2008 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp772)(struct Cyc_Absyn_Exp*yy1)=Cyc_YY57;struct Cyc_Absyn_Exp*_tmp773=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp772(_tmp773);});
goto _LL0;case 174U: _LL157: _LL158:
# 2009 "parse.y"
 yyval=({Cyc_YY57(0);});
goto _LL0;case 175U: _LL159: _LL15A: {
# 2015 "parse.y"
int is_extensible=({Cyc_yyget_YY31(&(yyyvsp[0]).v);});
struct Cyc_List_List*ts=({struct Cyc_List_List*(*_tmp783)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp784)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp784;});struct Cyc_Absyn_Tvar*(*_tmp785)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmp786=(unsigned)((yyyvsp[2]).l).first_line;struct Cyc_List_List*_tmp787=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp783(_tmp785,_tmp786,_tmp787);});
struct Cyc_Absyn_TypeDecl*_tmp774=({struct Cyc_Absyn_TypeDecl*(*_tmp77B)(enum Cyc_Absyn_Scope,struct _tuple0*,struct Cyc_List_List*ts,struct Cyc_Core_Opt*fs,int is_extensible,unsigned)=Cyc_Absyn_datatype_tdecl;enum Cyc_Absyn_Scope _tmp77C=Cyc_Absyn_Public;struct _tuple0*_tmp77D=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmp77E=ts;struct Cyc_Core_Opt*_tmp77F=({struct Cyc_Core_Opt*_tmp780=_cycalloc(sizeof(*_tmp780));({struct Cyc_List_List*_tmp11AB=({Cyc_yyget_YY34(&(yyyvsp[4]).v);});_tmp780->v=_tmp11AB;});_tmp780;});int _tmp781=is_extensible;unsigned _tmp782=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp77B(_tmp77C,_tmp77D,_tmp77E,_tmp77F,_tmp781,_tmp782);});
# 2017
struct Cyc_Absyn_TypeDecl*dd=_tmp774;
# 2019
yyval=({union Cyc_YYSTYPE(*_tmp775)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp776=({struct Cyc_Parse_Type_specifier(*_tmp777)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp778=(void*)({struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*_tmp779=_cycalloc(sizeof(*_tmp779));_tmp779->tag=10U,_tmp779->f1=dd,_tmp779->f2=0;_tmp779;});unsigned _tmp77A=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp777(_tmp778,_tmp77A);});_tmp775(_tmp776);});
# 2021
goto _LL0;}case 176U: _LL15B: _LL15C: {
# 2022 "parse.y"
int is_extensible=({Cyc_yyget_YY31(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp788)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp789=({struct Cyc_Parse_Type_specifier(*_tmp78A)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp78B=({void*(*_tmp78C)(union Cyc_Absyn_DatatypeInfo,struct Cyc_List_List*args)=Cyc_Absyn_datatype_type;union Cyc_Absyn_DatatypeInfo _tmp78D=({union Cyc_Absyn_DatatypeInfo(*_tmp78E)(struct Cyc_Absyn_UnknownDatatypeInfo)=Cyc_Absyn_UnknownDatatype;struct Cyc_Absyn_UnknownDatatypeInfo _tmp78F=({struct Cyc_Absyn_UnknownDatatypeInfo _tmpFFA;({struct _tuple0*_tmp11AC=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});_tmpFFA.name=_tmp11AC;}),_tmpFFA.is_extensible=is_extensible;_tmpFFA;});_tmp78E(_tmp78F);});struct Cyc_List_List*_tmp790=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp78C(_tmp78D,_tmp790);});unsigned _tmp791=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp78A(_tmp78B,_tmp791);});_tmp788(_tmp789);});
# 2025
goto _LL0;}case 177U: _LL15D: _LL15E: {
# 2026 "parse.y"
int is_extensible=({Cyc_yyget_YY31(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp792)(struct Cyc_Parse_Type_specifier yy1)=Cyc_YY21;struct Cyc_Parse_Type_specifier _tmp793=({struct Cyc_Parse_Type_specifier(*_tmp794)(void*t,unsigned loc)=Cyc_Parse_type_spec;void*_tmp795=({void*(*_tmp796)(union Cyc_Absyn_DatatypeFieldInfo,struct Cyc_List_List*args)=Cyc_Absyn_datatype_field_type;union Cyc_Absyn_DatatypeFieldInfo _tmp797=({union Cyc_Absyn_DatatypeFieldInfo(*_tmp798)(struct Cyc_Absyn_UnknownDatatypeFieldInfo)=Cyc_Absyn_UnknownDatatypefield;struct Cyc_Absyn_UnknownDatatypeFieldInfo _tmp799=({struct Cyc_Absyn_UnknownDatatypeFieldInfo _tmpFFB;({struct _tuple0*_tmp11AE=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});_tmpFFB.datatype_name=_tmp11AE;}),({struct _tuple0*_tmp11AD=({Cyc_yyget_QualId_tok(&(yyyvsp[3]).v);});_tmpFFB.field_name=_tmp11AD;}),_tmpFFB.is_extensible=is_extensible;_tmpFFB;});_tmp798(_tmp799);});struct Cyc_List_List*_tmp79A=({Cyc_yyget_YY40(&(yyyvsp[4]).v);});_tmp796(_tmp797,_tmp79A);});unsigned _tmp79B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp794(_tmp795,_tmp79B);});_tmp792(_tmp793);});
# 2029
goto _LL0;}case 178U: _LL15F: _LL160:
# 2032 "parse.y"
 yyval=({Cyc_YY31(0);});
goto _LL0;case 179U: _LL161: _LL162:
# 2033 "parse.y"
 yyval=({Cyc_YY31(1);});
goto _LL0;case 180U: _LL163: _LL164:
# 2037 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp79C)(struct Cyc_List_List*yy1)=Cyc_YY34;struct Cyc_List_List*_tmp79D=({struct Cyc_List_List*_tmp79E=_cycalloc(sizeof(*_tmp79E));({struct Cyc_Absyn_Datatypefield*_tmp11AF=({Cyc_yyget_YY33(&(yyyvsp[0]).v);});_tmp79E->hd=_tmp11AF;}),_tmp79E->tl=0;_tmp79E;});_tmp79C(_tmp79D);});
goto _LL0;case 181U: _LL165: _LL166:
# 2038 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp79F)(struct Cyc_List_List*yy1)=Cyc_YY34;struct Cyc_List_List*_tmp7A0=({struct Cyc_List_List*_tmp7A1=_cycalloc(sizeof(*_tmp7A1));({struct Cyc_Absyn_Datatypefield*_tmp11B0=({Cyc_yyget_YY33(&(yyyvsp[0]).v);});_tmp7A1->hd=_tmp11B0;}),_tmp7A1->tl=0;_tmp7A1;});_tmp79F(_tmp7A0);});
goto _LL0;case 182U: _LL167: _LL168:
# 2039 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7A2)(struct Cyc_List_List*yy1)=Cyc_YY34;struct Cyc_List_List*_tmp7A3=({struct Cyc_List_List*_tmp7A4=_cycalloc(sizeof(*_tmp7A4));({struct Cyc_Absyn_Datatypefield*_tmp11B2=({Cyc_yyget_YY33(&(yyyvsp[0]).v);});_tmp7A4->hd=_tmp11B2;}),({struct Cyc_List_List*_tmp11B1=({Cyc_yyget_YY34(&(yyyvsp[2]).v);});_tmp7A4->tl=_tmp11B1;});_tmp7A4;});_tmp7A2(_tmp7A3);});
goto _LL0;case 183U: _LL169: _LL16A:
# 2040 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7A5)(struct Cyc_List_List*yy1)=Cyc_YY34;struct Cyc_List_List*_tmp7A6=({struct Cyc_List_List*_tmp7A7=_cycalloc(sizeof(*_tmp7A7));({struct Cyc_Absyn_Datatypefield*_tmp11B4=({Cyc_yyget_YY33(&(yyyvsp[0]).v);});_tmp7A7->hd=_tmp11B4;}),({struct Cyc_List_List*_tmp11B3=({Cyc_yyget_YY34(&(yyyvsp[2]).v);});_tmp7A7->tl=_tmp11B3;});_tmp7A7;});_tmp7A5(_tmp7A6);});
goto _LL0;case 184U: _LL16B: _LL16C:
# 2044 "parse.y"
 yyval=({Cyc_YY32(Cyc_Absyn_Public);});
goto _LL0;case 185U: _LL16D: _LL16E:
# 2045 "parse.y"
 yyval=({Cyc_YY32(Cyc_Absyn_Extern);});
goto _LL0;case 186U: _LL16F: _LL170:
# 2046 "parse.y"
 yyval=({Cyc_YY32(Cyc_Absyn_Static);});
goto _LL0;case 187U: _LL171: _LL172:
# 2050 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7A8)(struct Cyc_Absyn_Datatypefield*yy1)=Cyc_YY33;struct Cyc_Absyn_Datatypefield*_tmp7A9=({struct Cyc_Absyn_Datatypefield*_tmp7AA=_cycalloc(sizeof(*_tmp7AA));({struct _tuple0*_tmp11B7=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});_tmp7AA->name=_tmp11B7;}),_tmp7AA->typs=0,({unsigned _tmp11B6=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp7AA->loc=_tmp11B6;}),({enum Cyc_Absyn_Scope _tmp11B5=({Cyc_yyget_YY32(&(yyyvsp[0]).v);});_tmp7AA->sc=_tmp11B5;});_tmp7AA;});_tmp7A8(_tmp7A9);});
goto _LL0;case 188U: _LL173: _LL174: {
# 2052 "parse.y"
struct Cyc_List_List*typs=({struct Cyc_List_List*(*_tmp7AE)(struct _tuple20*(*f)(unsigned,struct _tuple9*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp7AF)(struct _tuple20*(*f)(unsigned,struct _tuple9*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _tuple20*(*f)(unsigned,struct _tuple9*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp7AF;});struct _tuple20*(*_tmp7B0)(unsigned loc,struct _tuple9*t)=Cyc_Parse_get_tqual_typ;unsigned _tmp7B1=(unsigned)((yyyvsp[3]).l).first_line;struct Cyc_List_List*_tmp7B2=({struct Cyc_List_List*(*_tmp7B3)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp7B4=({Cyc_yyget_YY38(&(yyyvsp[3]).v);});_tmp7B3(_tmp7B4);});_tmp7AE(_tmp7B0,_tmp7B1,_tmp7B2);});
yyval=({union Cyc_YYSTYPE(*_tmp7AB)(struct Cyc_Absyn_Datatypefield*yy1)=Cyc_YY33;struct Cyc_Absyn_Datatypefield*_tmp7AC=({struct Cyc_Absyn_Datatypefield*_tmp7AD=_cycalloc(sizeof(*_tmp7AD));({struct _tuple0*_tmp11BA=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});_tmp7AD->name=_tmp11BA;}),_tmp7AD->typs=typs,({unsigned _tmp11B9=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp7AD->loc=_tmp11B9;}),({enum Cyc_Absyn_Scope _tmp11B8=({Cyc_yyget_YY32(&(yyyvsp[0]).v);});_tmp7AD->sc=_tmp11B8;});_tmp7AD;});_tmp7AB(_tmp7AC);});
goto _LL0;}case 189U: _LL175: _LL176:
# 2059 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 190U: _LL177: _LL178: {
# 2061 "parse.y"
struct Cyc_Parse_Declarator two=({Cyc_yyget_YY27(&(yyyvsp[1]).v);});
# 2063
((yyyvsp[0]).l).first_line=((yyyvsp[1]).l).first_line;
yyval=({union Cyc_YYSTYPE(*_tmp7B5)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7B6=({struct Cyc_Parse_Declarator _tmpFFC;_tmpFFC.id=two.id,_tmpFFC.varloc=two.varloc,({struct Cyc_List_List*_tmp11BB=({struct Cyc_List_List*(*_tmp7B7)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp7B8=({Cyc_yyget_YY26(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp7B9=two.tms;_tmp7B7(_tmp7B8,_tmp7B9);});_tmpFFC.tms=_tmp11BB;});_tmpFFC;});_tmp7B5(_tmp7B6);});
goto _LL0;}case 191U: _LL179: _LL17A:
# 2070 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 192U: _LL17B: _LL17C: {
# 2072 "parse.y"
struct Cyc_Parse_Declarator two=({Cyc_yyget_YY27(&(yyyvsp[1]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp7BA)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7BB=({struct Cyc_Parse_Declarator _tmpFFD;_tmpFFD.id=two.id,_tmpFFD.varloc=two.varloc,({struct Cyc_List_List*_tmp11BC=({struct Cyc_List_List*(*_tmp7BC)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp7BD=({Cyc_yyget_YY26(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp7BE=two.tms;_tmp7BC(_tmp7BD,_tmp7BE);});_tmpFFD.tms=_tmp11BC;});_tmpFFD;});_tmp7BA(_tmp7BB);});
goto _LL0;}case 193U: _LL17D: _LL17E:
# 2079 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7BF)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7C0=({struct Cyc_Parse_Declarator _tmpFFE;({struct _tuple0*_tmp11BD=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmpFFE.id=_tmp11BD;}),_tmpFFE.varloc=(unsigned)((yyyvsp[0]).l).first_line,_tmpFFE.tms=0;_tmpFFE;});_tmp7BF(_tmp7C0);});
# 2081
goto _LL0;case 194U: _LL17F: _LL180:
# 2082 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 195U: _LL181: _LL182: {
# 2086 "parse.y"
struct Cyc_Parse_Declarator d=({Cyc_yyget_YY27(&(yyyvsp[2]).v);});
({struct Cyc_List_List*_tmp11C0=({struct Cyc_List_List*_tmp7C2=_region_malloc(yyr,sizeof(*_tmp7C2));({void*_tmp11BF=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp7C1=_region_malloc(yyr,sizeof(*_tmp7C1));_tmp7C1->tag=5U,_tmp7C1->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmp11BE=({Cyc_yyget_YY45(&(yyyvsp[1]).v);});_tmp7C1->f2=_tmp11BE;});_tmp7C1;});_tmp7C2->hd=_tmp11BF;}),_tmp7C2->tl=d.tms;_tmp7C2;});d.tms=_tmp11C0;});
yyval=(yyyvsp[2]).v;
# 2090
goto _LL0;}case 196U: _LL183: _LL184:
# 2091 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7C3)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7C4=({struct Cyc_Parse_Declarator _tmpFFF;({struct _tuple0*_tmp11C6=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).id;_tmpFFF.id=_tmp11C6;}),({unsigned _tmp11C5=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).varloc;_tmpFFF.varloc=_tmp11C5;}),({struct Cyc_List_List*_tmp11C4=({struct Cyc_List_List*_tmp7C6=_region_malloc(yyr,sizeof(*_tmp7C6));({void*_tmp11C3=(void*)({struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_tmp7C5=_region_malloc(yyr,sizeof(*_tmp7C5));_tmp7C5->tag=0U,({void*_tmp11C2=({Cyc_yyget_YY51(&(yyyvsp[3]).v);});_tmp7C5->f1=_tmp11C2;}),_tmp7C5->f2=(unsigned)((yyyvsp[3]).l).first_line;_tmp7C5;});_tmp7C6->hd=_tmp11C3;}),({struct Cyc_List_List*_tmp11C1=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).tms;_tmp7C6->tl=_tmp11C1;});_tmp7C6;});_tmpFFF.tms=_tmp11C4;});_tmpFFF;});_tmp7C3(_tmp7C4);});
goto _LL0;case 197U: _LL185: _LL186:
# 2093 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7C7)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7C8=({struct Cyc_Parse_Declarator _tmp1000;({struct _tuple0*_tmp11CD=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).id;_tmp1000.id=_tmp11CD;}),({unsigned _tmp11CC=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).varloc;_tmp1000.varloc=_tmp11CC;}),({
struct Cyc_List_List*_tmp11CB=({struct Cyc_List_List*_tmp7CA=_region_malloc(yyr,sizeof(*_tmp7CA));({void*_tmp11CA=(void*)({struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_tmp7C9=_region_malloc(yyr,sizeof(*_tmp7C9));_tmp7C9->tag=1U,({struct Cyc_Absyn_Exp*_tmp11C9=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp7C9->f1=_tmp11C9;}),({void*_tmp11C8=({Cyc_yyget_YY51(&(yyyvsp[4]).v);});_tmp7C9->f2=_tmp11C8;}),_tmp7C9->f3=(unsigned)((yyyvsp[4]).l).first_line;_tmp7C9;});_tmp7CA->hd=_tmp11CA;}),({struct Cyc_List_List*_tmp11C7=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).tms;_tmp7CA->tl=_tmp11C7;});_tmp7CA;});_tmp1000.tms=_tmp11CB;});_tmp1000;});_tmp7C7(_tmp7C8);});
goto _LL0;case 198U: _LL187: _LL188: {
# 2099 "parse.y"
struct _tuple27*_stmttmp1A=({Cyc_yyget_YY39(&(yyyvsp[2]).v);});struct Cyc_List_List*_tmp7CF;void*_tmp7CE;struct Cyc_Absyn_VarargInfo*_tmp7CD;int _tmp7CC;struct Cyc_List_List*_tmp7CB;_LL489: _tmp7CB=_stmttmp1A->f1;_tmp7CC=_stmttmp1A->f2;_tmp7CD=_stmttmp1A->f3;_tmp7CE=_stmttmp1A->f4;_tmp7CF=_stmttmp1A->f5;_LL48A: {struct Cyc_List_List*lis=_tmp7CB;int b=_tmp7CC;struct Cyc_Absyn_VarargInfo*c=_tmp7CD;void*eff=_tmp7CE;struct Cyc_List_List*po=_tmp7CF;
struct Cyc_Absyn_Exp*req=({Cyc_yyget_YY57(&(yyyvsp[4]).v);});
struct Cyc_Absyn_Exp*ens=({Cyc_yyget_YY57(&(yyyvsp[5]).v);});
struct Cyc_List_List*ieff=({struct Cyc_List_List*(*_tmp7D8)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp7D9=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp7DA=({Cyc_yyget_YY49(&(yyyvsp[6]).v);});_tmp7D8(_tmp7D9,_tmp7DA);});
struct Cyc_List_List*oeff=({struct Cyc_List_List*(*_tmp7D5)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp7D6=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp7D7=({Cyc_yyget_YY49(&(yyyvsp[7]).v);});_tmp7D5(_tmp7D6,_tmp7D7);});
struct Cyc_List_List*throws=({Cyc_yyget_YY64(&(yyyvsp[8]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp7D0)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7D1=({struct Cyc_Parse_Declarator _tmp1001;({struct _tuple0*_tmp11D4=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).id;_tmp1001.id=_tmp11D4;}),({unsigned _tmp11D3=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).varloc;_tmp1001.varloc=_tmp11D3;}),({struct Cyc_List_List*_tmp11D2=({struct Cyc_List_List*_tmp7D4=_region_malloc(yyr,sizeof(*_tmp7D4));({void*_tmp11D1=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp7D3=_region_malloc(yyr,sizeof(*_tmp7D3));_tmp7D3->tag=3U,({void*_tmp11D0=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp7D2=_region_malloc(yyr,sizeof(*_tmp7D2));_tmp7D2->tag=1U,_tmp7D2->f1=lis,_tmp7D2->f2=b,_tmp7D2->f3=c,_tmp7D2->f4=eff,_tmp7D2->f5=po,_tmp7D2->f6=req,_tmp7D2->f7=ens,_tmp7D2->f8=ieff,_tmp7D2->f9=oeff,_tmp7D2->f10=throws,({int _tmp11CF=({Cyc_yyget_YY65(&(yyyvsp[9]).v);});_tmp7D2->f11=_tmp11CF;});_tmp7D2;});_tmp7D3->f1=_tmp11D0;});_tmp7D3;});_tmp7D4->hd=_tmp11D1;}),({struct Cyc_List_List*_tmp11CE=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).tms;_tmp7D4->tl=_tmp11CE;});_tmp7D4;});_tmp1001.tms=_tmp11D2;});_tmp1001;});_tmp7D0(_tmp7D1);});
# 2107
goto _LL0;}}case 199U: _LL189: _LL18A: {
# 2113 "parse.y"
struct Cyc_List_List*ieff=({struct Cyc_List_List*(*_tmp7E3)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp7E4=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp7E5=({Cyc_yyget_YY49(&(yyyvsp[7]).v);});_tmp7E3(_tmp7E4,_tmp7E5);});
struct Cyc_List_List*oeff=({struct Cyc_List_List*(*_tmp7E0)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp7E1=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp7E2=({Cyc_yyget_YY49(&(yyyvsp[8]).v);});_tmp7E0(_tmp7E1,_tmp7E2);});
struct Cyc_List_List*throws=({Cyc_yyget_YY64(&(yyyvsp[9]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp7DB)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7DC=({struct Cyc_Parse_Declarator _tmp1002;({struct _tuple0*_tmp11DF=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).id;_tmp1002.id=_tmp11DF;}),({unsigned _tmp11DE=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).varloc;_tmp1002.varloc=_tmp11DE;}),({
struct Cyc_List_List*_tmp11DD=({struct Cyc_List_List*_tmp7DF=_region_malloc(yyr,sizeof(*_tmp7DF));({void*_tmp11DC=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp7DE=_region_malloc(yyr,sizeof(*_tmp7DE));
_tmp7DE->tag=3U,({void*_tmp11DB=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp7DD=_region_malloc(yyr,sizeof(*_tmp7DD));_tmp7DD->tag=1U,_tmp7DD->f1=0,_tmp7DD->f2=0,_tmp7DD->f3=0,({
# 2120
void*_tmp11DA=({Cyc_yyget_YY49(&(yyyvsp[2]).v);});_tmp7DD->f4=_tmp11DA;}),({struct Cyc_List_List*_tmp11D9=({Cyc_yyget_YY50(&(yyyvsp[3]).v);});_tmp7DD->f5=_tmp11D9;}),({struct Cyc_Absyn_Exp*_tmp11D8=({Cyc_yyget_YY57(&(yyyvsp[5]).v);});_tmp7DD->f6=_tmp11D8;}),({struct Cyc_Absyn_Exp*_tmp11D7=({Cyc_yyget_YY57(&(yyyvsp[6]).v);});_tmp7DD->f7=_tmp11D7;}),_tmp7DD->f8=ieff,_tmp7DD->f9=oeff,_tmp7DD->f10=throws,({
# 2122
int _tmp11D6=({Cyc_yyget_YY65(&(yyyvsp[10]).v);});_tmp7DD->f11=_tmp11D6;});_tmp7DD;});
# 2118
_tmp7DE->f1=_tmp11DB;});_tmp7DE;});
# 2117
_tmp7DF->hd=_tmp11DC;}),({
# 2123
struct Cyc_List_List*_tmp11D5=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).tms;_tmp7DF->tl=_tmp11D5;});_tmp7DF;});
# 2117
_tmp1002.tms=_tmp11DD;});_tmp1002;});_tmp7DB(_tmp7DC);});
# 2125
goto _LL0;}case 200U: _LL18B: _LL18C:
# 2126 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7E6)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7E7=({struct Cyc_Parse_Declarator _tmp1003;({struct _tuple0*_tmp11E7=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).id;_tmp1003.id=_tmp11E7;}),({unsigned _tmp11E6=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).varloc;_tmp1003.varloc=_tmp11E6;}),({struct Cyc_List_List*_tmp11E5=({struct Cyc_List_List*_tmp7EA=_region_malloc(yyr,sizeof(*_tmp7EA));({void*_tmp11E4=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp7E9=_region_malloc(yyr,sizeof(*_tmp7E9));
_tmp7E9->tag=3U,({void*_tmp11E3=(void*)({struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*_tmp7E8=_region_malloc(yyr,sizeof(*_tmp7E8));_tmp7E8->tag=0U,({struct Cyc_List_List*_tmp11E2=({Cyc_yyget_YY36(&(yyyvsp[2]).v);});_tmp7E8->f1=_tmp11E2;}),({unsigned _tmp11E1=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp7E8->f2=_tmp11E1;});_tmp7E8;});_tmp7E9->f1=_tmp11E3;});_tmp7E9;});
# 2126
_tmp7EA->hd=_tmp11E4;}),({
struct Cyc_List_List*_tmp11E0=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).tms;_tmp7EA->tl=_tmp11E0;});_tmp7EA;});
# 2126
_tmp1003.tms=_tmp11E5;});_tmp1003;});_tmp7E6(_tmp7E7);});
# 2128
goto _LL0;case 201U: _LL18D: _LL18E: {
# 2130
struct Cyc_List_List*_tmp7EB=({struct Cyc_List_List*(*_tmp7F0)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp7F1)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp7F1;});struct Cyc_Absyn_Tvar*(*_tmp7F2)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmp7F3=(unsigned)({yylloc.first_line=((yyyvsp[1]).l).first_line;((yyyvsp[1]).l).first_line;});struct Cyc_List_List*_tmp7F4=({struct Cyc_List_List*(*_tmp7F5)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp7F6=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp7F5(_tmp7F6);});_tmp7F0(_tmp7F2,_tmp7F3,_tmp7F4);});struct Cyc_List_List*ts=_tmp7EB;
yyval=({union Cyc_YYSTYPE(*_tmp7EC)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7ED=({struct Cyc_Parse_Declarator _tmp1004;({struct _tuple0*_tmp11ED=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).id;_tmp1004.id=_tmp11ED;}),({unsigned _tmp11EC=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).varloc;_tmp1004.varloc=_tmp11EC;}),({struct Cyc_List_List*_tmp11EB=({struct Cyc_List_List*_tmp7EF=_region_malloc(yyr,sizeof(*_tmp7EF));({void*_tmp11EA=(void*)({struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*_tmp7EE=_region_malloc(yyr,sizeof(*_tmp7EE));_tmp7EE->tag=4U,_tmp7EE->f1=ts,({unsigned _tmp11E9=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp7EE->f2=_tmp11E9;}),_tmp7EE->f3=0;_tmp7EE;});_tmp7EF->hd=_tmp11EA;}),({struct Cyc_List_List*_tmp11E8=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).tms;_tmp7EF->tl=_tmp11E8;});_tmp7EF;});_tmp1004.tms=_tmp11EB;});_tmp1004;});_tmp7EC(_tmp7ED);});
# 2133
goto _LL0;}case 202U: _LL18F: _LL190:
# 2134 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7F7)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7F8=({struct Cyc_Parse_Declarator _tmp1005;({struct _tuple0*_tmp11F3=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).id;_tmp1005.id=_tmp11F3;}),({unsigned _tmp11F2=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).varloc;_tmp1005.varloc=_tmp11F2;}),({struct Cyc_List_List*_tmp11F1=({struct Cyc_List_List*_tmp7FA=_region_malloc(yyr,sizeof(*_tmp7FA));({void*_tmp11F0=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp7F9=_region_malloc(yyr,sizeof(*_tmp7F9));_tmp7F9->tag=5U,_tmp7F9->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmp11EF=({Cyc_yyget_YY45(&(yyyvsp[1]).v);});_tmp7F9->f2=_tmp11EF;});_tmp7F9;});_tmp7FA->hd=_tmp11F0;}),({
struct Cyc_List_List*_tmp11EE=({Cyc_yyget_YY27(&(yyyvsp[0]).v);}).tms;_tmp7FA->tl=_tmp11EE;});_tmp7FA;});
# 2134
_tmp1005.tms=_tmp11F1;});_tmp1005;});_tmp7F7(_tmp7F8);});
# 2137
goto _LL0;case 203U: _LL191: _LL192:
# 2142 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7FB)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7FC=({struct Cyc_Parse_Declarator _tmp1006;({struct _tuple0*_tmp11F4=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp1006.id=_tmp11F4;}),_tmp1006.varloc=(unsigned)((yyyvsp[0]).l).first_line,_tmp1006.tms=0;_tmp1006;});_tmp7FB(_tmp7FC);});
goto _LL0;case 204U: _LL193: _LL194:
# 2144 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp7FD)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp7FE=({struct Cyc_Parse_Declarator _tmp1007;({struct _tuple0*_tmp11F5=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmp1007.id=_tmp11F5;}),_tmp1007.varloc=(unsigned)((yyyvsp[0]).l).first_line,_tmp1007.tms=0;_tmp1007;});_tmp7FD(_tmp7FE);});
goto _LL0;case 205U: _LL195: _LL196:
# 2146 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 206U: _LL197: _LL198: {
# 2150 "parse.y"
struct Cyc_Parse_Declarator d=({Cyc_yyget_YY27(&(yyyvsp[2]).v);});
({struct Cyc_List_List*_tmp11F8=({struct Cyc_List_List*_tmp800=_region_malloc(yyr,sizeof(*_tmp800));({void*_tmp11F7=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp7FF=_region_malloc(yyr,sizeof(*_tmp7FF));_tmp7FF->tag=5U,_tmp7FF->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmp11F6=({Cyc_yyget_YY45(&(yyyvsp[1]).v);});_tmp7FF->f2=_tmp11F6;});_tmp7FF;});_tmp800->hd=_tmp11F7;}),_tmp800->tl=d.tms;_tmp800;});d.tms=_tmp11F8;});
yyval=(yyyvsp[2]).v;
# 2154
goto _LL0;}case 207U: _LL199: _LL19A: {
# 2155 "parse.y"
struct Cyc_Parse_Declarator one=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp801)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp802=({struct Cyc_Parse_Declarator _tmp1008;_tmp1008.id=one.id,_tmp1008.varloc=one.varloc,({
struct Cyc_List_List*_tmp11FB=({struct Cyc_List_List*_tmp804=_region_malloc(yyr,sizeof(*_tmp804));({void*_tmp11FA=(void*)({struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_tmp803=_region_malloc(yyr,sizeof(*_tmp803));_tmp803->tag=0U,({void*_tmp11F9=({Cyc_yyget_YY51(&(yyyvsp[3]).v);});_tmp803->f1=_tmp11F9;}),_tmp803->f2=(unsigned)((yyyvsp[3]).l).first_line;_tmp803;});_tmp804->hd=_tmp11FA;}),_tmp804->tl=one.tms;_tmp804;});_tmp1008.tms=_tmp11FB;});_tmp1008;});_tmp801(_tmp802);});
goto _LL0;}case 208U: _LL19B: _LL19C: {
# 2159 "parse.y"
struct Cyc_Parse_Declarator one=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp805)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp806=({struct Cyc_Parse_Declarator _tmp1009;_tmp1009.id=one.id,_tmp1009.varloc=one.varloc,({
struct Cyc_List_List*_tmp11FF=({struct Cyc_List_List*_tmp808=_region_malloc(yyr,sizeof(*_tmp808));({void*_tmp11FE=(void*)({struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_tmp807=_region_malloc(yyr,sizeof(*_tmp807));_tmp807->tag=1U,({struct Cyc_Absyn_Exp*_tmp11FD=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp807->f1=_tmp11FD;}),({void*_tmp11FC=({Cyc_yyget_YY51(&(yyyvsp[4]).v);});_tmp807->f2=_tmp11FC;}),_tmp807->f3=(unsigned)((yyyvsp[4]).l).first_line;_tmp807;});_tmp808->hd=_tmp11FE;}),_tmp808->tl=one.tms;_tmp808;});_tmp1009.tms=_tmp11FF;});_tmp1009;});_tmp805(_tmp806);});
# 2163
goto _LL0;}case 209U: _LL19D: _LL19E: {
# 2169 "parse.y"
struct _tuple27*_stmttmp1B=({Cyc_yyget_YY39(&(yyyvsp[2]).v);});struct Cyc_List_List*_tmp80D;void*_tmp80C;struct Cyc_Absyn_VarargInfo*_tmp80B;int _tmp80A;struct Cyc_List_List*_tmp809;_LL48C: _tmp809=_stmttmp1B->f1;_tmp80A=_stmttmp1B->f2;_tmp80B=_stmttmp1B->f3;_tmp80C=_stmttmp1B->f4;_tmp80D=_stmttmp1B->f5;_LL48D: {struct Cyc_List_List*lis=_tmp809;int b=_tmp80A;struct Cyc_Absyn_VarargInfo*c=_tmp80B;void*eff=_tmp80C;struct Cyc_List_List*po=_tmp80D;
struct Cyc_Absyn_Exp*req=({Cyc_yyget_YY57(&(yyyvsp[4]).v);});
struct Cyc_Absyn_Exp*ens=({Cyc_yyget_YY57(&(yyyvsp[5]).v);});
struct Cyc_Parse_Declarator one=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});
struct Cyc_List_List*ieff=({struct Cyc_List_List*(*_tmp816)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp817=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp818=({Cyc_yyget_YY49(&(yyyvsp[6]).v);});_tmp816(_tmp817,_tmp818);});
struct Cyc_List_List*oeff=({struct Cyc_List_List*(*_tmp813)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp814=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp815=({Cyc_yyget_YY49(&(yyyvsp[7]).v);});_tmp813(_tmp814,_tmp815);});
struct Cyc_List_List*throws=({Cyc_yyget_YY64(&(yyyvsp[8]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp80E)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp80F=({struct Cyc_Parse_Declarator _tmp100A;_tmp100A.id=one.id,_tmp100A.varloc=one.varloc,({
struct Cyc_List_List*_tmp1203=({struct Cyc_List_List*_tmp812=_region_malloc(yyr,sizeof(*_tmp812));({void*_tmp1202=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp811=_region_malloc(yyr,sizeof(*_tmp811));
_tmp811->tag=3U,({void*_tmp1201=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp810=_region_malloc(yyr,sizeof(*_tmp810));
_tmp810->tag=1U,_tmp810->f1=lis,_tmp810->f2=b,_tmp810->f3=c,_tmp810->f4=eff,_tmp810->f5=po,_tmp810->f6=req,_tmp810->f7=ens,_tmp810->f8=ieff,_tmp810->f9=oeff,_tmp810->f10=throws,({int _tmp1200=({Cyc_yyget_YY65(&(yyyvsp[9]).v);});_tmp810->f11=_tmp1200;});_tmp810;});
# 2178
_tmp811->f1=_tmp1201;});_tmp811;});
# 2177
_tmp812->hd=_tmp1202;}),_tmp812->tl=one.tms;_tmp812;});_tmp100A.tms=_tmp1203;});_tmp100A;});_tmp80E(_tmp80F);});
# 2182
goto _LL0;}}case 210U: _LL19F: _LL1A0: {
# 2188 "parse.y"
struct Cyc_Parse_Declarator one=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});
struct Cyc_List_List*ieff=({struct Cyc_List_List*(*_tmp821)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp822=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp823=({Cyc_yyget_YY49(&(yyyvsp[7]).v);});_tmp821(_tmp822,_tmp823);});
struct Cyc_List_List*oeff=({struct Cyc_List_List*(*_tmp81E)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp81F=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp820=({Cyc_yyget_YY49(&(yyyvsp[8]).v);});_tmp81E(_tmp81F,_tmp820);});
struct Cyc_List_List*throws=({Cyc_yyget_YY64(&(yyyvsp[9]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp819)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp81A=({struct Cyc_Parse_Declarator _tmp100B;_tmp100B.id=one.id,_tmp100B.varloc=one.varloc,({
struct Cyc_List_List*_tmp120B=({struct Cyc_List_List*_tmp81D=_region_malloc(yyr,sizeof(*_tmp81D));({void*_tmp120A=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp81C=_region_malloc(yyr,sizeof(*_tmp81C));
_tmp81C->tag=3U,({void*_tmp1209=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp81B=_region_malloc(yyr,sizeof(*_tmp81B));_tmp81B->tag=1U,_tmp81B->f1=0,_tmp81B->f2=0,_tmp81B->f3=0,({
# 2196
void*_tmp1208=({Cyc_yyget_YY49(&(yyyvsp[2]).v);});_tmp81B->f4=_tmp1208;}),({struct Cyc_List_List*_tmp1207=({Cyc_yyget_YY50(&(yyyvsp[3]).v);});_tmp81B->f5=_tmp1207;}),({struct Cyc_Absyn_Exp*_tmp1206=({Cyc_yyget_YY57(&(yyyvsp[5]).v);});_tmp81B->f6=_tmp1206;}),({struct Cyc_Absyn_Exp*_tmp1205=({Cyc_yyget_YY57(&(yyyvsp[6]).v);});_tmp81B->f7=_tmp1205;}),_tmp81B->f8=ieff,_tmp81B->f9=oeff,_tmp81B->f10=throws,({
int _tmp1204=({Cyc_yyget_YY65(&(yyyvsp[10]).v);});_tmp81B->f11=_tmp1204;});_tmp81B;});
# 2194
_tmp81C->f1=_tmp1209;});_tmp81C;});
# 2193
_tmp81D->hd=_tmp120A;}),_tmp81D->tl=one.tms;_tmp81D;});_tmp100B.tms=_tmp120B;});_tmp100B;});_tmp819(_tmp81A);});
# 2200
goto _LL0;}case 211U: _LL1A1: _LL1A2: {
# 2201 "parse.y"
struct Cyc_Parse_Declarator one=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp824)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp825=({struct Cyc_Parse_Declarator _tmp100C;_tmp100C.id=one.id,_tmp100C.varloc=one.varloc,({struct Cyc_List_List*_tmp1210=({struct Cyc_List_List*_tmp828=_region_malloc(yyr,sizeof(*_tmp828));({void*_tmp120F=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp827=_region_malloc(yyr,sizeof(*_tmp827));_tmp827->tag=3U,({void*_tmp120E=(void*)({struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*_tmp826=_region_malloc(yyr,sizeof(*_tmp826));_tmp826->tag=0U,({struct Cyc_List_List*_tmp120D=({Cyc_yyget_YY36(&(yyyvsp[2]).v);});_tmp826->f1=_tmp120D;}),({unsigned _tmp120C=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp826->f2=_tmp120C;});_tmp826;});_tmp827->f1=_tmp120E;});_tmp827;});_tmp828->hd=_tmp120F;}),_tmp828->tl=one.tms;_tmp828;});_tmp100C.tms=_tmp1210;});_tmp100C;});_tmp824(_tmp825);});
goto _LL0;}case 212U: _LL1A3: _LL1A4: {
# 2205
struct Cyc_List_List*_tmp829=({struct Cyc_List_List*(*_tmp82E)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp82F)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp82F;});struct Cyc_Absyn_Tvar*(*_tmp830)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmp831=(unsigned)({yylloc.first_line=((yyyvsp[1]).l).first_line;((yyyvsp[1]).l).first_line;});struct Cyc_List_List*_tmp832=({struct Cyc_List_List*(*_tmp833)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp834=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp833(_tmp834);});_tmp82E(_tmp830,_tmp831,_tmp832);});struct Cyc_List_List*ts=_tmp829;
struct Cyc_Parse_Declarator one=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp82A)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp82B=({struct Cyc_Parse_Declarator _tmp100D;_tmp100D.id=one.id,_tmp100D.varloc=one.varloc,({struct Cyc_List_List*_tmp1213=({struct Cyc_List_List*_tmp82D=_region_malloc(yyr,sizeof(*_tmp82D));({void*_tmp1212=(void*)({struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*_tmp82C=_region_malloc(yyr,sizeof(*_tmp82C));_tmp82C->tag=4U,_tmp82C->f1=ts,({unsigned _tmp1211=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp82C->f2=_tmp1211;}),_tmp82C->f3=0;_tmp82C;});_tmp82D->hd=_tmp1212;}),_tmp82D->tl=one.tms;_tmp82D;});_tmp100D.tms=_tmp1213;});_tmp100D;});_tmp82A(_tmp82B);});
# 2209
goto _LL0;}case 213U: _LL1A5: _LL1A6: {
# 2210 "parse.y"
struct Cyc_Parse_Declarator one=({Cyc_yyget_YY27(&(yyyvsp[0]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp835)(struct Cyc_Parse_Declarator yy1)=Cyc_YY27;struct Cyc_Parse_Declarator _tmp836=({struct Cyc_Parse_Declarator _tmp100E;_tmp100E.id=one.id,_tmp100E.varloc=one.varloc,({struct Cyc_List_List*_tmp1216=({struct Cyc_List_List*_tmp838=_region_malloc(yyr,sizeof(*_tmp838));({void*_tmp1215=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp837=_region_malloc(yyr,sizeof(*_tmp837));_tmp837->tag=5U,_tmp837->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmp1214=({Cyc_yyget_YY45(&(yyyvsp[1]).v);});_tmp837->f2=_tmp1214;});_tmp837;});_tmp838->hd=_tmp1215;}),_tmp838->tl=one.tms;_tmp838;});_tmp100E.tms=_tmp1216;});_tmp100E;});_tmp835(_tmp836);});
# 2213
goto _LL0;}case 214U: _LL1A7: _LL1A8:
# 2217 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 215U: _LL1A9: _LL1AA:
# 2218 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp839)(struct Cyc_List_List*yy1)=Cyc_YY26;struct Cyc_List_List*_tmp83A=({struct Cyc_List_List*(*_tmp83B)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp83C=({Cyc_yyget_YY26(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp83D=({Cyc_yyget_YY26(&(yyyvsp[1]).v);});_tmp83B(_tmp83C,_tmp83D);});_tmp839(_tmp83A);});
goto _LL0;case 216U: _LL1AB: _LL1AC: {
# 2223 "parse.y"
struct Cyc_List_List*ans=0;
if(({Cyc_yyget_YY45(&(yyyvsp[3]).v);})!= 0)
ans=({struct Cyc_List_List*_tmp83F=_region_malloc(yyr,sizeof(*_tmp83F));({void*_tmp1218=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp83E=_region_malloc(yyr,sizeof(*_tmp83E));_tmp83E->tag=5U,_tmp83E->f1=(unsigned)((yyyvsp[3]).l).first_line,({struct Cyc_List_List*_tmp1217=({Cyc_yyget_YY45(&(yyyvsp[3]).v);});_tmp83E->f2=_tmp1217;});_tmp83E;});_tmp83F->hd=_tmp1218;}),_tmp83F->tl=ans;_tmp83F;});{
# 2224
struct Cyc_Absyn_PtrLoc*ptrloc=0;
# 2228
struct _tuple22 _stmttmp1C=*({Cyc_yyget_YY1(&(yyyvsp[0]).v);});void*_tmp842;void*_tmp841;unsigned _tmp840;_LL48F: _tmp840=_stmttmp1C.f1;_tmp841=_stmttmp1C.f2;_tmp842=_stmttmp1C.f3;_LL490: {unsigned ploc=_tmp840;void*nullable=_tmp841;void*bound=_tmp842;
if(Cyc_Absyn_porting_c_code)
ptrloc=({struct Cyc_Absyn_PtrLoc*_tmp843=_cycalloc(sizeof(*_tmp843));_tmp843->ptr_loc=ploc,_tmp843->rgn_loc=(unsigned)((yyyvsp[2]).l).first_line,_tmp843->zt_loc=(unsigned)((yyyvsp[1]).l).first_line;_tmp843;});{
# 2229
void*mod=({void*(*_tmp845)(struct _RegionHandle*r,struct Cyc_Absyn_PtrLoc*loc,void*nullable,void*bound,void*eff,struct Cyc_List_List*pqs,struct Cyc_Absyn_Tqual tqs)=Cyc_Parse_make_pointer_mod;struct _RegionHandle*_tmp846=yyr;struct Cyc_Absyn_PtrLoc*_tmp847=ptrloc;void*_tmp848=nullable;void*_tmp849=bound;void*_tmp84A=({Cyc_yyget_YY44(&(yyyvsp[2]).v);});struct Cyc_List_List*_tmp84B=({Cyc_yyget_YY56(&(yyyvsp[1]).v);});struct Cyc_Absyn_Tqual _tmp84C=({Cyc_yyget_YY23(&(yyyvsp[4]).v);});_tmp845(_tmp846,_tmp847,_tmp848,_tmp849,_tmp84A,_tmp84B,_tmp84C);});
# 2235
ans=({struct Cyc_List_List*_tmp844=_region_malloc(yyr,sizeof(*_tmp844));_tmp844->hd=mod,_tmp844->tl=ans;_tmp844;});
yyval=({Cyc_YY26(ans);});
# 2238
goto _LL0;}}}}case 217U: _LL1AD: _LL1AE:
# 2240
 yyval=({Cyc_YY56(0);});
goto _LL0;case 218U: _LL1AF: _LL1B0:
# 2241 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp84D)(struct Cyc_List_List*yy1)=Cyc_YY56;struct Cyc_List_List*_tmp84E=({struct Cyc_List_List*_tmp84F=_region_malloc(yyr,sizeof(*_tmp84F));({void*_tmp121A=({Cyc_yyget_YY55(&(yyyvsp[0]).v);});_tmp84F->hd=_tmp121A;}),({struct Cyc_List_List*_tmp1219=({Cyc_yyget_YY56(&(yyyvsp[1]).v);});_tmp84F->tl=_tmp1219;});_tmp84F;});_tmp84D(_tmp84E);});
goto _LL0;case 219U: _LL1B1: _LL1B2:
# 2246 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp850)(void*yy1)=Cyc_YY55;void*_tmp851=(void*)({struct Cyc_Parse_Numelts_ptrqual_Parse_Pointer_qual_struct*_tmp852=_region_malloc(yyr,sizeof(*_tmp852));_tmp852->tag=0U,({struct Cyc_Absyn_Exp*_tmp121B=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp852->f1=_tmp121B;});_tmp852;});_tmp850(_tmp851);});
goto _LL0;case 220U: _LL1B3: _LL1B4:
# 2248 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp853)(void*yy1)=Cyc_YY55;void*_tmp854=(void*)({struct Cyc_Parse_Region_ptrqual_Parse_Pointer_qual_struct*_tmp855=_region_malloc(yyr,sizeof(*_tmp855));_tmp855->tag=1U,({struct Cyc_List_List*_tmp121C=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp855->f1=_tmp121C;});_tmp855;});_tmp853(_tmp854);});
goto _LL0;case 221U: _LL1B5: _LL1B6:
# 2250 "parse.y"
 yyval=({Cyc_YY55((void*)({struct Cyc_Parse_Thin_ptrqual_Parse_Pointer_qual_struct*_tmp856=_region_malloc(yyr,sizeof(*_tmp856));_tmp856->tag=2U;_tmp856;}));});
goto _LL0;case 222U: _LL1B7: _LL1B8:
# 2252 "parse.y"
 yyval=({Cyc_YY55((void*)({struct Cyc_Parse_Fat_ptrqual_Parse_Pointer_qual_struct*_tmp857=_region_malloc(yyr,sizeof(*_tmp857));_tmp857->tag=3U;_tmp857;}));});
goto _LL0;case 223U: _LL1B9: _LL1BA:
# 2254 "parse.y"
 yyval=({Cyc_YY55((void*)({struct Cyc_Parse_Zeroterm_ptrqual_Parse_Pointer_qual_struct*_tmp858=_region_malloc(yyr,sizeof(*_tmp858));_tmp858->tag=4U;_tmp858;}));});
goto _LL0;case 224U: _LL1BB: _LL1BC:
# 2256 "parse.y"
 yyval=({Cyc_YY55((void*)({struct Cyc_Parse_Nozeroterm_ptrqual_Parse_Pointer_qual_struct*_tmp859=_region_malloc(yyr,sizeof(*_tmp859));_tmp859->tag=5U;_tmp859;}));});
goto _LL0;case 225U: _LL1BD: _LL1BE:
# 2258 "parse.y"
 yyval=({Cyc_YY55((void*)({struct Cyc_Parse_Notnull_ptrqual_Parse_Pointer_qual_struct*_tmp85A=_region_malloc(yyr,sizeof(*_tmp85A));_tmp85A->tag=6U;_tmp85A;}));});
goto _LL0;case 226U: _LL1BF: _LL1C0:
# 2260 "parse.y"
 yyval=({Cyc_YY55((void*)({struct Cyc_Parse_Nullable_ptrqual_Parse_Pointer_qual_struct*_tmp85B=_region_malloc(yyr,sizeof(*_tmp85B));_tmp85B->tag=7U;_tmp85B;}));});
goto _LL0;case 227U: _LL1C1: _LL1C2: {
# 2266 "parse.y"
unsigned loc=(unsigned)((yyyvsp[0]).l).first_line;
if(!Cyc_Parse_parsing_tempest)
yyval=({union Cyc_YYSTYPE(*_tmp85C)(struct _tuple22*yy1)=Cyc_YY1;struct _tuple22*_tmp85D=({struct _tuple22*_tmp85E=_cycalloc(sizeof(*_tmp85E));_tmp85E->f1=loc,_tmp85E->f2=Cyc_Absyn_true_type,({void*_tmp121D=({Cyc_yyget_YY2(&(yyyvsp[1]).v);});_tmp85E->f3=_tmp121D;});_tmp85E;});_tmp85C(_tmp85D);});else{
# 2270
yyval=({Cyc_YY1(({struct _tuple22*_tmp85F=_cycalloc(sizeof(*_tmp85F));_tmp85F->f1=loc,_tmp85F->f2=Cyc_Absyn_true_type,_tmp85F->f3=Cyc_Absyn_fat_bound_type;_tmp85F;}));});}
# 2272
goto _LL0;}case 228U: _LL1C3: _LL1C4: {
# 2273 "parse.y"
unsigned loc=(unsigned)((yyyvsp[0]).l).first_line;
yyval=({union Cyc_YYSTYPE(*_tmp860)(struct _tuple22*yy1)=Cyc_YY1;struct _tuple22*_tmp861=({struct _tuple22*_tmp862=_cycalloc(sizeof(*_tmp862));_tmp862->f1=loc,_tmp862->f2=Cyc_Absyn_false_type,({void*_tmp121E=({Cyc_yyget_YY2(&(yyyvsp[1]).v);});_tmp862->f3=_tmp121E;});_tmp862;});_tmp860(_tmp861);});
# 2276
goto _LL0;}case 229U: _LL1C5: _LL1C6: {
# 2277 "parse.y"
unsigned loc=(unsigned)((yyyvsp[0]).l).first_line;
yyval=({Cyc_YY1(({struct _tuple22*_tmp863=_cycalloc(sizeof(*_tmp863));_tmp863->f1=loc,_tmp863->f2=Cyc_Absyn_true_type,_tmp863->f3=Cyc_Absyn_fat_bound_type;_tmp863;}));});
# 2280
goto _LL0;}case 230U: _LL1C7: _LL1C8:
# 2282
 yyval=({union Cyc_YYSTYPE(*_tmp864)(void*yy1)=Cyc_YY2;void*_tmp865=({Cyc_Absyn_bounds_one();});_tmp864(_tmp865);});
goto _LL0;case 231U: _LL1C9: _LL1CA:
# 2283 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp866)(void*yy1)=Cyc_YY2;void*_tmp867=({void*(*_tmp868)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_thin_bounds_exp;struct Cyc_Absyn_Exp*_tmp869=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});_tmp868(_tmp869);});_tmp866(_tmp867);});
goto _LL0;case 232U: _LL1CB: _LL1CC:
# 2286
 yyval=({union Cyc_YYSTYPE(*_tmp86A)(void*yy1)=Cyc_YY51;void*_tmp86B=({Cyc_Tcutil_any_bool(0);});_tmp86A(_tmp86B);});
goto _LL0;case 233U: _LL1CD: _LL1CE:
# 2287 "parse.y"
 yyval=({Cyc_YY51(Cyc_Absyn_true_type);});
goto _LL0;case 234U: _LL1CF: _LL1D0:
# 2288 "parse.y"
 yyval=({Cyc_YY51(Cyc_Absyn_false_type);});
goto _LL0;case 235U: _LL1D1: _LL1D2:
# 2299 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp86C)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp86D=({struct Cyc_List_List*_tmp86E=_cycalloc(sizeof(*_tmp86E));({void*_tmp121F=({Cyc_yyget_YY44(&(yyyvsp[0]).v);});_tmp86E->hd=_tmp121F;}),_tmp86E->tl=0;_tmp86E;});_tmp86C(_tmp86D);});
goto _LL0;case 236U: _LL1D3: _LL1D4:
# 2300 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp86F)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp870=({struct Cyc_List_List*_tmp871=_cycalloc(sizeof(*_tmp871));({void*_tmp1221=({Cyc_yyget_YY44(&(yyyvsp[0]).v);});_tmp871->hd=_tmp1221;}),({struct Cyc_List_List*_tmp1220=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp871->tl=_tmp1220;});_tmp871;});_tmp86F(_tmp870);});
goto _LL0;case 237U: _LL1D5: _LL1D6:
# 2304 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp872)(void*yy1)=Cyc_YY44;void*_tmp873=({Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,0);});_tmp872(_tmp873);});
goto _LL0;case 238U: _LL1D7: _LL1D8: {
# 2305 "parse.y"
struct Cyc_List_List*es=({Cyc_yyget_YY40(&(yyyvsp[0]).v);});
if(({Cyc_List_length(es);})== 1)
# 2308
yyval=({Cyc_YY44((void*)((struct Cyc_List_List*)_check_null(es))->hd);});else{
# 2312
yyval=({union Cyc_YYSTYPE(*_tmp874)(void*yy1)=Cyc_YY44;void*_tmp875=({void*(*_tmp876)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp877=({Cyc_yyget_YY40(&(yyyvsp[0]).v);});_tmp876(_tmp877);});_tmp874(_tmp875);});}
# 2315
goto _LL0;}case 239U: _LL1D9: _LL1DA:
# 2315 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp878)(void*yy1)=Cyc_YY44;void*_tmp879=({Cyc_Absyn_new_evar(& Cyc_Tcutil_trko,0);});_tmp878(_tmp879);});
goto _LL0;case 240U: _LL1DB: _LL1DC:
# 2319 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp87A)(struct Cyc_Absyn_Tqual yy1)=Cyc_YY23;struct Cyc_Absyn_Tqual _tmp87B=({Cyc_Absyn_empty_tqual((unsigned)((*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),yyvsp_offset + 1))).l).first_line);});_tmp87A(_tmp87B);});
goto _LL0;case 241U: _LL1DD: _LL1DE:
# 2320 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp87C)(struct Cyc_Absyn_Tqual yy1)=Cyc_YY23;struct Cyc_Absyn_Tqual _tmp87D=({struct Cyc_Absyn_Tqual(*_tmp87E)(struct Cyc_Absyn_Tqual,struct Cyc_Absyn_Tqual)=Cyc_Absyn_combine_tqual;struct Cyc_Absyn_Tqual _tmp87F=({Cyc_yyget_YY23(&(yyyvsp[0]).v);});struct Cyc_Absyn_Tqual _tmp880=({Cyc_yyget_YY23(&(yyyvsp[1]).v);});_tmp87E(_tmp87F,_tmp880);});_tmp87C(_tmp87D);});
goto _LL0;case 242U: _LL1DF: _LL1E0:
# 2325 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp881)(struct _tuple27*yy1)=Cyc_YY39;struct _tuple27*_tmp882=({struct _tuple27*_tmp885=_cycalloc(sizeof(*_tmp885));({struct Cyc_List_List*_tmp1224=({struct Cyc_List_List*(*_tmp883)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp884=({Cyc_yyget_YY38(&(yyyvsp[0]).v);});_tmp883(_tmp884);});_tmp885->f1=_tmp1224;}),_tmp885->f2=0,_tmp885->f3=0,({void*_tmp1223=({Cyc_yyget_YY49(&(yyyvsp[1]).v);});_tmp885->f4=_tmp1223;}),({struct Cyc_List_List*_tmp1222=({Cyc_yyget_YY50(&(yyyvsp[2]).v);});_tmp885->f5=_tmp1222;});_tmp885;});_tmp881(_tmp882);});
goto _LL0;case 243U: _LL1E1: _LL1E2:
# 2327 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp886)(struct _tuple27*yy1)=Cyc_YY39;struct _tuple27*_tmp887=({struct _tuple27*_tmp88A=_cycalloc(sizeof(*_tmp88A));({struct Cyc_List_List*_tmp1227=({struct Cyc_List_List*(*_tmp888)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp889=({Cyc_yyget_YY38(&(yyyvsp[0]).v);});_tmp888(_tmp889);});_tmp88A->f1=_tmp1227;}),_tmp88A->f2=1,_tmp88A->f3=0,({void*_tmp1226=({Cyc_yyget_YY49(&(yyyvsp[3]).v);});_tmp88A->f4=_tmp1226;}),({struct Cyc_List_List*_tmp1225=({Cyc_yyget_YY50(&(yyyvsp[4]).v);});_tmp88A->f5=_tmp1225;});_tmp88A;});_tmp886(_tmp887);});
goto _LL0;case 244U: _LL1E3: _LL1E4: {
# 2330
struct _tuple9*_stmttmp1D=({Cyc_yyget_YY37(&(yyyvsp[2]).v);});void*_tmp88D;struct Cyc_Absyn_Tqual _tmp88C;struct _fat_ptr*_tmp88B;_LL492: _tmp88B=_stmttmp1D->f1;_tmp88C=_stmttmp1D->f2;_tmp88D=_stmttmp1D->f3;_LL493: {struct _fat_ptr*n=_tmp88B;struct Cyc_Absyn_Tqual tq=_tmp88C;void*t=_tmp88D;
struct Cyc_Absyn_VarargInfo*v=({struct Cyc_Absyn_VarargInfo*_tmp891=_cycalloc(sizeof(*_tmp891));_tmp891->name=n,_tmp891->tq=tq,_tmp891->type=t,({int _tmp1228=({Cyc_yyget_YY31(&(yyyvsp[1]).v);});_tmp891->inject=_tmp1228;});_tmp891;});
yyval=({union Cyc_YYSTYPE(*_tmp88E)(struct _tuple27*yy1)=Cyc_YY39;struct _tuple27*_tmp88F=({struct _tuple27*_tmp890=_cycalloc(sizeof(*_tmp890));_tmp890->f1=0,_tmp890->f2=0,_tmp890->f3=v,({void*_tmp122A=({Cyc_yyget_YY49(&(yyyvsp[3]).v);});_tmp890->f4=_tmp122A;}),({struct Cyc_List_List*_tmp1229=({Cyc_yyget_YY50(&(yyyvsp[4]).v);});_tmp890->f5=_tmp1229;});_tmp890;});_tmp88E(_tmp88F);});
# 2334
goto _LL0;}}case 245U: _LL1E5: _LL1E6: {
# 2336
struct _tuple9*_stmttmp1E=({Cyc_yyget_YY37(&(yyyvsp[4]).v);});void*_tmp894;struct Cyc_Absyn_Tqual _tmp893;struct _fat_ptr*_tmp892;_LL495: _tmp892=_stmttmp1E->f1;_tmp893=_stmttmp1E->f2;_tmp894=_stmttmp1E->f3;_LL496: {struct _fat_ptr*n=_tmp892;struct Cyc_Absyn_Tqual tq=_tmp893;void*t=_tmp894;
struct Cyc_Absyn_VarargInfo*v=({struct Cyc_Absyn_VarargInfo*_tmp89A=_cycalloc(sizeof(*_tmp89A));_tmp89A->name=n,_tmp89A->tq=tq,_tmp89A->type=t,({int _tmp122B=({Cyc_yyget_YY31(&(yyyvsp[3]).v);});_tmp89A->inject=_tmp122B;});_tmp89A;});
yyval=({union Cyc_YYSTYPE(*_tmp895)(struct _tuple27*yy1)=Cyc_YY39;struct _tuple27*_tmp896=({struct _tuple27*_tmp899=_cycalloc(sizeof(*_tmp899));({struct Cyc_List_List*_tmp122E=({struct Cyc_List_List*(*_tmp897)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp898=({Cyc_yyget_YY38(&(yyyvsp[0]).v);});_tmp897(_tmp898);});_tmp899->f1=_tmp122E;}),_tmp899->f2=0,_tmp899->f3=v,({void*_tmp122D=({Cyc_yyget_YY49(&(yyyvsp[5]).v);});_tmp899->f4=_tmp122D;}),({struct Cyc_List_List*_tmp122C=({Cyc_yyget_YY50(&(yyyvsp[6]).v);});_tmp899->f5=_tmp122C;});_tmp899;});_tmp895(_tmp896);});
# 2340
goto _LL0;}}case 246U: _LL1E7: _LL1E8:
# 2344 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp89B)(void*yy1)=Cyc_YY44;void*_tmp89C=({void*(*_tmp89D)(struct _fat_ptr s,void*k)=Cyc_Parse_id2type;struct _fat_ptr _tmp89E=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});void*_tmp89F=(void*)({struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct*_tmp8A0=_cycalloc(sizeof(*_tmp8A0));_tmp8A0->tag=1U,_tmp8A0->f1=0;_tmp8A0;});_tmp89D(_tmp89E,_tmp89F);});_tmp89B(_tmp89C);});
goto _LL0;case 247U: _LL1E9: _LL1EA:
# 2345 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp8A1)(void*yy1)=Cyc_YY44;void*_tmp8A2=({void*(*_tmp8A3)(struct _fat_ptr s,void*k)=Cyc_Parse_id2type;struct _fat_ptr _tmp8A4=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});void*_tmp8A5=({void*(*_tmp8A6)(struct Cyc_Absyn_Kind*k)=Cyc_Tcutil_kind_to_bound;struct Cyc_Absyn_Kind*_tmp8A7=({Cyc_yyget_YY43(&(yyyvsp[2]).v);});_tmp8A6(_tmp8A7);});_tmp8A3(_tmp8A4,_tmp8A5);});_tmp8A1(_tmp8A2);});
goto _LL0;case 248U: _LL1EB: _LL1EC:
# 2348
 yyval=({Cyc_YY49(0);});
goto _LL0;case 249U: _LL1ED: _LL1EE:
# 2349 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp8A8)(void*yy1)=Cyc_YY49;void*_tmp8A9=({void*(*_tmp8AA)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp8AB=({Cyc_yyget_YY40(&(yyyvsp[1]).v);});_tmp8AA(_tmp8AB);});_tmp8A8(_tmp8A9);});
goto _LL0;case 250U: _LL1EF: _LL1F0:
# 2353 "parse.y"
 yyval=({Cyc_YY50(0);});
goto _LL0;case 251U: _LL1F1: _LL1F2:
# 2354 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 252U: _LL1F3: _LL1F4: {
# 2362 "parse.y"
struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*kb=({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp8B2=_cycalloc(sizeof(*_tmp8B2));_tmp8B2->tag=2U,_tmp8B2->f1=0,_tmp8B2->f2=& Cyc_Tcutil_trk;_tmp8B2;});
struct _fat_ptr id=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});
void*t=({Cyc_Parse_id2type(id,(void*)kb);});
yyval=({union Cyc_YYSTYPE(*_tmp8AC)(struct Cyc_List_List*yy1)=Cyc_YY50;struct Cyc_List_List*_tmp8AD=({struct Cyc_List_List*_tmp8B1=_cycalloc(sizeof(*_tmp8B1));({struct _tuple32*_tmp1230=({struct _tuple32*_tmp8B0=_cycalloc(sizeof(*_tmp8B0));({void*_tmp122F=({void*(*_tmp8AE)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp8AF=({Cyc_yyget_YY40(&(yyyvsp[0]).v);});_tmp8AE(_tmp8AF);});_tmp8B0->f1=_tmp122F;}),_tmp8B0->f2=t;_tmp8B0;});_tmp8B1->hd=_tmp1230;}),_tmp8B1->tl=0;_tmp8B1;});_tmp8AC(_tmp8AD);});
# 2367
goto _LL0;}case 253U: _LL1F5: _LL1F6: {
# 2369 "parse.y"
struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*kb=({struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*_tmp8B9=_cycalloc(sizeof(*_tmp8B9));_tmp8B9->tag=2U,_tmp8B9->f1=0,_tmp8B9->f2=& Cyc_Tcutil_trk;_tmp8B9;});
struct _fat_ptr id=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});
void*t=({Cyc_Parse_id2type(id,(void*)kb);});
yyval=({union Cyc_YYSTYPE(*_tmp8B3)(struct Cyc_List_List*yy1)=Cyc_YY50;struct Cyc_List_List*_tmp8B4=({struct Cyc_List_List*_tmp8B8=_cycalloc(sizeof(*_tmp8B8));({struct _tuple32*_tmp1233=({struct _tuple32*_tmp8B7=_cycalloc(sizeof(*_tmp8B7));({void*_tmp1232=({void*(*_tmp8B5)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp8B6=({Cyc_yyget_YY40(&(yyyvsp[0]).v);});_tmp8B5(_tmp8B6);});_tmp8B7->f1=_tmp1232;}),_tmp8B7->f2=t;_tmp8B7;});_tmp8B8->hd=_tmp1233;}),({struct Cyc_List_List*_tmp1231=({Cyc_yyget_YY50(&(yyyvsp[4]).v);});_tmp8B8->tl=_tmp1231;});_tmp8B8;});_tmp8B3(_tmp8B4);});
# 2374
goto _LL0;}case 254U: _LL1F7: _LL1F8:
# 2378 "parse.y"
 yyval=({Cyc_YY31(0);});
goto _LL0;case 255U: _LL1F9: _LL1FA:
# 2380 "parse.y"
 if(({int(*_tmp8BA)(struct _fat_ptr,struct _fat_ptr)=Cyc_zstrcmp;struct _fat_ptr _tmp8BB=(struct _fat_ptr)({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});struct _fat_ptr _tmp8BC=({const char*_tmp8BD="inject";_tag_fat(_tmp8BD,sizeof(char),7U);});_tmp8BA(_tmp8BB,_tmp8BC);})!= 0)
({void*_tmp8BE=0U;({unsigned _tmp1235=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp1234=({const char*_tmp8BF="missing type in function declaration";_tag_fat(_tmp8BF,sizeof(char),37U);});Cyc_Warn_err(_tmp1235,_tmp1234,_tag_fat(_tmp8BE,sizeof(void*),0U));});});
# 2380
yyval=({Cyc_YY31(1);});
# 2384
goto _LL0;case 256U: _LL1FB: _LL1FC:
# 2387 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 257U: _LL1FD: _LL1FE:
# 2388 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp8C0)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp8C1=({struct Cyc_List_List*(*_tmp8C2)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp8C3=({Cyc_yyget_YY40(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp8C4=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp8C2(_tmp8C3,_tmp8C4);});_tmp8C0(_tmp8C1);});
goto _LL0;case 258U: _LL1FF: _LL200:
# 2392 "parse.y"
 yyval=({Cyc_YY40(0);});
goto _LL0;case 259U: _LL201: _LL202:
# 2393 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 260U: _LL203: _LL204:
# 2395 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp8C5)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp8C6=({struct Cyc_List_List*_tmp8C9=_cycalloc(sizeof(*_tmp8C9));({void*_tmp1236=({void*(*_tmp8C7)(void*)=Cyc_Absyn_regionsof_eff;void*_tmp8C8=({Cyc_yyget_YY44(&(yyyvsp[2]).v);});_tmp8C7(_tmp8C8);});_tmp8C9->hd=_tmp1236;}),_tmp8C9->tl=0;_tmp8C9;});_tmp8C5(_tmp8C6);});
goto _LL0;case 261U: _LL205: _LL206:
# 2397 "parse.y"
({void(*_tmp8CA)(void*t,struct Cyc_Absyn_Kind*k,int leq)=Cyc_Parse_set_vartyp_kind;void*_tmp8CB=({Cyc_yyget_YY44(&(yyyvsp[0]).v);});struct Cyc_Absyn_Kind*_tmp8CC=& Cyc_Tcutil_ek;int _tmp8CD=0;_tmp8CA(_tmp8CB,_tmp8CC,_tmp8CD);});
yyval=({union Cyc_YYSTYPE(*_tmp8CE)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp8CF=({struct Cyc_List_List*_tmp8D0=_cycalloc(sizeof(*_tmp8D0));({void*_tmp1237=({Cyc_yyget_YY44(&(yyyvsp[0]).v);});_tmp8D0->hd=_tmp1237;}),_tmp8D0->tl=0;_tmp8D0;});_tmp8CE(_tmp8CF);});
# 2400
goto _LL0;case 262U: _LL207: _LL208:
# 2417 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp8D1)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp8D2=({struct Cyc_List_List*_tmp8D8=_cycalloc(sizeof(*_tmp8D8));({void*_tmp1238=({void*(*_tmp8D3)(void*)=Cyc_Absyn_access_eff;void*_tmp8D4=({void*(*_tmp8D5)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmp8D6=({Cyc_yyget_YY37(&(yyyvsp[0]).v);});unsigned _tmp8D7=(unsigned)((yyyvsp[0]).l).first_line;_tmp8D5(_tmp8D6,_tmp8D7);});_tmp8D3(_tmp8D4);});_tmp8D8->hd=_tmp1238;}),_tmp8D8->tl=0;_tmp8D8;});_tmp8D1(_tmp8D2);});
goto _LL0;case 263U: _LL209: _LL20A:
# 2419 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp8D9)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp8DA=({struct Cyc_List_List*_tmp8E0=_cycalloc(sizeof(*_tmp8E0));({void*_tmp123A=({void*(*_tmp8DB)(void*)=Cyc_Absyn_access_eff;void*_tmp8DC=({void*(*_tmp8DD)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmp8DE=({Cyc_yyget_YY37(&(yyyvsp[0]).v);});unsigned _tmp8DF=(unsigned)((yyyvsp[0]).l).first_line;_tmp8DD(_tmp8DE,_tmp8DF);});_tmp8DB(_tmp8DC);});_tmp8E0->hd=_tmp123A;}),({struct Cyc_List_List*_tmp1239=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp8E0->tl=_tmp1239;});_tmp8E0;});_tmp8D9(_tmp8DA);});
goto _LL0;case 264U: _LL20B: _LL20C:
# 2425 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp8E1)(struct Cyc_List_List*yy1)=Cyc_YY38;struct Cyc_List_List*_tmp8E2=({struct Cyc_List_List*_tmp8E3=_cycalloc(sizeof(*_tmp8E3));({struct _tuple9*_tmp123B=({Cyc_yyget_YY37(&(yyyvsp[0]).v);});_tmp8E3->hd=_tmp123B;}),_tmp8E3->tl=0;_tmp8E3;});_tmp8E1(_tmp8E2);});
goto _LL0;case 265U: _LL20D: _LL20E:
# 2427 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp8E4)(struct Cyc_List_List*yy1)=Cyc_YY38;struct Cyc_List_List*_tmp8E5=({struct Cyc_List_List*_tmp8E6=_cycalloc(sizeof(*_tmp8E6));({struct _tuple9*_tmp123D=({Cyc_yyget_YY37(&(yyyvsp[2]).v);});_tmp8E6->hd=_tmp123D;}),({struct Cyc_List_List*_tmp123C=({Cyc_yyget_YY38(&(yyyvsp[0]).v);});_tmp8E6->tl=_tmp123C;});_tmp8E6;});_tmp8E4(_tmp8E5);});
goto _LL0;case 266U: _LL20F: _LL210: {
# 2433 "parse.y"
struct _tuple26 _stmttmp1F=({Cyc_yyget_YY35(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp8E9;struct Cyc_Parse_Type_specifier _tmp8E8;struct Cyc_Absyn_Tqual _tmp8E7;_LL498: _tmp8E7=_stmttmp1F.f1;_tmp8E8=_stmttmp1F.f2;_tmp8E9=_stmttmp1F.f3;_LL499: {struct Cyc_Absyn_Tqual tq=_tmp8E7;struct Cyc_Parse_Type_specifier tspecs=_tmp8E8;struct Cyc_List_List*atts=_tmp8E9;
if(tq.loc == (unsigned)0)tq.loc=(unsigned)((yyyvsp[0]).l).first_line;{struct Cyc_Parse_Declarator _stmttmp20=({Cyc_yyget_YY27(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmp8EC;unsigned _tmp8EB;struct _tuple0*_tmp8EA;_LL49B: _tmp8EA=_stmttmp20.id;_tmp8EB=_stmttmp20.varloc;_tmp8EC=_stmttmp20.tms;_LL49C: {struct _tuple0*qv=_tmp8EA;unsigned varloc=_tmp8EB;struct Cyc_List_List*tms=_tmp8EC;
# 2436
void*t=({Cyc_Parse_speclist2typ(tspecs,(unsigned)((yyyvsp[0]).l).first_line);});
struct _tuple15 _stmttmp21=({Cyc_Parse_apply_tms(tq,t,atts,tms);});struct Cyc_List_List*_tmp8F0;struct Cyc_List_List*_tmp8EF;void*_tmp8EE;struct Cyc_Absyn_Tqual _tmp8ED;_LL49E: _tmp8ED=_stmttmp21.f1;_tmp8EE=_stmttmp21.f2;_tmp8EF=_stmttmp21.f3;_tmp8F0=_stmttmp21.f4;_LL49F: {struct Cyc_Absyn_Tqual tq2=_tmp8ED;void*t2=_tmp8EE;struct Cyc_List_List*tvs=_tmp8EF;struct Cyc_List_List*atts2=_tmp8F0;
if(tvs != 0)
({void*_tmp8F1=0U;({unsigned _tmp123F=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp123E=({const char*_tmp8F2="parameter with bad type params";_tag_fat(_tmp8F2,sizeof(char),31U);});Cyc_Warn_err(_tmp123F,_tmp123E,_tag_fat(_tmp8F1,sizeof(void*),0U));});});
# 2438
if(({Cyc_Absyn_is_qvar_qualified(qv);}))
# 2441
({void*_tmp8F3=0U;({unsigned _tmp1241=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp1240=({const char*_tmp8F4="parameter cannot be qualified with a namespace";_tag_fat(_tmp8F4,sizeof(char),47U);});Cyc_Warn_err(_tmp1241,_tmp1240,_tag_fat(_tmp8F3,sizeof(void*),0U));});});{
# 2438
struct _fat_ptr*idopt=(*qv).f2;
# 2443
if(atts2 != 0)
({void*_tmp8F5=0U;({unsigned _tmp1243=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct _fat_ptr _tmp1242=({const char*_tmp8F6="extra attributes on parameter, ignoring";_tag_fat(_tmp8F6,sizeof(char),40U);});Cyc_Warn_warn(_tmp1243,_tmp1242,_tag_fat(_tmp8F5,sizeof(void*),0U));});});
# 2443
yyval=({Cyc_YY37(({struct _tuple9*_tmp8F7=_cycalloc(sizeof(*_tmp8F7));_tmp8F7->f1=idopt,_tmp8F7->f2=tq2,_tmp8F7->f3=t2;_tmp8F7;}));});
# 2447
goto _LL0;}}}}}}case 267U: _LL211: _LL212: {
# 2448 "parse.y"
struct _tuple26 _stmttmp22=({Cyc_yyget_YY35(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp8FA;struct Cyc_Parse_Type_specifier _tmp8F9;struct Cyc_Absyn_Tqual _tmp8F8;_LL4A1: _tmp8F8=_stmttmp22.f1;_tmp8F9=_stmttmp22.f2;_tmp8FA=_stmttmp22.f3;_LL4A2: {struct Cyc_Absyn_Tqual tq=_tmp8F8;struct Cyc_Parse_Type_specifier tspecs=_tmp8F9;struct Cyc_List_List*atts=_tmp8FA;
if(tq.loc == (unsigned)0)tq.loc=(unsigned)((yyyvsp[0]).l).first_line;{void*t=({Cyc_Parse_speclist2typ(tspecs,(unsigned)((yyyvsp[0]).l).first_line);});
# 2451
if(atts != 0)
({void*_tmp8FB=0U;({unsigned _tmp1245=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp1244=({const char*_tmp8FC="bad attributes on parameter, ignoring";_tag_fat(_tmp8FC,sizeof(char),38U);});Cyc_Warn_warn(_tmp1245,_tmp1244,_tag_fat(_tmp8FB,sizeof(void*),0U));});});
# 2451
yyval=({Cyc_YY37(({struct _tuple9*_tmp8FD=_cycalloc(sizeof(*_tmp8FD));_tmp8FD->f1=0,_tmp8FD->f2=tq,_tmp8FD->f3=t;_tmp8FD;}));});
# 2455
goto _LL0;}}}case 268U: _LL213: _LL214: {
# 2456 "parse.y"
struct _tuple26 _stmttmp23=({Cyc_yyget_YY35(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp900;struct Cyc_Parse_Type_specifier _tmp8FF;struct Cyc_Absyn_Tqual _tmp8FE;_LL4A4: _tmp8FE=_stmttmp23.f1;_tmp8FF=_stmttmp23.f2;_tmp900=_stmttmp23.f3;_LL4A5: {struct Cyc_Absyn_Tqual tq=_tmp8FE;struct Cyc_Parse_Type_specifier tspecs=_tmp8FF;struct Cyc_List_List*atts=_tmp900;
if(tq.loc == (unsigned)0)tq.loc=(unsigned)((yyyvsp[0]).l).first_line;{void*t=({Cyc_Parse_speclist2typ(tspecs,(unsigned)((yyyvsp[0]).l).first_line);});
# 2459
struct Cyc_List_List*tms=({Cyc_yyget_YY30(&(yyyvsp[1]).v);}).tms;
struct _tuple15 _stmttmp24=({Cyc_Parse_apply_tms(tq,t,atts,tms);});struct Cyc_List_List*_tmp904;struct Cyc_List_List*_tmp903;void*_tmp902;struct Cyc_Absyn_Tqual _tmp901;_LL4A7: _tmp901=_stmttmp24.f1;_tmp902=_stmttmp24.f2;_tmp903=_stmttmp24.f3;_tmp904=_stmttmp24.f4;_LL4A8: {struct Cyc_Absyn_Tqual tq2=_tmp901;void*t2=_tmp902;struct Cyc_List_List*tvs=_tmp903;struct Cyc_List_List*atts2=_tmp904;
if(tvs != 0)
({void*_tmp905=0U;({unsigned _tmp1247=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct _fat_ptr _tmp1246=({const char*_tmp906="bad type parameters on formal argument, ignoring";_tag_fat(_tmp906,sizeof(char),49U);});Cyc_Warn_warn(_tmp1247,_tmp1246,_tag_fat(_tmp905,sizeof(void*),0U));});});
# 2461
if(atts2 != 0)
# 2465
({void*_tmp907=0U;({unsigned _tmp1249=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});struct _fat_ptr _tmp1248=({const char*_tmp908="bad attributes on parameter, ignoring";_tag_fat(_tmp908,sizeof(char),38U);});Cyc_Warn_warn(_tmp1249,_tmp1248,_tag_fat(_tmp907,sizeof(void*),0U));});});
# 2461
yyval=({Cyc_YY37(({struct _tuple9*_tmp909=_cycalloc(sizeof(*_tmp909));_tmp909->f1=0,_tmp909->f2=tq2,_tmp909->f3=t2;_tmp909;}));});
# 2468
goto _LL0;}}}}case 269U: _LL215: _LL216:
# 2472 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp90A)(struct Cyc_List_List*yy1)=Cyc_YY36;struct Cyc_List_List*_tmp90B=({struct Cyc_List_List*(*_tmp90C)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp90D=({Cyc_yyget_YY36(&(yyyvsp[0]).v);});_tmp90C(_tmp90D);});_tmp90A(_tmp90B);});
goto _LL0;case 270U: _LL217: _LL218:
# 2476 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp90E)(struct Cyc_List_List*yy1)=Cyc_YY36;struct Cyc_List_List*_tmp90F=({struct Cyc_List_List*_tmp911=_cycalloc(sizeof(*_tmp911));({struct _fat_ptr*_tmp124B=({struct _fat_ptr*_tmp910=_cycalloc(sizeof(*_tmp910));({struct _fat_ptr _tmp124A=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmp910=_tmp124A;});_tmp910;});_tmp911->hd=_tmp124B;}),_tmp911->tl=0;_tmp911;});_tmp90E(_tmp90F);});
goto _LL0;case 271U: _LL219: _LL21A:
# 2478 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp912)(struct Cyc_List_List*yy1)=Cyc_YY36;struct Cyc_List_List*_tmp913=({struct Cyc_List_List*_tmp915=_cycalloc(sizeof(*_tmp915));({struct _fat_ptr*_tmp124E=({struct _fat_ptr*_tmp914=_cycalloc(sizeof(*_tmp914));({struct _fat_ptr _tmp124D=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});*_tmp914=_tmp124D;});_tmp914;});_tmp915->hd=_tmp124E;}),({struct Cyc_List_List*_tmp124C=({Cyc_yyget_YY36(&(yyyvsp[0]).v);});_tmp915->tl=_tmp124C;});_tmp915;});_tmp912(_tmp913);});
goto _LL0;case 272U: _LL21B: _LL21C:
# 2482 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 273U: _LL21D: _LL21E:
# 2483 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 274U: _LL21F: _LL220:
# 2488 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp916)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmp917=({struct Cyc_Absyn_Exp*(*_tmp918)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp919=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp91A=_cycalloc(sizeof(*_tmp91A));_tmp91A->tag=37U,_tmp91A->f1=0,_tmp91A->f2=0;_tmp91A;});unsigned _tmp91B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp918(_tmp919,_tmp91B);});_tmp916(_tmp917);});
goto _LL0;case 275U: _LL221: _LL222:
# 2490 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp91C)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmp91D=({struct Cyc_Absyn_Exp*(*_tmp91E)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp91F=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp922=_cycalloc(sizeof(*_tmp922));_tmp922->tag=37U,_tmp922->f1=0,({struct Cyc_List_List*_tmp124F=({struct Cyc_List_List*(*_tmp920)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp921=({Cyc_yyget_YY5(&(yyyvsp[1]).v);});_tmp920(_tmp921);});_tmp922->f2=_tmp124F;});_tmp922;});unsigned _tmp923=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp91E(_tmp91F,_tmp923);});_tmp91C(_tmp91D);});
goto _LL0;case 276U: _LL223: _LL224:
# 2492 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp924)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmp925=({struct Cyc_Absyn_Exp*(*_tmp926)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp927=(void*)({struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*_tmp92A=_cycalloc(sizeof(*_tmp92A));_tmp92A->tag=37U,_tmp92A->f1=0,({struct Cyc_List_List*_tmp1250=({struct Cyc_List_List*(*_tmp928)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp929=({Cyc_yyget_YY5(&(yyyvsp[1]).v);});_tmp928(_tmp929);});_tmp92A->f2=_tmp1250;});_tmp92A;});unsigned _tmp92B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp926(_tmp927,_tmp92B);});_tmp924(_tmp925);});
goto _LL0;case 277U: _LL225: _LL226: {
# 2494 "parse.y"
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmp932)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmp933=(unsigned)((yyyvsp[2]).l).first_line;struct _tuple0*_tmp934=({struct _tuple0*_tmp936=_cycalloc(sizeof(*_tmp936));_tmp936->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp1252=({struct _fat_ptr*_tmp935=_cycalloc(sizeof(*_tmp935));({struct _fat_ptr _tmp1251=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});*_tmp935=_tmp1251;});_tmp935;});_tmp936->f2=_tmp1252;});_tmp936;});void*_tmp937=Cyc_Absyn_uint_type;struct Cyc_Absyn_Exp*_tmp938=({Cyc_Absyn_uint_exp(0U,(unsigned)((yyyvsp[2]).l).first_line);});_tmp932(_tmp933,_tmp934,_tmp937,_tmp938);});
# 2497
(vd->tq).real_const=1;
yyval=({union Cyc_YYSTYPE(*_tmp92C)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmp92D=({struct Cyc_Absyn_Exp*(*_tmp92E)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp92F=(void*)({struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*_tmp930=_cycalloc(sizeof(*_tmp930));_tmp930->tag=27U,_tmp930->f1=vd,({struct Cyc_Absyn_Exp*_tmp1254=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});_tmp930->f2=_tmp1254;}),({struct Cyc_Absyn_Exp*_tmp1253=({Cyc_yyget_Exp_tok(&(yyyvsp[6]).v);});_tmp930->f3=_tmp1253;}),_tmp930->f4=0;_tmp930;});unsigned _tmp931=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp92E(_tmp92F,_tmp931);});_tmp92C(_tmp92D);});
# 2500
goto _LL0;}case 278U: _LL227: _LL228: {
# 2502 "parse.y"
void*t=({void*(*_tmp93F)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmp940=({Cyc_yyget_YY37(&(yyyvsp[6]).v);});unsigned _tmp941=(unsigned)((yyyvsp[6]).l).first_line;_tmp93F(_tmp940,_tmp941);});
yyval=({union Cyc_YYSTYPE(*_tmp939)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmp93A=({struct Cyc_Absyn_Exp*(*_tmp93B)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmp93C=(void*)({struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*_tmp93D=_cycalloc(sizeof(*_tmp93D));_tmp93D->tag=28U,({struct Cyc_Absyn_Exp*_tmp1255=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});_tmp93D->f1=_tmp1255;}),_tmp93D->f2=t,_tmp93D->f3=0;_tmp93D;});unsigned _tmp93E=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp93B(_tmp93C,_tmp93E);});_tmp939(_tmp93A);});
# 2505
goto _LL0;}case 279U: _LL229: _LL22A:
# 2510 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp942)(struct Cyc_List_List*yy1)=Cyc_YY5;struct Cyc_List_List*_tmp943=({struct Cyc_List_List*_tmp945=_cycalloc(sizeof(*_tmp945));({struct _tuple33*_tmp1257=({struct _tuple33*_tmp944=_cycalloc(sizeof(*_tmp944));_tmp944->f1=0,({struct Cyc_Absyn_Exp*_tmp1256=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});_tmp944->f2=_tmp1256;});_tmp944;});_tmp945->hd=_tmp1257;}),_tmp945->tl=0;_tmp945;});_tmp942(_tmp943);});
goto _LL0;case 280U: _LL22B: _LL22C:
# 2512 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp946)(struct Cyc_List_List*yy1)=Cyc_YY5;struct Cyc_List_List*_tmp947=({struct Cyc_List_List*_tmp949=_cycalloc(sizeof(*_tmp949));({struct _tuple33*_tmp125A=({struct _tuple33*_tmp948=_cycalloc(sizeof(*_tmp948));({struct Cyc_List_List*_tmp1259=({Cyc_yyget_YY41(&(yyyvsp[0]).v);});_tmp948->f1=_tmp1259;}),({struct Cyc_Absyn_Exp*_tmp1258=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});_tmp948->f2=_tmp1258;});_tmp948;});_tmp949->hd=_tmp125A;}),_tmp949->tl=0;_tmp949;});_tmp946(_tmp947);});
goto _LL0;case 281U: _LL22D: _LL22E:
# 2514 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp94A)(struct Cyc_List_List*yy1)=Cyc_YY5;struct Cyc_List_List*_tmp94B=({struct Cyc_List_List*_tmp94D=_cycalloc(sizeof(*_tmp94D));({struct _tuple33*_tmp125D=({struct _tuple33*_tmp94C=_cycalloc(sizeof(*_tmp94C));_tmp94C->f1=0,({struct Cyc_Absyn_Exp*_tmp125C=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp94C->f2=_tmp125C;});_tmp94C;});_tmp94D->hd=_tmp125D;}),({struct Cyc_List_List*_tmp125B=({Cyc_yyget_YY5(&(yyyvsp[0]).v);});_tmp94D->tl=_tmp125B;});_tmp94D;});_tmp94A(_tmp94B);});
goto _LL0;case 282U: _LL22F: _LL230:
# 2516 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp94E)(struct Cyc_List_List*yy1)=Cyc_YY5;struct Cyc_List_List*_tmp94F=({struct Cyc_List_List*_tmp951=_cycalloc(sizeof(*_tmp951));({struct _tuple33*_tmp1261=({struct _tuple33*_tmp950=_cycalloc(sizeof(*_tmp950));({struct Cyc_List_List*_tmp1260=({Cyc_yyget_YY41(&(yyyvsp[2]).v);});_tmp950->f1=_tmp1260;}),({struct Cyc_Absyn_Exp*_tmp125F=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});_tmp950->f2=_tmp125F;});_tmp950;});_tmp951->hd=_tmp1261;}),({struct Cyc_List_List*_tmp125E=({Cyc_yyget_YY5(&(yyyvsp[0]).v);});_tmp951->tl=_tmp125E;});_tmp951;});_tmp94E(_tmp94F);});
goto _LL0;case 283U: _LL231: _LL232:
# 2520 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp952)(struct Cyc_List_List*yy1)=Cyc_YY41;struct Cyc_List_List*_tmp953=({struct Cyc_List_List*(*_tmp954)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp955=({Cyc_yyget_YY41(&(yyyvsp[0]).v);});_tmp954(_tmp955);});_tmp952(_tmp953);});
goto _LL0;case 284U: _LL233: _LL234:
# 2521 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp956)(struct Cyc_List_List*yy1)=Cyc_YY41;struct Cyc_List_List*_tmp957=({struct Cyc_List_List*_tmp95A=_cycalloc(sizeof(*_tmp95A));({void*_tmp1264=(void*)({struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_tmp959=_cycalloc(sizeof(*_tmp959));_tmp959->tag=1U,({struct _fat_ptr*_tmp1263=({struct _fat_ptr*_tmp958=_cycalloc(sizeof(*_tmp958));({struct _fat_ptr _tmp1262=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmp958=_tmp1262;});_tmp958;});_tmp959->f1=_tmp1263;});_tmp959;});_tmp95A->hd=_tmp1264;}),_tmp95A->tl=0;_tmp95A;});_tmp956(_tmp957);});
goto _LL0;case 285U: _LL235: _LL236:
# 2526 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp95B)(struct Cyc_List_List*yy1)=Cyc_YY41;struct Cyc_List_List*_tmp95C=({struct Cyc_List_List*_tmp95D=_cycalloc(sizeof(*_tmp95D));({void*_tmp1265=({Cyc_yyget_YY42(&(yyyvsp[0]).v);});_tmp95D->hd=_tmp1265;}),_tmp95D->tl=0;_tmp95D;});_tmp95B(_tmp95C);});
goto _LL0;case 286U: _LL237: _LL238:
# 2527 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp95E)(struct Cyc_List_List*yy1)=Cyc_YY41;struct Cyc_List_List*_tmp95F=({struct Cyc_List_List*_tmp960=_cycalloc(sizeof(*_tmp960));({void*_tmp1267=({Cyc_yyget_YY42(&(yyyvsp[1]).v);});_tmp960->hd=_tmp1267;}),({struct Cyc_List_List*_tmp1266=({Cyc_yyget_YY41(&(yyyvsp[0]).v);});_tmp960->tl=_tmp1266;});_tmp960;});_tmp95E(_tmp95F);});
goto _LL0;case 287U: _LL239: _LL23A:
# 2531 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp961)(void*yy1)=Cyc_YY42;void*_tmp962=(void*)({struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*_tmp963=_cycalloc(sizeof(*_tmp963));_tmp963->tag=0U,({struct Cyc_Absyn_Exp*_tmp1268=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});_tmp963->f1=_tmp1268;});_tmp963;});_tmp961(_tmp962);});
goto _LL0;case 288U: _LL23B: _LL23C:
# 2532 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp964)(void*yy1)=Cyc_YY42;void*_tmp965=(void*)({struct Cyc_Absyn_FieldName_Absyn_Designator_struct*_tmp967=_cycalloc(sizeof(*_tmp967));_tmp967->tag=1U,({struct _fat_ptr*_tmp126A=({struct _fat_ptr*_tmp966=_cycalloc(sizeof(*_tmp966));({struct _fat_ptr _tmp1269=({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});*_tmp966=_tmp1269;});_tmp966;});_tmp967->f1=_tmp126A;});_tmp967;});_tmp964(_tmp965);});
goto _LL0;case 289U: _LL23D: _LL23E: {
# 2537 "parse.y"
struct _tuple26 _stmttmp25=({Cyc_yyget_YY35(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp96A;struct Cyc_Parse_Type_specifier _tmp969;struct Cyc_Absyn_Tqual _tmp968;_LL4AA: _tmp968=_stmttmp25.f1;_tmp969=_stmttmp25.f2;_tmp96A=_stmttmp25.f3;_LL4AB: {struct Cyc_Absyn_Tqual tq=_tmp968;struct Cyc_Parse_Type_specifier tss=_tmp969;struct Cyc_List_List*atts=_tmp96A;
void*t=({Cyc_Parse_speclist2typ(tss,(unsigned)((yyyvsp[0]).l).first_line);});
if(atts != 0)
({void*_tmp96B=0U;({unsigned _tmp126C=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp126B=({const char*_tmp96C="ignoring attributes in type";_tag_fat(_tmp96C,sizeof(char),28U);});Cyc_Warn_warn(_tmp126C,_tmp126B,_tag_fat(_tmp96B,sizeof(void*),0U));});});
# 2539
yyval=({Cyc_YY37(({struct _tuple9*_tmp96D=_cycalloc(sizeof(*_tmp96D));_tmp96D->f1=0,_tmp96D->f2=tq,_tmp96D->f3=t;_tmp96D;}));});
# 2543
goto _LL0;}}case 290U: _LL23F: _LL240: {
# 2544 "parse.y"
struct _tuple26 _stmttmp26=({Cyc_yyget_YY35(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp970;struct Cyc_Parse_Type_specifier _tmp96F;struct Cyc_Absyn_Tqual _tmp96E;_LL4AD: _tmp96E=_stmttmp26.f1;_tmp96F=_stmttmp26.f2;_tmp970=_stmttmp26.f3;_LL4AE: {struct Cyc_Absyn_Tqual tq=_tmp96E;struct Cyc_Parse_Type_specifier tss=_tmp96F;struct Cyc_List_List*atts=_tmp970;
void*t=({Cyc_Parse_speclist2typ(tss,(unsigned)((yyyvsp[0]).l).first_line);});
struct Cyc_List_List*tms=({Cyc_yyget_YY30(&(yyyvsp[1]).v);}).tms;
struct _tuple15 t_info=({Cyc_Parse_apply_tms(tq,t,atts,tms);});
if(t_info.f3 != 0)
# 2550
({void*_tmp971=0U;({unsigned _tmp126E=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp126D=({const char*_tmp972="bad type params, ignoring";_tag_fat(_tmp972,sizeof(char),26U);});Cyc_Warn_warn(_tmp126E,_tmp126D,_tag_fat(_tmp971,sizeof(void*),0U));});});
# 2548
if(t_info.f4 != 0)
# 2552
({void*_tmp973=0U;({unsigned _tmp1270=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp126F=({const char*_tmp974="bad specifiers, ignoring";_tag_fat(_tmp974,sizeof(char),25U);});Cyc_Warn_warn(_tmp1270,_tmp126F,_tag_fat(_tmp973,sizeof(void*),0U));});});
# 2548
yyval=({Cyc_YY37(({struct _tuple9*_tmp975=_cycalloc(sizeof(*_tmp975));_tmp975->f1=0,_tmp975->f2=t_info.f1,_tmp975->f3=t_info.f2;_tmp975;}));});
# 2555
goto _LL0;}}case 291U: _LL241: _LL242:
# 2558 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp976)(void*yy1)=Cyc_YY44;void*_tmp977=({void*(*_tmp978)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmp979=({Cyc_yyget_YY37(&(yyyvsp[0]).v);});unsigned _tmp97A=(unsigned)((yyyvsp[0]).l).first_line;_tmp978(_tmp979,_tmp97A);});_tmp976(_tmp977);});
goto _LL0;case 292U: _LL243: _LL244:
# 2559 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp97B)(void*yy1)=Cyc_YY44;void*_tmp97C=({Cyc_Absyn_join_eff(0);});_tmp97B(_tmp97C);});
goto _LL0;case 293U: _LL245: _LL246:
# 2560 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp97D)(void*yy1)=Cyc_YY44;void*_tmp97E=({void*(*_tmp97F)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp980=({Cyc_yyget_YY40(&(yyyvsp[1]).v);});_tmp97F(_tmp980);});_tmp97D(_tmp97E);});
goto _LL0;case 294U: _LL247: _LL248:
# 2561 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp981)(void*yy1)=Cyc_YY44;void*_tmp982=({void*(*_tmp983)(void*)=Cyc_Absyn_regionsof_eff;void*_tmp984=({Cyc_yyget_YY44(&(yyyvsp[2]).v);});_tmp983(_tmp984);});_tmp981(_tmp982);});
goto _LL0;case 295U: _LL249: _LL24A:
# 2562 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp985)(void*yy1)=Cyc_YY44;void*_tmp986=({void*(*_tmp987)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp988=({struct Cyc_List_List*_tmp989=_cycalloc(sizeof(*_tmp989));({void*_tmp1272=({Cyc_yyget_YY44(&(yyyvsp[0]).v);});_tmp989->hd=_tmp1272;}),({struct Cyc_List_List*_tmp1271=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp989->tl=_tmp1271;});_tmp989;});_tmp987(_tmp988);});_tmp985(_tmp986);});
goto _LL0;case 296U: _LL24B: _LL24C:
# 2568 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp98A)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp98B=({struct Cyc_List_List*_tmp98C=_cycalloc(sizeof(*_tmp98C));({void*_tmp1273=({Cyc_yyget_YY44(&(yyyvsp[0]).v);});_tmp98C->hd=_tmp1273;}),_tmp98C->tl=0;_tmp98C;});_tmp98A(_tmp98B);});
goto _LL0;case 297U: _LL24D: _LL24E:
# 2569 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp98D)(struct Cyc_List_List*yy1)=Cyc_YY40;struct Cyc_List_List*_tmp98E=({struct Cyc_List_List*_tmp98F=_cycalloc(sizeof(*_tmp98F));({void*_tmp1275=({Cyc_yyget_YY44(&(yyyvsp[2]).v);});_tmp98F->hd=_tmp1275;}),({struct Cyc_List_List*_tmp1274=({Cyc_yyget_YY40(&(yyyvsp[0]).v);});_tmp98F->tl=_tmp1274;});_tmp98F;});_tmp98D(_tmp98E);});
goto _LL0;case 298U: _LL24F: _LL250:
# 2574 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp990)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp991=({struct Cyc_Parse_Abstractdeclarator _tmp100F;({struct Cyc_List_List*_tmp1276=({Cyc_yyget_YY26(&(yyyvsp[0]).v);});_tmp100F.tms=_tmp1276;});_tmp100F;});_tmp990(_tmp991);});
goto _LL0;case 299U: _LL251: _LL252:
# 2576 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 300U: _LL253: _LL254:
# 2578 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp992)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp993=({struct Cyc_Parse_Abstractdeclarator _tmp1010;({struct Cyc_List_List*_tmp1277=({struct Cyc_List_List*(*_tmp994)(struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_imp_append;struct Cyc_List_List*_tmp995=({Cyc_yyget_YY26(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmp996=({Cyc_yyget_YY30(&(yyyvsp[1]).v);}).tms;_tmp994(_tmp995,_tmp996);});_tmp1010.tms=_tmp1277;});_tmp1010;});_tmp992(_tmp993);});
goto _LL0;case 301U: _LL255: _LL256:
# 2583 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 302U: _LL257: _LL258:
# 2585 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp997)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp998=({struct Cyc_Parse_Abstractdeclarator _tmp1011;({struct Cyc_List_List*_tmp127A=({struct Cyc_List_List*_tmp99A=_region_malloc(yyr,sizeof(*_tmp99A));({void*_tmp1279=(void*)({struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_tmp999=_region_malloc(yyr,sizeof(*_tmp999));_tmp999->tag=0U,({void*_tmp1278=({Cyc_yyget_YY51(&(yyyvsp[2]).v);});_tmp999->f1=_tmp1278;}),_tmp999->f2=(unsigned)((yyyvsp[2]).l).first_line;_tmp999;});_tmp99A->hd=_tmp1279;}),_tmp99A->tl=0;_tmp99A;});_tmp1011.tms=_tmp127A;});_tmp1011;});_tmp997(_tmp998);});
goto _LL0;case 303U: _LL259: _LL25A:
# 2587 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp99B)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp99C=({struct Cyc_Parse_Abstractdeclarator _tmp1012;({struct Cyc_List_List*_tmp127E=({struct Cyc_List_List*_tmp99E=_region_malloc(yyr,sizeof(*_tmp99E));({void*_tmp127D=(void*)({struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*_tmp99D=_region_malloc(yyr,sizeof(*_tmp99D));_tmp99D->tag=0U,({void*_tmp127C=({Cyc_yyget_YY51(&(yyyvsp[3]).v);});_tmp99D->f1=_tmp127C;}),_tmp99D->f2=(unsigned)((yyyvsp[3]).l).first_line;_tmp99D;});_tmp99E->hd=_tmp127D;}),({struct Cyc_List_List*_tmp127B=({Cyc_yyget_YY30(&(yyyvsp[0]).v);}).tms;_tmp99E->tl=_tmp127B;});_tmp99E;});_tmp1012.tms=_tmp127E;});_tmp1012;});_tmp99B(_tmp99C);});
goto _LL0;case 304U: _LL25B: _LL25C:
# 2589 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp99F)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp9A0=({struct Cyc_Parse_Abstractdeclarator _tmp1013;({struct Cyc_List_List*_tmp1282=({struct Cyc_List_List*_tmp9A2=_region_malloc(yyr,sizeof(*_tmp9A2));({void*_tmp1281=(void*)({struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_tmp9A1=_region_malloc(yyr,sizeof(*_tmp9A1));_tmp9A1->tag=1U,({struct Cyc_Absyn_Exp*_tmp1280=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});_tmp9A1->f1=_tmp1280;}),({void*_tmp127F=({Cyc_yyget_YY51(&(yyyvsp[3]).v);});_tmp9A1->f2=_tmp127F;}),_tmp9A1->f3=(unsigned)((yyyvsp[3]).l).first_line;_tmp9A1;});_tmp9A2->hd=_tmp1281;}),_tmp9A2->tl=0;_tmp9A2;});_tmp1013.tms=_tmp1282;});_tmp1013;});_tmp99F(_tmp9A0);});
goto _LL0;case 305U: _LL25D: _LL25E:
# 2591 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp9A3)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp9A4=({struct Cyc_Parse_Abstractdeclarator _tmp1014;({struct Cyc_List_List*_tmp1287=({struct Cyc_List_List*_tmp9A6=_region_malloc(yyr,sizeof(*_tmp9A6));({void*_tmp1286=(void*)({struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*_tmp9A5=_region_malloc(yyr,sizeof(*_tmp9A5));_tmp9A5->tag=1U,({struct Cyc_Absyn_Exp*_tmp1285=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmp9A5->f1=_tmp1285;}),({void*_tmp1284=({Cyc_yyget_YY51(&(yyyvsp[4]).v);});_tmp9A5->f2=_tmp1284;}),_tmp9A5->f3=(unsigned)((yyyvsp[4]).l).first_line;_tmp9A5;});_tmp9A6->hd=_tmp1286;}),({
struct Cyc_List_List*_tmp1283=({Cyc_yyget_YY30(&(yyyvsp[0]).v);}).tms;_tmp9A6->tl=_tmp1283;});_tmp9A6;});
# 2591
_tmp1014.tms=_tmp1287;});_tmp1014;});_tmp9A3(_tmp9A4);});
# 2594
goto _LL0;case 306U: _LL25F: _LL260: {
# 2600 "parse.y"
struct Cyc_List_List*ieff=({struct Cyc_List_List*(*_tmp9AF)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp9B0=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp9B1=({Cyc_yyget_YY49(&(yyyvsp[6]).v);});_tmp9AF(_tmp9B0,_tmp9B1);});
struct Cyc_List_List*oeff=({struct Cyc_List_List*(*_tmp9AC)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp9AD=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp9AE=({Cyc_yyget_YY49(&(yyyvsp[7]).v);});_tmp9AC(_tmp9AD,_tmp9AE);});
struct Cyc_List_List*throws=({Cyc_yyget_YY64(&(yyyvsp[8]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp9A7)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp9A8=({struct Cyc_Parse_Abstractdeclarator _tmp1015;({struct Cyc_List_List*_tmp128F=({struct Cyc_List_List*_tmp9AB=_region_malloc(yyr,sizeof(*_tmp9AB));({void*_tmp128E=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp9AA=_region_malloc(yyr,sizeof(*_tmp9AA));
_tmp9AA->tag=3U,({void*_tmp128D=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp9A9=_region_malloc(yyr,sizeof(*_tmp9A9));
_tmp9A9->tag=1U,_tmp9A9->f1=0,_tmp9A9->f2=0,_tmp9A9->f3=0,({void*_tmp128C=({Cyc_yyget_YY49(&(yyyvsp[1]).v);});_tmp9A9->f4=_tmp128C;}),({struct Cyc_List_List*_tmp128B=({Cyc_yyget_YY50(&(yyyvsp[2]).v);});_tmp9A9->f5=_tmp128B;}),({struct Cyc_Absyn_Exp*_tmp128A=({Cyc_yyget_YY57(&(yyyvsp[4]).v);});_tmp9A9->f6=_tmp128A;}),({struct Cyc_Absyn_Exp*_tmp1289=({Cyc_yyget_YY57(&(yyyvsp[5]).v);});_tmp9A9->f7=_tmp1289;}),_tmp9A9->f8=ieff,_tmp9A9->f9=oeff,_tmp9A9->f10=throws,({
int _tmp1288=({Cyc_yyget_YY65(&(yyyvsp[9]).v);});_tmp9A9->f11=_tmp1288;});_tmp9A9;});
# 2604
_tmp9AA->f1=_tmp128D;});_tmp9AA;});
# 2603
_tmp9AB->hd=_tmp128E;}),_tmp9AB->tl=0;_tmp9AB;});_tmp1015.tms=_tmp128F;});_tmp1015;});_tmp9A7(_tmp9A8);});
# 2608
goto _LL0;}case 307U: _LL261: _LL262: {
# 2612 "parse.y"
struct _tuple27*_stmttmp27=({Cyc_yyget_YY39(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmp9B6;void*_tmp9B5;struct Cyc_Absyn_VarargInfo*_tmp9B4;int _tmp9B3;struct Cyc_List_List*_tmp9B2;_LL4B0: _tmp9B2=_stmttmp27->f1;_tmp9B3=_stmttmp27->f2;_tmp9B4=_stmttmp27->f3;_tmp9B5=_stmttmp27->f4;_tmp9B6=_stmttmp27->f5;_LL4B1: {struct Cyc_List_List*lis=_tmp9B2;int b=_tmp9B3;struct Cyc_Absyn_VarargInfo*c=_tmp9B4;void*eff=_tmp9B5;struct Cyc_List_List*po=_tmp9B6;
struct Cyc_List_List*ieff=({struct Cyc_List_List*(*_tmp9BF)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp9C0=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp9C1=({Cyc_yyget_YY49(&(yyyvsp[5]).v);});_tmp9BF(_tmp9C0,_tmp9C1);});
struct Cyc_List_List*oeff=({struct Cyc_List_List*(*_tmp9BC)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp9BD=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp9BE=({Cyc_yyget_YY49(&(yyyvsp[6]).v);});_tmp9BC(_tmp9BD,_tmp9BE);});
struct Cyc_List_List*throws=({Cyc_yyget_YY64(&(yyyvsp[7]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp9B7)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp9B8=({struct Cyc_Parse_Abstractdeclarator _tmp1016;({struct Cyc_List_List*_tmp1295=({struct Cyc_List_List*_tmp9BB=_region_malloc(yyr,sizeof(*_tmp9BB));({void*_tmp1294=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp9BA=_region_malloc(yyr,sizeof(*_tmp9BA));
_tmp9BA->tag=3U,({void*_tmp1293=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp9B9=_region_malloc(yyr,sizeof(*_tmp9B9));
_tmp9B9->tag=1U,_tmp9B9->f1=lis,_tmp9B9->f2=b,_tmp9B9->f3=c,_tmp9B9->f4=eff,_tmp9B9->f5=po,({struct Cyc_Absyn_Exp*_tmp1292=({Cyc_yyget_YY57(&(yyyvsp[3]).v);});_tmp9B9->f6=_tmp1292;}),({struct Cyc_Absyn_Exp*_tmp1291=({Cyc_yyget_YY57(&(yyyvsp[4]).v);});_tmp9B9->f7=_tmp1291;}),_tmp9B9->f8=ieff,_tmp9B9->f9=oeff,_tmp9B9->f10=throws,({
int _tmp1290=({Cyc_yyget_YY65(&(yyyvsp[8]).v);});_tmp9B9->f11=_tmp1290;});_tmp9B9;});
# 2617
_tmp9BA->f1=_tmp1293;});_tmp9BA;});
# 2616
_tmp9BB->hd=_tmp1294;}),_tmp9BB->tl=0;_tmp9BB;});_tmp1016.tms=_tmp1295;});_tmp1016;});_tmp9B7(_tmp9B8);});
# 2621
goto _LL0;}}case 308U: _LL263: _LL264: {
# 2626 "parse.y"
struct Cyc_List_List*ieff=({struct Cyc_List_List*(*_tmp9CA)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp9CB=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp9CC=({Cyc_yyget_YY49(&(yyyvsp[7]).v);});_tmp9CA(_tmp9CB,_tmp9CC);});
struct Cyc_List_List*oeff=({struct Cyc_List_List*(*_tmp9C7)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp9C8=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp9C9=({Cyc_yyget_YY49(&(yyyvsp[8]).v);});_tmp9C7(_tmp9C8,_tmp9C9);});
struct Cyc_List_List*throws=({Cyc_yyget_YY64(&(yyyvsp[9]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp9C2)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp9C3=({struct Cyc_Parse_Abstractdeclarator _tmp1017;({struct Cyc_List_List*_tmp129E=({struct Cyc_List_List*_tmp9C6=_region_malloc(yyr,sizeof(*_tmp9C6));({void*_tmp129D=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp9C5=_region_malloc(yyr,sizeof(*_tmp9C5));
_tmp9C5->tag=3U,({void*_tmp129C=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp9C4=_region_malloc(yyr,sizeof(*_tmp9C4));
_tmp9C4->tag=1U,_tmp9C4->f1=0,_tmp9C4->f2=0,_tmp9C4->f3=0,({
void*_tmp129B=({Cyc_yyget_YY49(&(yyyvsp[2]).v);});_tmp9C4->f4=_tmp129B;}),({struct Cyc_List_List*_tmp129A=({Cyc_yyget_YY50(&(yyyvsp[3]).v);});_tmp9C4->f5=_tmp129A;}),({struct Cyc_Absyn_Exp*_tmp1299=({Cyc_yyget_YY57(&(yyyvsp[5]).v);});_tmp9C4->f6=_tmp1299;}),({struct Cyc_Absyn_Exp*_tmp1298=({Cyc_yyget_YY57(&(yyyvsp[6]).v);});_tmp9C4->f7=_tmp1298;}),_tmp9C4->f8=ieff,_tmp9C4->f9=oeff,_tmp9C4->f10=throws,({
int _tmp1297=({Cyc_yyget_YY65(&(yyyvsp[10]).v);});_tmp9C4->f11=_tmp1297;});_tmp9C4;});
# 2630
_tmp9C5->f1=_tmp129C;});_tmp9C5;});
# 2629
_tmp9C6->hd=_tmp129D;}),({
# 2634
struct Cyc_List_List*_tmp1296=({Cyc_yyget_YY30(&(yyyvsp[0]).v);}).tms;_tmp9C6->tl=_tmp1296;});_tmp9C6;});
# 2629
_tmp1017.tms=_tmp129E;});_tmp1017;});_tmp9C2(_tmp9C3);});
# 2636
goto _LL0;}case 309U: _LL265: _LL266: {
# 2642 "parse.y"
struct _tuple27*_stmttmp28=({Cyc_yyget_YY39(&(yyyvsp[2]).v);});struct Cyc_List_List*_tmp9D1;void*_tmp9D0;struct Cyc_Absyn_VarargInfo*_tmp9CF;int _tmp9CE;struct Cyc_List_List*_tmp9CD;_LL4B3: _tmp9CD=_stmttmp28->f1;_tmp9CE=_stmttmp28->f2;_tmp9CF=_stmttmp28->f3;_tmp9D0=_stmttmp28->f4;_tmp9D1=_stmttmp28->f5;_LL4B4: {struct Cyc_List_List*lis=_tmp9CD;int b=_tmp9CE;struct Cyc_Absyn_VarargInfo*c=_tmp9CF;void*eff=_tmp9D0;struct Cyc_List_List*po=_tmp9D1;
struct Cyc_List_List*ieff=({struct Cyc_List_List*(*_tmp9DA)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp9DB=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp9DC=({Cyc_yyget_YY49(&(yyyvsp[6]).v);});_tmp9DA(_tmp9DB,_tmp9DC);});
struct Cyc_List_List*oeff=({struct Cyc_List_List*(*_tmp9D7)(unsigned loc,void*t)=Cyc_Absyn_parse_rgneffects;unsigned _tmp9D8=(unsigned)((yyyvsp[0]).l).first_line;void*_tmp9D9=({Cyc_yyget_YY49(&(yyyvsp[7]).v);});_tmp9D7(_tmp9D8,_tmp9D9);});
struct Cyc_List_List*throws=({Cyc_yyget_YY64(&(yyyvsp[8]).v);});
yyval=({union Cyc_YYSTYPE(*_tmp9D2)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp9D3=({struct Cyc_Parse_Abstractdeclarator _tmp1018;({struct Cyc_List_List*_tmp12A5=({struct Cyc_List_List*_tmp9D6=_region_malloc(yyr,sizeof(*_tmp9D6));
({void*_tmp12A4=(void*)({struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*_tmp9D5=_region_malloc(yyr,sizeof(*_tmp9D5));
_tmp9D5->tag=3U,({void*_tmp12A3=(void*)({struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*_tmp9D4=_region_malloc(yyr,sizeof(*_tmp9D4));
_tmp9D4->tag=1U,_tmp9D4->f1=lis,_tmp9D4->f2=b,_tmp9D4->f3=c,_tmp9D4->f4=eff,_tmp9D4->f5=po,({struct Cyc_Absyn_Exp*_tmp12A2=({Cyc_yyget_YY57(&(yyyvsp[4]).v);});_tmp9D4->f6=_tmp12A2;}),({struct Cyc_Absyn_Exp*_tmp12A1=({Cyc_yyget_YY57(&(yyyvsp[5]).v);});_tmp9D4->f7=_tmp12A1;}),_tmp9D4->f8=ieff,_tmp9D4->f9=oeff,_tmp9D4->f10=throws,({
int _tmp12A0=({Cyc_yyget_YY65(&(yyyvsp[9]).v);});_tmp9D4->f11=_tmp12A0;});_tmp9D4;});
# 2648
_tmp9D5->f1=_tmp12A3;});_tmp9D5;});
# 2647
_tmp9D6->hd=_tmp12A4;}),({
# 2650
struct Cyc_List_List*_tmp129F=({Cyc_yyget_YY30(&(yyyvsp[0]).v);}).tms;_tmp9D6->tl=_tmp129F;});_tmp9D6;});
# 2646
_tmp1018.tms=_tmp12A5;});_tmp1018;});_tmp9D2(_tmp9D3);});
# 2652
goto _LL0;}}case 310U: _LL267: _LL268: {
# 2654
struct Cyc_List_List*_tmp9DD=({struct Cyc_List_List*(*_tmp9E2)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp9E3)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmp9E3;});struct Cyc_Absyn_Tvar*(*_tmp9E4)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmp9E5=(unsigned)({yylloc.first_line=((yyyvsp[1]).l).first_line;((yyyvsp[1]).l).first_line;});struct Cyc_List_List*_tmp9E6=({struct Cyc_List_List*(*_tmp9E7)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmp9E8=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmp9E7(_tmp9E8);});_tmp9E2(_tmp9E4,_tmp9E5,_tmp9E6);});struct Cyc_List_List*ts=_tmp9DD;
yyval=({union Cyc_YYSTYPE(*_tmp9DE)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp9DF=({struct Cyc_Parse_Abstractdeclarator _tmp1019;({struct Cyc_List_List*_tmp12A9=({struct Cyc_List_List*_tmp9E1=_region_malloc(yyr,sizeof(*_tmp9E1));({void*_tmp12A8=(void*)({struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*_tmp9E0=_region_malloc(yyr,sizeof(*_tmp9E0));_tmp9E0->tag=4U,_tmp9E0->f1=ts,({unsigned _tmp12A7=(unsigned)({yylloc.first_line=((yyyvsp[1]).l).first_line;((yyyvsp[1]).l).first_line;});_tmp9E0->f2=_tmp12A7;}),_tmp9E0->f3=0;_tmp9E0;});_tmp9E1->hd=_tmp12A8;}),({
struct Cyc_List_List*_tmp12A6=({Cyc_yyget_YY30(&(yyyvsp[0]).v);}).tms;_tmp9E1->tl=_tmp12A6;});_tmp9E1;});
# 2655
_tmp1019.tms=_tmp12A9;});_tmp1019;});_tmp9DE(_tmp9DF);});
# 2658
goto _LL0;}case 311U: _LL269: _LL26A:
# 2659 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp9E9)(struct Cyc_Parse_Abstractdeclarator yy1)=Cyc_YY30;struct Cyc_Parse_Abstractdeclarator _tmp9EA=({struct Cyc_Parse_Abstractdeclarator _tmp101A;({struct Cyc_List_List*_tmp12AD=({struct Cyc_List_List*_tmp9EC=_region_malloc(yyr,sizeof(*_tmp9EC));({void*_tmp12AC=(void*)({struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*_tmp9EB=_region_malloc(yyr,sizeof(*_tmp9EB));_tmp9EB->tag=5U,_tmp9EB->f1=(unsigned)((yyyvsp[1]).l).first_line,({struct Cyc_List_List*_tmp12AB=({Cyc_yyget_YY45(&(yyyvsp[1]).v);});_tmp9EB->f2=_tmp12AB;});_tmp9EB;});_tmp9EC->hd=_tmp12AC;}),({struct Cyc_List_List*_tmp12AA=({Cyc_yyget_YY30(&(yyyvsp[0]).v);}).tms;_tmp9EC->tl=_tmp12AA;});_tmp9EC;});_tmp101A.tms=_tmp12AD;});_tmp101A;});_tmp9E9(_tmp9EA);});
# 2661
goto _LL0;case 312U: _LL26B: _LL26C:
# 2665 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 313U: _LL26D: _LL26E:
# 2666 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 314U: _LL26F: _LL270:
# 2667 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 315U: _LL271: _LL272:
# 2668 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 316U: _LL273: _LL274:
# 2669 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 317U: _LL275: _LL276:
# 2670 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 318U: _LL277: _LL278:
# 2676 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp9ED)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmp9EE=({struct Cyc_Absyn_Stmt*(*_tmp9EF)(void*,unsigned)=Cyc_Absyn_new_stmt;void*_tmp9F0=(void*)({struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_tmp9F2=_cycalloc(sizeof(*_tmp9F2));_tmp9F2->tag=13U,({struct _fat_ptr*_tmp12B0=({struct _fat_ptr*_tmp9F1=_cycalloc(sizeof(*_tmp9F1));({struct _fat_ptr _tmp12AF=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmp9F1=_tmp12AF;});_tmp9F1;});_tmp9F2->f1=_tmp12B0;}),({struct Cyc_Absyn_Stmt*_tmp12AE=({Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);});_tmp9F2->f2=_tmp12AE;});_tmp9F2;});unsigned _tmp9F3=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp9EF(_tmp9F0,_tmp9F3);});_tmp9ED(_tmp9EE);});
goto _LL0;case 319U: _LL279: _LL27A:
# 2680 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp9F4)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmp9F5=({Cyc_Absyn_skip_stmt((unsigned)((yyyvsp[0]).l).first_line);});_tmp9F4(_tmp9F5);});
goto _LL0;case 320U: _LL27B: _LL27C:
# 2681 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp9F6)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmp9F7=({struct Cyc_Absyn_Stmt*(*_tmp9F8)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp9F9=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});unsigned _tmp9FA=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp9F8(_tmp9F9,_tmp9FA);});_tmp9F6(_tmp9F7);});
goto _LL0;case 321U: _LL27D: _LL27E:
# 2686 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp9FB)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmp9FC=({struct Cyc_Absyn_Stmt*(*_tmp9FD)(unsigned)=Cyc_Absyn_skip_stmt;unsigned _tmp9FE=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmp9FD(_tmp9FE);});_tmp9FB(_tmp9FC);});
goto _LL0;case 322U: _LL27F: _LL280:
# 2687 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 323U: _LL281: _LL282:
# 2692 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmp9FF)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA00=({struct Cyc_Absyn_Stmt*(*_tmpA01)(struct Cyc_List_List*ds,struct Cyc_Absyn_Stmt*s)=Cyc_Parse_flatten_declarations;struct Cyc_List_List*_tmpA02=({Cyc_yyget_YY16(&(yyyvsp[0]).v);});struct Cyc_Absyn_Stmt*_tmpA03=({Cyc_Absyn_skip_stmt((unsigned)((yyyvsp[0]).l).first_line);});_tmpA01(_tmpA02,_tmpA03);});_tmp9FF(_tmpA00);});
goto _LL0;case 324U: _LL283: _LL284:
# 2693 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA04)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA05=({struct Cyc_Absyn_Stmt*(*_tmpA06)(struct Cyc_List_List*ds,struct Cyc_Absyn_Stmt*s)=Cyc_Parse_flatten_declarations;struct Cyc_List_List*_tmpA07=({Cyc_yyget_YY16(&(yyyvsp[0]).v);});struct Cyc_Absyn_Stmt*_tmpA08=({Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);});_tmpA06(_tmpA07,_tmpA08);});_tmpA04(_tmpA05);});
goto _LL0;case 325U: _LL285: _LL286:
# 2694 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA09)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA0A=({struct Cyc_Absyn_Stmt*(*_tmpA0B)(void*,unsigned)=Cyc_Absyn_new_stmt;void*_tmpA0C=(void*)({struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_tmpA11=_cycalloc(sizeof(*_tmpA11));_tmpA11->tag=13U,({struct _fat_ptr*_tmp12B3=({struct _fat_ptr*_tmpA0D=_cycalloc(sizeof(*_tmpA0D));({struct _fat_ptr _tmp12B2=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmpA0D=_tmp12B2;});_tmpA0D;});_tmpA11->f1=_tmp12B3;}),({struct Cyc_Absyn_Stmt*_tmp12B1=({struct Cyc_Absyn_Stmt*(*_tmpA0E)(struct Cyc_List_List*ds,struct Cyc_Absyn_Stmt*s)=Cyc_Parse_flatten_declarations;struct Cyc_List_List*_tmpA0F=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});struct Cyc_Absyn_Stmt*_tmpA10=({Cyc_Absyn_skip_stmt(0U);});_tmpA0E(_tmpA0F,_tmpA10);});_tmpA11->f2=_tmp12B1;});_tmpA11;});unsigned _tmpA12=(unsigned)((yyyvsp[0]).l).first_line;_tmpA0B(_tmpA0C,_tmpA12);});_tmpA09(_tmpA0A);});
# 2696
goto _LL0;case 326U: _LL287: _LL288:
# 2696 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA13)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA14=({struct Cyc_Absyn_Stmt*(*_tmpA15)(void*,unsigned)=Cyc_Absyn_new_stmt;void*_tmpA16=(void*)({struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*_tmpA1B=_cycalloc(sizeof(*_tmpA1B));_tmpA1B->tag=13U,({struct _fat_ptr*_tmp12B6=({struct _fat_ptr*_tmpA17=_cycalloc(sizeof(*_tmpA17));({struct _fat_ptr _tmp12B5=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmpA17=_tmp12B5;});_tmpA17;});_tmpA1B->f1=_tmp12B6;}),({struct Cyc_Absyn_Stmt*_tmp12B4=({struct Cyc_Absyn_Stmt*(*_tmpA18)(struct Cyc_List_List*ds,struct Cyc_Absyn_Stmt*s)=Cyc_Parse_flatten_declarations;struct Cyc_List_List*_tmpA19=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});struct Cyc_Absyn_Stmt*_tmpA1A=({Cyc_yyget_Stmt_tok(&(yyyvsp[3]).v);});_tmpA18(_tmpA19,_tmpA1A);});_tmpA1B->f2=_tmp12B4;});_tmpA1B;});unsigned _tmpA1C=(unsigned)((yyyvsp[0]).l).first_line;_tmpA15(_tmpA16,_tmpA1C);});_tmpA13(_tmpA14);});
# 2698
goto _LL0;case 327U: _LL289: _LL28A:
# 2698 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 328U: _LL28B: _LL28C:
# 2699 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA1D)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA1E=({struct Cyc_Absyn_Stmt*(*_tmpA1F)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmpA20=({Cyc_yyget_Stmt_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Stmt*_tmpA21=({Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);});unsigned _tmpA22=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA1F(_tmpA20,_tmpA21,_tmpA22);});_tmpA1D(_tmpA1E);});
goto _LL0;case 329U: _LL28D: _LL28E:
# 2700 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA23)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA24=({struct Cyc_Absyn_Stmt*(*_tmpA25)(struct Cyc_Absyn_Decl*d,struct Cyc_Absyn_Stmt*s)=Cyc_Parse_flatten_decl;struct Cyc_Absyn_Decl*_tmpA26=({struct Cyc_Absyn_Decl*(*_tmpA27)(void*,unsigned)=Cyc_Absyn_new_decl;void*_tmpA28=(void*)({struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmpA29=_cycalloc(sizeof(*_tmpA29));_tmpA29->tag=1U,({struct Cyc_Absyn_Fndecl*_tmp12B7=({Cyc_yyget_YY15(&(yyyvsp[0]).v);});_tmpA29->f1=_tmp12B7;});_tmpA29;});unsigned _tmpA2A=(unsigned)((yyyvsp[0]).l).first_line;_tmpA27(_tmpA28,_tmpA2A);});struct Cyc_Absyn_Stmt*_tmpA2B=({Cyc_Absyn_skip_stmt(0U);});_tmpA25(_tmpA26,_tmpA2B);});_tmpA23(_tmpA24);});
# 2702
goto _LL0;case 330U: _LL28F: _LL290:
# 2703 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA2C)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA2D=({struct Cyc_Absyn_Stmt*(*_tmpA2E)(struct Cyc_Absyn_Decl*d,struct Cyc_Absyn_Stmt*s)=Cyc_Parse_flatten_decl;struct Cyc_Absyn_Decl*_tmpA2F=({struct Cyc_Absyn_Decl*(*_tmpA30)(void*,unsigned)=Cyc_Absyn_new_decl;void*_tmpA31=(void*)({struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*_tmpA32=_cycalloc(sizeof(*_tmpA32));_tmpA32->tag=1U,({struct Cyc_Absyn_Fndecl*_tmp12B8=({Cyc_yyget_YY15(&(yyyvsp[0]).v);});_tmpA32->f1=_tmp12B8;});_tmpA32;});unsigned _tmpA33=(unsigned)((yyyvsp[0]).l).first_line;_tmpA30(_tmpA31,_tmpA33);});struct Cyc_Absyn_Stmt*_tmpA34=({Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);});_tmpA2E(_tmpA2F,_tmpA34);});_tmpA2C(_tmpA2D);});
goto _LL0;case 331U: _LL291: _LL292:
# 2708 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA35)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA36=({struct Cyc_Absyn_Stmt*(*_tmpA37)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_ifthenelse_stmt;struct Cyc_Absyn_Exp*_tmpA38=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Stmt*_tmpA39=({Cyc_yyget_Stmt_tok(&(yyyvsp[4]).v);});struct Cyc_Absyn_Stmt*_tmpA3A=({Cyc_Absyn_skip_stmt(0U);});unsigned _tmpA3B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA37(_tmpA38,_tmpA39,_tmpA3A,_tmpA3B);});_tmpA35(_tmpA36);});
goto _LL0;case 332U: _LL293: _LL294:
# 2710 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA3C)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA3D=({struct Cyc_Absyn_Stmt*(*_tmpA3E)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_ifthenelse_stmt;struct Cyc_Absyn_Exp*_tmpA3F=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Stmt*_tmpA40=({Cyc_yyget_Stmt_tok(&(yyyvsp[4]).v);});struct Cyc_Absyn_Stmt*_tmpA41=({Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);});unsigned _tmpA42=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA3E(_tmpA3F,_tmpA40,_tmpA41,_tmpA42);});_tmpA3C(_tmpA3D);});
goto _LL0;case 333U: _LL295: _LL296:
# 2716 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA43)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA44=({struct Cyc_Absyn_Stmt*(*_tmpA45)(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_switch_stmt;struct Cyc_Absyn_Exp*_tmpA46=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_List_List*_tmpA47=({Cyc_yyget_YY8(&(yyyvsp[5]).v);});unsigned _tmpA48=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA45(_tmpA46,_tmpA47,_tmpA48);});_tmpA43(_tmpA44);});
goto _LL0;case 334U: _LL297: _LL298: {
# 2719
struct Cyc_Absyn_Exp*e=({struct Cyc_Absyn_Exp*(*_tmpA4F)(struct _tuple0*,unsigned)=Cyc_Absyn_unknownid_exp;struct _tuple0*_tmpA50=({Cyc_yyget_QualId_tok(&(yyyvsp[1]).v);});unsigned _tmpA51=(unsigned)((yyyvsp[1]).l).first_line;_tmpA4F(_tmpA50,_tmpA51);});
yyval=({union Cyc_YYSTYPE(*_tmpA49)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA4A=({struct Cyc_Absyn_Stmt*(*_tmpA4B)(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_switch_stmt;struct Cyc_Absyn_Exp*_tmpA4C=e;struct Cyc_List_List*_tmpA4D=({Cyc_yyget_YY8(&(yyyvsp[3]).v);});unsigned _tmpA4E=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA4B(_tmpA4C,_tmpA4D,_tmpA4E);});_tmpA49(_tmpA4A);});
goto _LL0;}case 335U: _LL299: _LL29A: {
# 2723
struct Cyc_Absyn_Exp*_tmpA52=({struct Cyc_Absyn_Exp*(*_tmpA59)(struct Cyc_List_List*,unsigned)=Cyc_Absyn_tuple_exp;struct Cyc_List_List*_tmpA5A=({Cyc_yyget_YY4(&(yyyvsp[3]).v);});unsigned _tmpA5B=(unsigned)({yylloc.first_line=((yyyvsp[1]).l).first_line;((yyyvsp[1]).l).first_line;});_tmpA59(_tmpA5A,_tmpA5B);});struct Cyc_Absyn_Exp*e=_tmpA52;
yyval=({union Cyc_YYSTYPE(*_tmpA53)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA54=({struct Cyc_Absyn_Stmt*(*_tmpA55)(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_switch_stmt;struct Cyc_Absyn_Exp*_tmpA56=e;struct Cyc_List_List*_tmpA57=({Cyc_yyget_YY8(&(yyyvsp[6]).v);});unsigned _tmpA58=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA55(_tmpA56,_tmpA57,_tmpA58);});_tmpA53(_tmpA54);});
# 2726
goto _LL0;}case 336U: _LL29B: _LL29C:
# 2729 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA5C)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA5D=({struct Cyc_Absyn_Stmt*(*_tmpA5E)(struct Cyc_Absyn_Stmt*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_trycatch_stmt;struct Cyc_Absyn_Stmt*_tmpA5F=({Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmpA60=({Cyc_yyget_YY8(&(yyyvsp[4]).v);});unsigned _tmpA61=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA5E(_tmpA5F,_tmpA60,_tmpA61);});_tmpA5C(_tmpA5D);});
goto _LL0;case 337U: _LL29D: _LL29E:
# 2743 "parse.y"
 yyval=({Cyc_YY8(0);});
goto _LL0;case 338U: _LL29F: _LL2A0:
# 2747 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA62)(struct Cyc_List_List*yy1)=Cyc_YY8;struct Cyc_List_List*_tmpA63=({struct Cyc_List_List*_tmpA65=_cycalloc(sizeof(*_tmpA65));({struct Cyc_Absyn_Switch_clause*_tmp12BD=({struct Cyc_Absyn_Switch_clause*_tmpA64=_cycalloc(sizeof(*_tmpA64));({struct Cyc_Absyn_Pat*_tmp12BC=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,(unsigned)((yyyvsp[0]).l).first_line);});_tmpA64->pattern=_tmp12BC;}),_tmpA64->pat_vars=0,_tmpA64->where_clause=0,({
struct Cyc_Absyn_Stmt*_tmp12BB=({Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);});_tmpA64->body=_tmp12BB;}),({unsigned _tmp12BA=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA64->loc=_tmp12BA;});_tmpA64;});
# 2747
_tmpA65->hd=_tmp12BD;}),({
# 2749
struct Cyc_List_List*_tmp12B9=({Cyc_yyget_YY8(&(yyyvsp[3]).v);});_tmpA65->tl=_tmp12B9;});_tmpA65;});_tmpA62(_tmpA63);});
goto _LL0;case 339U: _LL2A1: _LL2A2:
# 2751 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA66)(struct Cyc_List_List*yy1)=Cyc_YY8;struct Cyc_List_List*_tmpA67=({struct Cyc_List_List*_tmpA69=_cycalloc(sizeof(*_tmpA69));({struct Cyc_Absyn_Switch_clause*_tmp12C2=({struct Cyc_Absyn_Switch_clause*_tmpA68=_cycalloc(sizeof(*_tmpA68));({struct Cyc_Absyn_Pat*_tmp12C1=({Cyc_yyget_YY9(&(yyyvsp[1]).v);});_tmpA68->pattern=_tmp12C1;}),_tmpA68->pat_vars=0,_tmpA68->where_clause=0,({
struct Cyc_Absyn_Stmt*_tmp12C0=({Cyc_Absyn_fallthru_stmt(0,(unsigned)((yyyvsp[2]).l).first_line);});_tmpA68->body=_tmp12C0;}),({
unsigned _tmp12BF=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA68->loc=_tmp12BF;});_tmpA68;});
# 2751
_tmpA69->hd=_tmp12C2;}),({
# 2753
struct Cyc_List_List*_tmp12BE=({Cyc_yyget_YY8(&(yyyvsp[3]).v);});_tmpA69->tl=_tmp12BE;});_tmpA69;});_tmpA66(_tmpA67);});
goto _LL0;case 340U: _LL2A3: _LL2A4:
# 2755 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA6A)(struct Cyc_List_List*yy1)=Cyc_YY8;struct Cyc_List_List*_tmpA6B=({struct Cyc_List_List*_tmpA6D=_cycalloc(sizeof(*_tmpA6D));({struct Cyc_Absyn_Switch_clause*_tmp12C7=({struct Cyc_Absyn_Switch_clause*_tmpA6C=_cycalloc(sizeof(*_tmpA6C));({struct Cyc_Absyn_Pat*_tmp12C6=({Cyc_yyget_YY9(&(yyyvsp[1]).v);});_tmpA6C->pattern=_tmp12C6;}),_tmpA6C->pat_vars=0,_tmpA6C->where_clause=0,({struct Cyc_Absyn_Stmt*_tmp12C5=({Cyc_yyget_Stmt_tok(&(yyyvsp[3]).v);});_tmpA6C->body=_tmp12C5;}),({unsigned _tmp12C4=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA6C->loc=_tmp12C4;});_tmpA6C;});_tmpA6D->hd=_tmp12C7;}),({struct Cyc_List_List*_tmp12C3=({Cyc_yyget_YY8(&(yyyvsp[4]).v);});_tmpA6D->tl=_tmp12C3;});_tmpA6D;});_tmpA6A(_tmpA6B);});
goto _LL0;case 341U: _LL2A5: _LL2A6:
# 2757 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA6E)(struct Cyc_List_List*yy1)=Cyc_YY8;struct Cyc_List_List*_tmpA6F=({struct Cyc_List_List*_tmpA71=_cycalloc(sizeof(*_tmpA71));({struct Cyc_Absyn_Switch_clause*_tmp12CD=({struct Cyc_Absyn_Switch_clause*_tmpA70=_cycalloc(sizeof(*_tmpA70));({struct Cyc_Absyn_Pat*_tmp12CC=({Cyc_yyget_YY9(&(yyyvsp[1]).v);});_tmpA70->pattern=_tmp12CC;}),_tmpA70->pat_vars=0,({struct Cyc_Absyn_Exp*_tmp12CB=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});_tmpA70->where_clause=_tmp12CB;}),({
struct Cyc_Absyn_Stmt*_tmp12CA=({Cyc_Absyn_fallthru_stmt(0,(unsigned)((yyyvsp[4]).l).first_line);});_tmpA70->body=_tmp12CA;}),({
unsigned _tmp12C9=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA70->loc=_tmp12C9;});_tmpA70;});
# 2757
_tmpA71->hd=_tmp12CD;}),({
# 2759
struct Cyc_List_List*_tmp12C8=({Cyc_yyget_YY8(&(yyyvsp[5]).v);});_tmpA71->tl=_tmp12C8;});_tmpA71;});_tmpA6E(_tmpA6F);});
goto _LL0;case 342U: _LL2A7: _LL2A8:
# 2761 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA72)(struct Cyc_List_List*yy1)=Cyc_YY8;struct Cyc_List_List*_tmpA73=({struct Cyc_List_List*_tmpA75=_cycalloc(sizeof(*_tmpA75));({struct Cyc_Absyn_Switch_clause*_tmp12D3=({struct Cyc_Absyn_Switch_clause*_tmpA74=_cycalloc(sizeof(*_tmpA74));({struct Cyc_Absyn_Pat*_tmp12D2=({Cyc_yyget_YY9(&(yyyvsp[1]).v);});_tmpA74->pattern=_tmp12D2;}),_tmpA74->pat_vars=0,({struct Cyc_Absyn_Exp*_tmp12D1=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});_tmpA74->where_clause=_tmp12D1;}),({struct Cyc_Absyn_Stmt*_tmp12D0=({Cyc_yyget_Stmt_tok(&(yyyvsp[5]).v);});_tmpA74->body=_tmp12D0;}),({unsigned _tmp12CF=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA74->loc=_tmp12CF;});_tmpA74;});_tmpA75->hd=_tmp12D3;}),({struct Cyc_List_List*_tmp12CE=({Cyc_yyget_YY8(&(yyyvsp[6]).v);});_tmpA75->tl=_tmp12CE;});_tmpA75;});_tmpA72(_tmpA73);});
goto _LL0;case 343U: _LL2A9: _LL2AA:
# 2768 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA76)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA77=({struct Cyc_Absyn_Stmt*(*_tmpA78)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_while_stmt;struct Cyc_Absyn_Exp*_tmpA79=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Stmt*_tmpA7A=({Cyc_yyget_Stmt_tok(&(yyyvsp[4]).v);});unsigned _tmpA7B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA78(_tmpA79,_tmpA7A,_tmpA7B);});_tmpA76(_tmpA77);});
goto _LL0;case 344U: _LL2AB: _LL2AC:
# 2772 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA7C)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA7D=({struct Cyc_Absyn_Stmt*(*_tmpA7E)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_do_stmt;struct Cyc_Absyn_Stmt*_tmpA7F=({Cyc_yyget_Stmt_tok(&(yyyvsp[1]).v);});struct Cyc_Absyn_Exp*_tmpA80=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});unsigned _tmpA81=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA7E(_tmpA7F,_tmpA80,_tmpA81);});_tmpA7C(_tmpA7D);});
goto _LL0;case 345U: _LL2AD: _LL2AE:
# 2776 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA82)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA83=({struct Cyc_Absyn_Stmt*(*_tmpA84)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpA85=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Exp*_tmpA86=({Cyc_Absyn_true_exp(0U);});struct Cyc_Absyn_Exp*_tmpA87=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Stmt*_tmpA88=({Cyc_yyget_Stmt_tok(&(yyyvsp[5]).v);});unsigned _tmpA89=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA84(_tmpA85,_tmpA86,_tmpA87,_tmpA88,_tmpA89);});_tmpA82(_tmpA83);});
goto _LL0;case 346U: _LL2AF: _LL2B0:
# 2779 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA8A)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA8B=({struct Cyc_Absyn_Stmt*(*_tmpA8C)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpA8D=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Exp*_tmpA8E=({Cyc_Absyn_true_exp(0U);});struct Cyc_Absyn_Exp*_tmpA8F=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});struct Cyc_Absyn_Stmt*_tmpA90=({Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);});unsigned _tmpA91=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA8C(_tmpA8D,_tmpA8E,_tmpA8F,_tmpA90,_tmpA91);});_tmpA8A(_tmpA8B);});
goto _LL0;case 347U: _LL2B1: _LL2B2:
# 2782 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA92)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA93=({struct Cyc_Absyn_Stmt*(*_tmpA94)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpA95=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Exp*_tmpA96=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});struct Cyc_Absyn_Exp*_tmpA97=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Stmt*_tmpA98=({Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);});unsigned _tmpA99=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA94(_tmpA95,_tmpA96,_tmpA97,_tmpA98,_tmpA99);});_tmpA92(_tmpA93);});
goto _LL0;case 348U: _LL2B3: _LL2B4:
# 2785 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpA9A)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpA9B=({struct Cyc_Absyn_Stmt*(*_tmpA9C)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpA9D=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Exp*_tmpA9E=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});struct Cyc_Absyn_Exp*_tmpA9F=({Cyc_yyget_Exp_tok(&(yyyvsp[5]).v);});struct Cyc_Absyn_Stmt*_tmpAA0=({Cyc_yyget_Stmt_tok(&(yyyvsp[7]).v);});unsigned _tmpAA1=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpA9C(_tmpA9D,_tmpA9E,_tmpA9F,_tmpAA0,_tmpAA1);});_tmpA9A(_tmpA9B);});
goto _LL0;case 349U: _LL2B5: _LL2B6:
# 2788 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAA2)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAA3=({struct Cyc_Absyn_Stmt*(*_tmpAA4)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpAA5=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Exp*_tmpAA6=({Cyc_Absyn_true_exp(0U);});struct Cyc_Absyn_Exp*_tmpAA7=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Stmt*_tmpAA8=({Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);});unsigned _tmpAA9=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpAA4(_tmpAA5,_tmpAA6,_tmpAA7,_tmpAA8,_tmpAA9);});_tmpAA2(_tmpAA3);});
goto _LL0;case 350U: _LL2B7: _LL2B8:
# 2791 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAAA)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAAB=({struct Cyc_Absyn_Stmt*(*_tmpAAC)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpAAD=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Exp*_tmpAAE=({Cyc_Absyn_true_exp(0U);});struct Cyc_Absyn_Exp*_tmpAAF=({Cyc_yyget_Exp_tok(&(yyyvsp[5]).v);});struct Cyc_Absyn_Stmt*_tmpAB0=({Cyc_yyget_Stmt_tok(&(yyyvsp[7]).v);});unsigned _tmpAB1=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpAAC(_tmpAAD,_tmpAAE,_tmpAAF,_tmpAB0,_tmpAB1);});_tmpAAA(_tmpAAB);});
goto _LL0;case 351U: _LL2B9: _LL2BA:
# 2794 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAB2)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAB3=({struct Cyc_Absyn_Stmt*(*_tmpAB4)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpAB5=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Exp*_tmpAB6=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});struct Cyc_Absyn_Exp*_tmpAB7=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Stmt*_tmpAB8=({Cyc_yyget_Stmt_tok(&(yyyvsp[7]).v);});unsigned _tmpAB9=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpAB4(_tmpAB5,_tmpAB6,_tmpAB7,_tmpAB8,_tmpAB9);});_tmpAB2(_tmpAB3);});
goto _LL0;case 352U: _LL2BB: _LL2BC:
# 2797 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpABA)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpABB=({struct Cyc_Absyn_Stmt*(*_tmpABC)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpABD=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Exp*_tmpABE=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});struct Cyc_Absyn_Exp*_tmpABF=({Cyc_yyget_Exp_tok(&(yyyvsp[6]).v);});struct Cyc_Absyn_Stmt*_tmpAC0=({Cyc_yyget_Stmt_tok(&(yyyvsp[8]).v);});unsigned _tmpAC1=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpABC(_tmpABD,_tmpABE,_tmpABF,_tmpAC0,_tmpAC1);});_tmpABA(_tmpABB);});
goto _LL0;case 353U: _LL2BD: _LL2BE: {
# 2800 "parse.y"
struct Cyc_List_List*decls=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});
struct Cyc_Absyn_Stmt*_tmpAC2=({struct Cyc_Absyn_Stmt*(*_tmpAC5)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpAC6=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Exp*_tmpAC7=({Cyc_Absyn_true_exp(0U);});struct Cyc_Absyn_Exp*_tmpAC8=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Stmt*_tmpAC9=({Cyc_yyget_Stmt_tok(&(yyyvsp[5]).v);});unsigned _tmpACA=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpAC5(_tmpAC6,_tmpAC7,_tmpAC8,_tmpAC9,_tmpACA);});
# 2801
struct Cyc_Absyn_Stmt*s=_tmpAC2;
# 2803
yyval=({union Cyc_YYSTYPE(*_tmpAC3)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAC4=({Cyc_Parse_flatten_declarations(decls,s);});_tmpAC3(_tmpAC4);});
# 2805
goto _LL0;}case 354U: _LL2BF: _LL2C0: {
# 2806 "parse.y"
struct Cyc_List_List*decls=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});
struct Cyc_Absyn_Stmt*_tmpACB=({struct Cyc_Absyn_Stmt*(*_tmpACE)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpACF=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Exp*_tmpAD0=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});struct Cyc_Absyn_Exp*_tmpAD1=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Stmt*_tmpAD2=({Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);});unsigned _tmpAD3=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpACE(_tmpACF,_tmpAD0,_tmpAD1,_tmpAD2,_tmpAD3);});
# 2807
struct Cyc_Absyn_Stmt*s=_tmpACB;
# 2809
yyval=({union Cyc_YYSTYPE(*_tmpACC)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpACD=({Cyc_Parse_flatten_declarations(decls,s);});_tmpACC(_tmpACD);});
# 2811
goto _LL0;}case 355U: _LL2C1: _LL2C2: {
# 2812 "parse.y"
struct Cyc_List_List*decls=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});
struct Cyc_Absyn_Stmt*_tmpAD4=({struct Cyc_Absyn_Stmt*(*_tmpAD7)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpAD8=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Exp*_tmpAD9=({Cyc_Absyn_true_exp(0U);});struct Cyc_Absyn_Exp*_tmpADA=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});struct Cyc_Absyn_Stmt*_tmpADB=({Cyc_yyget_Stmt_tok(&(yyyvsp[6]).v);});unsigned _tmpADC=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpAD7(_tmpAD8,_tmpAD9,_tmpADA,_tmpADB,_tmpADC);});
# 2813
struct Cyc_Absyn_Stmt*s=_tmpAD4;
# 2815
yyval=({union Cyc_YYSTYPE(*_tmpAD5)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAD6=({Cyc_Parse_flatten_declarations(decls,s);});_tmpAD5(_tmpAD6);});
# 2817
goto _LL0;}case 356U: _LL2C3: _LL2C4: {
# 2818 "parse.y"
struct Cyc_List_List*decls=({Cyc_yyget_YY16(&(yyyvsp[2]).v);});
struct Cyc_Absyn_Stmt*_tmpADD=({struct Cyc_Absyn_Stmt*(*_tmpAE0)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmpAE1=({Cyc_Absyn_false_exp(0U);});struct Cyc_Absyn_Exp*_tmpAE2=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});struct Cyc_Absyn_Exp*_tmpAE3=({Cyc_yyget_Exp_tok(&(yyyvsp[5]).v);});struct Cyc_Absyn_Stmt*_tmpAE4=({Cyc_yyget_Stmt_tok(&(yyyvsp[7]).v);});unsigned _tmpAE5=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpAE0(_tmpAE1,_tmpAE2,_tmpAE3,_tmpAE4,_tmpAE5);});
# 2819
struct Cyc_Absyn_Stmt*s=_tmpADD;
# 2821
yyval=({union Cyc_YYSTYPE(*_tmpADE)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpADF=({Cyc_Parse_flatten_declarations(decls,s);});_tmpADE(_tmpADF);});
# 2823
goto _LL0;}case 357U: _LL2C5: _LL2C6:
# 2828 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAE6)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAE7=({struct Cyc_Absyn_Stmt*(*_tmpAE8)(struct _fat_ptr*,unsigned)=Cyc_Absyn_goto_stmt;struct _fat_ptr*_tmpAE9=({struct _fat_ptr*_tmpAEA=_cycalloc(sizeof(*_tmpAEA));({struct _fat_ptr _tmp12D4=({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});*_tmpAEA=_tmp12D4;});_tmpAEA;});unsigned _tmpAEB=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpAE8(_tmpAE9,_tmpAEB);});_tmpAE6(_tmpAE7);});
goto _LL0;case 358U: _LL2C7: _LL2C8:
# 2829 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAEC)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAED=({Cyc_Absyn_continue_stmt((unsigned)((yyyvsp[0]).l).first_line);});_tmpAEC(_tmpAED);});
goto _LL0;case 359U: _LL2C9: _LL2CA:
# 2830 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAEE)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAEF=({Cyc_Absyn_break_stmt((unsigned)((yyyvsp[0]).l).first_line);});_tmpAEE(_tmpAEF);});
goto _LL0;case 360U: _LL2CB: _LL2CC:
# 2831 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAF0)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAF1=({Cyc_Absyn_return_stmt(0,(unsigned)((yyyvsp[0]).l).first_line);});_tmpAF0(_tmpAF1);});
goto _LL0;case 361U: _LL2CD: _LL2CE:
# 2832 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAF2)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAF3=({struct Cyc_Absyn_Stmt*(*_tmpAF4)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_return_stmt;struct Cyc_Absyn_Exp*_tmpAF5=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpAF6=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpAF4(_tmpAF5,_tmpAF6);});_tmpAF2(_tmpAF3);});
goto _LL0;case 362U: _LL2CF: _LL2D0:
# 2834 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAF7)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAF8=({Cyc_Absyn_fallthru_stmt(0,(unsigned)((yyyvsp[0]).l).first_line);});_tmpAF7(_tmpAF8);});
goto _LL0;case 363U: _LL2D1: _LL2D2:
# 2835 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAF9)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAFA=({Cyc_Absyn_fallthru_stmt(0,(unsigned)((yyyvsp[0]).l).first_line);});_tmpAF9(_tmpAFA);});
goto _LL0;case 364U: _LL2D3: _LL2D4:
# 2837 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpAFB)(struct Cyc_Absyn_Stmt*yy1)=Cyc_Stmt_tok;struct Cyc_Absyn_Stmt*_tmpAFC=({struct Cyc_Absyn_Stmt*(*_tmpAFD)(struct Cyc_List_List*,unsigned)=Cyc_Absyn_fallthru_stmt;struct Cyc_List_List*_tmpAFE=({Cyc_yyget_YY4(&(yyyvsp[2]).v);});unsigned _tmpAFF=(unsigned)((yyyvsp[0]).l).first_line;_tmpAFD(_tmpAFE,_tmpAFF);});_tmpAFB(_tmpAFC);});
goto _LL0;case 365U: _LL2D5: _LL2D6:
# 2846 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 366U: _LL2D7: _LL2D8:
# 2849
 yyval=(yyyvsp[0]).v;
goto _LL0;case 367U: _LL2D9: _LL2DA:
# 2851 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB00)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB01=({struct Cyc_Absyn_Pat*(*_tmpB02)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB03=({struct Cyc_Absyn_Exp*(*_tmpB04)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_conditional_exp;struct Cyc_Absyn_Exp*_tmpB05=({struct Cyc_Absyn_Exp*(*_tmpB06)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB07=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB06(_tmpB07);});struct Cyc_Absyn_Exp*_tmpB08=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Exp*_tmpB09=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});unsigned _tmpB0A=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB04(_tmpB05,_tmpB08,_tmpB09,_tmpB0A);});_tmpB02(_tmpB03);});_tmpB00(_tmpB01);});
goto _LL0;case 368U: _LL2DB: _LL2DC:
# 2854
 yyval=(yyyvsp[0]).v;
goto _LL0;case 369U: _LL2DD: _LL2DE:
# 2856 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB0B)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB0C=({struct Cyc_Absyn_Pat*(*_tmpB0D)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB0E=({struct Cyc_Absyn_Exp*(*_tmpB0F)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_or_exp;struct Cyc_Absyn_Exp*_tmpB10=({struct Cyc_Absyn_Exp*(*_tmpB11)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB12=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB11(_tmpB12);});struct Cyc_Absyn_Exp*_tmpB13=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB14=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB0F(_tmpB10,_tmpB13,_tmpB14);});_tmpB0D(_tmpB0E);});_tmpB0B(_tmpB0C);});
goto _LL0;case 370U: _LL2DF: _LL2E0:
# 2859
 yyval=(yyyvsp[0]).v;
goto _LL0;case 371U: _LL2E1: _LL2E2:
# 2861 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB15)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB16=({struct Cyc_Absyn_Pat*(*_tmpB17)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB18=({struct Cyc_Absyn_Exp*(*_tmpB19)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_and_exp;struct Cyc_Absyn_Exp*_tmpB1A=({struct Cyc_Absyn_Exp*(*_tmpB1B)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB1C=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB1B(_tmpB1C);});struct Cyc_Absyn_Exp*_tmpB1D=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB1E=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB19(_tmpB1A,_tmpB1D,_tmpB1E);});_tmpB17(_tmpB18);});_tmpB15(_tmpB16);});
goto _LL0;case 372U: _LL2E3: _LL2E4:
# 2864
 yyval=(yyyvsp[0]).v;
goto _LL0;case 373U: _LL2E5: _LL2E6:
# 2866 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB1F)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB20=({struct Cyc_Absyn_Pat*(*_tmpB21)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB22=({struct Cyc_Absyn_Exp*(*_tmpB23)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpB24=Cyc_Absyn_Bitor;struct Cyc_Absyn_Exp*_tmpB25=({struct Cyc_Absyn_Exp*(*_tmpB26)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB27=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB26(_tmpB27);});struct Cyc_Absyn_Exp*_tmpB28=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB29=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB23(_tmpB24,_tmpB25,_tmpB28,_tmpB29);});_tmpB21(_tmpB22);});_tmpB1F(_tmpB20);});
goto _LL0;case 374U: _LL2E7: _LL2E8:
# 2869
 yyval=(yyyvsp[0]).v;
goto _LL0;case 375U: _LL2E9: _LL2EA:
# 2871 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB2A)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB2B=({struct Cyc_Absyn_Pat*(*_tmpB2C)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB2D=({struct Cyc_Absyn_Exp*(*_tmpB2E)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpB2F=Cyc_Absyn_Bitxor;struct Cyc_Absyn_Exp*_tmpB30=({struct Cyc_Absyn_Exp*(*_tmpB31)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB32=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB31(_tmpB32);});struct Cyc_Absyn_Exp*_tmpB33=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB34=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB2E(_tmpB2F,_tmpB30,_tmpB33,_tmpB34);});_tmpB2C(_tmpB2D);});_tmpB2A(_tmpB2B);});
goto _LL0;case 376U: _LL2EB: _LL2EC:
# 2874
 yyval=(yyyvsp[0]).v;
goto _LL0;case 377U: _LL2ED: _LL2EE:
# 2876 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB35)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB36=({struct Cyc_Absyn_Pat*(*_tmpB37)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB38=({struct Cyc_Absyn_Exp*(*_tmpB39)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpB3A=Cyc_Absyn_Bitand;struct Cyc_Absyn_Exp*_tmpB3B=({struct Cyc_Absyn_Exp*(*_tmpB3C)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB3D=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB3C(_tmpB3D);});struct Cyc_Absyn_Exp*_tmpB3E=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB3F=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB39(_tmpB3A,_tmpB3B,_tmpB3E,_tmpB3F);});_tmpB37(_tmpB38);});_tmpB35(_tmpB36);});
goto _LL0;case 378U: _LL2EF: _LL2F0:
# 2879
 yyval=(yyyvsp[0]).v;
goto _LL0;case 379U: _LL2F1: _LL2F2:
# 2881 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB40)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB41=({struct Cyc_Absyn_Pat*(*_tmpB42)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB43=({struct Cyc_Absyn_Exp*(*_tmpB44)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmpB45=({struct Cyc_Absyn_Exp*(*_tmpB46)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB47=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB46(_tmpB47);});struct Cyc_Absyn_Exp*_tmpB48=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB49=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB44(_tmpB45,_tmpB48,_tmpB49);});_tmpB42(_tmpB43);});_tmpB40(_tmpB41);});
goto _LL0;case 380U: _LL2F3: _LL2F4:
# 2883 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB4A)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB4B=({struct Cyc_Absyn_Pat*(*_tmpB4C)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB4D=({struct Cyc_Absyn_Exp*(*_tmpB4E)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_neq_exp;struct Cyc_Absyn_Exp*_tmpB4F=({struct Cyc_Absyn_Exp*(*_tmpB50)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB51=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB50(_tmpB51);});struct Cyc_Absyn_Exp*_tmpB52=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB53=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB4E(_tmpB4F,_tmpB52,_tmpB53);});_tmpB4C(_tmpB4D);});_tmpB4A(_tmpB4B);});
goto _LL0;case 381U: _LL2F5: _LL2F6:
# 2886
 yyval=(yyyvsp[0]).v;
goto _LL0;case 382U: _LL2F7: _LL2F8:
# 2888 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB54)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB55=({struct Cyc_Absyn_Pat*(*_tmpB56)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB57=({struct Cyc_Absyn_Exp*(*_tmpB58)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_lt_exp;struct Cyc_Absyn_Exp*_tmpB59=({struct Cyc_Absyn_Exp*(*_tmpB5A)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB5B=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB5A(_tmpB5B);});struct Cyc_Absyn_Exp*_tmpB5C=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB5D=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB58(_tmpB59,_tmpB5C,_tmpB5D);});_tmpB56(_tmpB57);});_tmpB54(_tmpB55);});
goto _LL0;case 383U: _LL2F9: _LL2FA:
# 2890 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB5E)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB5F=({struct Cyc_Absyn_Pat*(*_tmpB60)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB61=({struct Cyc_Absyn_Exp*(*_tmpB62)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_gt_exp;struct Cyc_Absyn_Exp*_tmpB63=({struct Cyc_Absyn_Exp*(*_tmpB64)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB65=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB64(_tmpB65);});struct Cyc_Absyn_Exp*_tmpB66=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB67=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB62(_tmpB63,_tmpB66,_tmpB67);});_tmpB60(_tmpB61);});_tmpB5E(_tmpB5F);});
goto _LL0;case 384U: _LL2FB: _LL2FC:
# 2892 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB68)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB69=({struct Cyc_Absyn_Pat*(*_tmpB6A)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB6B=({struct Cyc_Absyn_Exp*(*_tmpB6C)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_lte_exp;struct Cyc_Absyn_Exp*_tmpB6D=({struct Cyc_Absyn_Exp*(*_tmpB6E)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB6F=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB6E(_tmpB6F);});struct Cyc_Absyn_Exp*_tmpB70=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB71=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB6C(_tmpB6D,_tmpB70,_tmpB71);});_tmpB6A(_tmpB6B);});_tmpB68(_tmpB69);});
goto _LL0;case 385U: _LL2FD: _LL2FE:
# 2894 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB72)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB73=({struct Cyc_Absyn_Pat*(*_tmpB74)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB75=({struct Cyc_Absyn_Exp*(*_tmpB76)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_gte_exp;struct Cyc_Absyn_Exp*_tmpB77=({struct Cyc_Absyn_Exp*(*_tmpB78)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB79=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB78(_tmpB79);});struct Cyc_Absyn_Exp*_tmpB7A=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB7B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB76(_tmpB77,_tmpB7A,_tmpB7B);});_tmpB74(_tmpB75);});_tmpB72(_tmpB73);});
goto _LL0;case 386U: _LL2FF: _LL300:
# 2897
 yyval=(yyyvsp[0]).v;
goto _LL0;case 387U: _LL301: _LL302:
# 2899 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB7C)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB7D=({struct Cyc_Absyn_Pat*(*_tmpB7E)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB7F=({struct Cyc_Absyn_Exp*(*_tmpB80)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpB81=Cyc_Absyn_Bitlshift;struct Cyc_Absyn_Exp*_tmpB82=({struct Cyc_Absyn_Exp*(*_tmpB83)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB84=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB83(_tmpB84);});struct Cyc_Absyn_Exp*_tmpB85=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB86=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB80(_tmpB81,_tmpB82,_tmpB85,_tmpB86);});_tmpB7E(_tmpB7F);});_tmpB7C(_tmpB7D);});
goto _LL0;case 388U: _LL303: _LL304:
# 2901 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB87)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB88=({struct Cyc_Absyn_Pat*(*_tmpB89)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB8A=({struct Cyc_Absyn_Exp*(*_tmpB8B)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpB8C=Cyc_Absyn_Bitlrshift;struct Cyc_Absyn_Exp*_tmpB8D=({struct Cyc_Absyn_Exp*(*_tmpB8E)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB8F=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB8E(_tmpB8F);});struct Cyc_Absyn_Exp*_tmpB90=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB91=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB8B(_tmpB8C,_tmpB8D,_tmpB90,_tmpB91);});_tmpB89(_tmpB8A);});_tmpB87(_tmpB88);});
goto _LL0;case 389U: _LL305: _LL306:
# 2904
 yyval=(yyyvsp[0]).v;
goto _LL0;case 390U: _LL307: _LL308:
# 2906 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB92)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB93=({struct Cyc_Absyn_Pat*(*_tmpB94)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpB95=({struct Cyc_Absyn_Exp*(*_tmpB96)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpB97=Cyc_Absyn_Plus;struct Cyc_Absyn_Exp*_tmpB98=({struct Cyc_Absyn_Exp*(*_tmpB99)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpB9A=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpB99(_tmpB9A);});struct Cyc_Absyn_Exp*_tmpB9B=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpB9C=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpB96(_tmpB97,_tmpB98,_tmpB9B,_tmpB9C);});_tmpB94(_tmpB95);});_tmpB92(_tmpB93);});
goto _LL0;case 391U: _LL309: _LL30A:
# 2908 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpB9D)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpB9E=({struct Cyc_Absyn_Pat*(*_tmpB9F)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBA0=({struct Cyc_Absyn_Exp*(*_tmpBA1)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpBA2=Cyc_Absyn_Minus;struct Cyc_Absyn_Exp*_tmpBA3=({struct Cyc_Absyn_Exp*(*_tmpBA4)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpBA5=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpBA4(_tmpBA5);});struct Cyc_Absyn_Exp*_tmpBA6=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpBA7=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpBA1(_tmpBA2,_tmpBA3,_tmpBA6,_tmpBA7);});_tmpB9F(_tmpBA0);});_tmpB9D(_tmpB9E);});
goto _LL0;case 392U: _LL30B: _LL30C:
# 2911
 yyval=(yyyvsp[0]).v;
goto _LL0;case 393U: _LL30D: _LL30E:
# 2913 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpBA8)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBA9=({struct Cyc_Absyn_Pat*(*_tmpBAA)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBAB=({struct Cyc_Absyn_Exp*(*_tmpBAC)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpBAD=Cyc_Absyn_Times;struct Cyc_Absyn_Exp*_tmpBAE=({struct Cyc_Absyn_Exp*(*_tmpBAF)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpBB0=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpBAF(_tmpBB0);});struct Cyc_Absyn_Exp*_tmpBB1=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpBB2=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpBAC(_tmpBAD,_tmpBAE,_tmpBB1,_tmpBB2);});_tmpBAA(_tmpBAB);});_tmpBA8(_tmpBA9);});
goto _LL0;case 394U: _LL30F: _LL310:
# 2915 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpBB3)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBB4=({struct Cyc_Absyn_Pat*(*_tmpBB5)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBB6=({struct Cyc_Absyn_Exp*(*_tmpBB7)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpBB8=Cyc_Absyn_Div;struct Cyc_Absyn_Exp*_tmpBB9=({struct Cyc_Absyn_Exp*(*_tmpBBA)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpBBB=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpBBA(_tmpBBB);});struct Cyc_Absyn_Exp*_tmpBBC=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpBBD=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpBB7(_tmpBB8,_tmpBB9,_tmpBBC,_tmpBBD);});_tmpBB5(_tmpBB6);});_tmpBB3(_tmpBB4);});
goto _LL0;case 395U: _LL311: _LL312:
# 2917 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpBBE)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBBF=({struct Cyc_Absyn_Pat*(*_tmpBC0)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBC1=({struct Cyc_Absyn_Exp*(*_tmpBC2)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpBC3=Cyc_Absyn_Mod;struct Cyc_Absyn_Exp*_tmpBC4=({struct Cyc_Absyn_Exp*(*_tmpBC5)(struct Cyc_Absyn_Pat*p)=Cyc_Parse_pat2exp;struct Cyc_Absyn_Pat*_tmpBC6=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpBC5(_tmpBC6);});struct Cyc_Absyn_Exp*_tmpBC7=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpBC8=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpBC2(_tmpBC3,_tmpBC4,_tmpBC7,_tmpBC8);});_tmpBC0(_tmpBC1);});_tmpBBE(_tmpBBF);});
goto _LL0;case 396U: _LL313: _LL314:
# 2920
 yyval=(yyyvsp[0]).v;
goto _LL0;case 397U: _LL315: _LL316: {
# 2922 "parse.y"
void*t=({void*(*_tmpBD3)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpBD4=({Cyc_yyget_YY37(&(yyyvsp[1]).v);});unsigned _tmpBD5=(unsigned)((yyyvsp[1]).l).first_line;_tmpBD3(_tmpBD4,_tmpBD5);});
yyval=({union Cyc_YYSTYPE(*_tmpBC9)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBCA=({struct Cyc_Absyn_Pat*(*_tmpBCB)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBCC=({struct Cyc_Absyn_Exp*(*_tmpBCD)(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned)=Cyc_Absyn_cast_exp;void*_tmpBCE=t;struct Cyc_Absyn_Exp*_tmpBCF=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});int _tmpBD0=1;enum Cyc_Absyn_Coercion _tmpBD1=Cyc_Absyn_Unknown_coercion;unsigned _tmpBD2=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpBCD(_tmpBCE,_tmpBCF,_tmpBD0,_tmpBD1,_tmpBD2);});_tmpBCB(_tmpBCC);});_tmpBC9(_tmpBCA);});
# 2925
goto _LL0;}case 398U: _LL317: _LL318:
# 2928 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 399U: _LL319: _LL31A:
# 2931
 yyval=({union Cyc_YYSTYPE(*_tmpBD6)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBD7=({struct Cyc_Absyn_Pat*(*_tmpBD8)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBD9=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});_tmpBD8(_tmpBD9);});_tmpBD6(_tmpBD7);});
goto _LL0;case 400U: _LL31B: _LL31C:
# 2933 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpBDA)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBDB=({struct Cyc_Absyn_Pat*(*_tmpBDC)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBDD=({struct Cyc_Absyn_Exp*(*_tmpBDE)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim1_exp;enum Cyc_Absyn_Primop _tmpBDF=({Cyc_yyget_YY6(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpBE0=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpBE1=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpBDE(_tmpBDF,_tmpBE0,_tmpBE1);});_tmpBDC(_tmpBDD);});_tmpBDA(_tmpBDB);});
goto _LL0;case 401U: _LL31D: _LL31E: {
# 2935 "parse.y"
void*t=({void*(*_tmpBE9)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpBEA=({Cyc_yyget_YY37(&(yyyvsp[2]).v);});unsigned _tmpBEB=(unsigned)((yyyvsp[2]).l).first_line;_tmpBE9(_tmpBEA,_tmpBEB);});
yyval=({union Cyc_YYSTYPE(*_tmpBE2)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBE3=({struct Cyc_Absyn_Pat*(*_tmpBE4)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBE5=({struct Cyc_Absyn_Exp*(*_tmpBE6)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmpBE7=t;unsigned _tmpBE8=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpBE6(_tmpBE7,_tmpBE8);});_tmpBE4(_tmpBE5);});_tmpBE2(_tmpBE3);});
# 2938
goto _LL0;}case 402U: _LL31F: _LL320:
# 2939 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpBEC)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBED=({struct Cyc_Absyn_Pat*(*_tmpBEE)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBEF=({struct Cyc_Absyn_Exp*(*_tmpBF0)(struct Cyc_Absyn_Exp*e,unsigned)=Cyc_Absyn_sizeofexp_exp;struct Cyc_Absyn_Exp*_tmpBF1=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpBF2=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpBF0(_tmpBF1,_tmpBF2);});_tmpBEE(_tmpBEF);});_tmpBEC(_tmpBED);});
goto _LL0;case 403U: _LL321: _LL322:
# 2941 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpBF3)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBF4=({struct Cyc_Absyn_Pat*(*_tmpBF5)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpBF6=({struct Cyc_Absyn_Exp*(*_tmpBF7)(void*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_offsetof_exp;void*_tmpBF8=(*({Cyc_yyget_YY37(&(yyyvsp[2]).v);})).f3;struct Cyc_List_List*_tmpBF9=({struct Cyc_List_List*(*_tmpBFA)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpBFB=({Cyc_yyget_YY3(&(yyyvsp[4]).v);});_tmpBFA(_tmpBFB);});unsigned _tmpBFC=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpBF7(_tmpBF8,_tmpBF9,_tmpBFC);});_tmpBF5(_tmpBF6);});_tmpBF3(_tmpBF4);});
goto _LL0;case 404U: _LL323: _LL324:
# 2946 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 405U: _LL325: _LL326:
# 2954 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 406U: _LL327: _LL328:
# 2959 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpBFD)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpBFE=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,(unsigned)((yyyvsp[0]).l).first_line);});_tmpBFD(_tmpBFE);});
goto _LL0;case 407U: _LL329: _LL32A:
# 2961 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpBFF)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC00=({struct Cyc_Absyn_Pat*(*_tmpC01)(struct Cyc_Absyn_Exp*)=Cyc_Absyn_exp_pat;struct Cyc_Absyn_Exp*_tmpC02=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});_tmpC01(_tmpC02);});_tmpBFF(_tmpC00);});
goto _LL0;case 408U: _LL32B: _LL32C: {
# 2963 "parse.y"
struct Cyc_Absyn_Exp*e=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});
{void*_stmttmp29=e->r;void*_tmpC03=_stmttmp29;int _tmpC05;struct _fat_ptr _tmpC04;int _tmpC07;enum Cyc_Absyn_Sign _tmpC06;short _tmpC09;enum Cyc_Absyn_Sign _tmpC08;char _tmpC0B;enum Cyc_Absyn_Sign _tmpC0A;if(((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->tag == 0U)switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->f1).LongLong_c).tag){case 2U: _LL4B6: _tmpC0A=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->f1).Char_c).val).f1;_tmpC0B=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->f1).Char_c).val).f2;_LL4B7: {enum Cyc_Absyn_Sign s=_tmpC0A;char i=_tmpC0B;
# 2967
yyval=({union Cyc_YYSTYPE(*_tmpC0C)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC0D=({({void*_tmp12D5=(void*)({struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*_tmpC0E=_cycalloc(sizeof(*_tmpC0E));_tmpC0E->tag=11U,_tmpC0E->f1=i;_tmpC0E;});Cyc_Absyn_new_pat(_tmp12D5,e->loc);});});_tmpC0C(_tmpC0D);});goto _LL4B5;}case 4U: _LL4B8: _tmpC08=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->f1).Short_c).val).f1;_tmpC09=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->f1).Short_c).val).f2;_LL4B9: {enum Cyc_Absyn_Sign s=_tmpC08;short i=_tmpC09;
# 2969
yyval=({union Cyc_YYSTYPE(*_tmpC0F)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC10=({({void*_tmp12D6=(void*)({struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*_tmpC11=_cycalloc(sizeof(*_tmpC11));_tmpC11->tag=10U,_tmpC11->f1=s,_tmpC11->f2=(int)i;_tmpC11;});Cyc_Absyn_new_pat(_tmp12D6,e->loc);});});_tmpC0F(_tmpC10);});goto _LL4B5;}case 5U: _LL4BA: _tmpC06=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->f1).Int_c).val).f1;_tmpC07=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->f1).Int_c).val).f2;_LL4BB: {enum Cyc_Absyn_Sign s=_tmpC06;int i=_tmpC07;
# 2971
yyval=({union Cyc_YYSTYPE(*_tmpC12)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC13=({({void*_tmp12D7=(void*)({struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*_tmpC14=_cycalloc(sizeof(*_tmpC14));_tmpC14->tag=10U,_tmpC14->f1=s,_tmpC14->f2=i;_tmpC14;});Cyc_Absyn_new_pat(_tmp12D7,e->loc);});});_tmpC12(_tmpC13);});goto _LL4B5;}case 7U: _LL4BC: _tmpC04=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->f1).Float_c).val).f1;_tmpC05=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpC03)->f1).Float_c).val).f2;_LL4BD: {struct _fat_ptr s=_tmpC04;int i=_tmpC05;
# 2973
yyval=({union Cyc_YYSTYPE(*_tmpC15)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC16=({({void*_tmp12D8=(void*)({struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*_tmpC17=_cycalloc(sizeof(*_tmpC17));_tmpC17->tag=12U,_tmpC17->f1=s,_tmpC17->f2=i;_tmpC17;});Cyc_Absyn_new_pat(_tmp12D8,e->loc);});});_tmpC15(_tmpC16);});goto _LL4B5;}case 1U: _LL4BE: _LL4BF:
# 2975
 yyval=({union Cyc_YYSTYPE(*_tmpC18)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC19=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Null_p_val,e->loc);});_tmpC18(_tmpC19);});goto _LL4B5;case 8U: _LL4C0: _LL4C1:
# 2977
({void*_tmpC1A=0U;({unsigned _tmp12DA=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp12D9=({const char*_tmpC1B="strings cannot occur within patterns";_tag_fat(_tmpC1B,sizeof(char),37U);});Cyc_Warn_err(_tmp12DA,_tmp12D9,_tag_fat(_tmpC1A,sizeof(void*),0U));});});goto _LL4B5;case 9U: _LL4C2: _LL4C3:
# 2979
({void*_tmpC1C=0U;({unsigned _tmp12DC=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp12DB=({const char*_tmpC1D="strings cannot occur within patterns";_tag_fat(_tmpC1D,sizeof(char),37U);});Cyc_Warn_err(_tmp12DC,_tmp12DB,_tag_fat(_tmpC1C,sizeof(void*),0U));});});goto _LL4B5;case 6U: _LL4C4: _LL4C5:
# 2981
({void*_tmpC1E=0U;({unsigned _tmp12DE=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp12DD=({const char*_tmpC1F="long long's in patterns not yet implemented";_tag_fat(_tmpC1F,sizeof(char),44U);});Cyc_Warn_err(_tmp12DE,_tmp12DD,_tag_fat(_tmpC1E,sizeof(void*),0U));});});goto _LL4B5;default: goto _LL4C6;}else{_LL4C6: _LL4C7:
# 2983
({void*_tmpC20=0U;({unsigned _tmp12E0=(unsigned)((yyyvsp[0]).l).first_line;struct _fat_ptr _tmp12DF=({const char*_tmpC21="bad constant in case";_tag_fat(_tmpC21,sizeof(char),21U);});Cyc_Warn_err(_tmp12E0,_tmp12DF,_tag_fat(_tmpC20,sizeof(void*),0U));});});}_LL4B5:;}
# 2986
goto _LL0;}case 409U: _LL32D: _LL32E:
# 2987 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpC22)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC23=({struct Cyc_Absyn_Pat*(*_tmpC24)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpC25=(void*)({struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct*_tmpC26=_cycalloc(sizeof(*_tmpC26));_tmpC26->tag=15U,({struct _tuple0*_tmp12E1=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmpC26->f1=_tmp12E1;});_tmpC26;});unsigned _tmpC27=(unsigned)((yyyvsp[0]).l).first_line;_tmpC24(_tmpC25,_tmpC27);});_tmpC22(_tmpC23);});
goto _LL0;case 410U: _LL32F: _LL330:
# 2989 "parse.y"
 if(({int(*_tmpC28)(struct _fat_ptr s1,struct _fat_ptr s2)=Cyc_strcmp;struct _fat_ptr _tmpC29=(struct _fat_ptr)({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});struct _fat_ptr _tmpC2A=({const char*_tmpC2B="as";_tag_fat(_tmpC2B,sizeof(char),3U);});_tmpC28(_tmpC29,_tmpC2A);})!= 0)
({void*_tmpC2C=0U;({unsigned _tmp12E3=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp12E2=({const char*_tmpC2D="expecting `as'";_tag_fat(_tmpC2D,sizeof(char),15U);});Cyc_Warn_err(_tmp12E3,_tmp12E2,_tag_fat(_tmpC2C,sizeof(void*),0U));});});
# 2989
yyval=({union Cyc_YYSTYPE(*_tmpC2E)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC2F=({struct Cyc_Absyn_Pat*(*_tmpC30)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpC31=(void*)({struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*_tmpC39=_cycalloc(sizeof(*_tmpC39));
# 2991
_tmpC39->tag=1U,({struct Cyc_Absyn_Vardecl*_tmp12E7=({struct Cyc_Absyn_Vardecl*(*_tmpC32)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpC33=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpC34=({struct _tuple0*_tmpC36=_cycalloc(sizeof(*_tmpC36));_tmpC36->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp12E6=({struct _fat_ptr*_tmpC35=_cycalloc(sizeof(*_tmpC35));({struct _fat_ptr _tmp12E5=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmpC35=_tmp12E5;});_tmpC35;});_tmpC36->f2=_tmp12E6;});_tmpC36;});void*_tmpC37=Cyc_Absyn_void_type;struct Cyc_Absyn_Exp*_tmpC38=0;_tmpC32(_tmpC33,_tmpC34,_tmpC37,_tmpC38);});_tmpC39->f1=_tmp12E7;}),({
struct Cyc_Absyn_Pat*_tmp12E4=({Cyc_yyget_YY9(&(yyyvsp[2]).v);});_tmpC39->f2=_tmp12E4;});_tmpC39;});unsigned _tmpC3A=(unsigned)((yyyvsp[0]).l).first_line;_tmpC30(_tmpC31,_tmpC3A);});_tmpC2E(_tmpC2F);});
# 2994
goto _LL0;case 411U: _LL331: _LL332:
# 2995 "parse.y"
 if(({int(*_tmpC3B)(struct _fat_ptr s1,struct _fat_ptr s2)=Cyc_strcmp;struct _fat_ptr _tmpC3C=(struct _fat_ptr)({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});struct _fat_ptr _tmpC3D=({const char*_tmpC3E="alias";_tag_fat(_tmpC3E,sizeof(char),6U);});_tmpC3B(_tmpC3C,_tmpC3D);})!= 0)
({void*_tmpC3F=0U;({unsigned _tmp12E9=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp12E8=({const char*_tmpC40="expecting `alias'";_tag_fat(_tmpC40,sizeof(char),18U);});Cyc_Warn_err(_tmp12E9,_tmp12E8,_tag_fat(_tmpC3F,sizeof(void*),0U));});});{
# 2995
int _tmpC41=({
# 2997
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});
# 2995
int location=_tmpC41;
# 2998
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmpC51=_cycalloc(sizeof(*_tmpC51));({struct _fat_ptr*_tmp12EC=({struct _fat_ptr*_tmpC4F=_cycalloc(sizeof(*_tmpC4F));({struct _fat_ptr _tmp12EB=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});*_tmpC4F=_tmp12EB;});_tmpC4F;});_tmpC51->name=_tmp12EC;}),_tmpC51->identity=- 1,({void*_tmp12EA=(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmpC50=_cycalloc(sizeof(*_tmpC50));_tmpC50->tag=0U,_tmpC50->f1=& Cyc_Tcutil_rk;_tmpC50;});_tmpC51->kind=_tmp12EA;});_tmpC51;});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmpC45)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpC46=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpC47=({struct _tuple0*_tmpC49=_cycalloc(sizeof(*_tmpC49));_tmpC49->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp12EE=({struct _fat_ptr*_tmpC48=_cycalloc(sizeof(*_tmpC48));({struct _fat_ptr _tmp12ED=({Cyc_yyget_String_tok(&(yyyvsp[5]).v);});*_tmpC48=_tmp12ED;});_tmpC48;});_tmpC49->f2=_tmp12EE;});_tmpC49;});void*_tmpC4A=({void*(*_tmpC4B)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpC4C=({Cyc_yyget_YY37(&(yyyvsp[4]).v);});unsigned _tmpC4D=(unsigned)((yyyvsp[4]).l).first_line;_tmpC4B(_tmpC4C,_tmpC4D);});struct Cyc_Absyn_Exp*_tmpC4E=0;_tmpC45(_tmpC46,_tmpC47,_tmpC4A,_tmpC4E);});
# 3001
yyval=({union Cyc_YYSTYPE(*_tmpC42)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC43=({({void*_tmp12EF=(void*)({struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*_tmpC44=_cycalloc(sizeof(*_tmpC44));_tmpC44->tag=2U,_tmpC44->f1=tv,_tmpC44->f2=vd;_tmpC44;});Cyc_Absyn_new_pat(_tmp12EF,(unsigned)location);});});_tmpC42(_tmpC43);});
# 3003
goto _LL0;}case 412U: _LL333: _LL334:
# 3004 "parse.y"
 if(({int(*_tmpC52)(struct _fat_ptr s1,struct _fat_ptr s2)=Cyc_strcmp;struct _fat_ptr _tmpC53=(struct _fat_ptr)({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});struct _fat_ptr _tmpC54=({const char*_tmpC55="alias";_tag_fat(_tmpC55,sizeof(char),6U);});_tmpC52(_tmpC53,_tmpC54);})!= 0)
({void*_tmpC56=0U;({unsigned _tmp12F1=(unsigned)((yyyvsp[1]).l).first_line;struct _fat_ptr _tmp12F0=({const char*_tmpC57="expecting `alias'";_tag_fat(_tmpC57,sizeof(char),18U);});Cyc_Warn_err(_tmp12F1,_tmp12F0,_tag_fat(_tmpC56,sizeof(void*),0U));});});{
# 3004
int _tmpC58=({
# 3006
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});
# 3004
int location=_tmpC58;
# 3007
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*_tmpC68=_cycalloc(sizeof(*_tmpC68));({struct _fat_ptr*_tmp12F4=({struct _fat_ptr*_tmpC66=_cycalloc(sizeof(*_tmpC66));({struct _fat_ptr _tmp12F3=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});*_tmpC66=_tmp12F3;});_tmpC66;});_tmpC68->name=_tmp12F4;}),_tmpC68->identity=- 1,({void*_tmp12F2=(void*)({struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*_tmpC67=_cycalloc(sizeof(*_tmpC67));_tmpC67->tag=0U,_tmpC67->f1=& Cyc_Tcutil_rk;_tmpC67;});_tmpC68->kind=_tmp12F2;});_tmpC68;});
struct Cyc_Absyn_Vardecl*vd=({struct Cyc_Absyn_Vardecl*(*_tmpC5C)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpC5D=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpC5E=({struct _tuple0*_tmpC60=_cycalloc(sizeof(*_tmpC60));_tmpC60->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp12F6=({struct _fat_ptr*_tmpC5F=_cycalloc(sizeof(*_tmpC5F));({struct _fat_ptr _tmp12F5=({Cyc_yyget_String_tok(&(yyyvsp[5]).v);});*_tmpC5F=_tmp12F5;});_tmpC5F;});_tmpC60->f2=_tmp12F6;});_tmpC60;});void*_tmpC61=({void*(*_tmpC62)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpC63=({Cyc_yyget_YY37(&(yyyvsp[4]).v);});unsigned _tmpC64=(unsigned)((yyyvsp[4]).l).first_line;_tmpC62(_tmpC63,_tmpC64);});struct Cyc_Absyn_Exp*_tmpC65=0;_tmpC5C(_tmpC5D,_tmpC5E,_tmpC61,_tmpC65);});
# 3010
yyval=({union Cyc_YYSTYPE(*_tmpC59)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC5A=({({void*_tmp12F7=(void*)({struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*_tmpC5B=_cycalloc(sizeof(*_tmpC5B));_tmpC5B->tag=2U,_tmpC5B->f1=tv,_tmpC5B->f2=vd;_tmpC5B;});Cyc_Absyn_new_pat(_tmp12F7,(unsigned)location);});});_tmpC59(_tmpC5A);});
# 3012
goto _LL0;}case 413U: _LL335: _LL336: {
# 3013 "parse.y"
struct _tuple23 _stmttmp2A=*({Cyc_yyget_YY10(&(yyyvsp[2]).v);});int _tmpC6A;struct Cyc_List_List*_tmpC69;_LL4C9: _tmpC69=_stmttmp2A.f1;_tmpC6A=_stmttmp2A.f2;_LL4CA: {struct Cyc_List_List*ps=_tmpC69;int dots=_tmpC6A;
yyval=({union Cyc_YYSTYPE(*_tmpC6B)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC6C=({struct Cyc_Absyn_Pat*(*_tmpC6D)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpC6E=(void*)({struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*_tmpC6F=_cycalloc(sizeof(*_tmpC6F));_tmpC6F->tag=5U,_tmpC6F->f1=ps,_tmpC6F->f2=dots;_tmpC6F;});unsigned _tmpC70=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpC6D(_tmpC6E,_tmpC70);});_tmpC6B(_tmpC6C);});
# 3016
goto _LL0;}}case 414U: _LL337: _LL338: {
# 3017 "parse.y"
struct _tuple23 _stmttmp2B=*({Cyc_yyget_YY10(&(yyyvsp[2]).v);});int _tmpC72;struct Cyc_List_List*_tmpC71;_LL4CC: _tmpC71=_stmttmp2B.f1;_tmpC72=_stmttmp2B.f2;_LL4CD: {struct Cyc_List_List*ps=_tmpC71;int dots=_tmpC72;
yyval=({union Cyc_YYSTYPE(*_tmpC73)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC74=({struct Cyc_Absyn_Pat*(*_tmpC75)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpC76=(void*)({struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*_tmpC77=_cycalloc(sizeof(*_tmpC77));_tmpC77->tag=16U,({struct _tuple0*_tmp12F8=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmpC77->f1=_tmp12F8;}),_tmpC77->f2=ps,_tmpC77->f3=dots;_tmpC77;});unsigned _tmpC78=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpC75(_tmpC76,_tmpC78);});_tmpC73(_tmpC74);});
# 3020
goto _LL0;}}case 415U: _LL339: _LL33A: {
# 3022 "parse.y"
struct _tuple23 _stmttmp2C=*({Cyc_yyget_YY14(&(yyyvsp[3]).v);});int _tmpC7A;struct Cyc_List_List*_tmpC79;_LL4CF: _tmpC79=_stmttmp2C.f1;_tmpC7A=_stmttmp2C.f2;_LL4D0: {struct Cyc_List_List*fps=_tmpC79;int dots=_tmpC7A;
struct Cyc_List_List*exist_ts=({struct Cyc_List_List*(*_tmpC86)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpC87)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmpC87;});struct Cyc_Absyn_Tvar*(*_tmpC88)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmpC89=(unsigned)((yyyvsp[2]).l).first_line;struct Cyc_List_List*_tmpC8A=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmpC86(_tmpC88,_tmpC89,_tmpC8A);});
yyval=({union Cyc_YYSTYPE(*_tmpC7B)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC7C=({struct Cyc_Absyn_Pat*(*_tmpC7D)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpC7E=(void*)({struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*_tmpC84=_cycalloc(sizeof(*_tmpC84));_tmpC84->tag=7U,({union Cyc_Absyn_AggrInfo*_tmp12FA=({union Cyc_Absyn_AggrInfo*_tmpC83=_cycalloc(sizeof(*_tmpC83));({union Cyc_Absyn_AggrInfo _tmp12F9=({union Cyc_Absyn_AggrInfo(*_tmpC7F)(enum Cyc_Absyn_AggrKind,struct _tuple0*,struct Cyc_Core_Opt*)=Cyc_Absyn_UnknownAggr;enum Cyc_Absyn_AggrKind _tmpC80=Cyc_Absyn_StructA;struct _tuple0*_tmpC81=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});struct Cyc_Core_Opt*_tmpC82=0;_tmpC7F(_tmpC80,_tmpC81,_tmpC82);});*_tmpC83=_tmp12F9;});_tmpC83;});_tmpC84->f1=_tmp12FA;}),_tmpC84->f2=exist_ts,_tmpC84->f3=fps,_tmpC84->f4=dots;_tmpC84;});unsigned _tmpC85=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpC7D(_tmpC7E,_tmpC85);});_tmpC7B(_tmpC7C);});
# 3027
goto _LL0;}}case 416U: _LL33B: _LL33C: {
# 3029 "parse.y"
struct _tuple23 _stmttmp2D=*({Cyc_yyget_YY14(&(yyyvsp[2]).v);});int _tmpC8C;struct Cyc_List_List*_tmpC8B;_LL4D2: _tmpC8B=_stmttmp2D.f1;_tmpC8C=_stmttmp2D.f2;_LL4D3: {struct Cyc_List_List*fps=_tmpC8B;int dots=_tmpC8C;
struct Cyc_List_List*exist_ts=({struct Cyc_List_List*(*_tmpC93)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmpC94)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct Cyc_Absyn_Tvar*(*f)(unsigned,void*),unsigned env,struct Cyc_List_List*x))Cyc_List_map_c;_tmpC94;});struct Cyc_Absyn_Tvar*(*_tmpC95)(unsigned loc,void*t)=Cyc_Parse_typ2tvar;unsigned _tmpC96=(unsigned)((yyyvsp[1]).l).first_line;struct Cyc_List_List*_tmpC97=({Cyc_yyget_YY40(&(yyyvsp[1]).v);});_tmpC93(_tmpC95,_tmpC96,_tmpC97);});
yyval=({union Cyc_YYSTYPE(*_tmpC8D)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC8E=({struct Cyc_Absyn_Pat*(*_tmpC8F)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpC90=(void*)({struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*_tmpC91=_cycalloc(sizeof(*_tmpC91));_tmpC91->tag=7U,_tmpC91->f1=0,_tmpC91->f2=exist_ts,_tmpC91->f3=fps,_tmpC91->f4=dots;_tmpC91;});unsigned _tmpC92=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpC8F(_tmpC90,_tmpC92);});_tmpC8D(_tmpC8E);});
# 3033
goto _LL0;}}case 417U: _LL33D: _LL33E:
# 3034 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpC98)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC99=({struct Cyc_Absyn_Pat*(*_tmpC9A)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpC9B=(void*)({struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*_tmpC9C=_cycalloc(sizeof(*_tmpC9C));_tmpC9C->tag=6U,({struct Cyc_Absyn_Pat*_tmp12FB=({Cyc_yyget_YY9(&(yyyvsp[1]).v);});_tmpC9C->f1=_tmp12FB;});_tmpC9C;});unsigned _tmpC9D=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpC9A(_tmpC9B,_tmpC9D);});_tmpC98(_tmpC99);});
goto _LL0;case 418U: _LL33F: _LL340:
# 3036 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpC9E)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpC9F=({struct Cyc_Absyn_Pat*(*_tmpCA0)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpCA1=(void*)({struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*_tmpCA6=_cycalloc(sizeof(*_tmpCA6));_tmpCA6->tag=6U,({struct Cyc_Absyn_Pat*_tmp12FD=({struct Cyc_Absyn_Pat*(*_tmpCA2)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpCA3=(void*)({struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*_tmpCA4=_cycalloc(sizeof(*_tmpCA4));_tmpCA4->tag=6U,({struct Cyc_Absyn_Pat*_tmp12FC=({Cyc_yyget_YY9(&(yyyvsp[1]).v);});_tmpCA4->f1=_tmp12FC;});_tmpCA4;});unsigned _tmpCA5=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpCA2(_tmpCA3,_tmpCA5);});_tmpCA6->f1=_tmp12FD;});_tmpCA6;});unsigned _tmpCA7=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpCA0(_tmpCA1,_tmpCA7);});_tmpC9E(_tmpC9F);});
goto _LL0;case 419U: _LL341: _LL342:
# 3038 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpCA8)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpCA9=({struct Cyc_Absyn_Pat*(*_tmpCAA)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpCAB=(void*)({struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*_tmpCB3=_cycalloc(sizeof(*_tmpCB3));_tmpCB3->tag=3U,({struct Cyc_Absyn_Vardecl*_tmp1301=({struct Cyc_Absyn_Vardecl*(*_tmpCAC)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpCAD=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpCAE=({struct _tuple0*_tmpCB0=_cycalloc(sizeof(*_tmpCB0));_tmpCB0->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp1300=({struct _fat_ptr*_tmpCAF=_cycalloc(sizeof(*_tmpCAF));({struct _fat_ptr _tmp12FF=({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});*_tmpCAF=_tmp12FF;});_tmpCAF;});_tmpCB0->f2=_tmp1300;});_tmpCB0;});void*_tmpCB1=Cyc_Absyn_void_type;struct Cyc_Absyn_Exp*_tmpCB2=0;_tmpCAC(_tmpCAD,_tmpCAE,_tmpCB1,_tmpCB2);});_tmpCB3->f1=_tmp1301;}),({
# 3040
struct Cyc_Absyn_Pat*_tmp12FE=({Cyc_Absyn_new_pat((void*)& Cyc_Absyn_Wild_p_val,(unsigned)((yyyvsp[1]).l).first_line);});_tmpCB3->f2=_tmp12FE;});_tmpCB3;});unsigned _tmpCB4=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpCAA(_tmpCAB,_tmpCB4);});_tmpCA8(_tmpCA9);});
goto _LL0;case 420U: _LL343: _LL344:
# 3043 "parse.y"
 if(({int(*_tmpCB5)(struct _fat_ptr s1,struct _fat_ptr s2)=Cyc_strcmp;struct _fat_ptr _tmpCB6=(struct _fat_ptr)({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});struct _fat_ptr _tmpCB7=({const char*_tmpCB8="as";_tag_fat(_tmpCB8,sizeof(char),3U);});_tmpCB5(_tmpCB6,_tmpCB7);})!= 0)
({void*_tmpCB9=0U;({unsigned _tmp1303=(unsigned)((yyyvsp[2]).l).first_line;struct _fat_ptr _tmp1302=({const char*_tmpCBA="expecting `as'";_tag_fat(_tmpCBA,sizeof(char),15U);});Cyc_Warn_err(_tmp1303,_tmp1302,_tag_fat(_tmpCB9,sizeof(void*),0U));});});
# 3043
yyval=({union Cyc_YYSTYPE(*_tmpCBB)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpCBC=({struct Cyc_Absyn_Pat*(*_tmpCBD)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpCBE=(void*)({struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*_tmpCC6=_cycalloc(sizeof(*_tmpCC6));
# 3045
_tmpCC6->tag=3U,({struct Cyc_Absyn_Vardecl*_tmp1307=({struct Cyc_Absyn_Vardecl*(*_tmpCBF)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpCC0=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpCC1=({struct _tuple0*_tmpCC3=_cycalloc(sizeof(*_tmpCC3));_tmpCC3->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp1306=({struct _fat_ptr*_tmpCC2=_cycalloc(sizeof(*_tmpCC2));({struct _fat_ptr _tmp1305=({Cyc_yyget_String_tok(&(yyyvsp[1]).v);});*_tmpCC2=_tmp1305;});_tmpCC2;});_tmpCC3->f2=_tmp1306;});_tmpCC3;});void*_tmpCC4=Cyc_Absyn_void_type;struct Cyc_Absyn_Exp*_tmpCC5=0;_tmpCBF(_tmpCC0,_tmpCC1,_tmpCC4,_tmpCC5);});_tmpCC6->f1=_tmp1307;}),({
# 3047
struct Cyc_Absyn_Pat*_tmp1304=({Cyc_yyget_YY9(&(yyyvsp[3]).v);});_tmpCC6->f2=_tmp1304;});_tmpCC6;});unsigned _tmpCC7=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpCBD(_tmpCBE,_tmpCC7);});_tmpCBB(_tmpCBC);});
# 3049
goto _LL0;case 421U: _LL345: _LL346: {
# 3050 "parse.y"
void*tag=({void*(*_tmpCD5)(struct _fat_ptr s,void*k)=Cyc_Parse_id2type;struct _fat_ptr _tmpCD6=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});void*_tmpCD7=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_ik);});_tmpCD5(_tmpCD6,_tmpCD7);});
yyval=({union Cyc_YYSTYPE(*_tmpCC8)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpCC9=({struct Cyc_Absyn_Pat*(*_tmpCCA)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpCCB=(void*)({struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*_tmpCD3=_cycalloc(sizeof(*_tmpCD3));_tmpCD3->tag=4U,({struct Cyc_Absyn_Tvar*_tmp130B=({Cyc_Parse_typ2tvar((unsigned)((yyyvsp[2]).l).first_line,tag);});_tmpCD3->f1=_tmp130B;}),({
struct Cyc_Absyn_Vardecl*_tmp130A=({struct Cyc_Absyn_Vardecl*(*_tmpCCC)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpCCD=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpCCE=({struct _tuple0*_tmpCD0=_cycalloc(sizeof(*_tmpCD0));_tmpCD0->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp1309=({struct _fat_ptr*_tmpCCF=_cycalloc(sizeof(*_tmpCCF));({struct _fat_ptr _tmp1308=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmpCCF=_tmp1308;});_tmpCCF;});_tmpCD0->f2=_tmp1309;});_tmpCD0;});void*_tmpCD1=({Cyc_Absyn_tag_type(tag);});struct Cyc_Absyn_Exp*_tmpCD2=0;_tmpCCC(_tmpCCD,_tmpCCE,_tmpCD1,_tmpCD2);});_tmpCD3->f2=_tmp130A;});_tmpCD3;});unsigned _tmpCD4=(unsigned)({
# 3054
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpCCA(_tmpCCB,_tmpCD4);});_tmpCC8(_tmpCC9);});
goto _LL0;}case 422U: _LL347: _LL348: {
# 3056 "parse.y"
struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*(*_tmpCE7)(void*)=Cyc_Tcutil_new_tvar;void*_tmpCE8=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_ik);});_tmpCE7(_tmpCE8);});
yyval=({union Cyc_YYSTYPE(*_tmpCD8)(struct Cyc_Absyn_Pat*yy1)=Cyc_YY9;struct Cyc_Absyn_Pat*_tmpCD9=({struct Cyc_Absyn_Pat*(*_tmpCDA)(void*,unsigned)=Cyc_Absyn_new_pat;void*_tmpCDB=(void*)({struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*_tmpCE5=_cycalloc(sizeof(*_tmpCE5));_tmpCE5->tag=4U,_tmpCE5->f1=tv,({
struct Cyc_Absyn_Vardecl*_tmp130E=({struct Cyc_Absyn_Vardecl*(*_tmpCDC)(unsigned varloc,struct _tuple0*,void*,struct Cyc_Absyn_Exp*init)=Cyc_Absyn_new_vardecl;unsigned _tmpCDD=(unsigned)((yyyvsp[0]).l).first_line;struct _tuple0*_tmpCDE=({struct _tuple0*_tmpCE0=_cycalloc(sizeof(*_tmpCE0));_tmpCE0->f1=Cyc_Absyn_Loc_n,({struct _fat_ptr*_tmp130D=({struct _fat_ptr*_tmpCDF=_cycalloc(sizeof(*_tmpCDF));({struct _fat_ptr _tmp130C=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmpCDF=_tmp130C;});_tmpCDF;});_tmpCE0->f2=_tmp130D;});_tmpCE0;});void*_tmpCE1=({void*(*_tmpCE2)(void*)=Cyc_Absyn_tag_type;void*_tmpCE3=({Cyc_Absyn_var_type(tv);});_tmpCE2(_tmpCE3);});struct Cyc_Absyn_Exp*_tmpCE4=0;_tmpCDC(_tmpCDD,_tmpCDE,_tmpCE1,_tmpCE4);});_tmpCE5->f2=_tmp130E;});_tmpCE5;});unsigned _tmpCE6=(unsigned)({
# 3060
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpCDA(_tmpCDB,_tmpCE6);});_tmpCD8(_tmpCD9);});
goto _LL0;}case 423U: _LL349: _LL34A:
# 3064 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpCE9)(struct _tuple23*yy1)=Cyc_YY10;struct _tuple23*_tmpCEA=({struct _tuple23*_tmpCED=_cycalloc(sizeof(*_tmpCED));({struct Cyc_List_List*_tmp130F=({struct Cyc_List_List*(*_tmpCEB)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmpCEC=({Cyc_yyget_YY11(&(yyyvsp[0]).v);});_tmpCEB(_tmpCEC);});_tmpCED->f1=_tmp130F;}),_tmpCED->f2=0;_tmpCED;});_tmpCE9(_tmpCEA);});
goto _LL0;case 424U: _LL34B: _LL34C:
# 3065 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpCEE)(struct _tuple23*yy1)=Cyc_YY10;struct _tuple23*_tmpCEF=({struct _tuple23*_tmpCF2=_cycalloc(sizeof(*_tmpCF2));({struct Cyc_List_List*_tmp1310=({struct Cyc_List_List*(*_tmpCF0)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmpCF1=({Cyc_yyget_YY11(&(yyyvsp[0]).v);});_tmpCF0(_tmpCF1);});_tmpCF2->f1=_tmp1310;}),_tmpCF2->f2=1;_tmpCF2;});_tmpCEE(_tmpCEF);});
goto _LL0;case 425U: _LL34D: _LL34E:
# 3066 "parse.y"
 yyval=({Cyc_YY10(({struct _tuple23*_tmpCF3=_cycalloc(sizeof(*_tmpCF3));_tmpCF3->f1=0,_tmpCF3->f2=1;_tmpCF3;}));});
goto _LL0;case 426U: _LL34F: _LL350:
# 3071 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpCF4)(struct Cyc_List_List*yy1)=Cyc_YY11;struct Cyc_List_List*_tmpCF5=({struct Cyc_List_List*_tmpCF6=_cycalloc(sizeof(*_tmpCF6));({struct Cyc_Absyn_Pat*_tmp1311=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpCF6->hd=_tmp1311;}),_tmpCF6->tl=0;_tmpCF6;});_tmpCF4(_tmpCF5);});
goto _LL0;case 427U: _LL351: _LL352:
# 3073 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpCF7)(struct Cyc_List_List*yy1)=Cyc_YY11;struct Cyc_List_List*_tmpCF8=({struct Cyc_List_List*_tmpCF9=_cycalloc(sizeof(*_tmpCF9));({struct Cyc_Absyn_Pat*_tmp1313=({Cyc_yyget_YY9(&(yyyvsp[2]).v);});_tmpCF9->hd=_tmp1313;}),({struct Cyc_List_List*_tmp1312=({Cyc_yyget_YY11(&(yyyvsp[0]).v);});_tmpCF9->tl=_tmp1312;});_tmpCF9;});_tmpCF7(_tmpCF8);});
goto _LL0;case 428U: _LL353: _LL354:
# 3078 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpCFA)(struct _tuple24*yy1)=Cyc_YY12;struct _tuple24*_tmpCFB=({struct _tuple24*_tmpCFC=_cycalloc(sizeof(*_tmpCFC));_tmpCFC->f1=0,({struct Cyc_Absyn_Pat*_tmp1314=({Cyc_yyget_YY9(&(yyyvsp[0]).v);});_tmpCFC->f2=_tmp1314;});_tmpCFC;});_tmpCFA(_tmpCFB);});
goto _LL0;case 429U: _LL355: _LL356:
# 3080 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpCFD)(struct _tuple24*yy1)=Cyc_YY12;struct _tuple24*_tmpCFE=({struct _tuple24*_tmpCFF=_cycalloc(sizeof(*_tmpCFF));({struct Cyc_List_List*_tmp1316=({Cyc_yyget_YY41(&(yyyvsp[0]).v);});_tmpCFF->f1=_tmp1316;}),({struct Cyc_Absyn_Pat*_tmp1315=({Cyc_yyget_YY9(&(yyyvsp[1]).v);});_tmpCFF->f2=_tmp1315;});_tmpCFF;});_tmpCFD(_tmpCFE);});
goto _LL0;case 430U: _LL357: _LL358:
# 3083
 yyval=({union Cyc_YYSTYPE(*_tmpD00)(struct _tuple23*yy1)=Cyc_YY14;struct _tuple23*_tmpD01=({struct _tuple23*_tmpD04=_cycalloc(sizeof(*_tmpD04));({struct Cyc_List_List*_tmp1317=({struct Cyc_List_List*(*_tmpD02)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmpD03=({Cyc_yyget_YY13(&(yyyvsp[0]).v);});_tmpD02(_tmpD03);});_tmpD04->f1=_tmp1317;}),_tmpD04->f2=0;_tmpD04;});_tmpD00(_tmpD01);});
goto _LL0;case 431U: _LL359: _LL35A:
# 3084 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD05)(struct _tuple23*yy1)=Cyc_YY14;struct _tuple23*_tmpD06=({struct _tuple23*_tmpD09=_cycalloc(sizeof(*_tmpD09));({struct Cyc_List_List*_tmp1318=({struct Cyc_List_List*(*_tmpD07)(struct Cyc_List_List*x)=Cyc_List_rev;struct Cyc_List_List*_tmpD08=({Cyc_yyget_YY13(&(yyyvsp[0]).v);});_tmpD07(_tmpD08);});_tmpD09->f1=_tmp1318;}),_tmpD09->f2=1;_tmpD09;});_tmpD05(_tmpD06);});
goto _LL0;case 432U: _LL35B: _LL35C:
# 3085 "parse.y"
 yyval=({Cyc_YY14(({struct _tuple23*_tmpD0A=_cycalloc(sizeof(*_tmpD0A));_tmpD0A->f1=0,_tmpD0A->f2=1;_tmpD0A;}));});
goto _LL0;case 433U: _LL35D: _LL35E:
# 3090 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD0B)(struct Cyc_List_List*yy1)=Cyc_YY13;struct Cyc_List_List*_tmpD0C=({struct Cyc_List_List*_tmpD0D=_cycalloc(sizeof(*_tmpD0D));({struct _tuple24*_tmp1319=({Cyc_yyget_YY12(&(yyyvsp[0]).v);});_tmpD0D->hd=_tmp1319;}),_tmpD0D->tl=0;_tmpD0D;});_tmpD0B(_tmpD0C);});
goto _LL0;case 434U: _LL35F: _LL360:
# 3092 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD0E)(struct Cyc_List_List*yy1)=Cyc_YY13;struct Cyc_List_List*_tmpD0F=({struct Cyc_List_List*_tmpD10=_cycalloc(sizeof(*_tmpD10));({struct _tuple24*_tmp131B=({Cyc_yyget_YY12(&(yyyvsp[2]).v);});_tmpD10->hd=_tmp131B;}),({struct Cyc_List_List*_tmp131A=({Cyc_yyget_YY13(&(yyyvsp[0]).v);});_tmpD10->tl=_tmp131A;});_tmpD10;});_tmpD0E(_tmpD0F);});
goto _LL0;case 435U: _LL361: _LL362:
# 3098 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 436U: _LL363: _LL364:
# 3100 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD11)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD12=({struct Cyc_Absyn_Exp*(*_tmpD13)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_seq_exp;struct Cyc_Absyn_Exp*_tmpD14=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD15=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD16=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD13(_tmpD14,_tmpD15,_tmpD16);});_tmpD11(_tmpD12);});
goto _LL0;case 437U: _LL365: _LL366:
# 3105 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 438U: _LL367: _LL368:
# 3107 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD17)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD18=({struct Cyc_Absyn_Exp*(*_tmpD19)(struct Cyc_Absyn_Exp*,struct Cyc_Core_Opt*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assignop_exp;struct Cyc_Absyn_Exp*_tmpD1A=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Core_Opt*_tmpD1B=({Cyc_yyget_YY7(&(yyyvsp[1]).v);});struct Cyc_Absyn_Exp*_tmpD1C=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD1D=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD19(_tmpD1A,_tmpD1B,_tmpD1C,_tmpD1D);});_tmpD17(_tmpD18);});
goto _LL0;case 439U: _LL369: _LL36A:
# 3109 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD1E)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD1F=({struct Cyc_Absyn_Exp*(*_tmpD20)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_swap_exp;struct Cyc_Absyn_Exp*_tmpD21=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD22=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD23=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD20(_tmpD21,_tmpD22,_tmpD23);});_tmpD1E(_tmpD1F);});
goto _LL0;case 440U: _LL36B: _LL36C:
# 3113 "parse.y"
 yyval=({Cyc_YY7(0);});
goto _LL0;case 441U: _LL36D: _LL36E:
# 3114 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD24=_cycalloc(sizeof(*_tmpD24));_tmpD24->v=(void*)Cyc_Absyn_Times;_tmpD24;}));});
goto _LL0;case 442U: _LL36F: _LL370:
# 3115 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD25=_cycalloc(sizeof(*_tmpD25));_tmpD25->v=(void*)Cyc_Absyn_Div;_tmpD25;}));});
goto _LL0;case 443U: _LL371: _LL372:
# 3116 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD26=_cycalloc(sizeof(*_tmpD26));_tmpD26->v=(void*)Cyc_Absyn_Mod;_tmpD26;}));});
goto _LL0;case 444U: _LL373: _LL374:
# 3117 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD27=_cycalloc(sizeof(*_tmpD27));_tmpD27->v=(void*)Cyc_Absyn_Plus;_tmpD27;}));});
goto _LL0;case 445U: _LL375: _LL376:
# 3118 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD28=_cycalloc(sizeof(*_tmpD28));_tmpD28->v=(void*)Cyc_Absyn_Minus;_tmpD28;}));});
goto _LL0;case 446U: _LL377: _LL378:
# 3119 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD29=_cycalloc(sizeof(*_tmpD29));_tmpD29->v=(void*)Cyc_Absyn_Bitlshift;_tmpD29;}));});
goto _LL0;case 447U: _LL379: _LL37A:
# 3120 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD2A=_cycalloc(sizeof(*_tmpD2A));_tmpD2A->v=(void*)Cyc_Absyn_Bitlrshift;_tmpD2A;}));});
goto _LL0;case 448U: _LL37B: _LL37C:
# 3121 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD2B=_cycalloc(sizeof(*_tmpD2B));_tmpD2B->v=(void*)Cyc_Absyn_Bitand;_tmpD2B;}));});
goto _LL0;case 449U: _LL37D: _LL37E:
# 3122 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD2C=_cycalloc(sizeof(*_tmpD2C));_tmpD2C->v=(void*)Cyc_Absyn_Bitxor;_tmpD2C;}));});
goto _LL0;case 450U: _LL37F: _LL380:
# 3123 "parse.y"
 yyval=({Cyc_YY7(({struct Cyc_Core_Opt*_tmpD2D=_cycalloc(sizeof(*_tmpD2D));_tmpD2D->v=(void*)Cyc_Absyn_Bitor;_tmpD2D;}));});
goto _LL0;case 451U: _LL381: _LL382:
# 3128 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 452U: _LL383: _LL384:
# 3130 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD2E)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD2F=({struct Cyc_Absyn_Exp*(*_tmpD30)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_conditional_exp;struct Cyc_Absyn_Exp*_tmpD31=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD32=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Exp*_tmpD33=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});unsigned _tmpD34=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD30(_tmpD31,_tmpD32,_tmpD33,_tmpD34);});_tmpD2E(_tmpD2F);});
goto _LL0;case 453U: _LL385: _LL386:
# 3133
 yyval=({union Cyc_YYSTYPE(*_tmpD35)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD36=({struct Cyc_Absyn_Exp*(*_tmpD37)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_throw_exp;struct Cyc_Absyn_Exp*_tmpD38=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpD39=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD37(_tmpD38,_tmpD39);});_tmpD35(_tmpD36);});
goto _LL0;case 454U: _LL387: _LL388:
# 3136
 yyval=({union Cyc_YYSTYPE(*_tmpD3A)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD3B=({struct Cyc_Absyn_Exp*(*_tmpD3C)(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_New_exp;struct Cyc_Absyn_Exp*_tmpD3D=0;struct Cyc_Absyn_Exp*_tmpD3E=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpD3F=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD3C(_tmpD3D,_tmpD3E,_tmpD3F);});_tmpD3A(_tmpD3B);});
goto _LL0;case 455U: _LL389: _LL38A:
# 3138 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD40)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD41=({struct Cyc_Absyn_Exp*(*_tmpD42)(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_New_exp;struct Cyc_Absyn_Exp*_tmpD43=0;struct Cyc_Absyn_Exp*_tmpD44=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpD45=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD42(_tmpD43,_tmpD44,_tmpD45);});_tmpD40(_tmpD41);});
goto _LL0;case 456U: _LL38B: _LL38C:
# 3140 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD46)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD47=({struct Cyc_Absyn_Exp*(*_tmpD48)(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_New_exp;struct Cyc_Absyn_Exp*_tmpD49=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Exp*_tmpD4A=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});unsigned _tmpD4B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD48(_tmpD49,_tmpD4A,_tmpD4B);});_tmpD46(_tmpD47);});
goto _LL0;case 457U: _LL38D: _LL38E:
# 3142 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD4C)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD4D=({struct Cyc_Absyn_Exp*(*_tmpD4E)(struct Cyc_Absyn_Exp*rgn_handle,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_New_exp;struct Cyc_Absyn_Exp*_tmpD4F=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});struct Cyc_Absyn_Exp*_tmpD50=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});unsigned _tmpD51=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD4E(_tmpD4F,_tmpD50,_tmpD51);});_tmpD4C(_tmpD4D);});
goto _LL0;case 458U: _LL38F: _LL390:
# 3144 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD52)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD53=({struct Cyc_Absyn_Exp*(*_tmpD54)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpD55=(void*)({struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*_tmpD57=_cycalloc(sizeof(*_tmpD57));_tmpD57->tag=34U,({struct Cyc_Absyn_Exp*_tmp131E=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmpD57->f1=_tmp131E;}),({struct Cyc_Absyn_Exp*_tmp131D=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});_tmpD57->f2=_tmp131D;}),({struct _tuple8*_tmp131C=({struct _tuple8*_tmpD56=_cycalloc(sizeof(*_tmpD56));_tmpD56->f1=0,_tmpD56->f2=0;_tmpD56;});_tmpD57->f3=_tmp131C;});_tmpD57;});unsigned _tmpD58=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD54(_tmpD55,_tmpD58);});_tmpD52(_tmpD53);});
goto _LL0;case 459U: _LL391: _LL392:
# 3148 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 460U: _LL393: _LL394:
# 3152 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 461U: _LL395: _LL396:
# 3154 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD59)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD5A=({struct Cyc_Absyn_Exp*(*_tmpD5B)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_or_exp;struct Cyc_Absyn_Exp*_tmpD5C=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD5D=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD5E=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD5B(_tmpD5C,_tmpD5D,_tmpD5E);});_tmpD59(_tmpD5A);});
goto _LL0;case 462U: _LL397: _LL398:
# 3158 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 463U: _LL399: _LL39A:
# 3160 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD5F)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD60=({struct Cyc_Absyn_Exp*(*_tmpD61)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_and_exp;struct Cyc_Absyn_Exp*_tmpD62=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD63=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD64=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD61(_tmpD62,_tmpD63,_tmpD64);});_tmpD5F(_tmpD60);});
goto _LL0;case 464U: _LL39B: _LL39C:
# 3164 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 465U: _LL39D: _LL39E:
# 3166 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD65)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD66=({struct Cyc_Absyn_Exp*(*_tmpD67)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpD68=Cyc_Absyn_Bitor;struct Cyc_Absyn_Exp*_tmpD69=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD6A=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD6B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD67(_tmpD68,_tmpD69,_tmpD6A,_tmpD6B);});_tmpD65(_tmpD66);});
goto _LL0;case 466U: _LL39F: _LL3A0:
# 3170 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 467U: _LL3A1: _LL3A2:
# 3172 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD6C)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD6D=({struct Cyc_Absyn_Exp*(*_tmpD6E)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpD6F=Cyc_Absyn_Bitxor;struct Cyc_Absyn_Exp*_tmpD70=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD71=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD72=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD6E(_tmpD6F,_tmpD70,_tmpD71,_tmpD72);});_tmpD6C(_tmpD6D);});
goto _LL0;case 468U: _LL3A3: _LL3A4:
# 3176 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 469U: _LL3A5: _LL3A6:
# 3178 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD73)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD74=({struct Cyc_Absyn_Exp*(*_tmpD75)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpD76=Cyc_Absyn_Bitand;struct Cyc_Absyn_Exp*_tmpD77=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD78=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD79=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD75(_tmpD76,_tmpD77,_tmpD78,_tmpD79);});_tmpD73(_tmpD74);});
goto _LL0;case 470U: _LL3A7: _LL3A8:
# 3182 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 471U: _LL3A9: _LL3AA:
# 3184 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD7A)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD7B=({struct Cyc_Absyn_Exp*(*_tmpD7C)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_eq_exp;struct Cyc_Absyn_Exp*_tmpD7D=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD7E=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD7F=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD7C(_tmpD7D,_tmpD7E,_tmpD7F);});_tmpD7A(_tmpD7B);});
goto _LL0;case 472U: _LL3AB: _LL3AC:
# 3186 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD80)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD81=({struct Cyc_Absyn_Exp*(*_tmpD82)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_neq_exp;struct Cyc_Absyn_Exp*_tmpD83=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD84=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD85=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD82(_tmpD83,_tmpD84,_tmpD85);});_tmpD80(_tmpD81);});
goto _LL0;case 473U: _LL3AD: _LL3AE:
# 3190 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 474U: _LL3AF: _LL3B0:
# 3192 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD86)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD87=({struct Cyc_Absyn_Exp*(*_tmpD88)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_lt_exp;struct Cyc_Absyn_Exp*_tmpD89=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD8A=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD8B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD88(_tmpD89,_tmpD8A,_tmpD8B);});_tmpD86(_tmpD87);});
goto _LL0;case 475U: _LL3B1: _LL3B2:
# 3194 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD8C)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD8D=({struct Cyc_Absyn_Exp*(*_tmpD8E)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_gt_exp;struct Cyc_Absyn_Exp*_tmpD8F=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD90=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD91=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD8E(_tmpD8F,_tmpD90,_tmpD91);});_tmpD8C(_tmpD8D);});
goto _LL0;case 476U: _LL3B3: _LL3B4:
# 3196 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD92)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD93=({struct Cyc_Absyn_Exp*(*_tmpD94)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_lte_exp;struct Cyc_Absyn_Exp*_tmpD95=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD96=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD97=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD94(_tmpD95,_tmpD96,_tmpD97);});_tmpD92(_tmpD93);});
goto _LL0;case 477U: _LL3B5: _LL3B6:
# 3198 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD98)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD99=({struct Cyc_Absyn_Exp*(*_tmpD9A)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_gte_exp;struct Cyc_Absyn_Exp*_tmpD9B=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpD9C=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpD9D=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpD9A(_tmpD9B,_tmpD9C,_tmpD9D);});_tmpD98(_tmpD99);});
goto _LL0;case 478U: _LL3B7: _LL3B8:
# 3202 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 479U: _LL3B9: _LL3BA:
# 3204 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpD9E)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpD9F=({struct Cyc_Absyn_Exp*(*_tmpDA0)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpDA1=Cyc_Absyn_Bitlshift;struct Cyc_Absyn_Exp*_tmpDA2=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpDA3=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpDA4=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDA0(_tmpDA1,_tmpDA2,_tmpDA3,_tmpDA4);});_tmpD9E(_tmpD9F);});
goto _LL0;case 480U: _LL3BB: _LL3BC:
# 3206 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDA5)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDA6=({struct Cyc_Absyn_Exp*(*_tmpDA7)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpDA8=Cyc_Absyn_Bitlrshift;struct Cyc_Absyn_Exp*_tmpDA9=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpDAA=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpDAB=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDA7(_tmpDA8,_tmpDA9,_tmpDAA,_tmpDAB);});_tmpDA5(_tmpDA6);});
goto _LL0;case 481U: _LL3BD: _LL3BE:
# 3210 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 482U: _LL3BF: _LL3C0:
# 3212 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDAC)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDAD=({struct Cyc_Absyn_Exp*(*_tmpDAE)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpDAF=Cyc_Absyn_Plus;struct Cyc_Absyn_Exp*_tmpDB0=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpDB1=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpDB2=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDAE(_tmpDAF,_tmpDB0,_tmpDB1,_tmpDB2);});_tmpDAC(_tmpDAD);});
goto _LL0;case 483U: _LL3C1: _LL3C2:
# 3214 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDB3)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDB4=({struct Cyc_Absyn_Exp*(*_tmpDB5)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpDB6=Cyc_Absyn_Minus;struct Cyc_Absyn_Exp*_tmpDB7=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpDB8=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpDB9=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDB5(_tmpDB6,_tmpDB7,_tmpDB8,_tmpDB9);});_tmpDB3(_tmpDB4);});
goto _LL0;case 484U: _LL3C3: _LL3C4:
# 3218 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 485U: _LL3C5: _LL3C6:
# 3220 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDBA)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDBB=({struct Cyc_Absyn_Exp*(*_tmpDBC)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpDBD=Cyc_Absyn_Times;struct Cyc_Absyn_Exp*_tmpDBE=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpDBF=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpDC0=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDBC(_tmpDBD,_tmpDBE,_tmpDBF,_tmpDC0);});_tmpDBA(_tmpDBB);});
goto _LL0;case 486U: _LL3C7: _LL3C8:
# 3222 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDC1)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDC2=({struct Cyc_Absyn_Exp*(*_tmpDC3)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpDC4=Cyc_Absyn_Div;struct Cyc_Absyn_Exp*_tmpDC5=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpDC6=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpDC7=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDC3(_tmpDC4,_tmpDC5,_tmpDC6,_tmpDC7);});_tmpDC1(_tmpDC2);});
goto _LL0;case 487U: _LL3C9: _LL3CA:
# 3224 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDC8)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDC9=({struct Cyc_Absyn_Exp*(*_tmpDCA)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim2_exp;enum Cyc_Absyn_Primop _tmpDCB=Cyc_Absyn_Mod;struct Cyc_Absyn_Exp*_tmpDCC=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpDCD=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpDCE=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDCA(_tmpDCB,_tmpDCC,_tmpDCD,_tmpDCE);});_tmpDC8(_tmpDC9);});
goto _LL0;case 488U: _LL3CB: _LL3CC:
# 3228 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 489U: _LL3CD: _LL3CE: {
# 3230 "parse.y"
void*t=({void*(*_tmpDD7)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpDD8=({Cyc_yyget_YY37(&(yyyvsp[1]).v);});unsigned _tmpDD9=(unsigned)((yyyvsp[1]).l).first_line;_tmpDD7(_tmpDD8,_tmpDD9);});
yyval=({union Cyc_YYSTYPE(*_tmpDCF)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDD0=({struct Cyc_Absyn_Exp*(*_tmpDD1)(void*,struct Cyc_Absyn_Exp*,int user_cast,enum Cyc_Absyn_Coercion,unsigned)=Cyc_Absyn_cast_exp;void*_tmpDD2=t;struct Cyc_Absyn_Exp*_tmpDD3=({Cyc_yyget_Exp_tok(&(yyyvsp[3]).v);});int _tmpDD4=1;enum Cyc_Absyn_Coercion _tmpDD5=Cyc_Absyn_Unknown_coercion;unsigned _tmpDD6=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDD1(_tmpDD2,_tmpDD3,_tmpDD4,_tmpDD5,_tmpDD6);});_tmpDCF(_tmpDD0);});
# 3233
goto _LL0;}case 490U: _LL3CF: _LL3D0:
# 3236 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 491U: _LL3D1: _LL3D2:
# 3237 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDDA)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDDB=({struct Cyc_Absyn_Exp*(*_tmpDDC)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned)=Cyc_Absyn_increment_exp;struct Cyc_Absyn_Exp*_tmpDDD=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});enum Cyc_Absyn_Incrementor _tmpDDE=Cyc_Absyn_PreInc;unsigned _tmpDDF=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDDC(_tmpDDD,_tmpDDE,_tmpDDF);});_tmpDDA(_tmpDDB);});
goto _LL0;case 492U: _LL3D3: _LL3D4:
# 3238 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDE0)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDE1=({struct Cyc_Absyn_Exp*(*_tmpDE2)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned)=Cyc_Absyn_increment_exp;struct Cyc_Absyn_Exp*_tmpDE3=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});enum Cyc_Absyn_Incrementor _tmpDE4=Cyc_Absyn_PreDec;unsigned _tmpDE5=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDE2(_tmpDE3,_tmpDE4,_tmpDE5);});_tmpDE0(_tmpDE1);});
goto _LL0;case 493U: _LL3D5: _LL3D6:
# 3239 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDE6)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDE7=({struct Cyc_Absyn_Exp*(*_tmpDE8)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_address_exp;struct Cyc_Absyn_Exp*_tmpDE9=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpDEA=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDE8(_tmpDE9,_tmpDEA);});_tmpDE6(_tmpDE7);});
goto _LL0;case 494U: _LL3D7: _LL3D8:
# 3240 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDEB)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDEC=({struct Cyc_Absyn_Exp*(*_tmpDED)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmpDEE=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpDEF=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDED(_tmpDEE,_tmpDEF);});_tmpDEB(_tmpDEC);});
goto _LL0;case 495U: _LL3D9: _LL3DA:
# 3241 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDF0)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDF1=({struct Cyc_Absyn_Exp*(*_tmpDF2)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim1_exp;enum Cyc_Absyn_Primop _tmpDF3=Cyc_Absyn_Plus;struct Cyc_Absyn_Exp*_tmpDF4=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpDF5=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDF2(_tmpDF3,_tmpDF4,_tmpDF5);});_tmpDF0(_tmpDF1);});
goto _LL0;case 496U: _LL3DB: _LL3DC:
# 3242 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpDF6)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDF7=({struct Cyc_Absyn_Exp*(*_tmpDF8)(enum Cyc_Absyn_Primop,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_prim1_exp;enum Cyc_Absyn_Primop _tmpDF9=({Cyc_yyget_YY6(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpDFA=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpDFB=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDF8(_tmpDF9,_tmpDFA,_tmpDFB);});_tmpDF6(_tmpDF7);});
goto _LL0;case 497U: _LL3DD: _LL3DE: {
# 3244 "parse.y"
void*t=({void*(*_tmpE01)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpE02=({Cyc_yyget_YY37(&(yyyvsp[2]).v);});unsigned _tmpE03=(unsigned)((yyyvsp[2]).l).first_line;_tmpE01(_tmpE02,_tmpE03);});
yyval=({union Cyc_YYSTYPE(*_tmpDFC)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpDFD=({struct Cyc_Absyn_Exp*(*_tmpDFE)(void*t,unsigned)=Cyc_Absyn_sizeoftype_exp;void*_tmpDFF=t;unsigned _tmpE00=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpDFE(_tmpDFF,_tmpE00);});_tmpDFC(_tmpDFD);});
# 3247
goto _LL0;}case 498U: _LL3DF: _LL3E0:
# 3247 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE04)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE05=({struct Cyc_Absyn_Exp*(*_tmpE06)(struct Cyc_Absyn_Exp*e,unsigned)=Cyc_Absyn_sizeofexp_exp;struct Cyc_Absyn_Exp*_tmpE07=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpE08=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE06(_tmpE07,_tmpE08);});_tmpE04(_tmpE05);});
goto _LL0;case 499U: _LL3E1: _LL3E2: {
# 3249 "parse.y"
void*t=({void*(*_tmpE11)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpE12=({Cyc_yyget_YY37(&(yyyvsp[2]).v);});unsigned _tmpE13=(unsigned)((yyyvsp[2]).l).first_line;_tmpE11(_tmpE12,_tmpE13);});
yyval=({union Cyc_YYSTYPE(*_tmpE09)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE0A=({struct Cyc_Absyn_Exp*(*_tmpE0B)(void*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_offsetof_exp;void*_tmpE0C=t;struct Cyc_List_List*_tmpE0D=({struct Cyc_List_List*(*_tmpE0E)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpE0F=({Cyc_yyget_YY3(&(yyyvsp[4]).v);});_tmpE0E(_tmpE0F);});unsigned _tmpE10=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE0B(_tmpE0C,_tmpE0D,_tmpE10);});_tmpE09(_tmpE0A);});
# 3252
goto _LL0;}case 500U: _LL3E3: _LL3E4:
# 3254
 yyval=({union Cyc_YYSTYPE(*_tmpE14)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE15=({struct Cyc_Absyn_Exp*(*_tmpE16)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpE17=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmpE18=_cycalloc(sizeof(*_tmpE18));_tmpE18->tag=35U,(_tmpE18->f1).is_calloc=0,(_tmpE18->f1).rgn=0,(_tmpE18->f1).elt_type=0,({struct Cyc_Absyn_Exp*_tmp131F=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});(_tmpE18->f1).num_elts=_tmp131F;}),(_tmpE18->f1).fat_result=0,(_tmpE18->f1).inline_call=0;_tmpE18;});unsigned _tmpE19=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE16(_tmpE17,_tmpE19);});_tmpE14(_tmpE15);});
goto _LL0;case 501U: _LL3E5: _LL3E6:
# 3257 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE1A)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE1B=({struct Cyc_Absyn_Exp*(*_tmpE1C)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpE1D=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmpE1E=_cycalloc(sizeof(*_tmpE1E));_tmpE1E->tag=35U,(_tmpE1E->f1).is_calloc=0,({struct Cyc_Absyn_Exp*_tmp1321=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});(_tmpE1E->f1).rgn=_tmp1321;}),(_tmpE1E->f1).elt_type=0,({struct Cyc_Absyn_Exp*_tmp1320=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});(_tmpE1E->f1).num_elts=_tmp1320;}),(_tmpE1E->f1).fat_result=0,(_tmpE1E->f1).inline_call=0;_tmpE1E;});unsigned _tmpE1F=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE1C(_tmpE1D,_tmpE1F);});_tmpE1A(_tmpE1B);});
goto _LL0;case 502U: _LL3E7: _LL3E8:
# 3260 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE20)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE21=({struct Cyc_Absyn_Exp*(*_tmpE22)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpE23=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmpE24=_cycalloc(sizeof(*_tmpE24));_tmpE24->tag=35U,(_tmpE24->f1).is_calloc=0,({struct Cyc_Absyn_Exp*_tmp1323=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});(_tmpE24->f1).rgn=_tmp1323;}),(_tmpE24->f1).elt_type=0,({struct Cyc_Absyn_Exp*_tmp1322=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});(_tmpE24->f1).num_elts=_tmp1322;}),(_tmpE24->f1).fat_result=0,(_tmpE24->f1).inline_call=1;_tmpE24;});unsigned _tmpE25=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE22(_tmpE23,_tmpE25);});_tmpE20(_tmpE21);});
goto _LL0;case 503U: _LL3E9: _LL3EA: {
# 3263 "parse.y"
void*t=({void*(*_tmpE2D)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpE2E=({Cyc_yyget_YY37(&(yyyvsp[6]).v);});unsigned _tmpE2F=(unsigned)((yyyvsp[6]).l).first_line;_tmpE2D(_tmpE2E,_tmpE2F);});
yyval=({union Cyc_YYSTYPE(*_tmpE26)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE27=({struct Cyc_Absyn_Exp*(*_tmpE28)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpE29=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmpE2B=_cycalloc(sizeof(*_tmpE2B));_tmpE2B->tag=35U,(_tmpE2B->f1).is_calloc=1,(_tmpE2B->f1).rgn=0,({void**_tmp1325=({void**_tmpE2A=_cycalloc(sizeof(*_tmpE2A));*_tmpE2A=t;_tmpE2A;});(_tmpE2B->f1).elt_type=_tmp1325;}),({struct Cyc_Absyn_Exp*_tmp1324=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});(_tmpE2B->f1).num_elts=_tmp1324;}),(_tmpE2B->f1).fat_result=0,(_tmpE2B->f1).inline_call=0;_tmpE2B;});unsigned _tmpE2C=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE28(_tmpE29,_tmpE2C);});_tmpE26(_tmpE27);});
goto _LL0;}case 504U: _LL3EB: _LL3EC: {
# 3268
void*t=({void*(*_tmpE37)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpE38=({Cyc_yyget_YY37(&(yyyvsp[8]).v);});unsigned _tmpE39=(unsigned)((yyyvsp[8]).l).first_line;_tmpE37(_tmpE38,_tmpE39);});
yyval=({union Cyc_YYSTYPE(*_tmpE30)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE31=({struct Cyc_Absyn_Exp*(*_tmpE32)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpE33=(void*)({struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*_tmpE35=_cycalloc(sizeof(*_tmpE35));_tmpE35->tag=35U,(_tmpE35->f1).is_calloc=1,({struct Cyc_Absyn_Exp*_tmp1328=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});(_tmpE35->f1).rgn=_tmp1328;}),({void**_tmp1327=({void**_tmpE34=_cycalloc(sizeof(*_tmpE34));*_tmpE34=t;_tmpE34;});(_tmpE35->f1).elt_type=_tmp1327;}),({struct Cyc_Absyn_Exp*_tmp1326=({Cyc_yyget_Exp_tok(&(yyyvsp[4]).v);});(_tmpE35->f1).num_elts=_tmp1326;}),(_tmpE35->f1).fat_result=0,(_tmpE35->f1).inline_call=0;_tmpE35;});unsigned _tmpE36=(unsigned)({
yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE32(_tmpE33,_tmpE36);});_tmpE30(_tmpE31);});
goto _LL0;}case 505U: _LL3ED: _LL3EE:
# 3272 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE3A)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE3B=({struct Cyc_Absyn_Exp*(*_tmpE3C)(enum Cyc_Absyn_Primop,struct Cyc_List_List*,unsigned)=Cyc_Absyn_primop_exp;enum Cyc_Absyn_Primop _tmpE3D=Cyc_Absyn_Numelts;struct Cyc_List_List*_tmpE3E=({struct Cyc_Absyn_Exp*_tmpE3F[1U];({struct Cyc_Absyn_Exp*_tmp1329=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmpE3F[0]=_tmp1329;});Cyc_List_list(_tag_fat(_tmpE3F,sizeof(struct Cyc_Absyn_Exp*),1U));});unsigned _tmpE40=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE3C(_tmpE3D,_tmpE3E,_tmpE40);});_tmpE3A(_tmpE3B);});
goto _LL0;case 506U: _LL3EF: _LL3F0:
# 3274 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE41)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE42=({struct Cyc_Absyn_Exp*(*_tmpE43)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpE44=(void*)({struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_tmpE46=_cycalloc(sizeof(*_tmpE46));_tmpE46->tag=39U,({struct Cyc_Absyn_Exp*_tmp132C=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmpE46->f1=_tmp132C;}),({struct _fat_ptr*_tmp132B=({struct _fat_ptr*_tmpE45=_cycalloc(sizeof(*_tmpE45));({struct _fat_ptr _tmp132A=({Cyc_yyget_String_tok(&(yyyvsp[4]).v);});*_tmpE45=_tmp132A;});_tmpE45;});_tmpE46->f2=_tmp132B;});_tmpE46;});unsigned _tmpE47=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE43(_tmpE44,_tmpE47);});_tmpE41(_tmpE42);});
goto _LL0;case 507U: _LL3F1: _LL3F2:
# 3276 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE48)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE49=({struct Cyc_Absyn_Exp*(*_tmpE4A)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpE4B=(void*)({struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*_tmpE50=_cycalloc(sizeof(*_tmpE50));_tmpE50->tag=39U,({struct Cyc_Absyn_Exp*_tmp132F=({struct Cyc_Absyn_Exp*(*_tmpE4C)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_deref_exp;struct Cyc_Absyn_Exp*_tmpE4D=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpE4E=(unsigned)((yyyvsp[2]).l).first_line;_tmpE4C(_tmpE4D,_tmpE4E);});_tmpE50->f1=_tmp132F;}),({struct _fat_ptr*_tmp132E=({struct _fat_ptr*_tmpE4F=_cycalloc(sizeof(*_tmpE4F));({struct _fat_ptr _tmp132D=({Cyc_yyget_String_tok(&(yyyvsp[4]).v);});*_tmpE4F=_tmp132D;});_tmpE4F;});_tmpE50->f2=_tmp132E;});_tmpE50;});unsigned _tmpE51=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE4A(_tmpE4B,_tmpE51);});_tmpE48(_tmpE49);});
goto _LL0;case 508U: _LL3F3: _LL3F4: {
# 3278 "parse.y"
void*t=({void*(*_tmpE57)(struct _tuple9*tqt,unsigned loc)=Cyc_Parse_type_name_to_type;struct _tuple9*_tmpE58=({Cyc_yyget_YY37(&(yyyvsp[2]).v);});unsigned _tmpE59=(unsigned)((yyyvsp[2]).l).first_line;_tmpE57(_tmpE58,_tmpE59);});
yyval=({union Cyc_YYSTYPE(*_tmpE52)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE53=({struct Cyc_Absyn_Exp*(*_tmpE54)(void*,unsigned)=Cyc_Absyn_valueof_exp;void*_tmpE55=t;unsigned _tmpE56=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE54(_tmpE55,_tmpE56);});_tmpE52(_tmpE53);});
goto _LL0;}case 509U: _LL3F5: _LL3F6:
# 3281 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE5A)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE5B=({struct Cyc_Absyn_Exp*(*_tmpE5C)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpE5D=({Cyc_yyget_YY58(&(yyyvsp[1]).v);});unsigned _tmpE5E=(unsigned)((yyyvsp[0]).l).first_line;_tmpE5C(_tmpE5D,_tmpE5E);});_tmpE5A(_tmpE5B);});
goto _LL0;case 510U: _LL3F7: _LL3F8:
# 3282 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE5F)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE60=({struct Cyc_Absyn_Exp*(*_tmpE61)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_extension_exp;struct Cyc_Absyn_Exp*_tmpE62=({Cyc_yyget_Exp_tok(&(yyyvsp[1]).v);});unsigned _tmpE63=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE61(_tmpE62,_tmpE63);});_tmpE5F(_tmpE60);});
goto _LL0;case 511U: _LL3F9: _LL3FA: {
# 3287 "parse.y"
struct _tuple29*_stmttmp2E=({Cyc_yyget_YY59(&(yyyvsp[3]).v);});struct Cyc_List_List*_tmpE66;struct Cyc_List_List*_tmpE65;struct Cyc_List_List*_tmpE64;_LL4D5: _tmpE64=_stmttmp2E->f1;_tmpE65=_stmttmp2E->f2;_tmpE66=_stmttmp2E->f3;_LL4D6: {struct Cyc_List_List*outlist=_tmpE64;struct Cyc_List_List*inlist=_tmpE65;struct Cyc_List_List*clobbers=_tmpE66;
yyval=({union Cyc_YYSTYPE(*_tmpE67)(void*yy1)=Cyc_YY58;void*_tmpE68=(void*)({struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*_tmpE69=_cycalloc(sizeof(*_tmpE69));_tmpE69->tag=41U,({int _tmp1331=({Cyc_yyget_YY31(&(yyyvsp[0]).v);});_tmpE69->f1=_tmp1331;}),({struct _fat_ptr _tmp1330=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});_tmpE69->f2=_tmp1330;}),_tmpE69->f3=outlist,_tmpE69->f4=inlist,_tmpE69->f5=clobbers;_tmpE69;});_tmpE67(_tmpE68);});
goto _LL0;}}case 512U: _LL3FB: _LL3FC:
# 3292 "parse.y"
 yyval=({Cyc_YY31(0);});
goto _LL0;case 513U: _LL3FD: _LL3FE:
# 3293 "parse.y"
 yyval=({Cyc_YY31(1);});
goto _LL0;case 514U: _LL3FF: _LL400:
# 3297 "parse.y"
 yyval=({Cyc_YY59(({struct _tuple29*_tmpE6A=_cycalloc(sizeof(*_tmpE6A));_tmpE6A->f1=0,_tmpE6A->f2=0,_tmpE6A->f3=0;_tmpE6A;}));});
goto _LL0;case 515U: _LL401: _LL402: {
# 3299 "parse.y"
struct _tuple8*_stmttmp2F=({Cyc_yyget_YY60(&(yyyvsp[1]).v);});struct Cyc_List_List*_tmpE6C;struct Cyc_List_List*_tmpE6B;_LL4D8: _tmpE6B=_stmttmp2F->f1;_tmpE6C=_stmttmp2F->f2;_LL4D9: {struct Cyc_List_List*inlist=_tmpE6B;struct Cyc_List_List*clobbers=_tmpE6C;
yyval=({Cyc_YY59(({struct _tuple29*_tmpE6D=_cycalloc(sizeof(*_tmpE6D));_tmpE6D->f1=0,_tmpE6D->f2=inlist,_tmpE6D->f3=clobbers;_tmpE6D;}));});
goto _LL0;}}case 516U: _LL403: _LL404: {
# 3302 "parse.y"
struct _tuple8*_stmttmp30=({Cyc_yyget_YY60(&(yyyvsp[2]).v);});struct Cyc_List_List*_tmpE6F;struct Cyc_List_List*_tmpE6E;_LL4DB: _tmpE6E=_stmttmp30->f1;_tmpE6F=_stmttmp30->f2;_LL4DC: {struct Cyc_List_List*inlist=_tmpE6E;struct Cyc_List_List*clobbers=_tmpE6F;
yyval=({union Cyc_YYSTYPE(*_tmpE70)(struct _tuple29*yy1)=Cyc_YY59;struct _tuple29*_tmpE71=({struct _tuple29*_tmpE74=_cycalloc(sizeof(*_tmpE74));({struct Cyc_List_List*_tmp1332=({struct Cyc_List_List*(*_tmpE72)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpE73=({Cyc_yyget_YY62(&(yyyvsp[1]).v);});_tmpE72(_tmpE73);});_tmpE74->f1=_tmp1332;}),_tmpE74->f2=inlist,_tmpE74->f3=clobbers;_tmpE74;});_tmpE70(_tmpE71);});
goto _LL0;}}case 517U: _LL405: _LL406:
# 3307 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE75)(struct Cyc_List_List*yy1)=Cyc_YY62;struct Cyc_List_List*_tmpE76=({struct Cyc_List_List*_tmpE77=_cycalloc(sizeof(*_tmpE77));({struct _tuple30*_tmp1333=({Cyc_yyget_YY63(&(yyyvsp[0]).v);});_tmpE77->hd=_tmp1333;}),_tmpE77->tl=0;_tmpE77;});_tmpE75(_tmpE76);});
goto _LL0;case 518U: _LL407: _LL408:
# 3308 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE78)(struct Cyc_List_List*yy1)=Cyc_YY62;struct Cyc_List_List*_tmpE79=({struct Cyc_List_List*_tmpE7A=_cycalloc(sizeof(*_tmpE7A));({struct _tuple30*_tmp1335=({Cyc_yyget_YY63(&(yyyvsp[2]).v);});_tmpE7A->hd=_tmp1335;}),({struct Cyc_List_List*_tmp1334=({Cyc_yyget_YY62(&(yyyvsp[0]).v);});_tmpE7A->tl=_tmp1334;});_tmpE7A;});_tmpE78(_tmpE79);});
goto _LL0;case 519U: _LL409: _LL40A:
# 3312 "parse.y"
 yyval=({Cyc_YY60(({struct _tuple8*_tmpE7B=_cycalloc(sizeof(*_tmpE7B));_tmpE7B->f1=0,_tmpE7B->f2=0;_tmpE7B;}));});
goto _LL0;case 520U: _LL40B: _LL40C:
# 3314 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE7C)(struct _tuple8*yy1)=Cyc_YY60;struct _tuple8*_tmpE7D=({struct _tuple8*_tmpE7E=_cycalloc(sizeof(*_tmpE7E));_tmpE7E->f1=0,({struct Cyc_List_List*_tmp1336=({Cyc_yyget_YY61(&(yyyvsp[1]).v);});_tmpE7E->f2=_tmp1336;});_tmpE7E;});_tmpE7C(_tmpE7D);});
goto _LL0;case 521U: _LL40D: _LL40E:
# 3316 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE7F)(struct _tuple8*yy1)=Cyc_YY60;struct _tuple8*_tmpE80=({struct _tuple8*_tmpE83=_cycalloc(sizeof(*_tmpE83));({struct Cyc_List_List*_tmp1338=({struct Cyc_List_List*(*_tmpE81)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpE82=({Cyc_yyget_YY62(&(yyyvsp[1]).v);});_tmpE81(_tmpE82);});_tmpE83->f1=_tmp1338;}),({struct Cyc_List_List*_tmp1337=({Cyc_yyget_YY61(&(yyyvsp[2]).v);});_tmpE83->f2=_tmp1337;});_tmpE83;});_tmpE7F(_tmpE80);});
goto _LL0;case 522U: _LL40F: _LL410:
# 3320 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE84)(struct Cyc_List_List*yy1)=Cyc_YY62;struct Cyc_List_List*_tmpE85=({struct Cyc_List_List*_tmpE86=_cycalloc(sizeof(*_tmpE86));({struct _tuple30*_tmp1339=({Cyc_yyget_YY63(&(yyyvsp[0]).v);});_tmpE86->hd=_tmp1339;}),_tmpE86->tl=0;_tmpE86;});_tmpE84(_tmpE85);});
goto _LL0;case 523U: _LL411: _LL412:
# 3321 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE87)(struct Cyc_List_List*yy1)=Cyc_YY62;struct Cyc_List_List*_tmpE88=({struct Cyc_List_List*_tmpE89=_cycalloc(sizeof(*_tmpE89));({struct _tuple30*_tmp133B=({Cyc_yyget_YY63(&(yyyvsp[2]).v);});_tmpE89->hd=_tmp133B;}),({struct Cyc_List_List*_tmp133A=({Cyc_yyget_YY62(&(yyyvsp[0]).v);});_tmpE89->tl=_tmp133A;});_tmpE89;});_tmpE87(_tmpE88);});
goto _LL0;case 524U: _LL413: _LL414: {
# 3326 "parse.y"
struct Cyc_Absyn_Exp*pf_exp=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});
yyval=({union Cyc_YYSTYPE(*_tmpE8A)(struct _tuple30*yy1)=Cyc_YY63;struct _tuple30*_tmpE8B=({struct _tuple30*_tmpE8C=_cycalloc(sizeof(*_tmpE8C));({struct _fat_ptr _tmp133D=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});_tmpE8C->f1=_tmp133D;}),({struct Cyc_Absyn_Exp*_tmp133C=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmpE8C->f2=_tmp133C;});_tmpE8C;});_tmpE8A(_tmpE8B);});
goto _LL0;}case 525U: _LL415: _LL416:
# 3332 "parse.y"
 yyval=({Cyc_YY61(0);});
goto _LL0;case 526U: _LL417: _LL418:
# 3333 "parse.y"
 yyval=({Cyc_YY61(0);});
goto _LL0;case 527U: _LL419: _LL41A:
# 3334 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE8D)(struct Cyc_List_List*yy1)=Cyc_YY61;struct Cyc_List_List*_tmpE8E=({struct Cyc_List_List*(*_tmpE8F)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpE90=({Cyc_yyget_YY61(&(yyyvsp[1]).v);});_tmpE8F(_tmpE90);});_tmpE8D(_tmpE8E);});
goto _LL0;case 528U: _LL41B: _LL41C:
# 3338 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE91)(struct Cyc_List_List*yy1)=Cyc_YY61;struct Cyc_List_List*_tmpE92=({struct Cyc_List_List*_tmpE94=_cycalloc(sizeof(*_tmpE94));({struct _fat_ptr*_tmp133F=({struct _fat_ptr*_tmpE93=_cycalloc(sizeof(*_tmpE93));({struct _fat_ptr _tmp133E=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmpE93=_tmp133E;});_tmpE93;});_tmpE94->hd=_tmp133F;}),_tmpE94->tl=0;_tmpE94;});_tmpE91(_tmpE92);});
goto _LL0;case 529U: _LL41D: _LL41E:
# 3339 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE95)(struct Cyc_List_List*yy1)=Cyc_YY61;struct Cyc_List_List*_tmpE96=({struct Cyc_List_List*_tmpE98=_cycalloc(sizeof(*_tmpE98));({struct _fat_ptr*_tmp1342=({struct _fat_ptr*_tmpE97=_cycalloc(sizeof(*_tmpE97));({struct _fat_ptr _tmp1341=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});*_tmpE97=_tmp1341;});_tmpE97;});_tmpE98->hd=_tmp1342;}),({struct Cyc_List_List*_tmp1340=({Cyc_yyget_YY61(&(yyyvsp[0]).v);});_tmpE98->tl=_tmp1340;});_tmpE98;});_tmpE95(_tmpE96);});
goto _LL0;case 530U: _LL41F: _LL420:
# 3343 "parse.y"
 yyval=({Cyc_YY6(Cyc_Absyn_Bitnot);});
goto _LL0;case 531U: _LL421: _LL422:
# 3344 "parse.y"
 yyval=({Cyc_YY6(Cyc_Absyn_Not);});
goto _LL0;case 532U: _LL423: _LL424:
# 3345 "parse.y"
 yyval=({Cyc_YY6(Cyc_Absyn_Minus);});
goto _LL0;case 533U: _LL425: _LL426:
# 3350 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 534U: _LL427: _LL428:
# 3352 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE99)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpE9A=({struct Cyc_Absyn_Exp*(*_tmpE9B)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_subscript_exp;struct Cyc_Absyn_Exp*_tmpE9C=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_Absyn_Exp*_tmpE9D=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});unsigned _tmpE9E=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpE9B(_tmpE9C,_tmpE9D,_tmpE9E);});_tmpE99(_tmpE9A);});
goto _LL0;case 535U: _LL429: _LL42A:
# 3354 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpE9F)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEA0=({struct Cyc_Absyn_Exp*(*_tmpEA1)(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_unknowncall_exp;struct Cyc_Absyn_Exp*_tmpEA2=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmpEA3=0;unsigned _tmpEA4=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpEA1(_tmpEA2,_tmpEA3,_tmpEA4);});_tmpE9F(_tmpEA0);});
goto _LL0;case 536U: _LL42B: _LL42C:
# 3356 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEA5)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEA6=({struct Cyc_Absyn_Exp*(*_tmpEA7)(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_unknowncall_exp;struct Cyc_Absyn_Exp*_tmpEA8=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmpEA9=({Cyc_yyget_YY4(&(yyyvsp[2]).v);});unsigned _tmpEAA=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpEA7(_tmpEA8,_tmpEA9,_tmpEAA);});_tmpEA5(_tmpEA6);});
goto _LL0;case 537U: _LL42D: _LL42E:
# 3358 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEAB)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEAC=({struct Cyc_Absyn_Exp*(*_tmpEAD)(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned)=Cyc_Absyn_aggrmember_exp;struct Cyc_Absyn_Exp*_tmpEAE=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct _fat_ptr*_tmpEAF=({struct _fat_ptr*_tmpEB0=_cycalloc(sizeof(*_tmpEB0));({struct _fat_ptr _tmp1343=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});*_tmpEB0=_tmp1343;});_tmpEB0;});unsigned _tmpEB1=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpEAD(_tmpEAE,_tmpEAF,_tmpEB1);});_tmpEAB(_tmpEAC);});
goto _LL0;case 538U: _LL42F: _LL430:
# 3360 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEB2)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEB3=({struct Cyc_Absyn_Exp*(*_tmpEB4)(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned)=Cyc_Absyn_aggrarrow_exp;struct Cyc_Absyn_Exp*_tmpEB5=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct _fat_ptr*_tmpEB6=({struct _fat_ptr*_tmpEB7=_cycalloc(sizeof(*_tmpEB7));({struct _fat_ptr _tmp1344=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});*_tmpEB7=_tmp1344;});_tmpEB7;});unsigned _tmpEB8=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpEB4(_tmpEB5,_tmpEB6,_tmpEB8);});_tmpEB2(_tmpEB3);});
goto _LL0;case 539U: _LL431: _LL432:
# 3362 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEB9)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEBA=({struct Cyc_Absyn_Exp*(*_tmpEBB)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned)=Cyc_Absyn_increment_exp;struct Cyc_Absyn_Exp*_tmpEBC=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});enum Cyc_Absyn_Incrementor _tmpEBD=Cyc_Absyn_PostInc;unsigned _tmpEBE=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpEBB(_tmpEBC,_tmpEBD,_tmpEBE);});_tmpEB9(_tmpEBA);});
goto _LL0;case 540U: _LL433: _LL434:
# 3364 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEBF)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEC0=({struct Cyc_Absyn_Exp*(*_tmpEC1)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned)=Cyc_Absyn_increment_exp;struct Cyc_Absyn_Exp*_tmpEC2=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});enum Cyc_Absyn_Incrementor _tmpEC3=Cyc_Absyn_PostDec;unsigned _tmpEC4=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpEC1(_tmpEC2,_tmpEC3,_tmpEC4);});_tmpEBF(_tmpEC0);});
goto _LL0;case 541U: _LL435: _LL436:
# 3366 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEC5)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEC6=({struct Cyc_Absyn_Exp*(*_tmpEC7)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpEC8=(void*)({struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_tmpEC9=_cycalloc(sizeof(*_tmpEC9));_tmpEC9->tag=25U,({struct _tuple9*_tmp1345=({Cyc_yyget_YY37(&(yyyvsp[1]).v);});_tmpEC9->f1=_tmp1345;}),_tmpEC9->f2=0;_tmpEC9;});unsigned _tmpECA=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpEC7(_tmpEC8,_tmpECA);});_tmpEC5(_tmpEC6);});
goto _LL0;case 542U: _LL437: _LL438:
# 3368 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpECB)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpECC=({struct Cyc_Absyn_Exp*(*_tmpECD)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpECE=(void*)({struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_tmpED1=_cycalloc(sizeof(*_tmpED1));_tmpED1->tag=25U,({struct _tuple9*_tmp1347=({Cyc_yyget_YY37(&(yyyvsp[1]).v);});_tmpED1->f1=_tmp1347;}),({struct Cyc_List_List*_tmp1346=({struct Cyc_List_List*(*_tmpECF)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpED0=({Cyc_yyget_YY5(&(yyyvsp[4]).v);});_tmpECF(_tmpED0);});_tmpED1->f2=_tmp1346;});_tmpED1;});unsigned _tmpED2=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpECD(_tmpECE,_tmpED2);});_tmpECB(_tmpECC);});
goto _LL0;case 543U: _LL439: _LL43A:
# 3370 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpED3)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpED4=({struct Cyc_Absyn_Exp*(*_tmpED5)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpED6=(void*)({struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*_tmpED9=_cycalloc(sizeof(*_tmpED9));_tmpED9->tag=25U,({struct _tuple9*_tmp1349=({Cyc_yyget_YY37(&(yyyvsp[1]).v);});_tmpED9->f1=_tmp1349;}),({struct Cyc_List_List*_tmp1348=({struct Cyc_List_List*(*_tmpED7)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpED8=({Cyc_yyget_YY5(&(yyyvsp[4]).v);});_tmpED7(_tmpED8);});_tmpED9->f2=_tmp1348;});_tmpED9;});unsigned _tmpEDA=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpED5(_tmpED6,_tmpEDA);});_tmpED3(_tmpED4);});
goto _LL0;case 544U: _LL43B: _LL43C:
# 3375 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEDB)(struct Cyc_List_List*yy1)=Cyc_YY3;struct Cyc_List_List*_tmpEDC=({struct Cyc_List_List*_tmpEDF=_cycalloc(sizeof(*_tmpEDF));({void*_tmp134C=(void*)({struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_tmpEDE=_cycalloc(sizeof(*_tmpEDE));_tmpEDE->tag=0U,({struct _fat_ptr*_tmp134B=({struct _fat_ptr*_tmpEDD=_cycalloc(sizeof(*_tmpEDD));({struct _fat_ptr _tmp134A=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmpEDD=_tmp134A;});_tmpEDD;});_tmpEDE->f1=_tmp134B;});_tmpEDE;});_tmpEDF->hd=_tmp134C;}),_tmpEDF->tl=0;_tmpEDF;});_tmpEDB(_tmpEDC);});
goto _LL0;case 545U: _LL43D: _LL43E:
# 3378
 yyval=({union Cyc_YYSTYPE(*_tmpEE0)(struct Cyc_List_List*yy1)=Cyc_YY3;struct Cyc_List_List*_tmpEE1=({struct Cyc_List_List*_tmpEE6=_cycalloc(sizeof(*_tmpEE6));({void*_tmp134E=(void*)({struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*_tmpEE5=_cycalloc(sizeof(*_tmpEE5));_tmpEE5->tag=1U,({unsigned _tmp134D=({unsigned(*_tmpEE2)(unsigned loc,union Cyc_Absyn_Cnst x)=Cyc_Parse_cnst2uint;unsigned _tmpEE3=(unsigned)((yyyvsp[0]).l).first_line;union Cyc_Absyn_Cnst _tmpEE4=({Cyc_yyget_Int_tok(&(yyyvsp[0]).v);});_tmpEE2(_tmpEE3,_tmpEE4);});_tmpEE5->f1=_tmp134D;});_tmpEE5;});_tmpEE6->hd=_tmp134E;}),_tmpEE6->tl=0;_tmpEE6;});_tmpEE0(_tmpEE1);});
goto _LL0;case 546U: _LL43F: _LL440:
# 3380 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEE7)(struct Cyc_List_List*yy1)=Cyc_YY3;struct Cyc_List_List*_tmpEE8=({struct Cyc_List_List*_tmpEEB=_cycalloc(sizeof(*_tmpEEB));({void*_tmp1352=(void*)({struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*_tmpEEA=_cycalloc(sizeof(*_tmpEEA));_tmpEEA->tag=0U,({struct _fat_ptr*_tmp1351=({struct _fat_ptr*_tmpEE9=_cycalloc(sizeof(*_tmpEE9));({struct _fat_ptr _tmp1350=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});*_tmpEE9=_tmp1350;});_tmpEE9;});_tmpEEA->f1=_tmp1351;});_tmpEEA;});_tmpEEB->hd=_tmp1352;}),({struct Cyc_List_List*_tmp134F=({Cyc_yyget_YY3(&(yyyvsp[0]).v);});_tmpEEB->tl=_tmp134F;});_tmpEEB;});_tmpEE7(_tmpEE8);});
goto _LL0;case 547U: _LL441: _LL442:
# 3383
 yyval=({union Cyc_YYSTYPE(*_tmpEEC)(struct Cyc_List_List*yy1)=Cyc_YY3;struct Cyc_List_List*_tmpEED=({struct Cyc_List_List*_tmpEF2=_cycalloc(sizeof(*_tmpEF2));({void*_tmp1355=(void*)({struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*_tmpEF1=_cycalloc(sizeof(*_tmpEF1));_tmpEF1->tag=1U,({unsigned _tmp1354=({unsigned(*_tmpEEE)(unsigned loc,union Cyc_Absyn_Cnst x)=Cyc_Parse_cnst2uint;unsigned _tmpEEF=(unsigned)((yyyvsp[2]).l).first_line;union Cyc_Absyn_Cnst _tmpEF0=({Cyc_yyget_Int_tok(&(yyyvsp[2]).v);});_tmpEEE(_tmpEEF,_tmpEF0);});_tmpEF1->f1=_tmp1354;});_tmpEF1;});_tmpEF2->hd=_tmp1355;}),({struct Cyc_List_List*_tmp1353=({Cyc_yyget_YY3(&(yyyvsp[0]).v);});_tmpEF2->tl=_tmp1353;});_tmpEF2;});_tmpEEC(_tmpEED);});
goto _LL0;case 548U: _LL443: _LL444:
# 3389 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEF3)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEF4=({struct Cyc_Absyn_Exp*(*_tmpEF5)(struct _tuple0*,unsigned)=Cyc_Absyn_unknownid_exp;struct _tuple0*_tmpEF6=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});unsigned _tmpEF7=(unsigned)((yyyvsp[0]).l).first_line;_tmpEF5(_tmpEF6,_tmpEF7);});_tmpEF3(_tmpEF4);});
goto _LL0;case 549U: _LL445: _LL446:
# 3391 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEF8)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEF9=({struct Cyc_Absyn_Exp*(*_tmpEFA)(struct _fat_ptr,unsigned)=Cyc_Absyn_pragma_exp;struct _fat_ptr _tmpEFB=({Cyc_yyget_String_tok(&(yyyvsp[2]).v);});unsigned _tmpEFC=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpEFA(_tmpEFB,_tmpEFC);});_tmpEF8(_tmpEF9);});
goto _LL0;case 550U: _LL447: _LL448:
# 3393 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 551U: _LL449: _LL44A:
# 3395 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpEFD)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpEFE=({struct Cyc_Absyn_Exp*(*_tmpEFF)(struct _fat_ptr,unsigned)=Cyc_Absyn_string_exp;struct _fat_ptr _tmpF00=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});unsigned _tmpF01=(unsigned)((yyyvsp[0]).l).first_line;_tmpEFF(_tmpF00,_tmpF01);});_tmpEFD(_tmpEFE);});
goto _LL0;case 552U: _LL44B: _LL44C:
# 3397 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF02)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF03=({struct Cyc_Absyn_Exp*(*_tmpF04)(struct _fat_ptr,unsigned)=Cyc_Absyn_wstring_exp;struct _fat_ptr _tmpF05=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});unsigned _tmpF06=(unsigned)((yyyvsp[0]).l).first_line;_tmpF04(_tmpF05,_tmpF06);});_tmpF02(_tmpF03);});
goto _LL0;case 553U: _LL44D: _LL44E:
# 3399 "parse.y"
 yyval=(yyyvsp[1]).v;
goto _LL0;case 554U: _LL44F: _LL450:
# 3404 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF07)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF08=({struct Cyc_Absyn_Exp*(*_tmpF09)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_noinstantiate_exp;struct Cyc_Absyn_Exp*_tmpF0A=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});unsigned _tmpF0B=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpF09(_tmpF0A,_tmpF0B);});_tmpF07(_tmpF08);});
goto _LL0;case 555U: _LL451: _LL452:
# 3406 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF0C)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF0D=({struct Cyc_Absyn_Exp*(*_tmpF0E)(struct Cyc_Absyn_Exp*,struct Cyc_List_List*,unsigned)=Cyc_Absyn_instantiate_exp;struct Cyc_Absyn_Exp*_tmpF0F=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});struct Cyc_List_List*_tmpF10=({struct Cyc_List_List*(*_tmpF11)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpF12=({Cyc_yyget_YY40(&(yyyvsp[3]).v);});_tmpF11(_tmpF12);});unsigned _tmpF13=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpF0E(_tmpF0F,_tmpF10,_tmpF13);});_tmpF0C(_tmpF0D);});
goto _LL0;case 556U: _LL453: _LL454:
# 3409
 yyval=({union Cyc_YYSTYPE(*_tmpF14)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF15=({struct Cyc_Absyn_Exp*(*_tmpF16)(struct Cyc_List_List*,unsigned)=Cyc_Absyn_tuple_exp;struct Cyc_List_List*_tmpF17=({Cyc_yyget_YY4(&(yyyvsp[2]).v);});unsigned _tmpF18=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpF16(_tmpF17,_tmpF18);});_tmpF14(_tmpF15);});
goto _LL0;case 557U: _LL455: _LL456:
# 3412
 yyval=({union Cyc_YYSTYPE(*_tmpF19)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF1A=({struct Cyc_Absyn_Exp*(*_tmpF1B)(void*,unsigned)=Cyc_Absyn_new_exp;void*_tmpF1C=(void*)({struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*_tmpF1F=_cycalloc(sizeof(*_tmpF1F));_tmpF1F->tag=29U,({struct _tuple0*_tmp1358=({Cyc_yyget_QualId_tok(&(yyyvsp[0]).v);});_tmpF1F->f1=_tmp1358;}),({struct Cyc_List_List*_tmp1357=({Cyc_yyget_YY40(&(yyyvsp[2]).v);});_tmpF1F->f2=_tmp1357;}),({struct Cyc_List_List*_tmp1356=({struct Cyc_List_List*(*_tmpF1D)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpF1E=({Cyc_yyget_YY5(&(yyyvsp[3]).v);});_tmpF1D(_tmpF1E);});_tmpF1F->f3=_tmp1356;}),_tmpF1F->f4=0;_tmpF1F;});unsigned _tmpF20=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpF1B(_tmpF1C,_tmpF20);});_tmpF19(_tmpF1A);});
goto _LL0;case 558U: _LL457: _LL458:
# 3415
 yyval=({union Cyc_YYSTYPE(*_tmpF21)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF22=({struct Cyc_Absyn_Exp*(*_tmpF23)(struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_stmt_exp;struct Cyc_Absyn_Stmt*_tmpF24=({Cyc_yyget_Stmt_tok(&(yyyvsp[2]).v);});unsigned _tmpF25=(unsigned)({yylloc.first_line=((yyyvsp[0]).l).first_line;((yyyvsp[0]).l).first_line;});_tmpF23(_tmpF24,_tmpF25);});_tmpF21(_tmpF22);});
goto _LL0;case 559U: _LL459: _LL45A:
# 3419 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF26)(struct Cyc_List_List*yy1)=Cyc_YY4;struct Cyc_List_List*_tmpF27=({struct Cyc_List_List*(*_tmpF28)(struct Cyc_List_List*x)=Cyc_List_imp_rev;struct Cyc_List_List*_tmpF29=({Cyc_yyget_YY4(&(yyyvsp[0]).v);});_tmpF28(_tmpF29);});_tmpF26(_tmpF27);});
goto _LL0;case 560U: _LL45B: _LL45C:
# 3425 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF2A)(struct Cyc_List_List*yy1)=Cyc_YY4;struct Cyc_List_List*_tmpF2B=({struct Cyc_List_List*_tmpF2C=_cycalloc(sizeof(*_tmpF2C));({struct Cyc_Absyn_Exp*_tmp1359=({Cyc_yyget_Exp_tok(&(yyyvsp[0]).v);});_tmpF2C->hd=_tmp1359;}),_tmpF2C->tl=0;_tmpF2C;});_tmpF2A(_tmpF2B);});
goto _LL0;case 561U: _LL45D: _LL45E:
# 3427 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF2D)(struct Cyc_List_List*yy1)=Cyc_YY4;struct Cyc_List_List*_tmpF2E=({struct Cyc_List_List*_tmpF2F=_cycalloc(sizeof(*_tmpF2F));({struct Cyc_Absyn_Exp*_tmp135B=({Cyc_yyget_Exp_tok(&(yyyvsp[2]).v);});_tmpF2F->hd=_tmp135B;}),({struct Cyc_List_List*_tmp135A=({Cyc_yyget_YY4(&(yyyvsp[0]).v);});_tmpF2F->tl=_tmp135A;});_tmpF2F;});_tmpF2D(_tmpF2E);});
goto _LL0;case 562U: _LL45F: _LL460:
# 3433 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF30)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF31=({struct Cyc_Absyn_Exp*(*_tmpF32)(union Cyc_Absyn_Cnst,unsigned)=Cyc_Absyn_const_exp;union Cyc_Absyn_Cnst _tmpF33=({Cyc_yyget_Int_tok(&(yyyvsp[0]).v);});unsigned _tmpF34=(unsigned)((yyyvsp[0]).l).first_line;_tmpF32(_tmpF33,_tmpF34);});_tmpF30(_tmpF31);});
goto _LL0;case 563U: _LL461: _LL462:
# 3434 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF35)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF36=({struct Cyc_Absyn_Exp*(*_tmpF37)(char,unsigned)=Cyc_Absyn_char_exp;char _tmpF38=({Cyc_yyget_Char_tok(&(yyyvsp[0]).v);});unsigned _tmpF39=(unsigned)((yyyvsp[0]).l).first_line;_tmpF37(_tmpF38,_tmpF39);});_tmpF35(_tmpF36);});
goto _LL0;case 564U: _LL463: _LL464:
# 3435 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF3A)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF3B=({struct Cyc_Absyn_Exp*(*_tmpF3C)(struct _fat_ptr,unsigned)=Cyc_Absyn_wchar_exp;struct _fat_ptr _tmpF3D=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});unsigned _tmpF3E=(unsigned)((yyyvsp[0]).l).first_line;_tmpF3C(_tmpF3D,_tmpF3E);});_tmpF3A(_tmpF3B);});
goto _LL0;case 565U: _LL465: _LL466: {
# 3437 "parse.y"
struct _fat_ptr f=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});
int l=(int)({Cyc_strlen((struct _fat_ptr)f);});
int i=1;
if(l > 0){
char c=*((const char*)_check_fat_subscript(f,sizeof(char),l - 1));
if((int)c == (int)'f' ||(int)c == (int)'F')i=0;else{
if((int)c == (int)'l' ||(int)c == (int)'L')i=2;}}
# 3440
yyval=({union Cyc_YYSTYPE(*_tmpF3F)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF40=({Cyc_Absyn_float_exp(f,i,(unsigned)((yyyvsp[0]).l).first_line);});_tmpF3F(_tmpF40);});
# 3447
goto _LL0;}case 566U: _LL467: _LL468:
# 3448 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF41)(struct Cyc_Absyn_Exp*yy1)=Cyc_Exp_tok;struct Cyc_Absyn_Exp*_tmpF42=({Cyc_Absyn_null_exp((unsigned)((yyyvsp[0]).l).first_line);});_tmpF41(_tmpF42);});
goto _LL0;case 567U: _LL469: _LL46A:
# 3452 "parse.y"
 yyval=({union Cyc_YYSTYPE(*_tmpF43)(struct _tuple0*yy1)=Cyc_QualId_tok;struct _tuple0*_tmpF44=({struct _tuple0*_tmpF46=_cycalloc(sizeof(*_tmpF46));({union Cyc_Absyn_Nmspace _tmp135E=({Cyc_Absyn_Rel_n(0);});_tmpF46->f1=_tmp135E;}),({struct _fat_ptr*_tmp135D=({struct _fat_ptr*_tmpF45=_cycalloc(sizeof(*_tmpF45));({struct _fat_ptr _tmp135C=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmpF45=_tmp135C;});_tmpF45;});_tmpF46->f2=_tmp135D;});_tmpF46;});_tmpF43(_tmpF44);});
goto _LL0;case 568U: _LL46B: _LL46C:
# 3453 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 569U: _LL46D: _LL46E:
# 3456
 yyval=({union Cyc_YYSTYPE(*_tmpF47)(struct _tuple0*yy1)=Cyc_QualId_tok;struct _tuple0*_tmpF48=({struct _tuple0*_tmpF4A=_cycalloc(sizeof(*_tmpF4A));({union Cyc_Absyn_Nmspace _tmp1361=({Cyc_Absyn_Rel_n(0);});_tmpF4A->f1=_tmp1361;}),({struct _fat_ptr*_tmp1360=({struct _fat_ptr*_tmpF49=_cycalloc(sizeof(*_tmpF49));({struct _fat_ptr _tmp135F=({Cyc_yyget_String_tok(&(yyyvsp[0]).v);});*_tmpF49=_tmp135F;});_tmpF49;});_tmpF4A->f2=_tmp1360;});_tmpF4A;});_tmpF47(_tmpF48);});
goto _LL0;case 570U: _LL46F: _LL470:
# 3457 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 571U: _LL471: _LL472:
# 3462 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 572U: _LL473: _LL474:
# 3463 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 573U: _LL475: _LL476:
# 3466
 yyval=(yyyvsp[0]).v;
goto _LL0;case 574U: _LL477: _LL478:
# 3467 "parse.y"
 yyval=(yyyvsp[0]).v;
goto _LL0;case 575U: _LL479: _LL47A:
# 3472 "parse.y"
 goto _LL0;case 576U: _LL47B: _LL47C:
# 3472 "parse.y"
 yylex_buf->lex_curr_pos -=1;
goto _LL0;default: _LL47D: _LL47E:
# 3476
 goto _LL0;}_LL0:;}
# 375 "cycbison.simple"
yyvsp_offset -=yylen;
yyssp_offset -=yylen;
# 389 "cycbison.simple"
(*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),++ yyvsp_offset))).v=yyval;
# 392
if(yylen == 0){
struct Cyc_Yystacktype*p=(struct Cyc_Yystacktype*)_check_null(_untag_fat_ptr(_fat_ptr_plus(yyvs,sizeof(struct Cyc_Yystacktype),yyvsp_offset - 1),sizeof(struct Cyc_Yystacktype),2U));
((p[1]).l).first_line=yylloc.first_line;
((p[1]).l).first_column=yylloc.first_column;
((p[1]).l).last_line=((p[0]).l).last_line;
((p[1]).l).last_column=((p[0]).l).last_column;}else{
# 399
({int _tmp1362=((*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),(yyvsp_offset + yylen)- 1))).l).last_line;((*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),yyvsp_offset))).l).last_line=_tmp1362;});
((((struct Cyc_Yystacktype*)yyvs.curr)[yyvsp_offset]).l).last_column=((*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),(yyvsp_offset + yylen)- 1))).l).last_column;}
# 409
yyn=(int)Cyc_yyr1[yyn];
# 411
yystate=({int _tmp1363=(int)*((short*)_check_known_subscript_notnull(Cyc_yypgoto,168U,sizeof(short),yyn - 158));_tmp1363 + (int)*((short*)_check_fat_subscript(yyss,sizeof(short),yyssp_offset));});
if((yystate >= 0 && yystate <= 6937)&&(int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,6938U,sizeof(short),yystate))== (int)((short*)yyss.curr)[yyssp_offset])
yystate=(int)Cyc_yytable[yystate];else{
# 415
yystate=(int)*((short*)_check_known_subscript_notnull(Cyc_yydefgoto,168U,sizeof(short),yyn - 158));}
# 417
goto yynewstate;
# 419
yyerrlab:
# 421
 if(yyerrstatus == 0){
# 424
++ yynerrs;
# 427
yyn=(int)Cyc_yypact[yystate];
# 429
if(yyn > - 32768 && yyn < 6937){
# 431
int sze=0;
struct _fat_ptr msg;
int x;int count;
# 435
count=0;
# 437
for(x=yyn < 0?- yyn: 0;(unsigned)x < 326U / sizeof(char*);++ x){
# 439
if((int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,6938U,sizeof(short),x + yyn))== x)
({unsigned long _tmp1364=({Cyc_strlen((struct _fat_ptr)*((struct _fat_ptr*)_check_known_subscript_notnull(Cyc_yytname,326U,sizeof(struct _fat_ptr),x)));})+ (unsigned long)15;sze +=_tmp1364;}),count ++;}
# 437
msg=({unsigned _tmpF4C=(unsigned)(sze + 15)+ 1U;char*_tmpF4B=({struct _RegionHandle*_tmp1365=yyregion;_region_malloc(_tmp1365,_check_times(_tmpF4C,sizeof(char)));});({{unsigned _tmp101B=(unsigned)(sze + 15);unsigned i;for(i=0;i < _tmp101B;++ i){_tmpF4B[i]='\000';}_tmpF4B[_tmp101B]=0;}0;});_tag_fat(_tmpF4B,sizeof(char),_tmpF4C);});
# 442
({({struct _fat_ptr _tmp1366=msg;Cyc_strcpy(_tmp1366,({const char*_tmpF4D="parse error";_tag_fat(_tmpF4D,sizeof(char),12U);}));});});
# 444
if(count < 5){
# 446
count=0;
for(x=yyn < 0?- yyn: 0;(unsigned)x < 326U / sizeof(char*);++ x){
# 449
if((int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,6938U,sizeof(short),x + yyn))== x){
# 451
({({struct _fat_ptr _tmp1367=msg;Cyc_strcat(_tmp1367,(struct _fat_ptr)(count == 0?(struct _fat_ptr)({const char*_tmpF4E=", expecting `";_tag_fat(_tmpF4E,sizeof(char),14U);}):(struct _fat_ptr)({const char*_tmpF4F=" or `";_tag_fat(_tmpF4F,sizeof(char),6U);})));});});
# 454
({Cyc_strcat(msg,(struct _fat_ptr)*((struct _fat_ptr*)_check_known_subscript_notnull(Cyc_yytname,326U,sizeof(struct _fat_ptr),x)));});
({({struct _fat_ptr _tmp1368=msg;Cyc_strcat(_tmp1368,({const char*_tmpF50="'";_tag_fat(_tmpF50,sizeof(char),2U);}));});});
++ count;}}}
# 444
({Cyc_yyerror((struct _fat_ptr)msg,yystate,yychar);});}else{
# 463
({({struct _fat_ptr _tmp136A=({const char*_tmpF51="parse error";_tag_fat(_tmpF51,sizeof(char),12U);});int _tmp1369=yystate;Cyc_yyerror(_tmp136A,_tmp1369,yychar);});});}}
# 419
goto yyerrlab1;
# 467
yyerrlab1:
# 469
 if(yyerrstatus == 3){
# 474
if(yychar == 0){
int _tmpF52=1;_npop_handler(0U);return _tmpF52;}
# 474
yychar=-2;}
# 467
yyerrstatus=3;
# 491
goto yyerrhandle;
# 493
yyerrdefault:
# 503 "cycbison.simple"
 yyerrpop:
# 505
 if(yyssp_offset == 0){int _tmpF53=1;_npop_handler(0U);return _tmpF53;}
# 493 "cycbison.simple"
-- yyvsp_offset;
# 507 "cycbison.simple"
yystate=(int)*((short*)_check_fat_subscript(yyss,sizeof(short),-- yyssp_offset));
# 521 "cycbison.simple"
yyerrhandle:
 yyn=(int)*((short*)_check_known_subscript_notnull(Cyc_yypact,1198U,sizeof(short),yystate));
if(yyn == -32768)goto yyerrdefault;yyn +=1;
# 526
if((yyn < 0 || yyn > 6937)||(int)*((short*)_check_known_subscript_notnull(Cyc_yycheck,6938U,sizeof(short),yyn))!= 1)goto yyerrdefault;yyn=(int)Cyc_yytable[yyn];
# 529
if(yyn < 0){
# 531
if(yyn == -32768)goto yyerrpop;yyn=- yyn;
# 533
goto yyreduce;}else{
# 535
if(yyn == 0)goto yyerrpop;}
# 529
if(yyn == 1197){
# 538
int _tmpF54=0;_npop_handler(0U);return _tmpF54;}
# 529
({struct Cyc_Yystacktype _tmp136B=({struct Cyc_Yystacktype _tmp101C;_tmp101C.v=yylval,_tmp101C.l=yylloc;_tmp101C;});*((struct Cyc_Yystacktype*)_check_fat_subscript(yyvs,sizeof(struct Cyc_Yystacktype),++ yyvsp_offset))=_tmp136B;});
# 551
goto yynewstate;}
# 149 "cycbison.simple"
;_pop_region();}
# 3475 "parse.y"
void Cyc_yyprint(int i,union Cyc_YYSTYPE v){
union Cyc_YYSTYPE _tmpF59=v;struct Cyc_Absyn_Stmt*_tmpF5A;struct Cyc_Absyn_Exp*_tmpF5B;struct _tuple0*_tmpF5C;struct _fat_ptr _tmpF5D;char _tmpF5E;union Cyc_Absyn_Cnst _tmpF5F;switch((_tmpF59.Stmt_tok).tag){case 1U: _LL1: _tmpF5F=(_tmpF59.Int_tok).val;_LL2: {union Cyc_Absyn_Cnst c=_tmpF5F;
({struct Cyc_String_pa_PrintArg_struct _tmpF62=({struct Cyc_String_pa_PrintArg_struct _tmp101F;_tmp101F.tag=0U,({struct _fat_ptr _tmp136C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_cnst2string(c);}));_tmp101F.f1=_tmp136C;});_tmp101F;});void*_tmpF60[1U];_tmpF60[0]=& _tmpF62;({struct Cyc___cycFILE*_tmp136E=Cyc_stderr;struct _fat_ptr _tmp136D=({const char*_tmpF61="%s";_tag_fat(_tmpF61,sizeof(char),3U);});Cyc_fprintf(_tmp136E,_tmp136D,_tag_fat(_tmpF60,sizeof(void*),1U));});});goto _LL0;}case 2U: _LL3: _tmpF5E=(_tmpF59.Char_tok).val;_LL4: {char c=_tmpF5E;
({struct Cyc_Int_pa_PrintArg_struct _tmpF65=({struct Cyc_Int_pa_PrintArg_struct _tmp1020;_tmp1020.tag=1U,_tmp1020.f1=(unsigned long)((int)c);_tmp1020;});void*_tmpF63[1U];_tmpF63[0]=& _tmpF65;({struct Cyc___cycFILE*_tmp1370=Cyc_stderr;struct _fat_ptr _tmp136F=({const char*_tmpF64="%c";_tag_fat(_tmpF64,sizeof(char),3U);});Cyc_fprintf(_tmp1370,_tmp136F,_tag_fat(_tmpF63,sizeof(void*),1U));});});goto _LL0;}case 3U: _LL5: _tmpF5D=(_tmpF59.String_tok).val;_LL6: {struct _fat_ptr s=_tmpF5D;
({struct Cyc_String_pa_PrintArg_struct _tmpF68=({struct Cyc_String_pa_PrintArg_struct _tmp1021;_tmp1021.tag=0U,_tmp1021.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp1021;});void*_tmpF66[1U];_tmpF66[0]=& _tmpF68;({struct Cyc___cycFILE*_tmp1372=Cyc_stderr;struct _fat_ptr _tmp1371=({const char*_tmpF67="\"%s\"";_tag_fat(_tmpF67,sizeof(char),5U);});Cyc_fprintf(_tmp1372,_tmp1371,_tag_fat(_tmpF66,sizeof(void*),1U));});});goto _LL0;}case 5U: _LL7: _tmpF5C=(_tmpF59.QualId_tok).val;_LL8: {struct _tuple0*q=_tmpF5C;
({struct Cyc_String_pa_PrintArg_struct _tmpF6B=({struct Cyc_String_pa_PrintArg_struct _tmp1022;_tmp1022.tag=0U,({struct _fat_ptr _tmp1373=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(q);}));_tmp1022.f1=_tmp1373;});_tmp1022;});void*_tmpF69[1U];_tmpF69[0]=& _tmpF6B;({struct Cyc___cycFILE*_tmp1375=Cyc_stderr;struct _fat_ptr _tmp1374=({const char*_tmpF6A="%s";_tag_fat(_tmpF6A,sizeof(char),3U);});Cyc_fprintf(_tmp1375,_tmp1374,_tag_fat(_tmpF69,sizeof(void*),1U));});});goto _LL0;}case 7U: _LL9: _tmpF5B=(_tmpF59.Exp_tok).val;_LLA: {struct Cyc_Absyn_Exp*e=_tmpF5B;
({struct Cyc_String_pa_PrintArg_struct _tmpF6E=({struct Cyc_String_pa_PrintArg_struct _tmp1023;_tmp1023.tag=0U,({struct _fat_ptr _tmp1376=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp1023.f1=_tmp1376;});_tmp1023;});void*_tmpF6C[1U];_tmpF6C[0]=& _tmpF6E;({struct Cyc___cycFILE*_tmp1378=Cyc_stderr;struct _fat_ptr _tmp1377=({const char*_tmpF6D="%s";_tag_fat(_tmpF6D,sizeof(char),3U);});Cyc_fprintf(_tmp1378,_tmp1377,_tag_fat(_tmpF6C,sizeof(void*),1U));});});goto _LL0;}case 8U: _LLB: _tmpF5A=(_tmpF59.Stmt_tok).val;_LLC: {struct Cyc_Absyn_Stmt*s=_tmpF5A;
({struct Cyc_String_pa_PrintArg_struct _tmpF71=({struct Cyc_String_pa_PrintArg_struct _tmp1024;_tmp1024.tag=0U,({struct _fat_ptr _tmp1379=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_stmt2string(s);}));_tmp1024.f1=_tmp1379;});_tmp1024;});void*_tmpF6F[1U];_tmpF6F[0]=& _tmpF71;({struct Cyc___cycFILE*_tmp137B=Cyc_stderr;struct _fat_ptr _tmp137A=({const char*_tmpF70="%s";_tag_fat(_tmpF70,sizeof(char),3U);});Cyc_fprintf(_tmp137B,_tmp137A,_tag_fat(_tmpF6F,sizeof(void*),1U));});});goto _LL0;}default: _LLD: _LLE:
({void*_tmpF72=0U;({struct Cyc___cycFILE*_tmp137D=Cyc_stderr;struct _fat_ptr _tmp137C=({const char*_tmpF73="?";_tag_fat(_tmpF73,sizeof(char),2U);});Cyc_fprintf(_tmp137D,_tmp137C,_tag_fat(_tmpF72,sizeof(void*),0U));});});goto _LL0;}_LL0:;}
# 3475
struct _fat_ptr Cyc_token2string(int token){
# 3488
if(token <= 0)
return({const char*_tmpF75="end-of-file";_tag_fat(_tmpF75,sizeof(char),12U);});
# 3488
if(token == 373)
# 3491
return Cyc_Lex_token_string;else{
if(token == 382)
return({Cyc_Absynpp_qvar2string(Cyc_Lex_token_qvar);});}{
# 3488
int z=
# 3494
token > 0 && token <= 385?(int)*((short*)_check_known_subscript_notnull(Cyc_yytranslate,386U,sizeof(short),token)): 326;
if((unsigned)z < 326U)
return Cyc_yytname[z];else{
return _tag_fat(0,0,0);}}}
# 3502
struct _fat_ptr Cyc_Parse_filename={(void*)0,(void*)0,(void*)(0 + 0)};
# 3504
void Cyc_Parse_die(){
# 3506
({struct Cyc_String_pa_PrintArg_struct _tmpF79=({struct Cyc_String_pa_PrintArg_struct _tmp1026;_tmp1026.tag=0U,_tmp1026.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_Parse_filename);_tmp1026;});struct Cyc_Int_pa_PrintArg_struct _tmpF7A=({struct Cyc_Int_pa_PrintArg_struct _tmp1025;_tmp1025.tag=1U,_tmp1025.f1=(unsigned long)Cyc_yylloc.first_line;_tmp1025;});void*_tmpF77[2U];_tmpF77[0]=& _tmpF79,_tmpF77[1]=& _tmpF7A;({struct Cyc___cycFILE*_tmp137F=Cyc_stderr;struct _fat_ptr _tmp137E=({const char*_tmpF78="\nBefore die! File name : %s Line : %d\n";_tag_fat(_tmpF78,sizeof(char),39U);});Cyc_fprintf(_tmp137F,_tmp137E,_tag_fat(_tmpF77,sizeof(void*),2U));});});}
# 3510
struct Cyc_List_List*Cyc_Parse_parse_file(struct Cyc___cycFILE*f,struct _fat_ptr name){
Cyc_Parse_parse_result=0;{
struct _RegionHandle _tmpF7C=_new_region("yyr");struct _RegionHandle*yyr=& _tmpF7C;_push_region(yyr);
Cyc_Parse_filename=name;
# 3515
({int(*_tmpF7D)(struct _RegionHandle*yyr,struct Cyc_Lexing_lexbuf*yylex_buf)=Cyc_yyparse;struct _RegionHandle*_tmpF7E=yyr;struct Cyc_Lexing_lexbuf*_tmpF7F=({Cyc_Lexing_from_file(f);});_tmpF7D(_tmpF7E,_tmpF7F);});{
struct Cyc_List_List*_tmpF80=Cyc_Parse_parse_result;_npop_handler(0U);return _tmpF80;}
# 3513
;_pop_region();}}
