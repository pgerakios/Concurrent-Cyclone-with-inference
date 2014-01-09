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
 struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 165 "core.h"
extern struct _RegionHandle*Cyc_Core_heap_region;
# 168
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};
# 275 "core.h"
void Cyc_Core_rethrow(void*);
# 281
const char*Cyc_Core_get_exn_filename();
# 288
int Cyc_Core_get_exn_lineno();struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};
# 38 "cycboot.h"
int Cyc_open(const char*,int,struct _fat_ptr);struct Cyc___cycFILE;
# 51
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
# 224 "cycboot.h"
int Cyc_vfprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 300 "cycboot.h"
int isspace(int);
# 310
int toupper(int);
# 318
int system(const char*);
void exit(int);
# 323
int mkdir(const char*pathname,unsigned short mode);
# 326
int close(int);
int chdir(const char*);
struct _fat_ptr Cyc_getcwd(struct _fat_ptr buf,unsigned long size);extern char Cyc_Lexing_Error[6U];struct Cyc_Lexing_Error_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Lexing_lexbuf{void(*refill_buff)(struct Cyc_Lexing_lexbuf*);void*refill_state;struct _fat_ptr lex_buffer;int lex_buffer_len;int lex_abs_pos;int lex_start_pos;int lex_curr_pos;int lex_last_pos;int lex_last_action;int lex_eof_reached;};struct Cyc_Lexing_function_lexbuf_state{int(*read_fun)(struct _fat_ptr,int,void*);void*read_fun_state;};struct Cyc_Lexing_lex_tables{struct _fat_ptr lex_base;struct _fat_ptr lex_backtrk;struct _fat_ptr lex_default;struct _fat_ptr lex_trans;struct _fat_ptr lex_check;};
# 78 "lexing.h"
struct Cyc_Lexing_lexbuf*Cyc_Lexing_from_file(struct Cyc___cycFILE*);
# 82
struct _fat_ptr Cyc_Lexing_lexeme(struct Cyc_Lexing_lexbuf*);
char Cyc_Lexing_lexeme_char(struct Cyc_Lexing_lexbuf*,int);
int Cyc_Lexing_lexeme_start(struct Cyc_Lexing_lexbuf*);
int Cyc_Lexing_lexeme_end(struct Cyc_Lexing_lexbuf*);struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 76
struct Cyc_List_List*Cyc_List_map(void*(*f)(void*),struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct _tuple0{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 294
struct _tuple0 Cyc_List_split(struct Cyc_List_List*x);
# 322
int Cyc_List_mem(int(*compare)(void*,void*),struct Cyc_List_List*l,void*x);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};
# 37 "iter.h"
int Cyc_Iter_next(struct Cyc_Iter_Iter,void*);struct Cyc_Set_Set;
# 51 "set.h"
struct Cyc_Set_Set*Cyc_Set_empty(int(*cmp)(void*,void*));
# 63
struct Cyc_Set_Set*Cyc_Set_insert(struct Cyc_Set_Set*s,void*elt);
# 75
struct Cyc_Set_Set*Cyc_Set_union_two(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2);
# 82
struct Cyc_Set_Set*Cyc_Set_diff(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2);
# 85
struct Cyc_Set_Set*Cyc_Set_delete(struct Cyc_Set_Set*s,void*elt);
# 94
int Cyc_Set_cardinality(struct Cyc_Set_Set*s);
# 100
int Cyc_Set_member(struct Cyc_Set_Set*s,void*elt);extern char Cyc_Set_Absent[7U];struct Cyc_Set_Absent_exn_struct{char*tag;};
# 141
struct Cyc_Iter_Iter Cyc_Set_make_iter(struct _RegionHandle*rgn,struct Cyc_Set_Set*s);
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);
int Cyc_strncmp(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long len);
# 62
struct _fat_ptr Cyc_strconcat(struct _fat_ptr,struct _fat_ptr);
# 64
struct _fat_ptr Cyc_strconcat_l(struct Cyc_List_List*);
# 66
struct _fat_ptr Cyc_str_sepstr(struct Cyc_List_List*,struct _fat_ptr);
# 104 "string.h"
struct _fat_ptr Cyc_strdup(struct _fat_ptr src);
# 109
struct _fat_ptr Cyc_substring(struct _fat_ptr,int ofs,unsigned long n);struct Cyc_Hashtable_Table;
# 39 "hashtable.h"
struct Cyc_Hashtable_Table*Cyc_Hashtable_create(int sz,int(*cmp)(void*,void*),int(*hash)(void*));
# 50
void Cyc_Hashtable_insert(struct Cyc_Hashtable_Table*t,void*key,void*val);
# 52
void*Cyc_Hashtable_lookup(struct Cyc_Hashtable_Table*t,void*key);
# 78
int Cyc_Hashtable_hash_stringptr(struct _fat_ptr*p);
# 30 "filename.h"
struct _fat_ptr Cyc_Filename_concat(struct _fat_ptr,struct _fat_ptr);
# 34
struct _fat_ptr Cyc_Filename_chop_extension(struct _fat_ptr);
# 40
struct _fat_ptr Cyc_Filename_dirname(struct _fat_ptr);
# 43
struct _fat_ptr Cyc_Filename_basename(struct _fat_ptr);extern char Cyc_Arg_Bad[4U];struct Cyc_Arg_Bad_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Arg_Error[6U];struct Cyc_Arg_Error_exn_struct{char*tag;};struct Cyc_Arg_Unit_spec_Arg_Spec_struct{int tag;void(*f1)();};struct Cyc_Arg_Flag_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_FlagString_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr,struct _fat_ptr);};struct Cyc_Arg_Set_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_Clear_spec_Arg_Spec_struct{int tag;int*f1;};struct Cyc_Arg_String_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};struct Cyc_Arg_Int_spec_Arg_Spec_struct{int tag;void(*f1)(int);};struct Cyc_Arg_Rest_spec_Arg_Spec_struct{int tag;void(*f1)(struct _fat_ptr);};
# 66 "arg.h"
void Cyc_Arg_usage(struct Cyc_List_List*,struct _fat_ptr);
# 69
extern int Cyc_Arg_current;
# 71
void Cyc_Arg_parse(struct Cyc_List_List*specs,void(*anonfun)(struct _fat_ptr),int(*anonflagfun)(struct _fat_ptr),struct _fat_ptr errmsg,struct _fat_ptr args);struct Cyc_Buffer_t;
# 50 "buffer.h"
struct Cyc_Buffer_t*Cyc_Buffer_create(unsigned n);
# 58
struct _fat_ptr Cyc_Buffer_contents(struct Cyc_Buffer_t*);
# 81
void Cyc_Buffer_add_char(struct Cyc_Buffer_t*,char);
# 92 "buffer.h"
void Cyc_Buffer_add_string(struct Cyc_Buffer_t*,struct _fat_ptr);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;
# 28 "position.h"
void Cyc_Position_reset_position(struct _fat_ptr);struct Cyc_Position_Error;struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple1{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple0*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple0*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple1*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple1*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple1*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple1*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple1*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple1*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple1*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 1153 "absyn.h"
struct Cyc_Absyn_Decl*Cyc_Absyn_new_decl(void*,unsigned);
# 1209
struct Cyc_Absyn_Decl*Cyc_Absyn_lookup_decl(struct Cyc_List_List*decls,struct _fat_ptr*name);struct _tuple12{enum Cyc_Absyn_AggrKind f1;struct _tuple1*f2;};
# 1223
struct _tuple12 Cyc_Absyn_aggr_kinded_name(union Cyc_Absyn_AggrInfo);
# 1231
struct _tuple1*Cyc_Absyn_binding2qvar(void*);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 53 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);
# 55
extern struct Cyc_Absynpp_Params Cyc_Absynpp_cyc_params_r;
# 57
void Cyc_Absynpp_decllist2file(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f);
# 62
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 28 "parse.h"
struct Cyc_List_List*Cyc_Parse_parse_file(struct Cyc___cycFILE*f,struct _fat_ptr name);extern char Cyc_Parse_Exit[5U];struct Cyc_Parse_Exit_exn_struct{char*tag;};struct Cyc_FlatList{struct Cyc_FlatList*tl;void*hd[0U] __attribute__((aligned )) ;};struct Cyc_Type_specifier{int Signed_spec: 1;int Unsigned_spec: 1;int Short_spec: 1;int Long_spec: 1;int Long_Long_spec: 1;int Valid_type_spec: 1;void*Type_spec;unsigned loc;};struct Cyc_Declarator{struct _tuple1*id;struct Cyc_List_List*tms;};struct _tuple14{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;};struct _tuple13{struct _tuple13*tl;struct _tuple14 hd  __attribute__((aligned )) ;};
# 52
enum Cyc_Storage_class{Cyc_Typedef_sc =0U,Cyc_Extern_sc =1U,Cyc_ExternC_sc =2U,Cyc_Static_sc =3U,Cyc_Auto_sc =4U,Cyc_Register_sc =5U,Cyc_Abstract_sc =6U};struct Cyc_Declaration_spec{enum Cyc_Storage_class*sc;struct Cyc_Absyn_Tqual tq;struct Cyc_Type_specifier type_specs;int is_inline;struct Cyc_List_List*attributes;};struct Cyc_Abstractdeclarator{struct Cyc_List_List*tms;};struct Cyc_Numelts_ptrqual_Pointer_qual_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Region_ptrqual_Pointer_qual_struct{int tag;void*f1;};struct Cyc_Thin_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Fat_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Zeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nozeroterm_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Notnull_ptrqual_Pointer_qual_struct{int tag;};struct Cyc_Nullable_ptrqual_Pointer_qual_struct{int tag;};struct _union_YYSTYPE_Int_tok{int tag;union Cyc_Absyn_Cnst val;};struct _union_YYSTYPE_Char_tok{int tag;char val;};struct _union_YYSTYPE_String_tok{int tag;struct _fat_ptr val;};struct _union_YYSTYPE_Stringopt_tok{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_QualId_tok{int tag;struct _tuple1*val;};struct _tuple15{int f1;struct _fat_ptr f2;};struct _union_YYSTYPE_Asm_tok{int tag;struct _tuple15 val;};struct _union_YYSTYPE_Exp_tok{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_Stmt_tok{int tag;struct Cyc_Absyn_Stmt*val;};struct _tuple16{unsigned f1;void*f2;void*f3;};struct _union_YYSTYPE_YY1{int tag;struct _tuple16*val;};struct _union_YYSTYPE_YY2{int tag;void*val;};struct _union_YYSTYPE_YY3{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY4{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY5{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY6{int tag;enum Cyc_Absyn_Primop val;};struct _union_YYSTYPE_YY7{int tag;struct Cyc_Core_Opt*val;};struct _union_YYSTYPE_YY8{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY9{int tag;struct Cyc_Absyn_Pat*val;};struct _tuple17{struct Cyc_List_List*f1;int f2;};struct _union_YYSTYPE_YY10{int tag;struct _tuple17*val;};struct _union_YYSTYPE_YY11{int tag;struct Cyc_List_List*val;};struct _tuple18{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};struct _union_YYSTYPE_YY12{int tag;struct _tuple18*val;};struct _union_YYSTYPE_YY13{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY14{int tag;struct _tuple17*val;};struct _union_YYSTYPE_YY15{int tag;struct Cyc_Absyn_Fndecl*val;};struct _union_YYSTYPE_YY16{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY17{int tag;struct Cyc_Declaration_spec val;};struct _union_YYSTYPE_YY18{int tag;struct _tuple14 val;};struct _union_YYSTYPE_YY19{int tag;struct _tuple13*val;};struct _union_YYSTYPE_YY20{int tag;enum Cyc_Storage_class*val;};struct _union_YYSTYPE_YY21{int tag;struct Cyc_Type_specifier val;};struct _union_YYSTYPE_YY22{int tag;enum Cyc_Absyn_AggrKind val;};struct _union_YYSTYPE_YY23{int tag;struct Cyc_Absyn_Tqual val;};struct _union_YYSTYPE_YY24{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY25{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY26{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY27{int tag;struct Cyc_Declarator val;};struct _tuple19{struct Cyc_Declarator f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct _union_YYSTYPE_YY28{int tag;struct _tuple19*val;};struct _union_YYSTYPE_YY29{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY30{int tag;struct Cyc_Abstractdeclarator val;};struct _union_YYSTYPE_YY31{int tag;int val;};struct _union_YYSTYPE_YY32{int tag;enum Cyc_Absyn_Scope val;};struct _union_YYSTYPE_YY33{int tag;struct Cyc_Absyn_Datatypefield*val;};struct _union_YYSTYPE_YY34{int tag;struct Cyc_List_List*val;};struct _tuple20{struct Cyc_Absyn_Tqual f1;struct Cyc_Type_specifier f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY35{int tag;struct _tuple20 val;};struct _union_YYSTYPE_YY36{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY37{int tag;struct _tuple9*val;};struct _union_YYSTYPE_YY38{int tag;struct Cyc_List_List*val;};struct _tuple21{struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;};struct _union_YYSTYPE_YY39{int tag;struct _tuple21*val;};struct _union_YYSTYPE_YY40{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY41{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY42{int tag;void*val;};struct _union_YYSTYPE_YY43{int tag;struct Cyc_Absyn_Kind*val;};struct _union_YYSTYPE_YY44{int tag;void*val;};struct _union_YYSTYPE_YY45{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY46{int tag;void*val;};struct _union_YYSTYPE_YY47{int tag;struct Cyc_Absyn_Enumfield*val;};struct _union_YYSTYPE_YY48{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY49{int tag;void*val;};struct _union_YYSTYPE_YY50{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY51{int tag;void*val;};struct _union_YYSTYPE_YY52{int tag;struct Cyc_List_List*val;};struct _tuple22{struct Cyc_List_List*f1;unsigned f2;};struct _union_YYSTYPE_YY53{int tag;struct _tuple22*val;};struct _union_YYSTYPE_YY54{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY55{int tag;void*val;};struct _union_YYSTYPE_YY56{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY57{int tag;struct Cyc_Absyn_Exp*val;};struct _union_YYSTYPE_YY58{int tag;void*val;};struct _tuple23{struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};struct _union_YYSTYPE_YY59{int tag;struct _tuple23*val;};struct _union_YYSTYPE_YY60{int tag;struct _tuple0*val;};struct _union_YYSTYPE_YY61{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY62{int tag;struct Cyc_List_List*val;};struct _tuple24{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};struct _union_YYSTYPE_YY63{int tag;struct _tuple24*val;};struct _union_YYSTYPE_YY64{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YY65{int tag;int val;};struct _union_YYSTYPE_YY66{int tag;struct Cyc_List_List*val;};struct _union_YYSTYPE_YYINITIALSVAL{int tag;int val;};union Cyc_YYSTYPE{struct _union_YYSTYPE_Int_tok Int_tok;struct _union_YYSTYPE_Char_tok Char_tok;struct _union_YYSTYPE_String_tok String_tok;struct _union_YYSTYPE_Stringopt_tok Stringopt_tok;struct _union_YYSTYPE_QualId_tok QualId_tok;struct _union_YYSTYPE_Asm_tok Asm_tok;struct _union_YYSTYPE_Exp_tok Exp_tok;struct _union_YYSTYPE_Stmt_tok Stmt_tok;struct _union_YYSTYPE_YY1 YY1;struct _union_YYSTYPE_YY2 YY2;struct _union_YYSTYPE_YY3 YY3;struct _union_YYSTYPE_YY4 YY4;struct _union_YYSTYPE_YY5 YY5;struct _union_YYSTYPE_YY6 YY6;struct _union_YYSTYPE_YY7 YY7;struct _union_YYSTYPE_YY8 YY8;struct _union_YYSTYPE_YY9 YY9;struct _union_YYSTYPE_YY10 YY10;struct _union_YYSTYPE_YY11 YY11;struct _union_YYSTYPE_YY12 YY12;struct _union_YYSTYPE_YY13 YY13;struct _union_YYSTYPE_YY14 YY14;struct _union_YYSTYPE_YY15 YY15;struct _union_YYSTYPE_YY16 YY16;struct _union_YYSTYPE_YY17 YY17;struct _union_YYSTYPE_YY18 YY18;struct _union_YYSTYPE_YY19 YY19;struct _union_YYSTYPE_YY20 YY20;struct _union_YYSTYPE_YY21 YY21;struct _union_YYSTYPE_YY22 YY22;struct _union_YYSTYPE_YY23 YY23;struct _union_YYSTYPE_YY24 YY24;struct _union_YYSTYPE_YY25 YY25;struct _union_YYSTYPE_YY26 YY26;struct _union_YYSTYPE_YY27 YY27;struct _union_YYSTYPE_YY28 YY28;struct _union_YYSTYPE_YY29 YY29;struct _union_YYSTYPE_YY30 YY30;struct _union_YYSTYPE_YY31 YY31;struct _union_YYSTYPE_YY32 YY32;struct _union_YYSTYPE_YY33 YY33;struct _union_YYSTYPE_YY34 YY34;struct _union_YYSTYPE_YY35 YY35;struct _union_YYSTYPE_YY36 YY36;struct _union_YYSTYPE_YY37 YY37;struct _union_YYSTYPE_YY38 YY38;struct _union_YYSTYPE_YY39 YY39;struct _union_YYSTYPE_YY40 YY40;struct _union_YYSTYPE_YY41 YY41;struct _union_YYSTYPE_YY42 YY42;struct _union_YYSTYPE_YY43 YY43;struct _union_YYSTYPE_YY44 YY44;struct _union_YYSTYPE_YY45 YY45;struct _union_YYSTYPE_YY46 YY46;struct _union_YYSTYPE_YY47 YY47;struct _union_YYSTYPE_YY48 YY48;struct _union_YYSTYPE_YY49 YY49;struct _union_YYSTYPE_YY50 YY50;struct _union_YYSTYPE_YY51 YY51;struct _union_YYSTYPE_YY52 YY52;struct _union_YYSTYPE_YY53 YY53;struct _union_YYSTYPE_YY54 YY54;struct _union_YYSTYPE_YY55 YY55;struct _union_YYSTYPE_YY56 YY56;struct _union_YYSTYPE_YY57 YY57;struct _union_YYSTYPE_YY58 YY58;struct _union_YYSTYPE_YY59 YY59;struct _union_YYSTYPE_YY60 YY60;struct _union_YYSTYPE_YY61 YY61;struct _union_YYSTYPE_YY62 YY62;struct _union_YYSTYPE_YY63 YY63;struct _union_YYSTYPE_YY64 YY64;struct _union_YYSTYPE_YY65 YY65;struct _union_YYSTYPE_YY66 YY66;struct _union_YYSTYPE_YYINITIALSVAL YYINITIALSVAL;};struct Cyc_Yyltype{int timestamp;int first_line;int first_column;int last_line;int last_column;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 68 "tcenv.h"
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_tc_init();
# 90
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 33 "tc.h"
void Cyc_Tc_tc(struct Cyc_Tcenv_Tenv*te,int var_default_init,struct Cyc_List_List*ds,int);extern char Cyc_Tcdecl_Incompatible[13U];struct Cyc_Tcdecl_Incompatible_exn_struct{char*tag;};struct Cyc_Tcdecl_Xdatatypefielddecl{struct Cyc_Absyn_Datatypedecl*base;struct Cyc_Absyn_Datatypefield*field;};
# 32 "cifc.h"
void Cyc_Cifc_merge_sys_user_decl(unsigned,int is_buildlib,struct Cyc_Absyn_Decl*user_decl,struct Cyc_Absyn_Decl*c_decl);
# 30 "binding.h"
void Cyc_Binding_resolve_all(struct Cyc_List_List*tds);struct Cyc_Binding_Namespace_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_Using_Binding_NSDirective_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Binding_NSCtxt{struct Cyc_List_List*curr_ns;struct Cyc_List_List*availables;struct Cyc_Dict_Dict ns_data;};
# 29 "specsfile.h"
struct _fat_ptr Cyc_Specsfile_target_arch;
void Cyc_Specsfile_set_target_arch(struct _fat_ptr s);
struct Cyc_List_List*Cyc_Specsfile_cyclone_exec_path;
void Cyc_Specsfile_add_cyclone_exec_path(struct _fat_ptr s);
# 34
struct Cyc_List_List*Cyc_Specsfile_read_specs(struct _fat_ptr file);
# 36
struct _fat_ptr Cyc_Specsfile_get_spec(struct Cyc_List_List*specs,struct _fat_ptr spec_name);
struct Cyc_List_List*Cyc_Specsfile_cyclone_arch_path;
struct _fat_ptr Cyc_Specsfile_def_lib_path;
struct _fat_ptr Cyc_Specsfile_parse_b(struct Cyc_List_List*specs,void(*anonfun)(struct _fat_ptr),int(*anonflagfun)(struct _fat_ptr),struct _fat_ptr errmsg,struct _fat_ptr argv);
# 44
struct _fat_ptr Cyc_Specsfile_find_in_arch_path(struct _fat_ptr s);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};
# 80 "buildlib.cyl"
void Cyc_Lex_lex_init(int use_cyclone_keywords);static char _tmp0[4U]="gcc";
# 91
static struct _fat_ptr Cyc_cyclone_cc={_tmp0,_tmp0,_tmp0 + 4U};static char _tmp1[1U]="";
static struct _fat_ptr Cyc_target_cflags={_tmp1,_tmp1,_tmp1 + 1U};
# 94
static int Cyc_do_setjmp=0;
static int Cyc_verbose=0;
int Cyc_noexpand_r=1;
# 98
struct Cyc___cycFILE*Cyc_log_file=0;
struct Cyc___cycFILE*Cyc_cstubs_file=0;
struct Cyc___cycFILE*Cyc_cycstubs_file=0;
# 103
int Cyc_log(struct _fat_ptr fmt,struct _fat_ptr ap){
# 106
if(Cyc_log_file == 0){
({void*_tmp2=0U;({struct Cyc___cycFILE*_tmp498=Cyc_stderr;struct _fat_ptr _tmp497=({const char*_tmp3="Internal error: log file is NULL\n";_tag_fat(_tmp3,sizeof(char),34U);});Cyc_fprintf(_tmp498,_tmp497,_tag_fat(_tmp2,sizeof(void*),0U));});});
({exit(1);});}{
# 106
int x=({Cyc_vfprintf((struct Cyc___cycFILE*)_check_null(Cyc_log_file),fmt,ap);});
# 111
({Cyc_fflush((struct Cyc___cycFILE*)_check_null(Cyc_log_file));});
return x;}}
# 103
static struct _fat_ptr*Cyc_current_source=0;
# 116
static struct Cyc_List_List*Cyc_current_args=0;
static struct Cyc_Set_Set**Cyc_current_targets=0;
static void Cyc_add_target(struct _fat_ptr*sptr){
Cyc_current_targets=({struct Cyc_Set_Set**_tmp6=_cycalloc(sizeof(*_tmp6));({struct Cyc_Set_Set*_tmp49B=({({struct Cyc_Set_Set*(*_tmp49A)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({struct Cyc_Set_Set*(*_tmp5)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_insert;_tmp5;});struct Cyc_Set_Set*_tmp499=*((struct Cyc_Set_Set**)_check_null(Cyc_current_targets));_tmp49A(_tmp499,sptr);});});*_tmp6=_tmp49B;});_tmp6;});}struct _tuple25{struct _fat_ptr*f1;struct Cyc_Set_Set*f2;};
# 123
struct _tuple25*Cyc_line(struct Cyc_Lexing_lexbuf*);
int Cyc_macroname(struct Cyc_Lexing_lexbuf*);
int Cyc_args(struct Cyc_Lexing_lexbuf*);
int Cyc_token(struct Cyc_Lexing_lexbuf*);
int Cyc_string(struct Cyc_Lexing_lexbuf*);
# 129
struct Cyc___cycFILE*Cyc_slurp_out=0;
int Cyc_slurp(struct Cyc_Lexing_lexbuf*);
int Cyc_slurp_string(struct Cyc_Lexing_lexbuf*);
# 133
int Cyc_asm_string(struct Cyc_Lexing_lexbuf*);
int Cyc_asm_comment(struct Cyc_Lexing_lexbuf*);struct _tuple26{struct _fat_ptr f1;struct _fat_ptr*f2;};
# 137
struct _tuple26*Cyc_suck_line(struct Cyc_Lexing_lexbuf*);
int Cyc_suck_macroname(struct Cyc_Lexing_lexbuf*);
int Cyc_suck_restofline(struct Cyc_Lexing_lexbuf*);
struct _fat_ptr Cyc_current_line={(void*)0,(void*)0,(void*)(0 + 0)};struct _tuple27{struct _fat_ptr f1;struct _fat_ptr f2;};struct _tuple28{struct _fat_ptr*f1;struct _fat_ptr*f2;};struct _tuple29{struct _fat_ptr f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;struct Cyc_List_List*f6;struct Cyc_List_List*f7;struct Cyc_List_List*f8;};
# 153
struct _tuple29*Cyc_spec(struct Cyc_Lexing_lexbuf*);
int Cyc_commands(struct Cyc_Lexing_lexbuf*);
int Cyc_snarfsymbols(struct Cyc_Lexing_lexbuf*);
int Cyc_block(struct Cyc_Lexing_lexbuf*);
int Cyc_block_string(struct Cyc_Lexing_lexbuf*);
int Cyc_block_comment(struct Cyc_Lexing_lexbuf*);
struct _fat_ptr Cyc_current_headerfile={(void*)0,(void*)0,(void*)(0 + 0)};
struct Cyc_List_List*Cyc_snarfed_symbols=0;
struct Cyc_List_List*Cyc_current_symbols=0;
struct Cyc_List_List*Cyc_current_user_defs=0;
struct Cyc_List_List*Cyc_current_cstubs=0;
struct Cyc_List_List*Cyc_current_cycstubs=0;
struct Cyc_List_List*Cyc_current_hstubs=0;
struct Cyc_List_List*Cyc_current_omit_symbols=0;
struct Cyc_List_List*Cyc_current_cpp=0;
struct Cyc_Buffer_t*Cyc_specbuf=0;
struct _fat_ptr Cyc_current_symbol={(void*)0,(void*)0,(void*)(0 + 0)};
int Cyc_rename_current_symbol=0;
int Cyc_braces_to_match=0;
int Cyc_parens_to_match=0;
# 174
int Cyc_numdef=0;
# 176
static struct Cyc_List_List*Cyc_cppargs=0;static char _tmp8[14U]="BUILDLIB_sym_";
# 178
struct _fat_ptr Cyc_user_prefix={_tmp8,_tmp8,_tmp8 + 14U};
static struct _fat_ptr*Cyc_add_user_prefix(struct _fat_ptr*symbol){
struct _fat_ptr s=(struct _fat_ptr)({Cyc_strconcat((struct _fat_ptr)Cyc_user_prefix,(struct _fat_ptr)*symbol);});
return({struct _fat_ptr*_tmp9=_cycalloc(sizeof(*_tmp9));*_tmp9=s;_tmp9;});}
# 183
static struct _fat_ptr Cyc_remove_user_prefix(struct _fat_ptr symbol){
unsigned prefix_len=({Cyc_strlen((struct _fat_ptr)Cyc_user_prefix);});
if(({Cyc_strncmp((struct _fat_ptr)symbol,(struct _fat_ptr)Cyc_user_prefix,prefix_len);})!= 0){
({struct Cyc_String_pa_PrintArg_struct _tmpD=({struct Cyc_String_pa_PrintArg_struct _tmp438;_tmp438.tag=0U,_tmp438.f1=(struct _fat_ptr)((struct _fat_ptr)symbol);_tmp438;});void*_tmpB[1U];_tmpB[0]=& _tmpD;({struct Cyc___cycFILE*_tmp49D=Cyc_stderr;struct _fat_ptr _tmp49C=({const char*_tmpC="Internal error: bad user type name %s\n";_tag_fat(_tmpC,sizeof(char),39U);});Cyc_fprintf(_tmp49D,_tmp49C,_tag_fat(_tmpB,sizeof(void*),1U));});});
return symbol;}
# 185
return(struct _fat_ptr)({struct _fat_ptr(*_tmpE)(struct _fat_ptr,int ofs,unsigned long n)=Cyc_substring;struct _fat_ptr _tmpF=(struct _fat_ptr)symbol;int _tmp10=(int)prefix_len;unsigned long _tmp11=({
# 189
unsigned long _tmp49E=({Cyc_strlen((struct _fat_ptr)symbol);});_tmp49E - prefix_len;});_tmpE(_tmpF,_tmp10,_tmp11);});}
# 192
static void Cyc_rename_decl(struct Cyc_Absyn_Decl*d){
void*_stmttmp0=d->r;void*_tmp13=_stmttmp0;struct Cyc_Absyn_Typedefdecl*_tmp14;struct Cyc_Absyn_Enumdecl*_tmp15;struct Cyc_Absyn_Aggrdecl*_tmp16;switch(*((int*)_tmp13)){case 5U: _LL1: _tmp16=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp13)->f1;_LL2: {struct Cyc_Absyn_Aggrdecl*ad=_tmp16;
# 195
({struct _fat_ptr*_tmp4A0=({struct _fat_ptr*_tmp17=_cycalloc(sizeof(*_tmp17));({struct _fat_ptr _tmp49F=({Cyc_remove_user_prefix(*(*ad->name).f2);});*_tmp17=_tmp49F;});_tmp17;});(*ad->name).f2=_tmp4A0;});
goto _LL0;}case 7U: _LL3: _tmp15=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp13)->f1;_LL4: {struct Cyc_Absyn_Enumdecl*ed=_tmp15;
# 198
({struct _fat_ptr*_tmp4A2=({struct _fat_ptr*_tmp18=_cycalloc(sizeof(*_tmp18));({struct _fat_ptr _tmp4A1=({Cyc_remove_user_prefix(*(*ed->name).f2);});*_tmp18=_tmp4A1;});_tmp18;});(*ed->name).f2=_tmp4A2;});
goto _LL0;}case 8U: _LL5: _tmp14=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp13)->f1;_LL6: {struct Cyc_Absyn_Typedefdecl*td=_tmp14;
# 201
({struct _fat_ptr*_tmp4A4=({struct _fat_ptr*_tmp19=_cycalloc(sizeof(*_tmp19));({struct _fat_ptr _tmp4A3=({Cyc_remove_user_prefix(*(*td->name).f2);});*_tmp19=_tmp4A3;});_tmp19;});(*td->name).f2=_tmp4A4;});
goto _LL0;}case 1U: _LL7: _LL8:
# 204
 goto _LLA;case 0U: _LL9: _LLA:
 goto _LLC;case 4U: _LLB: _LLC:
 goto _LLE;case 6U: _LLD: _LLE:
 goto _LL10;case 2U: _LLF: _LL10:
 goto _LL12;case 3U: _LL11: _LL12:
 goto _LL14;case 9U: _LL13: _LL14:
 goto _LL16;case 10U: _LL15: _LL16:
 goto _LL18;case 11U: _LL17: _LL18:
 goto _LL1A;case 12U: _LL19: _LL1A:
 goto _LL1C;case 13U: _LL1B: _LL1C:
 goto _LL1E;case 14U: _LL1D: _LL1E:
 goto _LL20;case 15U: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
# 218
({void*_tmp1A=0U;({struct Cyc___cycFILE*_tmp4A6=Cyc_stderr;struct _fat_ptr _tmp4A5=({const char*_tmp1B="Error in .cys file: bad user-defined type definition\n";_tag_fat(_tmp1B,sizeof(char),54U);});Cyc_fprintf(_tmp4A6,_tmp4A5,_tag_fat(_tmp1A,sizeof(void*),0U));});});
({exit(1);});}_LL0:;}
# 225
const int Cyc_lex_base[489U]={0,0,75,192,305,310,311,166,312,91,27,384,28,523,637,715,829,325,92,- 3,0,- 1,- 2,- 8,- 3,1,- 2,323,- 4,2,166,- 5,605,907,312,- 6,- 8,- 4,16,945,- 3,1021,29,11,1135,- 4,13,1250,223,- 14,317,12,- 2,216,20,27,29,34,50,49,71,55,66,101,101,100,108,95,370,386,112,104,105,123,126,375,399,112,134,190,381,1365,1480,414,205,213,233,219,224,224,245,498,1594,1709,- 9,525,- 10,233,253,507,1823,1938,- 7,654,684,- 11,432,514,516,2015,2092,2169,2250,434,465,531,2325,253,253,253,251,247,257,0,13,4,2406,5,628,2414,2479,660,- 4,- 3,49,467,6,2440,7,701,2502,2540,793,- 28,1162,1167,273,264,267,271,281,286,292,294,- 25,752,276,276,287,280,291,305,305,308,304,314,316,324,326,- 23,311,312,315,325,377,394,- 27,404,408,404,414,423,424,- 19,405,427,421,427,438,434,437,454,455,441,437,437,459,461,- 21,453,451,444,457,454,470,464,486,489,477,481,472,498,- 26,499,502,512,521,532,535,551,570,571,1141,4,592,573,584,577,577,594,599,602,584,600,617,5,20,- 16,622,617,608,609,612,630,619,615,621,637,621,636,629,624,648,627,635,641,646,648,664,665,23,51,- 18,651,652,656,666,667,681,683,1,53,823,53,765,708,708,716,743,820,799,759,760,788,785,764,766,821,824,827,774,775,1308,798,799,800,801,764,773,788,794,795,850,851,879,- 15,826,827,882,884,912,870,871,926,927,928,- 12,875,883,938,939,940,887,888,943,944,945,- 13,875,874,888,891,894,876,902,936,942,943,998,1000,- 17,1278,971,983,980,979,978,973,1002,1005,1006,1004,1018,1372,1038,1039,1038,1053,1482,1397,1056,1058,1048,1049,1047,1060,1596,1052,1053,1051,1064,1601,- 7,- 8,8,1260,2572,9,1449,2596,2634,1567,1280,- 49,1253,- 2,1103,- 4,1104,1132,1369,1105,1135,1222,1675,1107,2661,2704,1111,1166,1113,1136,2774,1118,1217,- 36,- 42,- 37,2849,1121,- 40,1134,- 45,- 39,- 48,2924,2953,1694,1192,1203,1789,2963,2993,1808,1904,3026,3057,3095,1307,1317,3165,3203,1309,1320,1409,1431,1423,1433,- 6,- 34,1183,3135,- 47,- 30,- 32,- 46,- 29,- 31,- 33,1192,3243,1229,1233,1923,1235,1236,1241,1247,1256,1257,1258,1269,1274,3316,3400,- 22,2469,1303,- 20,- 24,- 41,- 38,- 35,1391,3482,2,1297,1298,1305,3565,1342,15,1284,1285,1286,1285,1285,1297,4};
const int Cyc_lex_backtrk[489U]={- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,2,- 1,- 1,- 1,- 1,2,- 1,8,- 1,3,5,- 1,- 1,6,5,- 1,- 1,- 1,7,6,- 1,6,5,2,0,- 1,- 1,0,2,- 1,12,13,- 1,13,13,13,13,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,11,12,2,4,4,- 1,0,0,0,2,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,2,2,8,3,5,- 1,6,5,- 1,- 1,6,5,2,8,3,5,- 1,6,5,- 1,27,27,27,27,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,23,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,19,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,21,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,6,1,9,2,4,- 1,5,4,- 1,- 1,2,- 1,48,- 1,48,48,48,48,48,48,48,48,5,7,48,48,48,48,0,48,48,- 1,- 1,- 1,0,43,- 1,42,- 1,- 1,- 1,9,7,- 1,7,7,- 1,8,9,- 1,- 1,9,5,6,5,5,- 1,4,4,4,6,6,5,5,- 1,- 1,- 1,9,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,2,- 1,1,- 1,- 1,- 1,2,1,1,- 1,- 1,- 1,- 1,- 1,- 1,- 1};
const int Cyc_lex_default[489U]={- 1,- 1,- 1,383,372,143,23,102,23,19,- 1,- 1,12,31,49,35,36,23,19,0,- 1,0,0,0,0,- 1,0,- 1,0,- 1,- 1,0,- 1,- 1,- 1,0,0,0,- 1,- 1,0,- 1,42,- 1,- 1,0,- 1,- 1,- 1,0,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,0,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,0,106,- 1,- 1,- 1,- 1,- 1,113,113,113,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,- 1,135,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,272,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,0,- 1,0,- 1,- 1,449,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,0,- 1,- 1,0,- 1,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,0,- 1,- 1,0,0,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,- 1,- 1,0,0,0,0,0,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1};
const int Cyc_lex_trans[3822U]={0,0,0,0,0,0,0,0,0,0,22,19,28,481,19,28,19,28,36,19,48,48,46,46,48,22,46,0,0,0,0,0,21,271,21,482,21,22,- 1,- 1,22,- 1,- 1,48,224,46,236,22,479,479,479,479,479,479,479,479,479,479,31,106,22,237,117,42,261,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,31,262,272,274,479,135,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,474,474,474,474,474,474,474,474,474,474,124,20,77,22,70,57,58,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,59,60,61,62,474,63,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,64,65,132,384,385,384,384,385,133,22,66,67,68,71,72,134,34,34,34,34,34,34,34,34,73,74,384,386,387,75,78,388,389,390,48,48,391,392,48,393,394,395,396,397,397,397,397,397,397,397,397,397,398,79,399,400,401,48,19,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,19,- 1,- 1,403,402,80,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,373,404,104,374,144,144,24,24,144,136,125,107,107,84,97,107,85,87,28,88,24,29,86,25,375,89,90,144,91,22,26,26,21,21,107,98,99,145,118,119,120,121,122,123,26,35,35,35,35,35,35,35,35,155,148,149,30,30,30,30,30,30,30,30,69,69,150,151,69,76,76,152,153,76,154,81,81,213,199,81,69,69,376,193,69,184,177,69,137,126,146,170,76,76,76,163,164,76,81,165,166,167,27,69,168,31,169,21,83,83,147,171,83,172,173,174,76,116,116,116,116,116,116,116,116,116,116,- 1,32,- 1,- 1,83,- 1,22,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,- 1,175,- 1,- 1,116,- 1,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,92,92,28,176,92,178,179,180,181,100,100,182,183,100,185,19,107,107,115,115,107,186,115,92,187,108,108,95,95,108,35,95,100,115,115,188,189,115,190,107,191,115,192,36,194,195,196,197,108,198,95,200,201,202,94,203,115,204,205,21,21,21,109,110,109,109,109,109,109,109,109,109,109,109,21,206,207,208,209,210,211,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,212,214,215,216,109,217,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,50,50,96,218,50,219,220,33,33,33,33,33,33,33,33,33,33,103,103,221,222,103,238,50,33,33,33,33,33,33,131,131,131,131,131,131,131,131,51,227,103,225,- 1,228,- 1,229,226,104,104,230,52,104,231,232,233,234,33,33,33,33,33,33,35,35,35,35,35,35,35,35,104,235,328,275,263,- 1,239,- 1,43,43,243,244,43,245,246,240,241,247,248,249,53,250,242,251,252,54,55,253,254,255,256,43,56,142,142,142,142,142,142,142,142,257,258,259,260,264,265,44,44,44,44,44,44,44,44,44,44,266,267,268,269,36,270,28,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,94,105,276,277,44,278,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,279,290,45,35,35,35,35,35,35,35,35,156,157,158,280,159,282,283,284,160,285,286,281,287,288,37,273,289,161,162,49,291,292,322,317,311,306,298,38,39,39,39,39,39,39,39,39,39,39,299,300,301,302,303,304,21,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,305,307,308,309,39,310,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,40,105,22,33,33,33,33,33,33,33,33,33,33,312,313,314,315,316,318,31,33,33,33,33,33,33,319,320,321,96,323,324,325,326,327,329,330,331,332,333,334,41,41,41,41,41,41,41,41,41,41,335,33,33,33,33,33,33,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,336,337,338,339,41,340,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,354,41,41,41,41,41,41,41,41,41,41,- 1,349,345,346,347,348,21,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,102,350,351,352,41,353,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,46,46,355,356,46,357,222,222,358,367,222,362,363,364,365,366,368,369,370,371,212,472,448,46,439,414,470,359,359,222,154,359,341,341,446,407,341,223,410,47,47,47,47,47,47,47,47,47,47,471,359,413,447,143,409,341,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,411,176,412,442,47,469,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,22,46,46,360,473,46,473,473,342,443,468,382,382,361,183,382,198,262,343,36,405,36,105,344,46,444,445,473,316,341,341,382,382,341,382,382,21,327,49,305,47,47,47,47,47,47,47,47,47,47,237,36,341,36,382,340,21,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,406,465,477,478,47,28,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,22,81,81,293,342,81,- 1,294,353,353,21,483,353,484,343,485,295,486,296,35,35,31,487,31,81,488,0,473,0,473,473,353,0,359,359,- 1,0,359,0,19,82,82,82,82,82,82,82,82,82,82,473,35,35,31,297,31,359,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,0,0,0,0,82,450,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,31,83,83,358,358,83,102,358,360,381,381,381,381,381,381,381,381,0,361,102,438,438,0,0,83,0,358,0,0,0,0,0,0,0,31,0,0,0,102,0,82,82,82,82,82,82,82,82,82,82,0,102,438,438,0,0,0,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,0,0,0,0,82,0,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,92,92,366,366,92,0,366,371,371,0,0,371,31,31,31,31,31,31,31,31,0,0,- 1,92,0,366,0,0,0,0,371,0,0,28,0,0,0,0,35,93,93,93,93,93,93,93,93,93,93,0,0,0,0,0,0,0,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,0,0,0,0,93,0,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,94,95,95,0,440,95,441,441,441,441,441,441,441,441,441,441,0,0,0,0,420,0,420,0,95,421,421,421,421,421,421,421,421,421,421,0,0,0,0,0,93,93,93,93,93,93,93,93,93,93,0,0,0,0,0,0,0,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,0,0,0,0,93,0,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,100,100,0,0,100,421,421,421,421,421,421,421,421,421,421,0,0,0,0,424,0,424,0,100,425,425,425,425,425,425,425,425,425,425,0,0,0,0,0,101,101,101,101,101,101,101,101,101,101,0,0,0,0,0,0,0,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,0,0,0,0,101,0,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,102,103,103,0,0,103,425,425,425,425,425,425,425,425,425,425,465,0,0,0,0,0,0,0,103,466,466,466,466,466,466,466,466,0,0,0,0,0,0,0,101,101,101,101,101,101,101,101,101,101,0,0,0,0,0,0,0,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,0,0,0,0,101,0,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,111,111,111,111,111,111,111,111,111,111,111,111,22,0,0,0,0,0,0,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,0,0,0,0,111,0,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,112,111,111,111,111,111,111,111,111,111,111,22,0,0,0,0,0,0,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,0,0,0,0,111,0,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,22,0,0,0,0,0,0,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,- 1,0,0,- 1,111,0,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,0,0,0,0,112,112,112,112,112,112,112,112,112,112,112,112,114,0,0,0,0,0,0,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,0,0,0,0,112,0,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,116,116,116,116,116,116,116,116,116,116,0,0,0,0,0,0,0,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,28,0,0,127,116,0,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,0,0,28,0,0,138,128,128,128,128,128,128,128,128,130,130,130,130,130,130,130,130,130,130,0,0,0,0,0,0,0,130,130,130,130,130,130,0,0,0,139,139,139,139,139,139,139,139,0,0,0,0,0,0,0,31,0,0,- 1,0,465,0,0,130,130,130,130,130,130,467,467,467,467,467,467,467,467,0,129,130,130,130,130,130,130,130,130,130,130,31,0,0,0,0,0,0,130,130,130,130,130,130,141,141,141,141,141,141,141,141,141,141,140,0,0,0,0,0,0,141,141,141,141,141,141,0,0,0,130,130,130,130,130,130,19,0,0,377,0,0,141,141,141,141,141,141,141,141,141,141,0,141,141,141,141,141,141,141,141,141,141,141,141,0,0,0,0,0,0,0,0,0,378,378,378,378,378,378,378,378,0,0,0,0,0,0,0,0,0,141,141,141,141,141,141,0,380,380,380,380,380,380,380,380,380,380,0,0,0,0,0,0,0,380,380,380,380,380,380,0,0,28,0,0,0,0,0,0,0,0,0,0,0,0,380,380,380,380,380,380,380,380,380,380,379,380,380,380,380,380,380,380,380,380,380,380,380,0,0,415,0,426,426,426,426,426,426,426,426,427,427,0,0,0,0,0,0,0,0,0,0,0,417,380,380,380,380,380,380,428,0,0,0,0,0,0,0,0,429,0,0,430,415,0,416,416,416,416,416,416,416,416,416,416,417,0,0,0,0,0,0,428,0,0,0,417,0,0,0,0,429,0,418,430,0,0,0,0,0,0,0,419,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,417,0,0,0,0,0,0,418,0,0,0,0,0,0,0,0,419,408,408,408,408,408,408,408,408,408,408,0,0,0,0,0,0,0,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,0,0,0,0,408,0,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,0,0,0,0,0,0,0,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,0,0,0,0,408,0,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,422,422,422,422,422,422,422,422,422,422,0,0,0,0,0,0,0,0,0,0,0,423,96,0,0,0,0,415,96,416,416,416,416,416,416,416,416,416,416,421,421,421,421,421,421,421,421,421,421,0,417,0,0,423,96,0,0,418,0,0,96,94,0,0,0,0,419,94,0,422,422,422,422,422,422,422,422,422,422,0,0,0,417,0,0,0,0,0,0,418,423,96,0,94,0,0,0,96,419,94,0,0,425,425,425,425,425,425,425,425,425,425,0,0,0,0,0,0,0,0,0,0,423,96,96,0,0,0,0,96,96,415,0,426,426,426,426,426,426,426,426,427,427,0,0,0,0,0,0,0,0,0,0,0,417,0,96,0,0,0,0,436,96,0,0,0,0,0,0,415,437,427,427,427,427,427,427,427,427,427,427,0,0,0,0,0,417,0,0,0,0,0,417,436,0,0,0,0,0,434,0,0,437,0,0,0,0,0,435,0,0,441,441,441,441,441,441,441,441,441,441,0,0,0,417,0,0,0,0,0,0,434,423,96,0,0,0,0,0,96,435,431,431,431,431,431,431,431,431,431,431,0,0,0,0,0,0,0,431,431,431,431,431,431,423,96,0,0,0,0,0,96,0,0,0,0,0,0,0,431,431,431,431,431,431,431,431,431,431,0,431,431,431,431,431,431,431,431,431,431,431,431,0,0,0,451,0,432,0,0,452,0,0,0,0,0,433,0,0,453,453,453,453,453,453,453,453,0,431,431,431,431,431,431,454,0,0,0,0,432,0,0,0,0,0,0,0,0,433,0,0,0,0,0,0,0,0,0,0,0,0,0,0,455,0,0,0,0,456,457,0,0,0,458,0,0,0,0,0,0,0,459,0,0,0,460,0,461,0,462,0,463,464,464,464,464,464,464,464,464,464,464,0,0,0,0,0,0,0,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,0,0,0,0,0,0,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,169,0,0,0,0,0,0,0,0,464,464,464,464,464,464,464,464,464,464,0,0,0,0,0,0,0,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,0,0,0,0,0,0,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,19,0,0,475,0,476,0,474,474,474,474,474,474,474,474,474,474,0,0,0,0,0,0,0,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,0,0,0,0,474,0,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,480,0,0,0,0,0,0,0,479,479,479,479,479,479,479,479,479,479,0,0,0,0,0,0,0,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,0,0,0,0,479,0,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
const int Cyc_lex_check[3822U]={- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,0,25,29,0,125,127,136,138,374,377,43,43,46,46,43,481,46,- 1,- 1,- 1,- 1,- 1,123,270,475,0,488,10,12,42,10,12,42,43,223,46,235,20,1,1,1,1,1,1,1,1,1,1,38,51,124,236,10,38,260,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,134,261,271,273,1,134,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,9,18,54,46,55,56,57,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,58,59,60,61,2,62,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,63,64,7,3,3,3,3,3,7,7,65,66,67,70,71,7,30,30,30,30,30,30,30,30,72,73,3,3,3,74,77,3,3,3,48,48,3,3,48,3,3,3,3,3,3,3,3,3,3,3,3,3,3,78,3,3,3,48,0,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,10,12,42,3,3,79,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,3,84,4,5,5,6,8,5,6,8,50,50,53,85,50,53,86,27,87,17,27,53,17,4,88,89,5,90,5,6,8,9,18,50,97,98,5,117,118,119,120,121,122,17,34,34,34,34,34,34,34,34,146,147,148,27,27,27,27,27,27,27,27,68,68,149,150,68,75,75,151,152,75,153,80,80,156,157,80,69,69,4,158,69,159,160,68,6,8,5,161,75,76,76,162,163,76,80,164,165,166,17,69,167,27,168,7,83,83,5,170,83,171,172,173,76,11,11,11,11,11,11,11,11,11,11,106,27,113,106,83,113,3,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,114,174,135,114,11,135,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,91,91,69,175,91,177,178,179,180,99,99,181,182,99,184,76,107,107,108,108,107,185,108,91,186,13,13,95,95,13,83,95,99,115,115,187,188,115,189,107,190,108,191,192,193,194,195,196,13,197,95,199,200,201,4,202,115,203,204,5,6,8,13,13,13,13,13,13,13,13,13,13,13,13,17,205,206,207,208,209,210,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,211,213,214,215,13,216,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,13,14,14,95,217,14,218,219,32,32,32,32,32,32,32,32,32,32,103,103,220,221,103,225,14,32,32,32,32,32,32,128,128,128,128,128,128,128,128,14,226,103,224,106,227,113,228,224,104,104,229,14,104,230,231,232,233,32,32,32,32,32,32,131,131,131,131,131,131,131,131,104,234,239,240,241,114,238,135,15,15,242,243,15,244,245,238,238,246,247,248,14,249,238,250,251,14,14,252,253,254,255,15,14,139,139,139,139,139,139,139,139,256,257,258,259,263,264,15,15,15,15,15,15,15,15,15,15,265,266,267,268,103,269,13,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,274,104,275,276,15,277,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,278,280,15,142,142,142,142,142,142,142,142,155,155,155,279,155,281,282,283,155,284,285,279,286,287,16,272,288,155,155,289,290,291,293,294,295,296,297,16,16,16,16,16,16,16,16,16,16,16,298,299,300,301,302,303,14,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,304,306,307,308,16,309,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,16,310,16,33,33,33,33,33,33,33,33,33,33,311,312,313,314,315,317,15,33,33,33,33,33,33,318,319,320,321,322,323,324,325,326,328,329,330,331,332,333,39,39,39,39,39,39,39,39,39,39,334,33,33,33,33,33,33,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,335,336,337,338,39,339,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,39,342,41,41,41,41,41,41,41,41,41,41,272,343,344,345,346,347,16,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,348,349,350,351,41,352,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,41,44,44,354,355,44,356,222,222,357,360,222,361,362,363,364,365,367,368,369,370,386,388,391,44,395,398,389,144,144,222,400,144,145,145,392,403,145,222,409,44,44,44,44,44,44,44,44,44,44,389,144,411,392,401,401,145,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,399,399,399,440,44,449,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,44,47,47,144,384,47,384,384,145,393,451,375,375,144,452,375,454,455,145,418,404,419,456,145,47,393,393,384,457,341,341,382,382,341,375,382,375,458,459,460,47,47,47,47,47,47,47,47,47,47,461,418,341,419,382,462,382,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,404,467,476,477,47,478,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,47,81,81,292,341,81,390,292,353,353,480,482,353,483,341,484,292,485,292,428,429,432,486,433,81,487,- 1,473,- 1,473,473,353,- 1,359,359,390,- 1,359,- 1,353,81,81,81,81,81,81,81,81,81,81,473,428,429,432,292,433,359,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,- 1,- 1,- 1,- 1,81,390,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,81,82,82,358,358,82,434,358,359,378,378,378,378,378,378,378,378,- 1,359,435,436,437,- 1,- 1,82,- 1,358,- 1,- 1,- 1,- 1,- 1,- 1,- 1,358,- 1,- 1,- 1,434,- 1,82,82,82,82,82,82,82,82,82,82,- 1,435,436,437,- 1,- 1,- 1,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,- 1,- 1,- 1,- 1,82,- 1,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,82,92,92,366,366,92,- 1,366,371,371,- 1,- 1,371,381,381,381,381,381,381,381,381,- 1,- 1,390,92,- 1,366,- 1,- 1,- 1,- 1,371,- 1,- 1,366,- 1,- 1,- 1,- 1,371,92,92,92,92,92,92,92,92,92,92,- 1,- 1,- 1,- 1,- 1,- 1,- 1,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,- 1,- 1,- 1,- 1,92,- 1,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,92,93,93,- 1,394,93,394,394,394,394,394,394,394,394,394,394,- 1,- 1,- 1,- 1,417,- 1,417,- 1,93,417,417,417,417,417,417,417,417,417,417,- 1,- 1,- 1,- 1,- 1,93,93,93,93,93,93,93,93,93,93,- 1,- 1,- 1,- 1,- 1,- 1,- 1,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,- 1,- 1,- 1,- 1,93,- 1,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,93,100,100,- 1,- 1,100,420,420,420,420,420,420,420,420,420,420,- 1,- 1,- 1,- 1,423,- 1,423,- 1,100,423,423,423,423,423,423,423,423,423,423,- 1,- 1,- 1,- 1,- 1,100,100,100,100,100,100,100,100,100,100,- 1,- 1,- 1,- 1,- 1,- 1,- 1,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,- 1,- 1,- 1,- 1,100,- 1,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,100,101,101,- 1,- 1,101,424,424,424,424,424,424,424,424,424,424,453,- 1,- 1,- 1,- 1,- 1,- 1,- 1,101,453,453,453,453,453,453,453,453,- 1,- 1,- 1,- 1,- 1,- 1,- 1,101,101,101,101,101,101,101,101,101,101,- 1,- 1,- 1,- 1,- 1,- 1,- 1,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,- 1,- 1,- 1,- 1,101,- 1,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,101,109,109,109,109,109,109,109,109,109,109,109,109,109,- 1,- 1,- 1,- 1,- 1,- 1,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,- 1,- 1,- 1,- 1,109,- 1,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,109,110,110,110,110,110,110,110,110,110,110,110,110,110,- 1,- 1,- 1,- 1,- 1,- 1,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,- 1,- 1,- 1,- 1,110,- 1,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,110,111,111,111,111,111,111,111,111,111,111,111,111,111,- 1,- 1,- 1,- 1,- 1,- 1,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,112,- 1,- 1,112,111,- 1,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,- 1,- 1,- 1,- 1,112,112,112,112,112,112,112,112,112,112,112,112,112,- 1,- 1,- 1,- 1,- 1,- 1,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,- 1,- 1,- 1,- 1,112,- 1,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,112,116,116,116,116,116,116,116,116,116,116,- 1,- 1,- 1,- 1,- 1,- 1,- 1,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,126,- 1,- 1,126,116,- 1,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,116,- 1,- 1,137,- 1,- 1,137,126,126,126,126,126,126,126,126,129,129,129,129,129,129,129,129,129,129,- 1,- 1,- 1,- 1,- 1,- 1,- 1,129,129,129,129,129,129,- 1,- 1,- 1,137,137,137,137,137,137,137,137,- 1,- 1,- 1,- 1,- 1,- 1,- 1,126,- 1,- 1,112,- 1,466,- 1,- 1,129,129,129,129,129,129,466,466,466,466,466,466,466,466,- 1,126,130,130,130,130,130,130,130,130,130,130,137,- 1,- 1,- 1,- 1,- 1,- 1,130,130,130,130,130,130,140,140,140,140,140,140,140,140,140,140,137,- 1,- 1,- 1,- 1,- 1,- 1,140,140,140,140,140,140,- 1,- 1,- 1,130,130,130,130,130,130,376,- 1,- 1,376,- 1,- 1,141,141,141,141,141,141,141,141,141,141,- 1,140,140,140,140,140,140,141,141,141,141,141,141,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,376,376,376,376,376,376,376,376,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,141,141,141,141,141,141,- 1,379,379,379,379,379,379,379,379,379,379,- 1,- 1,- 1,- 1,- 1,- 1,- 1,379,379,379,379,379,379,- 1,- 1,376,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,380,380,380,380,380,380,380,380,380,380,376,379,379,379,379,379,379,380,380,380,380,380,380,- 1,- 1,396,- 1,396,396,396,396,396,396,396,396,396,396,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,396,380,380,380,380,380,380,396,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,396,- 1,- 1,396,397,- 1,397,397,397,397,397,397,397,397,397,397,396,- 1,- 1,- 1,- 1,- 1,- 1,396,- 1,- 1,- 1,397,- 1,- 1,- 1,- 1,396,- 1,397,396,- 1,- 1,- 1,- 1,- 1,- 1,- 1,397,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,397,- 1,- 1,- 1,- 1,- 1,- 1,397,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,397,402,402,402,402,402,402,402,402,402,402,- 1,- 1,- 1,- 1,- 1,- 1,- 1,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,- 1,- 1,- 1,- 1,402,- 1,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,402,408,408,408,408,408,408,408,408,408,408,- 1,- 1,- 1,- 1,- 1,- 1,- 1,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,- 1,- 1,- 1,- 1,408,- 1,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,408,415,415,415,415,415,415,415,415,415,415,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,415,415,- 1,- 1,- 1,- 1,416,415,416,416,416,416,416,416,416,416,416,416,421,421,421,421,421,421,421,421,421,421,- 1,416,- 1,- 1,415,415,- 1,- 1,416,- 1,- 1,415,421,- 1,- 1,- 1,- 1,416,421,- 1,422,422,422,422,422,422,422,422,422,422,- 1,- 1,- 1,416,- 1,- 1,- 1,- 1,- 1,- 1,416,422,422,- 1,421,- 1,- 1,- 1,422,416,421,- 1,- 1,425,425,425,425,425,425,425,425,425,425,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,422,422,425,- 1,- 1,- 1,- 1,422,425,426,- 1,426,426,426,426,426,426,426,426,426,426,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,426,- 1,425,- 1,- 1,- 1,- 1,426,425,- 1,- 1,- 1,- 1,- 1,- 1,427,426,427,427,427,427,427,427,427,427,427,427,- 1,- 1,- 1,- 1,- 1,426,- 1,- 1,- 1,- 1,- 1,427,426,- 1,- 1,- 1,- 1,- 1,427,- 1,- 1,426,- 1,- 1,- 1,- 1,- 1,427,- 1,- 1,441,441,441,441,441,441,441,441,441,441,- 1,- 1,- 1,427,- 1,- 1,- 1,- 1,- 1,- 1,427,441,441,- 1,- 1,- 1,- 1,- 1,441,427,430,430,430,430,430,430,430,430,430,430,- 1,- 1,- 1,- 1,- 1,- 1,- 1,430,430,430,430,430,430,441,441,- 1,- 1,- 1,- 1,- 1,441,- 1,- 1,- 1,- 1,- 1,- 1,- 1,431,431,431,431,431,431,431,431,431,431,- 1,430,430,430,430,430,430,431,431,431,431,431,431,- 1,- 1,- 1,450,- 1,431,- 1,- 1,450,- 1,- 1,- 1,- 1,- 1,431,- 1,- 1,450,450,450,450,450,450,450,450,- 1,431,431,431,431,431,431,450,- 1,- 1,- 1,- 1,431,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,431,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,450,- 1,- 1,- 1,- 1,450,450,- 1,- 1,- 1,450,- 1,- 1,- 1,- 1,- 1,- 1,- 1,450,- 1,- 1,- 1,450,- 1,450,- 1,450,- 1,450,463,463,463,463,463,463,463,463,463,463,- 1,- 1,- 1,- 1,- 1,- 1,- 1,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,- 1,- 1,- 1,- 1,- 1,- 1,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,463,464,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,464,464,464,464,464,464,464,464,464,464,- 1,- 1,- 1,- 1,- 1,- 1,- 1,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,- 1,- 1,- 1,- 1,- 1,- 1,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,464,474,- 1,- 1,474,- 1,474,- 1,474,474,474,474,474,474,474,474,474,474,- 1,- 1,- 1,- 1,- 1,- 1,- 1,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,- 1,- 1,- 1,- 1,474,- 1,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,474,479,- 1,- 1,- 1,- 1,- 1,- 1,- 1,479,479,479,479,479,479,479,479,479,479,- 1,- 1,- 1,- 1,- 1,- 1,- 1,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,- 1,- 1,- 1,- 1,479,- 1,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,479,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1,- 1};
int Cyc_lex_engine(int start_state,struct Cyc_Lexing_lexbuf*lbuf){
# 232
int state;int base;int backtrk;
int c;
state=start_state;
# 236
if(state >= 0){
({int _tmp4A7=lbuf->lex_start_pos=lbuf->lex_curr_pos;lbuf->lex_last_pos=_tmp4A7;});
lbuf->lex_last_action=- 1;}else{
# 240
state=(- state)- 1;}
# 242
while(1){
base=*((const int*)_check_known_subscript_notnull(Cyc_lex_base,489U,sizeof(int),state));
if(base < 0)return(- base)- 1;backtrk=Cyc_lex_backtrk[state];
# 246
if(backtrk >= 0){
lbuf->lex_last_pos=lbuf->lex_curr_pos;
lbuf->lex_last_action=backtrk;}
# 246
if(lbuf->lex_curr_pos >= lbuf->lex_buffer_len){
# 251
if(!lbuf->lex_eof_reached)
return(- state)- 1;else{
# 254
c=256;}}else{
# 256
c=(int)*((char*)_check_fat_subscript(lbuf->lex_buffer,sizeof(char),lbuf->lex_curr_pos ++));
if(c == -1)c=256;}
# 259
if(*((const int*)_check_known_subscript_notnull(Cyc_lex_check,3822U,sizeof(int),base + c))== state)
state=*((const int*)_check_known_subscript_notnull(Cyc_lex_trans,3822U,sizeof(int),base + c));else{
# 262
state=Cyc_lex_default[state];}
if(state < 0){
lbuf->lex_curr_pos=lbuf->lex_last_pos;
if(lbuf->lex_last_action == - 1)
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp1E=_cycalloc(sizeof(*_tmp1E));_tmp1E->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4A8=({const char*_tmp1D="empty token";_tag_fat(_tmp1D,sizeof(char),12U);});_tmp1E->f1=_tmp4A8;});_tmp1E;}));else{
# 268
return lbuf->lex_last_action;}}else{
# 271
if(c == 256)lbuf->lex_eof_reached=0;}}}
# 230
struct _tuple25*Cyc_line_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
# 276
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp20=lexstate;switch(_tmp20){case 0U: _LL1: _LL2:
# 235 "buildlib.cyl"
({Cyc_macroname(lexbuf);});
for(0;Cyc_current_args != 0;Cyc_current_args=((struct Cyc_List_List*)_check_null(Cyc_current_args))->tl){
Cyc_current_targets=({struct Cyc_Set_Set**_tmp22=_cycalloc(sizeof(*_tmp22));({struct Cyc_Set_Set*_tmp4AB=({({struct Cyc_Set_Set*(*_tmp4AA)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({struct Cyc_Set_Set*(*_tmp21)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_delete;_tmp21;});struct Cyc_Set_Set*_tmp4A9=*((struct Cyc_Set_Set**)_check_null(Cyc_current_targets));_tmp4AA(_tmp4A9,(struct _fat_ptr*)((struct Cyc_List_List*)_check_null(Cyc_current_args))->hd);});});*_tmp22=_tmp4AB;});_tmp22;});}
# 240
return({struct _tuple25*_tmp23=_cycalloc(sizeof(*_tmp23));_tmp23->f1=(struct _fat_ptr*)_check_null(Cyc_current_source),_tmp23->f2=*((struct Cyc_Set_Set**)_check_null(Cyc_current_targets));_tmp23;});case 1U: _LL3: _LL4:
# 243 "buildlib.cyl"
 return({Cyc_line(lexbuf);});case 2U: _LL5: _LL6:
# 245
 return 0;default: _LL7: _LL8:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_line_rec(lexbuf,lexstate);});}_LL0:;}
# 249
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp25=_cycalloc(sizeof(*_tmp25));_tmp25->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4AC=({const char*_tmp24="some action didn't return!";_tag_fat(_tmp24,sizeof(char),27U);});_tmp25->f1=_tmp4AC;});_tmp25;}));}
# 230 "buildlib.cyl"
struct _tuple25*Cyc_line(struct Cyc_Lexing_lexbuf*lexbuf){
# 251 "buildlib.cyl"
return({Cyc_line_rec(lexbuf,0);});}
int Cyc_macroname_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp28=lexstate;switch(_tmp28){case 0U: _LL1: _LL2:
# 249 "buildlib.cyl"
 Cyc_current_source=({struct _fat_ptr*_tmp2D=_cycalloc(sizeof(*_tmp2D));({struct _fat_ptr _tmp4AE=(struct _fat_ptr)({struct _fat_ptr(*_tmp29)(struct _fat_ptr,int ofs,unsigned long n)=Cyc_substring;struct _fat_ptr _tmp2A=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});int _tmp2B=0;unsigned long _tmp2C=(unsigned long)(({
int _tmp4AD=({Cyc_Lexing_lexeme_end(lexbuf);});_tmp4AD - ({Cyc_Lexing_lexeme_start(lexbuf);});})- 2);_tmp29(_tmp2A,_tmp2B,_tmp2C);});
# 249
*_tmp2D=_tmp4AE;});_tmp2D;});
# 251
Cyc_current_args=0;
Cyc_current_targets=({struct Cyc_Set_Set**_tmp2F=_cycalloc(sizeof(*_tmp2F));({struct Cyc_Set_Set*_tmp4AF=({({struct Cyc_Set_Set*(*_tmp2E)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Set_empty;_tmp2E;})(Cyc_strptrcmp);});*_tmp2F=_tmp4AF;});_tmp2F;});
({Cyc_token(lexbuf);});
return 0;case 1U: _LL3: _LL4:
# 257
 Cyc_current_source=({struct _fat_ptr*_tmp34=_cycalloc(sizeof(*_tmp34));({struct _fat_ptr _tmp4B1=(struct _fat_ptr)({struct _fat_ptr(*_tmp30)(struct _fat_ptr,int ofs,unsigned long n)=Cyc_substring;struct _fat_ptr _tmp31=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});int _tmp32=0;unsigned long _tmp33=(unsigned long)(({
int _tmp4B0=({Cyc_Lexing_lexeme_end(lexbuf);});_tmp4B0 - ({Cyc_Lexing_lexeme_start(lexbuf);});})- 1);_tmp30(_tmp31,_tmp32,_tmp33);});
# 257
*_tmp34=_tmp4B1;});_tmp34;});
# 259
Cyc_current_args=0;
Cyc_current_targets=({struct Cyc_Set_Set**_tmp36=_cycalloc(sizeof(*_tmp36));({struct Cyc_Set_Set*_tmp4B2=({({struct Cyc_Set_Set*(*_tmp35)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Set_empty;_tmp35;})(Cyc_strptrcmp);});*_tmp36=_tmp4B2;});_tmp36;});
({Cyc_args(lexbuf);});
return 0;case 2U: _LL5: _LL6:
# 265
 Cyc_current_source=({struct _fat_ptr*_tmp37=_cycalloc(sizeof(*_tmp37));({struct _fat_ptr _tmp4B3=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});*_tmp37=_tmp4B3;});_tmp37;});
Cyc_current_args=0;
Cyc_current_targets=({struct Cyc_Set_Set**_tmp39=_cycalloc(sizeof(*_tmp39));({struct Cyc_Set_Set*_tmp4B4=({({struct Cyc_Set_Set*(*_tmp38)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Set_empty;_tmp38;})(Cyc_strptrcmp);});*_tmp39=_tmp4B4;});_tmp39;});
({Cyc_token(lexbuf);});
return 0;default: _LL7: _LL8:
# 271
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_macroname_rec(lexbuf,lexstate);});}_LL0:;}
# 274
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp3B=_cycalloc(sizeof(*_tmp3B));_tmp3B->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4B5=({const char*_tmp3A="some action didn't return!";_tag_fat(_tmp3A,sizeof(char),27U);});_tmp3B->f1=_tmp4B5;});_tmp3B;}));}
# 276
int Cyc_macroname(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_macroname_rec(lexbuf,1);});}
int Cyc_args_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp3E=lexstate;switch(_tmp3E){case 0U: _LL1: _LL2: {
# 274 "buildlib.cyl"
struct _fat_ptr*a=({struct _fat_ptr*_tmp44=_cycalloc(sizeof(*_tmp44));({struct _fat_ptr _tmp4B7=(struct _fat_ptr)({struct _fat_ptr(*_tmp40)(struct _fat_ptr,int ofs,unsigned long n)=Cyc_substring;struct _fat_ptr _tmp41=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});int _tmp42=0;unsigned long _tmp43=(unsigned long)(({
int _tmp4B6=({Cyc_Lexing_lexeme_end(lexbuf);});_tmp4B6 - ({Cyc_Lexing_lexeme_start(lexbuf);});})- 2);_tmp40(_tmp41,_tmp42,_tmp43);});
# 274
*_tmp44=_tmp4B7;});_tmp44;});
# 276
Cyc_current_args=({struct Cyc_List_List*_tmp3F=_cycalloc(sizeof(*_tmp3F));_tmp3F->hd=a,_tmp3F->tl=Cyc_current_args;_tmp3F;});
return({Cyc_args(lexbuf);});}case 1U: _LL3: _LL4: {
# 280
struct _fat_ptr*a=({struct _fat_ptr*_tmp4A=_cycalloc(sizeof(*_tmp4A));({struct _fat_ptr _tmp4B9=(struct _fat_ptr)({struct _fat_ptr(*_tmp46)(struct _fat_ptr,int ofs,unsigned long n)=Cyc_substring;struct _fat_ptr _tmp47=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});int _tmp48=0;unsigned long _tmp49=(unsigned long)(({
int _tmp4B8=({Cyc_Lexing_lexeme_end(lexbuf);});_tmp4B8 - ({Cyc_Lexing_lexeme_start(lexbuf);});})- 1);_tmp46(_tmp47,_tmp48,_tmp49);});
# 280
*_tmp4A=_tmp4B9;});_tmp4A;});
# 282
Cyc_current_args=({struct Cyc_List_List*_tmp45=_cycalloc(sizeof(*_tmp45));_tmp45->hd=a,_tmp45->tl=Cyc_current_args;_tmp45;});
return({Cyc_args(lexbuf);});}case 2U: _LL5: _LL6: {
# 286
struct _fat_ptr*a=({struct _fat_ptr*_tmp50=_cycalloc(sizeof(*_tmp50));({struct _fat_ptr _tmp4BB=(struct _fat_ptr)({struct _fat_ptr(*_tmp4C)(struct _fat_ptr,int ofs,unsigned long n)=Cyc_substring;struct _fat_ptr _tmp4D=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});int _tmp4E=0;unsigned long _tmp4F=(unsigned long)(({
int _tmp4BA=({Cyc_Lexing_lexeme_end(lexbuf);});_tmp4BA - ({Cyc_Lexing_lexeme_start(lexbuf);});})- 1);_tmp4C(_tmp4D,_tmp4E,_tmp4F);});
# 286
*_tmp50=_tmp4BB;});_tmp50;});
# 288
Cyc_current_args=({struct Cyc_List_List*_tmp4B=_cycalloc(sizeof(*_tmp4B));_tmp4B->hd=a,_tmp4B->tl=Cyc_current_args;_tmp4B;});
return({Cyc_token(lexbuf);});}case 3U: _LL7: _LL8: {
# 292
struct _fat_ptr*a=({struct _fat_ptr*_tmp56=_cycalloc(sizeof(*_tmp56));({struct _fat_ptr _tmp4BD=(struct _fat_ptr)({struct _fat_ptr(*_tmp52)(struct _fat_ptr,int ofs,unsigned long n)=Cyc_substring;struct _fat_ptr _tmp53=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});int _tmp54=0;unsigned long _tmp55=(unsigned long)(({
int _tmp4BC=({Cyc_Lexing_lexeme_end(lexbuf);});_tmp4BC - ({Cyc_Lexing_lexeme_start(lexbuf);});})- 1);_tmp52(_tmp53,_tmp54,_tmp55);});
# 292
*_tmp56=_tmp4BD;});_tmp56;});
# 294
Cyc_current_args=({struct Cyc_List_List*_tmp51=_cycalloc(sizeof(*_tmp51));_tmp51->hd=a,_tmp51->tl=Cyc_current_args;_tmp51;});
return({Cyc_token(lexbuf);});}default: _LL9: _LLA:
# 297
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_args_rec(lexbuf,lexstate);});}_LL0:;}
# 300
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp58=_cycalloc(sizeof(*_tmp58));_tmp58->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4BE=({const char*_tmp57="some action didn't return!";_tag_fat(_tmp57,sizeof(char),27U);});_tmp58->f1=_tmp4BE;});_tmp58;}));}
# 302
int Cyc_args(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_args_rec(lexbuf,2);});}
int Cyc_token_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp5B=lexstate;switch(_tmp5B){case 0U: _LL1: _LL2:
# 301 "buildlib.cyl"
({void(*_tmp5C)(struct _fat_ptr*sptr)=Cyc_add_target;struct _fat_ptr*_tmp5D=({struct _fat_ptr*_tmp5E=_cycalloc(sizeof(*_tmp5E));({struct _fat_ptr _tmp4BF=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});*_tmp5E=_tmp4BF;});_tmp5E;});_tmp5C(_tmp5D);});return({Cyc_token(lexbuf);});case 1U: _LL3: _LL4:
# 304 "buildlib.cyl"
 return 0;case 2U: _LL5: _LL6:
# 307 "buildlib.cyl"
 return({Cyc_token(lexbuf);});case 3U: _LL7: _LL8:
# 310 "buildlib.cyl"
({Cyc_string(lexbuf);});return({Cyc_token(lexbuf);});case 4U: _LL9: _LLA:
# 313 "buildlib.cyl"
 return({Cyc_token(lexbuf);});case 5U: _LLB: _LLC:
# 315
 return({Cyc_token(lexbuf);});case 6U: _LLD: _LLE:
# 317
 return({Cyc_token(lexbuf);});case 7U: _LLF: _LL10:
# 319
 return({Cyc_token(lexbuf);});case 8U: _LL11: _LL12:
# 322 "buildlib.cyl"
 return({Cyc_token(lexbuf);});case 9U: _LL13: _LL14:
# 325 "buildlib.cyl"
 return({Cyc_token(lexbuf);});case 10U: _LL15: _LL16:
# 328 "buildlib.cyl"
 return({Cyc_token(lexbuf);});case 11U: _LL17: _LL18:
# 330
 return({Cyc_token(lexbuf);});case 12U: _LL19: _LL1A:
# 332
 return({Cyc_token(lexbuf);});case 13U: _LL1B: _LL1C:
# 334
 return({Cyc_token(lexbuf);});case 14U: _LL1D: _LL1E:
# 336
 return({Cyc_token(lexbuf);});case 15U: _LL1F: _LL20:
# 338
 return({Cyc_token(lexbuf);});case 16U: _LL21: _LL22:
# 340
 return({Cyc_token(lexbuf);});case 17U: _LL23: _LL24:
# 342
 return({Cyc_token(lexbuf);});case 18U: _LL25: _LL26:
# 344
 return({Cyc_token(lexbuf);});case 19U: _LL27: _LL28:
# 346
 return({Cyc_token(lexbuf);});case 20U: _LL29: _LL2A:
# 348
 return({Cyc_token(lexbuf);});case 21U: _LL2B: _LL2C:
# 350
 return({Cyc_token(lexbuf);});case 22U: _LL2D: _LL2E:
# 352
 return({Cyc_token(lexbuf);});case 23U: _LL2F: _LL30:
# 354
 return({Cyc_token(lexbuf);});case 24U: _LL31: _LL32:
# 357 "buildlib.cyl"
 return({Cyc_token(lexbuf);});case 25U: _LL33: _LL34:
# 359
 return({Cyc_token(lexbuf);});case 26U: _LL35: _LL36:
# 361
 return({Cyc_token(lexbuf);});case 27U: _LL37: _LL38:
# 363
 return({Cyc_token(lexbuf);});case 28U: _LL39: _LL3A:
# 365
 return({Cyc_token(lexbuf);});case 29U: _LL3B: _LL3C:
# 367
 return({Cyc_token(lexbuf);});case 30U: _LL3D: _LL3E:
# 369
 return({Cyc_token(lexbuf);});case 31U: _LL3F: _LL40:
# 371
 return({Cyc_token(lexbuf);});case 32U: _LL41: _LL42:
# 373
 return({Cyc_token(lexbuf);});case 33U: _LL43: _LL44:
# 375
 return({Cyc_token(lexbuf);});case 34U: _LL45: _LL46:
# 377
 return({Cyc_token(lexbuf);});case 35U: _LL47: _LL48:
# 379
 return({Cyc_token(lexbuf);});case 36U: _LL49: _LL4A:
# 381
 return({Cyc_token(lexbuf);});case 37U: _LL4B: _LL4C:
# 383
 return({Cyc_token(lexbuf);});case 38U: _LL4D: _LL4E:
# 385
 return({Cyc_token(lexbuf);});case 39U: _LL4F: _LL50:
# 387
 return({Cyc_token(lexbuf);});case 40U: _LL51: _LL52:
# 389
 return({Cyc_token(lexbuf);});case 41U: _LL53: _LL54:
# 391
 return({Cyc_token(lexbuf);});case 42U: _LL55: _LL56:
# 393
 return({Cyc_token(lexbuf);});case 43U: _LL57: _LL58:
# 395
 return({Cyc_token(lexbuf);});case 44U: _LL59: _LL5A:
# 397
 return({Cyc_token(lexbuf);});case 45U: _LL5B: _LL5C:
# 399
 return({Cyc_token(lexbuf);});case 46U: _LL5D: _LL5E:
# 401
 return({Cyc_token(lexbuf);});case 47U: _LL5F: _LL60:
# 403
 return({Cyc_token(lexbuf);});case 48U: _LL61: _LL62:
# 406 "buildlib.cyl"
 return({Cyc_token(lexbuf);});default: _LL63: _LL64:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_token_rec(lexbuf,lexstate);});}_LL0:;}
# 410
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp60=_cycalloc(sizeof(*_tmp60));_tmp60->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4C0=({const char*_tmp5F="some action didn't return!";_tag_fat(_tmp5F,sizeof(char),27U);});_tmp60->f1=_tmp4C0;});_tmp60;}));}
# 412
int Cyc_token(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_token_rec(lexbuf,3);});}
int Cyc_string_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp63=lexstate;switch(_tmp63){case 0U: _LL1: _LL2:
# 411 "buildlib.cyl"
 return({Cyc_string(lexbuf);});case 1U: _LL3: _LL4:
# 412 "buildlib.cyl"
 return 0;case 2U: _LL5: _LL6:
# 413 "buildlib.cyl"
 return({Cyc_string(lexbuf);});case 3U: _LL7: _LL8:
# 414 "buildlib.cyl"
 return({Cyc_string(lexbuf);});case 4U: _LL9: _LLA:
# 417 "buildlib.cyl"
 return({Cyc_string(lexbuf);});case 5U: _LLB: _LLC:
# 420 "buildlib.cyl"
 return({Cyc_string(lexbuf);});case 6U: _LLD: _LLE:
# 422
 return({Cyc_string(lexbuf);});case 7U: _LLF: _LL10:
# 423 "buildlib.cyl"
 return 0;case 8U: _LL11: _LL12:
# 424 "buildlib.cyl"
 return 0;case 9U: _LL13: _LL14:
# 425 "buildlib.cyl"
 return({Cyc_string(lexbuf);});default: _LL15: _LL16:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_string_rec(lexbuf,lexstate);});}_LL0:;}
# 429
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp65=_cycalloc(sizeof(*_tmp65));_tmp65->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4C1=({const char*_tmp64="some action didn't return!";_tag_fat(_tmp64,sizeof(char),27U);});_tmp65->f1=_tmp4C1;});_tmp65;}));}
# 431
int Cyc_string(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_string_rec(lexbuf,4);});}
int Cyc_slurp_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp68=lexstate;switch(_tmp68){case 0U: _LL1: _LL2:
# 434 "buildlib.cyl"
 return 0;case 1U: _LL3: _LL4:
# 436
({Cyc_fputc((int)'"',(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});
while(({Cyc_slurp_string(lexbuf);})){;}
return 1;case 2U: _LL5: _LL6:
# 443 "buildlib.cyl"
({Cyc_fputs("*__IGNORE_FOR_CYCLONE_MALLOC(",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});
({void*_tmp69=0U;({struct _fat_ptr _tmp4C2=({const char*_tmp6A="Warning: declaration of malloc sidestepped\n";_tag_fat(_tmp6A,sizeof(char),44U);});Cyc_log(_tmp4C2,_tag_fat(_tmp69,sizeof(void*),0U));});});
return 1;case 3U: _LL7: _LL8:
# 449 "buildlib.cyl"
({Cyc_fputs(" __IGNORE_FOR_CYCLONE_MALLOC(",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});
({void*_tmp6B=0U;({struct _fat_ptr _tmp4C3=({const char*_tmp6C="Warning: declaration of malloc sidestepped\n";_tag_fat(_tmp6C,sizeof(char),44U);});Cyc_log(_tmp4C3,_tag_fat(_tmp6B,sizeof(void*),0U));});});
return 1;case 4U: _LL9: _LLA:
# 455 "buildlib.cyl"
({Cyc_fputs("*__IGNORE_FOR_CYCLONE_CALLOC(",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});
({void*_tmp6D=0U;({struct _fat_ptr _tmp4C4=({const char*_tmp6E="Warning: declaration of calloc sidestepped\n";_tag_fat(_tmp6E,sizeof(char),44U);});Cyc_log(_tmp4C4,_tag_fat(_tmp6D,sizeof(void*),0U));});});
return 1;case 5U: _LLB: _LLC:
# 461 "buildlib.cyl"
({Cyc_fputs(" __IGNORE_FOR_CYCLONE_CALLOC(",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});
({void*_tmp6F=0U;({struct _fat_ptr _tmp4C5=({const char*_tmp70="Warning: declaration of calloc sidestepped\n";_tag_fat(_tmp70,sizeof(char),44U);});Cyc_log(_tmp4C5,_tag_fat(_tmp6F,sizeof(void*),0U));});});
return 1;case 6U: _LLD: _LLE:
# 465
({Cyc_fputs("__region",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});
({void*_tmp71=0U;({struct _fat_ptr _tmp4C6=({const char*_tmp72="Warning: use of region sidestepped\n";_tag_fat(_tmp72,sizeof(char),36U);});Cyc_log(_tmp4C6,_tag_fat(_tmp71,sizeof(void*),0U));});});
return 1;case 7U: _LLF: _LL10:
# 469
({void*_tmp73=0U;({struct _fat_ptr _tmp4C7=({const char*_tmp74="Warning: use of __extension__ deleted\n";_tag_fat(_tmp74,sizeof(char),39U);});Cyc_log(_tmp4C7,_tag_fat(_tmp73,sizeof(void*),0U));});});
return 1;case 8U: _LL11: _LL12:
# 473 "buildlib.cyl"
({void*_tmp75=0U;({struct _fat_ptr _tmp4C8=({const char*_tmp76="Warning: use of nonnull attribute deleted\n";_tag_fat(_tmp76,sizeof(char),43U);});Cyc_log(_tmp4C8,_tag_fat(_tmp75,sizeof(void*),0U));});});
return 1;case 9U: _LL13: _LL14:
# 478 "buildlib.cyl"
({void*_tmp77=0U;({struct _fat_ptr _tmp4C9=({const char*_tmp78="Warning: use of mode HI deleted\n";_tag_fat(_tmp78,sizeof(char),33U);});Cyc_log(_tmp4C9,_tag_fat(_tmp77,sizeof(void*),0U));});});
return 1;case 10U: _LL15: _LL16:
# 481
({void*_tmp79=0U;({struct _fat_ptr _tmp4CA=({const char*_tmp7A="Warning: use of mode SI deleted\n";_tag_fat(_tmp7A,sizeof(char),33U);});Cyc_log(_tmp4CA,_tag_fat(_tmp79,sizeof(void*),0U));});});
return 1;case 11U: _LL17: _LL18:
# 484
({void*_tmp7B=0U;({struct _fat_ptr _tmp4CB=({const char*_tmp7C="Warning: use of mode QI deleted\n";_tag_fat(_tmp7C,sizeof(char),33U);});Cyc_log(_tmp4CB,_tag_fat(_tmp7B,sizeof(void*),0U));});});
return 1;case 12U: _LL19: _LL1A:
# 487
({void*_tmp7D=0U;({struct _fat_ptr _tmp4CC=({const char*_tmp7E="Warning: use of mode DI deleted\n";_tag_fat(_tmp7E,sizeof(char),33U);});Cyc_log(_tmp4CC,_tag_fat(_tmp7D,sizeof(void*),0U));});});
return 1;case 13U: _LL1B: _LL1C:
# 490
({void*_tmp7F=0U;({struct _fat_ptr _tmp4CD=({const char*_tmp80="Warning: use of mode DI deleted\n";_tag_fat(_tmp80,sizeof(char),33U);});Cyc_log(_tmp4CD,_tag_fat(_tmp7F,sizeof(void*),0U));});});
return 1;case 14U: _LL1D: _LL1E:
# 493
({void*_tmp81=0U;({struct _fat_ptr _tmp4CE=({const char*_tmp82="Warning: use of mode word deleted\n";_tag_fat(_tmp82,sizeof(char),35U);});Cyc_log(_tmp4CE,_tag_fat(_tmp81,sizeof(void*),0U));});});
return 1;case 15U: _LL1F: _LL20:
# 496
({void*_tmp83=0U;({struct _fat_ptr _tmp4CF=({const char*_tmp84="Warning: use of __attribute__((deprecated)) deleted\n";_tag_fat(_tmp84,sizeof(char),53U);});Cyc_log(_tmp4CF,_tag_fat(_tmp83,sizeof(void*),0U));});});
return 1;case 16U: _LL21: _LL22:
# 499
({void*_tmp85=0U;({struct _fat_ptr _tmp4D0=({const char*_tmp86="Warning: use of __attribute__((__deprecated__)) deleted\n";_tag_fat(_tmp86,sizeof(char),57U);});Cyc_log(_tmp4D0,_tag_fat(_tmp85,sizeof(void*),0U));});});
return 1;case 17U: _LL23: _LL24:
# 502
({void*_tmp87=0U;({struct _fat_ptr _tmp4D1=({const char*_tmp88="Warning: use of __attribute__((__transparent_union__)) deleted\n";_tag_fat(_tmp88,sizeof(char),64U);});Cyc_log(_tmp4D1,_tag_fat(_tmp87,sizeof(void*),0U));});});
return 1;case 18U: _LL25: _LL26:
# 505
({Cyc_fputs("inline",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});return 1;case 19U: _LL27: _LL28:
# 507
({Cyc_fputs("inline",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});return 1;case 20U: _LL29: _LL2A:
# 509
({Cyc_fputs("const",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});return 1;case 21U: _LL2B: _LL2C:
# 511
({Cyc_fputs("const",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});return 1;case 22U: _LL2D: _LL2E:
# 513
({Cyc_fputs("signed",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});return 1;case 23U: _LL2F: _LL30:
# 515
({Cyc_fputs("signed",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});return 1;case 24U: _LL31: _LL32:
# 517
({Cyc_fputs("signed",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});return 1;case 25U: _LL33: _LL34:
# 522 "buildlib.cyl"
({Cyc_fputs("int",(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});return 1;case 26U: _LL35: _LL36:
# 524
 return 1;case 27U: _LL37: _LL38:
# 526
({int(*_tmp89)(int,struct Cyc___cycFILE*)=Cyc_fputc;int _tmp8A=(int)({Cyc_Lexing_lexeme_char(lexbuf,0);});struct Cyc___cycFILE*_tmp8B=(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out);_tmp89(_tmp8A,_tmp8B);});return 1;default: _LL39: _LL3A:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_slurp_rec(lexbuf,lexstate);});}_LL0:;}
# 530
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp8D=_cycalloc(sizeof(*_tmp8D));_tmp8D->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4D2=({const char*_tmp8C="some action didn't return!";_tag_fat(_tmp8C,sizeof(char),27U);});_tmp8D->f1=_tmp4D2;});_tmp8D;}));}
# 532
int Cyc_slurp(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_slurp_rec(lexbuf,5);});}
int Cyc_slurp_string_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp90=lexstate;switch(_tmp90){case 0U: _LL1: _LL2:
# 531 "buildlib.cyl"
 return 0;case 1U: _LL3: _LL4:
# 533
({Cyc_fputc((int)'"',(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});return 0;case 2U: _LL5: _LL6:
# 535
({void*_tmp91=0U;({struct _fat_ptr _tmp4D3=({const char*_tmp92="Warning: unclosed string\n";_tag_fat(_tmp92,sizeof(char),26U);});Cyc_log(_tmp4D3,_tag_fat(_tmp91,sizeof(void*),0U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp95=({struct Cyc_String_pa_PrintArg_struct _tmp439;_tmp439.tag=0U,({struct _fat_ptr _tmp4D4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);}));_tmp439.f1=_tmp4D4;});_tmp439;});void*_tmp93[1U];_tmp93[0]=& _tmp95;({struct Cyc___cycFILE*_tmp4D6=(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out);struct _fat_ptr _tmp4D5=({const char*_tmp94="%s";_tag_fat(_tmp94,sizeof(char),3U);});Cyc_fprintf(_tmp4D6,_tmp4D5,_tag_fat(_tmp93,sizeof(void*),1U));});});return 1;case 3U: _LL7: _LL8:
# 538
({struct Cyc_String_pa_PrintArg_struct _tmp98=({struct Cyc_String_pa_PrintArg_struct _tmp43A;_tmp43A.tag=0U,({struct _fat_ptr _tmp4D7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);}));_tmp43A.f1=_tmp4D7;});_tmp43A;});void*_tmp96[1U];_tmp96[0]=& _tmp98;({struct Cyc___cycFILE*_tmp4D9=(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out);struct _fat_ptr _tmp4D8=({const char*_tmp97="%s";_tag_fat(_tmp97,sizeof(char),3U);});Cyc_fprintf(_tmp4D9,_tmp4D8,_tag_fat(_tmp96,sizeof(void*),1U));});});return 1;case 4U: _LL9: _LLA:
# 540
({struct Cyc_String_pa_PrintArg_struct _tmp9B=({struct Cyc_String_pa_PrintArg_struct _tmp43B;_tmp43B.tag=0U,({struct _fat_ptr _tmp4DA=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);}));_tmp43B.f1=_tmp4DA;});_tmp43B;});void*_tmp99[1U];_tmp99[0]=& _tmp9B;({struct Cyc___cycFILE*_tmp4DC=(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out);struct _fat_ptr _tmp4DB=({const char*_tmp9A="%s";_tag_fat(_tmp9A,sizeof(char),3U);});Cyc_fprintf(_tmp4DC,_tmp4DB,_tag_fat(_tmp99,sizeof(void*),1U));});});return 1;case 5U: _LLB: _LLC:
# 542
({struct Cyc_String_pa_PrintArg_struct _tmp9E=({struct Cyc_String_pa_PrintArg_struct _tmp43C;_tmp43C.tag=0U,({struct _fat_ptr _tmp4DD=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);}));_tmp43C.f1=_tmp4DD;});_tmp43C;});void*_tmp9C[1U];_tmp9C[0]=& _tmp9E;({struct Cyc___cycFILE*_tmp4DF=(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out);struct _fat_ptr _tmp4DE=({const char*_tmp9D="%s";_tag_fat(_tmp9D,sizeof(char),3U);});Cyc_fprintf(_tmp4DF,_tmp4DE,_tag_fat(_tmp9C,sizeof(void*),1U));});});return 1;case 6U: _LLD: _LLE:
# 544
({struct Cyc_String_pa_PrintArg_struct _tmpA1=({struct Cyc_String_pa_PrintArg_struct _tmp43D;_tmp43D.tag=0U,({struct _fat_ptr _tmp4E0=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);}));_tmp43D.f1=_tmp4E0;});_tmp43D;});void*_tmp9F[1U];_tmp9F[0]=& _tmpA1;({struct Cyc___cycFILE*_tmp4E2=(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out);struct _fat_ptr _tmp4E1=({const char*_tmpA0="%s";_tag_fat(_tmpA0,sizeof(char),3U);});Cyc_fprintf(_tmp4E2,_tmp4E1,_tag_fat(_tmp9F,sizeof(void*),1U));});});return 1;case 7U: _LLF: _LL10:
# 546
({struct Cyc_String_pa_PrintArg_struct _tmpA4=({struct Cyc_String_pa_PrintArg_struct _tmp43E;_tmp43E.tag=0U,({struct _fat_ptr _tmp4E3=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);}));_tmp43E.f1=_tmp4E3;});_tmp43E;});void*_tmpA2[1U];_tmpA2[0]=& _tmpA4;({struct Cyc___cycFILE*_tmp4E5=(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out);struct _fat_ptr _tmp4E4=({const char*_tmpA3="%s";_tag_fat(_tmpA3,sizeof(char),3U);});Cyc_fprintf(_tmp4E5,_tmp4E4,_tag_fat(_tmpA2,sizeof(void*),1U));});});return 1;case 8U: _LL11: _LL12:
# 548
({struct Cyc_String_pa_PrintArg_struct _tmpA7=({struct Cyc_String_pa_PrintArg_struct _tmp43F;_tmp43F.tag=0U,({struct _fat_ptr _tmp4E6=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);}));_tmp43F.f1=_tmp4E6;});_tmp43F;});void*_tmpA5[1U];_tmpA5[0]=& _tmpA7;({struct Cyc___cycFILE*_tmp4E8=(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out);struct _fat_ptr _tmp4E7=({const char*_tmpA6="%s";_tag_fat(_tmpA6,sizeof(char),3U);});Cyc_fprintf(_tmp4E8,_tmp4E7,_tag_fat(_tmpA5,sizeof(void*),1U));});});return 1;default: _LL13: _LL14:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_slurp_string_rec(lexbuf,lexstate);});}_LL0:;}
# 552
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmpA9=_cycalloc(sizeof(*_tmpA9));_tmpA9->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4E9=({const char*_tmpA8="some action didn't return!";_tag_fat(_tmpA8,sizeof(char),27U);});_tmpA9->f1=_tmp4E9;});_tmpA9;}));}
# 554
int Cyc_slurp_string(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_slurp_string_rec(lexbuf,6);});}
int Cyc_asmtok_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmpAC=lexstate;switch(_tmpAC){case 0U: _LL1: _LL2:
# 558 "buildlib.cyl"
 return 0;case 1U: _LL3: _LL4:
# 560
 if(Cyc_parens_to_match == 1)return 0;-- Cyc_parens_to_match;
# 562
return 1;case 2U: _LL5: _LL6:
# 564
 ++ Cyc_parens_to_match;
return 1;case 3U: _LL7: _LL8:
# 567
 while(({Cyc_asm_string(lexbuf);})){;}
return 1;case 4U: _LL9: _LLA:
# 570
 while(({Cyc_asm_comment(lexbuf);})){;}
return 1;case 5U: _LLB: _LLC:
# 573
 return 1;case 6U: _LLD: _LLE:
# 575
 return 1;default: _LLF: _LL10:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_asmtok_rec(lexbuf,lexstate);});}_LL0:;}
# 579
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmpAE=_cycalloc(sizeof(*_tmpAE));_tmpAE->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4EA=({const char*_tmpAD="some action didn't return!";_tag_fat(_tmpAD,sizeof(char),27U);});_tmpAE->f1=_tmp4EA;});_tmpAE;}));}
# 581
int Cyc_asmtok(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_asmtok_rec(lexbuf,7);});}
int Cyc_asm_string_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmpB1=lexstate;switch(_tmpB1){case 0U: _LL1: _LL2:
# 579 "buildlib.cyl"
({void*_tmpB2=0U;({struct _fat_ptr _tmp4EB=({const char*_tmpB3="Warning: unclosed string\n";_tag_fat(_tmpB3,sizeof(char),26U);});Cyc_log(_tmp4EB,_tag_fat(_tmpB2,sizeof(void*),0U));});});return 0;case 1U: _LL3: _LL4:
# 581
 return 0;case 2U: _LL5: _LL6:
# 583
({void*_tmpB4=0U;({struct _fat_ptr _tmp4EC=({const char*_tmpB5="Warning: unclosed string\n";_tag_fat(_tmpB5,sizeof(char),26U);});Cyc_log(_tmp4EC,_tag_fat(_tmpB4,sizeof(void*),0U));});});return 1;case 3U: _LL7: _LL8:
# 585
 return 1;case 4U: _LL9: _LLA:
# 587
 return 1;case 5U: _LLB: _LLC:
# 589
 return 1;case 6U: _LLD: _LLE:
# 591
 return 1;case 7U: _LLF: _LL10:
# 593
 return 1;case 8U: _LL11: _LL12:
# 595
 return 1;default: _LL13: _LL14:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_asm_string_rec(lexbuf,lexstate);});}_LL0:;}
# 599
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmpB7=_cycalloc(sizeof(*_tmpB7));_tmpB7->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4ED=({const char*_tmpB6="some action didn't return!";_tag_fat(_tmpB6,sizeof(char),27U);});_tmpB7->f1=_tmp4ED;});_tmpB7;}));}
# 601
int Cyc_asm_string(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_asm_string_rec(lexbuf,8);});}
int Cyc_asm_comment_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmpBA=lexstate;switch(_tmpBA){case 0U: _LL1: _LL2:
# 599 "buildlib.cyl"
({void*_tmpBB=0U;({struct _fat_ptr _tmp4EE=({const char*_tmpBC="Warning: unclosed comment\n";_tag_fat(_tmpBC,sizeof(char),27U);});Cyc_log(_tmp4EE,_tag_fat(_tmpBB,sizeof(void*),0U));});});return 0;case 1U: _LL3: _LL4:
# 601
 return 0;case 2U: _LL5: _LL6:
# 603
 return 1;default: _LL7: _LL8:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_asm_comment_rec(lexbuf,lexstate);});}_LL0:;}
# 607
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmpBE=_cycalloc(sizeof(*_tmpBE));_tmpBE->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4EF=({const char*_tmpBD="some action didn't return!";_tag_fat(_tmpBD,sizeof(char),27U);});_tmpBE->f1=_tmp4EF;});_tmpBE;}));}
# 609
int Cyc_asm_comment(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_asm_comment_rec(lexbuf,9);});}struct _tuple26*Cyc_suck_line_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
# 611
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmpC1=lexstate;switch(_tmpC1){case 0U: _LL1: _LL2:
# 611 "buildlib.cyl"
 Cyc_current_line=({const char*_tmpC2="#define ";_tag_fat(_tmpC2,sizeof(char),9U);});
({Cyc_suck_macroname(lexbuf);});
return({struct _tuple26*_tmpC3=_cycalloc(sizeof(*_tmpC3));_tmpC3->f1=Cyc_current_line,_tmpC3->f2=(struct _fat_ptr*)_check_null(Cyc_current_source);_tmpC3;});case 1U: _LL3: _LL4:
# 615
 return({Cyc_suck_line(lexbuf);});case 2U: _LL5: _LL6:
# 617
 return 0;default: _LL7: _LL8:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_suck_line_rec(lexbuf,lexstate);});}_LL0:;}
# 621
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmpC5=_cycalloc(sizeof(*_tmpC5));_tmpC5->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4F0=({const char*_tmpC4="some action didn't return!";_tag_fat(_tmpC4,sizeof(char),27U);});_tmpC5->f1=_tmp4F0;});_tmpC5;}));}
# 609 "buildlib.cyl"
struct _tuple26*Cyc_suck_line(struct Cyc_Lexing_lexbuf*lexbuf){
# 623 "buildlib.cyl"
return({Cyc_suck_line_rec(lexbuf,10);});}
int Cyc_suck_macroname_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmpC8=lexstate;if(_tmpC8 == 0){_LL1: _LL2:
# 621 "buildlib.cyl"
 Cyc_current_source=({struct _fat_ptr*_tmpC9=_cycalloc(sizeof(*_tmpC9));({struct _fat_ptr _tmp4F1=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});*_tmpC9=_tmp4F1;});_tmpC9;});
Cyc_current_line=(struct _fat_ptr)({Cyc_strconcat((struct _fat_ptr)Cyc_current_line,(struct _fat_ptr)*((struct _fat_ptr*)_check_null(Cyc_current_source)));});
return({Cyc_suck_restofline(lexbuf);});}else{_LL3: _LL4:
# 625
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_suck_macroname_rec(lexbuf,lexstate);});}_LL0:;}
# 628
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmpCB=_cycalloc(sizeof(*_tmpCB));_tmpCB->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4F2=({const char*_tmpCA="some action didn't return!";_tag_fat(_tmpCA,sizeof(char),27U);});_tmpCB->f1=_tmp4F2;});_tmpCB;}));}
# 630
int Cyc_suck_macroname(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_suck_macroname_rec(lexbuf,11);});}
int Cyc_suck_restofline_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmpCE=lexstate;if(_tmpCE == 0){_LL1: _LL2:
# 628 "buildlib.cyl"
 Cyc_current_line=(struct _fat_ptr)({struct _fat_ptr(*_tmpCF)(struct _fat_ptr,struct _fat_ptr)=Cyc_strconcat;struct _fat_ptr _tmpD0=(struct _fat_ptr)Cyc_current_line;struct _fat_ptr _tmpD1=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmpCF(_tmpD0,_tmpD1);});return 0;}else{_LL3: _LL4:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_suck_restofline_rec(lexbuf,lexstate);});}_LL0:;}
# 632
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmpD3=_cycalloc(sizeof(*_tmpD3));_tmpD3->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4F3=({const char*_tmpD2="some action didn't return!";_tag_fat(_tmpD2,sizeof(char),27U);});_tmpD3->f1=_tmp4F3;});_tmpD3;}));}
# 634
int Cyc_suck_restofline(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_suck_restofline_rec(lexbuf,12);});}struct _tuple29*Cyc_spec_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
# 636
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmpD6=lexstate;switch(_tmpD6){case 0U: _LL1: _LL2:
# 635 "buildlib.cyl"
 return({Cyc_spec(lexbuf);});case 1U: _LL3: _LL4:
# 637
 Cyc_current_headerfile=(struct _fat_ptr)({struct _fat_ptr(*_tmpD7)(struct _fat_ptr,int ofs,unsigned long n)=Cyc_substring;struct _fat_ptr _tmpD8=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});int _tmpD9=0;unsigned long _tmpDA=(unsigned long)(({
# 639
int _tmp4F4=({Cyc_Lexing_lexeme_end(lexbuf);});_tmp4F4 - ({Cyc_Lexing_lexeme_start(lexbuf);});})- 1);_tmpD7(_tmpD8,_tmpD9,_tmpDA);});
Cyc_current_symbols=0;
Cyc_current_user_defs=0;
Cyc_current_omit_symbols=0;
Cyc_current_cstubs=0;
Cyc_current_cycstubs=0;
Cyc_current_hstubs=0;
Cyc_current_cpp=0;
while(({Cyc_commands(lexbuf);})){;}
Cyc_current_hstubs=({Cyc_List_imp_rev(Cyc_current_hstubs);});
Cyc_current_cstubs=({Cyc_List_imp_rev(Cyc_current_cstubs);});
Cyc_current_cycstubs=({Cyc_List_imp_rev(Cyc_current_cycstubs);});
Cyc_current_cpp=({Cyc_List_imp_rev(Cyc_current_cpp);});
return({struct _tuple29*_tmpDB=_cycalloc(sizeof(*_tmpDB));_tmpDB->f1=Cyc_current_headerfile,_tmpDB->f2=Cyc_current_symbols,_tmpDB->f3=Cyc_current_user_defs,_tmpDB->f4=Cyc_current_omit_symbols,_tmpDB->f5=Cyc_current_hstubs,_tmpDB->f6=Cyc_current_cstubs,_tmpDB->f7=Cyc_current_cycstubs,_tmpDB->f8=Cyc_current_cpp;_tmpDB;});case 2U: _LL5: _LL6:
# 662
 return({Cyc_spec(lexbuf);});case 3U: _LL7: _LL8:
# 664
 return 0;case 4U: _LL9: _LLA:
# 666
({struct Cyc_Int_pa_PrintArg_struct _tmpDE=({struct Cyc_Int_pa_PrintArg_struct _tmp440;_tmp440.tag=1U,({
# 668
unsigned long _tmp4F5=(unsigned long)((int)({Cyc_Lexing_lexeme_char(lexbuf,0);}));_tmp440.f1=_tmp4F5;});_tmp440;});void*_tmpDC[1U];_tmpDC[0]=& _tmpDE;({struct Cyc___cycFILE*_tmp4F7=Cyc_stderr;struct _fat_ptr _tmp4F6=({const char*_tmpDD="Error in .cys file: expected header file name, found '%c' instead\n";_tag_fat(_tmpDD,sizeof(char),67U);});Cyc_fprintf(_tmp4F7,_tmp4F6,_tag_fat(_tmpDC,sizeof(void*),1U));});});
return 0;default: _LLB: _LLC:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_spec_rec(lexbuf,lexstate);});}_LL0:;}
# 673
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmpE0=_cycalloc(sizeof(*_tmpE0));_tmpE0->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp4F8=({const char*_tmpDF="some action didn't return!";_tag_fat(_tmpDF,sizeof(char),27U);});_tmpE0->f1=_tmp4F8;});_tmpE0;}));}
# 634 "buildlib.cyl"
struct _tuple29*Cyc_spec(struct Cyc_Lexing_lexbuf*lexbuf){
# 675 "buildlib.cyl"
return({Cyc_spec_rec(lexbuf,13);});}
int Cyc_commands_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmpE3=lexstate;switch(_tmpE3){case 0U: _LL1: _LL2:
# 673 "buildlib.cyl"
 return 0;case 1U: _LL3: _LL4:
# 675
 return 0;case 2U: _LL5: _LL6:
# 677
 Cyc_snarfed_symbols=0;
while(({Cyc_snarfsymbols(lexbuf);})){;}
Cyc_current_symbols=({Cyc_List_append(Cyc_snarfed_symbols,Cyc_current_symbols);});
return 1;case 3U: _LL7: _LL8:
# 682
 Cyc_snarfed_symbols=0;{
struct Cyc_List_List*tmp_user_defs=Cyc_current_user_defs;
while(({Cyc_snarfsymbols(lexbuf);})){;}
if(tmp_user_defs != Cyc_current_user_defs){
({void*_tmpE4=0U;({struct Cyc___cycFILE*_tmp4FA=Cyc_stderr;struct _fat_ptr _tmp4F9=({const char*_tmpE5="Error in .cys file: got optional definition in omitsymbols\n";_tag_fat(_tmpE5,sizeof(char),60U);});Cyc_fprintf(_tmp4FA,_tmp4F9,_tag_fat(_tmpE4,sizeof(void*),0U));});});
# 688
return 0;}
# 685
Cyc_current_omit_symbols=({Cyc_List_append(Cyc_snarfed_symbols,Cyc_current_omit_symbols);});
# 691
return 1;}case 4U: _LL9: _LLA:
# 693
 Cyc_braces_to_match=1;
Cyc_specbuf=({Cyc_Buffer_create(255U);});
while(({Cyc_block(lexbuf);})){;}{
struct _tuple27*x=({struct _tuple27*_tmpE7=_cycalloc(sizeof(*_tmpE7));({struct _fat_ptr _tmp4FC=(struct _fat_ptr)_tag_fat(0,0,0);_tmpE7->f1=_tmp4FC;}),({
struct _fat_ptr _tmp4FB=(struct _fat_ptr)({Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf));});_tmpE7->f2=_tmp4FB;});_tmpE7;});
Cyc_current_hstubs=({struct Cyc_List_List*_tmpE6=_cycalloc(sizeof(*_tmpE6));_tmpE6->hd=x,_tmpE6->tl=Cyc_current_hstubs;_tmpE6;});
return 1;}case 5U: _LLB: _LLC: {
# 701
struct _fat_ptr s=({Cyc_Lexing_lexeme(lexbuf);});
_fat_ptr_inplace_plus(& s,sizeof(char),(int)({Cyc_strlen(({const char*_tmpE8="hstub";_tag_fat(_tmpE8,sizeof(char),6U);}));}));
while(({isspace((int)*((char*)_check_fat_subscript(s,sizeof(char),0U)));})){_fat_ptr_inplace_plus(& s,sizeof(char),1);}{
struct _fat_ptr t=s;
while(!({isspace((int)*((char*)_check_fat_subscript(t,sizeof(char),0U)));})){_fat_ptr_inplace_plus(& t,sizeof(char),1);}{
struct _fat_ptr symbol=({Cyc_substring((struct _fat_ptr)s,0,(unsigned long)((t.curr - s.curr)/ sizeof(char)));});
Cyc_braces_to_match=1;
Cyc_specbuf=({Cyc_Buffer_create(255U);});
while(({Cyc_block(lexbuf);})){;}{
struct _tuple27*x=({struct _tuple27*_tmpEA=_cycalloc(sizeof(*_tmpEA));_tmpEA->f1=(struct _fat_ptr)symbol,({
struct _fat_ptr _tmp4FD=(struct _fat_ptr)({Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf));});_tmpEA->f2=_tmp4FD;});_tmpEA;});
Cyc_current_hstubs=({struct Cyc_List_List*_tmpE9=_cycalloc(sizeof(*_tmpE9));_tmpE9->hd=x,_tmpE9->tl=Cyc_current_hstubs;_tmpE9;});
return 1;}}}}case 6U: _LLD: _LLE:
# 715
 Cyc_braces_to_match=1;
Cyc_specbuf=({Cyc_Buffer_create(255U);});
while(({Cyc_block(lexbuf);})){;}{
struct _tuple27*x=({struct _tuple27*_tmpEC=_cycalloc(sizeof(*_tmpEC));({struct _fat_ptr _tmp4FF=(struct _fat_ptr)_tag_fat(0,0,0);_tmpEC->f1=_tmp4FF;}),({
struct _fat_ptr _tmp4FE=(struct _fat_ptr)({Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf));});_tmpEC->f2=_tmp4FE;});_tmpEC;});
Cyc_current_cstubs=({struct Cyc_List_List*_tmpEB=_cycalloc(sizeof(*_tmpEB));_tmpEB->hd=x,_tmpEB->tl=Cyc_current_cstubs;_tmpEB;});
return 1;}case 7U: _LLF: _LL10: {
# 723
struct _fat_ptr s=({Cyc_Lexing_lexeme(lexbuf);});
_fat_ptr_inplace_plus(& s,sizeof(char),(int)({Cyc_strlen(({const char*_tmpED="cstub";_tag_fat(_tmpED,sizeof(char),6U);}));}));
while(({isspace((int)*((char*)_check_fat_subscript(s,sizeof(char),0U)));})){_fat_ptr_inplace_plus(& s,sizeof(char),1);}{
struct _fat_ptr t=s;
while(!({isspace((int)*((char*)_check_fat_subscript(t,sizeof(char),0U)));})){_fat_ptr_inplace_plus(& t,sizeof(char),1);}{
struct _fat_ptr symbol=({Cyc_substring((struct _fat_ptr)s,0,(unsigned long)((t.curr - s.curr)/ sizeof(char)));});
Cyc_braces_to_match=1;
Cyc_specbuf=({Cyc_Buffer_create(255U);});
while(({Cyc_block(lexbuf);})){;}{
struct _tuple27*x=({struct _tuple27*_tmpEF=_cycalloc(sizeof(*_tmpEF));_tmpEF->f1=(struct _fat_ptr)symbol,({
struct _fat_ptr _tmp500=(struct _fat_ptr)({Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf));});_tmpEF->f2=_tmp500;});_tmpEF;});
Cyc_current_cstubs=({struct Cyc_List_List*_tmpEE=_cycalloc(sizeof(*_tmpEE));_tmpEE->hd=x,_tmpEE->tl=Cyc_current_cstubs;_tmpEE;});
return 1;}}}}case 8U: _LL11: _LL12:
# 737
 Cyc_braces_to_match=1;
Cyc_specbuf=({Cyc_Buffer_create(255U);});
while(({Cyc_block(lexbuf);})){;}{
struct _tuple27*x=({struct _tuple27*_tmpF1=_cycalloc(sizeof(*_tmpF1));({struct _fat_ptr _tmp502=(struct _fat_ptr)_tag_fat(0,0,0);_tmpF1->f1=_tmp502;}),({
struct _fat_ptr _tmp501=(struct _fat_ptr)({Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf));});_tmpF1->f2=_tmp501;});_tmpF1;});
Cyc_current_cycstubs=({struct Cyc_List_List*_tmpF0=_cycalloc(sizeof(*_tmpF0));_tmpF0->hd=x,_tmpF0->tl=Cyc_current_cycstubs;_tmpF0;});
return 1;}case 9U: _LL13: _LL14: {
# 745
struct _fat_ptr s=({Cyc_Lexing_lexeme(lexbuf);});
_fat_ptr_inplace_plus(& s,sizeof(char),(int)({Cyc_strlen(({const char*_tmpF2="cycstub";_tag_fat(_tmpF2,sizeof(char),8U);}));}));
while(({isspace((int)*((char*)_check_fat_subscript(s,sizeof(char),0U)));})){_fat_ptr_inplace_plus(& s,sizeof(char),1);}{
struct _fat_ptr t=s;
while(!({isspace((int)*((char*)_check_fat_subscript(t,sizeof(char),0U)));})){_fat_ptr_inplace_plus(& t,sizeof(char),1);}{
struct _fat_ptr symbol=({Cyc_substring((struct _fat_ptr)s,0,(unsigned long)((t.curr - s.curr)/ sizeof(char)));});
Cyc_braces_to_match=1;
Cyc_specbuf=({Cyc_Buffer_create(255U);});
while(({Cyc_block(lexbuf);})){;}{
struct _tuple27*x=({struct _tuple27*_tmpF4=_cycalloc(sizeof(*_tmpF4));_tmpF4->f1=(struct _fat_ptr)symbol,({
struct _fat_ptr _tmp503=(struct _fat_ptr)({Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf));});_tmpF4->f2=_tmp503;});_tmpF4;});
Cyc_current_cycstubs=({struct Cyc_List_List*_tmpF3=_cycalloc(sizeof(*_tmpF3));_tmpF3->hd=x,_tmpF3->tl=Cyc_current_cycstubs;_tmpF3;});
return 1;}}}}case 10U: _LL15: _LL16:
# 759
 Cyc_braces_to_match=1;
Cyc_specbuf=({Cyc_Buffer_create(255U);});
while(({Cyc_block(lexbuf);})){;}{
struct _fat_ptr*x=({struct _fat_ptr*_tmpF6=_cycalloc(sizeof(*_tmpF6));({struct _fat_ptr _tmp504=(struct _fat_ptr)({Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf));});*_tmpF6=_tmp504;});_tmpF6;});
Cyc_current_cpp=({struct Cyc_List_List*_tmpF5=_cycalloc(sizeof(*_tmpF5));_tmpF5->hd=x,_tmpF5->tl=Cyc_current_cpp;_tmpF5;});
return 1;}case 11U: _LL17: _LL18:
# 766
 return 1;case 12U: _LL19: _LL1A:
# 768
 return 1;case 13U: _LL1B: _LL1C:
# 770
({struct Cyc_Int_pa_PrintArg_struct _tmpF9=({struct Cyc_Int_pa_PrintArg_struct _tmp441;_tmp441.tag=1U,({
# 772
unsigned long _tmp505=(unsigned long)((int)({Cyc_Lexing_lexeme_char(lexbuf,0);}));_tmp441.f1=_tmp505;});_tmp441;});void*_tmpF7[1U];_tmpF7[0]=& _tmpF9;({struct Cyc___cycFILE*_tmp507=Cyc_stderr;struct _fat_ptr _tmp506=({const char*_tmpF8="Error in .cys file: expected command, found '%c' instead\n";_tag_fat(_tmpF8,sizeof(char),58U);});Cyc_fprintf(_tmp507,_tmp506,_tag_fat(_tmpF7,sizeof(void*),1U));});});
return 0;default: _LL1D: _LL1E:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_commands_rec(lexbuf,lexstate);});}_LL0:;}
# 777
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmpFB=_cycalloc(sizeof(*_tmpFB));_tmpFB->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp508=({const char*_tmpFA="some action didn't return!";_tag_fat(_tmpFA,sizeof(char),27U);});_tmpFB->f1=_tmp508;});_tmpFB;}));}
# 779
int Cyc_commands(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_commands_rec(lexbuf,14);});}
int Cyc_snarfsymbols_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmpFE=lexstate;switch(_tmpFE){case 0U: _LL1: _LL2:
# 783 "buildlib.cyl"
 Cyc_snarfed_symbols=({struct Cyc_List_List*_tmp100=_cycalloc(sizeof(*_tmp100));({struct _fat_ptr*_tmp50A=({struct _fat_ptr*_tmpFF=_cycalloc(sizeof(*_tmpFF));({struct _fat_ptr _tmp509=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});*_tmpFF=_tmp509;});_tmpFF;});_tmp100->hd=_tmp50A;}),_tmp100->tl=Cyc_snarfed_symbols;_tmp100;});
return 1;case 1U: _LL3: _LL4: {
# 786
struct _fat_ptr s=({Cyc_Lexing_lexeme(lexbuf);});
struct _fat_ptr t=s;
while(!({isspace((int)*((char*)_check_fat_subscript(t,sizeof(char),0U)));})){_fat_ptr_inplace_plus(& t,sizeof(char),1);}
Cyc_current_symbol=(struct _fat_ptr)({Cyc_substring((struct _fat_ptr)s,0,(unsigned long)((t.curr - s.curr)/ sizeof(char)));});
Cyc_rename_current_symbol=1;
Cyc_braces_to_match=1;
Cyc_specbuf=({Cyc_Buffer_create(255U);});
while(({Cyc_block(lexbuf);})){;}
# 795
Cyc_rename_current_symbol=0;{
struct _tuple28*user_def=({struct _tuple28*_tmp106=_cycalloc(sizeof(*_tmp106));({struct _fat_ptr*_tmp50D=({struct _fat_ptr*_tmp104=_cycalloc(sizeof(*_tmp104));*_tmp104=Cyc_current_symbol;_tmp104;});_tmp106->f1=_tmp50D;}),({
struct _fat_ptr*_tmp50C=({struct _fat_ptr*_tmp105=_cycalloc(sizeof(*_tmp105));({struct _fat_ptr _tmp50B=(struct _fat_ptr)({Cyc_Buffer_contents((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf));});*_tmp105=_tmp50B;});_tmp105;});_tmp106->f2=_tmp50C;});_tmp106;});
Cyc_snarfed_symbols=({struct Cyc_List_List*_tmp102=_cycalloc(sizeof(*_tmp102));({struct _fat_ptr*_tmp50E=({struct _fat_ptr*_tmp101=_cycalloc(sizeof(*_tmp101));*_tmp101=(struct _fat_ptr)Cyc_current_symbol;_tmp101;});_tmp102->hd=_tmp50E;}),_tmp102->tl=Cyc_snarfed_symbols;_tmp102;});
Cyc_current_user_defs=({struct Cyc_List_List*_tmp103=_cycalloc(sizeof(*_tmp103));_tmp103->hd=user_def,_tmp103->tl=Cyc_current_user_defs;_tmp103;});
return 1;}}case 2U: _LL5: _LL6:
# 802
 return 1;case 3U: _LL7: _LL8:
# 804
 return 0;case 4U: _LL9: _LLA:
# 806
({void*_tmp107=0U;({struct Cyc___cycFILE*_tmp510=Cyc_stderr;struct _fat_ptr _tmp50F=({const char*_tmp108="Error in .cys file: unexpected end-of-file\n";_tag_fat(_tmp108,sizeof(char),44U);});Cyc_fprintf(_tmp510,_tmp50F,_tag_fat(_tmp107,sizeof(void*),0U));});});
# 808
return 0;case 5U: _LLB: _LLC:
# 810
({struct Cyc_Int_pa_PrintArg_struct _tmp10B=({struct Cyc_Int_pa_PrintArg_struct _tmp442;_tmp442.tag=1U,({
# 812
unsigned long _tmp511=(unsigned long)((int)({Cyc_Lexing_lexeme_char(lexbuf,0);}));_tmp442.f1=_tmp511;});_tmp442;});void*_tmp109[1U];_tmp109[0]=& _tmp10B;({struct Cyc___cycFILE*_tmp513=Cyc_stderr;struct _fat_ptr _tmp512=({const char*_tmp10A="Error in .cys file: expected symbol, found '%c' instead\n";_tag_fat(_tmp10A,sizeof(char),57U);});Cyc_fprintf(_tmp513,_tmp512,_tag_fat(_tmp109,sizeof(void*),1U));});});
return 0;default: _LLD: _LLE:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_snarfsymbols_rec(lexbuf,lexstate);});}_LL0:;}
# 817
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp10D=_cycalloc(sizeof(*_tmp10D));_tmp10D->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp514=({const char*_tmp10C="some action didn't return!";_tag_fat(_tmp10C,sizeof(char),27U);});_tmp10D->f1=_tmp514;});_tmp10D;}));}
# 819
int Cyc_snarfsymbols(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_snarfsymbols_rec(lexbuf,15);});}
int Cyc_block_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp110=lexstate;switch(_tmp110){case 0U: _LL1: _LL2:
# 823 "buildlib.cyl"
({void*_tmp111=0U;({struct _fat_ptr _tmp515=({const char*_tmp112="Warning: unclosed brace\n";_tag_fat(_tmp112,sizeof(char),25U);});Cyc_log(_tmp515,_tag_fat(_tmp111,sizeof(void*),0U));});});return 0;case 1U: _LL3: _LL4:
# 825
 if(Cyc_braces_to_match == 1)return 0;-- Cyc_braces_to_match;
# 827
({Cyc_Buffer_add_char((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),'}');});
return 1;case 2U: _LL5: _LL6:
# 830
 ++ Cyc_braces_to_match;
({Cyc_Buffer_add_char((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),'{');});
return 1;case 3U: _LL7: _LL8:
# 834
({Cyc_Buffer_add_char((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),'"');});
while(({Cyc_block_string(lexbuf);})){;}
return 1;case 4U: _LL9: _LLA:
# 838
({({struct Cyc_Buffer_t*_tmp516=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);Cyc_Buffer_add_string(_tmp516,({const char*_tmp113="/*";_tag_fat(_tmp113,sizeof(char),3U);}));});});
while(({Cyc_block_comment(lexbuf);})){;}
return 1;case 5U: _LLB: _LLC:
# 842
({void(*_tmp114)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp115=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp116=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp114(_tmp115,_tmp116);});
return 1;case 6U: _LLD: _LLE: {
# 845
struct _fat_ptr symbol=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});
if(Cyc_rename_current_symbol && !({Cyc_strcmp((struct _fat_ptr)symbol,(struct _fat_ptr)Cyc_current_symbol);}))
({void(*_tmp117)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp118=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp119=*({Cyc_add_user_prefix(({struct _fat_ptr*_tmp11A=_cycalloc(sizeof(*_tmp11A));*_tmp11A=symbol;_tmp11A;}));});_tmp117(_tmp118,_tmp119);});else{
# 849
({Cyc_Buffer_add_string((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),symbol);});}
return 1;}case 7U: _LLF: _LL10:
# 852
({void(*_tmp11B)(struct Cyc_Buffer_t*,char)=Cyc_Buffer_add_char;struct Cyc_Buffer_t*_tmp11C=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);char _tmp11D=({Cyc_Lexing_lexeme_char(lexbuf,0);});_tmp11B(_tmp11C,_tmp11D);});
return 1;default: _LL11: _LL12:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_block_rec(lexbuf,lexstate);});}_LL0:;}
# 857
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp11F=_cycalloc(sizeof(*_tmp11F));_tmp11F->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp517=({const char*_tmp11E="some action didn't return!";_tag_fat(_tmp11E,sizeof(char),27U);});_tmp11F->f1=_tmp517;});_tmp11F;}));}
# 859
int Cyc_block(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_block_rec(lexbuf,16);});}
int Cyc_block_string_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp122=lexstate;switch(_tmp122){case 0U: _LL1: _LL2:
# 857 "buildlib.cyl"
({void*_tmp123=0U;({struct _fat_ptr _tmp518=({const char*_tmp124="Warning: unclosed string\n";_tag_fat(_tmp124,sizeof(char),26U);});Cyc_log(_tmp518,_tag_fat(_tmp123,sizeof(void*),0U));});});return 0;case 1U: _LL3: _LL4:
# 859
({Cyc_Buffer_add_char((struct Cyc_Buffer_t*)_check_null(Cyc_specbuf),'"');});return 0;case 2U: _LL5: _LL6:
# 861
({void*_tmp125=0U;({struct _fat_ptr _tmp519=({const char*_tmp126="Warning: unclosed string\n";_tag_fat(_tmp126,sizeof(char),26U);});Cyc_log(_tmp519,_tag_fat(_tmp125,sizeof(void*),0U));});});
({void(*_tmp127)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp128=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp129=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp127(_tmp128,_tmp129);});
return 1;case 3U: _LL7: _LL8:
# 865
({void(*_tmp12A)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp12B=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp12C=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp12A(_tmp12B,_tmp12C);});
return 1;case 4U: _LL9: _LLA:
# 868
({void(*_tmp12D)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp12E=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp12F=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp12D(_tmp12E,_tmp12F);});
return 1;case 5U: _LLB: _LLC:
# 871
({void(*_tmp130)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp131=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp132=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp130(_tmp131,_tmp132);});
return 1;case 6U: _LLD: _LLE:
# 874
({void(*_tmp133)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp134=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp135=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp133(_tmp134,_tmp135);});
return 1;case 7U: _LLF: _LL10:
# 877
({void(*_tmp136)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp137=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp138=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp136(_tmp137,_tmp138);});
return 1;case 8U: _LL11: _LL12:
# 880
({void(*_tmp139)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp13A=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp13B=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp139(_tmp13A,_tmp13B);});
return 1;default: _LL13: _LL14:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_block_string_rec(lexbuf,lexstate);});}_LL0:;}
# 885
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp13D=_cycalloc(sizeof(*_tmp13D));_tmp13D->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp51A=({const char*_tmp13C="some action didn't return!";_tag_fat(_tmp13C,sizeof(char),27U);});_tmp13D->f1=_tmp51A;});_tmp13D;}));}
# 887
int Cyc_block_string(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_block_string_rec(lexbuf,17);});}
int Cyc_block_comment_rec(struct Cyc_Lexing_lexbuf*lexbuf,int lexstate){
lexstate=({Cyc_lex_engine(lexstate,lexbuf);});
{int _tmp140=lexstate;switch(_tmp140){case 0U: _LL1: _LL2:
# 885 "buildlib.cyl"
({void*_tmp141=0U;({struct _fat_ptr _tmp51B=({const char*_tmp142="Warning: unclosed comment\n";_tag_fat(_tmp142,sizeof(char),27U);});Cyc_log(_tmp51B,_tag_fat(_tmp141,sizeof(void*),0U));});});return 0;case 1U: _LL3: _LL4:
# 887
({({struct Cyc_Buffer_t*_tmp51C=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);Cyc_Buffer_add_string(_tmp51C,({const char*_tmp143="*/";_tag_fat(_tmp143,sizeof(char),3U);}));});});return 0;case 2U: _LL5: _LL6:
# 889
({void(*_tmp144)(struct Cyc_Buffer_t*,struct _fat_ptr)=Cyc_Buffer_add_string;struct Cyc_Buffer_t*_tmp145=(struct Cyc_Buffer_t*)_check_null(Cyc_specbuf);struct _fat_ptr _tmp146=(struct _fat_ptr)({Cyc_Lexing_lexeme(lexbuf);});_tmp144(_tmp145,_tmp146);});
return 1;default: _LL7: _LL8:
({(lexbuf->refill_buff)(lexbuf);});
return({Cyc_block_comment_rec(lexbuf,lexstate);});}_LL0:;}
# 894
(int)_throw(({struct Cyc_Lexing_Error_exn_struct*_tmp148=_cycalloc(sizeof(*_tmp148));_tmp148->tag=Cyc_Lexing_Error,({struct _fat_ptr _tmp51D=({const char*_tmp147="some action didn't return!";_tag_fat(_tmp147,sizeof(char),27U);});_tmp148->f1=_tmp51D;});_tmp148;}));}
# 896
int Cyc_block_comment(struct Cyc_Lexing_lexbuf*lexbuf){return({Cyc_block_comment_rec(lexbuf,18);});}
# 902 "buildlib.cyl"
void Cyc_scan_type(void*t,struct Cyc_Hashtable_Table*dep);struct _tuple30{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
void Cyc_scan_exp(struct Cyc_Absyn_Exp*e,struct Cyc_Hashtable_Table*dep){
void*_stmttmp1=((struct Cyc_Absyn_Exp*)_check_null(e))->r;void*_tmp14B=_stmttmp1;struct Cyc_List_List*_tmp14C;struct Cyc_List_List*_tmp14E;void*_tmp14D;struct _fat_ptr*_tmp150;struct Cyc_Absyn_Exp*_tmp14F;struct _fat_ptr*_tmp152;struct Cyc_Absyn_Exp*_tmp151;void*_tmp153;void*_tmp154;struct Cyc_Absyn_Exp*_tmp155;struct Cyc_Absyn_Exp*_tmp159;void**_tmp158;struct Cyc_Absyn_Exp*_tmp157;int _tmp156;struct Cyc_Absyn_Exp*_tmp15B;void*_tmp15A;struct Cyc_List_List*_tmp15D;struct Cyc_Absyn_Exp*_tmp15C;struct Cyc_Absyn_Exp*_tmp15F;struct Cyc_Absyn_Exp*_tmp15E;struct Cyc_Absyn_Exp*_tmp161;struct Cyc_Absyn_Exp*_tmp160;struct Cyc_Absyn_Exp*_tmp164;struct Cyc_Absyn_Exp*_tmp163;struct Cyc_Absyn_Exp*_tmp162;struct Cyc_Absyn_Exp*_tmp165;struct Cyc_Absyn_Exp*_tmp166;struct Cyc_Absyn_Exp*_tmp167;struct Cyc_Absyn_Exp*_tmp168;struct Cyc_Absyn_Exp*_tmp169;struct Cyc_Absyn_Exp*_tmp16B;struct Cyc_Absyn_Exp*_tmp16A;struct Cyc_Absyn_Exp*_tmp16D;struct Cyc_Absyn_Exp*_tmp16C;struct Cyc_Absyn_Exp*_tmp16F;struct Cyc_Absyn_Exp*_tmp16E;struct Cyc_List_List*_tmp170;void*_tmp171;switch(*((int*)_tmp14B)){case 1U: _LL1: _tmp171=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_LL2: {void*b=_tmp171;
# 906
struct _fat_ptr*v=(*({Cyc_Absyn_binding2qvar(b);})).f2;
({Cyc_add_target(v);});
return;}case 3U: _LL3: _tmp170=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL4: {struct Cyc_List_List*x=_tmp170;
# 910
for(0;x != 0;x=x->tl){
({Cyc_scan_exp((struct Cyc_Absyn_Exp*)x->hd,dep);});}
# 913
return;}case 23U: _LL5: _tmp16E=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp16F=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp16E;struct Cyc_Absyn_Exp*e2=_tmp16F;
# 915
_tmp16C=e1;_tmp16D=e2;goto _LL8;}case 9U: _LL7: _tmp16C=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp16D=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp16C;struct Cyc_Absyn_Exp*e2=_tmp16D;
# 917
_tmp16A=e1;_tmp16B=e2;goto _LLA;}case 4U: _LL9: _tmp16A=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp16B=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp14B)->f3;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp16A;struct Cyc_Absyn_Exp*e2=_tmp16B;
# 919
({Cyc_scan_exp(e1,dep);});
({Cyc_scan_exp(e2,dep);});
return;}case 42U: _LLB: _tmp169=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp169;
_tmp168=e1;goto _LLE;}case 20U: _LLD: _tmp168=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp168;
# 924
_tmp167=e1;goto _LL10;}case 18U: _LLF: _tmp167=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp167;
# 926
_tmp166=e1;goto _LL12;}case 15U: _LL11: _tmp166=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp166;
# 928
_tmp165=e1;goto _LL14;}case 5U: _LL13: _tmp165=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp165;
# 930
({Cyc_scan_exp(e1,dep);});
return;}case 6U: _LL15: _tmp162=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp163=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_tmp164=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp14B)->f3;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp162;struct Cyc_Absyn_Exp*e2=_tmp163;struct Cyc_Absyn_Exp*e3=_tmp164;
# 933
({Cyc_scan_exp(e1,dep);});
({Cyc_scan_exp(e2,dep);});
({Cyc_scan_exp(e3,dep);});
return;}case 7U: _LL17: _tmp160=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp161=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp160;struct Cyc_Absyn_Exp*e2=_tmp161;
_tmp15E=e1;_tmp15F=e2;goto _LL1A;}case 8U: _LL19: _tmp15E=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp15F=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp15E;struct Cyc_Absyn_Exp*e2=_tmp15F;
# 939
({Cyc_scan_exp(e1,dep);});
({Cyc_scan_exp(e2,dep);});
return;}case 10U: _LL1B: _tmp15C=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp15D=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp15C;struct Cyc_List_List*x=_tmp15D;
# 943
({Cyc_scan_exp(e1,dep);});
for(0;x != 0;x=x->tl){
({Cyc_scan_exp((struct Cyc_Absyn_Exp*)x->hd,dep);});}
# 947
return;}case 14U: _LL1D: _tmp15A=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp15B=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL1E: {void*t1=_tmp15A;struct Cyc_Absyn_Exp*e1=_tmp15B;
# 949
({Cyc_scan_type(t1,dep);});
({Cyc_scan_exp(e1,dep);});
return;}case 35U: _LL1F: _tmp156=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp14B)->f1).is_calloc;_tmp157=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp14B)->f1).rgn;_tmp158=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp14B)->f1).elt_type;_tmp159=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp14B)->f1).num_elts;_LL20: {int iscalloc=_tmp156;struct Cyc_Absyn_Exp*ropt=_tmp157;void**topt=_tmp158;struct Cyc_Absyn_Exp*e=_tmp159;
# 953
if(ropt != 0)({Cyc_scan_exp(ropt,dep);});if(topt != 0)
({Cyc_scan_type(*topt,dep);});
# 953
({Cyc_scan_exp(e,dep);});
# 956
return;}case 39U: _LL21: _tmp155=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_LL22: {struct Cyc_Absyn_Exp*e=_tmp155;
# 958
({Cyc_scan_exp(e,dep);});return;}case 40U: _LL23: _tmp154=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_LL24: {void*t1=_tmp154;
_tmp153=t1;goto _LL26;}case 17U: _LL25: _tmp153=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_LL26: {void*t1=_tmp153;
# 961
({Cyc_scan_type(t1,dep);});
return;}case 21U: _LL27: _tmp151=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp152=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp151;struct _fat_ptr*fn=_tmp152;
# 964
_tmp14F=e1;_tmp150=fn;goto _LL2A;}case 22U: _LL29: _tmp14F=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp150=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp14F;struct _fat_ptr*fn=_tmp150;
# 966
({Cyc_scan_exp(e1,dep);});
({Cyc_add_target(fn);});
return;}case 19U: _LL2B: _tmp14D=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp14B)->f1;_tmp14E=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL2C: {void*t1=_tmp14D;struct Cyc_List_List*f=_tmp14E;
# 970
({Cyc_scan_type(t1,dep);});
# 972
{void*_stmttmp2=(void*)((struct Cyc_List_List*)_check_null(f))->hd;void*_tmp172=_stmttmp2;struct _fat_ptr*_tmp173;if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp172)->tag == 0U){_LL58: _tmp173=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp172)->f1;_LL59: {struct _fat_ptr*fn=_tmp173;
({Cyc_add_target(fn);});goto _LL57;}}else{_LL5A: _LL5B:
 goto _LL57;}_LL57:;}
# 976
return;}case 0U: _LL2D: _LL2E:
# 978
 return;case 37U: _LL2F: _tmp14C=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp14B)->f2;_LL30: {struct Cyc_List_List*x=_tmp14C;
# 980
for(0;x != 0;x=x->tl){
struct _tuple30*_stmttmp3=(struct _tuple30*)x->hd;struct Cyc_Absyn_Exp*_tmp174;_LL5D: _tmp174=_stmttmp3->f2;_LL5E: {struct Cyc_Absyn_Exp*e1=_tmp174;
({Cyc_scan_exp(e1,dep);});}}
# 984
return;}case 41U: _LL31: _LL32:
 return;case 2U: _LL33: _LL34:
# 987
({void*_tmp175=0U;({struct Cyc___cycFILE*_tmp51F=Cyc_stderr;struct _fat_ptr _tmp51E=({const char*_tmp176="Error: unexpected Pragma_e\n";_tag_fat(_tmp176,sizeof(char),28U);});Cyc_fprintf(_tmp51F,_tmp51E,_tag_fat(_tmp175,sizeof(void*),0U));});});
({exit(1);});return;case 36U: _LL35: _LL36:
# 990
({void*_tmp177=0U;({struct Cyc___cycFILE*_tmp521=Cyc_stderr;struct _fat_ptr _tmp520=({const char*_tmp178="Error: unexpected Swap_e\n";_tag_fat(_tmp178,sizeof(char),26U);});Cyc_fprintf(_tmp521,_tmp520,_tag_fat(_tmp177,sizeof(void*),0U));});});
({exit(1);});return;case 38U: _LL37: _LL38:
# 993
({void*_tmp179=0U;({struct Cyc___cycFILE*_tmp523=Cyc_stderr;struct _fat_ptr _tmp522=({const char*_tmp17A="Error: unexpected Stmt_e\n";_tag_fat(_tmp17A,sizeof(char),26U);});Cyc_fprintf(_tmp523,_tmp522,_tag_fat(_tmp179,sizeof(void*),0U));});});
({exit(1);});return;case 11U: _LL39: _LL3A:
# 996
({void*_tmp17B=0U;({struct Cyc___cycFILE*_tmp525=Cyc_stderr;struct _fat_ptr _tmp524=({const char*_tmp17C="Error: unexpected Throw_e\n";_tag_fat(_tmp17C,sizeof(char),27U);});Cyc_fprintf(_tmp525,_tmp524,_tag_fat(_tmp17B,sizeof(void*),0U));});});
({exit(1);});return;case 12U: _LL3B: _LL3C:
# 999
({void*_tmp17D=0U;({struct Cyc___cycFILE*_tmp527=Cyc_stderr;struct _fat_ptr _tmp526=({const char*_tmp17E="Error: unexpected NoInstantiate_e\n";_tag_fat(_tmp17E,sizeof(char),35U);});Cyc_fprintf(_tmp527,_tmp526,_tag_fat(_tmp17D,sizeof(void*),0U));});});
({exit(1);});return;case 13U: _LL3D: _LL3E:
# 1002
({void*_tmp17F=0U;({struct Cyc___cycFILE*_tmp529=Cyc_stderr;struct _fat_ptr _tmp528=({const char*_tmp180="Error: unexpected Instantiate_e\n";_tag_fat(_tmp180,sizeof(char),33U);});Cyc_fprintf(_tmp529,_tmp528,_tag_fat(_tmp17F,sizeof(void*),0U));});});
({exit(1);});return;case 16U: _LL3F: _LL40:
# 1005
({void*_tmp181=0U;({struct Cyc___cycFILE*_tmp52B=Cyc_stderr;struct _fat_ptr _tmp52A=({const char*_tmp182="Error: unexpected New_e\n";_tag_fat(_tmp182,sizeof(char),25U);});Cyc_fprintf(_tmp52B,_tmp52A,_tag_fat(_tmp181,sizeof(void*),0U));});});
({exit(1);});return;case 24U: _LL41: _LL42:
# 1008
({void*_tmp183=0U;({struct Cyc___cycFILE*_tmp52D=Cyc_stderr;struct _fat_ptr _tmp52C=({const char*_tmp184="Error: unexpected Tuple_e\n";_tag_fat(_tmp184,sizeof(char),27U);});Cyc_fprintf(_tmp52D,_tmp52C,_tag_fat(_tmp183,sizeof(void*),0U));});});
({exit(1);});return;case 34U: _LL43: _LL44:
# 1011
({void*_tmp185=0U;({struct Cyc___cycFILE*_tmp52F=Cyc_stderr;struct _fat_ptr _tmp52E=({const char*_tmp186="Error: unexpected Spawn_e\n";_tag_fat(_tmp186,sizeof(char),27U);});Cyc_fprintf(_tmp52F,_tmp52E,_tag_fat(_tmp185,sizeof(void*),0U));});});
({exit(1);});return;case 25U: _LL45: _LL46:
# 1014
({void*_tmp187=0U;({struct Cyc___cycFILE*_tmp531=Cyc_stderr;struct _fat_ptr _tmp530=({const char*_tmp188="Error: unexpected CompoundLit_e\n";_tag_fat(_tmp188,sizeof(char),33U);});Cyc_fprintf(_tmp531,_tmp530,_tag_fat(_tmp187,sizeof(void*),0U));});});
({exit(1);});return;case 26U: _LL47: _LL48:
# 1017
({void*_tmp189=0U;({struct Cyc___cycFILE*_tmp533=Cyc_stderr;struct _fat_ptr _tmp532=({const char*_tmp18A="Error: unexpected Array_e\n";_tag_fat(_tmp18A,sizeof(char),27U);});Cyc_fprintf(_tmp533,_tmp532,_tag_fat(_tmp189,sizeof(void*),0U));});});
({exit(1);});return;case 27U: _LL49: _LL4A:
# 1020
({void*_tmp18B=0U;({struct Cyc___cycFILE*_tmp535=Cyc_stderr;struct _fat_ptr _tmp534=({const char*_tmp18C="Error: unexpected Comprehension_e\n";_tag_fat(_tmp18C,sizeof(char),35U);});Cyc_fprintf(_tmp535,_tmp534,_tag_fat(_tmp18B,sizeof(void*),0U));});});
({exit(1);});return;case 28U: _LL4B: _LL4C:
# 1023
({void*_tmp18D=0U;({struct Cyc___cycFILE*_tmp537=Cyc_stderr;struct _fat_ptr _tmp536=({const char*_tmp18E="Error: unexpected ComprehensionNoinit_e\n";_tag_fat(_tmp18E,sizeof(char),41U);});Cyc_fprintf(_tmp537,_tmp536,_tag_fat(_tmp18D,sizeof(void*),0U));});});
({exit(1);});return;case 29U: _LL4D: _LL4E:
# 1026
({void*_tmp18F=0U;({struct Cyc___cycFILE*_tmp539=Cyc_stderr;struct _fat_ptr _tmp538=({const char*_tmp190="Error: unexpected Aggregate_e\n";_tag_fat(_tmp190,sizeof(char),31U);});Cyc_fprintf(_tmp539,_tmp538,_tag_fat(_tmp18F,sizeof(void*),0U));});});
({exit(1);});return;case 30U: _LL4F: _LL50:
# 1029
({void*_tmp191=0U;({struct Cyc___cycFILE*_tmp53B=Cyc_stderr;struct _fat_ptr _tmp53A=({const char*_tmp192="Error: unexpected AnonStruct_e\n";_tag_fat(_tmp192,sizeof(char),32U);});Cyc_fprintf(_tmp53B,_tmp53A,_tag_fat(_tmp191,sizeof(void*),0U));});});
({exit(1);});return;case 31U: _LL51: _LL52:
# 1032
({void*_tmp193=0U;({struct Cyc___cycFILE*_tmp53D=Cyc_stderr;struct _fat_ptr _tmp53C=({const char*_tmp194="Error: unexpected Datatype_e\n";_tag_fat(_tmp194,sizeof(char),30U);});Cyc_fprintf(_tmp53D,_tmp53C,_tag_fat(_tmp193,sizeof(void*),0U));});});
({exit(1);});return;case 32U: _LL53: _LL54:
# 1035
({void*_tmp195=0U;({struct Cyc___cycFILE*_tmp53F=Cyc_stderr;struct _fat_ptr _tmp53E=({const char*_tmp196="Error: unexpected Enum_e\n";_tag_fat(_tmp196,sizeof(char),26U);});Cyc_fprintf(_tmp53F,_tmp53E,_tag_fat(_tmp195,sizeof(void*),0U));});});
({exit(1);});return;default: _LL55: _LL56:
# 1038
({void*_tmp197=0U;({struct Cyc___cycFILE*_tmp541=Cyc_stderr;struct _fat_ptr _tmp540=({const char*_tmp198="Error: unexpected AnonEnum_e\n";_tag_fat(_tmp198,sizeof(char),30U);});Cyc_fprintf(_tmp541,_tmp540,_tag_fat(_tmp197,sizeof(void*),0U));});});
({exit(1);});return;}_LL0:;}
# 1043
void Cyc_scan_exp_opt(struct Cyc_Absyn_Exp*eo,struct Cyc_Hashtable_Table*dep){
if((unsigned)eo)({Cyc_scan_exp(eo,dep);});return;}
# 1043
void Cyc_scan_decl(struct Cyc_Absyn_Decl*d,struct Cyc_Hashtable_Table*dep);
# 1049
void Cyc_scan_type(void*t,struct Cyc_Hashtable_Table*dep){
void*_tmp19B=t;struct Cyc_Absyn_Datatypedecl*_tmp19C;struct Cyc_Absyn_Enumdecl*_tmp19D;struct Cyc_Absyn_Aggrdecl*_tmp19E;struct _fat_ptr*_tmp19F;struct Cyc_List_List*_tmp1A0;struct Cyc_Absyn_FnInfo _tmp1A1;struct Cyc_Absyn_Exp*_tmp1A2;void*_tmp1A5;struct Cyc_Absyn_Exp*_tmp1A4;void*_tmp1A3;struct Cyc_Absyn_PtrInfo _tmp1A6;struct Cyc_List_List*_tmp1A8;void*_tmp1A7;switch(*((int*)_tmp19B)){case 0U: _LL1: _tmp1A7=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp19B)->f1;_tmp1A8=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp19B)->f2;_LL2: {void*c=_tmp1A7;struct Cyc_List_List*ts=_tmp1A8;
# 1052
void*_tmp1A9=c;struct _fat_ptr*_tmp1AA;union Cyc_Absyn_AggrInfo _tmp1AB;switch(*((int*)_tmp1A9)){case 0U: _LL1E: _LL1F:
 goto _LL21;case 1U: _LL20: _LL21:
 goto _LL23;case 19U: _LL22: _LL23:
 goto _LL25;case 2U: _LL24: _LL25:
 goto _LL27;case 18U: _LL26: _LL27:
# 1058
 return;case 22U: _LL28: _tmp1AB=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp1A9)->f1;_LL29: {union Cyc_Absyn_AggrInfo info=_tmp1AB;
# 1060
struct _tuple12 _stmttmp4=({Cyc_Absyn_aggr_kinded_name(info);});struct _fat_ptr*_tmp1AC;_LL4D: _tmp1AC=(_stmttmp4.f2)->f2;_LL4E: {struct _fat_ptr*v=_tmp1AC;
_tmp1AA=v;goto _LL2B;}}case 17U: _LL2A: _tmp1AA=(((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp1A9)->f1)->f2;_LL2B: {struct _fat_ptr*v=_tmp1AA;
({Cyc_add_target(v);});return;}case 20U: _LL2C: _LL2D:
 goto _LL2F;case 21U: _LL2E: _LL2F: goto _LL31;case 3U: _LL30: _LL31:
 goto _LL33;case 6U: _LL32: _LL33: goto _LL35;case 4U: _LL34: _LL35: goto _LL37;case 7U: _LL36: _LL37:
 goto _LL39;case 8U: _LL38: _LL39: goto _LL3B;case 10U: _LL3A: _LL3B:
 goto _LL3D;case 9U: _LL3C: _LL3D:
 goto _LL3F;case 11U: _LL3E: _LL3F: goto _LL41;case 12U: _LL40: _LL41:
 goto _LL43;case 5U: _LL42: _LL43: goto _LL45;case 13U: _LL44: _LL45:
 goto _LL47;case 14U: _LL46: _LL47: goto _LL49;case 15U: _LL48: _LL49:
 goto _LL4B;default: _LL4A: _LL4B:
({struct Cyc_String_pa_PrintArg_struct _tmp1AF=({struct Cyc_String_pa_PrintArg_struct _tmp443;_tmp443.tag=0U,({struct _fat_ptr _tmp542=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp443.f1=_tmp542;});_tmp443;});void*_tmp1AD[1U];_tmp1AD[0]=& _tmp1AF;({struct Cyc___cycFILE*_tmp544=Cyc_stderr;struct _fat_ptr _tmp543=({const char*_tmp1AE="Error: unexpected type %s\n";_tag_fat(_tmp1AE,sizeof(char),27U);});Cyc_fprintf(_tmp544,_tmp543,_tag_fat(_tmp1AD,sizeof(void*),1U));});});
({exit(1);});return;}_LL1D:;}case 3U: _LL3: _tmp1A6=((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp19B)->f1;_LL4: {struct Cyc_Absyn_PtrInfo x=_tmp1A6;
# 1076
({Cyc_scan_type(x.elt_type,dep);});
return;}case 4U: _LL5: _tmp1A3=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp19B)->f1).elt_type;_tmp1A4=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp19B)->f1).num_elts;_tmp1A5=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmp19B)->f1).zero_term;_LL6: {void*t=_tmp1A3;struct Cyc_Absyn_Exp*sz=_tmp1A4;void*zt=_tmp1A5;
# 1079
({Cyc_scan_type(t,dep);});
({Cyc_scan_exp_opt(sz,dep);});
return;}case 11U: _LL7: _tmp1A2=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp19B)->f1;_LL8: {struct Cyc_Absyn_Exp*e=_tmp1A2;
# 1083
({Cyc_scan_exp(e,dep);});
return;}case 5U: _LL9: _tmp1A1=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp19B)->f1;_LLA: {struct Cyc_Absyn_FnInfo x=_tmp1A1;
# 1086
({Cyc_scan_type(x.ret_type,dep);});
{struct Cyc_List_List*a=x.args;for(0;a != 0;a=a->tl){
struct _tuple9*_stmttmp5=(struct _tuple9*)a->hd;void*_tmp1B0;_LL50: _tmp1B0=_stmttmp5->f3;_LL51: {void*argt=_tmp1B0;
({Cyc_scan_type(argt,dep);});}}}
# 1091
if(x.cyc_varargs != 0)
({Cyc_scan_type((x.cyc_varargs)->type,dep);});
# 1091
return;}case 7U: _LLB: _tmp1A0=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp19B)->f2;_LLC: {struct Cyc_List_List*sfs=_tmp1A0;
# 1095
for(0;sfs != 0;sfs=sfs->tl){
({Cyc_scan_type(((struct Cyc_Absyn_Aggrfield*)sfs->hd)->type,dep);});
({Cyc_scan_exp_opt(((struct Cyc_Absyn_Aggrfield*)sfs->hd)->width,dep);});}
# 1099
return;}case 8U: _LLD: _tmp19F=(((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp19B)->f1)->f2;_LLE: {struct _fat_ptr*v=_tmp19F;
# 1101
({Cyc_add_target(v);});
return;}case 10U: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp19B)->f1)->r)){case 0U: _LLF: _tmp19E=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp19B)->f1)->r)->f1;_LL10: {struct Cyc_Absyn_Aggrdecl*x=_tmp19E;
# 1105
({void(*_tmp1B1)(struct Cyc_Absyn_Decl*d,struct Cyc_Hashtable_Table*dep)=Cyc_scan_decl;struct Cyc_Absyn_Decl*_tmp1B2=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*_tmp1B3=_cycalloc(sizeof(*_tmp1B3));_tmp1B3->tag=5U,_tmp1B3->f1=x;_tmp1B3;}),0U);});struct Cyc_Hashtable_Table*_tmp1B4=dep;_tmp1B1(_tmp1B2,_tmp1B4);});{
struct _tuple1*_stmttmp6=x->name;struct _fat_ptr*_tmp1B5;_LL53: _tmp1B5=_stmttmp6->f2;_LL54: {struct _fat_ptr*n=_tmp1B5;
({Cyc_add_target(n);});
return;}}}case 1U: _LL11: _tmp19D=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp19B)->f1)->r)->f1;_LL12: {struct Cyc_Absyn_Enumdecl*x=_tmp19D;
# 1111
({void(*_tmp1B6)(struct Cyc_Absyn_Decl*d,struct Cyc_Hashtable_Table*dep)=Cyc_scan_decl;struct Cyc_Absyn_Decl*_tmp1B7=({Cyc_Absyn_new_decl((void*)({struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*_tmp1B8=_cycalloc(sizeof(*_tmp1B8));_tmp1B8->tag=7U,_tmp1B8->f1=x;_tmp1B8;}),0U);});struct Cyc_Hashtable_Table*_tmp1B9=dep;_tmp1B6(_tmp1B7,_tmp1B9);});{
struct _tuple1*_stmttmp7=x->name;struct _fat_ptr*_tmp1BA;_LL56: _tmp1BA=_stmttmp7->f2;_LL57: {struct _fat_ptr*n=_tmp1BA;
({Cyc_add_target(n);});
return;}}}default: _LL13: _tmp19C=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp19B)->f1)->r)->f1;_LL14: {struct Cyc_Absyn_Datatypedecl*dd=_tmp19C;
# 1117
({void*_tmp1BB=0U;({struct Cyc___cycFILE*_tmp546=Cyc_stderr;struct _fat_ptr _tmp545=({const char*_tmp1BC="Error: unexpected Datatype declaration\n";_tag_fat(_tmp1BC,sizeof(char),40U);});Cyc_fprintf(_tmp546,_tmp545,_tag_fat(_tmp1BB,sizeof(void*),0U));});});
({exit(1);});return;}}case 1U: _LL15: _LL16:
# 1120
({void*_tmp1BD=0U;({struct Cyc___cycFILE*_tmp548=Cyc_stderr;struct _fat_ptr _tmp547=({const char*_tmp1BE="Error: unexpected Evar\n";_tag_fat(_tmp1BE,sizeof(char),24U);});Cyc_fprintf(_tmp548,_tmp547,_tag_fat(_tmp1BD,sizeof(void*),0U));});});
({exit(1);});return;case 2U: _LL17: _LL18:
# 1123
({void*_tmp1BF=0U;({struct Cyc___cycFILE*_tmp54A=Cyc_stderr;struct _fat_ptr _tmp549=({const char*_tmp1C0="Error: unexpected VarType\n";_tag_fat(_tmp1C0,sizeof(char),27U);});Cyc_fprintf(_tmp54A,_tmp549,_tag_fat(_tmp1BF,sizeof(void*),0U));});});
({exit(1);});return;case 6U: _LL19: _LL1A:
# 1126
({void*_tmp1C1=0U;({struct Cyc___cycFILE*_tmp54C=Cyc_stderr;struct _fat_ptr _tmp54B=({const char*_tmp1C2="Error: unexpected TupleType\n";_tag_fat(_tmp1C2,sizeof(char),29U);});Cyc_fprintf(_tmp54C,_tmp54B,_tag_fat(_tmp1C1,sizeof(void*),0U));});});
({exit(1);});return;default: _LL1B: _LL1C:
# 1129
({void*_tmp1C3=0U;({struct Cyc___cycFILE*_tmp54E=Cyc_stderr;struct _fat_ptr _tmp54D=({const char*_tmp1C4="Error: unexpected valueof_t\n";_tag_fat(_tmp1C4,sizeof(char),29U);});Cyc_fprintf(_tmp54E,_tmp54D,_tag_fat(_tmp1C3,sizeof(void*),0U));});});
({exit(1);});return;}_LL0:;}
# 1134
void Cyc_scan_decl(struct Cyc_Absyn_Decl*d,struct Cyc_Hashtable_Table*dep){
struct Cyc_Set_Set**saved_targets=Cyc_current_targets;
struct _fat_ptr*saved_source=Cyc_current_source;
Cyc_current_targets=({struct Cyc_Set_Set**_tmp1C7=_cycalloc(sizeof(*_tmp1C7));({struct Cyc_Set_Set*_tmp54F=({({struct Cyc_Set_Set*(*_tmp1C6)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Set_empty;_tmp1C6;})(Cyc_strptrcmp);});*_tmp1C7=_tmp54F;});_tmp1C7;});
{void*_stmttmp8=d->r;void*_tmp1C8=_stmttmp8;struct Cyc_Absyn_Typedefdecl*_tmp1C9;struct Cyc_Absyn_Enumdecl*_tmp1CA;struct Cyc_Absyn_Aggrdecl*_tmp1CB;struct Cyc_Absyn_Fndecl*_tmp1CC;struct Cyc_Absyn_Vardecl*_tmp1CD;switch(*((int*)_tmp1C8)){case 0U: _LL1: _tmp1CD=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp1C8)->f1;_LL2: {struct Cyc_Absyn_Vardecl*x=_tmp1CD;
# 1140
struct _tuple1*_stmttmp9=x->name;struct _fat_ptr*_tmp1CE;_LL24: _tmp1CE=_stmttmp9->f2;_LL25: {struct _fat_ptr*v=_tmp1CE;
Cyc_current_source=v;
({Cyc_scan_type(x->type,dep);});
({Cyc_scan_exp_opt(x->initializer,dep);});
goto _LL0;}}case 1U: _LL3: _tmp1CC=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp1C8)->f1;_LL4: {struct Cyc_Absyn_Fndecl*x=_tmp1CC;
# 1146
struct _tuple1*_stmttmpA=x->name;struct _fat_ptr*_tmp1CF;_LL27: _tmp1CF=_stmttmpA->f2;_LL28: {struct _fat_ptr*v=_tmp1CF;
Cyc_current_source=v;
({Cyc_scan_type((x->i).ret_type,dep);});
{struct Cyc_List_List*a=(x->i).args;for(0;a != 0;a=a->tl){
struct _tuple9*_stmttmpB=(struct _tuple9*)a->hd;void*_tmp1D0;_LL2A: _tmp1D0=_stmttmpB->f3;_LL2B: {void*t1=_tmp1D0;
({Cyc_scan_type(t1,dep);});}}}
# 1153
if((x->i).cyc_varargs != 0)
({Cyc_scan_type(((struct Cyc_Absyn_VarargInfo*)_check_null((x->i).cyc_varargs))->type,dep);});
# 1153
if(x->is_inline)
# 1156
({void*_tmp1D1=0U;({struct Cyc___cycFILE*_tmp551=Cyc_stderr;struct _fat_ptr _tmp550=({const char*_tmp1D2="Warning: ignoring inline function\n";_tag_fat(_tmp1D2,sizeof(char),35U);});Cyc_fprintf(_tmp551,_tmp550,_tag_fat(_tmp1D1,sizeof(void*),0U));});});
# 1153
goto _LL0;}}case 5U: _LL5: _tmp1CB=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp1C8)->f1;_LL6: {struct Cyc_Absyn_Aggrdecl*x=_tmp1CB;
# 1159
struct _tuple1*_stmttmpC=x->name;struct _fat_ptr*_tmp1D3;_LL2D: _tmp1D3=_stmttmpC->f2;_LL2E: {struct _fat_ptr*v=_tmp1D3;
Cyc_current_source=v;
if((unsigned)x->impl){
{struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(x->impl))->fields;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Aggrfield*f=(struct Cyc_Absyn_Aggrfield*)fs->hd;
({Cyc_scan_type(f->type,dep);});
({Cyc_scan_exp_opt(f->width,dep);});}}{
# 1169
struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(x->impl))->fields;for(0;fs != 0;fs=fs->tl){;}}}
# 1161
goto _LL0;}}case 7U: _LL7: _tmp1CA=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp1C8)->f1;_LL8: {struct Cyc_Absyn_Enumdecl*x=_tmp1CA;
# 1175
struct _tuple1*_stmttmpD=x->name;struct _fat_ptr*_tmp1D4;_LL30: _tmp1D4=_stmttmpD->f2;_LL31: {struct _fat_ptr*v=_tmp1D4;
Cyc_current_source=v;
if((unsigned)x->fields){
{struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(x->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*f=(struct Cyc_Absyn_Enumfield*)fs->hd;
({Cyc_scan_exp_opt(f->tag,dep);});}}{
# 1184
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(x->fields))->v;for(0;fs != 0;fs=fs->tl){;}}}
# 1177
goto _LL0;}}case 8U: _LL9: _tmp1C9=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp1C8)->f1;_LLA: {struct Cyc_Absyn_Typedefdecl*x=_tmp1C9;
# 1190
struct _tuple1*_stmttmpE=x->name;struct _fat_ptr*_tmp1D5;_LL33: _tmp1D5=_stmttmpE->f2;_LL34: {struct _fat_ptr*v=_tmp1D5;
Cyc_current_source=v;
if((unsigned)x->defn)
({Cyc_scan_type((void*)_check_null(x->defn),dep);});
# 1192
goto _LL0;}}case 4U: _LLB: _LLC:
# 1196
({void*_tmp1D6=0U;({struct Cyc___cycFILE*_tmp553=Cyc_stderr;struct _fat_ptr _tmp552=({const char*_tmp1D7="Error: unexpected region declaration";_tag_fat(_tmp1D7,sizeof(char),37U);});Cyc_fprintf(_tmp553,_tmp552,_tag_fat(_tmp1D6,sizeof(void*),0U));});});
({exit(1);});case 13U: _LLD: _LLE:
# 1199
({void*_tmp1D8=0U;({struct Cyc___cycFILE*_tmp555=Cyc_stderr;struct _fat_ptr _tmp554=({const char*_tmp1D9="Error: unexpected __cyclone_port_on__";_tag_fat(_tmp1D9,sizeof(char),38U);});Cyc_fprintf(_tmp555,_tmp554,_tag_fat(_tmp1D8,sizeof(void*),0U));});});
({exit(1);});case 14U: _LLF: _LL10:
# 1202
({void*_tmp1DA=0U;({struct Cyc___cycFILE*_tmp557=Cyc_stderr;struct _fat_ptr _tmp556=({const char*_tmp1DB="Error: unexpected __cyclone_port_off__";_tag_fat(_tmp1DB,sizeof(char),39U);});Cyc_fprintf(_tmp557,_tmp556,_tag_fat(_tmp1DA,sizeof(void*),0U));});});
({exit(1);});case 15U: _LL11: _LL12:
# 1205
({void*_tmp1DC=0U;({struct Cyc___cycFILE*_tmp559=Cyc_stderr;struct _fat_ptr _tmp558=({const char*_tmp1DD="Error: unexpected __tempest_on__";_tag_fat(_tmp1DD,sizeof(char),33U);});Cyc_fprintf(_tmp559,_tmp558,_tag_fat(_tmp1DC,sizeof(void*),0U));});});
({exit(1);});case 16U: _LL13: _LL14:
# 1208
({void*_tmp1DE=0U;({struct Cyc___cycFILE*_tmp55B=Cyc_stderr;struct _fat_ptr _tmp55A=({const char*_tmp1DF="Error: unexpected __tempest_off__";_tag_fat(_tmp1DF,sizeof(char),34U);});Cyc_fprintf(_tmp55B,_tmp55A,_tag_fat(_tmp1DE,sizeof(void*),0U));});});
({exit(1);});case 2U: _LL15: _LL16:
# 1211
({void*_tmp1E0=0U;({struct Cyc___cycFILE*_tmp55D=Cyc_stderr;struct _fat_ptr _tmp55C=({const char*_tmp1E1="Error: unexpected let declaration\n";_tag_fat(_tmp1E1,sizeof(char),35U);});Cyc_fprintf(_tmp55D,_tmp55C,_tag_fat(_tmp1E0,sizeof(void*),0U));});});
({exit(1);});case 6U: _LL17: _LL18:
# 1214
({void*_tmp1E2=0U;({struct Cyc___cycFILE*_tmp55F=Cyc_stderr;struct _fat_ptr _tmp55E=({const char*_tmp1E3="Error: unexpected datatype declaration\n";_tag_fat(_tmp1E3,sizeof(char),40U);});Cyc_fprintf(_tmp55F,_tmp55E,_tag_fat(_tmp1E2,sizeof(void*),0U));});});
({exit(1);});case 3U: _LL19: _LL1A:
# 1217
({void*_tmp1E4=0U;({struct Cyc___cycFILE*_tmp561=Cyc_stderr;struct _fat_ptr _tmp560=({const char*_tmp1E5="Error: unexpected let declaration\n";_tag_fat(_tmp1E5,sizeof(char),35U);});Cyc_fprintf(_tmp561,_tmp560,_tag_fat(_tmp1E4,sizeof(void*),0U));});});
({exit(1);});case 9U: _LL1B: _LL1C:
# 1220
({void*_tmp1E6=0U;({struct Cyc___cycFILE*_tmp563=Cyc_stderr;struct _fat_ptr _tmp562=({const char*_tmp1E7="Error: unexpected namespace declaration\n";_tag_fat(_tmp1E7,sizeof(char),41U);});Cyc_fprintf(_tmp563,_tmp562,_tag_fat(_tmp1E6,sizeof(void*),0U));});});
({exit(1);});case 10U: _LL1D: _LL1E:
# 1223
({void*_tmp1E8=0U;({struct Cyc___cycFILE*_tmp565=Cyc_stderr;struct _fat_ptr _tmp564=({const char*_tmp1E9="Error: unexpected using declaration\n";_tag_fat(_tmp1E9,sizeof(char),37U);});Cyc_fprintf(_tmp565,_tmp564,_tag_fat(_tmp1E8,sizeof(void*),0U));});});
({exit(1);});case 11U: _LL1F: _LL20:
# 1226
({void*_tmp1EA=0U;({struct Cyc___cycFILE*_tmp567=Cyc_stderr;struct _fat_ptr _tmp566=({const char*_tmp1EB="Error: unexpected extern \"C\" declaration\n";_tag_fat(_tmp1EB,sizeof(char),42U);});Cyc_fprintf(_tmp567,_tmp566,_tag_fat(_tmp1EA,sizeof(void*),0U));});});
({exit(1);});default: _LL21: _LL22:
# 1229
({void*_tmp1EC=0U;({struct Cyc___cycFILE*_tmp569=Cyc_stderr;struct _fat_ptr _tmp568=({const char*_tmp1ED="Error: unexpected extern \"C include\" declaration\n";_tag_fat(_tmp1ED,sizeof(char),50U);});Cyc_fprintf(_tmp569,_tmp568,_tag_fat(_tmp1EC,sizeof(void*),0U));});});
({exit(1);});}_LL0:;}{
# 1237
struct Cyc_Set_Set*old;
struct _fat_ptr*name=(struct _fat_ptr*)_check_null(Cyc_current_source);
{struct _handler_cons _tmp1EE;_push_handler(& _tmp1EE);{int _tmp1F0=0;if(setjmp(_tmp1EE.handler))_tmp1F0=1;if(!_tmp1F0){
old=({({struct Cyc_Set_Set*(*_tmp56B)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key)=({struct Cyc_Set_Set*(*_tmp1F1)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key)=(struct Cyc_Set_Set*(*)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key))Cyc_Hashtable_lookup;_tmp1F1;});struct Cyc_Hashtable_Table*_tmp56A=dep;_tmp56B(_tmp56A,name);});});;_pop_handler();}else{void*_tmp1EF=(void*)Cyc_Core_get_exn_thrown();void*_tmp1F2=_tmp1EF;void*_tmp1F3;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp1F2)->tag == Cyc_Core_Not_found){_LL36: _LL37:
# 1242
 old=({({struct Cyc_Set_Set*(*_tmp1F4)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Set_empty;_tmp1F4;})(Cyc_strptrcmp);});goto _LL35;}else{_LL38: _tmp1F3=_tmp1F2;_LL39: {void*exn=_tmp1F3;(int)_rethrow(exn);}}_LL35:;}}}{
# 1244
struct Cyc_Set_Set*targets=({Cyc_Set_union_two(*((struct Cyc_Set_Set**)_check_null(Cyc_current_targets)),old);});
({({void(*_tmp56E)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_Set_Set*val)=({void(*_tmp1F5)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_Set_Set*val)=(void(*)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_Set_Set*val))Cyc_Hashtable_insert;_tmp1F5;});struct Cyc_Hashtable_Table*_tmp56D=dep;struct _fat_ptr*_tmp56C=name;_tmp56E(_tmp56D,_tmp56C,targets);});});
# 1247
Cyc_current_targets=saved_targets;
Cyc_current_source=saved_source;}}}
# 1134
struct Cyc_Hashtable_Table*Cyc_new_deps(){
# 1252
return({({struct Cyc_Hashtable_Table*(*_tmp1F7)(int sz,int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),int(*hash)(struct _fat_ptr*))=(struct Cyc_Hashtable_Table*(*)(int sz,int(*cmp)(struct _fat_ptr*,struct _fat_ptr*),int(*hash)(struct _fat_ptr*)))Cyc_Hashtable_create;_tmp1F7;})(107,Cyc_strptrcmp,Cyc_Hashtable_hash_stringptr);});}
# 1255
struct Cyc_Set_Set*Cyc_find(struct Cyc_Hashtable_Table*t,struct _fat_ptr*x){
struct _handler_cons _tmp1F9;_push_handler(& _tmp1F9);{int _tmp1FB=0;if(setjmp(_tmp1F9.handler))_tmp1FB=1;if(!_tmp1FB){{struct Cyc_Set_Set*_tmp1FD=({({struct Cyc_Set_Set*(*_tmp570)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key)=({struct Cyc_Set_Set*(*_tmp1FC)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key)=(struct Cyc_Set_Set*(*)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key))Cyc_Hashtable_lookup;_tmp1FC;});struct Cyc_Hashtable_Table*_tmp56F=t;_tmp570(_tmp56F,x);});});_npop_handler(0U);return _tmp1FD;};_pop_handler();}else{void*_tmp1FA=(void*)Cyc_Core_get_exn_thrown();void*_tmp1FE=_tmp1FA;void*_tmp1FF;if(((struct Cyc_Core_Not_found_exn_struct*)_tmp1FE)->tag == Cyc_Core_Not_found){_LL1: _LL2:
# 1258
 return({({struct Cyc_Set_Set*(*_tmp200)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Set_empty;_tmp200;})(Cyc_strptrcmp);});}else{_LL3: _tmp1FF=_tmp1FE;_LL4: {void*exn=_tmp1FF;(int)_rethrow(exn);}}_LL0:;}}}
# 1262
struct Cyc_Set_Set*Cyc_reachable(struct Cyc_List_List*init,struct Cyc_Hashtable_Table*t){
# 1272 "buildlib.cyl"
struct Cyc_Set_Set*emptyset=({({struct Cyc_Set_Set*(*_tmp209)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Set_empty;_tmp209;})(Cyc_strptrcmp);});
struct Cyc_Set_Set*curr;
for(curr=emptyset;init != 0;init=init->tl){
curr=({({struct Cyc_Set_Set*(*_tmp572)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({struct Cyc_Set_Set*(*_tmp202)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_insert;_tmp202;});struct Cyc_Set_Set*_tmp571=curr;_tmp572(_tmp571,(struct _fat_ptr*)init->hd);});});}{
# 1277
struct Cyc_Set_Set*delta=curr;
# 1279
struct _fat_ptr*sptr=({struct _fat_ptr*_tmp208=_cycalloc(sizeof(*_tmp208));({struct _fat_ptr _tmp573=({const char*_tmp207="";_tag_fat(_tmp207,sizeof(char),1U);});*_tmp208=_tmp573;});_tmp208;});
while(({Cyc_Set_cardinality(delta);})> 0){
struct Cyc_Set_Set*next=emptyset;
struct Cyc_Iter_Iter iter=({Cyc_Set_make_iter(Cyc_Core_heap_region,delta);});
while(({({int(*_tmp574)(struct Cyc_Iter_Iter,struct _fat_ptr**)=({int(*_tmp203)(struct Cyc_Iter_Iter,struct _fat_ptr**)=(int(*)(struct Cyc_Iter_Iter,struct _fat_ptr**))Cyc_Iter_next;_tmp203;});_tmp574(iter,& sptr);});})){
next=({struct Cyc_Set_Set*(*_tmp204)(struct Cyc_Set_Set*s1,struct Cyc_Set_Set*s2)=Cyc_Set_union_two;struct Cyc_Set_Set*_tmp205=next;struct Cyc_Set_Set*_tmp206=({Cyc_find(t,sptr);});_tmp204(_tmp205,_tmp206);});}
delta=({Cyc_Set_diff(next,curr);});
curr=({Cyc_Set_union_two(curr,delta);});}
# 1288
return curr;}}
# 1262 "buildlib.cyl"
enum Cyc_buildlib_mode{Cyc_NORMAL =0U,Cyc_GATHER =1U,Cyc_GATHERSCRIPT =2U,Cyc_FINISH =3U};
# 1292 "buildlib.cyl"
static enum Cyc_buildlib_mode Cyc_mode=Cyc_NORMAL;
static int Cyc_gathering(){
return(int)Cyc_mode == (int)Cyc_GATHER ||(int)Cyc_mode == (int)Cyc_GATHERSCRIPT;}
# 1293
static struct Cyc___cycFILE*Cyc_script_file=0;
# 1298
int Cyc_prscript(struct _fat_ptr fmt,struct _fat_ptr ap){
# 1301
if(Cyc_script_file == 0){
({void*_tmp20C=0U;({struct Cyc___cycFILE*_tmp576=Cyc_stderr;struct _fat_ptr _tmp575=({const char*_tmp20D="Internal error: script file is NULL\n";_tag_fat(_tmp20D,sizeof(char),37U);});Cyc_fprintf(_tmp576,_tmp575,_tag_fat(_tmp20C,sizeof(void*),0U));});});
({exit(1);});}
# 1301
return({Cyc_vfprintf((struct Cyc___cycFILE*)_check_null(Cyc_script_file),fmt,ap);});}
# 1308
int Cyc_force_directory(struct _fat_ptr d){
if((int)Cyc_mode == (int)Cyc_GATHERSCRIPT)
({struct Cyc_String_pa_PrintArg_struct _tmp211=({struct Cyc_String_pa_PrintArg_struct _tmp445;_tmp445.tag=0U,_tmp445.f1=(struct _fat_ptr)((struct _fat_ptr)d);_tmp445;});struct Cyc_String_pa_PrintArg_struct _tmp212=({struct Cyc_String_pa_PrintArg_struct _tmp444;_tmp444.tag=0U,_tmp444.f1=(struct _fat_ptr)((struct _fat_ptr)d);_tmp444;});void*_tmp20F[2U];_tmp20F[0]=& _tmp211,_tmp20F[1]=& _tmp212;({struct _fat_ptr _tmp577=({const char*_tmp210="if ! test -e %s; then mkdir %s; fi\n";_tag_fat(_tmp210,sizeof(char),36U);});Cyc_prscript(_tmp577,_tag_fat(_tmp20F,sizeof(void*),2U));});});else{
# 1315
int fd=({void*_tmp216=0U;({const char*_tmp578=(const char*)_untag_fat_ptr(d,sizeof(char),1U);Cyc_open(_tmp578,0,_tag_fat(_tmp216,sizeof(unsigned short),0U));});});
if(fd == -1){
if(({mkdir((const char*)_check_null(_untag_fat_ptr(d,sizeof(char),1U)),448);})== - 1){
({struct Cyc_String_pa_PrintArg_struct _tmp215=({struct Cyc_String_pa_PrintArg_struct _tmp446;_tmp446.tag=0U,_tmp446.f1=(struct _fat_ptr)((struct _fat_ptr)d);_tmp446;});void*_tmp213[1U];_tmp213[0]=& _tmp215;({struct Cyc___cycFILE*_tmp57A=Cyc_stderr;struct _fat_ptr _tmp579=({const char*_tmp214="Error: could not create directory %s\n";_tag_fat(_tmp214,sizeof(char),38U);});Cyc_fprintf(_tmp57A,_tmp579,_tag_fat(_tmp213,sizeof(void*),1U));});});
return 1;}}else{
# 1322
({close(fd);});}}
# 1324
return 0;}
# 1327
int Cyc_force_directory_prefixes(struct _fat_ptr file){
# 1331
struct _fat_ptr curr=({Cyc_strdup((struct _fat_ptr)file);});
# 1333
struct Cyc_List_List*x=0;
while(1){
curr=({Cyc_Filename_dirname((struct _fat_ptr)curr);});
if(({Cyc_strlen((struct _fat_ptr)curr);})== (unsigned long)0)break;x=({struct Cyc_List_List*_tmp219=_cycalloc(sizeof(*_tmp219));
({struct _fat_ptr*_tmp57B=({struct _fat_ptr*_tmp218=_cycalloc(sizeof(*_tmp218));*_tmp218=(struct _fat_ptr)curr;_tmp218;});_tmp219->hd=_tmp57B;}),_tmp219->tl=x;_tmp219;});}
# 1340
for(0;x != 0;x=x->tl){
if(({Cyc_force_directory(*((struct _fat_ptr*)x->hd));}))return 1;}
# 1343
return 0;}char Cyc_NO_SUPPORT[11U]="NO_SUPPORT";struct Cyc_NO_SUPPORT_exn_struct{char*tag;struct _fat_ptr f1;};
# 1350
static int Cyc_is_other_special(char c){
char _tmp21B=c;switch(_tmp21B){case 92U: _LL1: _LL2:
 goto _LL4;case 34U: _LL3: _LL4:
 goto _LL6;case 59U: _LL5: _LL6:
 goto _LL8;case 38U: _LL7: _LL8:
 goto _LLA;case 40U: _LL9: _LLA:
 goto _LLC;case 41U: _LLB: _LLC:
 goto _LLE;case 124U: _LLD: _LLE:
 goto _LL10;case 94U: _LLF: _LL10:
 goto _LL12;case 60U: _LL11: _LL12:
 goto _LL14;case 62U: _LL13: _LL14:
 goto _LL16;case 10U: _LL15: _LL16:
# 1365
 goto _LL18;case 9U: _LL17: _LL18:
 return 1;default: _LL19: _LL1A:
 return 0;}_LL0:;}
# 1371
static struct _fat_ptr Cyc_sh_escape_string(struct _fat_ptr s){
unsigned long len=({Cyc_strlen((struct _fat_ptr)s);});
# 1375
int single_quotes=0;
int other_special=0;
{int i=0;for(0;(unsigned long)i < len;++ i){
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)'\'')++ single_quotes;else{
if(({Cyc_is_other_special(c);}))++ other_special;}}}
# 1384
if(single_quotes == 0 && other_special == 0)
return s;
# 1384
if(single_quotes == 0)
# 1389
return(struct _fat_ptr)({struct _fat_ptr(*_tmp21D)(struct Cyc_List_List*)=Cyc_strconcat_l;struct Cyc_List_List*_tmp21E=({struct _fat_ptr*_tmp21F[3U];({struct _fat_ptr*_tmp580=({struct _fat_ptr*_tmp221=_cycalloc(sizeof(*_tmp221));({struct _fat_ptr _tmp57F=({const char*_tmp220="'";_tag_fat(_tmp220,sizeof(char),2U);});*_tmp221=_tmp57F;});_tmp221;});_tmp21F[0]=_tmp580;}),({struct _fat_ptr*_tmp57E=({struct _fat_ptr*_tmp222=_cycalloc(sizeof(*_tmp222));*_tmp222=(struct _fat_ptr)s;_tmp222;});_tmp21F[1]=_tmp57E;}),({struct _fat_ptr*_tmp57D=({struct _fat_ptr*_tmp224=_cycalloc(sizeof(*_tmp224));({struct _fat_ptr _tmp57C=({const char*_tmp223="'";_tag_fat(_tmp223,sizeof(char),2U);});*_tmp224=_tmp57C;});_tmp224;});_tmp21F[2]=_tmp57D;});Cyc_List_list(_tag_fat(_tmp21F,sizeof(struct _fat_ptr*),3U));});_tmp21D(_tmp21E);});{
# 1384
unsigned long len2=(len + (unsigned long)single_quotes)+ (unsigned long)other_special;
# 1393
struct _fat_ptr s2=({unsigned _tmp22C=(len2 + (unsigned long)1)+ 1U;char*_tmp22B=_cycalloc_atomic(_check_times(_tmp22C,sizeof(char)));({{unsigned _tmp447=len2 + (unsigned long)1;unsigned i;for(i=0;i < _tmp447;++ i){_tmp22B[i]='\000';}_tmp22B[_tmp447]=0;}0;});_tag_fat(_tmp22B,sizeof(char),_tmp22C);});
int i=0;
int j=0;
for(0;(unsigned long)i < len;++ i){
char c=*((const char*)_check_fat_subscript(s,sizeof(char),i));
if((int)c == (int)'\'' ||({Cyc_is_other_special(c);}))
({struct _fat_ptr _tmp225=_fat_ptr_plus(s2,sizeof(char),j ++);char _tmp226=*((char*)_check_fat_subscript(_tmp225,sizeof(char),0U));char _tmp227='\\';if(_get_fat_size(_tmp225,sizeof(char))== 1U &&(_tmp226 == 0 && _tmp227 != 0))_throw_arraybounds();*((char*)_tmp225.curr)=_tmp227;});
# 1398
({struct _fat_ptr _tmp228=_fat_ptr_plus(s2,sizeof(char),j ++);char _tmp229=*((char*)_check_fat_subscript(_tmp228,sizeof(char),0U));char _tmp22A=c;if(_get_fat_size(_tmp228,sizeof(char))== 1U &&(_tmp229 == 0 && _tmp22A != 0))_throw_arraybounds();*((char*)_tmp228.curr)=_tmp22A;});}
# 1402
return(struct _fat_ptr)s2;}}
# 1404
static struct _fat_ptr*Cyc_sh_escape_stringptr(struct _fat_ptr*sp){
return({struct _fat_ptr*_tmp22E=_cycalloc(sizeof(*_tmp22E));({struct _fat_ptr _tmp581=({Cyc_sh_escape_string(*sp);});*_tmp22E=_tmp581;});_tmp22E;});}
# 1409
int Cyc_process_file(const char*filename,struct Cyc_List_List*start_symbols,struct Cyc_List_List*user_defs,struct Cyc_List_List*omit_symbols,struct Cyc_List_List*hstubs,struct Cyc_List_List*cstubs,struct Cyc_List_List*cycstubs,struct Cyc_List_List*cpp_insert){
# 1417
struct Cyc___cycFILE*maybe;
struct Cyc___cycFILE*in_file;
struct Cyc___cycFILE*out_file;
int errorcode=0;
# 1422
({struct Cyc_String_pa_PrintArg_struct _tmp232=({struct Cyc_String_pa_PrintArg_struct _tmp448;_tmp448.tag=0U,({
struct _fat_ptr _tmp582=(struct _fat_ptr)({const char*_tmp233=filename;_tag_fat(_tmp233,sizeof(char),_get_zero_arr_size_char((void*)_tmp233,1U));});_tmp448.f1=_tmp582;});_tmp448;});void*_tmp230[1U];_tmp230[0]=& _tmp232;({struct Cyc___cycFILE*_tmp584=Cyc_stderr;struct _fat_ptr _tmp583=({const char*_tmp231="********************************* %s...\n";_tag_fat(_tmp231,sizeof(char),41U);});Cyc_fprintf(_tmp584,_tmp583,_tag_fat(_tmp230,sizeof(void*),1U));});});
# 1425
if(!({Cyc_gathering();}))({struct Cyc_String_pa_PrintArg_struct _tmp236=({struct Cyc_String_pa_PrintArg_struct _tmp449;_tmp449.tag=0U,({struct _fat_ptr _tmp585=(struct _fat_ptr)({const char*_tmp237=filename;_tag_fat(_tmp237,sizeof(char),_get_zero_arr_size_char((void*)_tmp237,1U));});_tmp449.f1=_tmp585;});_tmp449;});void*_tmp234[1U];_tmp234[0]=& _tmp236;({struct _fat_ptr _tmp586=({const char*_tmp235="\n%s:\n";_tag_fat(_tmp235,sizeof(char),6U);});Cyc_log(_tmp586,_tag_fat(_tmp234,sizeof(void*),1U));});});{struct _fat_ptr basename=({Cyc_Filename_basename(({const char*_tmp37E=filename;_tag_fat(_tmp37E,sizeof(char),_get_zero_arr_size_char((void*)_tmp37E,1U));}));});
# 1438 "buildlib.cyl"
struct _fat_ptr dirname=({Cyc_Filename_dirname(({const char*_tmp37D=filename;_tag_fat(_tmp37D,sizeof(char),_get_zero_arr_size_char((void*)_tmp37D,1U));}));});
struct _fat_ptr choppedname=({Cyc_Filename_chop_extension((struct _fat_ptr)basename);});
const char*cppinfile=(const char*)_check_null(_untag_fat_ptr(({({struct _fat_ptr _tmp587=(struct _fat_ptr)choppedname;Cyc_strconcat(_tmp587,({const char*_tmp37C=".iA";_tag_fat(_tmp37C,sizeof(char),4U);}));});}),sizeof(char),1U));
# 1442
const char*macrosfile=(const char*)_check_null(_untag_fat_ptr(_get_fat_size(dirname,sizeof(char))== (unsigned)0?({struct Cyc_String_pa_PrintArg_struct _tmp375=({struct Cyc_String_pa_PrintArg_struct _tmp47F;_tmp47F.tag=0U,_tmp47F.f1=(struct _fat_ptr)((struct _fat_ptr)choppedname);_tmp47F;});void*_tmp373[1U];_tmp373[0]=& _tmp375;({struct _fat_ptr _tmp589=({const char*_tmp374="%s.iB";_tag_fat(_tmp374,sizeof(char),6U);});Cyc_aprintf(_tmp589,_tag_fat(_tmp373,sizeof(void*),1U));});}):({struct _fat_ptr(*_tmp376)(struct _fat_ptr,struct _fat_ptr)=Cyc_Filename_concat;struct _fat_ptr _tmp377=(struct _fat_ptr)dirname;struct _fat_ptr _tmp378=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp37B=({struct Cyc_String_pa_PrintArg_struct _tmp480;_tmp480.tag=0U,_tmp480.f1=(struct _fat_ptr)((struct _fat_ptr)choppedname);_tmp480;});void*_tmp379[1U];_tmp379[0]=& _tmp37B;({struct _fat_ptr _tmp588=({const char*_tmp37A="%s.iB";_tag_fat(_tmp37A,sizeof(char),6U);});Cyc_aprintf(_tmp588,_tag_fat(_tmp379,sizeof(void*),1U));});});_tmp376(_tmp377,_tmp378);}),sizeof(char),1U));
# 1447
const char*declsfile=(const char*)_check_null(_untag_fat_ptr(_get_fat_size(dirname,sizeof(char))== (unsigned)0?({struct Cyc_String_pa_PrintArg_struct _tmp36C=({struct Cyc_String_pa_PrintArg_struct _tmp47D;_tmp47D.tag=0U,_tmp47D.f1=(struct _fat_ptr)((struct _fat_ptr)choppedname);_tmp47D;});void*_tmp36A[1U];_tmp36A[0]=& _tmp36C;({struct _fat_ptr _tmp58B=({const char*_tmp36B="%s.iC";_tag_fat(_tmp36B,sizeof(char),6U);});Cyc_aprintf(_tmp58B,_tag_fat(_tmp36A,sizeof(void*),1U));});}):({struct _fat_ptr(*_tmp36D)(struct _fat_ptr,struct _fat_ptr)=Cyc_Filename_concat;struct _fat_ptr _tmp36E=(struct _fat_ptr)dirname;struct _fat_ptr _tmp36F=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp372=({struct Cyc_String_pa_PrintArg_struct _tmp47E;_tmp47E.tag=0U,_tmp47E.f1=(struct _fat_ptr)((struct _fat_ptr)choppedname);_tmp47E;});void*_tmp370[1U];_tmp370[0]=& _tmp372;({struct _fat_ptr _tmp58A=({const char*_tmp371="%s.iC";_tag_fat(_tmp371,sizeof(char),6U);});Cyc_aprintf(_tmp58A,_tag_fat(_tmp370,sizeof(void*),1U));});});_tmp36D(_tmp36E,_tmp36F);}),sizeof(char),1U));
# 1452
const char*filtereddeclsfile=(const char*)_check_null(_untag_fat_ptr(_get_fat_size(dirname,sizeof(char))== (unsigned)0?({struct Cyc_String_pa_PrintArg_struct _tmp363=({struct Cyc_String_pa_PrintArg_struct _tmp47B;_tmp47B.tag=0U,_tmp47B.f1=(struct _fat_ptr)((struct _fat_ptr)choppedname);_tmp47B;});void*_tmp361[1U];_tmp361[0]=& _tmp363;({struct _fat_ptr _tmp58D=({const char*_tmp362="%s.iD";_tag_fat(_tmp362,sizeof(char),6U);});Cyc_aprintf(_tmp58D,_tag_fat(_tmp361,sizeof(void*),1U));});}):({struct _fat_ptr(*_tmp364)(struct _fat_ptr,struct _fat_ptr)=Cyc_Filename_concat;struct _fat_ptr _tmp365=(struct _fat_ptr)dirname;struct _fat_ptr _tmp366=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp369=({struct Cyc_String_pa_PrintArg_struct _tmp47C;_tmp47C.tag=0U,_tmp47C.f1=(struct _fat_ptr)((struct _fat_ptr)choppedname);_tmp47C;});void*_tmp367[1U];_tmp367[0]=& _tmp369;({struct _fat_ptr _tmp58C=({const char*_tmp368="%s.iD";_tag_fat(_tmp368,sizeof(char),6U);});Cyc_aprintf(_tmp58C,_tag_fat(_tmp367,sizeof(void*),1U));});});_tmp364(_tmp365,_tmp366);}),sizeof(char),1U));
# 1457
{struct _handler_cons _tmp238;_push_handler(& _tmp238);{int _tmp23A=0;if(setjmp(_tmp238.handler))_tmp23A=1;if(!_tmp23A){
# 1460
if(({Cyc_force_directory_prefixes(({const char*_tmp23B=filename;_tag_fat(_tmp23B,sizeof(char),_get_zero_arr_size_char((void*)_tmp23B,1U));}));})){
int _tmp23C=1;_npop_handler(0U);return _tmp23C;}
# 1460
if((int)Cyc_mode != (int)Cyc_FINISH){
# 1466
if((int)Cyc_mode == (int)Cyc_GATHERSCRIPT){
({struct Cyc_String_pa_PrintArg_struct _tmp23F=({struct Cyc_String_pa_PrintArg_struct _tmp44A;_tmp44A.tag=0U,({struct _fat_ptr _tmp58E=(struct _fat_ptr)({const char*_tmp240=cppinfile;_tag_fat(_tmp240,sizeof(char),_get_zero_arr_size_char((void*)_tmp240,1U));});_tmp44A.f1=_tmp58E;});_tmp44A;});void*_tmp23D[1U];_tmp23D[0]=& _tmp23F;({struct _fat_ptr _tmp58F=({const char*_tmp23E="cat >%s <<XXX\n";_tag_fat(_tmp23E,sizeof(char),15U);});Cyc_prscript(_tmp58F,_tag_fat(_tmp23D,sizeof(void*),1U));});});
{struct Cyc_List_List*l=cpp_insert;for(0;l != 0;l=l->tl){
({struct Cyc_String_pa_PrintArg_struct _tmp243=({struct Cyc_String_pa_PrintArg_struct _tmp44B;_tmp44B.tag=0U,_tmp44B.f1=(struct _fat_ptr)((struct _fat_ptr)*((struct _fat_ptr*)l->hd));_tmp44B;});void*_tmp241[1U];_tmp241[0]=& _tmp243;({struct _fat_ptr _tmp590=({const char*_tmp242="%s";_tag_fat(_tmp242,sizeof(char),3U);});Cyc_prscript(_tmp590,_tag_fat(_tmp241,sizeof(void*),1U));});});}}
({struct Cyc_String_pa_PrintArg_struct _tmp246=({struct Cyc_String_pa_PrintArg_struct _tmp44C;_tmp44C.tag=0U,({struct _fat_ptr _tmp591=(struct _fat_ptr)({const char*_tmp247=filename;_tag_fat(_tmp247,sizeof(char),_get_zero_arr_size_char((void*)_tmp247,1U));});_tmp44C.f1=_tmp591;});_tmp44C;});void*_tmp244[1U];_tmp244[0]=& _tmp246;({struct _fat_ptr _tmp592=({const char*_tmp245="#include <%s>\n";_tag_fat(_tmp245,sizeof(char),15U);});Cyc_prscript(_tmp592,_tag_fat(_tmp244,sizeof(void*),1U));});});
({void*_tmp248=0U;({struct _fat_ptr _tmp593=({const char*_tmp249="XXX\n";_tag_fat(_tmp249,sizeof(char),5U);});Cyc_prscript(_tmp593,_tag_fat(_tmp248,sizeof(void*),0U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp24C=({struct Cyc_String_pa_PrintArg_struct _tmp44F;_tmp44F.tag=0U,_tmp44F.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_target_cflags);_tmp44F;});struct Cyc_String_pa_PrintArg_struct _tmp24D=({struct Cyc_String_pa_PrintArg_struct _tmp44E;_tmp44E.tag=0U,({struct _fat_ptr _tmp594=(struct _fat_ptr)({const char*_tmp250=macrosfile;_tag_fat(_tmp250,sizeof(char),_get_zero_arr_size_char((void*)_tmp250,1U));});_tmp44E.f1=_tmp594;});_tmp44E;});struct Cyc_String_pa_PrintArg_struct _tmp24E=({struct Cyc_String_pa_PrintArg_struct _tmp44D;_tmp44D.tag=0U,({struct _fat_ptr _tmp595=(struct _fat_ptr)({const char*_tmp24F=cppinfile;_tag_fat(_tmp24F,sizeof(char),_get_zero_arr_size_char((void*)_tmp24F,1U));});_tmp44D.f1=_tmp595;});_tmp44D;});void*_tmp24A[3U];_tmp24A[0]=& _tmp24C,_tmp24A[1]=& _tmp24D,_tmp24A[2]=& _tmp24E;({struct _fat_ptr _tmp596=({const char*_tmp24B="$GCC %s -E -dM -o %s -x c %s && \\\n";_tag_fat(_tmp24B,sizeof(char),35U);});Cyc_prscript(_tmp596,_tag_fat(_tmp24A,sizeof(void*),3U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp253=({struct Cyc_String_pa_PrintArg_struct _tmp452;_tmp452.tag=0U,_tmp452.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_target_cflags);_tmp452;});struct Cyc_String_pa_PrintArg_struct _tmp254=({struct Cyc_String_pa_PrintArg_struct _tmp451;_tmp451.tag=0U,({struct _fat_ptr _tmp597=(struct _fat_ptr)({const char*_tmp257=declsfile;_tag_fat(_tmp257,sizeof(char),_get_zero_arr_size_char((void*)_tmp257,1U));});_tmp451.f1=_tmp597;});_tmp451;});struct Cyc_String_pa_PrintArg_struct _tmp255=({struct Cyc_String_pa_PrintArg_struct _tmp450;_tmp450.tag=0U,({struct _fat_ptr _tmp598=(struct _fat_ptr)({const char*_tmp256=cppinfile;_tag_fat(_tmp256,sizeof(char),_get_zero_arr_size_char((void*)_tmp256,1U));});_tmp450.f1=_tmp598;});_tmp450;});void*_tmp251[3U];_tmp251[0]=& _tmp253,_tmp251[1]=& _tmp254,_tmp251[2]=& _tmp255;({struct _fat_ptr _tmp599=({const char*_tmp252="$GCC %s -E     -o %s -x c %s;\n";_tag_fat(_tmp252,sizeof(char),31U);});Cyc_prscript(_tmp599,_tag_fat(_tmp251,sizeof(void*),3U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp25A=({struct Cyc_String_pa_PrintArg_struct _tmp453;_tmp453.tag=0U,({struct _fat_ptr _tmp59A=(struct _fat_ptr)({const char*_tmp25B=cppinfile;_tag_fat(_tmp25B,sizeof(char),_get_zero_arr_size_char((void*)_tmp25B,1U));});_tmp453.f1=_tmp59A;});_tmp453;});void*_tmp258[1U];_tmp258[0]=& _tmp25A;({struct _fat_ptr _tmp59B=({const char*_tmp259="rm %s\n";_tag_fat(_tmp259,sizeof(char),7U);});Cyc_prscript(_tmp59B,_tag_fat(_tmp258,sizeof(void*),1U));});});}else{
# 1477
maybe=({Cyc_fopen(cppinfile,"w");});
if(!((unsigned)maybe)){
({struct Cyc_String_pa_PrintArg_struct _tmp25E=({struct Cyc_String_pa_PrintArg_struct _tmp454;_tmp454.tag=0U,({struct _fat_ptr _tmp59C=(struct _fat_ptr)({const char*_tmp25F=cppinfile;_tag_fat(_tmp25F,sizeof(char),_get_zero_arr_size_char((void*)_tmp25F,1U));});_tmp454.f1=_tmp59C;});_tmp454;});void*_tmp25C[1U];_tmp25C[0]=& _tmp25E;({struct Cyc___cycFILE*_tmp59E=Cyc_stderr;struct _fat_ptr _tmp59D=({const char*_tmp25D="Error: could not create file %s\n";_tag_fat(_tmp25D,sizeof(char),33U);});Cyc_fprintf(_tmp59E,_tmp59D,_tag_fat(_tmp25C,sizeof(void*),1U));});});{
int _tmp260=1;_npop_handler(0U);return _tmp260;}}
# 1478
if(Cyc_verbose)
# 1483
({struct Cyc_String_pa_PrintArg_struct _tmp263=({struct Cyc_String_pa_PrintArg_struct _tmp455;_tmp455.tag=0U,({struct _fat_ptr _tmp59F=(struct _fat_ptr)({const char*_tmp264=cppinfile;_tag_fat(_tmp264,sizeof(char),_get_zero_arr_size_char((void*)_tmp264,1U));});_tmp455.f1=_tmp59F;});_tmp455;});void*_tmp261[1U];_tmp261[0]=& _tmp263;({struct Cyc___cycFILE*_tmp5A1=Cyc_stderr;struct _fat_ptr _tmp5A0=({const char*_tmp262="Creating %s\n";_tag_fat(_tmp262,sizeof(char),13U);});Cyc_fprintf(_tmp5A1,_tmp5A0,_tag_fat(_tmp261,sizeof(void*),1U));});});
# 1478
out_file=maybe;
# 1485
{struct Cyc_List_List*l=cpp_insert;for(0;l != 0;l=l->tl){
({Cyc_fputs((const char*)_check_null(_untag_fat_ptr(*((struct _fat_ptr*)l->hd),sizeof(char),1U)),out_file);});}}
# 1488
({struct Cyc_String_pa_PrintArg_struct _tmp267=({struct Cyc_String_pa_PrintArg_struct _tmp456;_tmp456.tag=0U,({struct _fat_ptr _tmp5A2=(struct _fat_ptr)({const char*_tmp268=filename;_tag_fat(_tmp268,sizeof(char),_get_zero_arr_size_char((void*)_tmp268,1U));});_tmp456.f1=_tmp5A2;});_tmp456;});void*_tmp265[1U];_tmp265[0]=& _tmp267;({struct Cyc___cycFILE*_tmp5A4=out_file;struct _fat_ptr _tmp5A3=({const char*_tmp266="#include <%s>\n";_tag_fat(_tmp266,sizeof(char),15U);});Cyc_fprintf(_tmp5A4,_tmp5A3,_tag_fat(_tmp265,sizeof(void*),1U));});});
({Cyc_fclose(out_file);});{
struct _fat_ptr cppargs_string=({struct _fat_ptr(*_tmp283)(struct Cyc_List_List*,struct _fat_ptr)=Cyc_str_sepstr;struct Cyc_List_List*_tmp284=({struct Cyc_List_List*_tmp28B=_cycalloc(sizeof(*_tmp28B));
({struct _fat_ptr*_tmp5A7=({struct _fat_ptr*_tmp286=_cycalloc(sizeof(*_tmp286));({struct _fat_ptr _tmp5A6=(struct _fat_ptr)({const char*_tmp285="";_tag_fat(_tmp285,sizeof(char),1U);});*_tmp286=_tmp5A6;});_tmp286;});_tmp28B->hd=_tmp5A7;}),({
struct Cyc_List_List*_tmp5A5=({struct Cyc_List_List*(*_tmp287)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp288)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_map;_tmp288;});struct _fat_ptr*(*_tmp289)(struct _fat_ptr*sp)=Cyc_sh_escape_stringptr;struct Cyc_List_List*_tmp28A=({Cyc_List_rev(Cyc_cppargs);});_tmp287(_tmp289,_tmp28A);});_tmp28B->tl=_tmp5A5;});_tmp28B;});struct _fat_ptr _tmp28C=({const char*_tmp28D=" ";_tag_fat(_tmp28D,sizeof(char),2U);});_tmp283(_tmp284,_tmp28C);});
# 1494
char*cmd=(char*)_check_null(_untag_fat_ptr(({struct Cyc_String_pa_PrintArg_struct _tmp27C=({struct Cyc_String_pa_PrintArg_struct _tmp462;_tmp462.tag=0U,_tmp462.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_cyclone_cc);_tmp462;});struct Cyc_String_pa_PrintArg_struct _tmp27D=({struct Cyc_String_pa_PrintArg_struct _tmp461;_tmp461.tag=0U,_tmp461.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_target_cflags);_tmp461;});struct Cyc_String_pa_PrintArg_struct _tmp27E=({struct Cyc_String_pa_PrintArg_struct _tmp460;_tmp460.tag=0U,_tmp460.f1=(struct _fat_ptr)((struct _fat_ptr)cppargs_string);_tmp460;});struct Cyc_String_pa_PrintArg_struct _tmp27F=({struct Cyc_String_pa_PrintArg_struct _tmp45F;_tmp45F.tag=0U,({struct _fat_ptr _tmp5A8=(struct _fat_ptr)({const char*_tmp282=macrosfile;_tag_fat(_tmp282,sizeof(char),_get_zero_arr_size_char((void*)_tmp282,1U));});_tmp45F.f1=_tmp5A8;});_tmp45F;});struct Cyc_String_pa_PrintArg_struct _tmp280=({struct Cyc_String_pa_PrintArg_struct _tmp45E;_tmp45E.tag=0U,({struct _fat_ptr _tmp5A9=(struct _fat_ptr)({const char*_tmp281=cppinfile;_tag_fat(_tmp281,sizeof(char),_get_zero_arr_size_char((void*)_tmp281,1U));});_tmp45E.f1=_tmp5A9;});_tmp45E;});void*_tmp27A[5U];_tmp27A[0]=& _tmp27C,_tmp27A[1]=& _tmp27D,_tmp27A[2]=& _tmp27E,_tmp27A[3]=& _tmp27F,_tmp27A[4]=& _tmp280;({struct _fat_ptr _tmp5AA=({const char*_tmp27B="%s %s %s -E -dM -o %s -x c %s";_tag_fat(_tmp27B,sizeof(char),30U);});Cyc_aprintf(_tmp5AA,_tag_fat(_tmp27A,sizeof(void*),5U));});}),sizeof(char),1U));
# 1497
if(Cyc_verbose)
({struct Cyc_String_pa_PrintArg_struct _tmp26B=({struct Cyc_String_pa_PrintArg_struct _tmp457;_tmp457.tag=0U,({struct _fat_ptr _tmp5AB=(struct _fat_ptr)({char*_tmp26C=cmd;_tag_fat(_tmp26C,sizeof(char),_get_zero_arr_size_char((void*)_tmp26C,1U));});_tmp457.f1=_tmp5AB;});_tmp457;});void*_tmp269[1U];_tmp269[0]=& _tmp26B;({struct Cyc___cycFILE*_tmp5AD=Cyc_stderr;struct _fat_ptr _tmp5AC=({const char*_tmp26A="%s\n";_tag_fat(_tmp26A,sizeof(char),4U);});Cyc_fprintf(_tmp5AD,_tmp5AC,_tag_fat(_tmp269,sizeof(void*),1U));});});
# 1497
if(!({system((const char*)cmd);})){
# 1502
cmd=(char*)_check_null(_untag_fat_ptr(({struct Cyc_String_pa_PrintArg_struct _tmp26F=({struct Cyc_String_pa_PrintArg_struct _tmp45C;_tmp45C.tag=0U,_tmp45C.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_cyclone_cc);_tmp45C;});struct Cyc_String_pa_PrintArg_struct _tmp270=({struct Cyc_String_pa_PrintArg_struct _tmp45B;_tmp45B.tag=0U,_tmp45B.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_target_cflags);_tmp45B;});struct Cyc_String_pa_PrintArg_struct _tmp271=({struct Cyc_String_pa_PrintArg_struct _tmp45A;_tmp45A.tag=0U,_tmp45A.f1=(struct _fat_ptr)((struct _fat_ptr)cppargs_string);_tmp45A;});struct Cyc_String_pa_PrintArg_struct _tmp272=({struct Cyc_String_pa_PrintArg_struct _tmp459;_tmp459.tag=0U,({struct _fat_ptr _tmp5AE=(struct _fat_ptr)({const char*_tmp275=declsfile;_tag_fat(_tmp275,sizeof(char),_get_zero_arr_size_char((void*)_tmp275,1U));});_tmp459.f1=_tmp5AE;});_tmp459;});struct Cyc_String_pa_PrintArg_struct _tmp273=({struct Cyc_String_pa_PrintArg_struct _tmp458;_tmp458.tag=0U,({struct _fat_ptr _tmp5AF=(struct _fat_ptr)({const char*_tmp274=cppinfile;_tag_fat(_tmp274,sizeof(char),_get_zero_arr_size_char((void*)_tmp274,1U));});_tmp458.f1=_tmp5AF;});_tmp458;});void*_tmp26D[5U];_tmp26D[0]=& _tmp26F,_tmp26D[1]=& _tmp270,_tmp26D[2]=& _tmp271,_tmp26D[3]=& _tmp272,_tmp26D[4]=& _tmp273;({struct _fat_ptr _tmp5B0=({const char*_tmp26E="%s %s %s -E -o %s -x c %s";_tag_fat(_tmp26E,sizeof(char),26U);});Cyc_aprintf(_tmp5B0,_tag_fat(_tmp26D,sizeof(void*),5U));});}),sizeof(char),1U));
# 1505
if(Cyc_verbose)
({struct Cyc_String_pa_PrintArg_struct _tmp278=({struct Cyc_String_pa_PrintArg_struct _tmp45D;_tmp45D.tag=0U,({struct _fat_ptr _tmp5B1=(struct _fat_ptr)({char*_tmp279=cmd;_tag_fat(_tmp279,sizeof(char),_get_zero_arr_size_char((void*)_tmp279,1U));});_tmp45D.f1=_tmp5B1;});_tmp45D;});void*_tmp276[1U];_tmp276[0]=& _tmp278;({struct Cyc___cycFILE*_tmp5B3=Cyc_stderr;struct _fat_ptr _tmp5B2=({const char*_tmp277="%s\n";_tag_fat(_tmp277,sizeof(char),4U);});Cyc_fprintf(_tmp5B3,_tmp5B2,_tag_fat(_tmp276,sizeof(void*),1U));});});
# 1505
({system((const char*)cmd);});}}}}
# 1460
if(({Cyc_gathering();})){
# 1512
int _tmp28E=0;_npop_handler(0U);return _tmp28E;}{
# 1460
struct Cyc_Hashtable_Table*t=({Cyc_new_deps();});
# 1516
maybe=({Cyc_fopen(macrosfile,"r");});
if(!((unsigned)maybe))(int)_throw(({struct Cyc_NO_SUPPORT_exn_struct*_tmp293=_cycalloc(sizeof(*_tmp293));_tmp293->tag=Cyc_NO_SUPPORT,({struct _fat_ptr _tmp5B6=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp291=({struct Cyc_String_pa_PrintArg_struct _tmp463;_tmp463.tag=0U,({struct _fat_ptr _tmp5B4=(struct _fat_ptr)({const char*_tmp292=macrosfile;_tag_fat(_tmp292,sizeof(char),_get_zero_arr_size_char((void*)_tmp292,1U));});_tmp463.f1=_tmp5B4;});_tmp463;});void*_tmp28F[1U];_tmp28F[0]=& _tmp291;({struct _fat_ptr _tmp5B5=({const char*_tmp290="can't open macrosfile %s";_tag_fat(_tmp290,sizeof(char),25U);});Cyc_aprintf(_tmp5B5,_tag_fat(_tmp28F,sizeof(void*),1U));});});_tmp293->f1=_tmp5B6;});_tmp293;}));in_file=maybe;{
# 1521
struct Cyc_Lexing_lexbuf*l=({Cyc_Lexing_from_file(in_file);});
struct _tuple25*entry;
while((entry=({Cyc_line(l);}))!= 0){
struct _tuple25*_stmttmpF=(struct _tuple25*)_check_null(entry);struct Cyc_Set_Set*_tmp295;struct _fat_ptr*_tmp294;_LL1: _tmp294=_stmttmpF->f1;_tmp295=_stmttmpF->f2;_LL2: {struct _fat_ptr*name=_tmp294;struct Cyc_Set_Set*uses=_tmp295;
({({void(*_tmp5B9)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_Set_Set*val)=({void(*_tmp296)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_Set_Set*val)=(void(*)(struct Cyc_Hashtable_Table*t,struct _fat_ptr*key,struct Cyc_Set_Set*val))Cyc_Hashtable_insert;_tmp296;});struct Cyc_Hashtable_Table*_tmp5B8=t;struct _fat_ptr*_tmp5B7=name;_tmp5B9(_tmp5B8,_tmp5B7,uses);});});}}
# 1529
({Cyc_fclose(in_file);});
# 1532
maybe=({Cyc_fopen(declsfile,"r");});
if(!((unsigned)maybe))(int)_throw(({struct Cyc_NO_SUPPORT_exn_struct*_tmp29B=_cycalloc(sizeof(*_tmp29B));_tmp29B->tag=Cyc_NO_SUPPORT,({struct _fat_ptr _tmp5BC=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp299=({struct Cyc_String_pa_PrintArg_struct _tmp464;_tmp464.tag=0U,({struct _fat_ptr _tmp5BA=(struct _fat_ptr)({const char*_tmp29A=declsfile;_tag_fat(_tmp29A,sizeof(char),_get_zero_arr_size_char((void*)_tmp29A,1U));});_tmp464.f1=_tmp5BA;});_tmp464;});void*_tmp297[1U];_tmp297[0]=& _tmp299;({struct _fat_ptr _tmp5BB=({const char*_tmp298="can't open declsfile %s";_tag_fat(_tmp298,sizeof(char),24U);});Cyc_aprintf(_tmp5BB,_tag_fat(_tmp297,sizeof(void*),1U));});});_tmp29B->f1=_tmp5BC;});_tmp29B;}));in_file=maybe;
# 1537
l=({Cyc_Lexing_from_file(in_file);});
Cyc_slurp_out=({Cyc_fopen(filtereddeclsfile,"w");});
if(!((unsigned)Cyc_slurp_out)){int _tmp29C=1;_npop_handler(0U);return _tmp29C;}while(({Cyc_slurp(l);})){
;}{
# 1542
struct Cyc_List_List*x=user_defs;
while(x != 0){
struct _tuple28*_stmttmp10=(struct _tuple28*)x->hd;struct _fat_ptr*_tmp29D;_LL4: _tmp29D=_stmttmp10->f2;_LL5: {struct _fat_ptr*s=_tmp29D;
({struct Cyc_String_pa_PrintArg_struct _tmp2A0=({struct Cyc_String_pa_PrintArg_struct _tmp465;_tmp465.tag=0U,_tmp465.f1=(struct _fat_ptr)((struct _fat_ptr)*s);_tmp465;});void*_tmp29E[1U];_tmp29E[0]=& _tmp2A0;({struct Cyc___cycFILE*_tmp5BE=(struct Cyc___cycFILE*)_check_null(Cyc_slurp_out);struct _fat_ptr _tmp5BD=({const char*_tmp29F="%s";_tag_fat(_tmp29F,sizeof(char),3U);});Cyc_fprintf(_tmp5BE,_tmp5BD,_tag_fat(_tmp29E,sizeof(void*),1U));});});
x=x->tl;}}
# 1548
({Cyc_fclose(in_file);});
({Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_slurp_out));});
if((int)Cyc_mode != (int)Cyc_FINISH)
;
# 1550
maybe=({Cyc_fopen(filtereddeclsfile,"r");});
# 1555
if(!((unsigned)maybe)){int _tmp2A1=1;_npop_handler(0U);return _tmp2A1;}in_file=maybe;
# 1557
({Cyc_Position_reset_position(({const char*_tmp2A2=filtereddeclsfile;_tag_fat(_tmp2A2,sizeof(char),_get_zero_arr_size_char((void*)_tmp2A2,1U));}));});
({Cyc_Lex_lex_init(0);});{
struct Cyc_List_List*decls=({({struct Cyc___cycFILE*_tmp5BF=in_file;Cyc_Parse_parse_file(_tmp5BF,({const char*_tmp335=filtereddeclsfile;_tag_fat(_tmp335,sizeof(char),_get_zero_arr_size_char((void*)_tmp335,1U));}));});});
({Cyc_Lex_lex_init(0);});
({Cyc_fclose(in_file);});
# 1564
{struct Cyc_List_List*d=decls;for(0;d != 0;d=d->tl){
({Cyc_scan_decl((struct Cyc_Absyn_Decl*)d->hd,t);});}}{
# 1568
struct Cyc_List_List*user_symbols=({struct Cyc_List_List*(*_tmp331)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp332)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _fat_ptr*(*f)(struct _fat_ptr*),struct Cyc_List_List*x))Cyc_List_map;_tmp332;});struct _fat_ptr*(*_tmp333)(struct _fat_ptr*symbol)=Cyc_add_user_prefix;struct Cyc_List_List*_tmp334=({Cyc_List_split(user_defs);}).f1;_tmp331(_tmp333,_tmp334);});
struct Cyc_Set_Set*reachable_set=({struct Cyc_Set_Set*(*_tmp32E)(struct Cyc_List_List*init,struct Cyc_Hashtable_Table*t)=Cyc_reachable;struct Cyc_List_List*_tmp32F=({Cyc_List_append(start_symbols,user_symbols);});struct Cyc_Hashtable_Table*_tmp330=t;_tmp32E(_tmp32F,_tmp330);});
# 1572
struct Cyc_List_List*reachable_decls=0;
struct Cyc_List_List*user_decls=0;
struct Cyc_Set_Set*defined_symbols=({({struct Cyc_Set_Set*(*_tmp32D)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*))=(struct Cyc_Set_Set*(*)(int(*cmp)(struct _fat_ptr*,struct _fat_ptr*)))Cyc_Set_empty;_tmp32D;})(Cyc_strptrcmp);});
{struct Cyc_List_List*d=decls;for(0;d != 0;d=d->tl){
struct Cyc_Absyn_Decl*decl=(struct Cyc_Absyn_Decl*)d->hd;
struct _fat_ptr*name;
{void*_stmttmp11=decl->r;void*_tmp2A3=_stmttmp11;struct Cyc_Absyn_Typedefdecl*_tmp2A4;struct Cyc_Absyn_Enumdecl*_tmp2A5;struct Cyc_Absyn_Aggrdecl*_tmp2A6;struct Cyc_Absyn_Fndecl*_tmp2A7;struct Cyc_Absyn_Vardecl*_tmp2A8;switch(*((int*)_tmp2A3)){case 0U: _LL7: _tmp2A8=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp2A3)->f1;_LL8: {struct Cyc_Absyn_Vardecl*x=_tmp2A8;
# 1580
struct _tuple1*_stmttmp12=x->name;struct _fat_ptr*_tmp2A9;_LL2A: _tmp2A9=_stmttmp12->f2;_LL2B: {struct _fat_ptr*v=_tmp2A9;
defined_symbols=({({struct Cyc_Set_Set*(*_tmp5C1)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({struct Cyc_Set_Set*(*_tmp2AA)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_insert;_tmp2AA;});struct Cyc_Set_Set*_tmp5C0=defined_symbols;_tmp5C1(_tmp5C0,v);});});
if(({({int(*_tmp5C3)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=({int(*_tmp2AB)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=(int(*)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x))Cyc_List_mem;_tmp2AB;});struct Cyc_List_List*_tmp5C2=omit_symbols;_tmp5C3(Cyc_strptrcmp,_tmp5C2,v);});}))name=0;else{
name=v;}
goto _LL6;}}case 1U: _LL9: _tmp2A7=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp2A3)->f1;_LLA: {struct Cyc_Absyn_Fndecl*x=_tmp2A7;
# 1586
struct _tuple1*_stmttmp13=x->name;struct _fat_ptr*_tmp2AC;_LL2D: _tmp2AC=_stmttmp13->f2;_LL2E: {struct _fat_ptr*v=_tmp2AC;
defined_symbols=({({struct Cyc_Set_Set*(*_tmp5C5)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({struct Cyc_Set_Set*(*_tmp2AD)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(struct Cyc_Set_Set*(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_insert;_tmp2AD;});struct Cyc_Set_Set*_tmp5C4=defined_symbols;_tmp5C5(_tmp5C4,v);});});
if(({({int(*_tmp5C7)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=({int(*_tmp2AE)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x)=(int(*)(int(*compare)(struct _fat_ptr*,struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr*x))Cyc_List_mem;_tmp2AE;});struct Cyc_List_List*_tmp5C6=omit_symbols;_tmp5C7(Cyc_strptrcmp,_tmp5C6,v);});}))name=0;else{
name=v;}
goto _LL6;}}case 5U: _LLB: _tmp2A6=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp2A3)->f1;_LLC: {struct Cyc_Absyn_Aggrdecl*x=_tmp2A6;
# 1592
struct _tuple1*_stmttmp14=x->name;struct _fat_ptr*_tmp2AF;_LL30: _tmp2AF=_stmttmp14->f2;_LL31: {struct _fat_ptr*v=_tmp2AF;
name=v;
goto _LL6;}}case 7U: _LLD: _tmp2A5=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp2A3)->f1;_LLE: {struct Cyc_Absyn_Enumdecl*x=_tmp2A5;
# 1596
struct _tuple1*_stmttmp15=x->name;struct _fat_ptr*_tmp2B0;_LL33: _tmp2B0=_stmttmp15->f2;_LL34: {struct _fat_ptr*v=_tmp2B0;
name=v;
# 1600
if(name != 0 &&({({int(*_tmp5C9)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({int(*_tmp2B1)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(int(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_member;_tmp2B1;});struct Cyc_Set_Set*_tmp5C8=reachable_set;_tmp5C9(_tmp5C8,name);});}))
reachable_decls=({struct Cyc_List_List*_tmp2B2=_cycalloc(sizeof(*_tmp2B2));_tmp2B2->hd=decl,_tmp2B2->tl=reachable_decls;_tmp2B2;});else{
# 1603
if((unsigned)x->fields){
struct Cyc_List_List*fs=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(x->fields))->v;for(0;fs != 0;fs=fs->tl){
struct Cyc_Absyn_Enumfield*f=(struct Cyc_Absyn_Enumfield*)fs->hd;
struct _tuple1*_stmttmp16=f->name;struct _fat_ptr*_tmp2B3;_LL36: _tmp2B3=_stmttmp16->f2;_LL37: {struct _fat_ptr*v=_tmp2B3;
if(({({int(*_tmp5CB)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({int(*_tmp2B4)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(int(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_member;_tmp2B4;});struct Cyc_Set_Set*_tmp5CA=reachable_set;_tmp5CB(_tmp5CA,v);});})){
reachable_decls=({struct Cyc_List_List*_tmp2B5=_cycalloc(sizeof(*_tmp2B5));_tmp2B5->hd=decl,_tmp2B5->tl=reachable_decls;_tmp2B5;});
break;}}}}}
# 1600
name=0;
# 1614
goto _LL6;}}case 8U: _LLF: _tmp2A4=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp2A3)->f1;_LL10: {struct Cyc_Absyn_Typedefdecl*x=_tmp2A4;
# 1616
struct _tuple1*_stmttmp17=x->name;struct _fat_ptr*_tmp2B6;_LL39: _tmp2B6=_stmttmp17->f2;_LL3A: {struct _fat_ptr*v=_tmp2B6;
name=v;
goto _LL6;}}case 13U: _LL11: _LL12:
 goto _LL14;case 14U: _LL13: _LL14:
 goto _LL16;case 15U: _LL15: _LL16:
 goto _LL18;case 16U: _LL17: _LL18:
 goto _LL1A;case 2U: _LL19: _LL1A:
 goto _LL1C;case 6U: _LL1B: _LL1C:
 goto _LL1E;case 3U: _LL1D: _LL1E:
 goto _LL20;case 9U: _LL1F: _LL20:
 goto _LL22;case 10U: _LL21: _LL22:
 goto _LL24;case 11U: _LL23: _LL24:
 goto _LL26;case 12U: _LL25: _LL26:
 goto _LL28;default: _LL27: _LL28:
# 1631
 name=0;
goto _LL6;}_LL6:;}
# 1635
if(name != 0 &&({({int(*_tmp5CD)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({int(*_tmp2B7)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(int(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_member;_tmp2B7;});struct Cyc_Set_Set*_tmp5CC=reachable_set;_tmp5CD(_tmp5CC,name);});})){
if(({int(*_tmp2B8)(struct _fat_ptr s1,struct _fat_ptr s2,unsigned long len)=Cyc_strncmp;struct _fat_ptr _tmp2B9=(struct _fat_ptr)*name;struct _fat_ptr _tmp2BA=(struct _fat_ptr)Cyc_user_prefix;unsigned long _tmp2BB=({Cyc_strlen((struct _fat_ptr)Cyc_user_prefix);});_tmp2B8(_tmp2B9,_tmp2BA,_tmp2BB);})!= 0)
reachable_decls=({struct Cyc_List_List*_tmp2BC=_cycalloc(sizeof(*_tmp2BC));_tmp2BC->hd=decl,_tmp2BC->tl=reachable_decls;_tmp2BC;});else{
# 1640
({Cyc_rename_decl(decl);});
user_decls=({struct Cyc_List_List*_tmp2BD=_cycalloc(sizeof(*_tmp2BD));_tmp2BD->hd=decl,_tmp2BD->tl=user_decls;_tmp2BD;});}}}}
# 1647
if(!Cyc_do_setjmp){
maybe=({Cyc_fopen(filename,"w");});
if(!((unsigned)maybe)){int _tmp2BE=1;_npop_handler(0U);return _tmp2BE;}out_file=maybe;}else{
# 1651
out_file=Cyc_stdout;}{
struct _fat_ptr ifdefmacro=({struct Cyc_String_pa_PrintArg_struct _tmp32B=({struct Cyc_String_pa_PrintArg_struct _tmp471;_tmp471.tag=0U,({struct _fat_ptr _tmp5CE=(struct _fat_ptr)({const char*_tmp32C=filename;_tag_fat(_tmp32C,sizeof(char),_get_zero_arr_size_char((void*)_tmp32C,1U));});_tmp471.f1=_tmp5CE;});_tmp471;});void*_tmp329[1U];_tmp329[0]=& _tmp32B;({struct _fat_ptr _tmp5CF=({const char*_tmp32A="_%s_";_tag_fat(_tmp32A,sizeof(char),5U);});Cyc_aprintf(_tmp5CF,_tag_fat(_tmp329,sizeof(void*),1U));});});
{int j=0;for(0;(unsigned)j < _get_fat_size(ifdefmacro,sizeof(char));++ j){
if((int)((char*)ifdefmacro.curr)[j]== (int)'.' ||(int)((char*)ifdefmacro.curr)[j]== (int)'/')
({struct _fat_ptr _tmp2BF=_fat_ptr_plus(ifdefmacro,sizeof(char),j);char _tmp2C0=*((char*)_check_fat_subscript(_tmp2BF,sizeof(char),0U));char _tmp2C1='_';if(_get_fat_size(_tmp2BF,sizeof(char))== 1U &&(_tmp2C0 == 0 && _tmp2C1 != 0))_throw_arraybounds();*((char*)_tmp2BF.curr)=_tmp2C1;});else{
if((int)((char*)ifdefmacro.curr)[j]!= (int)'_' &&(int)((char*)ifdefmacro.curr)[j]!= (int)'/')
({struct _fat_ptr _tmp2C2=_fat_ptr_plus(ifdefmacro,sizeof(char),j);char _tmp2C3=*((char*)_check_fat_subscript(_tmp2C2,sizeof(char),0U));char _tmp2C4=(char)({toupper((int)((char*)ifdefmacro.curr)[j]);});if(_get_fat_size(_tmp2C2,sizeof(char))== 1U &&(_tmp2C3 == 0 && _tmp2C4 != 0))_throw_arraybounds();*((char*)_tmp2C2.curr)=_tmp2C4;});}}}
# 1659
({struct Cyc_String_pa_PrintArg_struct _tmp2C7=({struct Cyc_String_pa_PrintArg_struct _tmp467;_tmp467.tag=0U,_tmp467.f1=(struct _fat_ptr)((struct _fat_ptr)ifdefmacro);_tmp467;});struct Cyc_String_pa_PrintArg_struct _tmp2C8=({struct Cyc_String_pa_PrintArg_struct _tmp466;_tmp466.tag=0U,_tmp466.f1=(struct _fat_ptr)((struct _fat_ptr)ifdefmacro);_tmp466;});void*_tmp2C5[2U];_tmp2C5[0]=& _tmp2C7,_tmp2C5[1]=& _tmp2C8;({struct Cyc___cycFILE*_tmp5D1=out_file;struct _fat_ptr _tmp5D0=({const char*_tmp2C6="#ifndef %s\n#define %s\n";_tag_fat(_tmp2C6,sizeof(char),23U);});Cyc_fprintf(_tmp5D1,_tmp5D0,_tag_fat(_tmp2C5,sizeof(void*),2U));});});{
# 1666
struct Cyc_List_List*print_decls=0;
struct Cyc_List_List*names=0;
{struct Cyc_List_List*d=reachable_decls;for(0;d != 0;d=d->tl){
struct Cyc_Absyn_Decl*decl=(struct Cyc_Absyn_Decl*)d->hd;
int anon_enum=0;
struct _fat_ptr*name;
{void*_stmttmp18=decl->r;void*_tmp2C9=_stmttmp18;struct Cyc_Absyn_Typedefdecl*_tmp2CA;struct Cyc_Absyn_Enumdecl*_tmp2CB;struct Cyc_Absyn_Aggrdecl*_tmp2CC;struct Cyc_Absyn_Fndecl*_tmp2CD;struct Cyc_Absyn_Vardecl*_tmp2CE;switch(*((int*)_tmp2C9)){case 0U: _LL3C: _tmp2CE=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp2C9)->f1;_LL3D: {struct Cyc_Absyn_Vardecl*x=_tmp2CE;
# 1674
struct _tuple1*_stmttmp19=x->name;struct _fat_ptr*_tmp2CF;_LL5F: _tmp2CF=_stmttmp19->f2;_LL60: {struct _fat_ptr*v=_tmp2CF;
name=v;
goto _LL3B;}}case 1U: _LL3E: _tmp2CD=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp2C9)->f1;_LL3F: {struct Cyc_Absyn_Fndecl*x=_tmp2CD;
# 1678
if(x->is_inline){name=0;goto _LL3B;}{struct _tuple1*_stmttmp1A=x->name;struct _fat_ptr*_tmp2D0;_LL62: _tmp2D0=_stmttmp1A->f2;_LL63: {struct _fat_ptr*v=_tmp2D0;
# 1680
name=v;
goto _LL3B;}}}case 5U: _LL40: _tmp2CC=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp2C9)->f1;_LL41: {struct Cyc_Absyn_Aggrdecl*x=_tmp2CC;
# 1683
struct _tuple1*_stmttmp1B=x->name;struct _fat_ptr*_tmp2D1;_LL65: _tmp2D1=_stmttmp1B->f2;_LL66: {struct _fat_ptr*v=_tmp2D1;
name=v;
goto _LL3B;}}case 7U: _LL42: _tmp2CB=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp2C9)->f1;_LL43: {struct Cyc_Absyn_Enumdecl*x=_tmp2CB;
# 1687
struct _tuple1*_stmttmp1C=x->name;struct _fat_ptr*_tmp2D2;_LL68: _tmp2D2=_stmttmp1C->f2;_LL69: {struct _fat_ptr*v=_tmp2D2;
name=v;
goto _LL3B;}}case 8U: _LL44: _tmp2CA=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp2C9)->f1;_LL45: {struct Cyc_Absyn_Typedefdecl*x=_tmp2CA;
# 1691
struct _tuple1*_stmttmp1D=x->name;struct _fat_ptr*_tmp2D3;_LL6B: _tmp2D3=_stmttmp1D->f2;_LL6C: {struct _fat_ptr*v=_tmp2D3;
name=v;
goto _LL3B;}}case 4U: _LL46: _LL47:
 goto _LL49;case 13U: _LL48: _LL49:
 goto _LL4B;case 14U: _LL4A: _LL4B:
 goto _LL4D;case 15U: _LL4C: _LL4D:
 goto _LL4F;case 16U: _LL4E: _LL4F:
 goto _LL51;case 2U: _LL50: _LL51:
 goto _LL53;case 6U: _LL52: _LL53:
 goto _LL55;case 3U: _LL54: _LL55:
 goto _LL57;case 9U: _LL56: _LL57:
 goto _LL59;case 10U: _LL58: _LL59:
 goto _LL5B;case 11U: _LL5A: _LL5B:
 goto _LL5D;default: _LL5C: _LL5D:
# 1706
 name=0;
goto _LL3B;}_LL3B:;}
# 1709
if(!((unsigned)name)&& !anon_enum)continue;if(({int(*_tmp2D4)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({
# 1714
int(*_tmp2D5)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(int(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_member;_tmp2D5;});struct Cyc_Set_Set*_tmp2D6=reachable_set;struct _fat_ptr*_tmp2D7=({Cyc_add_user_prefix(name);});_tmp2D4(_tmp2D6,_tmp2D7);})){
struct Cyc_Absyn_Decl*user_decl=({Cyc_Absyn_lookup_decl(user_decls,name);});
if(user_decl == 0)
(int)_throw(({struct Cyc_Core_Impossible_exn_struct*_tmp2D9=_cycalloc(sizeof(*_tmp2D9));_tmp2D9->tag=Cyc_Core_Impossible,({struct _fat_ptr _tmp5D2=({const char*_tmp2D8="Internal Error: bad user-def name";_tag_fat(_tmp2D8,sizeof(char),34U);});_tmp2D9->f1=_tmp5D2;});_tmp2D9;}));else{
# 1720
void*_stmttmp1E=user_decl->r;void*_tmp2DA=_stmttmp1E;switch(*((int*)_tmp2DA)){case 0U: _LL6E: _LL6F:
 goto _LL71;case 1U: _LL70: _LL71:
# 1723
(int)_throw(({struct Cyc_NO_SUPPORT_exn_struct*_tmp2DC=_cycalloc(sizeof(*_tmp2DC));_tmp2DC->tag=Cyc_NO_SUPPORT,({struct _fat_ptr _tmp5D3=({const char*_tmp2DB="user defintions for function or variable decls";_tag_fat(_tmp2DB,sizeof(char),47U);});_tmp2DC->f1=_tmp5D3;});_tmp2DC;}));default: _LL72: _LL73:
 goto _LL6D;}_LL6D:;}
# 1727
({Cyc_Cifc_merge_sys_user_decl(0U,1,user_decl,decl);});
print_decls=({struct Cyc_List_List*_tmp2DD=_cycalloc(sizeof(*_tmp2DD));_tmp2DD->hd=decl,_tmp2DD->tl=print_decls;_tmp2DD;});}else{
# 1731
print_decls=({struct Cyc_List_List*_tmp2DE=_cycalloc(sizeof(*_tmp2DE));_tmp2DE->hd=decl,_tmp2DE->tl=print_decls;_tmp2DE;});}
names=({struct Cyc_List_List*_tmp2DF=_cycalloc(sizeof(*_tmp2DF));_tmp2DF->hd=name,_tmp2DF->tl=names;_tmp2DF;});}}
# 1736
{struct _handler_cons _tmp2E0;_push_handler(& _tmp2E0);{int _tmp2E2=0;if(setjmp(_tmp2E0.handler))_tmp2E2=1;if(!_tmp2E2){
({Cyc_Binding_resolve_all(print_decls);});
({void(*_tmp2E3)(struct Cyc_Tcenv_Tenv*te,int var_default_init,struct Cyc_List_List*ds,int)=Cyc_Tc_tc;struct Cyc_Tcenv_Tenv*_tmp2E4=({Cyc_Tcenv_tc_init();});int _tmp2E5=1;struct Cyc_List_List*_tmp2E6=print_decls;int _tmp2E7=0;_tmp2E3(_tmp2E4,_tmp2E5,_tmp2E6,_tmp2E7);});
# 1737
;_pop_handler();}else{void*_tmp2E1=(void*)Cyc_Core_get_exn_thrown();void*_tmp2E8=_tmp2E1;_LL75: _LL76:
# 1740
(int)_throw(({struct Cyc_NO_SUPPORT_exn_struct*_tmp2EA=_cycalloc(sizeof(*_tmp2EA));_tmp2EA->tag=Cyc_NO_SUPPORT,({struct _fat_ptr _tmp5D4=({const char*_tmp2E9="can't typecheck acquired declarations";_tag_fat(_tmp2E9,sizeof(char),38U);});_tmp2EA->f1=_tmp5D4;});_tmp2EA;}));_LL74:;}}}
# 1744
{struct _tuple0 _stmttmp1F=({struct _tuple0 _tmp46B;_tmp46B.f1=print_decls,_tmp46B.f2=names;_tmp46B;});struct Cyc_List_List*_tmp2EC;struct Cyc_List_List*_tmp2EB;_LL7A: _tmp2EB=_stmttmp1F.f1;_tmp2EC=_stmttmp1F.f2;_LL7B: {struct Cyc_List_List*d=_tmp2EB;struct Cyc_List_List*n=_tmp2EC;for(0;
d != 0 && n != 0;(d=d->tl,n=n->tl)){
struct Cyc_Absyn_Decl*decl=(struct Cyc_Absyn_Decl*)d->hd;
struct _fat_ptr*name=(struct _fat_ptr*)n->hd;
int anon_enum=0;
if(!((unsigned)name))
anon_enum=1;
# 1749
({Cyc_Absynpp_set_params(& Cyc_Absynpp_cyc_params_r);});
# 1754
if((unsigned)name){
ifdefmacro=({struct Cyc_String_pa_PrintArg_struct _tmp2EF=({struct Cyc_String_pa_PrintArg_struct _tmp468;_tmp468.tag=0U,_tmp468.f1=(struct _fat_ptr)((struct _fat_ptr)*name);_tmp468;});void*_tmp2ED[1U];_tmp2ED[0]=& _tmp2EF;({struct _fat_ptr _tmp5D5=({const char*_tmp2EE="_%s_def_";_tag_fat(_tmp2EE,sizeof(char),9U);});Cyc_aprintf(_tmp5D5,_tag_fat(_tmp2ED,sizeof(void*),1U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp2F2=({struct Cyc_String_pa_PrintArg_struct _tmp469;_tmp469.tag=0U,_tmp469.f1=(struct _fat_ptr)((struct _fat_ptr)ifdefmacro);_tmp469;});void*_tmp2F0[1U];_tmp2F0[0]=& _tmp2F2;({struct Cyc___cycFILE*_tmp5D7=out_file;struct _fat_ptr _tmp5D6=({const char*_tmp2F1="#ifndef %s\n";_tag_fat(_tmp2F1,sizeof(char),12U);});Cyc_fprintf(_tmp5D7,_tmp5D6,_tag_fat(_tmp2F0,sizeof(void*),1U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp2F5=({struct Cyc_String_pa_PrintArg_struct _tmp46A;_tmp46A.tag=0U,_tmp46A.f1=(struct _fat_ptr)((struct _fat_ptr)ifdefmacro);_tmp46A;});void*_tmp2F3[1U];_tmp2F3[0]=& _tmp2F5;({struct Cyc___cycFILE*_tmp5D9=out_file;struct _fat_ptr _tmp5D8=({const char*_tmp2F4="#define %s\n";_tag_fat(_tmp2F4,sizeof(char),12U);});Cyc_fprintf(_tmp5D9,_tmp5D8,_tag_fat(_tmp2F3,sizeof(void*),1U));});});
# 1759
({void(*_tmp2F6)(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f)=Cyc_Absynpp_decllist2file;struct Cyc_List_List*_tmp2F7=({struct Cyc_Absyn_Decl*_tmp2F8[1U];_tmp2F8[0]=decl;Cyc_List_list(_tag_fat(_tmp2F8,sizeof(struct Cyc_Absyn_Decl*),1U));});struct Cyc___cycFILE*_tmp2F9=out_file;_tmp2F6(_tmp2F7,_tmp2F9);});
({void*_tmp2FA=0U;({struct Cyc___cycFILE*_tmp5DB=out_file;struct _fat_ptr _tmp5DA=({const char*_tmp2FB="#endif\n";_tag_fat(_tmp2FB,sizeof(char),8U);});Cyc_fprintf(_tmp5DB,_tmp5DA,_tag_fat(_tmp2FA,sizeof(void*),0U));});});}else{
# 1764
({void(*_tmp2FC)(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f)=Cyc_Absynpp_decllist2file;struct Cyc_List_List*_tmp2FD=({struct Cyc_Absyn_Decl*_tmp2FE[1U];_tmp2FE[0]=decl;Cyc_List_list(_tag_fat(_tmp2FE,sizeof(struct Cyc_Absyn_Decl*),1U));});struct Cyc___cycFILE*_tmp2FF=out_file;_tmp2FC(_tmp2FD,_tmp2FF);});}}}}
# 1769
maybe=({Cyc_fopen(macrosfile,"r");});
if(!((unsigned)maybe))(int)_throw(({struct Cyc_NO_SUPPORT_exn_struct*_tmp304=_cycalloc(sizeof(*_tmp304));_tmp304->tag=Cyc_NO_SUPPORT,({struct _fat_ptr _tmp5DE=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp302=({struct Cyc_String_pa_PrintArg_struct _tmp46C;_tmp46C.tag=0U,({struct _fat_ptr _tmp5DC=(struct _fat_ptr)({const char*_tmp303=macrosfile;_tag_fat(_tmp303,sizeof(char),_get_zero_arr_size_char((void*)_tmp303,1U));});_tmp46C.f1=_tmp5DC;});_tmp46C;});void*_tmp300[1U];_tmp300[0]=& _tmp302;({struct _fat_ptr _tmp5DD=({const char*_tmp301="can't open macrosfile %s";_tag_fat(_tmp301,sizeof(char),25U);});Cyc_aprintf(_tmp5DD,_tag_fat(_tmp300,sizeof(void*),1U));});});_tmp304->f1=_tmp5DE;});_tmp304;}));in_file=maybe;
# 1773
l=({Cyc_Lexing_from_file(in_file);});{
struct _tuple26*entry2;
while((entry2=({Cyc_suck_line(l);}))!= 0){
struct _tuple26*_stmttmp20=(struct _tuple26*)_check_null(entry2);struct _fat_ptr*_tmp306;struct _fat_ptr _tmp305;_LL7D: _tmp305=_stmttmp20->f1;_tmp306=_stmttmp20->f2;_LL7E: {struct _fat_ptr line=_tmp305;struct _fat_ptr*name=_tmp306;
if(({({int(*_tmp5E0)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({int(*_tmp307)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(int(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_member;_tmp307;});struct Cyc_Set_Set*_tmp5DF=reachable_set;_tmp5E0(_tmp5DF,name);});})){
({struct Cyc_String_pa_PrintArg_struct _tmp30A=({struct Cyc_String_pa_PrintArg_struct _tmp46D;_tmp46D.tag=0U,_tmp46D.f1=(struct _fat_ptr)((struct _fat_ptr)*name);_tmp46D;});void*_tmp308[1U];_tmp308[0]=& _tmp30A;({struct Cyc___cycFILE*_tmp5E2=out_file;struct _fat_ptr _tmp5E1=({const char*_tmp309="#ifndef %s\n";_tag_fat(_tmp309,sizeof(char),12U);});Cyc_fprintf(_tmp5E2,_tmp5E1,_tag_fat(_tmp308,sizeof(void*),1U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp30D=({struct Cyc_String_pa_PrintArg_struct _tmp46E;_tmp46E.tag=0U,_tmp46E.f1=(struct _fat_ptr)((struct _fat_ptr)line);_tmp46E;});void*_tmp30B[1U];_tmp30B[0]=& _tmp30D;({struct Cyc___cycFILE*_tmp5E4=out_file;struct _fat_ptr _tmp5E3=({const char*_tmp30C="%s\n";_tag_fat(_tmp30C,sizeof(char),4U);});Cyc_fprintf(_tmp5E4,_tmp5E3,_tag_fat(_tmp30B,sizeof(void*),1U));});});
({void*_tmp30E=0U;({struct Cyc___cycFILE*_tmp5E6=out_file;struct _fat_ptr _tmp5E5=({const char*_tmp30F="#endif\n";_tag_fat(_tmp30F,sizeof(char),8U);});Cyc_fprintf(_tmp5E6,_tmp5E5,_tag_fat(_tmp30E,sizeof(void*),0U));});});}}}
# 1783
({Cyc_fclose(in_file);});
if((int)Cyc_mode != (int)Cyc_FINISH);if(hstubs != 0){
# 1787
struct Cyc_List_List*x=hstubs;for(0;x != 0;x=x->tl){
struct _tuple27*_stmttmp21=(struct _tuple27*)x->hd;struct _fat_ptr _tmp311;struct _fat_ptr _tmp310;_LL80: _tmp310=_stmttmp21->f1;_tmp311=_stmttmp21->f2;_LL81: {struct _fat_ptr symbol=_tmp310;struct _fat_ptr text=_tmp311;
if(({char*_tmp5EA=(char*)text.curr;_tmp5EA != (char*)(_tag_fat(0,0,0)).curr;})&&(
({char*_tmp5E9=(char*)symbol.curr;_tmp5E9 == (char*)(_tag_fat(0,0,0)).curr;})||({({int(*_tmp5E8)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({int(*_tmp312)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(int(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_member;_tmp312;});struct Cyc_Set_Set*_tmp5E7=defined_symbols;_tmp5E8(_tmp5E7,({struct _fat_ptr*_tmp313=_cycalloc(sizeof(*_tmp313));*_tmp313=symbol;_tmp313;}));});})))
# 1792
({Cyc_fputs((const char*)_untag_fat_ptr(text,sizeof(char),1U),out_file);});else{
# 1794
({struct Cyc_String_pa_PrintArg_struct _tmp316=({struct Cyc_String_pa_PrintArg_struct _tmp46F;_tmp46F.tag=0U,_tmp46F.f1=(struct _fat_ptr)((struct _fat_ptr)symbol);_tmp46F;});void*_tmp314[1U];_tmp314[0]=& _tmp316;({struct _fat_ptr _tmp5EB=({const char*_tmp315="%s is not supported on this platform\n";_tag_fat(_tmp315,sizeof(char),38U);});Cyc_log(_tmp5EB,_tag_fat(_tmp314,sizeof(void*),1U));});});}}}}
# 1784
({void*_tmp317=0U;({struct Cyc___cycFILE*_tmp5ED=out_file;struct _fat_ptr _tmp5EC=({const char*_tmp318="#endif\n";_tag_fat(_tmp318,sizeof(char),8U);});Cyc_fprintf(_tmp5ED,_tmp5EC,_tag_fat(_tmp317,sizeof(void*),0U));});});
# 1798
if(Cyc_do_setjmp){int _tmp319=0;_npop_handler(0U);return _tmp319;}else{
({Cyc_fclose(out_file);});}
# 1802
if(cstubs != 0){
out_file=(struct Cyc___cycFILE*)_check_null(Cyc_cstubs_file);{
struct Cyc_List_List*x=cstubs;for(0;x != 0;x=x->tl){
struct _tuple27*_stmttmp22=(struct _tuple27*)x->hd;struct _fat_ptr _tmp31B;struct _fat_ptr _tmp31A;_LL83: _tmp31A=_stmttmp22->f1;_tmp31B=_stmttmp22->f2;_LL84: {struct _fat_ptr symbol=_tmp31A;struct _fat_ptr text=_tmp31B;
if(({char*_tmp5F1=(char*)text.curr;_tmp5F1 != (char*)(_tag_fat(0,0,0)).curr;})&&(
({char*_tmp5F0=(char*)symbol.curr;_tmp5F0 == (char*)(_tag_fat(0,0,0)).curr;})||({({int(*_tmp5EF)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({int(*_tmp31C)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(int(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_member;_tmp31C;});struct Cyc_Set_Set*_tmp5EE=defined_symbols;_tmp5EF(_tmp5EE,({struct _fat_ptr*_tmp31D=_cycalloc(sizeof(*_tmp31D));*_tmp31D=symbol;_tmp31D;}));});})))
({Cyc_fputs((const char*)_untag_fat_ptr(text,sizeof(char),1U),out_file);});}}}}
# 1802
out_file=(struct Cyc___cycFILE*)_check_null(Cyc_cycstubs_file);
# 1815
({struct Cyc_String_pa_PrintArg_struct _tmp320=({struct Cyc_String_pa_PrintArg_struct _tmp470;_tmp470.tag=0U,({struct _fat_ptr _tmp5F2=(struct _fat_ptr)({const char*_tmp321=filename;_tag_fat(_tmp321,sizeof(char),_get_zero_arr_size_char((void*)_tmp321,1U));});_tmp470.f1=_tmp5F2;});_tmp470;});void*_tmp31E[1U];_tmp31E[0]=& _tmp320;({struct Cyc___cycFILE*_tmp5F4=out_file;struct _fat_ptr _tmp5F3=({const char*_tmp31F="#include <%s>\n\n";_tag_fat(_tmp31F,sizeof(char),16U);});Cyc_fprintf(_tmp5F4,_tmp5F3,_tag_fat(_tmp31E,sizeof(void*),1U));});});
if(cycstubs != 0){
out_file=(struct Cyc___cycFILE*)_check_null(Cyc_cycstubs_file);
{struct Cyc_List_List*x=cycstubs;for(0;x != 0;x=x->tl){
struct _tuple27*_stmttmp23=(struct _tuple27*)x->hd;struct _fat_ptr _tmp323;struct _fat_ptr _tmp322;_LL86: _tmp322=_stmttmp23->f1;_tmp323=_stmttmp23->f2;_LL87: {struct _fat_ptr symbol=_tmp322;struct _fat_ptr text=_tmp323;
if(({char*_tmp5F8=(char*)text.curr;_tmp5F8 != (char*)(_tag_fat(0,0,0)).curr;})&&(
({char*_tmp5F7=(char*)symbol.curr;_tmp5F7 == (char*)(_tag_fat(0,0,0)).curr;})||({({int(*_tmp5F6)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=({int(*_tmp324)(struct Cyc_Set_Set*s,struct _fat_ptr*elt)=(int(*)(struct Cyc_Set_Set*s,struct _fat_ptr*elt))Cyc_Set_member;_tmp324;});struct Cyc_Set_Set*_tmp5F5=defined_symbols;_tmp5F6(_tmp5F5,({struct _fat_ptr*_tmp325=_cycalloc(sizeof(*_tmp325));*_tmp325=symbol;_tmp325;}));});})))
({Cyc_fputs((const char*)_untag_fat_ptr(text,sizeof(char),1U),out_file);});}}}
# 1824
({void*_tmp326=0U;({struct Cyc___cycFILE*_tmp5FA=out_file;struct _fat_ptr _tmp5F9=({const char*_tmp327="\n";_tag_fat(_tmp327,sizeof(char),2U);});Cyc_fprintf(_tmp5FA,_tmp5F9,_tag_fat(_tmp326,sizeof(void*),0U));});});}{
# 1816
int _tmp328=0;_npop_handler(0U);return _tmp328;}}}}}}}}}
# 1460
;_pop_handler();}else{void*_tmp239=(void*)Cyc_Core_get_exn_thrown();void*_tmp336=_tmp239;void*_tmp337;struct _fat_ptr _tmp338;struct _fat_ptr _tmp339;struct _fat_ptr _tmp33A;struct _fat_ptr _tmp33B;if(((struct Cyc_Core_Impossible_exn_struct*)_tmp336)->tag == Cyc_Core_Impossible){_LL89: _tmp33B=((struct Cyc_Core_Impossible_exn_struct*)_tmp336)->f1;_LL8A: {struct _fat_ptr s=_tmp33B;
# 1831
({struct Cyc_String_pa_PrintArg_struct _tmp33E=({struct Cyc_String_pa_PrintArg_struct _tmp472;_tmp472.tag=0U,_tmp472.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp472;});void*_tmp33C[1U];_tmp33C[0]=& _tmp33E;({struct Cyc___cycFILE*_tmp5FC=Cyc_stderr;struct _fat_ptr _tmp5FB=({const char*_tmp33D="Got Core::Impossible(%s)\n";_tag_fat(_tmp33D,sizeof(char),26U);});Cyc_fprintf(_tmp5FC,_tmp5FB,_tag_fat(_tmp33C,sizeof(void*),1U));});});goto _LL88;}}else{if(((struct Cyc_Dict_Absent_exn_struct*)_tmp336)->tag == Cyc_Dict_Absent){_LL8B: _LL8C:
# 1833
({void*_tmp33F=0U;({struct Cyc___cycFILE*_tmp5FE=Cyc_stderr;struct _fat_ptr _tmp5FD=({const char*_tmp340="Got Dict::Absent\n";_tag_fat(_tmp340,sizeof(char),18U);});Cyc_fprintf(_tmp5FE,_tmp5FD,_tag_fat(_tmp33F,sizeof(void*),0U));});});goto _LL88;}else{if(((struct Cyc_Core_Failure_exn_struct*)_tmp336)->tag == Cyc_Core_Failure){_LL8D: _tmp33A=((struct Cyc_Core_Failure_exn_struct*)_tmp336)->f1;_LL8E: {struct _fat_ptr s=_tmp33A;
# 1835
({struct Cyc_String_pa_PrintArg_struct _tmp343=({struct Cyc_String_pa_PrintArg_struct _tmp474;_tmp474.tag=0U,({
struct _fat_ptr _tmp5FF=(struct _fat_ptr)({const char*_tmp345=({Cyc_Core_get_exn_filename();});_tag_fat(_tmp345,sizeof(char),_get_zero_arr_size_char((void*)_tmp345,1U));});_tmp474.f1=_tmp5FF;});_tmp474;});struct Cyc_Int_pa_PrintArg_struct _tmp344=({struct Cyc_Int_pa_PrintArg_struct _tmp473;_tmp473.tag=1U,({unsigned long _tmp600=(unsigned long)({Cyc_Core_get_exn_lineno();});_tmp473.f1=_tmp600;});_tmp473;});void*_tmp341[2U];_tmp341[0]=& _tmp343,_tmp341[1]=& _tmp344;({struct Cyc___cycFILE*_tmp602=Cyc_stderr;struct _fat_ptr _tmp601=({const char*_tmp342="exception from file %s, line %d was thrown\n";_tag_fat(_tmp342,sizeof(char),44U);});Cyc_fprintf(_tmp602,_tmp601,_tag_fat(_tmp341,sizeof(void*),2U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp348=({struct Cyc_String_pa_PrintArg_struct _tmp475;_tmp475.tag=0U,_tmp475.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp475;});void*_tmp346[1U];_tmp346[0]=& _tmp348;({struct Cyc___cycFILE*_tmp604=Cyc_stderr;struct _fat_ptr _tmp603=({const char*_tmp347="Got Core::Failure(%s)\n";_tag_fat(_tmp347,sizeof(char),23U);});Cyc_fprintf(_tmp604,_tmp603,_tag_fat(_tmp346,sizeof(void*),1U));});});goto _LL88;}}else{if(((struct Cyc_Core_Invalid_argument_exn_struct*)_tmp336)->tag == Cyc_Core_Invalid_argument){_LL8F: _tmp339=((struct Cyc_Core_Invalid_argument_exn_struct*)_tmp336)->f1;_LL90: {struct _fat_ptr s=_tmp339;
# 1839
({struct Cyc_String_pa_PrintArg_struct _tmp34B=({struct Cyc_String_pa_PrintArg_struct _tmp476;_tmp476.tag=0U,_tmp476.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp476;});void*_tmp349[1U];_tmp349[0]=& _tmp34B;({struct Cyc___cycFILE*_tmp606=Cyc_stderr;struct _fat_ptr _tmp605=({const char*_tmp34A="Got Invalid_argument(%s)\n";_tag_fat(_tmp34A,sizeof(char),26U);});Cyc_fprintf(_tmp606,_tmp605,_tag_fat(_tmp349,sizeof(void*),1U));});});goto _LL88;}}else{if(((struct Cyc_Core_Not_found_exn_struct*)_tmp336)->tag == Cyc_Core_Not_found){_LL91: _LL92:
# 1841
({void*_tmp34C=0U;({struct Cyc___cycFILE*_tmp608=Cyc_stderr;struct _fat_ptr _tmp607=({const char*_tmp34D="Got Not_found\n";_tag_fat(_tmp34D,sizeof(char),15U);});Cyc_fprintf(_tmp608,_tmp607,_tag_fat(_tmp34C,sizeof(void*),0U));});});goto _LL88;}else{if(((struct Cyc_NO_SUPPORT_exn_struct*)_tmp336)->tag == Cyc_NO_SUPPORT){_LL93: _tmp338=((struct Cyc_NO_SUPPORT_exn_struct*)_tmp336)->f1;_LL94: {struct _fat_ptr s=_tmp338;
# 1843
({struct Cyc_String_pa_PrintArg_struct _tmp350=({struct Cyc_String_pa_PrintArg_struct _tmp477;_tmp477.tag=0U,_tmp477.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp477;});void*_tmp34E[1U];_tmp34E[0]=& _tmp350;({struct Cyc___cycFILE*_tmp60A=Cyc_stderr;struct _fat_ptr _tmp609=({const char*_tmp34F="No support because %s\n";_tag_fat(_tmp34F,sizeof(char),23U);});Cyc_fprintf(_tmp60A,_tmp609,_tag_fat(_tmp34E,sizeof(void*),1U));});});goto _LL88;}}else{_LL95: _tmp337=_tmp336;_LL96: {void*x=_tmp337;
# 1845
({void*_tmp351=0U;({struct Cyc___cycFILE*_tmp60C=Cyc_stderr;struct _fat_ptr _tmp60B=({const char*_tmp352="Got unknown exception\n";_tag_fat(_tmp352,sizeof(char),23U);});Cyc_fprintf(_tmp60C,_tmp60B,_tag_fat(_tmp351,sizeof(void*),0U));});});
({Cyc_Core_rethrow(x);});}}}}}}}_LL88:;}}}
# 1851
maybe=({Cyc_fopen(filename,"w");});
if(!((unsigned)maybe)){
({struct Cyc_String_pa_PrintArg_struct _tmp355=({struct Cyc_String_pa_PrintArg_struct _tmp478;_tmp478.tag=0U,({struct _fat_ptr _tmp60D=(struct _fat_ptr)({const char*_tmp356=filename;_tag_fat(_tmp356,sizeof(char),_get_zero_arr_size_char((void*)_tmp356,1U));});_tmp478.f1=_tmp60D;});_tmp478;});void*_tmp353[1U];_tmp353[0]=& _tmp355;({struct Cyc___cycFILE*_tmp60F=Cyc_stderr;struct _fat_ptr _tmp60E=({const char*_tmp354="Error: could not create file %s\n";_tag_fat(_tmp354,sizeof(char),33U);});Cyc_fprintf(_tmp60F,_tmp60E,_tag_fat(_tmp353,sizeof(void*),1U));});});
return 1;}
# 1852
out_file=maybe;
# 1857
({struct Cyc_String_pa_PrintArg_struct _tmp359=({struct Cyc_String_pa_PrintArg_struct _tmp479;_tmp479.tag=0U,({
# 1859
struct _fat_ptr _tmp610=(struct _fat_ptr)({const char*_tmp35A=filename;_tag_fat(_tmp35A,sizeof(char),_get_zero_arr_size_char((void*)_tmp35A,1U));});_tmp479.f1=_tmp610;});_tmp479;});void*_tmp357[1U];_tmp357[0]=& _tmp359;({struct Cyc___cycFILE*_tmp612=out_file;struct _fat_ptr _tmp611=({const char*_tmp358="#error -- %s is not supported on this platform\n";_tag_fat(_tmp358,sizeof(char),48U);});Cyc_fprintf(_tmp612,_tmp611,_tag_fat(_tmp357,sizeof(void*),1U));});});
({Cyc_fclose(out_file);});
({struct Cyc_String_pa_PrintArg_struct _tmp35D=({struct Cyc_String_pa_PrintArg_struct _tmp47A;_tmp47A.tag=0U,({
struct _fat_ptr _tmp613=(struct _fat_ptr)({const char*_tmp35E=filename;_tag_fat(_tmp35E,sizeof(char),_get_zero_arr_size_char((void*)_tmp35E,1U));});_tmp47A.f1=_tmp613;});_tmp47A;});void*_tmp35B[1U];_tmp35B[0]=& _tmp35D;({struct Cyc___cycFILE*_tmp615=Cyc_stderr;struct _fat_ptr _tmp614=({const char*_tmp35C="Warning: %s will not be supported on this platform\n";_tag_fat(_tmp35C,sizeof(char),52U);});Cyc_fprintf(_tmp615,_tmp614,_tag_fat(_tmp35B,sizeof(void*),1U));});});
({void*_tmp35F=0U;({struct _fat_ptr _tmp616=({const char*_tmp360="Not supported on this platform\n";_tag_fat(_tmp360,sizeof(char),32U);});Cyc_log(_tmp616,_tag_fat(_tmp35F,sizeof(void*),0U));});});
# 1871
return 0;}}
# 1875
int Cyc_process_specfile(const char*file,const char*dir){
if(Cyc_verbose)
({struct Cyc_String_pa_PrintArg_struct _tmp382=({struct Cyc_String_pa_PrintArg_struct _tmp481;_tmp481.tag=0U,({struct _fat_ptr _tmp617=(struct _fat_ptr)({const char*_tmp383=file;_tag_fat(_tmp383,sizeof(char),_get_zero_arr_size_char((void*)_tmp383,1U));});_tmp481.f1=_tmp617;});_tmp481;});void*_tmp380[1U];_tmp380[0]=& _tmp382;({struct Cyc___cycFILE*_tmp619=Cyc_stderr;struct _fat_ptr _tmp618=({const char*_tmp381="Processing %s\n";_tag_fat(_tmp381,sizeof(char),15U);});Cyc_fprintf(_tmp619,_tmp618,_tag_fat(_tmp380,sizeof(void*),1U));});});{
# 1876
struct Cyc___cycFILE*maybe=({Cyc_fopen(file,"r");});
# 1879
if(!((unsigned)maybe)){
({struct Cyc_String_pa_PrintArg_struct _tmp386=({struct Cyc_String_pa_PrintArg_struct _tmp482;_tmp482.tag=0U,({struct _fat_ptr _tmp61A=(struct _fat_ptr)({const char*_tmp387=file;_tag_fat(_tmp387,sizeof(char),_get_zero_arr_size_char((void*)_tmp387,1U));});_tmp482.f1=_tmp61A;});_tmp482;});void*_tmp384[1U];_tmp384[0]=& _tmp386;({struct Cyc___cycFILE*_tmp61C=Cyc_stderr;struct _fat_ptr _tmp61B=({const char*_tmp385="Error: could not open %s\n";_tag_fat(_tmp385,sizeof(char),26U);});Cyc_fprintf(_tmp61C,_tmp61B,_tag_fat(_tmp384,sizeof(void*),1U));});});
return 1;}{
# 1879
struct Cyc___cycFILE*in_file=maybe;
# 1887
struct _fat_ptr buf=({char*_tmp3A0=({unsigned _tmp39F=1024U + 1U;char*_tmp39E=_cycalloc_atomic(_check_times(_tmp39F,sizeof(char)));({{unsigned _tmp488=1024U;unsigned i;for(i=0;i < _tmp488;++ i){_tmp39E[i]='\000';}_tmp39E[_tmp488]=0;}0;});_tmp39E;});_tag_fat(_tmp3A0,sizeof(char),1025U);});
struct _fat_ptr cwd=({Cyc_getcwd(buf,_get_fat_size(buf,sizeof(char)));});
if((int)Cyc_mode != (int)Cyc_GATHERSCRIPT){
if(({chdir(dir);})){
({struct Cyc_String_pa_PrintArg_struct _tmp38A=({struct Cyc_String_pa_PrintArg_struct _tmp483;_tmp483.tag=0U,({struct _fat_ptr _tmp61D=(struct _fat_ptr)({const char*_tmp38B=dir;_tag_fat(_tmp38B,sizeof(char),_get_zero_arr_size_char((void*)_tmp38B,1U));});_tmp483.f1=_tmp61D;});_tmp483;});void*_tmp388[1U];_tmp388[0]=& _tmp38A;({struct Cyc___cycFILE*_tmp61F=Cyc_stderr;struct _fat_ptr _tmp61E=({const char*_tmp389="Error: can't change directory to %s\n";_tag_fat(_tmp389,sizeof(char),37U);});Cyc_fprintf(_tmp61F,_tmp61E,_tag_fat(_tmp388,sizeof(void*),1U));});});
return 1;}}
# 1889
if((int)Cyc_mode == (int)Cyc_GATHER){
# 1897
struct _fat_ptr cmd=({struct Cyc_String_pa_PrintArg_struct _tmp391=({struct Cyc_String_pa_PrintArg_struct _tmp486;_tmp486.tag=0U,_tmp486.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_cyclone_cc);_tmp486;});struct Cyc_String_pa_PrintArg_struct _tmp392=({struct Cyc_String_pa_PrintArg_struct _tmp485;_tmp485.tag=0U,_tmp485.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_target_cflags);_tmp485;});void*_tmp38F[2U];_tmp38F[0]=& _tmp391,_tmp38F[1]=& _tmp392;({struct _fat_ptr _tmp620=({const char*_tmp390="echo | %s %s -E -dM - -o INITMACROS.h\n";_tag_fat(_tmp390,sizeof(char),39U);});Cyc_aprintf(_tmp620,_tag_fat(_tmp38F,sizeof(void*),2U));});});
# 1899
if(Cyc_verbose)
({struct Cyc_String_pa_PrintArg_struct _tmp38E=({struct Cyc_String_pa_PrintArg_struct _tmp484;_tmp484.tag=0U,_tmp484.f1=(struct _fat_ptr)((struct _fat_ptr)cmd);_tmp484;});void*_tmp38C[1U];_tmp38C[0]=& _tmp38E;({struct Cyc___cycFILE*_tmp622=Cyc_stderr;struct _fat_ptr _tmp621=({const char*_tmp38D="%s\n";_tag_fat(_tmp38D,sizeof(char),4U);});Cyc_fprintf(_tmp622,_tmp621,_tag_fat(_tmp38C,sizeof(void*),1U));});});
# 1899
({system((const char*)_check_null(_untag_fat_ptr(cmd,sizeof(char),1U)));});}{
# 1889
struct Cyc_Lexing_lexbuf*l=({Cyc_Lexing_from_file(in_file);});
# 1905
struct _tuple29*entry;
while((entry=({Cyc_spec(l);}))!= 0){
struct _tuple29*_stmttmp24=(struct _tuple29*)_check_null(entry);struct Cyc_List_List*_tmp39A;struct Cyc_List_List*_tmp399;struct Cyc_List_List*_tmp398;struct Cyc_List_List*_tmp397;struct Cyc_List_List*_tmp396;struct Cyc_List_List*_tmp395;struct Cyc_List_List*_tmp394;struct _fat_ptr _tmp393;_LL1: _tmp393=_stmttmp24->f1;_tmp394=_stmttmp24->f2;_tmp395=_stmttmp24->f3;_tmp396=_stmttmp24->f4;_tmp397=_stmttmp24->f5;_tmp398=_stmttmp24->f6;_tmp399=_stmttmp24->f7;_tmp39A=_stmttmp24->f8;_LL2: {struct _fat_ptr headerfile=_tmp393;struct Cyc_List_List*start_symbols=_tmp394;struct Cyc_List_List*user_defs=_tmp395;struct Cyc_List_List*omit_symbols=_tmp396;struct Cyc_List_List*hstubs=_tmp397;struct Cyc_List_List*cstubs=_tmp398;struct Cyc_List_List*cycstubs=_tmp399;struct Cyc_List_List*cpp_insert=_tmp39A;
# 1909
if(({Cyc_process_file((const char*)_check_null(_untag_fat_ptr(headerfile,sizeof(char),1U)),start_symbols,user_defs,omit_symbols,hstubs,cstubs,cycstubs,cpp_insert);}))
# 1911
return 1;}}
# 1913
({Cyc_fclose(in_file);});
# 1915
if((int)Cyc_mode != (int)Cyc_GATHERSCRIPT){
if(({chdir((const char*)((char*)_check_null(_untag_fat_ptr(cwd,sizeof(char),1U))));})){
({struct Cyc_String_pa_PrintArg_struct _tmp39D=({struct Cyc_String_pa_PrintArg_struct _tmp487;_tmp487.tag=0U,_tmp487.f1=(struct _fat_ptr)((struct _fat_ptr)cwd);_tmp487;});void*_tmp39B[1U];_tmp39B[0]=& _tmp39D;({struct Cyc___cycFILE*_tmp624=Cyc_stderr;struct _fat_ptr _tmp623=({const char*_tmp39C="Error: could not change directory to %s\n";_tag_fat(_tmp39C,sizeof(char),41U);});Cyc_fprintf(_tmp624,_tmp623,_tag_fat(_tmp39B,sizeof(void*),1U));});});
return 1;}}
# 1915
return 0;}}}}
# 1925
int Cyc_process_setjmp(const char*dir){
# 1928
struct _fat_ptr buf=({char*_tmp3BB=({unsigned _tmp3BA=1024U + 1U;char*_tmp3B9=_cycalloc_atomic(_check_times(_tmp3BA,sizeof(char)));({{unsigned _tmp48B=1024U;unsigned i;for(i=0;i < _tmp48B;++ i){_tmp3B9[i]='\000';}_tmp3B9[_tmp48B]=0;}0;});_tmp3B9;});_tag_fat(_tmp3BB,sizeof(char),1025U);});
struct _fat_ptr cwd=({Cyc_getcwd(buf,_get_fat_size(buf,sizeof(char)));});
if(({chdir(dir);})){
({struct Cyc_String_pa_PrintArg_struct _tmp3A4=({struct Cyc_String_pa_PrintArg_struct _tmp489;_tmp489.tag=0U,({struct _fat_ptr _tmp625=(struct _fat_ptr)({const char*_tmp3A5=dir;_tag_fat(_tmp3A5,sizeof(char),_get_zero_arr_size_char((void*)_tmp3A5,1U));});_tmp489.f1=_tmp625;});_tmp489;});void*_tmp3A2[1U];_tmp3A2[0]=& _tmp3A4;({struct Cyc___cycFILE*_tmp627=Cyc_stderr;struct _fat_ptr _tmp626=({const char*_tmp3A3="Error: can't change directory to %s\n";_tag_fat(_tmp3A3,sizeof(char),37U);});Cyc_fprintf(_tmp627,_tmp626,_tag_fat(_tmp3A2,sizeof(void*),1U));});});
return 1;}
# 1930
if(({int(*_tmp3A6)(const char*filename,struct Cyc_List_List*start_symbols,struct Cyc_List_List*user_defs,struct Cyc_List_List*omit_symbols,struct Cyc_List_List*hstubs,struct Cyc_List_List*cstubs,struct Cyc_List_List*cycstubs,struct Cyc_List_List*cpp_insert)=Cyc_process_file;const char*_tmp3A7="setjmp.h";struct Cyc_List_List*_tmp3A8=({struct _fat_ptr*_tmp3A9[1U];({struct _fat_ptr*_tmp629=({struct _fat_ptr*_tmp3AB=_cycalloc(sizeof(*_tmp3AB));({
# 1934
struct _fat_ptr _tmp628=({const char*_tmp3AA="jmp_buf";_tag_fat(_tmp3AA,sizeof(char),8U);});*_tmp3AB=_tmp628;});_tmp3AB;});_tmp3A9[0]=_tmp629;});Cyc_List_list(_tag_fat(_tmp3A9,sizeof(struct _fat_ptr*),1U));});struct Cyc_List_List*_tmp3AC=0;struct Cyc_List_List*_tmp3AD=0;struct Cyc_List_List*_tmp3AE=({struct _tuple27*_tmp3AF[1U];({struct _tuple27*_tmp62C=({struct _tuple27*_tmp3B2=_cycalloc(sizeof(*_tmp3B2));
({struct _fat_ptr _tmp62B=({const char*_tmp3B0="setjmp";_tag_fat(_tmp3B0,sizeof(char),7U);});_tmp3B2->f1=_tmp62B;}),({struct _fat_ptr _tmp62A=({const char*_tmp3B1="extern int setjmp(jmp_buf);\n";_tag_fat(_tmp3B1,sizeof(char),29U);});_tmp3B2->f2=_tmp62A;});_tmp3B2;});_tmp3AF[0]=_tmp62C;});Cyc_List_list(_tag_fat(_tmp3AF,sizeof(struct _tuple27*),1U));});struct Cyc_List_List*_tmp3B3=0;struct Cyc_List_List*_tmp3B4=0;struct Cyc_List_List*_tmp3B5=0;_tmp3A6(_tmp3A7,_tmp3A8,_tmp3AC,_tmp3AD,_tmp3AE,_tmp3B3,_tmp3B4,_tmp3B5);}))
# 1937
return 1;
# 1930
if(({chdir((const char*)((char*)_check_null(_untag_fat_ptr(cwd,sizeof(char),1U))));})){
# 1939
({struct Cyc_String_pa_PrintArg_struct _tmp3B8=({struct Cyc_String_pa_PrintArg_struct _tmp48A;_tmp48A.tag=0U,_tmp48A.f1=(struct _fat_ptr)((struct _fat_ptr)cwd);_tmp48A;});void*_tmp3B6[1U];_tmp3B6[0]=& _tmp3B8;({struct Cyc___cycFILE*_tmp62E=Cyc_stderr;struct _fat_ptr _tmp62D=({const char*_tmp3B7="Error: could not change directory to %s\n";_tag_fat(_tmp3B7,sizeof(char),41U);});Cyc_fprintf(_tmp62E,_tmp62D,_tag_fat(_tmp3B6,sizeof(void*),1U));});});
return 1;}
# 1930
return 0;}static char _tmp3BD[13U]="BUILDLIB.OUT";
# 1925
static struct _fat_ptr Cyc_output_dir={_tmp3BD,_tmp3BD,_tmp3BD + 13U};
# 1947
static void Cyc_set_output_dir(struct _fat_ptr s){
Cyc_output_dir=s;}
# 1947
static struct Cyc_List_List*Cyc_spec_files=0;
# 1951
static void Cyc_add_spec_file(struct _fat_ptr s){
Cyc_spec_files=({struct Cyc_List_List*_tmp3BF=_cycalloc(sizeof(*_tmp3BF));_tmp3BF->hd=(const char*)_check_null(_untag_fat_ptr(s,sizeof(char),1U)),_tmp3BF->tl=Cyc_spec_files;_tmp3BF;});}
# 1954
static int Cyc_no_other(struct _fat_ptr s){return 0;}
static void Cyc_set_GATHER(){
Cyc_mode=Cyc_GATHER;}
# 1958
static void Cyc_set_GATHERSCRIPT(){
Cyc_mode=Cyc_GATHERSCRIPT;}
# 1961
static void Cyc_set_FINISH(){
Cyc_mode=Cyc_FINISH;}
# 1964
static void Cyc_add_cpparg(struct _fat_ptr s){
Cyc_cppargs=({struct Cyc_List_List*_tmp3C6=_cycalloc(sizeof(*_tmp3C6));({struct _fat_ptr*_tmp62F=({struct _fat_ptr*_tmp3C5=_cycalloc(sizeof(*_tmp3C5));*_tmp3C5=s;_tmp3C5;});_tmp3C6->hd=_tmp62F;}),_tmp3C6->tl=Cyc_cppargs;_tmp3C6;});}
# 1964
static int Cyc_badparse=0;
# 1968
static void Cyc_unsupported_option(struct _fat_ptr s){
({struct Cyc_String_pa_PrintArg_struct _tmp3CA=({struct Cyc_String_pa_PrintArg_struct _tmp48C;_tmp48C.tag=0U,_tmp48C.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp48C;});void*_tmp3C8[1U];_tmp3C8[0]=& _tmp3CA;({struct Cyc___cycFILE*_tmp631=Cyc_stderr;struct _fat_ptr _tmp630=({const char*_tmp3C9="Unsupported option %s\n";_tag_fat(_tmp3C9,sizeof(char),23U);});Cyc_fprintf(_tmp631,_tmp630,_tag_fat(_tmp3C8,sizeof(void*),1U));});});
Cyc_badparse=1;}
# 1976
void GC_blacklist_warn_clear();struct _tuple31{struct _fat_ptr f1;int f2;struct _fat_ptr f3;void*f4;struct _fat_ptr f5;};
int Cyc_main(int argc,struct _fat_ptr argv){
({GC_blacklist_warn_clear();});{
# 1980
struct Cyc_List_List*options=({struct _tuple31*_tmp409[9U];({
struct _tuple31*_tmp65E=({struct _tuple31*_tmp40E=_cycalloc(sizeof(*_tmp40E));({struct _fat_ptr _tmp65D=({const char*_tmp40A="-d";_tag_fat(_tmp40A,sizeof(char),3U);});_tmp40E->f1=_tmp65D;}),_tmp40E->f2=0,({struct _fat_ptr _tmp65C=({const char*_tmp40B=" <file>";_tag_fat(_tmp40B,sizeof(char),8U);});_tmp40E->f3=_tmp65C;}),({
void*_tmp65B=(void*)({struct Cyc_Arg_String_spec_Arg_Spec_struct*_tmp40C=_cycalloc(sizeof(*_tmp40C));_tmp40C->tag=5U,_tmp40C->f1=Cyc_set_output_dir;_tmp40C;});_tmp40E->f4=_tmp65B;}),({
struct _fat_ptr _tmp65A=({const char*_tmp40D="Set the output directory to <file>";_tag_fat(_tmp40D,sizeof(char),35U);});_tmp40E->f5=_tmp65A;});_tmp40E;});
# 1981
_tmp409[0]=_tmp65E;}),({
# 1984
struct _tuple31*_tmp659=({struct _tuple31*_tmp413=_cycalloc(sizeof(*_tmp413));({struct _fat_ptr _tmp658=({const char*_tmp40F="-gather";_tag_fat(_tmp40F,sizeof(char),8U);});_tmp413->f1=_tmp658;}),_tmp413->f2=0,({struct _fat_ptr _tmp657=({const char*_tmp410="";_tag_fat(_tmp410,sizeof(char),1U);});_tmp413->f3=_tmp657;}),({
void*_tmp656=(void*)({struct Cyc_Arg_Unit_spec_Arg_Spec_struct*_tmp411=_cycalloc(sizeof(*_tmp411));_tmp411->tag=0U,_tmp411->f1=Cyc_set_GATHER;_tmp411;});_tmp413->f4=_tmp656;}),({
struct _fat_ptr _tmp655=({const char*_tmp412="Gather C library info but don't produce Cyclone headers";_tag_fat(_tmp412,sizeof(char),56U);});_tmp413->f5=_tmp655;});_tmp413;});
# 1984
_tmp409[1]=_tmp659;}),({
# 1987
struct _tuple31*_tmp654=({struct _tuple31*_tmp418=_cycalloc(sizeof(*_tmp418));({struct _fat_ptr _tmp653=({const char*_tmp414="-gatherscript";_tag_fat(_tmp414,sizeof(char),14U);});_tmp418->f1=_tmp653;}),_tmp418->f2=0,({struct _fat_ptr _tmp652=({const char*_tmp415="";_tag_fat(_tmp415,sizeof(char),1U);});_tmp418->f3=_tmp652;}),({
void*_tmp651=(void*)({struct Cyc_Arg_Unit_spec_Arg_Spec_struct*_tmp416=_cycalloc(sizeof(*_tmp416));_tmp416->tag=0U,_tmp416->f1=Cyc_set_GATHERSCRIPT;_tmp416;});_tmp418->f4=_tmp651;}),({
struct _fat_ptr _tmp650=({const char*_tmp417="Produce a script to gather C library info";_tag_fat(_tmp417,sizeof(char),42U);});_tmp418->f5=_tmp650;});_tmp418;});
# 1987
_tmp409[2]=_tmp654;}),({
# 1990
struct _tuple31*_tmp64F=({struct _tuple31*_tmp41D=_cycalloc(sizeof(*_tmp41D));({struct _fat_ptr _tmp64E=({const char*_tmp419="-finish";_tag_fat(_tmp419,sizeof(char),8U);});_tmp41D->f1=_tmp64E;}),_tmp41D->f2=0,({struct _fat_ptr _tmp64D=({const char*_tmp41A="";_tag_fat(_tmp41A,sizeof(char),1U);});_tmp41D->f3=_tmp64D;}),({
void*_tmp64C=(void*)({struct Cyc_Arg_Unit_spec_Arg_Spec_struct*_tmp41B=_cycalloc(sizeof(*_tmp41B));_tmp41B->tag=0U,_tmp41B->f1=Cyc_set_FINISH;_tmp41B;});_tmp41D->f4=_tmp64C;}),({
struct _fat_ptr _tmp64B=({const char*_tmp41C="Produce Cyclone headers from pre-gathered C library info";_tag_fat(_tmp41C,sizeof(char),57U);});_tmp41D->f5=_tmp64B;});_tmp41D;});
# 1990
_tmp409[3]=_tmp64F;}),({
# 1993
struct _tuple31*_tmp64A=({struct _tuple31*_tmp422=_cycalloc(sizeof(*_tmp422));({struct _fat_ptr _tmp649=({const char*_tmp41E="-setjmp";_tag_fat(_tmp41E,sizeof(char),8U);});_tmp422->f1=_tmp649;}),_tmp422->f2=0,({struct _fat_ptr _tmp648=({const char*_tmp41F="";_tag_fat(_tmp41F,sizeof(char),1U);});_tmp422->f3=_tmp648;}),({
void*_tmp647=(void*)({struct Cyc_Arg_Set_spec_Arg_Spec_struct*_tmp420=_cycalloc(sizeof(*_tmp420));_tmp420->tag=3U,_tmp420->f1=& Cyc_do_setjmp;_tmp420;});_tmp422->f4=_tmp647;}),({
# 1998
struct _fat_ptr _tmp646=({const char*_tmp421="Produce the jmp_buf and setjmp declarations on the standard output, for use by the Cyclone compiler special file cyc_setjmp.h.  Cannot be used with -gather, -gatherscript, or specfiles.";_tag_fat(_tmp421,sizeof(char),186U);});_tmp422->f5=_tmp646;});_tmp422;});
# 1993
_tmp409[4]=_tmp64A;}),({
# 1999
struct _tuple31*_tmp645=({struct _tuple31*_tmp427=_cycalloc(sizeof(*_tmp427));({struct _fat_ptr _tmp644=({const char*_tmp423="-b";_tag_fat(_tmp423,sizeof(char),3U);});_tmp427->f1=_tmp644;}),_tmp427->f2=0,({struct _fat_ptr _tmp643=({const char*_tmp424=" <machine>";_tag_fat(_tmp424,sizeof(char),11U);});_tmp427->f3=_tmp643;}),({
void*_tmp642=(void*)({struct Cyc_Arg_String_spec_Arg_Spec_struct*_tmp425=_cycalloc(sizeof(*_tmp425));_tmp425->tag=5U,_tmp425->f1=Cyc_Specsfile_set_target_arch;_tmp425;});_tmp427->f4=_tmp642;}),({
struct _fat_ptr _tmp641=({const char*_tmp426="Set the target machine for compilation to <machine>";_tag_fat(_tmp426,sizeof(char),52U);});_tmp427->f5=_tmp641;});_tmp427;});
# 1999
_tmp409[5]=_tmp645;}),({
# 2002
struct _tuple31*_tmp640=({struct _tuple31*_tmp42C=_cycalloc(sizeof(*_tmp42C));({struct _fat_ptr _tmp63F=({const char*_tmp428="-B";_tag_fat(_tmp428,sizeof(char),3U);});_tmp42C->f1=_tmp63F;}),_tmp42C->f2=1,({struct _fat_ptr _tmp63E=({const char*_tmp429="<file>";_tag_fat(_tmp429,sizeof(char),7U);});_tmp42C->f3=_tmp63E;}),({
void*_tmp63D=(void*)({struct Cyc_Arg_Flag_spec_Arg_Spec_struct*_tmp42A=_cycalloc(sizeof(*_tmp42A));_tmp42A->tag=1U,_tmp42A->f1=Cyc_Specsfile_add_cyclone_exec_path;_tmp42A;});_tmp42C->f4=_tmp63D;}),({
struct _fat_ptr _tmp63C=({const char*_tmp42B="Add to the list of directories to search for compiler files";_tag_fat(_tmp42B,sizeof(char),60U);});_tmp42C->f5=_tmp63C;});_tmp42C;});
# 2002
_tmp409[6]=_tmp640;}),({
# 2005
struct _tuple31*_tmp63B=({struct _tuple31*_tmp431=_cycalloc(sizeof(*_tmp431));({struct _fat_ptr _tmp63A=({const char*_tmp42D="-v";_tag_fat(_tmp42D,sizeof(char),3U);});_tmp431->f1=_tmp63A;}),_tmp431->f2=0,({struct _fat_ptr _tmp639=({const char*_tmp42E="";_tag_fat(_tmp42E,sizeof(char),1U);});_tmp431->f3=_tmp639;}),({
void*_tmp638=(void*)({struct Cyc_Arg_Set_spec_Arg_Spec_struct*_tmp42F=_cycalloc(sizeof(*_tmp42F));_tmp42F->tag=3U,_tmp42F->f1=& Cyc_verbose;_tmp42F;});_tmp431->f4=_tmp638;}),({
struct _fat_ptr _tmp637=({const char*_tmp430="Verbose operation";_tag_fat(_tmp430,sizeof(char),18U);});_tmp431->f5=_tmp637;});_tmp431;});
# 2005
_tmp409[7]=_tmp63B;}),({
# 2008
struct _tuple31*_tmp636=({struct _tuple31*_tmp436=_cycalloc(sizeof(*_tmp436));({struct _fat_ptr _tmp635=({const char*_tmp432="-";_tag_fat(_tmp432,sizeof(char),2U);});_tmp436->f1=_tmp635;}),_tmp436->f2=1,({struct _fat_ptr _tmp634=({const char*_tmp433="";_tag_fat(_tmp433,sizeof(char),1U);});_tmp436->f3=_tmp634;}),({
void*_tmp633=(void*)({struct Cyc_Arg_Flag_spec_Arg_Spec_struct*_tmp434=_cycalloc(sizeof(*_tmp434));_tmp434->tag=1U,_tmp434->f1=Cyc_add_cpparg;_tmp434;});_tmp436->f4=_tmp633;}),({
struct _fat_ptr _tmp632=({const char*_tmp435="";_tag_fat(_tmp435,sizeof(char),1U);});_tmp436->f5=_tmp632;});_tmp436;});
# 2008
_tmp409[8]=_tmp636;});Cyc_List_list(_tag_fat(_tmp409,sizeof(struct _tuple31*),9U));});
# 2013
struct _fat_ptr otherargs=({({struct Cyc_List_List*_tmp660=options;struct _fat_ptr _tmp65F=({const char*_tmp408="Options:";_tag_fat(_tmp408,sizeof(char),9U);});Cyc_Specsfile_parse_b(_tmp660,Cyc_add_spec_file,Cyc_no_other,_tmp65F,argv);});});
# 2015
Cyc_Arg_current=0;
({({struct Cyc_List_List*_tmp662=options;struct _fat_ptr _tmp661=({const char*_tmp3CC="Options:";_tag_fat(_tmp3CC,sizeof(char),9U);});Cyc_Arg_parse(_tmp662,Cyc_add_spec_file,Cyc_no_other,_tmp661,otherargs);});});
if((((Cyc_badparse ||
 !Cyc_do_setjmp && Cyc_spec_files == 0)||
 Cyc_do_setjmp && Cyc_spec_files != 0)||
 Cyc_do_setjmp &&(int)Cyc_mode == (int)Cyc_GATHER)||
 Cyc_do_setjmp &&(int)Cyc_mode == (int)Cyc_GATHERSCRIPT){
({({struct Cyc_List_List*_tmp663=options;Cyc_Arg_usage(_tmp663,({const char*_tmp3CD="Usage: buildlib [options] specfile1 specfile2 ...\nOptions:";_tag_fat(_tmp3CD,sizeof(char),59U);}));});});
# 2025
return 1;}{
# 2017
struct _fat_ptr specs_file=({Cyc_Specsfile_find_in_arch_path(({const char*_tmp407="cycspecs";_tag_fat(_tmp407,sizeof(char),9U);}));});
# 2032
if(Cyc_verbose)({struct Cyc_String_pa_PrintArg_struct _tmp3D0=({struct Cyc_String_pa_PrintArg_struct _tmp48D;_tmp48D.tag=0U,_tmp48D.f1=(struct _fat_ptr)((struct _fat_ptr)specs_file);_tmp48D;});void*_tmp3CE[1U];_tmp3CE[0]=& _tmp3D0;({struct Cyc___cycFILE*_tmp665=Cyc_stderr;struct _fat_ptr _tmp664=({const char*_tmp3CF="Reading from specs file %s\n";_tag_fat(_tmp3CF,sizeof(char),28U);});Cyc_fprintf(_tmp665,_tmp664,_tag_fat(_tmp3CE,sizeof(void*),1U));});});{struct Cyc_List_List*specs=({Cyc_Specsfile_read_specs(specs_file);});
# 2034
Cyc_target_cflags=({({struct Cyc_List_List*_tmp666=specs;Cyc_Specsfile_get_spec(_tmp666,({const char*_tmp3D1="cyclone_target_cflags";_tag_fat(_tmp3D1,sizeof(char),22U);}));});});
if(Cyc_verbose)({struct Cyc_String_pa_PrintArg_struct _tmp3D4=({struct Cyc_String_pa_PrintArg_struct _tmp48E;_tmp48E.tag=0U,_tmp48E.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_target_cflags);_tmp48E;});void*_tmp3D2[1U];_tmp3D2[0]=& _tmp3D4;({struct Cyc___cycFILE*_tmp668=Cyc_stderr;struct _fat_ptr _tmp667=({const char*_tmp3D3="Target cflags are %s\n";_tag_fat(_tmp3D3,sizeof(char),22U);});Cyc_fprintf(_tmp668,_tmp667,_tag_fat(_tmp3D2,sizeof(void*),1U));});});Cyc_cyclone_cc=({({struct Cyc_List_List*_tmp669=specs;Cyc_Specsfile_get_spec(_tmp669,({const char*_tmp3D5="cyclone_cc";_tag_fat(_tmp3D5,sizeof(char),11U);}));});});
# 2037
if(!((unsigned)Cyc_cyclone_cc.curr))Cyc_cyclone_cc=({const char*_tmp3D6="gcc";_tag_fat(_tmp3D6,sizeof(char),4U);});if(Cyc_verbose)
({struct Cyc_String_pa_PrintArg_struct _tmp3D9=({struct Cyc_String_pa_PrintArg_struct _tmp48F;_tmp48F.tag=0U,_tmp48F.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_cyclone_cc);_tmp48F;});void*_tmp3D7[1U];_tmp3D7[0]=& _tmp3D9;({struct Cyc___cycFILE*_tmp66B=Cyc_stderr;struct _fat_ptr _tmp66A=({const char*_tmp3D8="C compiler is %s\n";_tag_fat(_tmp3D8,sizeof(char),18U);});Cyc_fprintf(_tmp66B,_tmp66A,_tag_fat(_tmp3D7,sizeof(void*),1U));});});
# 2037
if((int)Cyc_mode == (int)Cyc_GATHERSCRIPT){
# 2041
if(Cyc_verbose)
({void*_tmp3DA=0U;({struct Cyc___cycFILE*_tmp66D=Cyc_stderr;struct _fat_ptr _tmp66C=({const char*_tmp3DB="Creating BUILDLIB.sh\n";_tag_fat(_tmp3DB,sizeof(char),22U);});Cyc_fprintf(_tmp66D,_tmp66C,_tag_fat(_tmp3DA,sizeof(void*),0U));});});
# 2041
Cyc_script_file=({Cyc_fopen("BUILDLIB.sh","w");});
# 2044
if(!((unsigned)Cyc_script_file)){
({void*_tmp3DC=0U;({struct Cyc___cycFILE*_tmp66F=Cyc_stderr;struct _fat_ptr _tmp66E=({const char*_tmp3DD="Could not create file BUILDLIB.sh\n";_tag_fat(_tmp3DD,sizeof(char),35U);});Cyc_fprintf(_tmp66F,_tmp66E,_tag_fat(_tmp3DC,sizeof(void*),0U));});});
({exit(1);});}
# 2044
({void*_tmp3DE=0U;({struct _fat_ptr _tmp670=({const char*_tmp3DF="#!/bin/sh\n";_tag_fat(_tmp3DF,sizeof(char),11U);});Cyc_prscript(_tmp670,_tag_fat(_tmp3DE,sizeof(void*),0U));});});
# 2049
({void*_tmp3E0=0U;({struct _fat_ptr _tmp671=({const char*_tmp3E1="GCC=\"gcc\"\n";_tag_fat(_tmp3E1,sizeof(char),11U);});Cyc_prscript(_tmp671,_tag_fat(_tmp3E0,sizeof(void*),0U));});});}
# 2037
if(
# 2053
({Cyc_force_directory_prefixes(Cyc_output_dir);})||({Cyc_force_directory(Cyc_output_dir);})){
({struct Cyc_String_pa_PrintArg_struct _tmp3E4=({struct Cyc_String_pa_PrintArg_struct _tmp490;_tmp490.tag=0U,_tmp490.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_output_dir);_tmp490;});void*_tmp3E2[1U];_tmp3E2[0]=& _tmp3E4;({struct Cyc___cycFILE*_tmp673=Cyc_stderr;struct _fat_ptr _tmp672=({const char*_tmp3E3="Error: could not create directory %s\n";_tag_fat(_tmp3E3,sizeof(char),38U);});Cyc_fprintf(_tmp673,_tmp672,_tag_fat(_tmp3E2,sizeof(void*),1U));});});
return 1;}
# 2037
if(Cyc_verbose)
# 2058
({struct Cyc_String_pa_PrintArg_struct _tmp3E7=({struct Cyc_String_pa_PrintArg_struct _tmp491;_tmp491.tag=0U,_tmp491.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_output_dir);_tmp491;});void*_tmp3E5[1U];_tmp3E5[0]=& _tmp3E7;({struct Cyc___cycFILE*_tmp675=Cyc_stderr;struct _fat_ptr _tmp674=({const char*_tmp3E6="Output directory is %s\n";_tag_fat(_tmp3E6,sizeof(char),24U);});Cyc_fprintf(_tmp675,_tmp674,_tag_fat(_tmp3E5,sizeof(void*),1U));});});
# 2037
if((int)Cyc_mode == (int)Cyc_GATHERSCRIPT){
# 2061
({struct Cyc_String_pa_PrintArg_struct _tmp3EA=({struct Cyc_String_pa_PrintArg_struct _tmp492;_tmp492.tag=0U,_tmp492.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_output_dir);_tmp492;});void*_tmp3E8[1U];_tmp3E8[0]=& _tmp3EA;({struct _fat_ptr _tmp676=({const char*_tmp3E9="cd %s\n";_tag_fat(_tmp3E9,sizeof(char),7U);});Cyc_prscript(_tmp676,_tag_fat(_tmp3E8,sizeof(void*),1U));});});
({struct Cyc_String_pa_PrintArg_struct _tmp3ED=({struct Cyc_String_pa_PrintArg_struct _tmp493;_tmp493.tag=0U,_tmp493.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_target_cflags);_tmp493;});void*_tmp3EB[1U];_tmp3EB[0]=& _tmp3ED;({struct _fat_ptr _tmp677=({const char*_tmp3EC="echo | $GCC %s -E -dM - -o INITMACROS.h\n";_tag_fat(_tmp3EC,sizeof(char),41U);});Cyc_prscript(_tmp677,_tag_fat(_tmp3EB,sizeof(void*),1U));});});}
# 2037
if(!({Cyc_gathering();})){
# 2069
Cyc_log_file=({struct Cyc___cycFILE*(*_tmp3EE)(const char*,const char*)=Cyc_fopen;const char*_tmp3EF=(const char*)_check_null(_untag_fat_ptr(({({struct _fat_ptr _tmp678=Cyc_output_dir;Cyc_Filename_concat(_tmp678,({const char*_tmp3F0="BUILDLIB.LOG";_tag_fat(_tmp3F0,sizeof(char),13U);}));});}),sizeof(char),1U));const char*_tmp3F1="w";_tmp3EE(_tmp3EF,_tmp3F1);});
if(!((unsigned)Cyc_log_file)){
({struct Cyc_String_pa_PrintArg_struct _tmp3F4=({struct Cyc_String_pa_PrintArg_struct _tmp494;_tmp494.tag=0U,_tmp494.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_output_dir);_tmp494;});void*_tmp3F2[1U];_tmp3F2[0]=& _tmp3F4;({struct Cyc___cycFILE*_tmp67A=Cyc_stderr;struct _fat_ptr _tmp679=({const char*_tmp3F3="Error: could not create log file in directory %s\n";_tag_fat(_tmp3F3,sizeof(char),50U);});Cyc_fprintf(_tmp67A,_tmp679,_tag_fat(_tmp3F2,sizeof(void*),1U));});});
return 1;}
# 2070
if(!Cyc_do_setjmp){
# 2077
Cyc_cstubs_file=({struct Cyc___cycFILE*(*_tmp3F5)(const char*,const char*)=Cyc_fopen;const char*_tmp3F6=(const char*)_check_null(_untag_fat_ptr(({({struct _fat_ptr _tmp67B=Cyc_output_dir;Cyc_Filename_concat(_tmp67B,({const char*_tmp3F7="cstubs.c";_tag_fat(_tmp3F7,sizeof(char),9U);}));});}),sizeof(char),1U));const char*_tmp3F8="w";_tmp3F5(_tmp3F6,_tmp3F8);});
if(!((unsigned)Cyc_cstubs_file)){
({struct Cyc_String_pa_PrintArg_struct _tmp3FB=({struct Cyc_String_pa_PrintArg_struct _tmp495;_tmp495.tag=0U,_tmp495.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_output_dir);_tmp495;});void*_tmp3F9[1U];_tmp3F9[0]=& _tmp3FB;({struct Cyc___cycFILE*_tmp67D=Cyc_stderr;struct _fat_ptr _tmp67C=({const char*_tmp3FA="Error: could not create cstubs.c in directory %s\n";_tag_fat(_tmp3FA,sizeof(char),50U);});Cyc_fprintf(_tmp67D,_tmp67C,_tag_fat(_tmp3F9,sizeof(void*),1U));});});
return 1;}
# 2078
Cyc_cycstubs_file=({struct Cyc___cycFILE*(*_tmp3FC)(const char*,const char*)=Cyc_fopen;const char*_tmp3FD=(const char*)_check_null(_untag_fat_ptr(({({struct _fat_ptr _tmp67E=Cyc_output_dir;Cyc_Filename_concat(_tmp67E,({const char*_tmp3FE="cycstubs.cyc";_tag_fat(_tmp3FE,sizeof(char),13U);}));});}),sizeof(char),1U));const char*_tmp3FF="w";_tmp3FC(_tmp3FD,_tmp3FF);});
# 2085
if(!((unsigned)Cyc_cycstubs_file)){
({struct Cyc_String_pa_PrintArg_struct _tmp402=({struct Cyc_String_pa_PrintArg_struct _tmp496;_tmp496.tag=0U,_tmp496.f1=(struct _fat_ptr)((struct _fat_ptr)Cyc_output_dir);_tmp496;});void*_tmp400[1U];_tmp400[0]=& _tmp402;({struct Cyc___cycFILE*_tmp680=Cyc_stderr;struct _fat_ptr _tmp67F=({const char*_tmp401="Error: could not create cycstubs.c in directory %s\n";_tag_fat(_tmp401,sizeof(char),52U);});Cyc_fprintf(_tmp680,_tmp67F,_tag_fat(_tmp400,sizeof(void*),1U));});});
# 2089
return 1;}
# 2085
({void*_tmp403=0U;({struct Cyc___cycFILE*_tmp682=(struct Cyc___cycFILE*)_check_null(Cyc_cycstubs_file);struct _fat_ptr _tmp681=({const char*_tmp404="#include <core.h>\nusing Core;\n\n";_tag_fat(_tmp404,sizeof(char),32U);});Cyc_fprintf(_tmp682,_tmp681,_tag_fat(_tmp403,sizeof(void*),0U));});});}}{
# 2037
const char*outdir=(const char*)_check_null(_untag_fat_ptr(Cyc_output_dir,sizeof(char),1U));
# 2099
if(Cyc_do_setjmp &&({Cyc_process_setjmp(outdir);}))
return 1;else{
# 2104
for(0;Cyc_spec_files != 0;Cyc_spec_files=((struct Cyc_List_List*)_check_null(Cyc_spec_files))->tl){
if(({Cyc_process_specfile((const char*)((struct Cyc_List_List*)_check_null(Cyc_spec_files))->hd,outdir);})){
({void*_tmp405=0U;({struct Cyc___cycFILE*_tmp684=Cyc_stderr;struct _fat_ptr _tmp683=({const char*_tmp406="FATAL ERROR -- QUIT!\n";_tag_fat(_tmp406,sizeof(char),22U);});Cyc_fprintf(_tmp684,_tmp683,_tag_fat(_tmp405,sizeof(void*),0U));});});
({exit(1);});}}}
# 2112
if((int)Cyc_mode == (int)Cyc_GATHERSCRIPT)
({Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_script_file));});else{
# 2115
if(!({Cyc_gathering();})){
({Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_log_file));});
if(!Cyc_do_setjmp){
({Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_cstubs_file));});
({Cyc_fclose((struct Cyc___cycFILE*)_check_null(Cyc_cycstubs_file));});}}}
# 2112
return 0;}}}}}
