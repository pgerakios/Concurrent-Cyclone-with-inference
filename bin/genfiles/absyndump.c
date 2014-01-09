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
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 61 "list.h"
int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);
# 195
struct Cyc_List_List*Cyc_List_imp_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;
# 51 "cycboot.h"
extern struct Cyc___cycFILE*Cyc_stdout;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);
# 104
int Cyc_fputc(int,struct Cyc___cycFILE*);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 278 "cycboot.h"
int Cyc_file_string_write(struct Cyc___cycFILE*,struct _fat_ptr src,int src_offset,int max_count);
# 38 "string.h"
unsigned long Cyc_strlen(struct _fat_ptr s);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_KindQual{Cyc_Absyn_AnyKind =0U,Cyc_Absyn_MemKind =1U,Cyc_Absyn_BoxKind =2U,Cyc_Absyn_XRgnKind =3U,Cyc_Absyn_RgnKind =4U,Cyc_Absyn_EffKind =5U,Cyc_Absyn_IntKind =6U,Cyc_Absyn_BoolKind =7U,Cyc_Absyn_PtrBndKind =8U};struct Cyc_Absyn_Kind{enum Cyc_Absyn_KindQual kind;enum Cyc_Absyn_AliasQual aliasqual;};struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct{int tag;struct Cyc_Absyn_Kind*f1;};struct Cyc_Absyn_Unknown_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;};struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_Tvar{struct _fat_ptr*name;int identity;void*kind;};struct Cyc_Absyn_PtrLoc{unsigned ptr_loc;unsigned rgn_loc;unsigned zt_loc;};struct Cyc_Absyn_PtrAtts{void*rgn;void*nullable;void*bounds;void*zero_term;struct Cyc_Absyn_PtrLoc*ptrloc;};struct Cyc_Absyn_PtrInfo{void*elt_type;struct Cyc_Absyn_Tqual elt_tq;struct Cyc_Absyn_PtrAtts ptr_atts;};struct Cyc_Absyn_VarargInfo{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;int inject;};struct Cyc_Absyn_Star_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Bot_Absyn_Cap_struct{int tag;};struct Cyc_Absyn_Nat_Absyn_Cap_struct{int tag;int f1;int f2;};struct Cyc_Absyn_RgnEffect{void*name;struct Cyc_List_List*caps;void*parent;};struct Cyc_Absyn_FnInfo{struct Cyc_List_List*tvars;void*effect;struct Cyc_Absyn_Tqual ret_tqual;void*ret_type;struct Cyc_List_List*args;int c_varargs;struct Cyc_Absyn_VarargInfo*cyc_varargs;struct Cyc_List_List*rgn_po;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;struct Cyc_List_List*requires_relns;struct Cyc_Absyn_Exp*ensures_clause;struct Cyc_List_List*ensures_relns;struct Cyc_List_List*ieffect;struct Cyc_List_List*oeffect;struct Cyc_List_List*throws;int reentrant;};struct Cyc_Absyn_UnknownDatatypeInfo{struct _tuple0*name;int is_extensible;};struct _union_DatatypeInfo_UnknownDatatype{int tag;struct Cyc_Absyn_UnknownDatatypeInfo val;};struct _union_DatatypeInfo_KnownDatatype{int tag;struct Cyc_Absyn_Datatypedecl**val;};union Cyc_Absyn_DatatypeInfo{struct _union_DatatypeInfo_UnknownDatatype UnknownDatatype;struct _union_DatatypeInfo_KnownDatatype KnownDatatype;};struct Cyc_Absyn_UnknownDatatypeFieldInfo{struct _tuple0*datatype_name;struct _tuple0*field_name;int is_extensible;};struct _union_DatatypeFieldInfo_UnknownDatatypefield{int tag;struct Cyc_Absyn_UnknownDatatypeFieldInfo val;};struct _tuple1{struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;};struct _union_DatatypeFieldInfo_KnownDatatypefield{int tag;struct _tuple1 val;};union Cyc_Absyn_DatatypeFieldInfo{struct _union_DatatypeFieldInfo_UnknownDatatypefield UnknownDatatypefield;struct _union_DatatypeFieldInfo_KnownDatatypefield KnownDatatypefield;};struct _tuple2{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;struct Cyc_Core_Opt*f3;};struct _union_AggrInfo_UnknownAggr{int tag;struct _tuple2 val;};struct _union_AggrInfo_KnownAggr{int tag;struct Cyc_Absyn_Aggrdecl**val;};union Cyc_Absyn_AggrInfo{struct _union_AggrInfo_UnknownAggr UnknownAggr;struct _union_AggrInfo_KnownAggr KnownAggr;};struct Cyc_Absyn_ArrayInfo{void*elt_type;struct Cyc_Absyn_Tqual tq;struct Cyc_Absyn_Exp*num_elts;void*zero_term;unsigned zt_loc;};struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_TypeDecl{void*r;unsigned loc;};struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_IntCon_Absyn_TyCon_struct{int tag;enum Cyc_Absyn_Sign f1;enum Cyc_Absyn_Size_of f2;};struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct{int tag;int f1;};struct Cyc_Absyn_RgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_XRgnHandleCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TagCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_HeapCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_UniqueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RefCntCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_AccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_CAccessCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_JoinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_RgnsCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_TrueCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FalseCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_ThinCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_FatCon_Absyn_TyCon_struct{int tag;};struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct{int tag;struct _tuple0*f1;struct Cyc_Absyn_Enumdecl*f2;};struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct{int tag;struct _fat_ptr f1;struct Cyc_Absyn_Kind*f2;};struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeInfo f1;};struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_DatatypeFieldInfo f1;};struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct{int tag;union Cyc_Absyn_AggrInfo f1;};struct Cyc_Absyn_AppType_Absyn_Type_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Evar_Absyn_Type_struct{int tag;struct Cyc_Core_Opt*f1;void*f2;int f3;struct Cyc_Core_Opt*f4;};struct Cyc_Absyn_VarType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Tvar*f1;};struct Cyc_Absyn_PointerType_Absyn_Type_struct{int tag;struct Cyc_Absyn_PtrInfo f1;};struct Cyc_Absyn_ArrayType_Absyn_Type_struct{int tag;struct Cyc_Absyn_ArrayInfo f1;};struct Cyc_Absyn_FnType_Absyn_Type_struct{int tag;struct Cyc_Absyn_FnInfo f1;};struct Cyc_Absyn_TupleType_Absyn_Type_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct{int tag;enum Cyc_Absyn_AggrKind f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_TypedefType_Absyn_Type_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_Typedefdecl*f3;void*f4;};struct Cyc_Absyn_ValueofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct{int tag;struct Cyc_Absyn_TypeDecl*f1;void**f2;};struct Cyc_Absyn_TypeofType_Absyn_Type_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;unsigned f2;};struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct{int tag;struct Cyc_List_List*f1;int f2;struct Cyc_Absyn_VarargInfo*f3;void*f4;struct Cyc_List_List*f5;struct Cyc_Absyn_Exp*f6;struct Cyc_Absyn_Exp*f7;struct Cyc_List_List*f8;struct Cyc_List_List*f9;struct Cyc_List_List*f10;int f11;};
# 478 "absyn.h"
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};
# 518
extern struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct Cyc_Absyn_Stdcall_att_val;
extern struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct Cyc_Absyn_Cdecl_att_val;
extern struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct Cyc_Absyn_Fastcall_att_val;
# 526
extern struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct Cyc_Absyn_Unused_att_val;struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};
# 771 "absyn.h"
extern struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct Cyc_Absyn_Wild_p_val;struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 963
struct Cyc_Absyn_Tqual Cyc_Absyn_empty_tqual(unsigned);
# 968
void*Cyc_Absyn_compress_kb(void*);
# 974
int Cyc_Absyn_type2bool(int def,void*);
# 979
void*Cyc_Absyn_new_evar(struct Cyc_Core_Opt*k,struct Cyc_Core_Opt*tenv);
# 1083
struct Cyc_Absyn_Exp*Cyc_Absyn_times_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1106
struct Cyc_Absyn_Exp*Cyc_Absyn_sizeoftype_exp(void*t,unsigned);
# 1213
struct _fat_ptr Cyc_Absyn_attribute2string(void*);struct _tuple12{enum Cyc_Absyn_AggrKind f1;struct _tuple0*f2;};
# 1223
struct _tuple12 Cyc_Absyn_aggr_kinded_name(union Cyc_Absyn_AggrInfo);
# 1231
struct _tuple0*Cyc_Absyn_binding2qvar(void*);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 53 "absynpp.h"
void Cyc_Absynpp_set_params(struct Cyc_Absynpp_Params*fs);
# 61
struct _fat_ptr Cyc_Absynpp_longlong2string(unsigned long long);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);
# 79
extern struct _fat_ptr*Cyc_Absynpp_cyc_stringptr;
int Cyc_Absynpp_exp_prec(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_char_escape(char);
struct _fat_ptr Cyc_Absynpp_string_escape(struct _fat_ptr);
struct _fat_ptr Cyc_Absynpp_prim2str(enum Cyc_Absyn_Primop p);
int Cyc_Absynpp_is_declaration(struct Cyc_Absyn_Stmt*s);struct _tuple13{struct Cyc_Absyn_Tqual f1;void*f2;struct Cyc_List_List*f3;};
struct _tuple13 Cyc_Absynpp_to_tms(struct _RegionHandle*,struct Cyc_Absyn_Tqual tq,void*t);struct _tuple14{int f1;struct Cyc_List_List*f2;};
# 94 "absynpp.h"
struct _tuple14 Cyc_Absynpp_shadows(struct Cyc_Absyn_Decl*d,struct Cyc_List_List*varsinblock);struct _tuple15{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple15 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 110 "tcutil.h"
void*Cyc_Tcutil_compress(void*);
# 40 "warn.h"
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};
# 26 "cyclone.h"
enum Cyc_Cyclone_C_Compilers{Cyc_Cyclone_Gcc_c =0U,Cyc_Cyclone_Vc_c =1U};
# 32
extern enum Cyc_Cyclone_C_Compilers Cyc_Cyclone_c_compiler;struct _tuple16{struct _fat_ptr f1;unsigned f2;};
# 27 "absyndump.cyc"
struct _tuple16 Cyc_Lex_xlate_pos(unsigned);
# 37
static int Cyc_Absyndump_expand_typedefs;
# 41
static int Cyc_Absyndump_qvar_to_Cids;
# 44
static int Cyc_Absyndump_add_cyc_prefix;
# 47
static int Cyc_Absyndump_generate_line_directives;
# 51
static int Cyc_Absyndump_to_VC;
# 53
void Cyc_Absyndump_set_params(struct Cyc_Absynpp_Params*fs){
Cyc_Absyndump_expand_typedefs=fs->expand_typedefs;
Cyc_Absyndump_qvar_to_Cids=fs->qvar_to_Cids;
Cyc_Absyndump_add_cyc_prefix=fs->add_cyc_prefix;
Cyc_Absyndump_to_VC=fs->to_VC;
Cyc_Absyndump_generate_line_directives=fs->generate_line_directives;
# 68 "absyndump.cyc"
({Cyc_Absynpp_set_params(fs);});}
# 53 "absyndump.cyc"
static void Cyc_Absyndump_dumptyp(void*);
# 72 "absyndump.cyc"
static void Cyc_Absyndump_dumpntyp(void*t);
static void Cyc_Absyndump_dumpexp(struct Cyc_Absyn_Exp*);
static void Cyc_Absyndump_dumpexp_prec(int,struct Cyc_Absyn_Exp*);
static void Cyc_Absyndump_dumppat(struct Cyc_Absyn_Pat*);
static void Cyc_Absyndump_dumpstmt(struct Cyc_Absyn_Stmt*,int expstmt,struct Cyc_List_List*);
static void Cyc_Absyndump_dumpvardecl(struct Cyc_Absyn_Vardecl*,unsigned,int do_semi);
static void Cyc_Absyndump_dump_aggrdecl(struct Cyc_Absyn_Aggrdecl*);
static void Cyc_Absyndump_dump_enumdecl(struct Cyc_Absyn_Enumdecl*);
static void Cyc_Absyndump_dump_datatypedecl(struct Cyc_Absyn_Datatypedecl*);
static void Cyc_Absyndump_dumpdecl(struct Cyc_Absyn_Decl*);
static void Cyc_Absyndump_dumptms(int is_char_ptr,struct Cyc_List_List*,void(*f)(void*),void*a);
static void Cyc_Absyndump_dumptqtd(struct Cyc_Absyn_Tqual,void*,void(*f)(void*),void*);
static void Cyc_Absyndump_dumpaggrfields(struct Cyc_List_List*fields);
static void Cyc_Absyndump_dumpenumfields(struct Cyc_List_List*fields);
# 90
static void Cyc_Absyndump_dumploc(unsigned);
# 93
struct Cyc___cycFILE**Cyc_Absyndump_dump_file=& Cyc_stdout;
static char Cyc_Absyndump_prev_char='x';
# 96
static int Cyc_Absyndump_need_space_before(){
char _tmp1=Cyc_Absyndump_prev_char;switch(_tmp1){case 123U: _LL1: _LL2:
 goto _LL4;case 125U: _LL3: _LL4:
 goto _LL6;case 40U: _LL5: _LL6:
 goto _LL8;case 41U: _LL7: _LL8:
 goto _LLA;case 91U: _LL9: _LLA:
 goto _LLC;case 93U: _LLB: _LLC:
 goto _LLE;case 59U: _LLD: _LLE:
 goto _LL10;case 44U: _LLF: _LL10:
 goto _LL12;case 61U: _LL11: _LL12:
 goto _LL14;case 63U: _LL13: _LL14:
 goto _LL16;case 33U: _LL15: _LL16:
 goto _LL18;case 32U: _LL17: _LL18:
 goto _LL1A;case 10U: _LL19: _LL1A:
 goto _LL1C;case 42U: _LL1B: _LL1C:
 return 0;default: _LL1D: _LL1E:
 return 1;}_LL0:;}
# 116
static void Cyc_Absyndump_dump(struct _fat_ptr s){
unsigned sz=({Cyc_strlen((struct _fat_ptr)s);});
# 123
if(({Cyc_Absyndump_need_space_before();}))
({Cyc_fputc((int)' ',*Cyc_Absyndump_dump_file);});
# 123
if(sz >= (unsigned)1){
# 126
Cyc_Absyndump_prev_char=*((const char*)_check_fat_subscript(s,sizeof(char),(int)(sz - (unsigned)1)));
({Cyc_file_string_write(*Cyc_Absyndump_dump_file,s,0,(int)sz);});}}
# 131
static void Cyc_Absyndump_dump_nospace(struct _fat_ptr s){
int sz=(int)({Cyc_strlen((struct _fat_ptr)s);});
# 134
if(sz >= 1){
({Cyc_file_string_write(*Cyc_Absyndump_dump_file,s,0,sz);});
Cyc_Absyndump_prev_char=*((const char*)_check_fat_subscript(s,sizeof(char),sz - 1));}}
# 139
static void Cyc_Absyndump_dump_char(int c){
# 141
({Cyc_fputc(c,*Cyc_Absyndump_dump_file);});
Cyc_Absyndump_prev_char=(char)c;}static char _tmp11[1U]="";
# 145
static void Cyc_Absyndump_dumploc(unsigned loc){
static struct _fat_ptr last_file={_tmp11,_tmp11,_tmp11 + 1U};
static unsigned last_line=0U;
if(loc == (unsigned)0)return;if(!Cyc_Absyndump_generate_line_directives)
return;{
# 148
struct _tuple16 _stmttmp0=({Cyc_Lex_xlate_pos(loc);});unsigned _tmp7;struct _fat_ptr _tmp6;_LL1: _tmp6=_stmttmp0.f1;_tmp7=_stmttmp0.f2;_LL2: {struct _fat_ptr f=_tmp6;unsigned d=_tmp7;
# 152
if(({char*_tmp3AA=(char*)f.curr;_tmp3AA == (char*)(_tag_fat(0,0,0)).curr;})||(char*)f.curr == (char*)last_file.curr && d == last_line)return;if(
(char*)f.curr == (char*)last_file.curr && d == last_line + (unsigned)1)
({void*_tmp8=0U;({struct Cyc___cycFILE*_tmp3AC=*Cyc_Absyndump_dump_file;struct _fat_ptr _tmp3AB=({const char*_tmp9="\n";_tag_fat(_tmp9,sizeof(char),2U);});Cyc_fprintf(_tmp3AC,_tmp3AB,_tag_fat(_tmp8,sizeof(void*),0U));});});else{
if((char*)f.curr == (char*)last_file.curr)
({struct Cyc_Int_pa_PrintArg_struct _tmpC=({struct Cyc_Int_pa_PrintArg_struct _tmp39D;_tmp39D.tag=1U,_tmp39D.f1=(unsigned long)((int)d);_tmp39D;});void*_tmpA[1U];_tmpA[0]=& _tmpC;({struct Cyc___cycFILE*_tmp3AE=*Cyc_Absyndump_dump_file;struct _fat_ptr _tmp3AD=({const char*_tmpB="\n# %d\n";_tag_fat(_tmpB,sizeof(char),7U);});Cyc_fprintf(_tmp3AE,_tmp3AD,_tag_fat(_tmpA,sizeof(void*),1U));});});else{
# 158
({struct Cyc_Int_pa_PrintArg_struct _tmpF=({struct Cyc_Int_pa_PrintArg_struct _tmp39F;_tmp39F.tag=1U,_tmp39F.f1=(unsigned long)((int)d);_tmp39F;});struct Cyc_String_pa_PrintArg_struct _tmp10=({struct Cyc_String_pa_PrintArg_struct _tmp39E;_tmp39E.tag=0U,_tmp39E.f1=(struct _fat_ptr)((struct _fat_ptr)f);_tmp39E;});void*_tmpD[2U];_tmpD[0]=& _tmpF,_tmpD[1]=& _tmp10;({struct Cyc___cycFILE*_tmp3B0=*Cyc_Absyndump_dump_file;struct _fat_ptr _tmp3AF=({const char*_tmpE="\n# %d %s\n";_tag_fat(_tmpE,sizeof(char),10U);});Cyc_fprintf(_tmp3B0,_tmp3AF,_tag_fat(_tmpD,sizeof(void*),2U));});});}}
# 160
last_file=f;
last_line=d;}}}
# 166
static void Cyc_Absyndump_dump_str(struct _fat_ptr*s){
({Cyc_Absyndump_dump(*s);});}
# 169
static void Cyc_Absyndump_dump_str_nospace(struct _fat_ptr*s){
({Cyc_Absyndump_dump_nospace(*s);});}
# 173
static void Cyc_Absyndump_dump_semi(){
({Cyc_Absyndump_dump_char((int)';');});}
# 177
static void Cyc_Absyndump_dump_sep(void(*f)(void*),struct Cyc_List_List*l,struct _fat_ptr sep){
if(l == 0)
return;
# 178
for(0;((struct Cyc_List_List*)_check_null(l))->tl != 0;l=l->tl){
# 181
({f(l->hd);});
({Cyc_Absyndump_dump_nospace(sep);});}
# 184
({f(l->hd);});}
# 186
static void Cyc_Absyndump_dump_sep_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*l,struct _fat_ptr sep){
if(l == 0)
return;
# 187
for(0;((struct Cyc_List_List*)_check_null(l))->tl != 0;l=l->tl){
# 190
({f(env,l->hd);});
({Cyc_Absyndump_dump_nospace(sep);});}
# 193
({f(env,l->hd);});}
# 195
static void Cyc_Absyndump_group(void(*f)(void*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep){
# 197
({Cyc_Absyndump_dump_nospace(start);});
({Cyc_Absyndump_dump_sep(f,l,sep);});
({Cyc_Absyndump_dump_nospace(end);});}
# 201
static void Cyc_Absyndump_group_c(void(*f)(void*,void*),void*env,struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep){
# 203
({Cyc_Absyndump_dump_nospace(start);});
({Cyc_Absyndump_dump_sep_c(f,env,l,sep);});
({Cyc_Absyndump_dump_nospace(end);});}
# 207
static void Cyc_Absyndump_egroup(void(*f)(void*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep){
# 209
if(l != 0)
({Cyc_Absyndump_group(f,l,start,end,sep);});}
# 213
static void Cyc_Absyndump_ignore(void*x){;}
# 215
static void Cyc_Absyndump_dumpqvar(struct _tuple0*v){
struct Cyc_List_List*nsl=0;
struct _fat_ptr**prefix=0;
{union Cyc_Absyn_Nmspace _stmttmp1=(*v).f1;union Cyc_Absyn_Nmspace _tmp1C=_stmttmp1;struct Cyc_List_List*_tmp1D;struct Cyc_List_List*_tmp1E;struct Cyc_List_List*_tmp1F;switch((_tmp1C.C_n).tag){case 4U: _LL1: _LL2:
 goto _LL0;case 1U: _LL3: _tmp1F=(_tmp1C.Rel_n).val;_LL4: {struct Cyc_List_List*x=_tmp1F;
nsl=x;goto _LL0;}case 3U: _LL5: _tmp1E=(_tmp1C.C_n).val;_LL6: {struct Cyc_List_List*x=_tmp1E;
goto _LL0;}default: _LL7: _tmp1D=(_tmp1C.Abs_n).val;_LL8: {struct Cyc_List_List*x=_tmp1D;
# 223
if(Cyc_Absyndump_qvar_to_Cids || Cyc_Absyndump_add_cyc_prefix)
prefix=& Cyc_Absynpp_cyc_stringptr;
# 223
nsl=x;
# 226
goto _LL0;}}_LL0:;}
# 228
if(({Cyc_Absyndump_need_space_before();}))
({Cyc_Absyndump_dump_char((int)' ');});{
# 228
struct _fat_ptr sep=
# 230
Cyc_Absyndump_qvar_to_Cids?({const char*_tmp21="_";_tag_fat(_tmp21,sizeof(char),2U);}):({const char*_tmp22="::";_tag_fat(_tmp22,sizeof(char),3U);});
if(prefix != 0){
({Cyc_Absyndump_dump_nospace(*(*prefix));});({Cyc_Absyndump_dump_nospace(sep);});}
# 231
if(nsl != 0){
# 235
({({void(*_tmp3B2)(void(*f)(struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr sep)=({void(*_tmp20)(void(*f)(struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr sep)=(void(*)(void(*f)(struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr sep))Cyc_Absyndump_dump_sep;_tmp20;});struct Cyc_List_List*_tmp3B1=nsl;_tmp3B2(Cyc_Absyndump_dump_str_nospace,_tmp3B1,sep);});});({Cyc_Absyndump_dump_nospace(sep);});}
# 231
({Cyc_Absyndump_dump_nospace(*(*v).f2);});}}
# 240
static void Cyc_Absyndump_dumptq(struct Cyc_Absyn_Tqual tq){
if(tq.q_restrict)({Cyc_Absyndump_dump(({const char*_tmp24="restrict";_tag_fat(_tmp24,sizeof(char),9U);}));});if(tq.q_volatile)
({Cyc_Absyndump_dump(({const char*_tmp25="volatile";_tag_fat(_tmp25,sizeof(char),9U);}));});
# 241
if(tq.print_const)
# 243
({Cyc_Absyndump_dump(({const char*_tmp26="const";_tag_fat(_tmp26,sizeof(char),6U);}));});}
# 245
static void Cyc_Absyndump_dumpscope(enum Cyc_Absyn_Scope sc){
enum Cyc_Absyn_Scope _tmp28=sc;switch(_tmp28){case Cyc_Absyn_Static: _LL1: _LL2:
({Cyc_Absyndump_dump(({const char*_tmp29="static";_tag_fat(_tmp29,sizeof(char),7U);}));});return;case Cyc_Absyn_Public: _LL3: _LL4:
 return;case Cyc_Absyn_Extern: _LL5: _LL6:
({Cyc_Absyndump_dump(({const char*_tmp2A="extern";_tag_fat(_tmp2A,sizeof(char),7U);}));});return;case Cyc_Absyn_ExternC: _LL7: _LL8:
({Cyc_Absyndump_dump(({const char*_tmp2B="extern \"C\"";_tag_fat(_tmp2B,sizeof(char),11U);}));});return;case Cyc_Absyn_Abstract: _LL9: _LLA:
({Cyc_Absyndump_dump(({const char*_tmp2C="abstract";_tag_fat(_tmp2C,sizeof(char),9U);}));});return;case Cyc_Absyn_Register: _LLB: _LLC:
({Cyc_Absyndump_dump(({const char*_tmp2D="register";_tag_fat(_tmp2D,sizeof(char),9U);}));});return;default: _LLD: _LLE:
 return;}_LL0:;}
# 256
static void Cyc_Absyndump_dumpaggr_kind(enum Cyc_Absyn_AggrKind k){
enum Cyc_Absyn_AggrKind _tmp2F=k;if(_tmp2F == Cyc_Absyn_StructA){_LL1: _LL2:
({Cyc_Absyndump_dump(({const char*_tmp30="struct";_tag_fat(_tmp30,sizeof(char),7U);}));});return;}else{_LL3: _LL4:
({Cyc_Absyndump_dump(({const char*_tmp31="union";_tag_fat(_tmp31,sizeof(char),6U);}));});return;}_LL0:;}
# 263
static void Cyc_Absyndump_dumpkind(struct Cyc_Absyn_Kind*ka){
({void(*_tmp33)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp34=({Cyc_Absynpp_kind2string(ka);});_tmp33(_tmp34);});}
# 267
static void Cyc_Absyndump_dumptps(struct Cyc_List_List*ts){
({({struct Cyc_List_List*_tmp3B5=ts;struct _fat_ptr _tmp3B4=({const char*_tmp36="<";_tag_fat(_tmp36,sizeof(char),2U);});struct _fat_ptr _tmp3B3=({const char*_tmp37=">";_tag_fat(_tmp37,sizeof(char),2U);});Cyc_Absyndump_egroup(Cyc_Absyndump_dumptyp,_tmp3B5,_tmp3B4,_tmp3B3,({const char*_tmp38=",";_tag_fat(_tmp38,sizeof(char),2U);}));});});}
# 270
static void Cyc_Absyndump_dumptvar(struct Cyc_Absyn_Tvar*tv){
struct _fat_ptr n=*tv->name;
if((int)*((const char*)_check_fat_subscript(n,sizeof(char),0))== (int)'#'){
({Cyc_Absyndump_dump(({const char*_tmp3A="`G";_tag_fat(_tmp3A,sizeof(char),3U);}));});
{void*_stmttmp2=({Cyc_Absyn_compress_kb(tv->kind);});void*_tmp3B=_stmttmp2;struct Cyc_Absyn_Kind*_tmp3C;struct Cyc_Absyn_Kind*_tmp3D;switch(*((int*)_tmp3B)){case 0U: _LL1: _tmp3D=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp3B)->f1;_LL2: {struct Cyc_Absyn_Kind*k=_tmp3D;
_tmp3C=k;goto _LL4;}case 2U: _LL3: _tmp3C=((struct Cyc_Absyn_Less_kb_Absyn_KindBound_struct*)_tmp3B)->f2;_LL4: {struct Cyc_Absyn_Kind*k=_tmp3C;
({Cyc_Absyndump_dumpkind(k);});goto _LL0;}default: _LL5: _LL6:
({Cyc_Absyndump_dump_nospace(({const char*_tmp3E="K";_tag_fat(_tmp3E,sizeof(char),2U);}));});goto _LL0;}_LL0:;}
# 279
({Cyc_Absyndump_dump_nospace(_fat_ptr_plus(n,sizeof(char),1));});}else{
# 281
({Cyc_Absyndump_dump(n);});}}
# 283
static void Cyc_Absyndump_dumpkindedtvar(struct Cyc_Absyn_Tvar*tv){
({Cyc_Absyndump_dumptvar(tv);});{
void*_stmttmp3=({Cyc_Absyn_compress_kb(tv->kind);});void*_tmp40=_stmttmp3;struct Cyc_Absyn_Kind*_tmp41;switch(*((int*)_tmp40)){case 1U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 goto _LL6;default: if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp40)->f1)->kind == Cyc_Absyn_BoxKind){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp40)->f1)->aliasqual == Cyc_Absyn_Aliasable){_LL5: _LL6:
 goto _LL0;}else{goto _LL7;}}else{_LL7: _tmp41=((struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct*)_tmp40)->f1;_LL8: {struct Cyc_Absyn_Kind*k=_tmp41;
({Cyc_Absyndump_dump(({const char*_tmp42="::";_tag_fat(_tmp42,sizeof(char),3U);}));});({Cyc_Absyndump_dumpkind(k);});goto _LL0;}}}_LL0:;}}
# 292
static void Cyc_Absyndump_dumptvars(struct Cyc_List_List*tvs){
({({void(*_tmp3B9)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp44)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_egroup;_tmp44;});struct Cyc_List_List*_tmp3B8=tvs;struct _fat_ptr _tmp3B7=({const char*_tmp45="<";_tag_fat(_tmp45,sizeof(char),2U);});struct _fat_ptr _tmp3B6=({const char*_tmp46=">";_tag_fat(_tmp46,sizeof(char),2U);});_tmp3B9(Cyc_Absyndump_dumptvar,_tmp3B8,_tmp3B7,_tmp3B6,({const char*_tmp47=",";_tag_fat(_tmp47,sizeof(char),2U);}));});});}
# 295
static void Cyc_Absyndump_dumpkindedtvars(struct Cyc_List_List*tvs){
({({void(*_tmp3BD)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp49)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_egroup;_tmp49;});struct Cyc_List_List*_tmp3BC=tvs;struct _fat_ptr _tmp3BB=({const char*_tmp4A="<";_tag_fat(_tmp4A,sizeof(char),2U);});struct _fat_ptr _tmp3BA=({const char*_tmp4B=">";_tag_fat(_tmp4B,sizeof(char),2U);});_tmp3BD(Cyc_Absyndump_dumpkindedtvar,_tmp3BC,_tmp3BB,_tmp3BA,({const char*_tmp4C=",";_tag_fat(_tmp4C,sizeof(char),2U);}));});});}struct _tuple17{struct Cyc_Absyn_Tqual f1;void*f2;};
# 298
static void Cyc_Absyndump_dumparg(struct _tuple17*pr){
({({void(*_tmp3C0)(struct Cyc_Absyn_Tqual,void*,void(*f)(int),int)=({void(*_tmp4E)(struct Cyc_Absyn_Tqual,void*,void(*f)(int),int)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(int),int))Cyc_Absyndump_dumptqtd;_tmp4E;});struct Cyc_Absyn_Tqual _tmp3BF=(*pr).f1;void*_tmp3BE=(*pr).f2;_tmp3C0(_tmp3BF,_tmp3BE,({void(*_tmp4F)(int x)=(void(*)(int x))Cyc_Absyndump_ignore;_tmp4F;}),0);});});}
# 301
static void Cyc_Absyndump_dumpargs(struct Cyc_List_List*ts){
({({void(*_tmp3C4)(void(*f)(struct _tuple17*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp51)(void(*f)(struct _tuple17*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple17*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp51;});struct Cyc_List_List*_tmp3C3=ts;struct _fat_ptr _tmp3C2=({const char*_tmp52="(";_tag_fat(_tmp52,sizeof(char),2U);});struct _fat_ptr _tmp3C1=({const char*_tmp53=")";_tag_fat(_tmp53,sizeof(char),2U);});_tmp3C4(Cyc_Absyndump_dumparg,_tmp3C3,_tmp3C2,_tmp3C1,({const char*_tmp54=",";_tag_fat(_tmp54,sizeof(char),2U);}));});});}
# 305
static void Cyc_Absyndump_dump_callconv(struct Cyc_List_List*atts){
for(0;atts != 0;atts=atts->tl){
void*_stmttmp4=(void*)atts->hd;void*_tmp56=_stmttmp4;switch(*((int*)_tmp56)){case 1U: _LL1: _LL2:
({Cyc_Absyndump_dump(({const char*_tmp57="_stdcall";_tag_fat(_tmp57,sizeof(char),9U);}));});return;case 2U: _LL3: _LL4:
({Cyc_Absyndump_dump(({const char*_tmp58="_cdecl";_tag_fat(_tmp58,sizeof(char),7U);}));});return;case 3U: _LL5: _LL6:
({Cyc_Absyndump_dump(({const char*_tmp59="_fastcall";_tag_fat(_tmp59,sizeof(char),10U);}));});return;default: _LL7: _LL8:
 goto _LL0;}_LL0:;}}
# 315
static void Cyc_Absyndump_dump_noncallconv(struct Cyc_List_List*atts){
# 317
int hasatt=0;
{struct Cyc_List_List*atts2=atts;for(0;atts2 != 0;atts2=atts2->tl){
void*_stmttmp5=(void*)((struct Cyc_List_List*)_check_null(atts))->hd;void*_tmp5B=_stmttmp5;switch(*((int*)_tmp5B)){case 1U: _LL1: _LL2:
 goto _LL4;case 2U: _LL3: _LL4:
 goto _LL6;case 3U: _LL5: _LL6:
 goto _LL0;default: _LL7: _LL8:
 hasatt=1;goto _LL0;}_LL0:;}}
# 325
if(!hasatt)
return;
# 325
({Cyc_Absyndump_dump(({const char*_tmp5C="__declspec(";_tag_fat(_tmp5C,sizeof(char),12U);}));});
# 328
for(0;atts != 0;atts=atts->tl){
void*_stmttmp6=(void*)atts->hd;void*_tmp5D=_stmttmp6;switch(*((int*)_tmp5D)){case 1U: _LLA: _LLB:
 goto _LLD;case 2U: _LLC: _LLD:
 goto _LLF;case 3U: _LLE: _LLF:
 goto _LL9;default: _LL10: _LL11:
({void(*_tmp5E)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp5F=({Cyc_Absyn_attribute2string((void*)atts->hd);});_tmp5E(_tmp5F);});goto _LL9;}_LL9:;}
# 335
({Cyc_Absyndump_dump_char((int)')');});}
# 338
static void Cyc_Absyndump_dumpatts(struct Cyc_List_List*atts){
if(atts == 0)return;{enum Cyc_Cyclone_C_Compilers _tmp61=Cyc_Cyclone_c_compiler;if(_tmp61 == Cyc_Cyclone_Gcc_c){_LL1: _LL2:
# 342
({Cyc_Absyndump_dump(({const char*_tmp62=" __attribute__((";_tag_fat(_tmp62,sizeof(char),17U);}));});
for(0;atts != 0;atts=atts->tl){
({void(*_tmp63)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp64=({Cyc_Absyn_attribute2string((void*)atts->hd);});_tmp63(_tmp64);});
if(atts->tl != 0)({Cyc_Absyndump_dump(({const char*_tmp65=",";_tag_fat(_tmp65,sizeof(char),2U);}));});}
# 347
({Cyc_Absyndump_dump(({const char*_tmp66=")) ";_tag_fat(_tmp66,sizeof(char),4U);}));});
return;}else{_LL3: _LL4:
# 350
({Cyc_Absyndump_dump_noncallconv(atts);});
return;}_LL0:;}}
# 355
static void Cyc_Absyndump_dumprgn(void*t){
({Cyc_Absyndump_dumpntyp(t);});}
# 359
static struct _tuple8 Cyc_Absyndump_effects_split(void*t){
struct Cyc_List_List*rgions=0;
struct Cyc_List_List*effects=0;
{void*_stmttmp7=({Cyc_Tcutil_compress(t);});void*_tmp69=_stmttmp7;struct Cyc_List_List*_tmp6A;void*_tmp6B;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp69)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp69)->f1)){case 9U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp69)->f2 != 0){_LL1: _tmp6B=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp69)->f2)->hd;_LL2: {void*r=_tmp6B;
rgions=({struct Cyc_List_List*_tmp6C=_cycalloc(sizeof(*_tmp6C));_tmp6C->hd=r,_tmp6C->tl=rgions;_tmp6C;});goto _LL0;}}else{goto _LL5;}case 11U: _LL3: _tmp6A=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp69)->f2;_LL4: {struct Cyc_List_List*ts=_tmp6A;
# 365
for(0;ts != 0;ts=ts->tl){
struct _tuple8 _stmttmp8=({Cyc_Absyndump_effects_split((void*)ts->hd);});struct Cyc_List_List*_tmp6E;struct Cyc_List_List*_tmp6D;_LL8: _tmp6D=_stmttmp8.f1;_tmp6E=_stmttmp8.f2;_LL9: {struct Cyc_List_List*rs=_tmp6D;struct Cyc_List_List*es=_tmp6E;
rgions=({Cyc_List_imp_append(rs,rgions);});
effects=({Cyc_List_imp_append(es,effects);});}}
# 370
goto _LL0;}default: goto _LL5;}else{_LL5: _LL6:
 effects=({struct Cyc_List_List*_tmp6F=_cycalloc(sizeof(*_tmp6F));_tmp6F->hd=t,_tmp6F->tl=effects;_tmp6F;});goto _LL0;}_LL0:;}
# 373
return({struct _tuple8 _tmp3A0;_tmp3A0.f1=rgions,_tmp3A0.f2=effects;_tmp3A0;});}
# 376
static void Cyc_Absyndump_dumpeff(void*t){
struct _tuple8 _stmttmp9=({Cyc_Absyndump_effects_split(t);});struct Cyc_List_List*_tmp72;struct Cyc_List_List*_tmp71;_LL1: _tmp71=_stmttmp9.f1;_tmp72=_stmttmp9.f2;_LL2: {struct Cyc_List_List*rgions=_tmp71;struct Cyc_List_List*effects=_tmp72;
rgions=({Cyc_List_imp_rev(rgions);});
effects=({Cyc_List_imp_rev(effects);});
if(effects != 0){
({({struct Cyc_List_List*_tmp3C7=effects;struct _fat_ptr _tmp3C6=({const char*_tmp73="";_tag_fat(_tmp73,sizeof(char),1U);});struct _fat_ptr _tmp3C5=({const char*_tmp74="";_tag_fat(_tmp74,sizeof(char),1U);});Cyc_Absyndump_group(Cyc_Absyndump_dumpntyp,_tmp3C7,_tmp3C6,_tmp3C5,({const char*_tmp75="+";_tag_fat(_tmp75,sizeof(char),2U);}));});});
if(rgions != 0)({Cyc_Absyndump_dump_char((int)'+');});}
# 380
if(
# 384
rgions != 0 || effects == 0)
({({struct Cyc_List_List*_tmp3CA=rgions;struct _fat_ptr _tmp3C9=({const char*_tmp76="{";_tag_fat(_tmp76,sizeof(char),2U);});struct _fat_ptr _tmp3C8=({const char*_tmp77="}";_tag_fat(_tmp77,sizeof(char),2U);});Cyc_Absyndump_group(Cyc_Absyndump_dumprgn,_tmp3CA,_tmp3C9,_tmp3C8,({const char*_tmp78=",";_tag_fat(_tmp78,sizeof(char),2U);}));});});}}
# 389
static void Cyc_Absyndump_dumpntyp(void*t){
void*_tmp7A=t;struct Cyc_Absyn_Exp*_tmp7B;struct Cyc_Absyn_Exp*_tmp7C;struct Cyc_List_List*_tmp7E;struct _tuple0*_tmp7D;struct Cyc_Absyn_Datatypedecl*_tmp7F;struct Cyc_Absyn_Enumdecl*_tmp80;struct Cyc_Absyn_Aggrdecl*_tmp81;struct Cyc_List_List*_tmp83;enum Cyc_Absyn_AggrKind _tmp82;struct Cyc_List_List*_tmp84;int _tmp87;void*_tmp86;struct Cyc_Core_Opt*_tmp85;int _tmp89;struct Cyc_Core_Opt*_tmp88;struct Cyc_Absyn_Tvar*_tmp8A;struct Cyc_List_List*_tmp8C;void*_tmp8B;switch(*((int*)_tmp7A)){case 4U: _LL1: _LL2:
# 392
 goto _LL4;case 5U: _LL3: _LL4:
 goto _LL6;case 3U: _LL5: _LL6:
 return;case 0U: if(((struct Cyc_Absyn_VoidCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp7A)->f1)->tag == 0U){_LL7: _LL8:
({Cyc_Absyndump_dump(({const char*_tmp8D="void";_tag_fat(_tmp8D,sizeof(char),5U);}));});return;}else{_LL1F: _tmp8B=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp7A)->f1;_tmp8C=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp7A)->f2;_LL20: {void*c=_tmp8B;struct Cyc_List_List*ts=_tmp8C;
# 412
void*_tmp99=c;struct _fat_ptr _tmp9A;struct Cyc_List_List*_tmp9B;struct _tuple0*_tmp9C;union Cyc_Absyn_AggrInfo _tmp9D;int _tmp9E;union Cyc_Absyn_DatatypeFieldInfo _tmp9F;union Cyc_Absyn_DatatypeInfo _tmpA0;switch(*((int*)_tmp99)){case 20U: _LL22: _tmpA0=((struct Cyc_Absyn_DatatypeCon_Absyn_TyCon_struct*)_tmp99)->f1;_LL23: {union Cyc_Absyn_DatatypeInfo tu_info=_tmpA0;
# 414
{union Cyc_Absyn_DatatypeInfo _tmpA1=tu_info;int _tmpA3;struct _tuple0*_tmpA2;int _tmpA5;struct _tuple0*_tmpA4;if((_tmpA1.UnknownDatatype).tag == 1){_LL69: _tmpA4=((_tmpA1.UnknownDatatype).val).name;_tmpA5=((_tmpA1.UnknownDatatype).val).is_extensible;_LL6A: {struct _tuple0*n=_tmpA4;int is_x=_tmpA5;
_tmpA2=n;_tmpA3=is_x;goto _LL6C;}}else{_LL6B: _tmpA2=(*(_tmpA1.KnownDatatype).val)->name;_tmpA3=(*(_tmpA1.KnownDatatype).val)->is_extensible;_LL6C: {struct _tuple0*n=_tmpA2;int is_x=_tmpA3;
# 417
if(is_x)({Cyc_Absyndump_dump(({const char*_tmpA6="@extensible";_tag_fat(_tmpA6,sizeof(char),12U);}));});({Cyc_Absyndump_dump(({const char*_tmpA7="datatype";_tag_fat(_tmpA7,sizeof(char),9U);}));});
({Cyc_Absyndump_dumpqvar(n);});({Cyc_Absyndump_dumptps(ts);});
goto _LL68;}}_LL68:;}
# 421
return;}case 21U: _LL24: _tmp9F=((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmp99)->f1;_LL25: {union Cyc_Absyn_DatatypeFieldInfo tuf_info=_tmp9F;
# 423
{union Cyc_Absyn_DatatypeFieldInfo _tmpA8=tuf_info;struct _tuple0*_tmpAB;int _tmpAA;struct _tuple0*_tmpA9;int _tmpAE;struct _tuple0*_tmpAD;struct _tuple0*_tmpAC;if((_tmpA8.UnknownDatatypefield).tag == 1){_LL6E: _tmpAC=((_tmpA8.UnknownDatatypefield).val).datatype_name;_tmpAD=((_tmpA8.UnknownDatatypefield).val).field_name;_tmpAE=((_tmpA8.UnknownDatatypefield).val).is_extensible;_LL6F: {struct _tuple0*tname=_tmpAC;struct _tuple0*fname=_tmpAD;int is_x=_tmpAE;
# 425
_tmpA9=tname;_tmpAA=is_x;_tmpAB=fname;goto _LL71;}}else{_LL70: _tmpA9=(((_tmpA8.KnownDatatypefield).val).f1)->name;_tmpAA=(((_tmpA8.KnownDatatypefield).val).f1)->is_extensible;_tmpAB=(((_tmpA8.KnownDatatypefield).val).f2)->name;_LL71: {struct _tuple0*tname=_tmpA9;int is_x=_tmpAA;struct _tuple0*fname=_tmpAB;
# 428
if(is_x)({Cyc_Absyndump_dump(({const char*_tmpAF="@extensible ";_tag_fat(_tmpAF,sizeof(char),13U);}));});({Cyc_Absyndump_dump(({const char*_tmpB0="datatype ";_tag_fat(_tmpB0,sizeof(char),10U);}));});
({Cyc_Absyndump_dumpqvar(tname);});({Cyc_Absyndump_dump(({const char*_tmpB1=".";_tag_fat(_tmpB1,sizeof(char),2U);}));});({Cyc_Absyndump_dumpqvar(fname);});
({Cyc_Absyndump_dumptps(ts);});
goto _LL6D;}}_LL6D:;}
# 433
return;}case 1U: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp99)->f1){case Cyc_Absyn_None: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp99)->f2){case Cyc_Absyn_Int_sz: _LL26: _LL27:
 goto _LL29;case Cyc_Absyn_Long_sz: _LL2A: _LL2B:
# 436
 goto _LL2D;case Cyc_Absyn_Char_sz: _LL2E: _LL2F:
# 438
({Cyc_Absyndump_dump(({const char*_tmpB4="char";_tag_fat(_tmpB4,sizeof(char),5U);}));});return;case Cyc_Absyn_Short_sz: _LL34: _LL35:
# 441
 goto _LL37;case Cyc_Absyn_LongLong_sz: _LL40: _LL41:
# 447
 goto _LL43;default: goto _LL66;}case Cyc_Absyn_Signed: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp99)->f2){case Cyc_Absyn_Int_sz: _LL28: _LL29:
# 435
({Cyc_Absyndump_dump(({const char*_tmpB2="int";_tag_fat(_tmpB2,sizeof(char),4U);}));});return;case Cyc_Absyn_Long_sz: _LL2C: _LL2D:
# 437
({Cyc_Absyndump_dump(({const char*_tmpB3="long";_tag_fat(_tmpB3,sizeof(char),5U);}));});return;case Cyc_Absyn_Char_sz: _LL30: _LL31:
# 439
({Cyc_Absyndump_dump(({const char*_tmpB5="signed char";_tag_fat(_tmpB5,sizeof(char),12U);}));});return;case Cyc_Absyn_Short_sz: _LL36: _LL37:
# 442
({Cyc_Absyndump_dump(({const char*_tmpB7="short";_tag_fat(_tmpB7,sizeof(char),6U);}));});return;case Cyc_Absyn_LongLong_sz: _LL42: _LL43: {
# 449
enum Cyc_Cyclone_C_Compilers _tmpBC=Cyc_Cyclone_c_compiler;if(_tmpBC == Cyc_Cyclone_Gcc_c){_LL73: _LL74:
({Cyc_Absyndump_dump(({const char*_tmpBD="long long";_tag_fat(_tmpBD,sizeof(char),10U);}));});return;}else{_LL75: _LL76:
({Cyc_Absyndump_dump(({const char*_tmpBE="__int64";_tag_fat(_tmpBE,sizeof(char),8U);}));});return;}_LL72:;}default: goto _LL66;}case Cyc_Absyn_Unsigned: switch(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)_tmp99)->f2){case Cyc_Absyn_Char_sz: _LL32: _LL33:
# 440
({Cyc_Absyndump_dump(({const char*_tmpB6="unsigned char";_tag_fat(_tmpB6,sizeof(char),14U);}));});return;case Cyc_Absyn_Short_sz: _LL38: _LL39:
# 443
({Cyc_Absyndump_dump(({const char*_tmpB8="unsigned short";_tag_fat(_tmpB8,sizeof(char),15U);}));});return;case Cyc_Absyn_Int_sz: _LL3A: _LL3B:
({Cyc_Absyndump_dump(({const char*_tmpB9="unsigned";_tag_fat(_tmpB9,sizeof(char),9U);}));});return;case Cyc_Absyn_Long_sz: _LL3C: _LL3D:
({Cyc_Absyndump_dump(({const char*_tmpBA="unsigned long";_tag_fat(_tmpBA,sizeof(char),14U);}));});return;case Cyc_Absyn_LongLong_sz: _LL3E: _LL3F:
({Cyc_Absyndump_dump(({const char*_tmpBB="unsigned";_tag_fat(_tmpBB,sizeof(char),9U);}));});goto _LL41;default: goto _LL66;}default: goto _LL66;}case 2U: _LL44: _tmp9E=((struct Cyc_Absyn_FloatCon_Absyn_TyCon_struct*)_tmp99)->f1;_LL45: {int i=_tmp9E;
# 454
int _tmpBF=i;switch(_tmpBF){case 0U: _LL78: _LL79:
({Cyc_Absyndump_dump(({const char*_tmpC0="float";_tag_fat(_tmpC0,sizeof(char),6U);}));});return;case 1U: _LL7A: _LL7B:
({Cyc_Absyndump_dump(({const char*_tmpC1="double";_tag_fat(_tmpC1,sizeof(char),7U);}));});return;default: _LL7C: _LL7D:
({Cyc_Absyndump_dump(({const char*_tmpC2="long double";_tag_fat(_tmpC2,sizeof(char),12U);}));});return;}_LL77:;}case 22U: _LL46: _tmp9D=((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmp99)->f1;_LL47: {union Cyc_Absyn_AggrInfo info=_tmp9D;
# 460
struct _tuple12 _stmttmpA=({Cyc_Absyn_aggr_kinded_name(info);});struct _tuple0*_tmpC4;enum Cyc_Absyn_AggrKind _tmpC3;_LL7F: _tmpC3=_stmttmpA.f1;_tmpC4=_stmttmpA.f2;_LL80: {enum Cyc_Absyn_AggrKind k=_tmpC3;struct _tuple0*n=_tmpC4;
({Cyc_Absyndump_dumpaggr_kind(k);});({Cyc_Absyndump_dumpqvar(n);});({Cyc_Absyndump_dumptps(ts);});
return;}}case 17U: _LL48: _tmp9C=((struct Cyc_Absyn_EnumCon_Absyn_TyCon_struct*)_tmp99)->f1;_LL49: {struct _tuple0*n=_tmp9C;
({Cyc_Absyndump_dump(({const char*_tmpC5="enum";_tag_fat(_tmpC5,sizeof(char),5U);}));});({Cyc_Absyndump_dumpqvar(n);});return;}case 18U: _LL4A: _tmp9B=((struct Cyc_Absyn_AnonEnumCon_Absyn_TyCon_struct*)_tmp99)->f1;_LL4B: {struct Cyc_List_List*fs=_tmp9B;
({Cyc_Absyndump_dump(({const char*_tmpC6="enum{";_tag_fat(_tmpC6,sizeof(char),6U);}));});({Cyc_Absyndump_dumpenumfields(fs);});({Cyc_Absyndump_dump(({const char*_tmpC7="}";_tag_fat(_tmpC7,sizeof(char),2U);}));});return;}case 19U: _LL4C: _tmp9A=((struct Cyc_Absyn_BuiltinCon_Absyn_TyCon_struct*)_tmp99)->f1;_LL4D: {struct _fat_ptr t=_tmp9A;
({Cyc_Absyndump_dump(t);});return;}case 3U: _LL4E: _LL4F:
({Cyc_Absyndump_dump(({const char*_tmpC8="region_t<";_tag_fat(_tmpC8,sizeof(char),10U);}));});({Cyc_Absyndump_dumprgn((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});({Cyc_Absyndump_dump(({const char*_tmpC9=">";_tag_fat(_tmpC9,sizeof(char),2U);}));});return;case 5U: _LL50: _LL51:
({Cyc_Absyndump_dump(({const char*_tmpCA="tag_t<";_tag_fat(_tmpCA,sizeof(char),7U);}));});({Cyc_Absyndump_dumptyp((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});({Cyc_Absyndump_dump(({const char*_tmpCB=">";_tag_fat(_tmpCB,sizeof(char),2U);}));});return;case 7U: _LL52: _LL53:
({Cyc_Absyndump_dump(({const char*_tmpCC="`U";_tag_fat(_tmpCC,sizeof(char),3U);}));});return;case 8U: _LL54: _LL55:
({Cyc_Absyndump_dump(({const char*_tmpCD="`RC";_tag_fat(_tmpCD,sizeof(char),4U);}));});return;case 6U: _LL56: _LL57:
({Cyc_Absyndump_dump(({const char*_tmpCE="`H";_tag_fat(_tmpCE,sizeof(char),3U);}));});return;case 9U: _LL58: _LL59:
({Cyc_Absyndump_dump(({const char*_tmpCF="{";_tag_fat(_tmpCF,sizeof(char),2U);}));});({Cyc_Absyndump_dumptyp((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});({Cyc_Absyndump_dump(({const char*_tmpD0="}";_tag_fat(_tmpD0,sizeof(char),2U);}));});return;case 12U: _LL5A: _LL5B:
({Cyc_Absyndump_dump(({const char*_tmpD1="regions(";_tag_fat(_tmpD1,sizeof(char),9U);}));});({Cyc_Absyndump_dumptyp((void*)((struct Cyc_List_List*)_check_null(ts))->hd);});({Cyc_Absyndump_dump(({const char*_tmpD2=")";_tag_fat(_tmpD2,sizeof(char),2U);}));});return;case 11U: _LL5C: _LL5D:
({({struct Cyc_List_List*_tmp3CD=ts;struct _fat_ptr _tmp3CC=({const char*_tmpD3="";_tag_fat(_tmpD3,sizeof(char),1U);});struct _fat_ptr _tmp3CB=({const char*_tmpD4="";_tag_fat(_tmpD4,sizeof(char),1U);});Cyc_Absyndump_group(Cyc_Absyndump_dumptyp,_tmp3CD,_tmp3CC,_tmp3CB,({const char*_tmpD5="+";_tag_fat(_tmpD5,sizeof(char),2U);}));});});return;case 13U: _LL5E: _LL5F:
({Cyc_Absyndump_dump(({const char*_tmpD6="@true";_tag_fat(_tmpD6,sizeof(char),6U);}));});return;case 14U: _LL60: _LL61:
({Cyc_Absyndump_dump(({const char*_tmpD7="@false";_tag_fat(_tmpD7,sizeof(char),7U);}));});return;case 15U: _LL62: _LL63:
({Cyc_Absyndump_dump(({const char*_tmpD8="@thin";_tag_fat(_tmpD8,sizeof(char),6U);}));});return;case 16U: _LL64: _LL65:
({Cyc_Absyndump_dump(({const char*_tmpD9="@fat";_tag_fat(_tmpD9,sizeof(char),5U);}));});return;default: _LL66: _LL67:
({void*_tmpDA=0U;({int(*_tmp3CF)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpDC)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpDC;});struct _fat_ptr _tmp3CE=({const char*_tmpDB="bad type constructor";_tag_fat(_tmpDB,sizeof(char),21U);});_tmp3CF(_tmp3CE,_tag_fat(_tmpDA,sizeof(void*),0U));});});}_LL21:;}}case 2U: _LL9: _tmp8A=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp7A)->f1;_LLA: {struct Cyc_Absyn_Tvar*tv=_tmp8A;
# 396
({Cyc_Absyndump_dumptvar(tv);});return;}case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp7A)->f2 == 0){_LLB: _tmp88=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp7A)->f1;_tmp89=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp7A)->f3;_LLC: {struct Cyc_Core_Opt*k=_tmp88;int i=_tmp89;
# 398
({Cyc_Absyndump_dump(({const char*_tmp8E="`E";_tag_fat(_tmp8E,sizeof(char),3U);}));});
if(k == 0)({Cyc_Absyndump_dump(({const char*_tmp8F="K";_tag_fat(_tmp8F,sizeof(char),2U);}));});else{({Cyc_Absyndump_dumpkind((struct Cyc_Absyn_Kind*)k->v);});}
({void(*_tmp90)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp91=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp94=({struct Cyc_Int_pa_PrintArg_struct _tmp3A1;_tmp3A1.tag=1U,_tmp3A1.f1=(unsigned long)i;_tmp3A1;});void*_tmp92[1U];_tmp92[0]=& _tmp94;({struct _fat_ptr _tmp3D0=({const char*_tmp93="%d";_tag_fat(_tmp93,sizeof(char),3U);});Cyc_aprintf(_tmp3D0,_tag_fat(_tmp92,sizeof(void*),1U));});});_tmp90(_tmp91);});return;}}else{_LLD: _tmp85=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp7A)->f1;_tmp86=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp7A)->f2;_tmp87=((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp7A)->f3;_LLE: {struct Cyc_Core_Opt*k=_tmp85;void*t=_tmp86;int i=_tmp87;
({Cyc_Absyndump_dumpntyp((void*)_check_null(t));});return;}}case 6U: _LLF: _tmp84=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmp7A)->f1;_LL10: {struct Cyc_List_List*ts=_tmp84;
({Cyc_Absyndump_dump_char((int)'$');});({Cyc_Absyndump_dumpargs(ts);});return;}case 7U: _LL11: _tmp82=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp7A)->f1;_tmp83=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmp7A)->f2;_LL12: {enum Cyc_Absyn_AggrKind k=_tmp82;struct Cyc_List_List*fs=_tmp83;
# 404
({Cyc_Absyndump_dumpaggr_kind(k);});({Cyc_Absyndump_dump_char((int)'{');});({Cyc_Absyndump_dumpaggrfields(fs);});({Cyc_Absyndump_dump_char((int)'}');});return;}case 10U: switch(*((int*)((struct Cyc_Absyn_TypeDecl*)((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp7A)->f1)->r)){case 0U: _LL13: _tmp81=((struct Cyc_Absyn_Aggr_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp7A)->f1)->r)->f1;_LL14: {struct Cyc_Absyn_Aggrdecl*d=_tmp81;
({Cyc_Absyndump_dump_aggrdecl(d);});return;}case 1U: _LL15: _tmp80=((struct Cyc_Absyn_Enum_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp7A)->f1)->r)->f1;_LL16: {struct Cyc_Absyn_Enumdecl*d=_tmp80;
({Cyc_Absyndump_dump_enumdecl(d);});return;}default: _LL17: _tmp7F=((struct Cyc_Absyn_Datatype_td_Absyn_Raw_typedecl_struct*)(((struct Cyc_Absyn_TypeDeclType_Absyn_Type_struct*)_tmp7A)->f1)->r)->f1;_LL18: {struct Cyc_Absyn_Datatypedecl*d=_tmp7F;
({Cyc_Absyndump_dump_datatypedecl(d);});return;}}case 8U: _LL19: _tmp7D=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp7A)->f1;_tmp7E=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp7A)->f2;_LL1A: {struct _tuple0*n=_tmp7D;struct Cyc_List_List*ts=_tmp7E;
({Cyc_Absyndump_dumpqvar(n);}),({Cyc_Absyndump_dumptps(ts);});return;}case 9U: _LL1B: _tmp7C=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp7A)->f1;_LL1C: {struct Cyc_Absyn_Exp*e=_tmp7C;
({Cyc_Absyndump_dump(({const char*_tmp95="valueof_t(";_tag_fat(_tmp95,sizeof(char),11U);}));});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump(({const char*_tmp96=")";_tag_fat(_tmp96,sizeof(char),2U);}));});return;}default: _LL1D: _tmp7B=((struct Cyc_Absyn_TypeofType_Absyn_Type_struct*)_tmp7A)->f1;_LL1E: {struct Cyc_Absyn_Exp*e=_tmp7B;
({Cyc_Absyndump_dump(({const char*_tmp97="typeof(";_tag_fat(_tmp97,sizeof(char),8U);}));});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump(({const char*_tmp98=")";_tag_fat(_tmp98,sizeof(char),2U);}));});return;}}_LL0:;}
# 483
static void Cyc_Absyndump_dumpvaropt(struct _fat_ptr*vo){
if(vo != 0)({Cyc_Absyndump_dump_str(vo);});}
# 486
static void Cyc_Absyndump_dumpfunarg(struct _tuple9*t){
({({void(*_tmp3D3)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _fat_ptr*),struct _fat_ptr*)=({void(*_tmpDF)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _fat_ptr*),struct _fat_ptr*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _fat_ptr*),struct _fat_ptr*))Cyc_Absyndump_dumptqtd;_tmpDF;});struct Cyc_Absyn_Tqual _tmp3D2=(*t).f2;void*_tmp3D1=(*t).f3;_tmp3D3(_tmp3D2,_tmp3D1,Cyc_Absyndump_dumpvaropt,(*t).f1);});});}struct _tuple18{void*f1;void*f2;};
# 489
static void Cyc_Absyndump_dump_rgncmp(struct _tuple18*cmp){
({Cyc_Absyndump_dumpeff((*cmp).f1);});({Cyc_Absyndump_dump_char((int)'>');});({Cyc_Absyndump_dumprgn((*cmp).f2);});}
# 492
static void Cyc_Absyndump_dump_rgnpo(struct Cyc_List_List*rgn_po){
({({void(*_tmp3D5)(void(*f)(struct _tuple18*),struct Cyc_List_List*l,struct _fat_ptr sep)=({void(*_tmpE2)(void(*f)(struct _tuple18*),struct Cyc_List_List*l,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple18*),struct Cyc_List_List*l,struct _fat_ptr sep))Cyc_Absyndump_dump_sep;_tmpE2;});struct Cyc_List_List*_tmp3D4=rgn_po;_tmp3D5(Cyc_Absyndump_dump_rgncmp,_tmp3D4,({const char*_tmpE3=",";_tag_fat(_tmpE3,sizeof(char),2U);}));});});}
# 492
static void Cyc_Absyndump_dumpdatatypefield(struct Cyc_Absyn_Datatypefield*ef);
# 496
static void Cyc_Absyndump_dumpfunargs(struct Cyc_List_List*args,int c_varargs,struct Cyc_Absyn_VarargInfo*cyc_varargs,void*effopt,struct Cyc_List_List*rgn_po,struct Cyc_Absyn_Exp*req,struct Cyc_Absyn_Exp*ens,struct Cyc_List_List*ieff,struct Cyc_List_List*oeff,struct Cyc_List_List*throws,int reentrant){
# 505
({({void(*_tmp3D9)(void(*f)(struct _tuple9*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmpE5)(void(*f)(struct _tuple9*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple9*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmpE5;});struct Cyc_List_List*_tmp3D8=args;struct _fat_ptr _tmp3D7=({const char*_tmpE6="(";_tag_fat(_tmpE6,sizeof(char),2U);});struct _fat_ptr _tmp3D6=({const char*_tmpE7="";_tag_fat(_tmpE7,sizeof(char),1U);});_tmp3D9(Cyc_Absyndump_dumpfunarg,_tmp3D8,_tmp3D7,_tmp3D6,({const char*_tmpE8=",";_tag_fat(_tmpE8,sizeof(char),2U);}));});});
if(c_varargs || cyc_varargs != 0)
({Cyc_Absyndump_dump(({const char*_tmpE9=",...";_tag_fat(_tmpE9,sizeof(char),5U);}));});
# 506
if(cyc_varargs != 0){
# 509
if(cyc_varargs->inject)({Cyc_Absyndump_dump(({const char*_tmpEA=" inject ";_tag_fat(_tmpEA,sizeof(char),9U);}));});({Cyc_Absyndump_dumpfunarg(({struct _tuple9*_tmpEB=_cycalloc(sizeof(*_tmpEB));_tmpEB->f1=cyc_varargs->name,_tmpEB->f2=cyc_varargs->tq,_tmpEB->f3=cyc_varargs->type;_tmpEB;}));});}
# 506
if(effopt != 0){
# 513
({Cyc_Absyndump_dump_semi();});({Cyc_Absyndump_dumpeff(effopt);});}
# 506
if(rgn_po != 0){
# 516
({Cyc_Absyndump_dump_char((int)':');});({Cyc_Absyndump_dump_rgnpo(rgn_po);});}
# 506
({Cyc_Absyndump_dump_char((int)')');});
# 519
if(req != 0){
({Cyc_Absyndump_dump(({const char*_tmpEC=" @requires(";_tag_fat(_tmpEC,sizeof(char),12U);}));});({Cyc_Absyndump_dumpexp(req);});({Cyc_Absyndump_dump_char((int)')');});}
# 519
if(ens != 0){
# 523
({Cyc_Absyndump_dump(({const char*_tmpED=" @ensures(";_tag_fat(_tmpED,sizeof(char),11U);}));});({Cyc_Absyndump_dumpexp(ens);});({Cyc_Absyndump_dump_char((int)')');});}}
# 557 "absyndump.cyc"
static void Cyc_Absyndump_dumptyp(void*t){
({void(*_tmpEF)(struct Cyc_Absyn_Tqual,void*,void(*f)(int),int)=({void(*_tmpF0)(struct Cyc_Absyn_Tqual,void*,void(*f)(int),int)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(int),int))Cyc_Absyndump_dumptqtd;_tmpF0;});struct Cyc_Absyn_Tqual _tmpF1=({Cyc_Absyn_empty_tqual(0U);});void*_tmpF2=t;void(*_tmpF3)(int x)=({void(*_tmpF4)(int x)=(void(*)(int x))Cyc_Absyndump_ignore;_tmpF4;});int _tmpF5=0;_tmpEF(_tmpF1,_tmpF2,_tmpF3,_tmpF5);});}
# 561
static void Cyc_Absyndump_dumpdesignator(void*d){
void*_tmpF7=d;struct _fat_ptr*_tmpF8;struct Cyc_Absyn_Exp*_tmpF9;if(((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmpF7)->tag == 0U){_LL1: _tmpF9=((struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct*)_tmpF7)->f1;_LL2: {struct Cyc_Absyn_Exp*e=_tmpF9;
({Cyc_Absyndump_dump(({const char*_tmpFA=".[";_tag_fat(_tmpFA,sizeof(char),3U);}));});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump_char((int)']');});goto _LL0;}}else{_LL3: _tmpF8=((struct Cyc_Absyn_FieldName_Absyn_Designator_struct*)_tmpF7)->f1;_LL4: {struct _fat_ptr*v=_tmpF8;
({Cyc_Absyndump_dump_char((int)'.');});({Cyc_Absyndump_dump_nospace(*v);});goto _LL0;}}_LL0:;}struct _tuple19{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 567
static void Cyc_Absyndump_dumpde(struct _tuple19*de){
({({struct Cyc_List_List*_tmp3DC=(*de).f1;struct _fat_ptr _tmp3DB=({const char*_tmpFC="";_tag_fat(_tmpFC,sizeof(char),1U);});struct _fat_ptr _tmp3DA=({const char*_tmpFD="=";_tag_fat(_tmpFD,sizeof(char),2U);});Cyc_Absyndump_egroup(Cyc_Absyndump_dumpdesignator,_tmp3DC,_tmp3DB,_tmp3DA,({const char*_tmpFE="=";_tag_fat(_tmpFE,sizeof(char),2U);}));});});
({Cyc_Absyndump_dumpexp((*de).f2);});}
# 572
static void Cyc_Absyndump_dumpoffset_field(void*f){
void*_tmp100=f;unsigned _tmp101;struct _fat_ptr*_tmp102;if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp100)->tag == 0U){_LL1: _tmp102=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmp100)->f1;_LL2: {struct _fat_ptr*n=_tmp102;
({Cyc_Absyndump_dump_nospace(*n);});return;}}else{_LL3: _tmp101=((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmp100)->f1;_LL4: {unsigned n=_tmp101;
({void(*_tmp103)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp104=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp107=({struct Cyc_Int_pa_PrintArg_struct _tmp3A2;_tmp3A2.tag=1U,_tmp3A2.f1=(unsigned long)((int)n);_tmp3A2;});void*_tmp105[1U];_tmp105[0]=& _tmp107;({struct _fat_ptr _tmp3DD=({const char*_tmp106="%d";_tag_fat(_tmp106,sizeof(char),3U);});Cyc_aprintf(_tmp3DD,_tag_fat(_tmp105,sizeof(void*),1U));});});_tmp103(_tmp104);});return;}}_LL0:;}struct _tuple20{struct _fat_ptr f1;struct Cyc_Absyn_Exp*f2;};
# 579
static void Cyc_Absyndump_dump_asm_iolist(struct Cyc_List_List*iolist){
while((unsigned)iolist){
struct _tuple20*_stmttmpB=(struct _tuple20*)iolist->hd;struct Cyc_Absyn_Exp*_tmp10A;struct _fat_ptr _tmp109;_LL1: _tmp109=_stmttmpB->f1;_tmp10A=_stmttmpB->f2;_LL2: {struct _fat_ptr constr=_tmp109;struct Cyc_Absyn_Exp*e1=_tmp10A;
({Cyc_Absyndump_dump_char((int)'"');});({Cyc_Absyndump_dump_nospace(constr);});({Cyc_Absyndump_dump_char((int)'"');});
({Cyc_Absyndump_dump_char((int)'(');});({Cyc_Absyndump_dumpexp(e1);});({Cyc_Absyndump_dump_char((int)')');});
iolist=iolist->tl;
if((unsigned)iolist)
({Cyc_Absyndump_dump_char((int)',');});}}}
# 590
static void Cyc_Absyndump_dumpexps_prec(int inprec,struct Cyc_List_List*es){
({({void(*_tmp3E2)(void(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp10C)(void(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(int,struct Cyc_Absyn_Exp*),int env,struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group_c;_tmp10C;});int _tmp3E1=inprec;struct Cyc_List_List*_tmp3E0=es;struct _fat_ptr _tmp3DF=({const char*_tmp10D="";_tag_fat(_tmp10D,sizeof(char),1U);});struct _fat_ptr _tmp3DE=({const char*_tmp10E="";_tag_fat(_tmp10E,sizeof(char),1U);});_tmp3E2(Cyc_Absyndump_dumpexp_prec,_tmp3E1,_tmp3E0,_tmp3DF,_tmp3DE,({const char*_tmp10F=",";_tag_fat(_tmp10F,sizeof(char),2U);}));});});}
# 594
static void Cyc_Absyndump_dumpexp_prec(int inprec,struct Cyc_Absyn_Exp*e){
int myprec=({Cyc_Absynpp_exp_prec(e);});
if(inprec >= myprec)
({Cyc_Absyndump_dump_nospace(({const char*_tmp111="(";_tag_fat(_tmp111,sizeof(char),2U);}));});
# 596
{void*_stmttmpC=e->r;void*_tmp112=_stmttmpC;struct Cyc_Absyn_Stmt*_tmp113;struct Cyc_List_List*_tmp115;struct Cyc_Core_Opt*_tmp114;struct Cyc_Absyn_Exp*_tmp117;struct Cyc_Absyn_Exp*_tmp116;int _tmp11C;struct Cyc_Absyn_Exp*_tmp11B;void**_tmp11A;struct Cyc_Absyn_Exp*_tmp119;int _tmp118;struct Cyc_Absyn_Exp*_tmp11E;struct Cyc_Absyn_Exp*_tmp11D;struct Cyc_Absyn_Datatypefield*_tmp120;struct Cyc_List_List*_tmp11F;struct Cyc_Absyn_Enumfield*_tmp121;struct Cyc_Absyn_Enumfield*_tmp122;struct Cyc_List_List*_tmp123;struct Cyc_List_List*_tmp126;struct Cyc_List_List*_tmp125;struct _tuple0*_tmp124;void*_tmp128;struct Cyc_Absyn_Exp*_tmp127;struct Cyc_Absyn_Exp*_tmp12B;struct Cyc_Absyn_Exp*_tmp12A;struct Cyc_Absyn_Vardecl*_tmp129;struct Cyc_List_List*_tmp12C;struct Cyc_List_List*_tmp12E;struct _tuple9*_tmp12D;struct Cyc_List_List*_tmp12F;struct Cyc_Absyn_Exp*_tmp131;struct Cyc_Absyn_Exp*_tmp130;struct _fat_ptr*_tmp133;struct Cyc_Absyn_Exp*_tmp132;struct _fat_ptr*_tmp135;struct Cyc_Absyn_Exp*_tmp134;struct Cyc_List_List*_tmp137;void*_tmp136;struct _fat_ptr*_tmp139;struct Cyc_Absyn_Exp*_tmp138;struct Cyc_List_List*_tmp13E;struct Cyc_List_List*_tmp13D;struct Cyc_List_List*_tmp13C;struct _fat_ptr _tmp13B;int _tmp13A;struct Cyc_Absyn_Exp*_tmp13F;void*_tmp140;struct Cyc_Absyn_Exp*_tmp141;void*_tmp142;struct Cyc_Absyn_Exp*_tmp144;struct Cyc_Absyn_Exp*_tmp143;struct Cyc_Absyn_Exp*_tmp145;struct Cyc_Absyn_Exp*_tmp146;struct Cyc_Absyn_Exp*_tmp148;void*_tmp147;struct Cyc_Absyn_Exp*_tmp149;struct Cyc_Absyn_Exp*_tmp14A;struct Cyc_Absyn_Exp*_tmp14B;struct Cyc_List_List*_tmp14D;struct Cyc_Absyn_Exp*_tmp14C;struct Cyc_Absyn_Exp*_tmp14F;struct Cyc_Absyn_Exp*_tmp14E;struct Cyc_Absyn_Exp*_tmp151;struct Cyc_Absyn_Exp*_tmp150;struct Cyc_Absyn_Exp*_tmp153;struct Cyc_Absyn_Exp*_tmp152;struct Cyc_Absyn_Exp*_tmp156;struct Cyc_Absyn_Exp*_tmp155;struct Cyc_Absyn_Exp*_tmp154;struct Cyc_Absyn_Exp*_tmp157;struct Cyc_Absyn_Exp*_tmp158;struct Cyc_Absyn_Exp*_tmp159;struct Cyc_Absyn_Exp*_tmp15A;struct Cyc_Absyn_Exp*_tmp15D;struct Cyc_Core_Opt*_tmp15C;struct Cyc_Absyn_Exp*_tmp15B;struct Cyc_List_List*_tmp15F;enum Cyc_Absyn_Primop _tmp15E;struct _fat_ptr _tmp160;void*_tmp161;struct _fat_ptr _tmp162;struct _fat_ptr _tmp163;struct _fat_ptr _tmp164;long long _tmp166;enum Cyc_Absyn_Sign _tmp165;int _tmp167;int _tmp168;int _tmp169;short _tmp16B;enum Cyc_Absyn_Sign _tmp16A;struct _fat_ptr _tmp16C;char _tmp16E;enum Cyc_Absyn_Sign _tmp16D;switch(*((int*)_tmp112)){case 0U: switch(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).String_c).tag){case 2U: _LL1: _tmp16D=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Char_c).val).f1;_tmp16E=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Char_c).val).f2;_LL2: {enum Cyc_Absyn_Sign sg=_tmp16D;char ch=_tmp16E;
# 600
({Cyc_Absyndump_dump_char((int)'\'');});({void(*_tmp16F)(struct _fat_ptr s)=Cyc_Absyndump_dump_nospace;struct _fat_ptr _tmp170=({Cyc_Absynpp_char_escape(ch);});_tmp16F(_tmp170);});({Cyc_Absyndump_dump_char((int)'\'');});
goto _LL0;}case 3U: _LL3: _tmp16C=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Wchar_c).val;_LL4: {struct _fat_ptr s=_tmp16C;
({void(*_tmp171)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp172=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp175=({struct Cyc_String_pa_PrintArg_struct _tmp3A3;_tmp3A3.tag=0U,_tmp3A3.f1=(struct _fat_ptr)((struct _fat_ptr)s);_tmp3A3;});void*_tmp173[1U];_tmp173[0]=& _tmp175;({struct _fat_ptr _tmp3E3=({const char*_tmp174="L'%s'";_tag_fat(_tmp174,sizeof(char),6U);});Cyc_aprintf(_tmp3E3,_tag_fat(_tmp173,sizeof(void*),1U));});});_tmp171(_tmp172);});goto _LL0;}case 4U: _LL5: _tmp16A=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Short_c).val).f1;_tmp16B=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Short_c).val).f2;_LL6: {enum Cyc_Absyn_Sign sg=_tmp16A;short s=_tmp16B;
({void(*_tmp176)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp177=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp17A=({struct Cyc_Int_pa_PrintArg_struct _tmp3A4;_tmp3A4.tag=1U,_tmp3A4.f1=(unsigned long)((int)s);_tmp3A4;});void*_tmp178[1U];_tmp178[0]=& _tmp17A;({struct _fat_ptr _tmp3E4=({const char*_tmp179="%d";_tag_fat(_tmp179,sizeof(char),3U);});Cyc_aprintf(_tmp3E4,_tag_fat(_tmp178,sizeof(void*),1U));});});_tmp176(_tmp177);});goto _LL0;}case 5U: switch((((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Int_c).val).f1){case Cyc_Absyn_None: _LL7: _tmp169=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Int_c).val).f2;_LL8: {int i=_tmp169;
_tmp168=i;goto _LLA;}case Cyc_Absyn_Signed: _LL9: _tmp168=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Int_c).val).f2;_LLA: {int i=_tmp168;
({void(*_tmp17B)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp17C=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp17F=({struct Cyc_Int_pa_PrintArg_struct _tmp3A5;_tmp3A5.tag=1U,_tmp3A5.f1=(unsigned long)i;_tmp3A5;});void*_tmp17D[1U];_tmp17D[0]=& _tmp17F;({struct _fat_ptr _tmp3E5=({const char*_tmp17E="%d";_tag_fat(_tmp17E,sizeof(char),3U);});Cyc_aprintf(_tmp3E5,_tag_fat(_tmp17D,sizeof(void*),1U));});});_tmp17B(_tmp17C);});goto _LL0;}default: _LLB: _tmp167=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Int_c).val).f2;_LLC: {int i=_tmp167;
({void(*_tmp180)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp181=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp184=({struct Cyc_Int_pa_PrintArg_struct _tmp3A6;_tmp3A6.tag=1U,_tmp3A6.f1=(unsigned)i;_tmp3A6;});void*_tmp182[1U];_tmp182[0]=& _tmp184;({struct _fat_ptr _tmp3E6=({const char*_tmp183="%uU";_tag_fat(_tmp183,sizeof(char),4U);});Cyc_aprintf(_tmp3E6,_tag_fat(_tmp182,sizeof(void*),1U));});});_tmp180(_tmp181);});goto _LL0;}}case 6U: _LLD: _tmp165=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).LongLong_c).val).f1;_tmp166=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).LongLong_c).val).f2;_LLE: {enum Cyc_Absyn_Sign sg=_tmp165;long long i=_tmp166;
# 609
({void(*_tmp185)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp186=({Cyc_Absynpp_longlong2string((unsigned long long)i);});_tmp185(_tmp186);});goto _LL0;}case 7U: _LLF: _tmp164=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Float_c).val).f1;_LL10: {struct _fat_ptr x=_tmp164;
({Cyc_Absyndump_dump(x);});goto _LL0;}case 1U: _LL11: _LL12:
({Cyc_Absyndump_dump(({const char*_tmp187="NULL";_tag_fat(_tmp187,sizeof(char),5U);}));});goto _LL0;case 8U: _LL13: _tmp163=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).String_c).val;_LL14: {struct _fat_ptr s=_tmp163;
# 613
({Cyc_Absyndump_dump_char((int)'"');});({void(*_tmp188)(struct _fat_ptr s)=Cyc_Absyndump_dump_nospace;struct _fat_ptr _tmp189=({Cyc_Absynpp_string_escape(s);});_tmp188(_tmp189);});({Cyc_Absyndump_dump_char((int)'"');});
goto _LL0;}default: _LL15: _tmp162=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp112)->f1).Wstring_c).val;_LL16: {struct _fat_ptr s=_tmp162;
# 616
({Cyc_Absyndump_dump(({const char*_tmp18A="L\"";_tag_fat(_tmp18A,sizeof(char),3U);}));});({Cyc_Absyndump_dump_nospace(s);});({Cyc_Absyndump_dump_char((int)'"');});goto _LL0;}}case 1U: _LL17: _tmp161=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL18: {void*b=_tmp161;
# 618
({void(*_tmp18B)(struct _tuple0*v)=Cyc_Absyndump_dumpqvar;struct _tuple0*_tmp18C=({Cyc_Absyn_binding2qvar(b);});_tmp18B(_tmp18C);});goto _LL0;}case 2U: _LL19: _tmp160=((struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL1A: {struct _fat_ptr p=_tmp160;
# 621
({Cyc_Absyndump_dump(({const char*_tmp18D="__cyclone_pragma__(";_tag_fat(_tmp18D,sizeof(char),20U);}));});({Cyc_Absyndump_dump_nospace(p);});({Cyc_Absyndump_dump_char((int)')');});goto _LL0;}case 3U: _LL1B: _tmp15E=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp15F=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL1C: {enum Cyc_Absyn_Primop p=_tmp15E;struct Cyc_List_List*es=_tmp15F;
# 624
struct _fat_ptr pstr=({Cyc_Absynpp_prim2str(p);});
{int _stmttmpD=({Cyc_List_length(es);});int _tmp18E=_stmttmpD;switch(_tmp18E){case 1U: _LL72: _LL73:
# 627
 if((int)p == (int)18U){
({Cyc_Absyndump_dump(({const char*_tmp18F="numelts(";_tag_fat(_tmp18F,sizeof(char),9U);}));});
({Cyc_Absyndump_dumpexp((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd);});
({Cyc_Absyndump_dump(({const char*_tmp190=")";_tag_fat(_tmp190,sizeof(char),2U);}));});}else{
# 632
({Cyc_Absyndump_dump(pstr);});
({Cyc_Absyndump_dumpexp_prec(myprec,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd);});}
# 635
goto _LL71;case 2U: _LL74: _LL75:
# 637
({Cyc_Absyndump_dumpexp_prec(myprec,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es))->hd);});
({Cyc_Absyndump_dump(pstr);});
({Cyc_Absyndump_dump_char((int)' ');});
({Cyc_Absyndump_dumpexp_prec(myprec,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd);});
goto _LL71;default: _LL76: _LL77:
({void*_tmp191=0U;({int(*_tmp3E8)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp193)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp193;});struct _fat_ptr _tmp3E7=({const char*_tmp192="Absyndump -- Bad number of arguments to primop";_tag_fat(_tmp192,sizeof(char),47U);});_tmp3E8(_tmp3E7,_tag_fat(_tmp191,sizeof(void*),0U));});});}_LL71:;}
# 644
goto _LL0;}case 4U: _LL1D: _tmp15B=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp15C=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_tmp15D=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp112)->f3;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp15B;struct Cyc_Core_Opt*popt=_tmp15C;struct Cyc_Absyn_Exp*e2=_tmp15D;
# 647
({Cyc_Absyndump_dumpexp_prec(myprec,e1);});
if(popt != 0)
({void(*_tmp194)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp195=({Cyc_Absynpp_prim2str((enum Cyc_Absyn_Primop)popt->v);});_tmp194(_tmp195);});
# 648
({Cyc_Absyndump_dump_nospace(({const char*_tmp196="=";_tag_fat(_tmp196,sizeof(char),2U);}));});
# 651
({Cyc_Absyndump_dumpexp_prec(myprec,e2);});
goto _LL0;}case 5U: switch(((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp112)->f2){case Cyc_Absyn_PreInc: _LL1F: _tmp15A=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL20: {struct Cyc_Absyn_Exp*e2=_tmp15A;
# 654
({Cyc_Absyndump_dump(({const char*_tmp197="++";_tag_fat(_tmp197,sizeof(char),3U);}));});({Cyc_Absyndump_dumpexp_prec(myprec,e2);});goto _LL0;}case Cyc_Absyn_PreDec: _LL21: _tmp159=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL22: {struct Cyc_Absyn_Exp*e2=_tmp159;
({Cyc_Absyndump_dump(({const char*_tmp198="--";_tag_fat(_tmp198,sizeof(char),3U);}));});({Cyc_Absyndump_dumpexp_prec(myprec,e2);});goto _LL0;}case Cyc_Absyn_PostInc: _LL23: _tmp158=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL24: {struct Cyc_Absyn_Exp*e2=_tmp158;
({Cyc_Absyndump_dumpexp_prec(myprec,e2);});({Cyc_Absyndump_dump(({const char*_tmp199="++";_tag_fat(_tmp199,sizeof(char),3U);}));});goto _LL0;}default: _LL25: _tmp157=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL26: {struct Cyc_Absyn_Exp*e2=_tmp157;
({Cyc_Absyndump_dumpexp_prec(myprec,e2);});({Cyc_Absyndump_dump(({const char*_tmp19A="--";_tag_fat(_tmp19A,sizeof(char),3U);}));});goto _LL0;}}case 6U: _LL27: _tmp154=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp155=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_tmp156=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp112)->f3;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp154;struct Cyc_Absyn_Exp*e2=_tmp155;struct Cyc_Absyn_Exp*e3=_tmp156;
# 660
({Cyc_Absyndump_dumploc(e->loc);});
({Cyc_Absyndump_dumpexp_prec(myprec,e1);});
({Cyc_Absyndump_dump_char((int)'?');});({Cyc_Absyndump_dumpexp_prec(0,e2);});
({Cyc_Absyndump_dump_char((int)':');});({Cyc_Absyndump_dumpexp_prec(myprec,e3);});
goto _LL0;}case 7U: _LL29: _tmp152=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp153=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp152;struct Cyc_Absyn_Exp*e2=_tmp153;
# 667
({Cyc_Absyndump_dumploc(e->loc);});
({Cyc_Absyndump_dumpexp_prec(myprec,e1);});({Cyc_Absyndump_dump(({const char*_tmp19B="&&";_tag_fat(_tmp19B,sizeof(char),3U);}));});({Cyc_Absyndump_dumpexp_prec(myprec,e2);});goto _LL0;}case 8U: _LL2B: _tmp150=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp151=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL2C: {struct Cyc_Absyn_Exp*e1=_tmp150;struct Cyc_Absyn_Exp*e2=_tmp151;
# 671
({Cyc_Absyndump_dumploc(e->loc);});
({Cyc_Absyndump_dumpexp_prec(myprec,e1);});({Cyc_Absyndump_dump(({const char*_tmp19C="||";_tag_fat(_tmp19C,sizeof(char),3U);}));});({Cyc_Absyndump_dumpexp_prec(myprec,e2);});goto _LL0;}case 9U: _LL2D: _tmp14E=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp14F=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp14E;struct Cyc_Absyn_Exp*e2=_tmp14F;
# 677
({Cyc_Absyndump_dumploc(e->loc);});
# 680
({Cyc_Absyndump_dumpexp_prec(myprec - 1,e1);});({Cyc_Absyndump_dump_char((int)',');});({Cyc_Absyndump_dumpexp_prec(myprec - 1,e2);});
goto _LL0;}case 10U: _LL2F: _tmp14C=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp14D=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp14C;struct Cyc_List_List*es=_tmp14D;
# 684
({Cyc_Absyndump_dumploc(e->loc);});
({Cyc_Absyndump_dumpexp_prec(myprec,e1);});
({Cyc_Absyndump_dump_nospace(({const char*_tmp19D="(";_tag_fat(_tmp19D,sizeof(char),2U);}));});{
# 689
int old_generate_line_directives=Cyc_Absyndump_generate_line_directives;
Cyc_Absyndump_generate_line_directives=
(old_generate_line_directives && !(e->loc == (unsigned)0))&& !(e1->loc == (unsigned)0);
({Cyc_Absyndump_dumpexps_prec(20,es);});
({Cyc_Absyndump_dump_nospace(({const char*_tmp19E=")";_tag_fat(_tmp19E,sizeof(char),2U);}));});
Cyc_Absyndump_generate_line_directives=old_generate_line_directives;
goto _LL0;}}case 11U: _LL31: _tmp14B=((struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp14B;
# 698
({Cyc_Absyndump_dumploc(e->loc);});({Cyc_Absyndump_dump(({const char*_tmp19F="throw";_tag_fat(_tmp19F,sizeof(char),6U);}));});({Cyc_Absyndump_dumpexp_prec(myprec,e1);});goto _LL0;}case 12U: _LL33: _tmp14A=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp14A;
# 700
_tmp149=e1;goto _LL36;}case 13U: _LL35: _tmp149=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp149;
({Cyc_Absyndump_dumpexp_prec(inprec,e1);});goto _LL0;}case 14U: _LL37: _tmp147=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp148=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL38: {void*t=_tmp147;struct Cyc_Absyn_Exp*e1=_tmp148;
# 704
({Cyc_Absyndump_dump_char((int)'(');});({Cyc_Absyndump_dumptyp(t);});({Cyc_Absyndump_dump_char((int)')');});({Cyc_Absyndump_dumpexp_prec(myprec,e1);});
goto _LL0;}case 15U: _LL39: _tmp146=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL3A: {struct Cyc_Absyn_Exp*e1=_tmp146;
# 707
({Cyc_Absyndump_dump_char((int)'&');});({Cyc_Absyndump_dumpexp_prec(myprec,e1);});goto _LL0;}case 20U: _LL3B: _tmp145=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL3C: {struct Cyc_Absyn_Exp*e1=_tmp145;
({Cyc_Absyndump_dump_char((int)'*');});({Cyc_Absyndump_dumpexp_prec(myprec,e1);});goto _LL0;}case 16U: _LL3D: _tmp143=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp144=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL3E: {struct Cyc_Absyn_Exp*ropt=_tmp143;struct Cyc_Absyn_Exp*e1=_tmp144;
({Cyc_Absyndump_dump(({const char*_tmp1A0="new";_tag_fat(_tmp1A0,sizeof(char),4U);}));});({Cyc_Absyndump_dumpexp_prec(myprec,e1);});goto _LL0;}case 17U: _LL3F: _tmp142=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL40: {void*t=_tmp142;
({Cyc_Absyndump_dump(({const char*_tmp1A1="sizeof(";_tag_fat(_tmp1A1,sizeof(char),8U);}));});({Cyc_Absyndump_dumptyp(t);});({Cyc_Absyndump_dump_char((int)')');});goto _LL0;}case 18U: _LL41: _tmp141=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL42: {struct Cyc_Absyn_Exp*e1=_tmp141;
({Cyc_Absyndump_dump(({const char*_tmp1A2="sizeof(";_tag_fat(_tmp1A2,sizeof(char),8U);}));});({Cyc_Absyndump_dumpexp(e1);});({Cyc_Absyndump_dump_char((int)')');});goto _LL0;}case 40U: _LL43: _tmp140=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL44: {void*t=_tmp140;
({Cyc_Absyndump_dump(({const char*_tmp1A3="valueof(";_tag_fat(_tmp1A3,sizeof(char),9U);}));});({Cyc_Absyndump_dumptyp(t);});({Cyc_Absyndump_dump_char((int)')');});goto _LL0;}case 42U: _LL45: _tmp13F=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL46: {struct Cyc_Absyn_Exp*e1=_tmp13F;
# 715
({Cyc_Absyndump_dump(({const char*_tmp1A4="__extension__(";_tag_fat(_tmp1A4,sizeof(char),15U);}));});({Cyc_Absyndump_dumpexp(e1);});({Cyc_Absyndump_dump_char((int)')');});goto _LL0;}case 41U: _LL47: _tmp13A=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp13B=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_tmp13C=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp112)->f3;_tmp13D=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp112)->f4;_tmp13E=((struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct*)_tmp112)->f5;_LL48: {int vol=_tmp13A;struct _fat_ptr t=_tmp13B;struct Cyc_List_List*o=_tmp13C;struct Cyc_List_List*i=_tmp13D;struct Cyc_List_List*cl=_tmp13E;
# 718
({Cyc_Absyndump_dump(({const char*_tmp1A5="__asm__";_tag_fat(_tmp1A5,sizeof(char),8U);}));});
if(vol)({Cyc_Absyndump_dump(({const char*_tmp1A6=" volatile ";_tag_fat(_tmp1A6,sizeof(char),11U);}));});({Cyc_Absyndump_dump_nospace(({const char*_tmp1A7="(\"";_tag_fat(_tmp1A7,sizeof(char),3U);}));});
({Cyc_Absyndump_dump_nospace(t);});({Cyc_Absyndump_dump_char((int)'"');});
if((unsigned)o){
({Cyc_Absyndump_dump_char((int)':');});
({Cyc_Absyndump_dump_asm_iolist(o);});}
# 721
if((unsigned)i){
# 726
if(!((unsigned)o))
({Cyc_Absyndump_dump_char((int)':');});
# 726
({Cyc_Absyndump_dump_char((int)':');});
# 729
({Cyc_Absyndump_dump_asm_iolist(i);});}
# 721
if((unsigned)cl){
# 732
int ncol=(unsigned)i?2:((unsigned)o?1: 0);
{int cols=0;for(0;cols < 3 - ncol;++ cols){
({Cyc_Absyndump_dump_nospace(({const char*_tmp1A8=" :";_tag_fat(_tmp1A8,sizeof(char),3U);}));});}}
while((unsigned)cl){
({Cyc_Absyndump_dump_char((int)'"');});
({Cyc_Absyndump_dump_nospace(*((struct _fat_ptr*)cl->hd));});
({Cyc_Absyndump_dump_char((int)'"');});
cl=cl->tl;
if((unsigned)cl)
({Cyc_Absyndump_dump_char((int)',');});}}
# 721
({Cyc_Absyndump_dump_char((int)')');});
# 745
goto _LL0;}case 39U: _LL49: _tmp138=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp139=((struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL4A: {struct Cyc_Absyn_Exp*e=_tmp138;struct _fat_ptr*f=_tmp139;
# 748
({Cyc_Absyndump_dump(({const char*_tmp1A9="tagcheck(";_tag_fat(_tmp1A9,sizeof(char),10U);}));});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump_char((int)'.');});({Cyc_Absyndump_dump_nospace(*f);});
({Cyc_Absyndump_dump_char((int)')');});
goto _LL0;}case 19U: _LL4B: _tmp136=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp137=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL4C: {void*t=_tmp136;struct Cyc_List_List*l=_tmp137;
# 753
({Cyc_Absyndump_dump(({const char*_tmp1AA="offsetof(";_tag_fat(_tmp1AA,sizeof(char),10U);}));});({Cyc_Absyndump_dumptyp(t);});({({struct Cyc_List_List*_tmp3EB=l;struct _fat_ptr _tmp3EA=({const char*_tmp1AB=",";_tag_fat(_tmp1AB,sizeof(char),2U);});struct _fat_ptr _tmp3E9=({const char*_tmp1AC=")";_tag_fat(_tmp1AC,sizeof(char),2U);});Cyc_Absyndump_group(Cyc_Absyndump_dumpoffset_field,_tmp3EB,_tmp3EA,_tmp3E9,({const char*_tmp1AD=".";_tag_fat(_tmp1AD,sizeof(char),2U);}));});});goto _LL0;}case 21U: _LL4D: _tmp134=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp135=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL4E: {struct Cyc_Absyn_Exp*e1=_tmp134;struct _fat_ptr*n=_tmp135;
# 756
({Cyc_Absyndump_dumpexp_prec(myprec,e1);});({Cyc_Absyndump_dump_char((int)'.');});({Cyc_Absyndump_dump_nospace(*n);});goto _LL0;}case 22U: _LL4F: _tmp132=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp133=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL50: {struct Cyc_Absyn_Exp*e1=_tmp132;struct _fat_ptr*n=_tmp133;
# 758
({Cyc_Absyndump_dumpexp_prec(myprec,e1);});({Cyc_Absyndump_dump_nospace(({const char*_tmp1AE="->";_tag_fat(_tmp1AE,sizeof(char),3U);}));});({Cyc_Absyndump_dump_nospace(*n);});goto _LL0;}case 23U: _LL51: _tmp130=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp131=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL52: {struct Cyc_Absyn_Exp*e1=_tmp130;struct Cyc_Absyn_Exp*e2=_tmp131;
# 761
({Cyc_Absyndump_dumpexp_prec(myprec,e1);});({Cyc_Absyndump_dump_char((int)'[');});({Cyc_Absyndump_dumpexp(e2);});({Cyc_Absyndump_dump_char((int)']');});goto _LL0;}case 24U: _LL53: _tmp12F=((struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL54: {struct Cyc_List_List*es=_tmp12F;
# 763
({Cyc_Absyndump_dump(({const char*_tmp1AF="$(";_tag_fat(_tmp1AF,sizeof(char),3U);}));});({Cyc_Absyndump_dumpexps_prec(20,es);});({Cyc_Absyndump_dump_char((int)')');});goto _LL0;}case 25U: _LL55: _tmp12D=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp12E=((struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL56: {struct _tuple9*vat=_tmp12D;struct Cyc_List_List*des=_tmp12E;
# 766
({Cyc_Absyndump_dump_char((int)'(');});
({Cyc_Absyndump_dumptyp((*vat).f3);});
({Cyc_Absyndump_dump_char((int)')');});
({({void(*_tmp3EF)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp1B0)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp1B0;});struct Cyc_List_List*_tmp3EE=des;struct _fat_ptr _tmp3ED=({const char*_tmp1B1="{";_tag_fat(_tmp1B1,sizeof(char),2U);});struct _fat_ptr _tmp3EC=({const char*_tmp1B2="}";_tag_fat(_tmp1B2,sizeof(char),2U);});_tmp3EF(Cyc_Absyndump_dumpde,_tmp3EE,_tmp3ED,_tmp3EC,({const char*_tmp1B3=",";_tag_fat(_tmp1B3,sizeof(char),2U);}));});});
goto _LL0;}case 26U: _LL57: _tmp12C=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL58: {struct Cyc_List_List*des=_tmp12C;
# 772
({({void(*_tmp3F3)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp1B4)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp1B4;});struct Cyc_List_List*_tmp3F2=des;struct _fat_ptr _tmp3F1=({const char*_tmp1B5="{";_tag_fat(_tmp1B5,sizeof(char),2U);});struct _fat_ptr _tmp3F0=({const char*_tmp1B6="}";_tag_fat(_tmp1B6,sizeof(char),2U);});_tmp3F3(Cyc_Absyndump_dumpde,_tmp3F2,_tmp3F1,_tmp3F0,({const char*_tmp1B7=",";_tag_fat(_tmp1B7,sizeof(char),2U);}));});});goto _LL0;}case 27U: _LL59: _tmp129=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp12A=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_tmp12B=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmp112)->f3;_LL5A: {struct Cyc_Absyn_Vardecl*vd=_tmp129;struct Cyc_Absyn_Exp*e1=_tmp12A;struct Cyc_Absyn_Exp*e2=_tmp12B;
# 775
({Cyc_Absyndump_dump(({const char*_tmp1B8="{for";_tag_fat(_tmp1B8,sizeof(char),5U);}));});({Cyc_Absyndump_dump_str((*vd->name).f2);});({Cyc_Absyndump_dump_char((int)'<');});({Cyc_Absyndump_dumpexp(e1);});
({Cyc_Absyndump_dump_char((int)':');});({Cyc_Absyndump_dumpexp(e2);});({Cyc_Absyndump_dump_char((int)'}');});
goto _LL0;}case 28U: _LL5B: _tmp127=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp128=(void*)((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL5C: {struct Cyc_Absyn_Exp*e=_tmp127;void*t=_tmp128;
# 780
({Cyc_Absyndump_dump(({const char*_tmp1B9="{for x ";_tag_fat(_tmp1B9,sizeof(char),8U);}));});({Cyc_Absyndump_dump_char((int)'<');});({Cyc_Absyndump_dumpexp(e);});
({Cyc_Absyndump_dump_char((int)':');});({Cyc_Absyndump_dumptyp(t);});({Cyc_Absyndump_dump_char((int)'}');});
goto _LL0;}case 29U: _LL5D: _tmp124=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp125=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_tmp126=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmp112)->f3;_LL5E: {struct _tuple0*n=_tmp124;struct Cyc_List_List*ts=_tmp125;struct Cyc_List_List*des=_tmp126;
# 785
({Cyc_Absyndump_dumpqvar(n);});
({Cyc_Absyndump_dump_char((int)'{');});
if(ts != 0)
({Cyc_Absyndump_dumptps(ts);});
# 787
({({void(*_tmp3F7)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp1BA)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp1BA;});struct Cyc_List_List*_tmp3F6=des;struct _fat_ptr _tmp3F5=({const char*_tmp1BB="";_tag_fat(_tmp1BB,sizeof(char),1U);});struct _fat_ptr _tmp3F4=({const char*_tmp1BC="}";_tag_fat(_tmp1BC,sizeof(char),2U);});_tmp3F7(Cyc_Absyndump_dumpde,_tmp3F6,_tmp3F5,_tmp3F4,({const char*_tmp1BD=",";_tag_fat(_tmp1BD,sizeof(char),2U);}));});});
# 790
goto _LL0;}case 30U: _LL5F: _tmp123=((struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL60: {struct Cyc_List_List*des=_tmp123;
# 792
({({void(*_tmp3FB)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp1BE)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp1BE;});struct Cyc_List_List*_tmp3FA=des;struct _fat_ptr _tmp3F9=({const char*_tmp1BF="{";_tag_fat(_tmp1BF,sizeof(char),2U);});struct _fat_ptr _tmp3F8=({const char*_tmp1C0="}";_tag_fat(_tmp1C0,sizeof(char),2U);});_tmp3FB(Cyc_Absyndump_dumpde,_tmp3FA,_tmp3F9,_tmp3F8,({const char*_tmp1C1=",";_tag_fat(_tmp1C1,sizeof(char),2U);}));});});goto _LL0;}case 32U: _LL61: _tmp122=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL62: {struct Cyc_Absyn_Enumfield*ef=_tmp122;
_tmp121=ef;goto _LL64;}case 33U: _LL63: _tmp121=((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL64: {struct Cyc_Absyn_Enumfield*ef=_tmp121;
({Cyc_Absyndump_dumpqvar(ef->name);});goto _LL0;}case 31U: _LL65: _tmp11F=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp120=((struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct*)_tmp112)->f3;_LL66: {struct Cyc_List_List*es=_tmp11F;struct Cyc_Absyn_Datatypefield*tuf=_tmp120;
# 797
({Cyc_Absyndump_dumpqvar(tuf->name);});
if(es != 0)({({void(*_tmp3FF)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp1C2)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp1C2;});struct Cyc_List_List*_tmp3FE=es;struct _fat_ptr _tmp3FD=({const char*_tmp1C3="(";_tag_fat(_tmp1C3,sizeof(char),2U);});struct _fat_ptr _tmp3FC=({const char*_tmp1C4=")";_tag_fat(_tmp1C4,sizeof(char),2U);});_tmp3FF(Cyc_Absyndump_dumpexp,_tmp3FE,_tmp3FD,_tmp3FC,({const char*_tmp1C5=",";_tag_fat(_tmp1C5,sizeof(char),2U);}));});});goto _LL0;}case 34U: _LL67: _tmp11D=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp11E=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL68: {struct Cyc_Absyn_Exp*e1=_tmp11D;struct Cyc_Absyn_Exp*e2=_tmp11E;
# 802
({Cyc_Absyndump_dumploc(e->loc);});
({Cyc_Absyndump_dump_nospace(({const char*_tmp1C6="spawn (";_tag_fat(_tmp1C6,sizeof(char),8U);}));});
({Cyc_Absyndump_dumpexp_prec(myprec - 1,e1);});({Cyc_Absyndump_dump_nospace(({const char*_tmp1C7=") ";_tag_fat(_tmp1C7,sizeof(char),3U);}));});({Cyc_Absyndump_dumpexp_prec(myprec - 1,e2);});
goto _LL0;}case 35U: _LL69: _tmp118=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp112)->f1).is_calloc;_tmp119=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp112)->f1).rgn;_tmp11A=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp112)->f1).elt_type;_tmp11B=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp112)->f1).num_elts;_tmp11C=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp112)->f1).inline_call;_LL6A: {int is_calloc=_tmp118;struct Cyc_Absyn_Exp*ropt=_tmp119;void**topt=_tmp11A;struct Cyc_Absyn_Exp*e=_tmp11B;int inline_call=_tmp11C;
# 808
({Cyc_Absyndump_dumploc(e->loc);});
if(is_calloc){
if(ropt != 0){
({Cyc_Absyndump_dump(({const char*_tmp1C8="rcalloc(";_tag_fat(_tmp1C8,sizeof(char),9U);}));});({Cyc_Absyndump_dumpexp(ropt);});({Cyc_Absyndump_dump(({const char*_tmp1C9=",";_tag_fat(_tmp1C9,sizeof(char),2U);}));});}else{
# 813
({Cyc_Absyndump_dump(({const char*_tmp1CA="calloc(";_tag_fat(_tmp1CA,sizeof(char),8U);}));});}
# 815
({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump(({const char*_tmp1CB=",";_tag_fat(_tmp1CB,sizeof(char),2U);}));});({void(*_tmp1CC)(struct Cyc_Absyn_Exp*)=Cyc_Absyndump_dumpexp;struct Cyc_Absyn_Exp*_tmp1CD=({Cyc_Absyn_sizeoftype_exp(*((void**)_check_null(topt)),0U);});_tmp1CC(_tmp1CD);});({Cyc_Absyndump_dump(({const char*_tmp1CE=")";_tag_fat(_tmp1CE,sizeof(char),2U);}));});}else{
# 817
if(ropt != 0){
({Cyc_Absyndump_dump(inline_call?({const char*_tmp1CF="rmalloc_inline(";_tag_fat(_tmp1CF,sizeof(char),16U);}):({const char*_tmp1D0="rmalloc(";_tag_fat(_tmp1D0,sizeof(char),9U);}));});
({Cyc_Absyndump_dumpexp(ropt);});({Cyc_Absyndump_dump(({const char*_tmp1D1=",";_tag_fat(_tmp1D1,sizeof(char),2U);}));});}else{
# 821
({Cyc_Absyndump_dump(({const char*_tmp1D2="malloc(";_tag_fat(_tmp1D2,sizeof(char),8U);}));});}
# 824
if(topt != 0)
({void(*_tmp1D3)(struct Cyc_Absyn_Exp*)=Cyc_Absyndump_dumpexp;struct Cyc_Absyn_Exp*_tmp1D4=({struct Cyc_Absyn_Exp*(*_tmp1D5)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_times_exp;struct Cyc_Absyn_Exp*_tmp1D6=({Cyc_Absyn_sizeoftype_exp(*topt,0U);});struct Cyc_Absyn_Exp*_tmp1D7=e;unsigned _tmp1D8=0U;_tmp1D5(_tmp1D6,_tmp1D7,_tmp1D8);});_tmp1D3(_tmp1D4);});else{
# 827
({Cyc_Absyndump_dumpexp(e);});}
({Cyc_Absyndump_dump(({const char*_tmp1D9=")";_tag_fat(_tmp1D9,sizeof(char),2U);}));});}
# 830
goto _LL0;}case 36U: _LL6B: _tmp116=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp117=((struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL6C: {struct Cyc_Absyn_Exp*e1=_tmp116;struct Cyc_Absyn_Exp*e2=_tmp117;
# 833
({Cyc_Absyndump_dumpexp_prec(myprec,e1);});({Cyc_Absyndump_dump_nospace(({const char*_tmp1DA=":=:";_tag_fat(_tmp1DA,sizeof(char),4U);}));});({Cyc_Absyndump_dumpexp_prec(myprec,e2);});goto _LL0;}case 37U: _LL6D: _tmp114=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_tmp115=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp112)->f2;_LL6E: {struct Cyc_Core_Opt*n=_tmp114;struct Cyc_List_List*des=_tmp115;
# 836
({({void(*_tmp403)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp1DB)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple19*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp1DB;});struct Cyc_List_List*_tmp402=des;struct _fat_ptr _tmp401=({const char*_tmp1DC="{";_tag_fat(_tmp1DC,sizeof(char),2U);});struct _fat_ptr _tmp400=({const char*_tmp1DD="}";_tag_fat(_tmp1DD,sizeof(char),2U);});_tmp403(Cyc_Absyndump_dumpde,_tmp402,_tmp401,_tmp400,({const char*_tmp1DE=",";_tag_fat(_tmp1DE,sizeof(char),2U);}));});});goto _LL0;}default: _LL6F: _tmp113=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp112)->f1;_LL70: {struct Cyc_Absyn_Stmt*s=_tmp113;
# 839
({Cyc_Absyndump_dump_nospace(({const char*_tmp1DF="({";_tag_fat(_tmp1DF,sizeof(char),3U);}));});({Cyc_Absyndump_dumpstmt(s,1,0);});({Cyc_Absyndump_dump_nospace(({const char*_tmp1E0="})";_tag_fat(_tmp1E0,sizeof(char),3U);}));});goto _LL0;}}_LL0:;}
# 841
if(inprec >= myprec)
({Cyc_Absyndump_dump_char((int)')');});}
# 845
static void Cyc_Absyndump_dumpexp(struct Cyc_Absyn_Exp*e){
({Cyc_Absyndump_dumpexp_prec(0,e);});}
# 849
static void Cyc_Absyndump_dumpswitchclauses(struct Cyc_List_List*scs,struct Cyc_List_List*varsinblock){
# 851
for(0;scs != 0;scs=scs->tl){
struct Cyc_Absyn_Switch_clause*c=(struct Cyc_Absyn_Switch_clause*)scs->hd;
if(c->where_clause == 0 &&(c->pattern)->r == (void*)& Cyc_Absyn_Wild_p_val)
({Cyc_Absyndump_dump(({const char*_tmp1E3="default:";_tag_fat(_tmp1E3,sizeof(char),9U);}));});else{
# 856
({Cyc_Absyndump_dump(({const char*_tmp1E4="case";_tag_fat(_tmp1E4,sizeof(char),5U);}));});
({Cyc_Absyndump_dumppat(c->pattern);});
if(c->where_clause != 0){
({Cyc_Absyndump_dump(({const char*_tmp1E5="&&";_tag_fat(_tmp1E5,sizeof(char),3U);}));});
({Cyc_Absyndump_dumpexp((struct Cyc_Absyn_Exp*)_check_null(c->where_clause));});}
# 858
({Cyc_Absyndump_dump_nospace(({const char*_tmp1E6=":";_tag_fat(_tmp1E6,sizeof(char),2U);}));});}
# 864
if(({Cyc_Absynpp_is_declaration(c->body);})){
({Cyc_Absyndump_dump(({const char*_tmp1E7=" {";_tag_fat(_tmp1E7,sizeof(char),3U);}));});({Cyc_Absyndump_dumpstmt(c->body,0,0);}),({Cyc_Absyndump_dump_char((int)'}');});}else{
# 867
({Cyc_Absyndump_dumpstmt(c->body,0,varsinblock);});}}}
# 871
static void Cyc_Absyndump_dumpstmt(struct Cyc_Absyn_Stmt*s,int expstmt,struct Cyc_List_List*varsinblock){
({Cyc_Absyndump_dumploc(s->loc);});{
void*_stmttmpE=s->r;void*_tmp1E9=_stmttmpE;struct Cyc_List_List*_tmp1EA;struct Cyc_Absyn_Stmt*_tmp1EC;struct _fat_ptr*_tmp1EB;struct Cyc_Absyn_Stmt*_tmp1EE;struct Cyc_Absyn_Decl*_tmp1ED;struct Cyc_List_List*_tmp1F0;struct Cyc_Absyn_Stmt*_tmp1EF;struct Cyc_List_List*_tmp1F2;struct Cyc_Absyn_Exp*_tmp1F1;struct _fat_ptr*_tmp1F3;struct Cyc_Absyn_Exp*_tmp1F5;struct Cyc_Absyn_Stmt*_tmp1F4;struct Cyc_Absyn_Stmt*_tmp1F9;struct Cyc_Absyn_Exp*_tmp1F8;struct Cyc_Absyn_Exp*_tmp1F7;struct Cyc_Absyn_Exp*_tmp1F6;struct Cyc_Absyn_Stmt*_tmp1FB;struct Cyc_Absyn_Exp*_tmp1FA;struct Cyc_Absyn_Stmt*_tmp1FE;struct Cyc_Absyn_Stmt*_tmp1FD;struct Cyc_Absyn_Exp*_tmp1FC;struct Cyc_Absyn_Exp*_tmp1FF;struct Cyc_Absyn_Stmt*_tmp201;struct Cyc_Absyn_Stmt*_tmp200;struct Cyc_Absyn_Exp*_tmp202;switch(*((int*)_tmp1E9)){case 0U: _LL1: _LL2:
({Cyc_Absyndump_dump_semi();});goto _LL0;case 1U: _LL3: _tmp202=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmp202;
({Cyc_Absyndump_dumpexp_prec(- 100,e);});({Cyc_Absyndump_dump_semi();});goto _LL0;}case 2U: _LL5: _tmp200=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_tmp201=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f2;_LL6: {struct Cyc_Absyn_Stmt*s1=_tmp200;struct Cyc_Absyn_Stmt*s2=_tmp201;
# 880
if(({Cyc_Absynpp_is_declaration(s1);})){
({Cyc_Absyndump_dump_char((int)'{');});({Cyc_Absyndump_dumpstmt(s1,0,0);});({Cyc_Absyndump_dump_char((int)'}');});}else{
# 883
({Cyc_Absyndump_dumpstmt(s1,0,varsinblock);});}
if(({Cyc_Absynpp_is_declaration(s2);})){
if(expstmt)({Cyc_Absyndump_dump_char((int)'(');});({Cyc_Absyndump_dump_char((int)'{');});
({Cyc_Absyndump_dumpstmt(s2,expstmt,0);});({Cyc_Absyndump_dump_char((int)'}');});
if(expstmt){({Cyc_Absyndump_dump_char((int)')');});({Cyc_Absyndump_dump_semi();});}}else{
# 889
({Cyc_Absyndump_dumpstmt(s2,expstmt,varsinblock);});}
goto _LL0;}case 3U: if(((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1 == 0){_LL7: _LL8:
({Cyc_Absyndump_dump(({const char*_tmp203="return;";_tag_fat(_tmp203,sizeof(char),8U);}));});goto _LL0;}else{_LL9: _tmp1FF=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_LLA: {struct Cyc_Absyn_Exp*e=_tmp1FF;
({Cyc_Absyndump_dump(({const char*_tmp204="return";_tag_fat(_tmp204,sizeof(char),7U);}));});({Cyc_Absyndump_dumpexp((struct Cyc_Absyn_Exp*)_check_null(e));});({Cyc_Absyndump_dump_semi();});goto _LL0;}}case 4U: _LLB: _tmp1FC=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_tmp1FD=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f2;_tmp1FE=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f3;_LLC: {struct Cyc_Absyn_Exp*e=_tmp1FC;struct Cyc_Absyn_Stmt*s1=_tmp1FD;struct Cyc_Absyn_Stmt*s2=_tmp1FE;
# 894
({Cyc_Absyndump_dump(({const char*_tmp205="if(";_tag_fat(_tmp205,sizeof(char),4U);}));});({Cyc_Absyndump_dumpexp(e);});
{void*_stmttmpF=s1->r;void*_tmp206=_stmttmpF;switch(*((int*)_tmp206)){case 2U: _LL26: _LL27:
 goto _LL29;case 12U: _LL28: _LL29:
 goto _LL2B;case 4U: _LL2A: _LL2B:
 goto _LL2D;case 13U: _LL2C: _LL2D:
# 900
({Cyc_Absyndump_dump_nospace(({const char*_tmp207="){";_tag_fat(_tmp207,sizeof(char),3U);}));});({Cyc_Absyndump_dumpstmt(s1,0,0);});({Cyc_Absyndump_dump_char((int)'}');});goto _LL25;default: _LL2E: _LL2F:
({Cyc_Absyndump_dump_char((int)')');});({Cyc_Absyndump_dumpstmt(s1,0,varsinblock);});}_LL25:;}
# 903
{void*_stmttmp10=s2->r;void*_tmp208=_stmttmp10;if(((struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct*)_tmp208)->tag == 0U){_LL31: _LL32:
 goto _LL30;}else{_LL33: _LL34:
({Cyc_Absyndump_dump(({const char*_tmp209="else{";_tag_fat(_tmp209,sizeof(char),6U);}));});({Cyc_Absyndump_dumpstmt(s2,0,0);});({Cyc_Absyndump_dump_char((int)'}');});goto _LL30;}_LL30:;}
# 907
goto _LL0;}case 5U: _LLD: _tmp1FA=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1).f1;_tmp1FB=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f2;_LLE: {struct Cyc_Absyn_Exp*e=_tmp1FA;struct Cyc_Absyn_Stmt*s1=_tmp1FB;
# 910
({Cyc_Absyndump_dump(({const char*_tmp20A="while(";_tag_fat(_tmp20A,sizeof(char),7U);}));});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump_nospace(({const char*_tmp20B="){";_tag_fat(_tmp20B,sizeof(char),3U);}));});
({Cyc_Absyndump_dumpstmt(s1,0,0);});({Cyc_Absyndump_dump_char((int)'}');});
goto _LL0;}case 9U: _LLF: _tmp1F6=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_tmp1F7=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f2).f1;_tmp1F8=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f3).f1;_tmp1F9=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f4;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp1F6;struct Cyc_Absyn_Exp*e2=_tmp1F7;struct Cyc_Absyn_Exp*e3=_tmp1F8;struct Cyc_Absyn_Stmt*s1=_tmp1F9;
# 914
({Cyc_Absyndump_dump(({const char*_tmp20C="for(";_tag_fat(_tmp20C,sizeof(char),5U);}));});({Cyc_Absyndump_dumpexp(e1);});({Cyc_Absyndump_dump_semi();});({Cyc_Absyndump_dumpexp(e2);});({Cyc_Absyndump_dump_semi();});({Cyc_Absyndump_dumpexp(e3);});
({Cyc_Absyndump_dump_nospace(({const char*_tmp20D="){";_tag_fat(_tmp20D,sizeof(char),3U);}));});({Cyc_Absyndump_dumpstmt(s1,0,0);});({Cyc_Absyndump_dump_char((int)'}');});
goto _LL0;}case 14U: _LL11: _tmp1F4=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_tmp1F5=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f2).f1;_LL12: {struct Cyc_Absyn_Stmt*s1=_tmp1F4;struct Cyc_Absyn_Exp*e=_tmp1F5;
# 918
({Cyc_Absyndump_dump(({const char*_tmp20E="do{";_tag_fat(_tmp20E,sizeof(char),4U);}));});({Cyc_Absyndump_dumpstmt(s1,0,0);});
({Cyc_Absyndump_dump_nospace(({const char*_tmp20F="}while(";_tag_fat(_tmp20F,sizeof(char),8U);}));});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump_nospace(({const char*_tmp210=");";_tag_fat(_tmp210,sizeof(char),3U);}));});
goto _LL0;}case 6U: _LL13: _LL14:
# 922
({Cyc_Absyndump_dump(({const char*_tmp211="break;";_tag_fat(_tmp211,sizeof(char),7U);}));});goto _LL0;case 7U: _LL15: _LL16:
({Cyc_Absyndump_dump(({const char*_tmp212="continue;";_tag_fat(_tmp212,sizeof(char),10U);}));});goto _LL0;case 8U: _LL17: _tmp1F3=((struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_LL18: {struct _fat_ptr*x=_tmp1F3;
({Cyc_Absyndump_dump(({const char*_tmp213="goto";_tag_fat(_tmp213,sizeof(char),5U);}));});({Cyc_Absyndump_dump_str(x);});({Cyc_Absyndump_dump_semi();});goto _LL0;}case 10U: _LL19: _tmp1F1=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_tmp1F2=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f2;_LL1A: {struct Cyc_Absyn_Exp*e=_tmp1F1;struct Cyc_List_List*ss=_tmp1F2;
# 927
({Cyc_Absyndump_dump(({const char*_tmp214="switch(";_tag_fat(_tmp214,sizeof(char),8U);}));});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump_nospace(({const char*_tmp215="){";_tag_fat(_tmp215,sizeof(char),3U);}));});
({Cyc_Absyndump_dumpswitchclauses(ss,varsinblock);});
({Cyc_Absyndump_dump_char((int)'}');});
goto _LL0;}case 15U: _LL1B: _tmp1EF=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_tmp1F0=((struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f2;_LL1C: {struct Cyc_Absyn_Stmt*s1=_tmp1EF;struct Cyc_List_List*ss=_tmp1F0;
# 932
({Cyc_Absyndump_dump(({const char*_tmp216="try";_tag_fat(_tmp216,sizeof(char),4U);}));});({Cyc_Absyndump_dumpstmt(s1,0,varsinblock);});
({Cyc_Absyndump_dump(({const char*_tmp217="catch{";_tag_fat(_tmp217,sizeof(char),7U);}));});
({Cyc_Absyndump_dumpswitchclauses(ss,varsinblock);});({Cyc_Absyndump_dump_char((int)'}');});
goto _LL0;}case 12U: _LL1D: _tmp1ED=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_tmp1EE=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f2;_LL1E: {struct Cyc_Absyn_Decl*d=_tmp1ED;struct Cyc_Absyn_Stmt*s1=_tmp1EE;
# 938
struct _tuple14 _stmttmp11=({Cyc_Absynpp_shadows(d,varsinblock);});struct Cyc_List_List*_tmp219;int _tmp218;_LL36: _tmp218=_stmttmp11.f1;_tmp219=_stmttmp11.f2;_LL37: {int newblock=_tmp218;struct Cyc_List_List*newvarsinblock=_tmp219;
if(newblock){
if(expstmt)({Cyc_Absyndump_dump(({const char*_tmp21A="({";_tag_fat(_tmp21A,sizeof(char),3U);}));});else{({Cyc_Absyndump_dump(({const char*_tmp21B="{";_tag_fat(_tmp21B,sizeof(char),2U);}));});}
({Cyc_Absyndump_dumpdecl(d);});
({Cyc_Absyndump_dumpstmt(s1,expstmt,0);});
if(expstmt)({Cyc_Absyndump_dump_nospace(({const char*_tmp21C="});";_tag_fat(_tmp21C,sizeof(char),4U);}));});else{({Cyc_Absyndump_dump(({const char*_tmp21D="}";_tag_fat(_tmp21D,sizeof(char),2U);}));});}}else{
# 945
({Cyc_Absyndump_dumpdecl(d);});({Cyc_Absyndump_dumpstmt(s1,expstmt,newvarsinblock);});}
# 947
goto _LL0;}}case 13U: _LL1F: _tmp1EB=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_tmp1EC=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f2;_LL20: {struct _fat_ptr*x=_tmp1EB;struct Cyc_Absyn_Stmt*s1=_tmp1EC;
# 953
if(({Cyc_Absynpp_is_declaration(s1);})){
({Cyc_Absyndump_dump_str(x);});
if(expstmt)({Cyc_Absyndump_dump_nospace(({const char*_tmp21E=": ({";_tag_fat(_tmp21E,sizeof(char),5U);}));});else{({Cyc_Absyndump_dump_nospace(({const char*_tmp21F=": {";_tag_fat(_tmp21F,sizeof(char),4U);}));});}
({Cyc_Absyndump_dumpstmt(s1,expstmt,0);});
if(expstmt)({Cyc_Absyndump_dump_nospace(({const char*_tmp220="});";_tag_fat(_tmp220,sizeof(char),4U);}));});else{({Cyc_Absyndump_dump_char((int)'}');});}}else{
# 959
({Cyc_Absyndump_dump_str(x);});({Cyc_Absyndump_dump_char((int)':');});({Cyc_Absyndump_dumpstmt(s1,expstmt,varsinblock);});}
# 961
goto _LL0;}default: if(((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1 == 0){_LL21: _LL22:
# 963
({Cyc_Absyndump_dump(({const char*_tmp221="fallthru;";_tag_fat(_tmp221,sizeof(char),10U);}));});goto _LL0;}else{_LL23: _tmp1EA=((struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct*)_tmp1E9)->f1;_LL24: {struct Cyc_List_List*es=_tmp1EA;
# 965
({Cyc_Absyndump_dump(({const char*_tmp222="fallthru(";_tag_fat(_tmp222,sizeof(char),10U);}));});({Cyc_Absyndump_dumpexps_prec(20,es);});({Cyc_Absyndump_dump_nospace(({const char*_tmp223=");";_tag_fat(_tmp223,sizeof(char),3U);}));});goto _LL0;}}}_LL0:;}}struct _tuple21{struct Cyc_List_List*f1;struct Cyc_Absyn_Pat*f2;};
# 969
static void Cyc_Absyndump_dumpdp(struct _tuple21*dp){
({({struct Cyc_List_List*_tmp406=(*dp).f1;struct _fat_ptr _tmp405=({const char*_tmp225="";_tag_fat(_tmp225,sizeof(char),1U);});struct _fat_ptr _tmp404=({const char*_tmp226="=";_tag_fat(_tmp226,sizeof(char),2U);});Cyc_Absyndump_egroup(Cyc_Absyndump_dumpdesignator,_tmp406,_tmp405,_tmp404,({const char*_tmp227="=";_tag_fat(_tmp227,sizeof(char),2U);}));});});
({Cyc_Absyndump_dumppat((*dp).f2);});}
# 973
static struct _fat_ptr Cyc_Absyndump_pat_term(int dots){return dots?({const char*_tmp229="...)";_tag_fat(_tmp229,sizeof(char),5U);}):({const char*_tmp22A=")";_tag_fat(_tmp22A,sizeof(char),2U);});}
# 975
static void Cyc_Absyndump_dumppat(struct Cyc_Absyn_Pat*p){
void*_stmttmp12=p->r;void*_tmp22C=_stmttmp12;struct Cyc_Absyn_Exp*_tmp22D;int _tmp230;struct Cyc_List_List*_tmp22F;struct Cyc_Absyn_Datatypefield*_tmp22E;int _tmp233;struct Cyc_List_List*_tmp232;struct Cyc_List_List*_tmp231;int _tmp237;struct Cyc_List_List*_tmp236;struct Cyc_List_List*_tmp235;union Cyc_Absyn_AggrInfo _tmp234;int _tmp23A;struct Cyc_List_List*_tmp239;struct _tuple0*_tmp238;struct _tuple0*_tmp23B;struct Cyc_Absyn_Vardecl*_tmp23D;struct Cyc_Absyn_Tvar*_tmp23C;struct Cyc_Absyn_Pat*_tmp23E;int _tmp240;struct Cyc_List_List*_tmp23F;struct Cyc_Absyn_Vardecl*_tmp242;struct Cyc_Absyn_Tvar*_tmp241;struct Cyc_Absyn_Pat*_tmp244;struct Cyc_Absyn_Vardecl*_tmp243;struct Cyc_Absyn_Vardecl*_tmp245;struct Cyc_Absyn_Pat*_tmp247;struct Cyc_Absyn_Vardecl*_tmp246;struct Cyc_Absyn_Vardecl*_tmp248;char _tmp249;struct Cyc_Absyn_Enumfield*_tmp24A;struct Cyc_Absyn_Enumfield*_tmp24B;struct _fat_ptr _tmp24C;int _tmp24D;int _tmp24E;int _tmp24F;switch(*((int*)_tmp22C)){case 0U: _LL1: _LL2:
({Cyc_Absyndump_dump_char((int)'_');});goto _LL0;case 9U: _LL3: _LL4:
({Cyc_Absyndump_dump(({const char*_tmp250="NULL";_tag_fat(_tmp250,sizeof(char),5U);}));});goto _LL0;case 10U: switch(((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp22C)->f1){case Cyc_Absyn_None: _LL5: _tmp24F=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LL6: {int i=_tmp24F;
_tmp24E=i;goto _LL8;}case Cyc_Absyn_Signed: _LL7: _tmp24E=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LL8: {int i=_tmp24E;
({void(*_tmp251)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp252=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp255=({struct Cyc_Int_pa_PrintArg_struct _tmp3A7;_tmp3A7.tag=1U,_tmp3A7.f1=(unsigned long)i;_tmp3A7;});void*_tmp253[1U];_tmp253[0]=& _tmp255;({struct _fat_ptr _tmp407=({const char*_tmp254="%d";_tag_fat(_tmp254,sizeof(char),3U);});Cyc_aprintf(_tmp407,_tag_fat(_tmp253,sizeof(void*),1U));});});_tmp251(_tmp252);});goto _LL0;}default: _LL9: _tmp24D=((struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LLA: {int i=_tmp24D;
({void(*_tmp256)(struct _fat_ptr s)=Cyc_Absyndump_dump;struct _fat_ptr _tmp257=(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp25A=({struct Cyc_Int_pa_PrintArg_struct _tmp3A8;_tmp3A8.tag=1U,_tmp3A8.f1=(unsigned)i;_tmp3A8;});void*_tmp258[1U];_tmp258[0]=& _tmp25A;({struct _fat_ptr _tmp408=({const char*_tmp259="%u";_tag_fat(_tmp259,sizeof(char),3U);});Cyc_aprintf(_tmp408,_tag_fat(_tmp258,sizeof(void*),1U));});});_tmp256(_tmp257);});goto _LL0;}}case 12U: _LLB: _tmp24C=((struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_LLC: {struct _fat_ptr x=_tmp24C;
({Cyc_Absyndump_dump(x);});goto _LL0;}case 13U: _LLD: _tmp24B=((struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LLE: {struct Cyc_Absyn_Enumfield*ef=_tmp24B;
_tmp24A=ef;goto _LL10;}case 14U: _LLF: _tmp24A=((struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LL10: {struct Cyc_Absyn_Enumfield*ef=_tmp24A;
({Cyc_Absyndump_dumpqvar(ef->name);});goto _LL0;}case 11U: _LL11: _tmp249=((struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_LL12: {char ch=_tmp249;
# 986
({Cyc_Absyndump_dump(({const char*_tmp25B="'";_tag_fat(_tmp25B,sizeof(char),2U);}));});({void(*_tmp25C)(struct _fat_ptr s)=Cyc_Absyndump_dump_nospace;struct _fat_ptr _tmp25D=({Cyc_Absynpp_char_escape(ch);});_tmp25C(_tmp25D);});({Cyc_Absyndump_dump_nospace(({const char*_tmp25E="'";_tag_fat(_tmp25E,sizeof(char),2U);}));});goto _LL0;}case 3U: if(((struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*)((struct Cyc_Absyn_Pat*)((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp22C)->f2)->r)->tag == 0U){_LL13: _tmp248=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_LL14: {struct Cyc_Absyn_Vardecl*vd=_tmp248;
({Cyc_Absyndump_dump(({const char*_tmp25F="*";_tag_fat(_tmp25F,sizeof(char),2U);}));});_tmp245=vd;goto _LL16;}}else{_LL17: _tmp246=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_tmp247=((struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LL18: {struct Cyc_Absyn_Vardecl*vd=_tmp246;struct Cyc_Absyn_Pat*p2=_tmp247;
# 989
({Cyc_Absyndump_dump(({const char*_tmp260="*";_tag_fat(_tmp260,sizeof(char),2U);}));});_tmp243=vd;_tmp244=p2;goto _LL1A;}}case 1U: if(((struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct*)((struct Cyc_Absyn_Pat*)((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp22C)->f2)->r)->tag == 0U){_LL15: _tmp245=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_LL16: {struct Cyc_Absyn_Vardecl*vd=_tmp245;
# 988
({Cyc_Absyndump_dumpqvar(vd->name);});goto _LL0;}}else{_LL19: _tmp243=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_tmp244=((struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LL1A: {struct Cyc_Absyn_Vardecl*vd=_tmp243;struct Cyc_Absyn_Pat*p2=_tmp244;
# 990
({Cyc_Absyndump_dumpqvar(vd->name);});({Cyc_Absyndump_dump(({const char*_tmp261=" as ";_tag_fat(_tmp261,sizeof(char),5U);}));});({Cyc_Absyndump_dumppat(p2);});goto _LL0;}}case 2U: _LL1B: _tmp241=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_tmp242=((struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LL1C: {struct Cyc_Absyn_Tvar*tv=_tmp241;struct Cyc_Absyn_Vardecl*vd=_tmp242;
# 992
({Cyc_Absyndump_dump(({const char*_tmp262="alias";_tag_fat(_tmp262,sizeof(char),6U);}));});({Cyc_Absyndump_dump(({const char*_tmp263="<";_tag_fat(_tmp263,sizeof(char),2U);}));});({Cyc_Absyndump_dumptvar(tv);});({Cyc_Absyndump_dump(({const char*_tmp264=">";_tag_fat(_tmp264,sizeof(char),2U);}));});
({Cyc_Absyndump_dumpvardecl(vd,p->loc,0);});
goto _LL0;}case 5U: _LL1D: _tmp23F=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_tmp240=((struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LL1E: {struct Cyc_List_List*ts=_tmp23F;int dots=_tmp240;
({void(*_tmp265)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp266)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp266;});void(*_tmp267)(struct Cyc_Absyn_Pat*p)=Cyc_Absyndump_dumppat;struct Cyc_List_List*_tmp268=ts;struct _fat_ptr _tmp269=({const char*_tmp26A="$(";_tag_fat(_tmp26A,sizeof(char),3U);});struct _fat_ptr _tmp26B=({Cyc_Absyndump_pat_term(dots);});struct _fat_ptr _tmp26C=({const char*_tmp26D=",";_tag_fat(_tmp26D,sizeof(char),2U);});_tmp265(_tmp267,_tmp268,_tmp269,_tmp26B,_tmp26C);});goto _LL0;}case 6U: _LL1F: _tmp23E=((struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_LL20: {struct Cyc_Absyn_Pat*p2=_tmp23E;
({Cyc_Absyndump_dump(({const char*_tmp26E="&";_tag_fat(_tmp26E,sizeof(char),2U);}));});({Cyc_Absyndump_dumppat(p2);});goto _LL0;}case 4U: _LL21: _tmp23C=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_tmp23D=((struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_LL22: {struct Cyc_Absyn_Tvar*tv=_tmp23C;struct Cyc_Absyn_Vardecl*vd=_tmp23D;
# 998
({Cyc_Absyndump_dumpqvar(vd->name);});({Cyc_Absyndump_dump_char((int)'<');});({Cyc_Absyndump_dumptvar(tv);});({Cyc_Absyndump_dump_char((int)'>');});goto _LL0;}case 15U: _LL23: _tmp23B=((struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_LL24: {struct _tuple0*q=_tmp23B;
({Cyc_Absyndump_dumpqvar(q);});goto _LL0;}case 16U: _LL25: _tmp238=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_tmp239=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_tmp23A=((struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct*)_tmp22C)->f3;_LL26: {struct _tuple0*q=_tmp238;struct Cyc_List_List*ps=_tmp239;int dots=_tmp23A;
# 1001
({Cyc_Absyndump_dumpqvar(q);});({void(*_tmp26F)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp270)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp270;});void(*_tmp271)(struct Cyc_Absyn_Pat*p)=Cyc_Absyndump_dumppat;struct Cyc_List_List*_tmp272=ps;struct _fat_ptr _tmp273=({const char*_tmp274="(";_tag_fat(_tmp274,sizeof(char),2U);});struct _fat_ptr _tmp275=({Cyc_Absyndump_pat_term(dots);});struct _fat_ptr _tmp276=({const char*_tmp277=",";_tag_fat(_tmp277,sizeof(char),2U);});_tmp26F(_tmp271,_tmp272,_tmp273,_tmp275,_tmp276);});goto _LL0;}case 7U: if(((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp22C)->f1 != 0){_LL27: _tmp234=*((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_tmp235=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_tmp236=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp22C)->f3;_tmp237=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp22C)->f4;_LL28: {union Cyc_Absyn_AggrInfo info=_tmp234;struct Cyc_List_List*exists=_tmp235;struct Cyc_List_List*dps=_tmp236;int dots=_tmp237;
# 1003
({void(*_tmp278)(struct _tuple0*v)=Cyc_Absyndump_dumpqvar;struct _tuple0*_tmp279=({Cyc_Absyn_aggr_kinded_name(info);}).f2;_tmp278(_tmp279);});
_tmp231=exists;_tmp232=dps;_tmp233=dots;goto _LL2A;}}else{_LL29: _tmp231=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_tmp232=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp22C)->f3;_tmp233=((struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct*)_tmp22C)->f4;_LL2A: {struct Cyc_List_List*exists=_tmp231;struct Cyc_List_List*dps=_tmp232;int dots=_tmp233;
# 1006
({Cyc_Absyndump_dump_char((int)'{');});
({({void(*_tmp40C)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp27A)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_egroup;_tmp27A;});struct Cyc_List_List*_tmp40B=exists;struct _fat_ptr _tmp40A=({const char*_tmp27B="<";_tag_fat(_tmp27B,sizeof(char),2U);});struct _fat_ptr _tmp409=({const char*_tmp27C=">";_tag_fat(_tmp27C,sizeof(char),2U);});_tmp40C(Cyc_Absyndump_dumptvar,_tmp40B,_tmp40A,_tmp409,({const char*_tmp27D=",";_tag_fat(_tmp27D,sizeof(char),2U);}));});});
({void(*_tmp27E)(void(*f)(struct _tuple21*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp27F)(void(*f)(struct _tuple21*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple21*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp27F;});void(*_tmp280)(struct _tuple21*dp)=Cyc_Absyndump_dumpdp;struct Cyc_List_List*_tmp281=dps;struct _fat_ptr _tmp282=({const char*_tmp283="";_tag_fat(_tmp283,sizeof(char),1U);});struct _fat_ptr _tmp284=({Cyc_Absyndump_pat_term(dots);});struct _fat_ptr _tmp285=({const char*_tmp286=",";_tag_fat(_tmp286,sizeof(char),2U);});_tmp27E(_tmp280,_tmp281,_tmp282,_tmp284,_tmp285);});
goto _LL0;}}case 8U: _LL2B: _tmp22E=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp22C)->f2;_tmp22F=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp22C)->f3;_tmp230=((struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct*)_tmp22C)->f4;_LL2C: {struct Cyc_Absyn_Datatypefield*tuf=_tmp22E;struct Cyc_List_List*ps=_tmp22F;int dots=_tmp230;
# 1011
({Cyc_Absyndump_dumpqvar(tuf->name);});
if(ps != 0)({void(*_tmp287)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp288)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Pat*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp288;});void(*_tmp289)(struct Cyc_Absyn_Pat*p)=Cyc_Absyndump_dumppat;struct Cyc_List_List*_tmp28A=ps;struct _fat_ptr _tmp28B=({const char*_tmp28C="(";_tag_fat(_tmp28C,sizeof(char),2U);});struct _fat_ptr _tmp28D=({Cyc_Absyndump_pat_term(dots);});struct _fat_ptr _tmp28E=({const char*_tmp28F=",";_tag_fat(_tmp28F,sizeof(char),2U);});_tmp287(_tmp289,_tmp28A,_tmp28B,_tmp28D,_tmp28E);});goto _LL0;}default: _LL2D: _tmp22D=((struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct*)_tmp22C)->f1;_LL2E: {struct Cyc_Absyn_Exp*e=_tmp22D;
# 1014
({Cyc_Absyndump_dumpexp(e);});goto _LL0;}}_LL0:;}
# 1018
static void Cyc_Absyndump_dumpdatatypefield(struct Cyc_Absyn_Datatypefield*ef){
({Cyc_Absyndump_dumpqvar(ef->name);});
if(ef->typs != 0)
({Cyc_Absyndump_dumpargs(ef->typs);});}
# 1023
static void Cyc_Absyndump_dumpdatatypefields(struct Cyc_List_List*fields){
({({void(*_tmp40E)(void(*f)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*l,struct _fat_ptr sep)=({void(*_tmp292)(void(*f)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*l,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Datatypefield*),struct Cyc_List_List*l,struct _fat_ptr sep))Cyc_Absyndump_dump_sep;_tmp292;});struct Cyc_List_List*_tmp40D=fields;_tmp40E(Cyc_Absyndump_dumpdatatypefield,_tmp40D,({const char*_tmp293=",";_tag_fat(_tmp293,sizeof(char),2U);}));});});}
# 1026
static void Cyc_Absyndump_dumpenumfield(struct Cyc_Absyn_Enumfield*ef){
({Cyc_Absyndump_dumpqvar(ef->name);});
if(ef->tag != 0){
({Cyc_Absyndump_dump(({const char*_tmp295="=";_tag_fat(_tmp295,sizeof(char),2U);}));});({Cyc_Absyndump_dumpexp((struct Cyc_Absyn_Exp*)_check_null(ef->tag));});}}
# 1032
static void Cyc_Absyndump_dumpenumfields(struct Cyc_List_List*fields){
({({void(*_tmp410)(void(*f)(struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*l,struct _fat_ptr sep)=({void(*_tmp297)(void(*f)(struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*l,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Enumfield*),struct Cyc_List_List*l,struct _fat_ptr sep))Cyc_Absyndump_dump_sep;_tmp297;});struct Cyc_List_List*_tmp40F=fields;_tmp410(Cyc_Absyndump_dumpenumfield,_tmp40F,({const char*_tmp298=",";_tag_fat(_tmp298,sizeof(char),2U);}));});});}
# 1036
static void Cyc_Absyndump_dumpaggrfields(struct Cyc_List_List*fields){
for(0;fields != 0;fields=fields->tl){
struct Cyc_Absyn_Aggrfield*_stmttmp13=(struct Cyc_Absyn_Aggrfield*)fields->hd;struct Cyc_Absyn_Exp*_tmp29F;struct Cyc_List_List*_tmp29E;struct Cyc_Absyn_Exp*_tmp29D;void*_tmp29C;struct Cyc_Absyn_Tqual _tmp29B;struct _fat_ptr*_tmp29A;_LL1: _tmp29A=_stmttmp13->name;_tmp29B=_stmttmp13->tq;_tmp29C=_stmttmp13->type;_tmp29D=_stmttmp13->width;_tmp29E=_stmttmp13->attributes;_tmp29F=_stmttmp13->requires_clause;_LL2: {struct _fat_ptr*name=_tmp29A;struct Cyc_Absyn_Tqual tq=_tmp29B;void*type=_tmp29C;struct Cyc_Absyn_Exp*width=_tmp29D;struct Cyc_List_List*atts=_tmp29E;struct Cyc_Absyn_Exp*req=_tmp29F;
{enum Cyc_Cyclone_C_Compilers _tmp2A0=Cyc_Cyclone_c_compiler;if(_tmp2A0 == Cyc_Cyclone_Gcc_c){_LL4: _LL5:
({({void(*_tmp413)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _fat_ptr*),struct _fat_ptr*)=({void(*_tmp2A1)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _fat_ptr*),struct _fat_ptr*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _fat_ptr*),struct _fat_ptr*))Cyc_Absyndump_dumptqtd;_tmp2A1;});struct Cyc_Absyn_Tqual _tmp412=tq;void*_tmp411=type;_tmp413(_tmp412,_tmp411,Cyc_Absyndump_dump_str,name);});});({Cyc_Absyndump_dumpatts(atts);});goto _LL3;}else{_LL6: _LL7:
({Cyc_Absyndump_dumpatts(atts);});({({void(*_tmp416)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _fat_ptr*),struct _fat_ptr*)=({void(*_tmp2A2)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _fat_ptr*),struct _fat_ptr*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _fat_ptr*),struct _fat_ptr*))Cyc_Absyndump_dumptqtd;_tmp2A2;});struct Cyc_Absyn_Tqual _tmp415=tq;void*_tmp414=type;_tmp416(_tmp415,_tmp414,Cyc_Absyndump_dump_str,name);});});goto _LL3;}_LL3:;}
# 1043
if((unsigned)req){
({Cyc_Absyndump_dump(({const char*_tmp2A3="@requires ";_tag_fat(_tmp2A3,sizeof(char),11U);}));});({Cyc_Absyndump_dumpexp(req);});}
# 1043
if(width != 0){
# 1047
({Cyc_Absyndump_dump_char((int)':');});({Cyc_Absyndump_dumpexp(width);});}
# 1043
({Cyc_Absyndump_dump_semi();});}}}
# 1053
static void Cyc_Absyndump_dumptypedefname(struct Cyc_Absyn_Typedefdecl*td){
({Cyc_Absyndump_dumpqvar(td->name);});
({Cyc_Absyndump_dumptvars(td->tvs);});}
# 1058
static void Cyc_Absyndump_dump_atts_qvar(struct Cyc_Absyn_Fndecl*fd){
({Cyc_Absyndump_dumpatts((fd->i).attributes);});
({Cyc_Absyndump_dumpqvar(fd->name);});}struct _tuple22{void*f1;struct _tuple0*f2;};
# 1062
static void Cyc_Absyndump_dump_callconv_qvar(struct _tuple22*pr){
{void*_stmttmp14=(*pr).f1;void*_tmp2A7=_stmttmp14;switch(*((int*)_tmp2A7)){case 11U: _LL1: _LL2:
 goto _LL0;case 1U: _LL3: _LL4:
({Cyc_Absyndump_dump(({const char*_tmp2A8="_stdcall";_tag_fat(_tmp2A8,sizeof(char),9U);}));});goto _LL0;case 2U: _LL5: _LL6:
({Cyc_Absyndump_dump(({const char*_tmp2A9="_cdecl";_tag_fat(_tmp2A9,sizeof(char),7U);}));});goto _LL0;case 3U: _LL7: _LL8:
({Cyc_Absyndump_dump(({const char*_tmp2AA="_fastcall";_tag_fat(_tmp2AA,sizeof(char),10U);}));});goto _LL0;default: _LL9: _LLA:
 goto _LL0;}_LL0:;}
# 1070
({Cyc_Absyndump_dumpqvar((*pr).f2);});}
# 1072
static void Cyc_Absyndump_dump_callconv_fdqvar(struct Cyc_Absyn_Fndecl*fd){
({Cyc_Absyndump_dump_callconv((fd->i).attributes);});
({Cyc_Absyndump_dumpqvar(fd->name);});}
# 1076
static void Cyc_Absyndump_dumpid(struct Cyc_Absyn_Vardecl*vd){
({Cyc_Absyndump_dumpqvar(vd->name);});}
# 1080
static int Cyc_Absyndump_is_char_ptr(void*t){
# 1082
void*_tmp2AE=t;void*_tmp2AF;void*_tmp2B0;switch(*((int*)_tmp2AE)){case 1U: _LL1: _tmp2B0=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2AE)->f2;if(_tmp2B0 != 0){_LL2: {void*def=_tmp2B0;
return({Cyc_Absyndump_is_char_ptr(def);});}}else{goto _LL5;}case 3U: _LL3: _tmp2AF=(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmp2AE)->f1).elt_type;_LL4: {void*elt_typ=_tmp2AF;
# 1085
while(1){
void*_tmp2B1=elt_typ;void*_tmp2B2;void*_tmp2B3;switch(*((int*)_tmp2B1)){case 1U: _LL8: _tmp2B3=(void*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp2B1)->f2;if(_tmp2B3 != 0){_LL9: {void*t=_tmp2B3;
elt_typ=t;goto _LL7;}}else{goto _LLE;}case 8U: _LLA: _tmp2B2=(void*)((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmp2B1)->f4;if(_tmp2B2 != 0){_LLB: {void*t=_tmp2B2;
elt_typ=t;goto _LL7;}}else{goto _LLE;}case 0U: if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B1)->f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp2B1)->f1)->f2 == Cyc_Absyn_Char_sz){_LLC: _LLD:
 return 1;}else{goto _LLE;}}else{goto _LLE;}default: _LLE: _LLF:
 return 0;}_LL7:;}}default: _LL5: _LL6:
# 1092
 return 0;}_LL0:;}
# 1096
static void Cyc_Absyndump_dumpvardecl(struct Cyc_Absyn_Vardecl*vd,unsigned loc,int do_semi){
struct Cyc_List_List*_tmp2BB;struct Cyc_Absyn_Exp*_tmp2BA;void*_tmp2B9;struct Cyc_Absyn_Tqual _tmp2B8;unsigned _tmp2B7;struct _tuple0*_tmp2B6;enum Cyc_Absyn_Scope _tmp2B5;_LL1: _tmp2B5=vd->sc;_tmp2B6=vd->name;_tmp2B7=vd->varloc;_tmp2B8=vd->tq;_tmp2B9=vd->type;_tmp2BA=vd->initializer;_tmp2BB=vd->attributes;_LL2: {enum Cyc_Absyn_Scope sc=_tmp2B5;struct _tuple0*name=_tmp2B6;unsigned varloc=_tmp2B7;struct Cyc_Absyn_Tqual tq=_tmp2B8;void*type=_tmp2B9;struct Cyc_Absyn_Exp*initializer=_tmp2BA;struct Cyc_List_List*atts=_tmp2BB;
{enum Cyc_Cyclone_C_Compilers _tmp2BC=Cyc_Cyclone_c_compiler;if(_tmp2BC == Cyc_Cyclone_Gcc_c){_LL4: _LL5:
# 1101
 if((int)sc == (int)3U && Cyc_Absyndump_qvar_to_Cids){
void*_stmttmp15=({Cyc_Tcutil_compress(type);});void*_tmp2BD=_stmttmp15;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2BD)->tag == 5U){_LL9: _LLA:
 goto _LL8;}else{_LLB: _LLC:
({Cyc_Absyndump_dumpscope(sc);});}_LL8:;}else{
# 1107
({Cyc_Absyndump_dumpscope(sc);});}
({({void(*_tmp419)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _tuple0*),struct _tuple0*)=({void(*_tmp2BE)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _tuple0*),struct _tuple0*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct _tuple0*),struct _tuple0*))Cyc_Absyndump_dumptqtd;_tmp2BE;});struct Cyc_Absyn_Tqual _tmp418=tq;void*_tmp417=type;_tmp419(_tmp418,_tmp417,Cyc_Absyndump_dumpqvar,name);});});
({Cyc_Absyndump_dumpatts(atts);});
goto _LL3;}else{_LL6: _LL7:
# 1112
({Cyc_Absyndump_dumpatts(atts);});
({Cyc_Absyndump_dumpscope(sc);});{
struct _RegionHandle _tmp2BF=_new_region("temp");struct _RegionHandle*temp=& _tmp2BF;_push_region(temp);
{int is_cp=({Cyc_Absyndump_is_char_ptr(type);});
struct _tuple13 _stmttmp16=({Cyc_Absynpp_to_tms(temp,tq,type);});struct Cyc_List_List*_tmp2C2;void*_tmp2C1;struct Cyc_Absyn_Tqual _tmp2C0;_LLE: _tmp2C0=_stmttmp16.f1;_tmp2C1=_stmttmp16.f2;_tmp2C2=_stmttmp16.f3;_LLF: {struct Cyc_Absyn_Tqual tq=_tmp2C0;void*t=_tmp2C1;struct Cyc_List_List*tms=_tmp2C2;
# 1118
void*call_conv=(void*)& Cyc_Absyn_Unused_att_val;
{struct Cyc_List_List*tms2=tms;for(0;tms2 != 0;tms2=tms2->tl){
void*_stmttmp17=(void*)tms2->hd;void*_tmp2C3=_stmttmp17;struct Cyc_List_List*_tmp2C4;if(((struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_tmp2C3)->tag == 5U){_LL11: _tmp2C4=((struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_tmp2C3)->f2;_LL12: {struct Cyc_List_List*atts=_tmp2C4;
# 1122
for(0;atts != 0;atts=atts->tl){
void*_stmttmp18=(void*)atts->hd;void*_tmp2C5=_stmttmp18;switch(*((int*)_tmp2C5)){case 1U: _LL16: _LL17:
 call_conv=(void*)& Cyc_Absyn_Stdcall_att_val;goto _LL15;case 2U: _LL18: _LL19:
 call_conv=(void*)& Cyc_Absyn_Cdecl_att_val;goto _LL15;case 3U: _LL1A: _LL1B:
 call_conv=(void*)& Cyc_Absyn_Fastcall_att_val;goto _LL15;default: _LL1C: _LL1D:
 goto _LL15;}_LL15:;}
# 1129
goto _LL10;}}else{_LL13: _LL14:
 goto _LL10;}_LL10:;}}
# 1132
({Cyc_Absyndump_dumptq(tq);});
({Cyc_Absyndump_dumpntyp(t);});{
struct _tuple22 pr=({struct _tuple22 _tmp3A9;_tmp3A9.f1=call_conv,_tmp3A9.f2=name;_tmp3A9;});
({void(*_tmp2C6)(int is_char_ptr,struct Cyc_List_List*,void(*f)(struct _tuple22*),struct _tuple22*a)=({void(*_tmp2C7)(int is_char_ptr,struct Cyc_List_List*,void(*f)(struct _tuple22*),struct _tuple22*a)=(void(*)(int is_char_ptr,struct Cyc_List_List*,void(*f)(struct _tuple22*),struct _tuple22*a))Cyc_Absyndump_dumptms;_tmp2C7;});int _tmp2C8=is_cp;struct Cyc_List_List*_tmp2C9=({Cyc_List_imp_rev(tms);});void(*_tmp2CA)(struct _tuple22*pr)=Cyc_Absyndump_dump_callconv_qvar;struct _tuple22*_tmp2CB=& pr;_tmp2C6(_tmp2C8,_tmp2C9,_tmp2CA,_tmp2CB);});
_npop_handler(0U);goto _LL3;}}}
# 1115
;_pop_region();}}_LL3:;}
# 1139
if(initializer != 0){
({Cyc_Absyndump_dump_char((int)'=');});({Cyc_Absyndump_dumpexp(initializer);});}
# 1139
if(do_semi)
# 1142
({Cyc_Absyndump_dump_semi();});}}
# 1145
static void Cyc_Absyndump_dump_aggrdecl(struct Cyc_Absyn_Aggrdecl*ad){
({Cyc_Absyndump_dumpscope(ad->sc);});
if(ad->impl != 0 &&((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->tagged)
({Cyc_Absyndump_dump(({const char*_tmp2CD="@tagged ";_tag_fat(_tmp2CD,sizeof(char),9U);}));});
# 1147
({Cyc_Absyndump_dumpaggr_kind(ad->kind);});
# 1149
({Cyc_Absyndump_dumpqvar(ad->name);});({Cyc_Absyndump_dumpkindedtvars(ad->tvs);});
if(ad->impl != 0){
({Cyc_Absyndump_dump_char((int)'{');});
({({void(*_tmp41D)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp2CE)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Tvar*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_egroup;_tmp2CE;});struct Cyc_List_List*_tmp41C=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->exist_vars;struct _fat_ptr _tmp41B=({const char*_tmp2CF="<";_tag_fat(_tmp2CF,sizeof(char),2U);});struct _fat_ptr _tmp41A=({const char*_tmp2D0=">";_tag_fat(_tmp2D0,sizeof(char),2U);});_tmp41D(Cyc_Absyndump_dumpkindedtvar,_tmp41C,_tmp41B,_tmp41A,({const char*_tmp2D1=",";_tag_fat(_tmp2D1,sizeof(char),2U);}));});});
if(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->rgn_po != 0){
({Cyc_Absyndump_dump_char((int)':');});({Cyc_Absyndump_dump_rgnpo(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->rgn_po);});}
# 1153
({Cyc_Absyndump_dumpaggrfields(((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields);});
# 1157
({Cyc_Absyndump_dump_char((int)'}');});
({Cyc_Absyndump_dumpatts(ad->attributes);});}}
# 1161
static void Cyc_Absyndump_dump_enumdecl(struct Cyc_Absyn_Enumdecl*ed){
struct Cyc_Core_Opt*_tmp2D5;struct _tuple0*_tmp2D4;enum Cyc_Absyn_Scope _tmp2D3;_LL1: _tmp2D3=ed->sc;_tmp2D4=ed->name;_tmp2D5=ed->fields;_LL2: {enum Cyc_Absyn_Scope sc=_tmp2D3;struct _tuple0*nm=_tmp2D4;struct Cyc_Core_Opt*fields=_tmp2D5;
({Cyc_Absyndump_dumpscope(sc);});({Cyc_Absyndump_dump(({const char*_tmp2D6="enum ";_tag_fat(_tmp2D6,sizeof(char),6U);}));});({Cyc_Absyndump_dumpqvar(nm);});
if(fields != 0){
({Cyc_Absyndump_dump_char((int)'{');});({Cyc_Absyndump_dumpenumfields((struct Cyc_List_List*)fields->v);});({Cyc_Absyndump_dump_char((int)'}');});}}}
# 1168
static void Cyc_Absyndump_dump_datatypedecl(struct Cyc_Absyn_Datatypedecl*dd){
int _tmp2DC;struct Cyc_Core_Opt*_tmp2DB;struct Cyc_List_List*_tmp2DA;struct _tuple0*_tmp2D9;enum Cyc_Absyn_Scope _tmp2D8;_LL1: _tmp2D8=dd->sc;_tmp2D9=dd->name;_tmp2DA=dd->tvs;_tmp2DB=dd->fields;_tmp2DC=dd->is_extensible;_LL2: {enum Cyc_Absyn_Scope sc=_tmp2D8;struct _tuple0*name=_tmp2D9;struct Cyc_List_List*tvs=_tmp2DA;struct Cyc_Core_Opt*fields=_tmp2DB;int is_x=_tmp2DC;
({Cyc_Absyndump_dumpscope(sc);});
if(is_x)
({Cyc_Absyndump_dump(({const char*_tmp2DD="@extensible ";_tag_fat(_tmp2DD,sizeof(char),13U);}));});
# 1171
({Cyc_Absyndump_dump(({const char*_tmp2DE="datatype ";_tag_fat(_tmp2DE,sizeof(char),10U);}));});
# 1173
({Cyc_Absyndump_dumpqvar(name);});({Cyc_Absyndump_dumptvars(tvs);});
if(fields != 0){
({Cyc_Absyndump_dump_char((int)'{');});({Cyc_Absyndump_dumpdatatypefields((struct Cyc_List_List*)fields->v);});({Cyc_Absyndump_dump_char((int)'}');});}}}struct _tuple23{unsigned f1;struct _tuple0*f2;int f3;};
# 1179
static void Cyc_Absyndump_dumpexport(struct _tuple23*tup){
({Cyc_Absyndump_dumpqvar((*tup).f2);});}
# 1182
static void Cyc_Absyndump_dump_braced_decls(struct Cyc_List_List*tdl){
({Cyc_Absyndump_dump_char((int)'{');});
for(0;tdl != 0;tdl=tdl->tl){
({Cyc_Absyndump_dumpdecl((struct Cyc_Absyn_Decl*)tdl->hd);});}
({Cyc_Absyndump_dump_char((int)'}');});}
# 1189
static void Cyc_Absyndump_dumpdecl(struct Cyc_Absyn_Decl*d){
({Cyc_Absyndump_dumploc(d->loc);});{
void*_stmttmp19=d->r;void*_tmp2E2=_stmttmp19;struct _tuple11*_tmp2E6;struct Cyc_List_List*_tmp2E5;struct Cyc_List_List*_tmp2E4;struct Cyc_List_List*_tmp2E3;struct Cyc_List_List*_tmp2E7;struct Cyc_List_List*_tmp2E9;struct _tuple0*_tmp2E8;struct Cyc_List_List*_tmp2EB;struct _fat_ptr*_tmp2EA;struct Cyc_Absyn_Typedefdecl*_tmp2EC;struct Cyc_Absyn_Exp*_tmp2EF;struct Cyc_Absyn_Vardecl*_tmp2EE;struct Cyc_Absyn_Tvar*_tmp2ED;struct Cyc_Absyn_Exp*_tmp2F1;struct Cyc_Absyn_Pat*_tmp2F0;struct Cyc_List_List*_tmp2F2;struct Cyc_Absyn_Enumdecl*_tmp2F3;struct Cyc_Absyn_Datatypedecl*_tmp2F4;struct Cyc_Absyn_Aggrdecl*_tmp2F5;struct Cyc_Absyn_Vardecl*_tmp2F6;struct Cyc_Absyn_Fndecl*_tmp2F7;switch(*((int*)_tmp2E2)){case 1U: _LL1: _tmp2F7=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmp2F7;
# 1193
{enum Cyc_Cyclone_C_Compilers _tmp2F8=Cyc_Cyclone_c_compiler;if(_tmp2F8 == Cyc_Cyclone_Vc_c){_LL24: _LL25:
({Cyc_Absyndump_dumpatts((fd->i).attributes);});goto _LL23;}else{_LL26: _LL27:
 goto _LL23;}_LL23:;}
# 1197
if(fd->is_inline){
enum Cyc_Cyclone_C_Compilers _tmp2F9=Cyc_Cyclone_c_compiler;if(_tmp2F9 == Cyc_Cyclone_Vc_c){_LL29: _LL2A:
({Cyc_Absyndump_dump(({const char*_tmp2FA="__inline";_tag_fat(_tmp2FA,sizeof(char),9U);}));});goto _LL28;}else{_LL2B: _LL2C:
({Cyc_Absyndump_dump(({const char*_tmp2FB="inline";_tag_fat(_tmp2FB,sizeof(char),7U);}));});goto _LL28;}_LL28:;}
# 1197
({Cyc_Absyndump_dumpscope(fd->sc);});{
# 1204
struct Cyc_Absyn_FnInfo type_info=fd->i;
type_info.attributes=0;{
void*t=(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp30E=_cycalloc(sizeof(*_tmp30E));_tmp30E->tag=5U,_tmp30E->f1=type_info;_tmp30E;});
if(fd->cached_type != 0){
void*_stmttmp1A=({Cyc_Tcutil_compress((void*)_check_null(fd->cached_type));});void*_tmp2FC=_stmttmp1A;struct Cyc_Absyn_FnInfo _tmp2FD;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2FC)->tag == 5U){_LL2E: _tmp2FD=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp2FC)->f1;_LL2F: {struct Cyc_Absyn_FnInfo i=_tmp2FD;
# 1210
({struct Cyc_List_List*_tmp41E=({Cyc_List_append((fd->i).attributes,i.attributes);});(fd->i).attributes=_tmp41E;});goto _LL2D;}}else{_LL30: _LL31:
({void*_tmp2FE=0U;({int(*_tmp420)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp300)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp300;});struct _fat_ptr _tmp41F=({const char*_tmp2FF="function has non-function type";_tag_fat(_tmp2FF,sizeof(char),31U);});_tmp420(_tmp41F,_tag_fat(_tmp2FE,sizeof(void*),0U));});});}_LL2D:;}
# 1207
{enum Cyc_Cyclone_C_Compilers _tmp301=Cyc_Cyclone_c_compiler;if(_tmp301 == Cyc_Cyclone_Gcc_c){_LL33: _LL34:
# 1215
({void(*_tmp302)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*)=({void(*_tmp303)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*))Cyc_Absyndump_dumptqtd;_tmp303;});struct Cyc_Absyn_Tqual _tmp304=({Cyc_Absyn_empty_tqual(0U);});void*_tmp305=t;void(*_tmp306)(struct Cyc_Absyn_Fndecl*fd)=Cyc_Absyndump_dump_atts_qvar;struct Cyc_Absyn_Fndecl*_tmp307=fd;_tmp302(_tmp304,_tmp305,_tmp306,_tmp307);});goto _LL32;}else{_LL35: _LL36:
({void(*_tmp308)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*)=({void(*_tmp309)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*))Cyc_Absyndump_dumptqtd;_tmp309;});struct Cyc_Absyn_Tqual _tmp30A=({Cyc_Absyn_empty_tqual(0U);});void*_tmp30B=t;void(*_tmp30C)(struct Cyc_Absyn_Fndecl*fd)=Cyc_Absyndump_dump_callconv_fdqvar;struct Cyc_Absyn_Fndecl*_tmp30D=fd;_tmp308(_tmp30A,_tmp30B,_tmp30C,_tmp30D);});goto _LL32;}_LL32:;}
# 1218
({Cyc_Absyndump_dump_char((int)'{');});
({Cyc_Absyndump_dumpstmt(fd->body,0,0);});
({Cyc_Absyndump_dump_char((int)'}');});
goto _LL0;}}}case 0U: _LL3: _tmp2F6=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp2F6;
({Cyc_Absyndump_dumpvardecl(vd,d->loc,1);});goto _LL0;}case 5U: _LL5: _tmp2F5=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_LL6: {struct Cyc_Absyn_Aggrdecl*ad=_tmp2F5;
({Cyc_Absyndump_dump_aggrdecl(ad);});({Cyc_Absyndump_dump_semi();});goto _LL0;}case 6U: _LL7: _tmp2F4=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_LL8: {struct Cyc_Absyn_Datatypedecl*dd=_tmp2F4;
({Cyc_Absyndump_dump_datatypedecl(dd);});({Cyc_Absyndump_dump_semi();});goto _LL0;}case 7U: _LL9: _tmp2F3=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_LLA: {struct Cyc_Absyn_Enumdecl*ed=_tmp2F3;
({Cyc_Absyndump_dump_enumdecl(ed);});({Cyc_Absyndump_dump_semi();});goto _LL0;}case 3U: _LLB: _tmp2F2=((struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_LLC: {struct Cyc_List_List*vds=_tmp2F2;
({({void(*_tmp424)(void(*f)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp30F)(void(*f)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct Cyc_Absyn_Vardecl*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp30F;});struct Cyc_List_List*_tmp423=vds;struct _fat_ptr _tmp422=({const char*_tmp310=" let ";_tag_fat(_tmp310,sizeof(char),6U);});struct _fat_ptr _tmp421=({const char*_tmp311=";";_tag_fat(_tmp311,sizeof(char),2U);});_tmp424(Cyc_Absyndump_dumpid,_tmp423,_tmp422,_tmp421,({const char*_tmp312=",";_tag_fat(_tmp312,sizeof(char),2U);}));});});goto _LL0;}case 2U: _LLD: _tmp2F0=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_tmp2F1=((struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct*)_tmp2E2)->f3;_LLE: {struct Cyc_Absyn_Pat*p=_tmp2F0;struct Cyc_Absyn_Exp*e=_tmp2F1;
# 1228
({Cyc_Absyndump_dump(({const char*_tmp313="let";_tag_fat(_tmp313,sizeof(char),4U);}));});({Cyc_Absyndump_dumppat(p);});({Cyc_Absyndump_dump_char((int)'=');});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump_semi();});goto _LL0;}case 4U: _LLF: _tmp2ED=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_tmp2EE=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp2E2)->f2;_tmp2EF=((struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct*)_tmp2E2)->f3;_LL10: {struct Cyc_Absyn_Tvar*tv=_tmp2ED;struct Cyc_Absyn_Vardecl*vd=_tmp2EE;struct Cyc_Absyn_Exp*open_exp_opt=_tmp2EF;
# 1230
({Cyc_Absyndump_dump(({const char*_tmp314="region";_tag_fat(_tmp314,sizeof(char),7U);}));});({Cyc_Absyndump_dump(({const char*_tmp315="<";_tag_fat(_tmp315,sizeof(char),2U);}));});({Cyc_Absyndump_dumptvar(tv);});({Cyc_Absyndump_dump(({const char*_tmp316="> ";_tag_fat(_tmp316,sizeof(char),3U);}));});({Cyc_Absyndump_dumpqvar(vd->name);});
if((unsigned)open_exp_opt){
({Cyc_Absyndump_dump(({const char*_tmp317=" = open(";_tag_fat(_tmp317,sizeof(char),9U);}));});({Cyc_Absyndump_dumpexp(open_exp_opt);});({Cyc_Absyndump_dump(({const char*_tmp318=")";_tag_fat(_tmp318,sizeof(char),2U);}));});}
# 1231
({Cyc_Absyndump_dump_semi();});
# 1235
goto _LL0;}case 8U: _LL11: _tmp2EC=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_LL12: {struct Cyc_Absyn_Typedefdecl*td=_tmp2EC;
# 1241
({Cyc_Absyndump_dump(({const char*_tmp319="typedef";_tag_fat(_tmp319,sizeof(char),8U);}));});{
void*t=td->defn == 0?({Cyc_Absyn_new_evar(td->kind,0);}):(void*)_check_null(td->defn);
({({void(*_tmp427)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Absyn_Typedefdecl*),struct Cyc_Absyn_Typedefdecl*)=({void(*_tmp31A)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Absyn_Typedefdecl*),struct Cyc_Absyn_Typedefdecl*)=(void(*)(struct Cyc_Absyn_Tqual,void*,void(*f)(struct Cyc_Absyn_Typedefdecl*),struct Cyc_Absyn_Typedefdecl*))Cyc_Absyndump_dumptqtd;_tmp31A;});struct Cyc_Absyn_Tqual _tmp426=td->tq;void*_tmp425=t;_tmp427(_tmp426,_tmp425,Cyc_Absyndump_dumptypedefname,td);});});
({Cyc_Absyndump_dumpatts(td->atts);});
({Cyc_Absyndump_dump_semi();});
# 1247
goto _LL0;}}case 9U: _LL13: _tmp2EA=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_tmp2EB=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp2E2)->f2;_LL14: {struct _fat_ptr*v=_tmp2EA;struct Cyc_List_List*tdl=_tmp2EB;
# 1249
({Cyc_Absyndump_dump(({const char*_tmp31B="namespace";_tag_fat(_tmp31B,sizeof(char),10U);}));});({Cyc_Absyndump_dump_str(v);});({Cyc_Absyndump_dump_braced_decls(tdl);});goto _LL0;}case 10U: _LL15: _tmp2E8=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_tmp2E9=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp2E2)->f2;_LL16: {struct _tuple0*q=_tmp2E8;struct Cyc_List_List*tdl=_tmp2E9;
# 1251
({Cyc_Absyndump_dump(({const char*_tmp31C="using";_tag_fat(_tmp31C,sizeof(char),6U);}));});({Cyc_Absyndump_dumpqvar(q);});({Cyc_Absyndump_dump_braced_decls(tdl);});goto _LL0;}case 11U: _LL17: _tmp2E7=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_LL18: {struct Cyc_List_List*tdl=_tmp2E7;
# 1253
({Cyc_Absyndump_dump(({const char*_tmp31D="extern \"C\" ";_tag_fat(_tmp31D,sizeof(char),12U);}));});({Cyc_Absyndump_dump_braced_decls(tdl);});goto _LL0;}case 12U: _LL19: _tmp2E3=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp2E2)->f1;_tmp2E4=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp2E2)->f2;_tmp2E5=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp2E2)->f3;_tmp2E6=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp2E2)->f4;_LL1A: {struct Cyc_List_List*tdl=_tmp2E3;struct Cyc_List_List*ovrs=_tmp2E4;struct Cyc_List_List*exs=_tmp2E5;struct _tuple11*wc=_tmp2E6;
# 1255
({Cyc_Absyndump_dump(({const char*_tmp31E="extern \"C include\" ";_tag_fat(_tmp31E,sizeof(char),20U);}));});({Cyc_Absyndump_dump_braced_decls(tdl);});
if(ovrs != 0){
({Cyc_Absyndump_dump(({const char*_tmp31F=" cyclone_override ";_tag_fat(_tmp31F,sizeof(char),19U);}));});
({Cyc_Absyndump_dump_braced_decls(ovrs);});}
# 1256
if((unsigned)wc){
# 1261
({Cyc_Absyndump_dump(({const char*_tmp320=" export { * }\n";_tag_fat(_tmp320,sizeof(char),15U);}));});{
struct Cyc_List_List*_tmp321;_LL38: _tmp321=wc->f2;_LL39: {struct Cyc_List_List*hides=_tmp321;
if((unsigned)hides)
({({void(*_tmp42B)(void(*f)(struct _tuple0*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp322)(void(*f)(struct _tuple0*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple0*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_egroup;_tmp322;});struct Cyc_List_List*_tmp42A=hides;struct _fat_ptr _tmp429=({const char*_tmp323=" hide {";_tag_fat(_tmp323,sizeof(char),8U);});struct _fat_ptr _tmp428=({const char*_tmp324="}";_tag_fat(_tmp324,sizeof(char),2U);});_tmp42B(Cyc_Absyndump_dumpqvar,_tmp42A,_tmp429,_tmp428,({const char*_tmp325=",";_tag_fat(_tmp325,sizeof(char),2U);}));});});}}}else{
# 1268
({({void(*_tmp42F)(void(*f)(struct _tuple23*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp326)(void(*f)(struct _tuple23*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _tuple23*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_egroup;_tmp326;});struct Cyc_List_List*_tmp42E=exs;struct _fat_ptr _tmp42D=({const char*_tmp327=" export {";_tag_fat(_tmp327,sizeof(char),10U);});struct _fat_ptr _tmp42C=({const char*_tmp328="}";_tag_fat(_tmp328,sizeof(char),2U);});_tmp42F(Cyc_Absyndump_dumpexport,_tmp42E,_tmp42D,_tmp42C,({const char*_tmp329=",";_tag_fat(_tmp329,sizeof(char),2U);}));});});}
# 1270
goto _LL0;}case 13U: _LL1B: _LL1C:
({Cyc_Absyndump_dump(({const char*_tmp32A=" __cyclone_port_on__; ";_tag_fat(_tmp32A,sizeof(char),23U);}));});goto _LL0;case 14U: _LL1D: _LL1E:
({Cyc_Absyndump_dump(({const char*_tmp32B=" __cyclone_port_off__; ";_tag_fat(_tmp32B,sizeof(char),24U);}));});goto _LL0;case 15U: _LL1F: _LL20:
({Cyc_Absyndump_dump(({const char*_tmp32C=" __tempest_on__; ";_tag_fat(_tmp32C,sizeof(char),18U);}));});goto _LL0;default: _LL21: _LL22:
({Cyc_Absyndump_dump(({const char*_tmp32D=" __tempest_off__; ";_tag_fat(_tmp32D,sizeof(char),19U);}));});goto _LL0;}_LL0:;}}
# 1278
static void Cyc_Absyndump_dump_upperbound(struct Cyc_Absyn_Exp*e){
struct _tuple15 pr=({Cyc_Evexp_eval_const_uint_exp(e);});
if(pr.f1 != (unsigned)1 || !pr.f2){
({Cyc_Absyndump_dump_char((int)'{');});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump_char((int)'}');});}}
# 1285
static void Cyc_Absyndump_dumptms(int is_char_ptr,struct Cyc_List_List*tms,void(*f)(void*),void*a){
# 1287
if(tms == 0){
({f(a);});
return;}{
# 1287
void*_stmttmp1B=(void*)tms->hd;void*_tmp330=_stmttmp1B;struct Cyc_Absyn_Tqual _tmp335;void*_tmp334;void*_tmp333;void*_tmp332;void*_tmp331;if(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp330)->tag == 2U){_LL1: _tmp331=(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp330)->f1).rgn;_tmp332=(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp330)->f1).nullable;_tmp333=(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp330)->f1).bounds;_tmp334=(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp330)->f1).zero_term;_tmp335=((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp330)->f2;_LL2: {void*rgn=_tmp331;void*nullable=_tmp332;void*bd=_tmp333;void*zt=_tmp334;struct Cyc_Absyn_Tqual tq2=_tmp335;
# 1295
{void*_stmttmp1C=({Cyc_Tcutil_compress(bd);});void*_tmp336=_stmttmp1C;struct Cyc_Absyn_Exp*_tmp337;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp336)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp336)->f1)){case 16U: _LL6: _LL7:
({Cyc_Absyndump_dump_char((int)'?');});goto _LL5;case 15U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp336)->f2 != 0){if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)((struct Cyc_List_List*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp336)->f2)->hd)->tag == 9U){_LL8: _tmp337=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp336)->f2)->hd)->f1;_LL9: {struct Cyc_Absyn_Exp*e=_tmp337;
# 1298
({void(*_tmp338)(int c)=Cyc_Absyndump_dump_char;int _tmp339=(int)(({Cyc_Absyn_type2bool(1,nullable);})?'*':'@');_tmp338(_tmp339);});({Cyc_Absyndump_dump_upperbound(e);});goto _LL5;}}else{goto _LLA;}}else{goto _LLA;}default: goto _LLA;}else{_LLA: _LLB:
# 1300
({void(*_tmp33A)(int c)=Cyc_Absyndump_dump_char;int _tmp33B=(int)(({Cyc_Absyn_type2bool(1,nullable);})?'*':'@');_tmp33A(_tmp33B);});}_LL5:;}
# 1302
if((!Cyc_Absyndump_qvar_to_Cids && !is_char_ptr)&&({Cyc_Absyn_type2bool(0,zt);}))({Cyc_Absyndump_dump(({const char*_tmp33C="@zeroterm";_tag_fat(_tmp33C,sizeof(char),10U);}));});if(
(!Cyc_Absyndump_qvar_to_Cids && is_char_ptr)&& !({Cyc_Absyn_type2bool(0,zt);}))({Cyc_Absyndump_dump(({const char*_tmp33D="@nozeroterm";_tag_fat(_tmp33D,sizeof(char),12U);}));});
# 1302
{void*_stmttmp1D=({Cyc_Tcutil_compress(rgn);});void*_tmp33E=_stmttmp1D;struct Cyc_Absyn_Tvar*_tmp33F;switch(*((int*)_tmp33E)){case 0U: switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp33E)->f1)){case 6U: _LLD: _LLE:
# 1305
 if(!Cyc_Absyndump_qvar_to_Cids)({Cyc_Absyndump_dump(({const char*_tmp340="`H";_tag_fat(_tmp340,sizeof(char),3U);}));});goto _LLC;case 7U: _LLF: _LL10:
({Cyc_Absyndump_dump(({const char*_tmp341="`U";_tag_fat(_tmp341,sizeof(char),3U);}));});goto _LLC;case 8U: _LL11: _LL12:
({Cyc_Absyndump_dump(({const char*_tmp342="`RC";_tag_fat(_tmp342,sizeof(char),4U);}));});goto _LLC;default: goto _LL19;}case 2U: _LL13: _tmp33F=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp33E)->f1;_LL14: {struct Cyc_Absyn_Tvar*tv=_tmp33F;
({Cyc_Absyndump_dumptvar(tv);});goto _LLC;}case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmp33E)->f2 == 0){_LL15: _LL16:
({void(*_tmp343)(void*t)=Cyc_Absyndump_dumpntyp;void*_tmp344=({Cyc_Tcutil_compress(rgn);});_tmp343(_tmp344);});goto _LLC;}else{goto _LL19;}case 8U: _LL17: _LL18:
({Cyc_Absyndump_dump(({const char*_tmp345="@region(";_tag_fat(_tmp345,sizeof(char),9U);}));});({Cyc_Absyndump_dumptyp(rgn);});({Cyc_Absyndump_dump(({const char*_tmp346=")";_tag_fat(_tmp346,sizeof(char),2U);}));});goto _LLC;default: _LL19: _LL1A:
({void*_tmp347=0U;({struct _fat_ptr _tmp430=({const char*_tmp348="dumptms: bad rgn type in Pointer_mod";_tag_fat(_tmp348,sizeof(char),37U);});Cyc_Warn_impos(_tmp430,_tag_fat(_tmp347,sizeof(void*),0U));});});}_LLC:;}
# 1313
({Cyc_Absyndump_dumptq(tq2);});
({Cyc_Absyndump_dumptms(0,tms->tl,f,a);});
return;}}else{_LL3: _LL4: {
# 1318
int next_is_pointer=0;
if(tms->tl != 0){
void*_stmttmp1E=(void*)((struct Cyc_List_List*)_check_null(tms->tl))->hd;void*_tmp349=_stmttmp1E;if(((struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct*)_tmp349)->tag == 2U){_LL1C: _LL1D:
 next_is_pointer=1;goto _LL1B;}else{_LL1E: _LL1F:
 goto _LL1B;}_LL1B:;}
# 1319
if(next_is_pointer)
# 1325
({Cyc_Absyndump_dump_char((int)'(');});
# 1319
({Cyc_Absyndump_dumptms(0,tms->tl,f,a);});
# 1327
if(next_is_pointer)
({Cyc_Absyndump_dump_char((int)')');});
# 1327
{void*_stmttmp1F=(void*)tms->hd;void*_tmp34A=_stmttmp1F;struct Cyc_List_List*_tmp34B;int _tmp34E;unsigned _tmp34D;struct Cyc_List_List*_tmp34C;unsigned _tmp350;struct Cyc_List_List*_tmp34F;int _tmp35B;struct Cyc_List_List*_tmp35A;struct Cyc_List_List*_tmp359;struct Cyc_List_List*_tmp358;struct Cyc_Absyn_Exp*_tmp357;struct Cyc_Absyn_Exp*_tmp356;struct Cyc_List_List*_tmp355;void*_tmp354;struct Cyc_Absyn_VarargInfo*_tmp353;int _tmp352;struct Cyc_List_List*_tmp351;void*_tmp35D;struct Cyc_Absyn_Exp*_tmp35C;void*_tmp35E;switch(*((int*)_tmp34A)){case 0U: _LL21: _tmp35E=(void*)((struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1;_LL22: {void*zeroterm=_tmp35E;
# 1331
({Cyc_Absyndump_dump(({const char*_tmp35F="[]";_tag_fat(_tmp35F,sizeof(char),3U);}));});
if(({Cyc_Absyn_type2bool(0,zeroterm);}))({Cyc_Absyndump_dump(({const char*_tmp360="@zeroterm";_tag_fat(_tmp360,sizeof(char),10U);}));});goto _LL20;}case 1U: _LL23: _tmp35C=((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1;_tmp35D=(void*)((struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct*)_tmp34A)->f2;_LL24: {struct Cyc_Absyn_Exp*e=_tmp35C;void*zeroterm=_tmp35D;
# 1335
({Cyc_Absyndump_dump_char((int)'[');});({Cyc_Absyndump_dumpexp(e);});({Cyc_Absyndump_dump_char((int)']');});
if(({Cyc_Absyn_type2bool(0,zeroterm);}))({Cyc_Absyndump_dump(({const char*_tmp361="@zeroterm";_tag_fat(_tmp361,sizeof(char),10U);}));});goto _LL20;}case 3U: if(((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->tag == 1U){_LL25: _tmp351=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f1;_tmp352=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f2;_tmp353=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f3;_tmp354=(void*)((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f4;_tmp355=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f5;_tmp356=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f6;_tmp357=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f7;_tmp358=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f8;_tmp359=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f9;_tmp35A=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f10;_tmp35B=((struct Cyc_Absyn_WithTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f11;_LL26: {struct Cyc_List_List*args=_tmp351;int c_varargs=_tmp352;struct Cyc_Absyn_VarargInfo*cyc_varargs=_tmp353;void*effopt=_tmp354;struct Cyc_List_List*rgn_po=_tmp355;struct Cyc_Absyn_Exp*req=_tmp356;struct Cyc_Absyn_Exp*ens=_tmp357;struct Cyc_List_List*ieff=_tmp358;struct Cyc_List_List*oeff=_tmp359;struct Cyc_List_List*throws=_tmp35A;int reentrant=_tmp35B;
# 1339
({Cyc_Absyndump_dumpfunargs(args,c_varargs,cyc_varargs,effopt,rgn_po,req,ens,ieff,oeff,throws,reentrant);});
# 1341
goto _LL20;}}else{_LL27: _tmp34F=((struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f1;_tmp350=((struct Cyc_Absyn_NoTypes_Absyn_Funcparams_struct*)((struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1)->f2;_LL28: {struct Cyc_List_List*sl=_tmp34F;unsigned loc=_tmp350;
# 1343
({({void(*_tmp434)(void(*f)(struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=({void(*_tmp362)(void(*f)(struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep)=(void(*)(void(*f)(struct _fat_ptr*),struct Cyc_List_List*l,struct _fat_ptr start,struct _fat_ptr end,struct _fat_ptr sep))Cyc_Absyndump_group;_tmp362;});struct Cyc_List_List*_tmp433=sl;struct _fat_ptr _tmp432=({const char*_tmp363="(";_tag_fat(_tmp363,sizeof(char),2U);});struct _fat_ptr _tmp431=({const char*_tmp364=")";_tag_fat(_tmp364,sizeof(char),2U);});_tmp434(Cyc_Absyndump_dump_str,_tmp433,_tmp432,_tmp431,({const char*_tmp365=",";_tag_fat(_tmp365,sizeof(char),2U);}));});});goto _LL20;}}case 4U: _LL29: _tmp34C=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmp34A)->f1;_tmp34D=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmp34A)->f2;_tmp34E=((struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct*)_tmp34A)->f3;_LL2A: {struct Cyc_List_List*ts=_tmp34C;unsigned loc=_tmp34D;int print_kinds=_tmp34E;
# 1345
if(print_kinds)({Cyc_Absyndump_dumpkindedtvars(ts);});else{({Cyc_Absyndump_dumptvars(ts);});}goto _LL20;}case 5U: _LL2B: _tmp34B=((struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct*)_tmp34A)->f2;_LL2C: {struct Cyc_List_List*atts=_tmp34B;
({Cyc_Absyndump_dumpatts(atts);});goto _LL20;}default: _LL2D: _LL2E:
({void*_tmp366=0U;({struct _fat_ptr _tmp435=({const char*_tmp367="dumptms";_tag_fat(_tmp367,sizeof(char),8U);});Cyc_Warn_impos(_tmp435,_tag_fat(_tmp366,sizeof(void*),0U));});});}_LL20:;}
# 1349
return;}}_LL0:;}}
# 1353
static void Cyc_Absyndump_dumptqtd(struct Cyc_Absyn_Tqual tq,void*t,void(*f)(void*),void*a){
int cp=({Cyc_Absyndump_is_char_ptr(t);});
struct _RegionHandle _tmp369=_new_region("temp");struct _RegionHandle*temp=& _tmp369;_push_region(temp);
{struct _tuple13 _stmttmp20=({Cyc_Absynpp_to_tms(temp,tq,t);});struct Cyc_List_List*_tmp36C;void*_tmp36B;struct Cyc_Absyn_Tqual _tmp36A;_LL1: _tmp36A=_stmttmp20.f1;_tmp36B=_stmttmp20.f2;_tmp36C=_stmttmp20.f3;_LL2: {struct Cyc_Absyn_Tqual tq=_tmp36A;void*t=_tmp36B;struct Cyc_List_List*tms=_tmp36C;
({Cyc_Absyndump_dumptq(tq);});
({Cyc_Absyndump_dumpntyp(t);});
({void(*_tmp36D)(int is_char_ptr,struct Cyc_List_List*tms,void(*f)(void*),void*a)=Cyc_Absyndump_dumptms;int _tmp36E=cp;struct Cyc_List_List*_tmp36F=({Cyc_List_imp_rev(tms);});void(*_tmp370)(void*)=f;void*_tmp371=a;_tmp36D(_tmp36E,_tmp36F,_tmp370,_tmp371);});}}
# 1356
;_pop_region();}
# 1362
void Cyc_Absyndump_dumpdecllist2file(struct Cyc_List_List*tdl,struct Cyc___cycFILE*f){
# 1364
*Cyc_Absyndump_dump_file=f;
for(0;tdl != 0;tdl=tdl->tl){
({Cyc_Absyndump_dumpdecl((struct Cyc_Absyn_Decl*)tdl->hd);});}
({void*_tmp373=0U;({struct Cyc___cycFILE*_tmp437=f;struct _fat_ptr _tmp436=({const char*_tmp374="\n";_tag_fat(_tmp374,sizeof(char),2U);});Cyc_fprintf(_tmp437,_tmp436,_tag_fat(_tmp373,sizeof(void*),0U));});});}
# 1370
static void Cyc_Absyndump_dump_decl_interface(struct Cyc_Absyn_Decl*d){
void*_stmttmp21=d->r;void*_tmp376=_stmttmp21;struct _tuple11*_tmp37A;struct Cyc_List_List*_tmp379;struct Cyc_List_List*_tmp378;struct Cyc_List_List*_tmp377;struct Cyc_List_List*_tmp37B;struct Cyc_List_List*_tmp37C;struct Cyc_List_List*_tmp37E;struct _fat_ptr*_tmp37D;struct Cyc_Absyn_Typedefdecl*_tmp37F;struct Cyc_Absyn_Enumdecl*_tmp380;struct Cyc_Absyn_Datatypedecl*_tmp381;struct Cyc_Absyn_Aggrdecl*_tmp382;struct Cyc_Absyn_Fndecl*_tmp383;struct Cyc_Absyn_Vardecl*_tmp384;switch(*((int*)_tmp376)){case 0U: _LL1: _tmp384=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmp376)->f1;_LL2: {struct Cyc_Absyn_Vardecl*vd=_tmp384;
# 1373
if((int)vd->sc == (int)Cyc_Absyn_Static)return;{struct Cyc_Absyn_Exp*init=vd->initializer;
# 1375
vd->initializer=0;
if((int)vd->sc == (int)Cyc_Absyn_Public)
({Cyc_Absyndump_dump(({const char*_tmp385="extern ";_tag_fat(_tmp385,sizeof(char),8U);}));});
# 1376
({Cyc_Absyndump_dumpvardecl(vd,d->loc,1);});
# 1379
({Cyc_Absyndump_dump(({const char*_tmp386="\n";_tag_fat(_tmp386,sizeof(char),2U);}));});
vd->initializer=init;
goto _LL0;}}case 1U: _LL3: _tmp383=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmp376)->f1;_LL4: {struct Cyc_Absyn_Fndecl*fd=_tmp383;
# 1383
if((int)fd->sc == (int)Cyc_Absyn_Static)return;({Cyc_Absyndump_dumpscope(fd->sc);});{
# 1385
struct Cyc_Absyn_FnInfo type_info=fd->i;
type_info.attributes=0;{
void*t=(void*)({struct Cyc_Absyn_FnType_Absyn_Type_struct*_tmp38E=_cycalloc(sizeof(*_tmp38E));_tmp38E->tag=5U,_tmp38E->f1=type_info;_tmp38E;});
({void(*_tmp387)(struct Cyc_Absyn_Tqual tq,void*t,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*a)=({void(*_tmp388)(struct Cyc_Absyn_Tqual tq,void*t,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*a)=(void(*)(struct Cyc_Absyn_Tqual tq,void*t,void(*f)(struct Cyc_Absyn_Fndecl*),struct Cyc_Absyn_Fndecl*a))Cyc_Absyndump_dumptqtd;_tmp388;});struct Cyc_Absyn_Tqual _tmp389=({Cyc_Absyn_empty_tqual(0U);});void*_tmp38A=t;void(*_tmp38B)(struct Cyc_Absyn_Fndecl*fd)=Cyc_Absyndump_dump_atts_qvar;struct Cyc_Absyn_Fndecl*_tmp38C=fd;_tmp387(_tmp389,_tmp38A,_tmp38B,_tmp38C);});
({Cyc_Absyndump_dump(({const char*_tmp38D=";\n";_tag_fat(_tmp38D,sizeof(char),3U);}));});
goto _LL0;}}}case 5U: _LL5: _tmp382=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmp376)->f1;_LL6: {struct Cyc_Absyn_Aggrdecl*ad=_tmp382;
# 1392
if((int)ad->sc == (int)Cyc_Absyn_Static)return;{struct Cyc_Absyn_AggrdeclImpl*impl=ad->impl;
# 1394
if((int)ad->sc == (int)Cyc_Absyn_Abstract)ad->impl=0;({Cyc_Absyndump_dump_aggrdecl(ad);});
# 1396
ad->impl=impl;
({Cyc_Absyndump_dump(({const char*_tmp38F=";\n";_tag_fat(_tmp38F,sizeof(char),3U);}));});
goto _LL0;}}case 6U: _LL7: _tmp381=((struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct*)_tmp376)->f1;_LL8: {struct Cyc_Absyn_Datatypedecl*dd=_tmp381;
({Cyc_Absyndump_dump_datatypedecl(dd);});({Cyc_Absyndump_dump(({const char*_tmp390=";\n";_tag_fat(_tmp390,sizeof(char),3U);}));});goto _LL0;}case 7U: _LL9: _tmp380=((struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct*)_tmp376)->f1;_LLA: {struct Cyc_Absyn_Enumdecl*ed=_tmp380;
({Cyc_Absyndump_dump_enumdecl(ed);});({Cyc_Absyndump_dump(({const char*_tmp391=";\n";_tag_fat(_tmp391,sizeof(char),3U);}));});goto _LL0;}case 8U: _LLB: _tmp37F=((struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct*)_tmp376)->f1;_LLC: {struct Cyc_Absyn_Typedefdecl*td=_tmp37F;
# 1402
if(td->defn == 0){
({Cyc_Absyndump_dumpdecl(d);});
({Cyc_Absyndump_dump(({const char*_tmp392="\n";_tag_fat(_tmp392,sizeof(char),2U);}));});}
# 1402
goto _LL0;}case 9U: _LLD: _tmp37D=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp376)->f1;_tmp37E=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmp376)->f2;_LLE: {struct _fat_ptr*v=_tmp37D;struct Cyc_List_List*ds=_tmp37E;
# 1408
({Cyc_Absyndump_dump(({const char*_tmp393="namespace ";_tag_fat(_tmp393,sizeof(char),11U);}));});
({Cyc_Absyndump_dump_str(v);});
({Cyc_Absyndump_dump(({const char*_tmp394="{\n";_tag_fat(_tmp394,sizeof(char),3U);}));});
for(0;ds != 0;ds=ds->tl){
({Cyc_Absyndump_dump_decl_interface((struct Cyc_Absyn_Decl*)ds->hd);});}
({Cyc_Absyndump_dump(({const char*_tmp395="}\n";_tag_fat(_tmp395,sizeof(char),3U);}));});
goto _LL0;}case 10U: _LLF: _tmp37C=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmp376)->f2;_LL10: {struct Cyc_List_List*ds=_tmp37C;
# 1416
for(0;ds != 0;ds=ds->tl){
({Cyc_Absyndump_dump_decl_interface((struct Cyc_Absyn_Decl*)ds->hd);});}
goto _LL0;}case 11U: _LL11: _tmp37B=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmp376)->f1;_LL12: {struct Cyc_List_List*ds=_tmp37B;
# 1420
({Cyc_Absyndump_dump(({const char*_tmp396="extern \"C\" {";_tag_fat(_tmp396,sizeof(char),13U);}));});
for(0;ds != 0;ds=ds->tl){
({Cyc_Absyndump_dump_decl_interface((struct Cyc_Absyn_Decl*)ds->hd);});}
({Cyc_Absyndump_dump(({const char*_tmp397="}\n";_tag_fat(_tmp397,sizeof(char),3U);}));});
goto _LL0;}case 12U: _LL13: _tmp377=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp376)->f1;_tmp378=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp376)->f2;_tmp379=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp376)->f3;_tmp37A=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmp376)->f4;_LL14: {struct Cyc_List_List*ds=_tmp377;struct Cyc_List_List*ovrs=_tmp378;struct Cyc_List_List*exs=_tmp379;struct _tuple11*wc=_tmp37A;
# 1441 "absyndump.cyc"
goto _LL0;}default: _LL15: _LL16:
({void*_tmp398=0U;({int(*_tmp439)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp39A)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp39A;});struct _fat_ptr _tmp438=({const char*_tmp399="bad top-level declaration";_tag_fat(_tmp399,sizeof(char),26U);});_tmp439(_tmp438,_tag_fat(_tmp398,sizeof(void*),0U));});});}_LL0:;}
# 1446
void Cyc_Absyndump_dump_interface(struct Cyc_List_List*ds,struct Cyc___cycFILE*f){
*Cyc_Absyndump_dump_file=f;
for(0;ds != 0;ds=ds->tl){
({Cyc_Absyndump_dump_decl_interface((struct Cyc_Absyn_Decl*)ds->hd);});}}
