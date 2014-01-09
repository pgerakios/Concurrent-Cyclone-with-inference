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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178 "list.h"
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);
# 184
struct Cyc_List_List*Cyc_List_append(struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73 "cycboot.h"
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};
# 157 "cycboot.h"
int Cyc_printf(struct _fat_ptr,struct _fat_ptr);extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 50 "string.h"
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 62 "dict.h"
struct Cyc_Dict_Dict Cyc_Dict_empty(int(*cmp)(void*,void*));
# 87
struct Cyc_Dict_Dict Cyc_Dict_insert(struct Cyc_Dict_Dict d,void*k,void*v);
# 110
void*Cyc_Dict_lookup(struct Cyc_Dict_Dict d,void*k);struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
enum Cyc_Absyn_Format_Type{Cyc_Absyn_Printf_ft =0U,Cyc_Absyn_Scanf_ft =1U};struct Cyc_Absyn_Regparm_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Stdcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Cdecl_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Fastcall_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Noreturn_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Const_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Aligned_att_Absyn_Attribute_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Packed_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Section_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Nocommon_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Shared_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Unused_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Weak_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllimport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Dllexport_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_instrument_function_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Constructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Destructor_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_check_memory_usage_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Format_att_Absyn_Attribute_struct{int tag;enum Cyc_Absyn_Format_Type f1;int f2;int f3;};struct Cyc_Absyn_Initializes_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Noliveunique_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Consume_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Pure_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Mode_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Alias_att_Absyn_Attribute_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Always_inline_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_No_throw_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Non_null_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_Deprecated_att_Absyn_Attribute_struct{int tag;};struct Cyc_Absyn_VectorSize_att_Absyn_Attribute_struct{int tag;int f1;};struct Cyc_Absyn_Carray_mod_Absyn_Type_modifier_struct{int tag;void*f1;unsigned f2;};struct Cyc_Absyn_ConstArray_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;unsigned f3;};struct Cyc_Absyn_Pointer_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_Absyn_PtrAtts f1;struct Cyc_Absyn_Tqual f2;};struct Cyc_Absyn_Function_mod_Absyn_Type_modifier_struct{int tag;void*f1;};struct Cyc_Absyn_TypeParams_mod_Absyn_Type_modifier_struct{int tag;struct Cyc_List_List*f1;unsigned f2;int f3;};struct Cyc_Absyn_Attributes_mod_Absyn_Type_modifier_struct{int tag;unsigned f1;struct Cyc_List_List*f2;};struct _union_Cnst_Null_c{int tag;int val;};struct _tuple3{enum Cyc_Absyn_Sign f1;char f2;};struct _union_Cnst_Char_c{int tag;struct _tuple3 val;};struct _union_Cnst_Wchar_c{int tag;struct _fat_ptr val;};struct _tuple4{enum Cyc_Absyn_Sign f1;short f2;};struct _union_Cnst_Short_c{int tag;struct _tuple4 val;};struct _tuple5{enum Cyc_Absyn_Sign f1;int f2;};struct _union_Cnst_Int_c{int tag;struct _tuple5 val;};struct _tuple6{enum Cyc_Absyn_Sign f1;long long f2;};struct _union_Cnst_LongLong_c{int tag;struct _tuple6 val;};struct _tuple7{struct _fat_ptr f1;int f2;};struct _union_Cnst_Float_c{int tag;struct _tuple7 val;};struct _union_Cnst_String_c{int tag;struct _fat_ptr val;};struct _union_Cnst_Wstring_c{int tag;struct _fat_ptr val;};union Cyc_Absyn_Cnst{struct _union_Cnst_Null_c Null_c;struct _union_Cnst_Char_c Char_c;struct _union_Cnst_Wchar_c Wchar_c;struct _union_Cnst_Short_c Short_c;struct _union_Cnst_Int_c Int_c;struct _union_Cnst_LongLong_c LongLong_c;struct _union_Cnst_Float_c Float_c;struct _union_Cnst_String_c String_c;struct _union_Cnst_Wstring_c Wstring_c;};
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 954 "absyn.h"
int Cyc_Absyn_qvar_cmp(struct _tuple0*,struct _tuple0*);
# 991
extern void*Cyc_Absyn_heap_rgn_type;extern void*Cyc_Absyn_unique_rgn_type;extern void*Cyc_Absyn_refcnt_rgn_type;
# 993
extern void*Cyc_Absyn_empty_effect;
# 995
extern void*Cyc_Absyn_false_type;
# 997
void*Cyc_Absyn_access_eff(void*);void*Cyc_Absyn_join_eff(struct Cyc_List_List*);
# 1012
extern struct _tuple0*Cyc_Absyn_exn_name;
struct Cyc_Absyn_Datatypedecl*Cyc_Absyn_exn_tud();
# 1042
void*Cyc_Absyn_fatptr_type(void*t,void*rgn,struct Cyc_Absyn_Tqual,void*zeroterm);
# 1276
void*Cyc_Absyn_caccess_eff_dummy(void*r);
# 1288
int Cyc_Absyn_is_xrgn(void*r);
# 1374
int Cyc_Absyn_is_throwsany(struct Cyc_List_List*t);
# 1376
int Cyc_Absyn_is_nothrow(struct Cyc_List_List*t);
# 1399
int Cyc_Absyn_is_reentrant(int);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 64
struct _fat_ptr Cyc_Absynpp_kind2string(struct Cyc_Absyn_Kind*);struct Cyc_RgnOrder_RgnPO;
# 33 "rgnorder.h"
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_initial_fn_po(struct Cyc_List_List*tvs,struct Cyc_List_List*po,void*effect,struct Cyc_Absyn_Tvar*fst_rgn,unsigned);
# 37
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint(struct Cyc_RgnOrder_RgnPO*,void*eff,void*rgn,unsigned);
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_youngest(struct Cyc_RgnOrder_RgnPO*,struct Cyc_Absyn_Tvar*rgn,int opened);
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_unordered(struct Cyc_RgnOrder_RgnPO*,struct Cyc_Absyn_Tvar*rgn);
# 41
int Cyc_RgnOrder_rgn_outlives_rgn(struct Cyc_RgnOrder_RgnPO*,void*rgn1,void*rgn2);
int Cyc_RgnOrder_eff_outlives_eff(struct Cyc_RgnOrder_RgnPO*,void*eff1,void*eff2);
int Cyc_RgnOrder_satisfies_constraints(struct Cyc_RgnOrder_RgnPO*,struct Cyc_List_List*constraints,void*default_bound,int do_pin);
# 46
struct Cyc_RgnOrder_RgnPO*Cyc_RgnOrder_add_outlives_constraint_special(struct Cyc_RgnOrder_RgnPO*po,void*eff,void*rgn);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 66 "tcenv.h"
void*Cyc_Tcenv_env_err(struct _fat_ptr msg);
# 90
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 100
struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*);
# 133
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_named_block(unsigned,struct Cyc_Tcenv_Tenv*,struct Cyc_Absyn_Tvar*);
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraints(struct Cyc_Tcenv_Tenv*,struct Cyc_List_List*,unsigned);
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
# 103
int Cyc_Tcutil_kind_leq(struct Cyc_Absyn_Kind*,struct Cyc_Absyn_Kind*);
# 108
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*def);
struct Cyc_Absyn_Kind*Cyc_Tcutil_type_kind(void*);
void*Cyc_Tcutil_compress(void*);
# 139
extern struct Cyc_Absyn_Kind Cyc_Tcutil_rk;
# 141
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 178
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);struct _tuple12{struct Cyc_Absyn_Tvar*f1;void*f2;};
# 180
struct _tuple12 Cyc_Tcutil_swap_kind(void*,void*);
# 200
int Cyc_Tcutil_subset_effect(int may_constrain_evars,void*e1,void*e2);
# 204
int Cyc_Tcutil_region_in_effect(int constrain,void*r,void*e);
# 219
void Cyc_Tcutil_check_unique_tvars(unsigned,struct Cyc_List_List*);
# 261
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 263
int Cyc_Tcutil_new_tvar_id();
# 265
void Cyc_Tcutil_add_tvar_identity(struct Cyc_Absyn_Tvar*);
void Cyc_Tcutil_add_tvar_identities(struct Cyc_List_List*);
# 51 "tcenv.cyc"
static char Cyc_Tcenv_throw_annot(void*t);
static int Cyc_Tcenv_has_io_eff_annot(void*t);
static struct Cyc_List_List*Cyc_Tcenv_fn_throw_annot(void*t);char Cyc_Tcenv_Env_error[10U]="Env_error";
# 56
struct Cyc_Tcenv_Env_error_exn_struct Cyc_Tcenv_Env_error_val={Cyc_Tcenv_Env_error};
# 58
void*Cyc_Tcenv_env_err(struct _fat_ptr msg){
({struct Cyc_String_pa_PrintArg_struct _tmp2=({struct Cyc_String_pa_PrintArg_struct _tmp10B;_tmp10B.tag=0U,_tmp10B.f1=(struct _fat_ptr)((struct _fat_ptr)msg);_tmp10B;});void*_tmp0[1U];_tmp0[0]=& _tmp2;({struct _fat_ptr _tmp115=({const char*_tmp1="\n%s\n";_tag_fat(_tmp1,sizeof(char),5U);});Cyc_printf(_tmp115,_tag_fat(_tmp0,sizeof(void*),1U));});});
(int)_throw(& Cyc_Tcenv_Env_error_val);}struct _tuple13{struct Cyc_Absyn_Switch_clause*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;};struct Cyc_Tcenv_SharedFenv{void*return_typ;struct Cyc_List_List*delayed_effect_checks;struct Cyc_List_List*delayed_constraint_checks;};struct Cyc_Tcenv_FenvFlags{enum Cyc_Tcenv_NewStatus in_new;int in_notreadctxt: 1;int in_lhs: 1;int abstract_ok: 1;int in_stmt_exp: 1;int is_io_eff_annot: 1;unsigned char is_throw_annot: 2;unsigned char is_reentrant: 1;};struct Cyc_Tcenv_Fenv{struct Cyc_Tcenv_SharedFenv*shared;struct Cyc_List_List*type_vars;struct Cyc_RgnOrder_RgnPO*region_order;const struct _tuple13*ctrl_env;void*capability;void*curr_rgn;struct Cyc_Tcenv_FenvFlags flags;struct Cyc_List_List**throws;struct Cyc_List_List*annot;unsigned loc;};
# 129
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_tc_init(){
# 131
struct Cyc_Tcenv_Genv*ae=({struct Cyc_Tcenv_Genv*_tmp10=_cycalloc(sizeof(*_tmp10));({struct Cyc_Dict_Dict _tmp11A=({({struct Cyc_Dict_Dict(*_tmpB)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmpB;})(Cyc_Absyn_qvar_cmp);});_tmp10->aggrdecls=_tmp11A;}),({
struct Cyc_Dict_Dict _tmp119=({({struct Cyc_Dict_Dict(*_tmpC)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmpC;})(Cyc_Absyn_qvar_cmp);});_tmp10->datatypedecls=_tmp119;}),({
struct Cyc_Dict_Dict _tmp118=({({struct Cyc_Dict_Dict(*_tmpD)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmpD;})(Cyc_Absyn_qvar_cmp);});_tmp10->enumdecls=_tmp118;}),({
struct Cyc_Dict_Dict _tmp117=({({struct Cyc_Dict_Dict(*_tmpE)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmpE;})(Cyc_Absyn_qvar_cmp);});_tmp10->typedefs=_tmp117;}),({
struct Cyc_Dict_Dict _tmp116=({({struct Cyc_Dict_Dict(*_tmpF)(int(*cmp)(struct _tuple0*,struct _tuple0*))=(struct Cyc_Dict_Dict(*)(int(*cmp)(struct _tuple0*,struct _tuple0*)))Cyc_Dict_empty;_tmpF;})(Cyc_Absyn_qvar_cmp);});_tmp10->ordinaries=_tmp116;});_tmp10;});
({struct Cyc_Dict_Dict _tmp11C=({struct Cyc_Dict_Dict(*_tmp4)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl**v)=({struct Cyc_Dict_Dict(*_tmp5)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl**v)=(struct Cyc_Dict_Dict(*)(struct Cyc_Dict_Dict d,struct _tuple0*k,struct Cyc_Absyn_Datatypedecl**v))Cyc_Dict_insert;_tmp5;});struct Cyc_Dict_Dict _tmp6=ae->datatypedecls;struct _tuple0*_tmp7=Cyc_Absyn_exn_name;struct Cyc_Absyn_Datatypedecl**_tmp8=({struct Cyc_Absyn_Datatypedecl**_tmp9=_cycalloc(sizeof(*_tmp9));({struct Cyc_Absyn_Datatypedecl*_tmp11B=({Cyc_Absyn_exn_tud();});*_tmp9=_tmp11B;});_tmp9;});_tmp4(_tmp6,_tmp7,_tmp8);});ae->datatypedecls=_tmp11C;});
return({struct Cyc_Tcenv_Tenv*_tmpA=_cycalloc(sizeof(*_tmpA));_tmpA->ns=0,_tmpA->ae=ae,_tmpA->le=0,_tmpA->allow_valueof=0,_tmpA->in_extern_c_include=0,_tmpA->in_tempest=0,_tmpA->tempest_generalize=0;_tmpA;});}struct _tuple14{void*f1;int f2;};
# 129
void*Cyc_Tcenv_lookup_ordinary_global(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct _tuple0*q,int is_use){
# 140
struct _tuple14*ans=({({struct _tuple14*(*_tmp11E)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct _tuple14*(*_tmp12)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct _tuple14*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp12;});struct Cyc_Dict_Dict _tmp11D=(te->ae)->ordinaries;_tmp11E(_tmp11D,q);});});
if(is_use)
(*ans).f2=1;
# 141
return(*ans).f1;}
# 145
struct Cyc_Absyn_Aggrdecl**Cyc_Tcenv_lookup_aggrdecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct _tuple0*q){
return({({struct Cyc_Absyn_Aggrdecl**(*_tmp120)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Aggrdecl**(*_tmp14)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Aggrdecl**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp14;});struct Cyc_Dict_Dict _tmp11F=(te->ae)->aggrdecls;_tmp120(_tmp11F,q);});});}
# 148
struct Cyc_Absyn_Datatypedecl**Cyc_Tcenv_lookup_datatypedecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct _tuple0*q){
return({({struct Cyc_Absyn_Datatypedecl**(*_tmp122)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Datatypedecl**(*_tmp16)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp16;});struct Cyc_Dict_Dict _tmp121=(te->ae)->datatypedecls;_tmp122(_tmp121,q);});});}
# 151
struct Cyc_Absyn_Datatypedecl***Cyc_Tcenv_lookup_xdatatypedecl(struct _RegionHandle*r,struct Cyc_Tcenv_Tenv*te,unsigned loc,struct _tuple0*q){
return({struct Cyc_Absyn_Datatypedecl***_tmp19=_region_malloc(r,sizeof(*_tmp19));({struct Cyc_Absyn_Datatypedecl**_tmp125=({({struct Cyc_Absyn_Datatypedecl**(*_tmp124)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Datatypedecl**(*_tmp18)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Datatypedecl**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp18;});struct Cyc_Dict_Dict _tmp123=(te->ae)->datatypedecls;_tmp124(_tmp123,q);});});*_tmp19=_tmp125;});_tmp19;});}
# 154
struct Cyc_Absyn_Enumdecl**Cyc_Tcenv_lookup_enumdecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct _tuple0*q){
return({({struct Cyc_Absyn_Enumdecl**(*_tmp127)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Enumdecl**(*_tmp1B)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Enumdecl**(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp1B;});struct Cyc_Dict_Dict _tmp126=(te->ae)->enumdecls;_tmp127(_tmp126,q);});});}
# 154
struct Cyc_Absyn_Typedefdecl*Cyc_Tcenv_lookup_typedefdecl(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct _tuple0*q){
# 158
return({({struct Cyc_Absyn_Typedefdecl*(*_tmp129)(struct Cyc_Dict_Dict d,struct _tuple0*k)=({struct Cyc_Absyn_Typedefdecl*(*_tmp1D)(struct Cyc_Dict_Dict d,struct _tuple0*k)=(struct Cyc_Absyn_Typedefdecl*(*)(struct Cyc_Dict_Dict d,struct _tuple0*k))Cyc_Dict_lookup;_tmp1D;});struct Cyc_Dict_Dict _tmp128=(te->ae)->typedefs;_tmp129(_tmp128,q);});});}
# 163
static int Cyc_Tcenv_check_fenv(struct Cyc_Tcenv_Tenv*te){
# 165
return te->le != 0;}
# 168
static struct Cyc_Tcenv_Fenv*Cyc_Tcenv_get_fenv(struct Cyc_Tcenv_Tenv*te,struct _fat_ptr err_msg){
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)({({int(*_tmp12A)(struct _fat_ptr msg)=({int(*_tmp20)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_Tcenv_env_err;_tmp20;});_tmp12A(err_msg);});});return le;}
# 173
static struct Cyc_Tcenv_Tenv*Cyc_Tcenv_put_fenv(struct Cyc_Tcenv_Tenv*te,struct Cyc_Tcenv_Fenv*fe){
if(te->le == 0)({({int(*_tmp12B)(struct _fat_ptr msg)=({int(*_tmp22)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_Tcenv_env_err;_tmp22;});_tmp12B(({const char*_tmp23="put_fenv";_tag_fat(_tmp23,sizeof(char),9U);}));});});{struct Cyc_Tcenv_Tenv*ans=({struct Cyc_Tcenv_Tenv*_tmp24=_cycalloc(sizeof(*_tmp24));*_tmp24=*te;_tmp24;});
# 176
ans->le=fe;
return ans;}}
# 179
static struct Cyc_Tcenv_Tenv*Cyc_Tcenv_put_emptyfenv(struct Cyc_Tcenv_Tenv*te){
struct Cyc_Tcenv_Tenv*ans=({struct Cyc_Tcenv_Tenv*_tmp26=_cycalloc(sizeof(*_tmp26));*_tmp26=*te;_tmp26;});
ans->le=0;
return ans;}
# 179
void*Cyc_Tcenv_return_typ(struct Cyc_Tcenv_Tenv*te){
# 186
return(({({struct Cyc_Tcenv_Tenv*_tmp12C=te;Cyc_Tcenv_get_fenv(_tmp12C,({const char*_tmp28="return_typ";_tag_fat(_tmp28,sizeof(char),11U);}));});})->shared)->return_typ;}
# 189
struct Cyc_List_List*Cyc_Tcenv_lookup_type_vars(struct Cyc_Tcenv_Tenv*te){
struct Cyc_Tcenv_Fenv*le=te->le;
if(te->le == 0)return 0;return((struct Cyc_Tcenv_Fenv*)_check_null(le))->type_vars;}
# 195
struct Cyc_Core_Opt*Cyc_Tcenv_lookup_opt_type_vars(struct Cyc_Tcenv_Tenv*te){
return({struct Cyc_Core_Opt*_tmp2B=_cycalloc(sizeof(*_tmp2B));({struct Cyc_List_List*_tmp12D=({Cyc_Tcenv_lookup_type_vars(te);});_tmp2B->v=_tmp12D;});_tmp2B;});}
# 195
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_type_vars(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*tvs){
# 200
struct Cyc_Tcenv_Fenv*fe=({struct Cyc_Tcenv_Fenv*_tmp2E=_cycalloc(sizeof(*_tmp2E));({struct Cyc_Tcenv_Fenv _tmp12F=*({({struct Cyc_Tcenv_Tenv*_tmp12E=te;Cyc_Tcenv_get_fenv(_tmp12E,({const char*_tmp2D="add_type_vars";_tag_fat(_tmp2D,sizeof(char),14U);}));});});*_tmp2E=_tmp12F;});_tmp2E;});
({Cyc_Tcutil_add_tvar_identities(tvs);});{
struct Cyc_List_List*new_tvs=({Cyc_List_append(tvs,fe->type_vars);});
({Cyc_Tcutil_check_unique_tvars(loc,new_tvs);});
fe->type_vars=new_tvs;
return({Cyc_Tcenv_put_fenv(te,fe);});}}
# 195
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_new_status(enum Cyc_Tcenv_NewStatus status,struct Cyc_Tcenv_Tenv*te){
# 209
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return({Cyc_Tcenv_put_emptyfenv(te);});{struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp30=_cycalloc(sizeof(*_tmp30));*_tmp30=*le;_tmp30;});
# 212
(ans->flags).in_new=status;
return({Cyc_Tcenv_put_fenv(te,ans);});}}
# 195
enum Cyc_Tcenv_NewStatus Cyc_Tcenv_new_status(struct Cyc_Tcenv_Tenv*te){
# 216
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return Cyc_Tcenv_NoneNew;return(le->flags).in_new;}
# 195
int Cyc_Tcenv_abstract_val_ok(struct Cyc_Tcenv_Tenv*te){
# 221
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return 0;return(le->flags).abstract_ok;}
# 195
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_abstract_val_ok(struct Cyc_Tcenv_Tenv*te){
# 226
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return({Cyc_Tcenv_put_emptyfenv(te);});{struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp34=_cycalloc(sizeof(*_tmp34));*_tmp34=*le;_tmp34;});
# 229
(ans->flags).abstract_ok=1;
return({Cyc_Tcenv_put_fenv(te,ans);});}}
# 195
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_abstract_val_ok(struct Cyc_Tcenv_Tenv*te){
# 233
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return({Cyc_Tcenv_put_emptyfenv(te);});{struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp36=_cycalloc(sizeof(*_tmp36));*_tmp36=*le;_tmp36;});
# 236
(ans->flags).abstract_ok=0;
return({Cyc_Tcenv_put_fenv(te,ans);});}}
# 195
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_notreadctxt(struct Cyc_Tcenv_Tenv*te){
# 240
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return({Cyc_Tcenv_put_emptyfenv(te);});{struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp38=_cycalloc(sizeof(*_tmp38));*_tmp38=*le;_tmp38;});
# 243
(ans->flags).in_notreadctxt=1;
return({Cyc_Tcenv_put_fenv(te,ans);});}}
# 195
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_notreadctxt(struct Cyc_Tcenv_Tenv*te){
# 247
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return({Cyc_Tcenv_put_emptyfenv(te);});{struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp3A=_cycalloc(sizeof(*_tmp3A));*_tmp3A=*le;_tmp3A;});
# 250
(ans->flags).in_notreadctxt=0;
return({Cyc_Tcenv_put_fenv(te,ans);});}}
# 195
int Cyc_Tcenv_in_notreadctxt(struct Cyc_Tcenv_Tenv*te){
# 254
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return 0;return(le->flags).in_notreadctxt;}
# 195
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_lhs(struct Cyc_Tcenv_Tenv*te){
# 259
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return({Cyc_Tcenv_put_emptyfenv(te);});{struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp3D=_cycalloc(sizeof(*_tmp3D));*_tmp3D=*le;_tmp3D;});
# 262
(ans->flags).in_lhs=1;
return({Cyc_Tcenv_put_fenv(te,ans);});}}
# 195
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_lhs(struct Cyc_Tcenv_Tenv*te){
# 266
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return({Cyc_Tcenv_put_emptyfenv(te);});{struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp3F=_cycalloc(sizeof(*_tmp3F));*_tmp3F=*le;_tmp3F;});
# 269
(ans->flags).in_lhs=0;
return({Cyc_Tcenv_put_fenv(te,ans);});}}
# 195
int Cyc_Tcenv_in_lhs(struct Cyc_Tcenv_Tenv*te){
# 273
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return 0;return(le->flags).in_lhs;}
# 195
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_stmt_exp(struct Cyc_Tcenv_Tenv*te){
# 278
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return({Cyc_Tcenv_put_emptyfenv(te);});{struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp42=_cycalloc(sizeof(*_tmp42));*_tmp42=*le;_tmp42;});
# 281
(ans->flags).in_stmt_exp=1;
return({Cyc_Tcenv_put_fenv(te,ans);});}}
# 195
int Cyc_Tcenv_in_stmt_exp(struct Cyc_Tcenv_Tenv*te){
# 285
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return 0;return(le->flags).in_stmt_exp;}
# 292
const struct _tuple13*const Cyc_Tcenv_process_fallthru(struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Stmt*s,struct Cyc_Absyn_Switch_clause***clauseopt){
const struct _tuple13*ans=({({struct Cyc_Tcenv_Tenv*_tmp130=te;Cyc_Tcenv_get_fenv(_tmp130,({const char*_tmp46="process_fallthru";_tag_fat(_tmp46,sizeof(char),17U);}));});})->ctrl_env;
if(ans != (const struct _tuple13*)0)
({struct Cyc_Absyn_Switch_clause**_tmp131=({struct Cyc_Absyn_Switch_clause**_tmp45=_cycalloc(sizeof(*_tmp45));*_tmp45=(*ans).f1;_tmp45;});*clauseopt=_tmp131;});
# 294
return ans;}
# 292
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_set_fallthru(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*new_tvs,struct Cyc_List_List*vds,struct Cyc_Absyn_Switch_clause*clause){
# 301
struct Cyc_List_List*ft_typ=0;
for(0;vds != 0;vds=vds->tl){
ft_typ=({struct Cyc_List_List*_tmp48=_cycalloc(sizeof(*_tmp48));_tmp48->hd=((struct Cyc_Absyn_Vardecl*)vds->hd)->type,_tmp48->tl=ft_typ;_tmp48;});}{
struct _tuple13*new_ctrl_env=({struct _tuple13*_tmp4B=_cycalloc(sizeof(*_tmp4B));_tmp4B->f1=clause,_tmp4B->f2=new_tvs,({struct Cyc_List_List*_tmp132=({Cyc_List_imp_rev(ft_typ);});_tmp4B->f3=_tmp132;});_tmp4B;});
struct Cyc_Tcenv_Fenv*new_fe=({struct Cyc_Tcenv_Fenv*_tmp4A=_cycalloc(sizeof(*_tmp4A));({struct Cyc_Tcenv_Fenv _tmp134=*({({struct Cyc_Tcenv_Tenv*_tmp133=te;Cyc_Tcenv_get_fenv(_tmp133,({const char*_tmp49="set_fallthru";_tag_fat(_tmp49,sizeof(char),13U);}));});});*_tmp4A=_tmp134;});_tmp4A;});
new_fe->ctrl_env=(const struct _tuple13*)new_ctrl_env;
return({Cyc_Tcenv_put_fenv(te,new_fe);});}}
# 292
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_fallthru(struct Cyc_Tcenv_Tenv*te){
# 311
struct Cyc_Tcenv_Fenv*fe=({struct Cyc_Tcenv_Fenv*_tmp4E=_cycalloc(sizeof(*_tmp4E));({struct Cyc_Tcenv_Fenv _tmp136=*({({struct Cyc_Tcenv_Tenv*_tmp135=te;Cyc_Tcenv_get_fenv(_tmp135,({const char*_tmp4D="clear_fallthru";_tag_fat(_tmp4D,sizeof(char),15U);}));});});*_tmp4E=_tmp136;});_tmp4E;});
fe->ctrl_env=0;
return({Cyc_Tcenv_put_fenv(te,fe);});}
# 292
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_allow_valueof(struct Cyc_Tcenv_Tenv*te){
# 317
struct Cyc_Tcenv_Fenv*fe=te->le == 0?0:({struct Cyc_Tcenv_Fenv*_tmp51=_cycalloc(sizeof(*_tmp51));*_tmp51=*((struct Cyc_Tcenv_Fenv*)_check_null(te->le));_tmp51;});
struct Cyc_Tcenv_Tenv*ans=({struct Cyc_Tcenv_Tenv*_tmp50=_cycalloc(sizeof(*_tmp50));*_tmp50=*te;_tmp50;});
ans->allow_valueof=1;
return ans;}
# 292
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_extern_c_include(struct Cyc_Tcenv_Tenv*te){
# 323
struct Cyc_Tcenv_Fenv*fe=te->le == 0?0:({struct Cyc_Tcenv_Fenv*_tmp54=_cycalloc(sizeof(*_tmp54));*_tmp54=*((struct Cyc_Tcenv_Fenv*)_check_null(te->le));_tmp54;});
struct Cyc_Tcenv_Tenv*ans=({struct Cyc_Tcenv_Tenv*_tmp53=_cycalloc(sizeof(*_tmp53));*_tmp53=*te;_tmp53;});
ans->in_extern_c_include=1;
return ans;}
# 292
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_enter_tempest(struct Cyc_Tcenv_Tenv*te){
# 329
struct Cyc_Tcenv_Fenv*fe=te->le == 0?0:({struct Cyc_Tcenv_Fenv*_tmp57=_cycalloc(sizeof(*_tmp57));*_tmp57=*((struct Cyc_Tcenv_Fenv*)_check_null(te->le));_tmp57;});
struct Cyc_Tcenv_Tenv*ans=({struct Cyc_Tcenv_Tenv*_tmp56=_cycalloc(sizeof(*_tmp56));*_tmp56=*te;_tmp56;});
ans->in_tempest=1;
return ans;}
# 292
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_clear_tempest(struct Cyc_Tcenv_Tenv*te){
# 335
struct Cyc_Tcenv_Fenv*fe=te->le == 0?0:({struct Cyc_Tcenv_Fenv*_tmp5A=_cycalloc(sizeof(*_tmp5A));*_tmp5A=*((struct Cyc_Tcenv_Fenv*)_check_null(te->le));_tmp5A;});
struct Cyc_Tcenv_Tenv*ans=({struct Cyc_Tcenv_Tenv*_tmp59=_cycalloc(sizeof(*_tmp59));*_tmp59=*te;_tmp59;});
ans->in_tempest=0;
return ans;}
# 292
int Cyc_Tcenv_in_tempest(struct Cyc_Tcenv_Tenv*te){
# 341
return te->in_tempest;}
# 292
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_region(struct Cyc_Tcenv_Tenv*te,void*rgn,int opened){
# 344
struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp6A=_cycalloc(sizeof(*_tmp6A));({struct Cyc_Tcenv_Fenv _tmp138=*({({struct Cyc_Tcenv_Tenv*_tmp137=te;Cyc_Tcenv_get_fenv(_tmp137,({const char*_tmp69="add_region";_tag_fat(_tmp69,sizeof(char),11U);}));});});*_tmp6A=_tmp138;});_tmp6A;});
void*c=({Cyc_Tcutil_compress(rgn);});
struct Cyc_RgnOrder_RgnPO*region_order=ans->region_order;
if(!({Cyc_Absyn_is_xrgn(c);})){
# 349
{void*_tmp5D=c;struct Cyc_Absyn_Tvar*_tmp5E;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp5D)->tag == 2U){_LL1: _tmp5E=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp5D)->f1;_LL2: {struct Cyc_Absyn_Tvar*x=_tmp5E;
# 351
region_order=({Cyc_RgnOrder_add_youngest(region_order,x,opened);});
goto _LL0;}}else{_LL3: _LL4:
 goto _LL0;}_LL0:;}
# 355
({void*_tmp13B=({void*(*_tmp5F)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp60=({struct Cyc_List_List*_tmp62=_cycalloc(sizeof(*_tmp62));({void*_tmp13A=({Cyc_Absyn_access_eff(rgn);});_tmp62->hd=_tmp13A;}),({
struct Cyc_List_List*_tmp139=({struct Cyc_List_List*_tmp61=_cycalloc(sizeof(*_tmp61));_tmp61->hd=ans->capability,_tmp61->tl=0;_tmp61;});_tmp62->tl=_tmp139;});_tmp62;});_tmp5F(_tmp60);});
# 355
ans->capability=_tmp13B;});}else{
# 360
{void*_tmp63=c;struct Cyc_Absyn_Tvar*_tmp64;if(((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp63)->tag == 2U){_LL6: _tmp64=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmp63)->f1;_LL7: {struct Cyc_Absyn_Tvar*x=_tmp64;
# 362
region_order=({Cyc_RgnOrder_add_unordered(region_order,x);});
goto _LL5;}}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 366
({void*_tmp13E=({void*(*_tmp65)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp66=({struct Cyc_List_List*_tmp68=_cycalloc(sizeof(*_tmp68));({void*_tmp13D=({Cyc_Absyn_caccess_eff_dummy(rgn);});_tmp68->hd=_tmp13D;}),({
struct Cyc_List_List*_tmp13C=({struct Cyc_List_List*_tmp67=_cycalloc(sizeof(*_tmp67));_tmp67->hd=ans->capability,_tmp67->tl=0;_tmp67;});_tmp68->tl=_tmp13C;});_tmp68;});_tmp65(_tmp66);});
# 366
ans->capability=_tmp13E;});}
# 369
ans->region_order=region_order;
return({Cyc_Tcenv_put_fenv(te,ans);});}
# 292
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_named_block(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Tvar*block_rgn){
# 374
struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp73=_cycalloc(sizeof(*_tmp73));({struct Cyc_Tcenv_Fenv _tmp140=*({({struct Cyc_Tcenv_Tenv*_tmp13F=te;Cyc_Tcenv_get_fenv(_tmp13F,({const char*_tmp72="new_named_block";_tag_fat(_tmp72,sizeof(char),16U);}));});});*_tmp73=_tmp140;});_tmp73;});
struct Cyc_Absyn_VarType_Absyn_Type_struct*block_typ=({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmp71=_cycalloc(sizeof(*_tmp71));_tmp71->tag=2U,_tmp71->f1=block_rgn;_tmp71;});
({struct Cyc_List_List*_tmp141=({struct Cyc_List_List*_tmp6C=_cycalloc(sizeof(*_tmp6C));_tmp6C->hd=block_rgn,_tmp6C->tl=ans->type_vars;_tmp6C;});ans->type_vars=_tmp141;});
({Cyc_Tcutil_check_unique_tvars(loc,ans->type_vars);});
({struct Cyc_RgnOrder_RgnPO*_tmp142=({Cyc_RgnOrder_add_youngest(ans->region_order,block_rgn,0);});ans->region_order=_tmp142;});
({void*_tmp145=({void*(*_tmp6D)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmp6E=({struct Cyc_List_List*_tmp70=_cycalloc(sizeof(*_tmp70));({void*_tmp144=({Cyc_Absyn_access_eff((void*)block_typ);});_tmp70->hd=_tmp144;}),({
struct Cyc_List_List*_tmp143=({struct Cyc_List_List*_tmp6F=_cycalloc(sizeof(*_tmp6F));_tmp6F->hd=ans->capability,_tmp6F->tl=0;_tmp6F;});_tmp70->tl=_tmp143;});_tmp70;});_tmp6D(_tmp6E);});
# 379
ans->capability=_tmp145;});
# 381
ans->curr_rgn=(void*)block_typ;
return({Cyc_Tcenv_put_fenv(te,ans);});}
# 292
static struct Cyc_Absyn_Eq_kb_Absyn_KindBound_struct Cyc_Tcenv_rgn_kb={0U,& Cyc_Tcutil_rk};
# 387
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_block(unsigned loc,struct Cyc_Tcenv_Tenv*te){
struct Cyc_Absyn_Tvar*t=({Cyc_Tcutil_new_tvar((void*)& Cyc_Tcenv_rgn_kb);});
({Cyc_Tcutil_add_tvar_identity(t);});
# 391
return({Cyc_Tcenv_new_named_block(loc,te,t);});}struct _tuple15{void*f1;void*f2;};
# 387
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraints(struct Cyc_Tcenv_Tenv*te,struct Cyc_List_List*cs,unsigned loc){
# 401
struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp77=_cycalloc(sizeof(*_tmp77));({struct Cyc_Tcenv_Fenv _tmp147=*({({struct Cyc_Tcenv_Tenv*_tmp146=te;Cyc_Tcenv_get_fenv(_tmp146,({const char*_tmp76="new_outlives_constraints";_tag_fat(_tmp76,sizeof(char),25U);}));});});*_tmp77=_tmp147;});_tmp77;});
for(0;cs != 0;cs=cs->tl){
({struct Cyc_RgnOrder_RgnPO*_tmp148=({Cyc_RgnOrder_add_outlives_constraint(ans->region_order,(*((struct _tuple15*)cs->hd)).f1,(*((struct _tuple15*)cs->hd)).f2,loc);});ans->region_order=_tmp148;});}
# 405
return({Cyc_Tcenv_put_fenv(te,ans);});}
# 387
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_outlives_constraint_special(struct Cyc_Tcenv_Tenv*te,void*child,void*xparent){
# 409
struct Cyc_Tcenv_Fenv*ans=({struct Cyc_Tcenv_Fenv*_tmp7A=_cycalloc(sizeof(*_tmp7A));({struct Cyc_Tcenv_Fenv _tmp14A=*({({struct Cyc_Tcenv_Tenv*_tmp149=te;Cyc_Tcenv_get_fenv(_tmp149,({const char*_tmp79="new_outlives_constraint_special";_tag_fat(_tmp79,sizeof(char),32U);}));});});*_tmp7A=_tmp14A;});_tmp7A;});
({struct Cyc_RgnOrder_RgnPO*_tmp14B=({Cyc_RgnOrder_add_outlives_constraint_special(ans->region_order,xparent,child);});ans->region_order=_tmp14B;});
# 412
return({Cyc_Tcenv_put_fenv(te,ans);});}
# 387
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_add_region_equality(struct Cyc_Tcenv_Tenv*te,void*r10,void*r20,struct _tuple12**oldtv,unsigned loc){
# 419
void*r1=({Cyc_Tcutil_compress(r10);});
void*r2=({Cyc_Tcutil_compress(r20);});
struct Cyc_Absyn_Kind*r1k=({Cyc_Tcutil_type_kind(r1);});
struct Cyc_Absyn_Kind*r2k=({Cyc_Tcutil_type_kind(r2);});
# 425
int r1_le_r2=({Cyc_Tcutil_kind_leq(r1k,r2k);});
int r2_le_r1=({Cyc_Tcutil_kind_leq(r2k,r1k);});
if(!r1_le_r2 && !r2_le_r1){
({struct Cyc_String_pa_PrintArg_struct _tmp7E=({struct Cyc_String_pa_PrintArg_struct _tmp10F;_tmp10F.tag=0U,({
struct _fat_ptr _tmp14C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(r1);}));_tmp10F.f1=_tmp14C;});_tmp10F;});struct Cyc_String_pa_PrintArg_struct _tmp7F=({struct Cyc_String_pa_PrintArg_struct _tmp10E;_tmp10E.tag=0U,({struct _fat_ptr _tmp14D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(r2);}));_tmp10E.f1=_tmp14D;});_tmp10E;});struct Cyc_String_pa_PrintArg_struct _tmp80=({struct Cyc_String_pa_PrintArg_struct _tmp10D;_tmp10D.tag=0U,({struct _fat_ptr _tmp14E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(r1k);}));_tmp10D.f1=_tmp14E;});_tmp10D;});struct Cyc_String_pa_PrintArg_struct _tmp81=({struct Cyc_String_pa_PrintArg_struct _tmp10C;_tmp10C.tag=0U,({struct _fat_ptr _tmp14F=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_kind2string(r2k);}));_tmp10C.f1=_tmp14F;});_tmp10C;});void*_tmp7C[4U];_tmp7C[0]=& _tmp7E,_tmp7C[1]=& _tmp7F,_tmp7C[2]=& _tmp80,_tmp7C[3]=& _tmp81;({unsigned _tmp151=loc;struct _fat_ptr _tmp150=({const char*_tmp7D="Cannot compare region handles for %s and %s\n  kinds %s and %s are not compatible\n";_tag_fat(_tmp7D,sizeof(char),82U);});Cyc_Tcutil_terr(_tmp151,_tmp150,_tag_fat(_tmp7C,sizeof(void*),4U));});});
return({Cyc_Tcenv_new_outlives_constraints(te,0,loc);});}else{
# 432
if(r1_le_r2 && !r2_le_r1)
({struct _tuple12*_tmp153=({struct _tuple12*_tmp85=_cycalloc(sizeof(*_tmp85));({struct _tuple12 _tmp152=({struct _tuple12(*_tmp82)(void*,void*)=Cyc_Tcutil_swap_kind;void*_tmp83=r2;void*_tmp84=({Cyc_Tcutil_kind_to_bound(r1k);});_tmp82(_tmp83,_tmp84);});*_tmp85=_tmp152;});_tmp85;});*oldtv=_tmp153;});else{
if(!r1_le_r2 && r2_le_r1)
({struct _tuple12*_tmp155=({struct _tuple12*_tmp89=_cycalloc(sizeof(*_tmp89));({struct _tuple12 _tmp154=({struct _tuple12(*_tmp86)(void*,void*)=Cyc_Tcutil_swap_kind;void*_tmp87=r1;void*_tmp88=({Cyc_Tcutil_kind_to_bound(r2k);});_tmp86(_tmp87,_tmp88);});*_tmp89=_tmp154;});_tmp89;});*oldtv=_tmp155;});}}{
# 427
struct Cyc_List_List*bds=0;
# 439
if((r1 != Cyc_Absyn_heap_rgn_type && r1 != Cyc_Absyn_unique_rgn_type)&& r1 != Cyc_Absyn_refcnt_rgn_type)
bds=({struct Cyc_List_List*_tmp8B=_cycalloc(sizeof(*_tmp8B));({struct _tuple15*_tmp157=({struct _tuple15*_tmp8A=_cycalloc(sizeof(*_tmp8A));({void*_tmp156=({Cyc_Absyn_access_eff(r1);});_tmp8A->f1=_tmp156;}),_tmp8A->f2=r2;_tmp8A;});_tmp8B->hd=_tmp157;}),_tmp8B->tl=bds;_tmp8B;});
# 439
if(
# 441
(r2 != Cyc_Absyn_heap_rgn_type && r2 != Cyc_Absyn_unique_rgn_type)&& r2 != Cyc_Absyn_refcnt_rgn_type)
bds=({struct Cyc_List_List*_tmp8D=_cycalloc(sizeof(*_tmp8D));({struct _tuple15*_tmp159=({struct _tuple15*_tmp8C=_cycalloc(sizeof(*_tmp8C));({void*_tmp158=({Cyc_Absyn_access_eff(r2);});_tmp8C->f1=_tmp158;}),_tmp8C->f2=r1;_tmp8C;});_tmp8D->hd=_tmp159;}),_tmp8D->tl=bds;_tmp8D;});
# 439
return({Cyc_Tcenv_new_outlives_constraints(te,bds,loc);});}}
# 387
void*Cyc_Tcenv_get_capability(struct Cyc_Tcenv_Tenv*te){
# 448
struct Cyc_Tcenv_Fenv*fe=({({struct Cyc_Tcenv_Tenv*_tmp15A=te;Cyc_Tcenv_get_fenv(_tmp15A,({const char*_tmp8F="get_capability";_tag_fat(_tmp8F,sizeof(char),15U);}));});});
return fe->capability;}
# 387
void*Cyc_Tcenv_curr_rgn(struct Cyc_Tcenv_Tenv*te){
# 455
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return Cyc_Absyn_heap_rgn_type;return le->curr_rgn;}
# 461
static struct Cyc_RgnOrder_RgnPO*Cyc_Tcenv_curr_rgnpo(struct Cyc_Tcenv_Tenv*te){
# 463
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0)return 0;return le->region_order;}
# 471
void Cyc_Tcenv_check_rgn_accessible(struct Cyc_Tcenv_Tenv*te,unsigned loc,void*rgn){
# 473
struct Cyc_Tcenv_Fenv*fe=({({struct Cyc_Tcenv_Tenv*_tmp15B=te;Cyc_Tcenv_get_fenv(_tmp15B,({const char*_tmp9A="check_rgn_accessible";_tag_fat(_tmp9A,sizeof(char),21U);}));});});
if(({Cyc_Tcutil_region_in_effect(0,rgn,fe->capability);})||({Cyc_Tcutil_region_in_effect(1,rgn,fe->capability);}))
# 477
return;
# 474
if(({int(*_tmp93)(struct Cyc_RgnOrder_RgnPO*,void*eff1,void*eff2)=Cyc_RgnOrder_eff_outlives_eff;struct Cyc_RgnOrder_RgnPO*_tmp94=fe->region_order;void*_tmp95=({Cyc_Absyn_access_eff(rgn);});void*_tmp96=fe->capability;_tmp93(_tmp94,_tmp95,_tmp96);}))
# 481
return;
# 474
({struct Cyc_String_pa_PrintArg_struct _tmp99=({struct Cyc_String_pa_PrintArg_struct _tmp110;_tmp110.tag=0U,({
# 483
struct _fat_ptr _tmp15C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(rgn);}));_tmp110.f1=_tmp15C;});_tmp110;});void*_tmp97[1U];_tmp97[0]=& _tmp99;({unsigned _tmp15E=loc;struct _fat_ptr _tmp15D=({const char*_tmp98="Expression accesses unavailable region %s";_tag_fat(_tmp98,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp15E,_tmp15D,_tag_fat(_tmp97,sizeof(void*),1U));});});}struct _tuple16{void*f1;void*f2;struct Cyc_RgnOrder_RgnPO*f3;unsigned f4;};
# 488
void Cyc_Tcenv_check_effect_accessible(struct Cyc_Tcenv_Tenv*te,unsigned loc,void*eff){
# 490
struct Cyc_Tcenv_Fenv*_stmttmp0=({({struct Cyc_Tcenv_Tenv*_tmp15F=te;Cyc_Tcenv_get_fenv(_tmp15F,({const char*_tmpA1="check_effect_accessible";_tag_fat(_tmpA1,sizeof(char),24U);}));});});struct Cyc_Tcenv_SharedFenv*_tmp9E;struct Cyc_RgnOrder_RgnPO*_tmp9D;void*_tmp9C;_LL1: _tmp9C=_stmttmp0->capability;_tmp9D=_stmttmp0->region_order;_tmp9E=_stmttmp0->shared;_LL2: {void*capability=_tmp9C;struct Cyc_RgnOrder_RgnPO*ro=_tmp9D;struct Cyc_Tcenv_SharedFenv*shared=_tmp9E;
# 492
if(({Cyc_Tcutil_subset_effect(0,eff,capability);}))
return;
# 492
if(({Cyc_RgnOrder_eff_outlives_eff(ro,eff,capability);}))
# 495
return;
# 492
({struct Cyc_List_List*_tmp161=({struct Cyc_List_List*_tmpA0=_cycalloc(sizeof(*_tmpA0));
# 497
({struct _tuple16*_tmp160=({struct _tuple16*_tmp9F=_cycalloc(sizeof(*_tmp9F));_tmp9F->f1=capability,_tmp9F->f2=eff,_tmp9F->f3=ro,_tmp9F->f4=loc;_tmp9F;});_tmpA0->hd=_tmp160;}),_tmpA0->tl=shared->delayed_effect_checks;_tmpA0;});
# 492
shared->delayed_effect_checks=_tmp161;});}}
# 500
void Cyc_Tcenv_check_delayed_effects(struct Cyc_Tcenv_Tenv*te){
# 502
struct Cyc_List_List*checks=(({({struct Cyc_Tcenv_Tenv*_tmp162=te;Cyc_Tcenv_get_fenv(_tmp162,({const char*_tmpAB="check_delayed_constraints";_tag_fat(_tmpAB,sizeof(char),26U);}));});})->shared)->delayed_effect_checks;
for(0;checks != 0;checks=checks->tl){
# 505
struct _tuple16*_stmttmp1=(struct _tuple16*)checks->hd;unsigned _tmpA6;struct Cyc_RgnOrder_RgnPO*_tmpA5;void*_tmpA4;void*_tmpA3;_LL1: _tmpA3=_stmttmp1->f1;_tmpA4=_stmttmp1->f2;_tmpA5=_stmttmp1->f3;_tmpA6=_stmttmp1->f4;_LL2: {void*capability=_tmpA3;void*eff=_tmpA4;struct Cyc_RgnOrder_RgnPO*rgn_order=_tmpA5;unsigned loc=_tmpA6;
if(({Cyc_Tcutil_subset_effect(1,eff,capability);}))continue;if(({Cyc_RgnOrder_eff_outlives_eff(rgn_order,eff,capability);}))
continue;
# 506
({struct Cyc_String_pa_PrintArg_struct _tmpA9=({struct Cyc_String_pa_PrintArg_struct _tmp112;_tmp112.tag=0U,({
# 508
struct _fat_ptr _tmp163=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(capability);}));_tmp112.f1=_tmp163;});_tmp112;});struct Cyc_String_pa_PrintArg_struct _tmpAA=({struct Cyc_String_pa_PrintArg_struct _tmp111;_tmp111.tag=0U,({struct _fat_ptr _tmp164=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(eff);}));_tmp111.f1=_tmp164;});_tmp111;});void*_tmpA7[2U];_tmpA7[0]=& _tmpA9,_tmpA7[1]=& _tmpAA;({unsigned _tmp166=loc;struct _fat_ptr _tmp165=({const char*_tmpA8="Capability \n%s\ndoes not cover function's effect\n%s";_tag_fat(_tmpA8,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp166,_tmp165,_tag_fat(_tmpA7,sizeof(void*),2U));});});}}}struct _tuple17{struct Cyc_RgnOrder_RgnPO*f1;struct Cyc_List_List*f2;unsigned f3;};
# 518
void Cyc_Tcenv_check_rgn_partial_order(struct Cyc_Tcenv_Tenv*te,unsigned loc,struct Cyc_List_List*po){
# 521
struct Cyc_Tcenv_Fenv*le=te->le;
if(le == 0){
# 525
for(0;po != 0;po=po->tl){
if(!({Cyc_Tcutil_subset_effect(1,(*((struct _tuple15*)po->hd)).f1,Cyc_Absyn_empty_effect);})|| !({int(*_tmpAD)(int may_constrain_evars,void*e1,void*e2)=Cyc_Tcutil_subset_effect;int _tmpAE=1;void*_tmpAF=({Cyc_Absyn_access_eff((*((struct _tuple15*)po->hd)).f2);});void*_tmpB0=Cyc_Absyn_empty_effect;_tmpAD(_tmpAE,_tmpAF,_tmpB0);}))
# 528
({void*_tmpB1=0U;({unsigned _tmp168=loc;struct _fat_ptr _tmp167=({const char*_tmpB2="the required region ordering is not satisfied here";_tag_fat(_tmpB2,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp168,_tmp167,_tag_fat(_tmpB1,sizeof(void*),0U));});});}
# 525
return;}{
# 522
struct Cyc_Tcenv_Fenv*_stmttmp2=le;struct Cyc_Tcenv_SharedFenv*_tmpB4;struct Cyc_RgnOrder_RgnPO*_tmpB3;_LL1: _tmpB3=_stmttmp2->region_order;_tmpB4=_stmttmp2->shared;_LL2: {struct Cyc_RgnOrder_RgnPO*ro=_tmpB3;struct Cyc_Tcenv_SharedFenv*shared=_tmpB4;
# 532
if(!({Cyc_RgnOrder_satisfies_constraints(ro,po,Cyc_Absyn_heap_rgn_type,0);}))
({struct Cyc_List_List*_tmp16A=({struct Cyc_List_List*_tmpB6=_cycalloc(sizeof(*_tmpB6));
({struct _tuple17*_tmp169=({struct _tuple17*_tmpB5=_cycalloc(sizeof(*_tmpB5));_tmpB5->f1=ro,_tmpB5->f2=po,_tmpB5->f3=loc;_tmpB5;});_tmpB6->hd=_tmp169;}),_tmpB6->tl=shared->delayed_constraint_checks;_tmpB6;});
# 533
shared->delayed_constraint_checks=_tmp16A;});}}}
# 537
void Cyc_Tcenv_check_delayed_constraints(struct Cyc_Tcenv_Tenv*te){
struct Cyc_List_List*checks=(({({struct Cyc_Tcenv_Tenv*_tmp16B=te;Cyc_Tcenv_get_fenv(_tmp16B,({const char*_tmpBD="check_delayed_constraints";_tag_fat(_tmpBD,sizeof(char),26U);}));});})->shared)->delayed_constraint_checks;
# 540
for(0;checks != 0;checks=checks->tl){
struct _tuple17*_stmttmp3=(struct _tuple17*)checks->hd;unsigned _tmpBA;struct Cyc_List_List*_tmpB9;struct Cyc_RgnOrder_RgnPO*_tmpB8;_LL1: _tmpB8=_stmttmp3->f1;_tmpB9=_stmttmp3->f2;_tmpBA=_stmttmp3->f3;_LL2: {struct Cyc_RgnOrder_RgnPO*rgn_order=_tmpB8;struct Cyc_List_List*po=_tmpB9;unsigned loc=_tmpBA;
if(!({Cyc_RgnOrder_satisfies_constraints(rgn_order,po,Cyc_Absyn_heap_rgn_type,1);}))
({void*_tmpBB=0U;({unsigned _tmp16D=loc;struct _fat_ptr _tmp16C=({const char*_tmpBC="the required region ordering is not satisfied here";_tag_fat(_tmpBC,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp16D,_tmp16C,_tag_fat(_tmpBB,sizeof(void*),0U));});});}}}
# 547
static struct Cyc_Tcenv_SharedFenv*Cyc_Tcenv_new_shared_fenv(void*ret){
return({struct Cyc_Tcenv_SharedFenv*_tmpBF=_cycalloc(sizeof(*_tmpBF));_tmpBF->return_typ=ret,_tmpBF->delayed_effect_checks=0,_tmpBF->delayed_constraint_checks=0;_tmpBF;});}
# 547
void*Cyc_Tcenv_new_capability(void*param_rgn,struct Cyc_Absyn_Fndecl*fd,void*fntype){
# 557
return({void*(*_tmpC1)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmpC2=({struct Cyc_List_List*_tmpC4=_cycalloc(sizeof(*_tmpC4));({void*_tmp16F=({Cyc_Absyn_access_eff(param_rgn);});_tmpC4->hd=_tmp16F;}),({
struct Cyc_List_List*_tmp16E=({struct Cyc_List_List*_tmpC3=_cycalloc(sizeof(*_tmpC3));_tmpC3->hd=(void*)_check_null((fd->i).effect),_tmpC3->tl=0;_tmpC3;});_tmpC4->tl=_tmp16E;});_tmpC4;});_tmpC1(_tmpC2);});}
# 547
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_fenv_in_env(unsigned loc,struct Cyc_Tcenv_Tenv*te,struct Cyc_Absyn_Fndecl*fd,void*ftyp){
# 564
struct Cyc_Absyn_Tvar*rgn0=({struct Cyc_Absyn_Tvar*_tmpD3=_cycalloc(sizeof(*_tmpD3));
({struct _fat_ptr*_tmp173=({struct _fat_ptr*_tmpD2=_cycalloc(sizeof(*_tmpD2));({struct _fat_ptr _tmp172=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpD1=({struct Cyc_String_pa_PrintArg_struct _tmp113;_tmp113.tag=0U,_tmp113.f1=(struct _fat_ptr)((struct _fat_ptr)*(*fd->name).f2);_tmp113;});void*_tmpCF[1U];_tmpCF[0]=& _tmpD1;({struct _fat_ptr _tmp171=({const char*_tmpD0="`%s";_tag_fat(_tmpD0,sizeof(char),4U);});Cyc_aprintf(_tmp171,_tag_fat(_tmpCF,sizeof(void*),1U));});});*_tmpD2=_tmp172;});_tmpD2;});_tmpD3->name=_tmp173;}),({
int _tmp170=({Cyc_Tcutil_new_tvar_id();});_tmpD3->identity=_tmp170;}),_tmpD3->kind=(void*)& Cyc_Tcenv_rgn_kb;_tmpD3;});
struct Cyc_List_List*tvs=({struct Cyc_List_List*_tmpCE=_cycalloc(sizeof(*_tmpCE));_tmpCE->hd=rgn0,_tmpCE->tl=(fd->i).tvars;_tmpCE;});
({Cyc_Tcutil_check_unique_tvars(loc,tvs);});{
# 570
struct Cyc_RgnOrder_RgnPO*rgn_po=({Cyc_RgnOrder_initial_fn_po((fd->i).tvars,(fd->i).rgn_po,(void*)_check_null((fd->i).effect),rgn0,loc);});
void*param_rgn=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmpCD=_cycalloc(sizeof(*_tmpCD));_tmpCD->tag=2U,_tmpCD->f1=rgn0;_tmpCD;});
{struct Cyc_List_List*vds=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v;for(0;vds != 0;vds=vds->tl){
((struct Cyc_Absyn_Vardecl*)vds->hd)->rgn=param_rgn;}}
if((fd->i).cyc_varargs != 0){
struct Cyc_Absyn_VarargInfo _stmttmp4=*((struct Cyc_Absyn_VarargInfo*)_check_null((fd->i).cyc_varargs));int _tmpC9;void*_tmpC8;struct Cyc_Absyn_Tqual _tmpC7;struct _fat_ptr*_tmpC6;_LL1: _tmpC6=_stmttmp4.name;_tmpC7=_stmttmp4.tq;_tmpC8=_stmttmp4.type;_tmpC9=_stmttmp4.inject;_LL2: {struct _fat_ptr*n=_tmpC6;struct Cyc_Absyn_Tqual tq=_tmpC7;void*t=_tmpC8;int i=_tmpC9;
# 577
struct Cyc_List_List*vds=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v;for(0;vds != 0;vds=vds->tl){
struct Cyc_Absyn_Vardecl*vd=(struct Cyc_Absyn_Vardecl*)vds->hd;
if(({Cyc_strptrcmp((*vd->name).f2,(struct _fat_ptr*)_check_null(n));})== 0){
({void*_tmp174=({Cyc_Absyn_fatptr_type(t,param_rgn,tq,Cyc_Absyn_false_type);});vd->type=_tmp174;});
break;}}}}{
# 574
struct Cyc_Tcenv_Fenv*fenv=({struct Cyc_Tcenv_Fenv*_tmpCC=_cycalloc(sizeof(*_tmpCC));
# 586
({struct Cyc_Tcenv_SharedFenv*_tmp17B=({Cyc_Tcenv_new_shared_fenv((fd->i).ret_type);});_tmpCC->shared=_tmp17B;}),_tmpCC->type_vars=tvs,_tmpCC->region_order=rgn_po,_tmpCC->ctrl_env=0,({
# 590
void*_tmp17A=({Cyc_Tcenv_new_capability(param_rgn,fd,ftyp);});_tmpCC->capability=_tmp17A;}),_tmpCC->curr_rgn=param_rgn,
# 592
(_tmpCC->flags).in_new=Cyc_Tcenv_NoneNew,(_tmpCC->flags).in_notreadctxt=0,(_tmpCC->flags).in_lhs=0,(_tmpCC->flags).abstract_ok=0,(_tmpCC->flags).in_stmt_exp=0,({
int _tmp179=({Cyc_Tcenv_has_io_eff_annot(ftyp);});(_tmpCC->flags).is_io_eff_annot=_tmp179;}),({
unsigned char _tmp178=(unsigned char)({Cyc_Tcenv_throw_annot(ftyp);});(_tmpCC->flags).is_throw_annot=_tmp178;}),({
unsigned char _tmp177=(unsigned char)({Cyc_Absyn_is_reentrant((fd->i).reentrant);});(_tmpCC->flags).is_reentrant=_tmp177;}),({
# 598
struct Cyc_List_List**_tmp176=({struct Cyc_List_List**_tmpCB=_cycalloc(sizeof(*_tmpCB));*_tmpCB=0;_tmpCB;});_tmpCC->throws=_tmp176;}),({
struct Cyc_List_List*_tmp175=({Cyc_Tcenv_fn_throw_annot(ftyp);});_tmpCC->annot=_tmp175;}),_tmpCC->loc=loc;_tmpCC;});
# 602
return({struct Cyc_Tcenv_Tenv*_tmpCA=_cycalloc(sizeof(*_tmpCA));_tmpCA->ns=te->ns,_tmpCA->ae=te->ae,_tmpCA->le=fenv,_tmpCA->allow_valueof=0,_tmpCA->in_extern_c_include=0,_tmpCA->in_tempest=te->in_tempest,_tmpCA->tempest_generalize=te->tempest_generalize;_tmpCA;});}}}
# 547
int Cyc_Tcenv_is_reentrant_fun(struct Cyc_Tcenv_Tenv*tenv){
# 608
struct Cyc_Tcenv_Fenv*le=tenv->le;
if(le == 0)return 0;return(int)(le->flags).is_reentrant;}
# 547
struct Cyc_Tcenv_Fenv*Cyc_Tcenv_nested_fenv(unsigned loc,struct Cyc_Tcenv_Fenv*old_fenv,struct Cyc_Absyn_Fndecl*fd,void*ftyp){
# 616
struct Cyc_Tcenv_SharedFenv*_tmpD8;struct Cyc_List_List*_tmpD7;struct Cyc_RgnOrder_RgnPO*_tmpD6;_LL1: _tmpD6=old_fenv->region_order;_tmpD7=old_fenv->type_vars;_tmpD8=old_fenv->shared;_LL2: {struct Cyc_RgnOrder_RgnPO*rgn_po=_tmpD6;struct Cyc_List_List*type_vars=_tmpD7;struct Cyc_Tcenv_SharedFenv*shared=_tmpD8;
# 618
struct Cyc_Absyn_Tvar*rgn0=({struct Cyc_Absyn_Tvar*_tmpEA=_cycalloc(sizeof(*_tmpEA));
({struct _fat_ptr*_tmp17F=({struct _fat_ptr*_tmpE9=_cycalloc(sizeof(*_tmpE9));({struct _fat_ptr _tmp17E=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpE8=({struct Cyc_String_pa_PrintArg_struct _tmp114;_tmp114.tag=0U,_tmp114.f1=(struct _fat_ptr)((struct _fat_ptr)*(*fd->name).f2);_tmp114;});void*_tmpE6[1U];_tmpE6[0]=& _tmpE8;({struct _fat_ptr _tmp17D=({const char*_tmpE7="`%s";_tag_fat(_tmpE7,sizeof(char),4U);});Cyc_aprintf(_tmp17D,_tag_fat(_tmpE6,sizeof(void*),1U));});});*_tmpE9=_tmp17E;});_tmpE9;});_tmpEA->name=_tmp17F;}),({
int _tmp17C=({Cyc_Tcutil_new_tvar_id();});_tmpEA->identity=_tmp17C;}),_tmpEA->kind=(void*)& Cyc_Tcenv_rgn_kb;_tmpEA;});
{struct Cyc_List_List*tvars=(fd->i).tvars;for(0;tvars != 0;tvars=tvars->tl){
struct Cyc_Absyn_Kind*_stmttmp5=({Cyc_Tcutil_tvar_kind((struct Cyc_Absyn_Tvar*)tvars->hd,& Cyc_Tcutil_bk);});enum Cyc_Absyn_AliasQual _tmpDA;enum Cyc_Absyn_KindQual _tmpD9;_LL4: _tmpD9=_stmttmp5->kind;_tmpDA=_stmttmp5->aliasqual;_LL5: {enum Cyc_Absyn_KindQual k=_tmpD9;enum Cyc_Absyn_AliasQual a=_tmpDA;
if((int)k == (int)4U){
if((int)a == (int)0U)
rgn_po=({Cyc_RgnOrder_add_unordered(rgn_po,(struct Cyc_Absyn_Tvar*)tvars->hd);});else{
# 627
({void*_tmpDB=0U;({int(*_tmp181)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpDD)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpDD;});struct _fat_ptr _tmp180=({const char*_tmpDC="non-intuitionistic tvar in nested_fenv";_tag_fat(_tmpDC,sizeof(char),39U);});_tmp181(_tmp180,_tag_fat(_tmpDB,sizeof(void*),0U));});});}}}}}
# 629
rgn_po=({Cyc_RgnOrder_add_youngest(rgn_po,rgn0,0);});
{struct Cyc_List_List*po2=(fd->i).rgn_po;for(0;po2 != 0;po2=po2->tl){
rgn_po=({Cyc_RgnOrder_add_outlives_constraint(rgn_po,(*((struct _tuple15*)po2->hd)).f1,(*((struct _tuple15*)po2->hd)).f2,loc);});}}{
struct Cyc_List_List*tvs=({struct Cyc_List_List*_tmpE5=_cycalloc(sizeof(*_tmpE5));_tmpE5->hd=rgn0,({struct Cyc_List_List*_tmp182=({Cyc_List_append((fd->i).tvars,type_vars);});_tmpE5->tl=_tmp182;});_tmpE5;});
({Cyc_Tcutil_check_unique_tvars(loc,tvs);});{
void*param_rgn=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmpE4=_cycalloc(sizeof(*_tmpE4));_tmpE4->tag=2U,_tmpE4->f1=rgn0;_tmpE4;});
{struct Cyc_List_List*vds=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v;for(0;vds != 0;vds=vds->tl){
((struct Cyc_Absyn_Vardecl*)vds->hd)->rgn=param_rgn;}}
if((fd->i).cyc_varargs != 0){
struct Cyc_Absyn_VarargInfo _stmttmp6=*((struct Cyc_Absyn_VarargInfo*)_check_null((fd->i).cyc_varargs));int _tmpE1;void*_tmpE0;struct Cyc_Absyn_Tqual _tmpDF;struct _fat_ptr*_tmpDE;_LL7: _tmpDE=_stmttmp6.name;_tmpDF=_stmttmp6.tq;_tmpE0=_stmttmp6.type;_tmpE1=_stmttmp6.inject;_LL8: {struct _fat_ptr*n=_tmpDE;struct Cyc_Absyn_Tqual tq=_tmpDF;void*t=_tmpE0;int i=_tmpE1;
# 640
struct Cyc_List_List*vds=(struct Cyc_List_List*)((struct Cyc_Core_Opt*)_check_null(fd->param_vardecls))->v;for(0;vds != 0;vds=vds->tl){
struct Cyc_Absyn_Vardecl*vd=(struct Cyc_Absyn_Vardecl*)vds->hd;
if(({Cyc_strptrcmp((*vd->name).f2,(struct _fat_ptr*)_check_null(n));})== 0){
({void*_tmp183=({Cyc_Absyn_fatptr_type(t,param_rgn,tq,Cyc_Absyn_false_type);});vd->type=_tmp183;});
break;}}}}
# 637
return({struct Cyc_Tcenv_Fenv*_tmpE3=_cycalloc(sizeof(*_tmpE3));
# 649
({struct Cyc_Tcenv_SharedFenv*_tmp18A=({Cyc_Tcenv_new_shared_fenv((fd->i).ret_type);});_tmpE3->shared=_tmp18A;}),_tmpE3->type_vars=tvs,_tmpE3->region_order=rgn_po,_tmpE3->ctrl_env=0,({
# 653
void*_tmp189=({Cyc_Tcenv_new_capability(param_rgn,fd,ftyp);});_tmpE3->capability=_tmp189;}),_tmpE3->curr_rgn=param_rgn,
# 655
(_tmpE3->flags).in_new=Cyc_Tcenv_NoneNew,(_tmpE3->flags).in_notreadctxt=0,(_tmpE3->flags).in_lhs=0,(_tmpE3->flags).abstract_ok=0,(_tmpE3->flags).in_stmt_exp=0,({
int _tmp188=({Cyc_Tcenv_has_io_eff_annot(ftyp);});(_tmpE3->flags).is_io_eff_annot=_tmp188;}),({
unsigned char _tmp187=(unsigned char)({Cyc_Tcenv_throw_annot(ftyp);});(_tmpE3->flags).is_throw_annot=_tmp187;}),({
unsigned char _tmp186=(unsigned char)({Cyc_Absyn_is_reentrant((fd->i).reentrant);});(_tmpE3->flags).is_reentrant=_tmp186;}),({
# 661
struct Cyc_List_List**_tmp185=({struct Cyc_List_List**_tmpE2=_cycalloc(sizeof(*_tmpE2));*_tmpE2=0;_tmpE2;});_tmpE3->throws=_tmp185;}),({
struct Cyc_List_List*_tmp184=({Cyc_Tcenv_fn_throw_annot(ftyp);});_tmpE3->annot=_tmp184;}),_tmpE3->loc=loc;_tmpE3;});}}}}
# 547
struct Cyc_Tcenv_Tenv*Cyc_Tcenv_new_bogus_fenv_in_env(struct Cyc_Tcenv_Tenv*te,void*ret_type,struct Cyc_List_List*args){
# 669
struct Cyc_Absyn_Tvar*rgn0=({struct Cyc_Absyn_Tvar*_tmpFC=_cycalloc(sizeof(*_tmpFC));({struct _fat_ptr*_tmp18D=({struct _fat_ptr*_tmpFB=_cycalloc(sizeof(*_tmpFB));({struct _fat_ptr _tmp18C=({const char*_tmpFA="bogus";_tag_fat(_tmpFA,sizeof(char),6U);});*_tmpFB=_tmp18C;});_tmpFB;});_tmpFC->name=_tmp18D;}),({int _tmp18B=({Cyc_Tcutil_new_tvar_id();});_tmpFC->identity=_tmp18B;}),_tmpFC->kind=(void*)& Cyc_Tcenv_rgn_kb;_tmpFC;});
struct Cyc_List_List*tvs=({struct Cyc_List_List*_tmpF9=_cycalloc(sizeof(*_tmpF9));_tmpF9->hd=rgn0,_tmpF9->tl=0;_tmpF9;});
struct Cyc_RgnOrder_RgnPO*rgn_po=({struct Cyc_RgnOrder_RgnPO*(*_tmpF3)(struct Cyc_List_List*tvs,struct Cyc_List_List*po,void*effect,struct Cyc_Absyn_Tvar*fst_rgn,unsigned)=Cyc_RgnOrder_initial_fn_po;struct Cyc_List_List*_tmpF4=0;struct Cyc_List_List*_tmpF5=0;void*_tmpF6=({Cyc_Absyn_join_eff(0);});struct Cyc_Absyn_Tvar*_tmpF7=rgn0;unsigned _tmpF8=0U;_tmpF3(_tmpF4,_tmpF5,_tmpF6,_tmpF7,_tmpF8);});
void*param_rgn=(void*)({struct Cyc_Absyn_VarType_Absyn_Type_struct*_tmpF2=_cycalloc(sizeof(*_tmpF2));_tmpF2->tag=2U,_tmpF2->f1=rgn0;_tmpF2;});
struct Cyc_Tcenv_Fenv*fenv=({struct Cyc_Tcenv_Fenv*_tmpF1=_cycalloc(sizeof(*_tmpF1));
({struct Cyc_Tcenv_SharedFenv*_tmp191=({Cyc_Tcenv_new_shared_fenv(ret_type);});_tmpF1->shared=_tmp191;}),_tmpF1->type_vars=tvs,_tmpF1->region_order=rgn_po,_tmpF1->ctrl_env=0,({
# 678
void*_tmp190=({void*(*_tmpED)(struct Cyc_List_List*)=Cyc_Absyn_join_eff;struct Cyc_List_List*_tmpEE=({struct Cyc_List_List*_tmpEF=_cycalloc(sizeof(*_tmpEF));({void*_tmp18F=({Cyc_Absyn_access_eff(param_rgn);});_tmpEF->hd=_tmp18F;}),_tmpEF->tl=0;_tmpEF;});_tmpED(_tmpEE);});_tmpF1->capability=_tmp190;}),_tmpF1->curr_rgn=param_rgn,
# 680
(_tmpF1->flags).in_new=Cyc_Tcenv_NoneNew,(_tmpF1->flags).in_notreadctxt=0,(_tmpF1->flags).in_lhs=0,(_tmpF1->flags).abstract_ok=0,(_tmpF1->flags).in_stmt_exp=0,(_tmpF1->flags).is_io_eff_annot=0,(_tmpF1->flags).is_throw_annot='\000',(_tmpF1->flags).is_reentrant='\000',({
# 683
struct Cyc_List_List**_tmp18E=({struct Cyc_List_List**_tmpF0=_cycalloc(sizeof(*_tmpF0));*_tmpF0=0;_tmpF0;});_tmpF1->throws=_tmp18E;}),_tmpF1->annot=0,_tmpF1->loc=0U;_tmpF1;});
# 687
return({struct Cyc_Tcenv_Tenv*_tmpEC=_cycalloc(sizeof(*_tmpEC));_tmpEC->ns=te->ns,_tmpEC->ae=te->ae,_tmpEC->le=fenv,_tmpEC->allow_valueof=1,_tmpEC->in_extern_c_include=te->in_extern_c_include,_tmpEC->in_tempest=te->in_tempest,_tmpEC->tempest_generalize=te->tempest_generalize;_tmpEC;});}
# 547
int Cyc_Tcenv_rgn_outlives_rgn(struct Cyc_Tcenv_Tenv*te,void*rgn1,void*rgn2){
# 695
return({int(*_tmpFE)(struct Cyc_RgnOrder_RgnPO*,void*rgn1,void*rgn2)=Cyc_RgnOrder_rgn_outlives_rgn;struct Cyc_RgnOrder_RgnPO*_tmpFF=({Cyc_Tcenv_curr_rgnpo(te);});void*_tmp100=rgn1;void*_tmp101=rgn2;_tmpFE(_tmpFF,_tmp100,_tmp101);});}
# 698
static struct Cyc_Absyn_FnInfo Cyc_Tcenv_fn_type(void*t){
# 700
void*_tmp103=t;struct Cyc_Absyn_FnInfo _tmp104;if(((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp103)->tag == 5U){_LL1: _tmp104=((struct Cyc_Absyn_FnType_Absyn_Type_struct*)_tmp103)->f1;_LL2: {struct Cyc_Absyn_FnInfo i=_tmp104;
# 702
return i;}}else{_LL3: _LL4:
({({int(*_tmp192)(struct _fat_ptr msg)=({int(*_tmp105)(struct _fat_ptr msg)=(int(*)(struct _fat_ptr msg))Cyc_Tcenv_env_err;_tmp105;});_tmp192(({const char*_tmp106="Tcenv::fn_type";_tag_fat(_tmp106,sizeof(char),15U);}));});});}_LL0:;}
# 709
static struct Cyc_List_List*Cyc_Tcenv_fn_throw_annot(void*t){
# 711
return({Cyc_Tcenv_fn_type(t);}).throws;}
# 714
static int Cyc_Tcenv_has_io_eff_annot(void*t){
# 716
return({Cyc_Tcenv_fn_type(t);}).ieffect != 0;}
# 719
static char Cyc_Tcenv_throw_annot(void*t){
# 721
struct Cyc_List_List*x=({Cyc_Tcenv_fn_throw_annot(t);});
if(x == 0)return'\000';else{
if(({Cyc_Absyn_is_nothrow(x);}))return'\001';else{
if(({Cyc_Absyn_is_throwsany(x);}))return'\003';else{
return'\002';}}}}
