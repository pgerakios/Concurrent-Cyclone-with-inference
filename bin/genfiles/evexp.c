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
int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};
# 276
struct Cyc_List_List*Cyc_List_rzip(struct _RegionHandle*r1,struct _RegionHandle*r2,struct Cyc_List_List*x,struct Cyc_List_List*y);struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73 "cycboot.h"
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};
# 50 "string.h"
int Cyc_strptrcmp(struct _fat_ptr*s1,struct _fat_ptr*s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 564
union Cyc_Absyn_Cnst Cyc_Absyn_Char_c(enum Cyc_Absyn_Sign,char);
# 566
union Cyc_Absyn_Cnst Cyc_Absyn_Short_c(enum Cyc_Absyn_Sign,short);
union Cyc_Absyn_Cnst Cyc_Absyn_Int_c(enum Cyc_Absyn_Sign,int);
union Cyc_Absyn_Cnst Cyc_Absyn_LongLong_c(enum Cyc_Absyn_Sign,long long);
# 574
enum Cyc_Absyn_Primop{Cyc_Absyn_Plus =0U,Cyc_Absyn_Times =1U,Cyc_Absyn_Minus =2U,Cyc_Absyn_Div =3U,Cyc_Absyn_Mod =4U,Cyc_Absyn_Eq =5U,Cyc_Absyn_Neq =6U,Cyc_Absyn_Gt =7U,Cyc_Absyn_Lt =8U,Cyc_Absyn_Gte =9U,Cyc_Absyn_Lte =10U,Cyc_Absyn_Not =11U,Cyc_Absyn_Bitnot =12U,Cyc_Absyn_Bitand =13U,Cyc_Absyn_Bitor =14U,Cyc_Absyn_Bitxor =15U,Cyc_Absyn_Bitlshift =16U,Cyc_Absyn_Bitlrshift =17U,Cyc_Absyn_Numelts =18U};
# 581
enum Cyc_Absyn_Incrementor{Cyc_Absyn_PreInc =0U,Cyc_Absyn_PostInc =1U,Cyc_Absyn_PreDec =2U,Cyc_Absyn_PostDec =3U};struct Cyc_Absyn_VarargCallInfo{int num_varargs;struct Cyc_List_List*injectors;struct Cyc_Absyn_VarargInfo*vai;};struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct{int tag;unsigned f1;};
# 599
enum Cyc_Absyn_Coercion{Cyc_Absyn_Unknown_coercion =0U,Cyc_Absyn_No_coercion =1U,Cyc_Absyn_Null_to_NonNull =2U,Cyc_Absyn_Other_coercion =3U};struct Cyc_Absyn_MallocInfo{int is_calloc;struct Cyc_Absyn_Exp*rgn;void**elt_type;struct Cyc_Absyn_Exp*num_elts;int fat_result;int inline_call;};struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct{int tag;union Cyc_Absyn_Cnst f1;};struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Pragma_e_Absyn_Raw_exp_struct{int tag;struct _fat_ptr f1;};struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct{int tag;enum Cyc_Absyn_Primop f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;enum Cyc_Absyn_Incrementor f2;};struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct _tuple8{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;struct Cyc_Absyn_VarargCallInfo*f3;int f4;struct _tuple8*f5;};struct Cyc_Absyn_Throw_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;int f2;};struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Exp*f2;int f3;enum Cyc_Absyn_Coercion f4;};struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;int f3;int f4;};struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_Tuple_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct _tuple9{struct _fat_ptr*f1;struct Cyc_Absyn_Tqual f2;void*f3;};struct Cyc_Absyn_CompoundLit_e_Absyn_Raw_exp_struct{int tag;struct _tuple9*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Exp*f2;struct Cyc_Absyn_Exp*f3;int f4;};struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;void*f2;int f3;};struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct Cyc_Absyn_Aggrdecl*f4;};struct Cyc_Absyn_AnonStruct_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Datatype_e_Absyn_Raw_exp_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Datatypedecl*f2;struct Cyc_Absyn_Datatypefield*f3;};struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;struct _tuple8*f3;};struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_MallocInfo f1;};struct Cyc_Absyn_Swap_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Exp*f2;};struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Core_Opt*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Stmt*f1;};struct Cyc_Absyn_Tagcheck_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _fat_ptr*f2;};struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct{int tag;void*f1;};struct Cyc_Absyn_Asm_e_Absyn_Raw_exp_struct{int tag;int f1;struct _fat_ptr f2;struct Cyc_List_List*f3;struct Cyc_List_List*f4;struct Cyc_List_List*f5;};struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Exp{void*topt;void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Skip_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;struct Cyc_Absyn_Stmt*f3;};struct _tuple10{struct Cyc_Absyn_Exp*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct{int tag;struct _tuple10 f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Break_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Continue_s_Absyn_Raw_stmt_struct{int tag;};struct Cyc_Absyn_Goto_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;};struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct _tuple10 f2;struct _tuple10 f3;struct Cyc_Absyn_Stmt*f4;};struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Exp*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Fallthru_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_List_List*f1;struct Cyc_Absyn_Switch_clause**f2;};struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Decl*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct{int tag;struct _fat_ptr*f1;struct Cyc_Absyn_Stmt*f2;};struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct _tuple10 f2;};struct Cyc_Absyn_TryCatch_s_Absyn_Raw_stmt_struct{int tag;struct Cyc_Absyn_Stmt*f1;struct Cyc_List_List*f2;void*f3;};struct Cyc_Absyn_Stmt{void*r;unsigned loc;void*annot;};struct Cyc_Absyn_Wild_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Var_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_AliasVar_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Reference_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Vardecl*f1;struct Cyc_Absyn_Pat*f2;};struct Cyc_Absyn_TagInt_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;};struct Cyc_Absyn_Tuple_p_Absyn_Raw_pat_struct{int tag;struct Cyc_List_List*f1;int f2;};struct Cyc_Absyn_Pointer_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Pat*f1;};struct Cyc_Absyn_Aggr_p_Absyn_Raw_pat_struct{int tag;union Cyc_Absyn_AggrInfo*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Datatype_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;struct Cyc_Absyn_Datatypefield*f2;struct Cyc_List_List*f3;int f4;};struct Cyc_Absyn_Null_p_Absyn_Raw_pat_struct{int tag;};struct Cyc_Absyn_Int_p_Absyn_Raw_pat_struct{int tag;enum Cyc_Absyn_Sign f1;int f2;};struct Cyc_Absyn_Char_p_Absyn_Raw_pat_struct{int tag;char f1;};struct Cyc_Absyn_Float_p_Absyn_Raw_pat_struct{int tag;struct _fat_ptr f1;int f2;};struct Cyc_Absyn_Enum_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_AnonEnum_p_Absyn_Raw_pat_struct{int tag;void*f1;struct Cyc_Absyn_Enumfield*f2;};struct Cyc_Absyn_UnknownId_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_UnknownCall_p_Absyn_Raw_pat_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;int f3;};struct Cyc_Absyn_Exp_p_Absyn_Raw_pat_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_Pat{void*r;void*topt;unsigned loc;};struct Cyc_Absyn_Switch_clause{struct Cyc_Absyn_Pat*pattern;struct Cyc_Core_Opt*pat_vars;struct Cyc_Absyn_Exp*where_clause;struct Cyc_Absyn_Stmt*body;unsigned loc;};struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct{int tag;struct _tuple0*f1;};struct Cyc_Absyn_Global_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Funname_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Param_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Local_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Pat_b_Absyn_Binding_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Vardecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;unsigned varloc;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*initializer;void*rgn;struct Cyc_List_List*attributes;int escapes;};struct Cyc_Absyn_Fndecl{enum Cyc_Absyn_Scope sc;int is_inline;struct _tuple0*name;struct Cyc_Absyn_Stmt*body;struct Cyc_Absyn_FnInfo i;void*cached_type;struct Cyc_Core_Opt*param_vardecls;struct Cyc_Absyn_Vardecl*fn_vardecl;};struct Cyc_Absyn_Aggrfield{struct _fat_ptr*name;struct Cyc_Absyn_Tqual tq;void*type;struct Cyc_Absyn_Exp*width;struct Cyc_List_List*attributes;struct Cyc_Absyn_Exp*requires_clause;};struct Cyc_Absyn_AggrdeclImpl{struct Cyc_List_List*exist_vars;struct Cyc_List_List*rgn_po;struct Cyc_List_List*fields;int tagged;};struct Cyc_Absyn_Aggrdecl{enum Cyc_Absyn_AggrKind kind;enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Absyn_AggrdeclImpl*impl;struct Cyc_List_List*attributes;int expected_mem_kind;};struct Cyc_Absyn_Datatypefield{struct _tuple0*name;struct Cyc_List_List*typs;unsigned loc;enum Cyc_Absyn_Scope sc;};struct Cyc_Absyn_Datatypedecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*fields;int is_extensible;};struct Cyc_Absyn_Enumfield{struct _tuple0*name;struct Cyc_Absyn_Exp*tag;unsigned loc;};struct Cyc_Absyn_Enumdecl{enum Cyc_Absyn_Scope sc;struct _tuple0*name;struct Cyc_Core_Opt*fields;};struct Cyc_Absyn_Typedefdecl{struct _tuple0*name;struct Cyc_Absyn_Tqual tq;struct Cyc_List_List*tvs;struct Cyc_Core_Opt*kind;void*defn;struct Cyc_List_List*atts;int extern_c;};struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Vardecl*f1;};struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Fndecl*f1;};struct Cyc_Absyn_Let_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Pat*f1;struct Cyc_Core_Opt*f2;struct Cyc_Absyn_Exp*f3;void*f4;};struct Cyc_Absyn_Letv_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct Cyc_Absyn_Region_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Tvar*f1;struct Cyc_Absyn_Vardecl*f2;struct Cyc_Absyn_Exp*f3;};struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Aggrdecl*f1;};struct Cyc_Absyn_Datatype_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Datatypedecl*f1;};struct Cyc_Absyn_Enum_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Enumdecl*f1;};struct Cyc_Absyn_Typedef_d_Absyn_Raw_decl_struct{int tag;struct Cyc_Absyn_Typedefdecl*f1;};struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct{int tag;struct _fat_ptr*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct{int tag;struct _tuple0*f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;};struct _tuple11{unsigned f1;struct Cyc_List_List*f2;};struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct{int tag;struct Cyc_List_List*f1;struct Cyc_List_List*f2;struct Cyc_List_List*f3;struct _tuple11*f4;};struct Cyc_Absyn_Porton_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Portoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempeston_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Tempestoff_d_Absyn_Raw_decl_struct{int tag;};struct Cyc_Absyn_Decl{void*r;unsigned loc;};struct Cyc_Absyn_ArrayElement_Absyn_Designator_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Absyn_FieldName_Absyn_Designator_struct{int tag;struct _fat_ptr*f1;};extern char Cyc_Absyn_EmptyAnnot[11U];struct Cyc_Absyn_EmptyAnnot_Absyn_AbsynAnnot_struct{char*tag;};
# 984 "absyn.h"
extern void*Cyc_Absyn_uint_type;
# 1061
struct Cyc_Absyn_Exp*Cyc_Absyn_const_exp(union Cyc_Absyn_Cnst,unsigned);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 29 "unify.h"
int Cyc_Unify_unify(void*,void*);struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 30 "tcutil.h"
void*Cyc_Tcutil_impos(struct _fat_ptr,struct _fat_ptr);
void Cyc_Tcutil_terr(unsigned,struct _fat_ptr,struct _fat_ptr);
# 108
struct Cyc_Absyn_Kind*Cyc_Tcutil_tvar_kind(struct Cyc_Absyn_Tvar*,struct Cyc_Absyn_Kind*def);
# 110
void*Cyc_Tcutil_compress(void*);
# 141
extern struct Cyc_Absyn_Kind Cyc_Tcutil_bk;
# 186
int Cyc_Tcutil_typecmp(void*,void*);
# 191
void*Cyc_Tcutil_rsubstitute(struct _RegionHandle*,struct Cyc_List_List*,void*);struct _tuple12{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple12 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);
# 32
int Cyc_Evexp_c_can_eval(struct Cyc_Absyn_Exp*e);
# 41 "evexp.h"
int Cyc_Evexp_same_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
# 45
int Cyc_Evexp_const_exp_cmp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2);
# 47
int Cyc_Evexp_okay_szofarg(void*t);struct _tuple13{union Cyc_Absyn_Cnst f1;int f2;};
# 35 "evexp.cyc"
static struct _tuple13 Cyc_Evexp_eval_const_exp(struct Cyc_Absyn_Exp*);
# 37
static union Cyc_Absyn_Cnst Cyc_Evexp_promote_const(union Cyc_Absyn_Cnst cn){
union Cyc_Absyn_Cnst _tmp0=cn;short _tmp2;enum Cyc_Absyn_Sign _tmp1;char _tmp4;enum Cyc_Absyn_Sign _tmp3;switch((_tmp0.Short_c).tag){case 2U: _LL1: _tmp3=((_tmp0.Char_c).val).f1;_tmp4=((_tmp0.Char_c).val).f2;_LL2: {enum Cyc_Absyn_Sign sn=_tmp3;char c=_tmp4;
return({Cyc_Absyn_Int_c(sn,(int)c);});}case 4U: _LL3: _tmp1=((_tmp0.Short_c).val).f1;_tmp2=((_tmp0.Short_c).val).f2;_LL4: {enum Cyc_Absyn_Sign sn=_tmp1;short s=_tmp2;
# 41
return({Cyc_Absyn_Int_c(sn,(int)s);});}default: _LL5: _LL6:
 return cn;}_LL0:;}
# 46
struct _tuple12 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e){
struct _tuple13 _stmttmp0=({Cyc_Evexp_eval_const_exp(e);});int _tmp7;union Cyc_Absyn_Cnst _tmp6;_LL1: _tmp6=_stmttmp0.f1;_tmp7=_stmttmp0.f2;_LL2: {union Cyc_Absyn_Cnst cn=_tmp6;int known=_tmp7;
if(!known)
return({struct _tuple12 _tmpF1;_tmpF1.f1=0U,_tmpF1.f2=0;_tmpF1;});{
# 48
union Cyc_Absyn_Cnst _stmttmp1=({Cyc_Evexp_promote_const(cn);});union Cyc_Absyn_Cnst _tmp8=_stmttmp1;long long _tmp9;int _tmpA;switch((_tmp8.Null_c).tag){case 5U: _LL4: _tmpA=((_tmp8.Int_c).val).f2;_LL5: {int i=_tmpA;
# 51
return({struct _tuple12 _tmpF2;_tmpF2.f1=(unsigned)i,_tmpF2.f2=1;_tmpF2;});}case 6U: _LL6: _tmp9=((_tmp8.LongLong_c).val).f2;_LL7: {long long x=_tmp9;
# 54
unsigned long long y=(unsigned long long)x >> (unsigned long long)32;
if(y != (unsigned long long)-1 && y != (unsigned long long)0)
return({struct _tuple12 _tmpF3;_tmpF3.f1=0U,_tmpF3.f2=0;_tmpF3;});else{
return({struct _tuple12 _tmpF4;_tmpF4.f1=(unsigned)x,_tmpF4.f2=1;_tmpF4;});}}case 7U: _LL8: _LL9:
 return({struct _tuple12 _tmpF5;_tmpF5.f1=0U,_tmpF5.f2=0;_tmpF5;});case 1U: _LLA: _LLB:
 return({struct _tuple12 _tmpF6;_tmpF6.f1=0U,_tmpF6.f2=1;_tmpF6;});default: _LLC: _LLD:
# 61
({struct Cyc_String_pa_PrintArg_struct _tmpD=({struct Cyc_String_pa_PrintArg_struct _tmpF7;_tmpF7.tag=0U,({struct _fat_ptr _tmp11E=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpF7.f1=_tmp11E;});_tmpF7;});void*_tmpB[1U];_tmpB[0]=& _tmpD;({unsigned _tmp120=e->loc;struct _fat_ptr _tmp11F=({const char*_tmpC="expecting unsigned int found %s";_tag_fat(_tmpC,sizeof(char),32U);});Cyc_Tcutil_terr(_tmp120,_tmp11F,_tag_fat(_tmpB,sizeof(void*),1U));});});return({struct _tuple12 _tmpF8;_tmpF8.f1=0U,_tmpF8.f2=1;_tmpF8;});}_LL3:;}}}struct _tuple14{int f1;int f2;};
# 65
static struct _tuple14 Cyc_Evexp_eval_const_bool_exp(struct Cyc_Absyn_Exp*e){
struct _tuple13 _stmttmp2=({Cyc_Evexp_eval_const_exp(e);});int _tmp10;union Cyc_Absyn_Cnst _tmpF;_LL1: _tmpF=_stmttmp2.f1;_tmp10=_stmttmp2.f2;_LL2: {union Cyc_Absyn_Cnst cn=_tmpF;int known=_tmp10;
if(!known)
return({struct _tuple14 _tmpF9;_tmpF9.f1=0,_tmpF9.f2=0;_tmpF9;});{
# 67
union Cyc_Absyn_Cnst _stmttmp3=({Cyc_Evexp_promote_const(cn);});union Cyc_Absyn_Cnst _tmp11=_stmttmp3;long long _tmp12;int _tmp13;switch((_tmp11.Float_c).tag){case 5U: _LL4: _tmp13=((_tmp11.Int_c).val).f2;_LL5: {int b=_tmp13;
# 70
return({struct _tuple14 _tmpFA;_tmpFA.f1=b != 0,_tmpFA.f2=1;_tmpFA;});}case 6U: _LL6: _tmp12=((_tmp11.LongLong_c).val).f2;_LL7: {long long b=_tmp12;
return({struct _tuple14 _tmpFB;_tmpFB.f1=b != (long long)0,_tmpFB.f2=1;_tmpFB;});}case 1U: _LL8: _LL9:
 return({struct _tuple14 _tmpFC;_tmpFC.f1=0,_tmpFC.f2=0;_tmpFC;});case 7U: _LLA: _LLB:
 return({struct _tuple14 _tmpFD;_tmpFD.f1=0,_tmpFD.f2=1;_tmpFD;});default: _LLC: _LLD:
({void*_tmp14=0U;({unsigned _tmp122=e->loc;struct _fat_ptr _tmp121=({const char*_tmp15="expecting bool";_tag_fat(_tmp15,sizeof(char),15U);});Cyc_Tcutil_terr(_tmp122,_tmp121,_tag_fat(_tmp14,sizeof(void*),0U));});});return({struct _tuple14 _tmpFE;_tmpFE.f1=0,_tmpFE.f2=0;_tmpFE;});}_LL3:;}}}struct _tuple15{enum Cyc_Absyn_Primop f1;union Cyc_Absyn_Cnst f2;};
# 78
static struct _tuple13 Cyc_Evexp_eval_const_unprimop(enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e){
struct _tuple13 _stmttmp4=({Cyc_Evexp_eval_const_exp(e);});int _tmp18;union Cyc_Absyn_Cnst _tmp17;_LL1: _tmp17=_stmttmp4.f1;_tmp18=_stmttmp4.f2;_LL2: {union Cyc_Absyn_Cnst cn=_tmp17;int known=_tmp18;
if(!known)
return({struct _tuple13 _tmpFF;_tmpFF.f1=cn,_tmpFF.f2=0;_tmpFF;});
# 80
{struct _tuple15 _stmttmp5=({struct _tuple15 _tmp101;_tmp101.f1=p,_tmp101.f2=cn;_tmp101;});struct _tuple15 _tmp19=_stmttmp5;long long _tmp1A;char _tmp1B;short _tmp1C;int _tmp1D;long long _tmp1F;enum Cyc_Absyn_Sign _tmp1E;char _tmp21;enum Cyc_Absyn_Sign _tmp20;short _tmp23;enum Cyc_Absyn_Sign _tmp22;int _tmp25;enum Cyc_Absyn_Sign _tmp24;long long _tmp27;enum Cyc_Absyn_Sign _tmp26;char _tmp29;enum Cyc_Absyn_Sign _tmp28;short _tmp2B;enum Cyc_Absyn_Sign _tmp2A;int _tmp2D;enum Cyc_Absyn_Sign _tmp2C;switch(_tmp19.f1){case Cyc_Absyn_Plus: _LL4: _LL5:
# 83
 goto _LL3;case Cyc_Absyn_Minus: switch(((_tmp19.f2).LongLong_c).tag){case 5U: _LL6: _tmp2C=(((_tmp19.f2).Int_c).val).f1;_tmp2D=(((_tmp19.f2).Int_c).val).f2;_LL7: {enum Cyc_Absyn_Sign s=_tmp2C;int i=_tmp2D;
cn=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,- i);});goto _LL3;}case 4U: _LL8: _tmp2A=(((_tmp19.f2).Short_c).val).f1;_tmp2B=(((_tmp19.f2).Short_c).val).f2;_LL9: {enum Cyc_Absyn_Sign s=_tmp2A;short i=_tmp2B;
cn=({Cyc_Absyn_Short_c(Cyc_Absyn_Signed,- i);});goto _LL3;}case 2U: _LLA: _tmp28=(((_tmp19.f2).Char_c).val).f1;_tmp29=(((_tmp19.f2).Char_c).val).f2;_LLB: {enum Cyc_Absyn_Sign s=_tmp28;char i=_tmp29;
cn=({Cyc_Absyn_Char_c(Cyc_Absyn_Signed,- i);});goto _LL3;}case 6U: _LLC: _tmp26=(((_tmp19.f2).LongLong_c).val).f1;_tmp27=(((_tmp19.f2).LongLong_c).val).f2;_LLD: {enum Cyc_Absyn_Sign s=_tmp26;long long i=_tmp27;
cn=({Cyc_Absyn_LongLong_c(Cyc_Absyn_Signed,- i);});goto _LL3;}default: goto _LL20;}case Cyc_Absyn_Bitnot: switch(((_tmp19.f2).LongLong_c).tag){case 5U: _LLE: _tmp24=(((_tmp19.f2).Int_c).val).f1;_tmp25=(((_tmp19.f2).Int_c).val).f2;_LLF: {enum Cyc_Absyn_Sign s=_tmp24;int i=_tmp25;
cn=({Cyc_Absyn_Int_c(Cyc_Absyn_Unsigned,~ i);});goto _LL3;}case 4U: _LL10: _tmp22=(((_tmp19.f2).Short_c).val).f1;_tmp23=(((_tmp19.f2).Short_c).val).f2;_LL11: {enum Cyc_Absyn_Sign s=_tmp22;short i=_tmp23;
cn=({Cyc_Absyn_Short_c(Cyc_Absyn_Unsigned,~ i);});goto _LL3;}case 2U: _LL12: _tmp20=(((_tmp19.f2).Char_c).val).f1;_tmp21=(((_tmp19.f2).Char_c).val).f2;_LL13: {enum Cyc_Absyn_Sign s=_tmp20;char i=_tmp21;
cn=({Cyc_Absyn_Char_c(Cyc_Absyn_Unsigned,~ i);});goto _LL3;}case 6U: _LL14: _tmp1E=(((_tmp19.f2).LongLong_c).val).f1;_tmp1F=(((_tmp19.f2).LongLong_c).val).f2;_LL15: {enum Cyc_Absyn_Sign s=_tmp1E;long long i=_tmp1F;
cn=({Cyc_Absyn_LongLong_c(Cyc_Absyn_Unsigned,~ i);});goto _LL3;}default: goto _LL20;}case Cyc_Absyn_Not: switch(((_tmp19.f2).Null_c).tag){case 5U: _LL16: _tmp1D=(((_tmp19.f2).Int_c).val).f2;_LL17: {int i=_tmp1D;
cn=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,i == 0?1: 0);});goto _LL3;}case 4U: _LL18: _tmp1C=(((_tmp19.f2).Short_c).val).f2;_LL19: {short i=_tmp1C;
cn=({Cyc_Absyn_Short_c(Cyc_Absyn_Signed,(int)i == 0?1: 0);});goto _LL3;}case 2U: _LL1A: _tmp1B=(((_tmp19.f2).Char_c).val).f2;_LL1B: {char i=_tmp1B;
cn=({Cyc_Absyn_Char_c(Cyc_Absyn_Signed,(int)i == 0?'\001':'\000');});goto _LL3;}case 6U: _LL1C: _tmp1A=(((_tmp19.f2).LongLong_c).val).f2;_LL1D: {long long i=_tmp1A;
cn=({Cyc_Absyn_LongLong_c(Cyc_Absyn_Signed,(long long)(i == (long long)0?1: 0));});goto _LL3;}case 1U: _LL1E: _LL1F:
 cn=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,1);});goto _LL3;default: goto _LL20;}default: _LL20: _LL21:
 return({struct _tuple13 _tmp100;_tmp100.f1=cn,_tmp100.f2=0;_tmp100;});}_LL3:;}
# 99
return({struct _tuple13 _tmp102;_tmp102.f1=cn,_tmp102.f2=1;_tmp102;});}}struct _tuple16{enum Cyc_Absyn_Primop f1;int f2;};
# 103
static struct _tuple13 Cyc_Evexp_eval_const_binprimop(enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
struct _tuple13 _stmttmp6=({Cyc_Evexp_eval_const_exp(e1);});int _tmp30;union Cyc_Absyn_Cnst _tmp2F;_LL1: _tmp2F=_stmttmp6.f1;_tmp30=_stmttmp6.f2;_LL2: {union Cyc_Absyn_Cnst cn1=_tmp2F;int known1=_tmp30;
struct _tuple13 _stmttmp7=({Cyc_Evexp_eval_const_exp(e2);});int _tmp32;union Cyc_Absyn_Cnst _tmp31;_LL4: _tmp31=_stmttmp7.f1;_tmp32=_stmttmp7.f2;_LL5: {union Cyc_Absyn_Cnst cn2=_tmp31;int known2=_tmp32;
if(!known1 || !known2)
return({struct _tuple13 _tmp103;_tmp103.f1=cn1,_tmp103.f2=0;_tmp103;});
# 106
cn1=({Cyc_Evexp_promote_const(cn1);});
# 109
cn2=({Cyc_Evexp_promote_const(cn2);});{
enum Cyc_Absyn_Sign s1;enum Cyc_Absyn_Sign s2;
int i1;int i2;
{union Cyc_Absyn_Cnst _tmp33=cn1;int _tmp35;enum Cyc_Absyn_Sign _tmp34;if((_tmp33.Int_c).tag == 5){_LL7: _tmp34=((_tmp33.Int_c).val).f1;_tmp35=((_tmp33.Int_c).val).f2;_LL8: {enum Cyc_Absyn_Sign x=_tmp34;int y=_tmp35;
s1=x;i1=y;goto _LL6;}}else{_LL9: _LLA:
 return({struct _tuple13 _tmp104;_tmp104.f1=cn1,_tmp104.f2=0;_tmp104;});}_LL6:;}
# 116
{union Cyc_Absyn_Cnst _tmp36=cn2;int _tmp38;enum Cyc_Absyn_Sign _tmp37;if((_tmp36.Int_c).tag == 5){_LLC: _tmp37=((_tmp36.Int_c).val).f1;_tmp38=((_tmp36.Int_c).val).f2;_LLD: {enum Cyc_Absyn_Sign x=_tmp37;int y=_tmp38;
s2=x;i2=y;goto _LLB;}}else{_LLE: _LLF:
 return({struct _tuple13 _tmp105;_tmp105.f1=cn1,_tmp105.f2=0;_tmp105;});}_LLB:;}
# 120
{enum Cyc_Absyn_Primop _tmp39=p;switch(_tmp39){case Cyc_Absyn_Div: _LL11: _LL12:
 goto _LL14;case Cyc_Absyn_Mod: _LL13: _LL14:
# 123
 if(i2 == 0){
({void*_tmp3A=0U;({unsigned _tmp124=e2->loc;struct _fat_ptr _tmp123=({const char*_tmp3B="division by zero in constant expression";_tag_fat(_tmp3B,sizeof(char),40U);});Cyc_Tcutil_terr(_tmp124,_tmp123,_tag_fat(_tmp3A,sizeof(void*),0U));});});
return({struct _tuple13 _tmp106;_tmp106.f1=cn1,_tmp106.f2=1;_tmp106;});}
# 123
goto _LL10;default: _LL15: _LL16:
# 128
 goto _LL10;}_LL10:;}{
# 130
int has_u_arg=(int)s1 == (int)1U ||(int)s2 == (int)Cyc_Absyn_Unsigned;
unsigned u1=(unsigned)i1;
unsigned u2=(unsigned)i2;
int i3=0;
unsigned u3=0U;
int b3=1;
int use_i3=0;
int use_u3=0;
int use_b3=0;
{struct _tuple16 _stmttmp8=({struct _tuple16 _tmp108;_tmp108.f1=p,_tmp108.f2=has_u_arg;_tmp108;});struct _tuple16 _tmp3C=_stmttmp8;switch(_tmp3C.f1){case Cyc_Absyn_Plus: switch(_tmp3C.f2){case 0U: _LL18: _LL19:
 i3=i1 + i2;use_i3=1;goto _LL17;case 1U: _LL22: _LL23:
# 145
 u3=u1 + u2;use_u3=1;goto _LL17;default: goto _LL4A;}case Cyc_Absyn_Times: switch(_tmp3C.f2){case 0U: _LL1A: _LL1B:
# 141
 i3=i1 * i2;use_i3=1;goto _LL17;case 1U: _LL24: _LL25:
# 146
 u3=u1 * u2;use_u3=1;goto _LL17;default: goto _LL4A;}case Cyc_Absyn_Minus: switch(_tmp3C.f2){case 0U: _LL1C: _LL1D:
# 142
 i3=i1 - i2;use_i3=1;goto _LL17;case 1U: _LL26: _LL27:
# 147
 u3=u1 - u2;use_u3=1;goto _LL17;default: goto _LL4A;}case Cyc_Absyn_Div: switch(_tmp3C.f2){case 0U: _LL1E: _LL1F:
# 143
 i3=i1 / i2;use_i3=1;goto _LL17;case 1U: _LL28: _LL29:
# 148
 u3=u1 / u2;use_u3=1;goto _LL17;default: goto _LL4A;}case Cyc_Absyn_Mod: switch(_tmp3C.f2){case 0U: _LL20: _LL21:
# 144
 i3=i1 % i2;use_i3=1;goto _LL17;case 1U: _LL2A: _LL2B:
# 149
 u3=u1 % u2;use_u3=1;goto _LL17;default: goto _LL4A;}case Cyc_Absyn_Eq: _LL2C: _LL2D:
 b3=i1 == i2;use_b3=1;goto _LL17;case Cyc_Absyn_Neq: _LL2E: _LL2F:
 b3=i1 != i2;use_b3=1;goto _LL17;case Cyc_Absyn_Gt: switch(_tmp3C.f2){case 0U: _LL30: _LL31:
 b3=i1 > i2;use_b3=1;goto _LL17;case 1U: _LL38: _LL39:
# 156
 b3=u1 > u2;use_b3=1;goto _LL17;default: goto _LL4A;}case Cyc_Absyn_Lt: switch(_tmp3C.f2){case 0U: _LL32: _LL33:
# 153
 b3=i1 < i2;use_b3=1;goto _LL17;case 1U: _LL3A: _LL3B:
# 157
 b3=u1 < u2;use_b3=1;goto _LL17;default: goto _LL4A;}case Cyc_Absyn_Gte: switch(_tmp3C.f2){case 0U: _LL34: _LL35:
# 154
 b3=i1 >= i2;use_b3=1;goto _LL17;case 1U: _LL3C: _LL3D:
# 158
 b3=u1 >= u2;use_b3=1;goto _LL17;default: goto _LL4A;}case Cyc_Absyn_Lte: switch(_tmp3C.f2){case 0U: _LL36: _LL37:
# 155
 b3=i1 <= i2;use_b3=1;goto _LL17;case 1U: _LL3E: _LL3F:
# 159
 b3=u1 <= u2;use_b3=1;goto _LL17;default: goto _LL4A;}case Cyc_Absyn_Bitand: _LL40: _LL41:
 u3=u1 & u2;use_u3=1;goto _LL17;case Cyc_Absyn_Bitor: _LL42: _LL43:
 u3=u1 | u2;use_u3=1;goto _LL17;case Cyc_Absyn_Bitxor: _LL44: _LL45:
 u3=u1 ^ u2;use_u3=1;goto _LL17;case Cyc_Absyn_Bitlshift: _LL46: _LL47:
 u3=u1 << u2;use_u3=1;goto _LL17;case Cyc_Absyn_Bitlrshift: _LL48: _LL49:
 u3=u1 >> u2;use_u3=1;goto _LL17;default: _LL4A: _LL4B:
({void*_tmp3D=0U;({unsigned _tmp126=e1->loc;struct _fat_ptr _tmp125=({const char*_tmp3E="bad constant expression";_tag_fat(_tmp3E,sizeof(char),24U);});Cyc_Tcutil_terr(_tmp126,_tmp125,_tag_fat(_tmp3D,sizeof(void*),0U));});});return({struct _tuple13 _tmp107;_tmp107.f1=cn1,_tmp107.f2=1;_tmp107;});}_LL17:;}
# 167
if(use_i3)return({struct _tuple13 _tmp109;({union Cyc_Absyn_Cnst _tmp127=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,i3);});_tmp109.f1=_tmp127;}),_tmp109.f2=1;_tmp109;});if(use_u3)
return({struct _tuple13 _tmp10A;({union Cyc_Absyn_Cnst _tmp128=({Cyc_Absyn_Int_c(Cyc_Absyn_Unsigned,(int)u3);});_tmp10A.f1=_tmp128;}),_tmp10A.f2=1;_tmp10A;});
# 167
if(use_b3)
# 169
return({struct _tuple13 _tmp10B;({union Cyc_Absyn_Cnst _tmp129=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,b3?1: 0);});_tmp10B.f1=_tmp129;}),_tmp10B.f2=1;_tmp10B;});
# 167
({void*_tmp3F=0U;({int(*_tmp12B)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmp41)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmp41;});struct _fat_ptr _tmp12A=({const char*_tmp40="Evexp::eval_const_binop";_tag_fat(_tmp40,sizeof(char),24U);});_tmp12B(_tmp12A,_tag_fat(_tmp3F,sizeof(void*),0U));});});}}}}}struct _tuple17{void*f1;union Cyc_Absyn_Cnst f2;};
# 176
static struct _tuple13 Cyc_Evexp_eval_const_exp(struct Cyc_Absyn_Exp*e){
struct _tuple13 ans;
{void*_stmttmp9=e->r;void*_tmp43=_stmttmp9;struct Cyc_Absyn_Enumfield*_tmp44;struct Cyc_Absyn_Enumfield*_tmp45;struct Cyc_Absyn_Exp*_tmp47;void*_tmp46;void*_tmp48;struct Cyc_List_List*_tmp4A;enum Cyc_Absyn_Primop _tmp49;struct Cyc_Absyn_Exp*_tmp4C;struct Cyc_Absyn_Exp*_tmp4B;struct Cyc_Absyn_Exp*_tmp4E;struct Cyc_Absyn_Exp*_tmp4D;struct Cyc_Absyn_Exp*_tmp51;struct Cyc_Absyn_Exp*_tmp50;struct Cyc_Absyn_Exp*_tmp4F;union Cyc_Absyn_Cnst _tmp52;switch(*((int*)_tmp43)){case 1U: _LL1: _LL2:
# 180
 return({struct _tuple13 _tmp10C;({union Cyc_Absyn_Cnst _tmp12C=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp10C.f1=_tmp12C;}),_tmp10C.f2=0;_tmp10C;});case 0U: _LL3: _tmp52=((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp43)->f1;_LL4: {union Cyc_Absyn_Cnst c=_tmp52;
return({struct _tuple13 _tmp10D;_tmp10D.f1=c,_tmp10D.f2=1;_tmp10D;});}case 2U: _LL5: _LL6:
 return({struct _tuple13 _tmp10E;({union Cyc_Absyn_Cnst _tmp12D=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp10E.f1=_tmp12D;}),_tmp10E.f2=1;_tmp10E;});case 6U: _LL7: _tmp4F=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp43)->f1;_tmp50=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp43)->f2;_tmp51=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp43)->f3;_LL8: {struct Cyc_Absyn_Exp*e1=_tmp4F;struct Cyc_Absyn_Exp*e2=_tmp50;struct Cyc_Absyn_Exp*e3=_tmp51;
# 184
struct _tuple14 _stmttmpA=({Cyc_Evexp_eval_const_bool_exp(e1);});int _tmp54;int _tmp53;_LL20: _tmp53=_stmttmpA.f1;_tmp54=_stmttmpA.f2;_LL21: {int bool1=_tmp53;int known1=_tmp54;
if(!known1){
({Cyc_Evexp_eval_const_exp(e2);});
({Cyc_Evexp_eval_const_exp(e3);});
return({struct _tuple13 _tmp10F;({union Cyc_Absyn_Cnst _tmp12E=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp10F.f1=_tmp12E;}),_tmp10F.f2=0;_tmp10F;});}
# 185
ans=
# 190
bool1?({Cyc_Evexp_eval_const_exp(e2);}):({Cyc_Evexp_eval_const_exp(e3);});
goto _LL0;}}case 7U: _LL9: _tmp4D=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp43)->f1;_tmp4E=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp43)->f2;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp4D;struct Cyc_Absyn_Exp*e2=_tmp4E;
# 193
struct _tuple14 _stmttmpB=({Cyc_Evexp_eval_const_bool_exp(e1);});int _tmp56;int _tmp55;_LL23: _tmp55=_stmttmpB.f1;_tmp56=_stmttmpB.f2;_LL24: {int bool1=_tmp55;int known1=_tmp56;
if(!known1){
({Cyc_Evexp_eval_const_exp(e2);});
return({struct _tuple13 _tmp110;({union Cyc_Absyn_Cnst _tmp12F=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp110.f1=_tmp12F;}),_tmp110.f2=0;_tmp110;});}
# 194
if(bool1)
# 198
ans=({Cyc_Evexp_eval_const_exp(e2);});else{
ans=({struct _tuple13 _tmp111;({union Cyc_Absyn_Cnst _tmp130=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp111.f1=_tmp130;}),_tmp111.f2=1;_tmp111;});}
goto _LL0;}}case 8U: _LLB: _tmp4B=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp43)->f1;_tmp4C=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp43)->f2;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp4B;struct Cyc_Absyn_Exp*e2=_tmp4C;
# 202
struct _tuple14 _stmttmpC=({Cyc_Evexp_eval_const_bool_exp(e1);});int _tmp58;int _tmp57;_LL26: _tmp57=_stmttmpC.f1;_tmp58=_stmttmpC.f2;_LL27: {int bool1=_tmp57;int known1=_tmp58;
if(!known1){
({Cyc_Evexp_eval_const_exp(e2);});
return({struct _tuple13 _tmp112;({union Cyc_Absyn_Cnst _tmp131=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp112.f1=_tmp131;}),_tmp112.f2=0;_tmp112;});}
# 203
if(!bool1)
# 207
ans=({Cyc_Evexp_eval_const_exp(e2);});else{
ans=({struct _tuple13 _tmp113;({union Cyc_Absyn_Cnst _tmp132=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,1);});_tmp113.f1=_tmp132;}),_tmp113.f2=1;_tmp113;});}
goto _LL0;}}case 3U: _LLD: _tmp49=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp43)->f1;_tmp4A=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp43)->f2;_LLE: {enum Cyc_Absyn_Primop p=_tmp49;struct Cyc_List_List*es=_tmp4A;
# 211
if(es == 0){
({void*_tmp59=0U;({unsigned _tmp134=e->loc;struct _fat_ptr _tmp133=({const char*_tmp5A="bad static expression (no args to primop)";_tag_fat(_tmp5A,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp134,_tmp133,_tag_fat(_tmp59,sizeof(void*),0U));});});
return({struct _tuple13 _tmp114;({union Cyc_Absyn_Cnst _tmp135=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp114.f1=_tmp135;}),_tmp114.f2=1;_tmp114;});}
# 211
if(es->tl == 0){
# 216
ans=({Cyc_Evexp_eval_const_unprimop(p,(struct Cyc_Absyn_Exp*)es->hd);});
goto _LL0;}
# 211
if(((struct Cyc_List_List*)_check_null(es->tl))->tl == 0){
# 220
ans=({Cyc_Evexp_eval_const_binprimop(p,(struct Cyc_Absyn_Exp*)es->hd,(struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)_check_null(es->tl))->hd);});
goto _LL0;}
# 211
({void*_tmp5B=0U;({unsigned _tmp137=e->loc;struct _fat_ptr _tmp136=({const char*_tmp5C="bad static expression (too many args to primop)";_tag_fat(_tmp5C,sizeof(char),48U);});Cyc_Tcutil_terr(_tmp137,_tmp136,_tag_fat(_tmp5B,sizeof(void*),0U));});});
# 224
return({struct _tuple13 _tmp115;({union Cyc_Absyn_Cnst _tmp138=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp115.f1=_tmp138;}),_tmp115.f2=1;_tmp115;});}case 40U: _LLF: _tmp48=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp43)->f1;_LL10: {void*t=_tmp48;
# 227
{void*_stmttmpD=({Cyc_Tcutil_compress(t);});void*_tmp5D=_stmttmpD;struct Cyc_Absyn_Exp*_tmp5E;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp5D)->tag == 9U){_LL29: _tmp5E=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp5D)->f1;_LL2A: {struct Cyc_Absyn_Exp*e2=_tmp5E;
# 229
e->r=e2->r;
return({Cyc_Evexp_eval_const_exp(e2);});}}else{_LL2B: _LL2C:
 goto _LL28;}_LL28:;}
# 233
goto _LL12;}case 17U: _LL11: _LL12:
 goto _LL14;case 18U: _LL13: _LL14:
 goto _LL16;case 19U: _LL15: _LL16:
 ans=({struct _tuple13 _tmp116;({union Cyc_Absyn_Cnst _tmp139=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp116.f1=_tmp139;}),_tmp116.f2=0;_tmp116;});goto _LL0;case 14U: _LL17: _tmp46=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp43)->f1;_tmp47=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp43)->f2;_LL18: {void*t=_tmp46;struct Cyc_Absyn_Exp*e2=_tmp47;
# 239
ans=({Cyc_Evexp_eval_const_exp(e2);});
if(ans.f2){
struct _tuple17 _stmttmpE=({struct _tuple17 _tmp118;({void*_tmp13A=({Cyc_Tcutil_compress(t);});_tmp118.f1=_tmp13A;}),_tmp118.f2=ans.f1;_tmp118;});struct _tuple17 _tmp5F=_stmttmpE;int _tmp62;enum Cyc_Absyn_Sign _tmp61;void*_tmp60;short _tmp65;enum Cyc_Absyn_Sign _tmp64;void*_tmp63;char _tmp68;enum Cyc_Absyn_Sign _tmp67;void*_tmp66;int _tmp6C;enum Cyc_Absyn_Sign _tmp6B;enum Cyc_Absyn_Size_of _tmp6A;enum Cyc_Absyn_Sign _tmp69;short _tmp70;enum Cyc_Absyn_Sign _tmp6F;enum Cyc_Absyn_Size_of _tmp6E;enum Cyc_Absyn_Sign _tmp6D;char _tmp74;enum Cyc_Absyn_Sign _tmp73;enum Cyc_Absyn_Size_of _tmp72;enum Cyc_Absyn_Sign _tmp71;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->tag == 0U)switch(*((int*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f1)){case 1U: switch(((_tmp5F.f2).Int_c).tag){case 2U: _LL2E: _tmp71=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f1)->f1;_tmp72=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f1)->f2;_tmp73=(((_tmp5F.f2).Char_c).val).f1;_tmp74=(((_tmp5F.f2).Char_c).val).f2;_LL2F: {enum Cyc_Absyn_Sign sn=_tmp71;enum Cyc_Absyn_Size_of sz=_tmp72;enum Cyc_Absyn_Sign sn2=_tmp73;char x=_tmp74;
# 244
_tmp6D=sn;_tmp6E=sz;_tmp6F=sn2;_tmp70=(short)x;goto _LL31;}case 4U: _LL30: _tmp6D=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f1)->f1;_tmp6E=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f1)->f2;_tmp6F=(((_tmp5F.f2).Short_c).val).f1;_tmp70=(((_tmp5F.f2).Short_c).val).f2;_LL31: {enum Cyc_Absyn_Sign sn=_tmp6D;enum Cyc_Absyn_Size_of sz=_tmp6E;enum Cyc_Absyn_Sign sn2=_tmp6F;short x=_tmp70;
# 246
_tmp69=sn;_tmp6A=sz;_tmp6B=sn2;_tmp6C=(int)x;goto _LL33;}case 5U: _LL32: _tmp69=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f1)->f1;_tmp6A=((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f1)->f2;_tmp6B=(((_tmp5F.f2).Int_c).val).f1;_tmp6C=(((_tmp5F.f2).Int_c).val).f2;_LL33: {enum Cyc_Absyn_Sign sn=_tmp69;enum Cyc_Absyn_Size_of sz=_tmp6A;enum Cyc_Absyn_Sign sn2=_tmp6B;int x=_tmp6C;
# 248
if((int)sn != (int)sn2)
({union Cyc_Absyn_Cnst _tmp13B=({Cyc_Absyn_Int_c(sn,x);});ans.f1=_tmp13B;});
# 248
goto _LL2D;}default: goto _LL3A;}case 5U: if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f2 != 0)switch(((_tmp5F.f2).Int_c).tag){case 2U: _LL34: _tmp66=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f2)->hd;_tmp67=(((_tmp5F.f2).Char_c).val).f1;_tmp68=(((_tmp5F.f2).Char_c).val).f2;_LL35: {void*it=_tmp66;enum Cyc_Absyn_Sign sn2=_tmp67;char x=_tmp68;
# 253
_tmp63=it;_tmp64=sn2;_tmp65=(short)x;goto _LL37;}case 4U: _LL36: _tmp63=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f2)->hd;_tmp64=(((_tmp5F.f2).Short_c).val).f1;_tmp65=(((_tmp5F.f2).Short_c).val).f2;_LL37: {void*it=_tmp63;enum Cyc_Absyn_Sign sn2=_tmp64;short x=_tmp65;
# 255
_tmp60=it;_tmp61=sn2;_tmp62=(int)x;goto _LL39;}case 5U: _LL38: _tmp60=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp5F.f1)->f2)->hd;_tmp61=(((_tmp5F.f2).Int_c).val).f1;_tmp62=(((_tmp5F.f2).Int_c).val).f2;_LL39: {void*it=_tmp60;enum Cyc_Absyn_Sign sn2=_tmp61;int x=_tmp62;
# 257
if(x < 0)
({void*_tmp75=0U;({unsigned _tmp13D=e->loc;struct _fat_ptr _tmp13C=({const char*_tmp76="cannot cast negative number to a tag type";_tag_fat(_tmp76,sizeof(char),42U);});Cyc_Tcutil_terr(_tmp13D,_tmp13C,_tag_fat(_tmp75,sizeof(void*),0U));});});
# 257
({int(*_tmp77)(void*,void*)=Cyc_Unify_unify;void*_tmp78=it;void*_tmp79=(void*)({struct Cyc_Absyn_ValueofType_Absyn_Type_struct*_tmp7A=_cycalloc(sizeof(*_tmp7A));
# 259
_tmp7A->tag=9U,({struct Cyc_Absyn_Exp*_tmp13E=({Cyc_Absyn_const_exp(ans.f1,0U);});_tmp7A->f1=_tmp13E;});_tmp7A;});_tmp77(_tmp78,_tmp79);});
({union Cyc_Absyn_Cnst _tmp13F=({Cyc_Absyn_Int_c(Cyc_Absyn_Unsigned,x);});ans.f1=_tmp13F;});
goto _LL2D;}default: goto _LL3A;}else{goto _LL3A;}default: goto _LL3A;}else{_LL3A: _LL3B:
({struct Cyc_String_pa_PrintArg_struct _tmp7D=({struct Cyc_String_pa_PrintArg_struct _tmp117;_tmp117.tag=0U,({struct _fat_ptr _tmp140=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp117.f1=_tmp140;});_tmp117;});void*_tmp7B[1U];_tmp7B[0]=& _tmp7D;({unsigned _tmp142=e->loc;struct _fat_ptr _tmp141=({const char*_tmp7C="cannot cast to %s";_tag_fat(_tmp7C,sizeof(char),18U);});Cyc_Tcutil_terr(_tmp142,_tmp141,_tag_fat(_tmp7B,sizeof(void*),1U));});});goto _LL2D;}_LL2D:;}
# 240
goto _LL0;}case 33U: _LL19: _tmp45=((struct Cyc_Absyn_AnonEnum_e_Absyn_Raw_exp_struct*)_tmp43)->f2;_LL1A: {struct Cyc_Absyn_Enumfield*ef=_tmp45;
# 265
_tmp44=ef;goto _LL1C;}case 32U: _LL1B: _tmp44=((struct Cyc_Absyn_Enum_e_Absyn_Raw_exp_struct*)_tmp43)->f2;_LL1C: {struct Cyc_Absyn_Enumfield*ef=_tmp44;
ans=({Cyc_Evexp_eval_const_exp((struct Cyc_Absyn_Exp*)_check_null(ef->tag));});goto _LL0;}default: _LL1D: _LL1E:
# 269
 return({struct _tuple13 _tmp119;({union Cyc_Absyn_Cnst _tmp143=({Cyc_Absyn_Int_c(Cyc_Absyn_Signed,0);});_tmp119.f1=_tmp143;}),_tmp119.f2=0;_tmp119;});}_LL0:;}
# 271
if(ans.f2){
void*c;
{union Cyc_Absyn_Cnst c=ans.f1;
({void*_tmp144=(void*)({struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*_tmp7E=_cycalloc(sizeof(*_tmp7E));_tmp7E->tag=0U,_tmp7E->f1=c;_tmp7E;});e->r=_tmp144;});}}
# 271
return ans;}
# 176
int Cyc_Evexp_c_can_eval(struct Cyc_Absyn_Exp*e){
# 280
void*_stmttmpF=e->r;void*_tmp80=_stmttmpF;struct Cyc_Absyn_Exp*_tmp81;struct Cyc_List_List*_tmp83;enum Cyc_Absyn_Primop _tmp82;struct Cyc_Absyn_Exp*_tmp85;struct Cyc_Absyn_Exp*_tmp84;struct Cyc_Absyn_Exp*_tmp87;struct Cyc_Absyn_Exp*_tmp86;struct Cyc_Absyn_Exp*_tmp8A;struct Cyc_Absyn_Exp*_tmp89;struct Cyc_Absyn_Exp*_tmp88;switch(*((int*)_tmp80)){case 33U: _LL1: _LL2:
 goto _LL4;case 32U: _LL3: _LL4:
 goto _LL6;case 17U: _LL5: _LL6:
 goto _LL8;case 18U: _LL7: _LL8:
 goto _LLA;case 19U: _LL9: _LLA:
 goto _LLC;case 0U: _LLB: _LLC:
 return 1;case 6U: _LLD: _tmp88=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp80)->f1;_tmp89=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp80)->f2;_tmp8A=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp80)->f3;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp88;struct Cyc_Absyn_Exp*e2=_tmp89;struct Cyc_Absyn_Exp*e3=_tmp8A;
# 288
return(({Cyc_Evexp_c_can_eval(e1);})&&({Cyc_Evexp_c_can_eval(e2);}))&&({Cyc_Evexp_c_can_eval(e3);});}case 7U: _LLF: _tmp86=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp80)->f1;_tmp87=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp80)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp86;struct Cyc_Absyn_Exp*e2=_tmp87;
_tmp84=e1;_tmp85=e2;goto _LL12;}case 8U: _LL11: _tmp84=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp80)->f1;_tmp85=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp80)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp84;struct Cyc_Absyn_Exp*e2=_tmp85;
# 291
return({Cyc_Evexp_c_can_eval(e1);})&&({Cyc_Evexp_c_can_eval(e2);});}case 3U: _LL13: _tmp82=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp80)->f1;_tmp83=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp80)->f2;_LL14: {enum Cyc_Absyn_Primop p=_tmp82;struct Cyc_List_List*es=_tmp83;
# 293
for(0;es != 0;es=es->tl){
if(!({Cyc_Evexp_c_can_eval((struct Cyc_Absyn_Exp*)es->hd);}))return 0;}
# 293
return 1;}case 40U: _LL15: _LL16:
# 296
 return 0;case 14U: _LL17: _tmp81=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp80)->f2;_LL18: {struct Cyc_Absyn_Exp*e=_tmp81;
return({Cyc_Evexp_c_can_eval(e);});}default: _LL19: _LL1A:
 return 0;}_LL0:;}
# 302
static int Cyc_Evexp_const_exp_case_number(struct Cyc_Absyn_Exp*e){
void*_stmttmp10=e->r;void*_tmp8C=_stmttmp10;switch(*((int*)_tmp8C)){case 0U: _LL1: _LL2:
 return 1;case 6U: _LL3: _LL4:
 return 2;case 3U: _LL5: _LL6:
 return 3;case 17U: _LL7: _LL8:
 goto _LLA;case 18U: _LL9: _LLA:
 return 4;case 19U: _LLB: _LLC:
 return 5;case 14U: _LLD: _LLE:
 return 6;case 7U: _LLF: _LL10:
 return 7;case 8U: _LL11: _LL12:
 return 8;case 40U: _LL13: _LL14:
 return 9;default: _LL15: _LL16:
# 315
({struct Cyc_String_pa_PrintArg_struct _tmp8F=({struct Cyc_String_pa_PrintArg_struct _tmp11A;_tmp11A.tag=0U,({struct _fat_ptr _tmp145=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp11A.f1=_tmp145;});_tmp11A;});void*_tmp8D[1U];_tmp8D[0]=& _tmp8F;({unsigned _tmp147=e->loc;struct _fat_ptr _tmp146=({const char*_tmp8E="bad static expression %s";_tag_fat(_tmp8E,sizeof(char),25U);});Cyc_Tcutil_terr(_tmp147,_tmp146,_tag_fat(_tmp8D,sizeof(void*),1U));});});return 0;}_LL0:;}
# 320
static struct Cyc_Absyn_Exp*Cyc_Evexp_strip_cast(struct Cyc_Absyn_Exp*e){
{void*_stmttmp11=e->r;void*_tmp91=_stmttmp11;struct Cyc_Absyn_Exp*_tmp93;void*_tmp92;if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp91)->tag == 14U){_LL1: _tmp92=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp91)->f1;_tmp93=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp91)->f2;_LL2: {void*t=_tmp92;struct Cyc_Absyn_Exp*e2=_tmp93;
# 323
if(({Cyc_Unify_unify(t,Cyc_Absyn_uint_type);})){
void*_stmttmp12=e2->r;void*_tmp94=_stmttmp12;if(((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp94)->tag == 40U){_LL6: _LL7:
 return e2;}else{_LL8: _LL9:
 goto _LL5;}_LL5:;}
# 323
goto _LL0;}}else{_LL3: _LL4:
# 330
 goto _LL0;}_LL0:;}
# 332
return e;}struct _tuple18{void*f1;void*f2;};
# 335
int Cyc_Evexp_const_exp_cmp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 337
e1=({Cyc_Evexp_strip_cast(e1);});
e2=({Cyc_Evexp_strip_cast(e2);});{
struct _tuple12 _stmttmp13=({Cyc_Evexp_eval_const_uint_exp(e1);});int _tmp97;unsigned _tmp96;_LL1: _tmp96=_stmttmp13.f1;_tmp97=_stmttmp13.f2;_LL2: {unsigned i1=_tmp96;int known1=_tmp97;
struct _tuple12 _stmttmp14=({Cyc_Evexp_eval_const_uint_exp(e2);});int _tmp99;unsigned _tmp98;_LL4: _tmp98=_stmttmp14.f1;_tmp99=_stmttmp14.f2;_LL5: {unsigned i2=_tmp98;int known2=_tmp99;
if(known1 && known2)
return(int)(i1 - i2);{
# 341
int e1case=({Cyc_Evexp_const_exp_case_number(e1);});
# 344
int e2case=({Cyc_Evexp_const_exp_case_number(e2);});
if(e1case != e2case)
return e1case - e2case;{
# 345
struct _tuple18 _stmttmp15=({struct _tuple18 _tmp11C;_tmp11C.f1=e1->r,_tmp11C.f2=e2->r;_tmp11C;});struct _tuple18 _tmp9A=_stmttmp15;void*_tmp9C;void*_tmp9B;struct Cyc_Absyn_Exp*_tmpA0;void*_tmp9F;struct Cyc_Absyn_Exp*_tmp9E;void*_tmp9D;struct Cyc_List_List*_tmpA4;void*_tmpA3;struct Cyc_List_List*_tmpA2;void*_tmpA1;struct Cyc_Absyn_Exp*_tmpA6;struct Cyc_Absyn_Exp*_tmpA5;void*_tmpA8;struct Cyc_Absyn_Exp*_tmpA7;struct Cyc_Absyn_Exp*_tmpAA;void*_tmpA9;void*_tmpAC;void*_tmpAB;struct Cyc_List_List*_tmpB0;enum Cyc_Absyn_Primop _tmpAF;struct Cyc_List_List*_tmpAE;enum Cyc_Absyn_Primop _tmpAD;struct Cyc_Absyn_Exp*_tmpB4;struct Cyc_Absyn_Exp*_tmpB3;struct Cyc_Absyn_Exp*_tmpB2;struct Cyc_Absyn_Exp*_tmpB1;struct Cyc_Absyn_Exp*_tmpB8;struct Cyc_Absyn_Exp*_tmpB7;struct Cyc_Absyn_Exp*_tmpB6;struct Cyc_Absyn_Exp*_tmpB5;struct Cyc_Absyn_Exp*_tmpBE;struct Cyc_Absyn_Exp*_tmpBD;struct Cyc_Absyn_Exp*_tmpBC;struct Cyc_Absyn_Exp*_tmpBB;struct Cyc_Absyn_Exp*_tmpBA;struct Cyc_Absyn_Exp*_tmpB9;switch(*((int*)_tmp9A.f1)){case 6U: if(((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->tag == 6U){_LL7: _tmpB9=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmpBA=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f2;_tmpBB=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f3;_tmpBC=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_tmpBD=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f2;_tmpBE=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f3;_LL8: {struct Cyc_Absyn_Exp*e11=_tmpB9;struct Cyc_Absyn_Exp*e12=_tmpBA;struct Cyc_Absyn_Exp*e13=_tmpBB;struct Cyc_Absyn_Exp*e21=_tmpBC;struct Cyc_Absyn_Exp*e22=_tmpBD;struct Cyc_Absyn_Exp*e23=_tmpBE;
# 352
int c1=({Cyc_Evexp_const_exp_cmp(e13,e23);});
if(c1 != 0)return c1;_tmpB5=e11;_tmpB6=e12;_tmpB7=e21;_tmpB8=e22;goto _LLA;}}else{goto _LL1D;}case 7U: if(((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->tag == 7U){_LL9: _tmpB5=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmpB6=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f2;_tmpB7=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_tmpB8=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f2;_LLA: {struct Cyc_Absyn_Exp*e11=_tmpB5;struct Cyc_Absyn_Exp*e12=_tmpB6;struct Cyc_Absyn_Exp*e21=_tmpB7;struct Cyc_Absyn_Exp*e22=_tmpB8;
# 355
_tmpB1=e11;_tmpB2=e12;_tmpB3=e21;_tmpB4=e22;goto _LLC;}}else{goto _LL1D;}case 8U: if(((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->tag == 8U){_LLB: _tmpB1=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmpB2=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f2;_tmpB3=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_tmpB4=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f2;_LLC: {struct Cyc_Absyn_Exp*e11=_tmpB1;struct Cyc_Absyn_Exp*e12=_tmpB2;struct Cyc_Absyn_Exp*e21=_tmpB3;struct Cyc_Absyn_Exp*e22=_tmpB4;
# 357
int c1=({Cyc_Evexp_const_exp_cmp(e11,e21);});
if(c1 != 0)return c1;return({Cyc_Evexp_const_exp_cmp(e12,e22);});}}else{goto _LL1D;}case 3U: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->tag == 3U){_LLD: _tmpAD=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmpAE=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f2;_tmpAF=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_tmpB0=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f2;_LLE: {enum Cyc_Absyn_Primop p1=_tmpAD;struct Cyc_List_List*es1=_tmpAE;enum Cyc_Absyn_Primop p2=_tmpAF;struct Cyc_List_List*es2=_tmpB0;
# 361
int c1=(int)p1 - (int)p2;
if(c1 != 0)return c1;for(0;
es1 != 0 && es2 != 0;(es1=es1->tl,es2=es2->tl)){
int c2=({Cyc_Evexp_const_exp_cmp((struct Cyc_Absyn_Exp*)es1->hd,(struct Cyc_Absyn_Exp*)es2->hd);});
if(c2 != 0)
return c2;}
# 368
return 0;}}else{goto _LL1D;}case 17U: switch(*((int*)_tmp9A.f2)){case 17U: _LLF: _tmpAB=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmpAC=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_LL10: {void*t1=_tmpAB;void*t2=_tmpAC;
# 370
return({Cyc_Tcutil_typecmp(t1,t2);});}case 18U: _LL11: _tmpA9=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmpAA=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_LL12: {void*t1=_tmpA9;struct Cyc_Absyn_Exp*e2a=_tmpAA;
# 372
void*e2atopt=e2a->topt;
if(e2atopt == 0)
({void*_tmpBF=0U;({unsigned _tmp149=e2->loc;struct _fat_ptr _tmp148=({const char*_tmpC0="cannot handle sizeof(exp) here -- use sizeof(type)";_tag_fat(_tmpC0,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp149,_tmp148,_tag_fat(_tmpBF,sizeof(void*),0U));});});
# 373
return({Cyc_Tcutil_typecmp(t1,(void*)_check_null(e2atopt));});}default: goto _LL1D;}case 18U: switch(*((int*)_tmp9A.f2)){case 17U: _LL13: _tmpA7=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmpA8=(void*)((struct Cyc_Absyn_Sizeoftype_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_LL14: {struct Cyc_Absyn_Exp*e1a=_tmpA7;void*t2=_tmpA8;
# 377
void*e1atopt=e1a->topt;
if(e1atopt == 0)
({void*_tmpC1=0U;({unsigned _tmp14B=e1->loc;struct _fat_ptr _tmp14A=({const char*_tmpC2="cannot handle sizeof(exp) here -- use sizeof(type)";_tag_fat(_tmpC2,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp14B,_tmp14A,_tag_fat(_tmpC1,sizeof(void*),0U));});});
# 378
return({Cyc_Tcutil_typecmp((void*)_check_null(e1atopt),t2);});}case 18U: _LL15: _tmpA5=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmpA6=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_LL16: {struct Cyc_Absyn_Exp*e1a=_tmpA5;struct Cyc_Absyn_Exp*e2a=_tmpA6;
# 382
void*e1atopt=e1a->topt;
void*e2atopt=e2a->topt;
if(e1atopt == 0)
({void*_tmpC3=0U;({unsigned _tmp14D=e1->loc;struct _fat_ptr _tmp14C=({const char*_tmpC4="cannot handle sizeof(exp) here -- use sizeof(type)";_tag_fat(_tmpC4,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp14D,_tmp14C,_tag_fat(_tmpC3,sizeof(void*),0U));});});
# 384
if(e2atopt == 0)
# 387
({void*_tmpC5=0U;({unsigned _tmp14F=e2->loc;struct _fat_ptr _tmp14E=({const char*_tmpC6="cannot handle sizeof(exp) here -- use sizeof(type)";_tag_fat(_tmpC6,sizeof(char),51U);});Cyc_Tcutil_terr(_tmp14F,_tmp14E,_tag_fat(_tmpC5,sizeof(void*),0U));});});
# 384
return({({void*_tmp150=(void*)_check_null(e1atopt);Cyc_Tcutil_typecmp(_tmp150,(void*)_check_null(e2atopt));});});}default: goto _LL1D;}case 19U: if(((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->tag == 19U){_LL17: _tmpA1=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmpA2=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f2;_tmpA3=(void*)((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_tmpA4=((struct Cyc_Absyn_Offsetof_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f2;_LL18: {void*t1=_tmpA1;struct Cyc_List_List*f1=_tmpA2;void*t2=_tmpA3;struct Cyc_List_List*f2=_tmpA4;
# 390
int c1=({Cyc_Tcutil_typecmp(t1,t2);});
if(c1 != 0)return c1;{int l1=({Cyc_List_length(f1);});
# 393
int l2=({Cyc_List_length(f2);});
if(l1 < l2)return - 1;if(l2 < l1)
return 1;
# 394
for(0;
# 396
f1 != 0 && f2 != 0;(f1=f1->tl,f2=f2->tl)){
struct _tuple18 _stmttmp16=({struct _tuple18 _tmp11B;_tmp11B.f1=(void*)f1->hd,_tmp11B.f2=(void*)f2->hd;_tmp11B;});struct _tuple18 _tmpC7=_stmttmp16;unsigned _tmpC9;unsigned _tmpC8;struct _fat_ptr*_tmpCB;struct _fat_ptr*_tmpCA;if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmpC7.f1)->tag == 0U){if(((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmpC7.f2)->tag == 1U){_LL20: _LL21:
 return - 1;}else{_LL24: _tmpCA=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmpC7.f1)->f1;_tmpCB=((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmpC7.f2)->f1;_LL25: {struct _fat_ptr*fn1=_tmpCA;struct _fat_ptr*fn2=_tmpCB;
# 401
int c=({Cyc_strptrcmp(fn1,fn2);});
if(c != 0)return c;goto _LL1F;}}}else{if(((struct Cyc_Absyn_StructField_Absyn_OffsetofField_struct*)_tmpC7.f2)->tag == 0U){_LL22: _LL23:
# 399
 return 1;}else{_LL26: _tmpC8=((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmpC7.f1)->f1;_tmpC9=((struct Cyc_Absyn_TupleIndex_Absyn_OffsetofField_struct*)_tmpC7.f2)->f1;_LL27: {unsigned i1=_tmpC8;unsigned i2=_tmpC9;
# 405
int c=(int)(i1 - i2);
if(c != 0)return c;goto _LL1F;}}}_LL1F:;}
# 409
return 0;}}}else{goto _LL1D;}case 14U: if(((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->tag == 14U){_LL19: _tmp9D=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmp9E=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f2;_tmp9F=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_tmpA0=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f2;_LL1A: {void*t1=_tmp9D;struct Cyc_Absyn_Exp*e1a=_tmp9E;void*t2=_tmp9F;struct Cyc_Absyn_Exp*e2a=_tmpA0;
# 411
int c1=({Cyc_Tcutil_typecmp(t1,t2);});
if(c1 != 0)return c1;return({Cyc_Evexp_const_exp_cmp(e1a,e2a);});}}else{goto _LL1D;}case 40U: if(((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->tag == 40U){_LL1B: _tmp9B=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp9A.f1)->f1;_tmp9C=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp9A.f2)->f1;_LL1C: {void*t1=_tmp9B;void*t2=_tmp9C;
# 415
if(({Cyc_Unify_unify(t1,t2);}))return 0;return({Cyc_Tcutil_typecmp(t1,t2);});}}else{goto _LL1D;}default: _LL1D: _LL1E:
# 417
({void*_tmpCC=0U;({int(*_tmp152)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpCE)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpCE;});struct _fat_ptr _tmp151=({const char*_tmpCD="Evexp::const_exp_cmp, unexpected case";_tag_fat(_tmpCD,sizeof(char),38U);});_tmp152(_tmp151,_tag_fat(_tmpCC,sizeof(void*),0U));});});}_LL6:;}}}}}}
# 335
int Cyc_Evexp_same_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 421
return({Cyc_Evexp_const_exp_cmp(e1,e2);})== 0;}
# 335
int Cyc_Evexp_lte_const_exp(struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
# 424
struct _tuple12 _stmttmp17=({Cyc_Evexp_eval_const_uint_exp(e1);});int _tmpD2;unsigned _tmpD1;_LL1: _tmpD1=_stmttmp17.f1;_tmpD2=_stmttmp17.f2;_LL2: {unsigned i1=_tmpD1;int known1=_tmpD2;
struct _tuple12 _stmttmp18=({Cyc_Evexp_eval_const_uint_exp(e2);});int _tmpD4;unsigned _tmpD3;_LL4: _tmpD3=_stmttmp18.f1;_tmpD4=_stmttmp18.f2;_LL5: {unsigned i2=_tmpD3;int known2=_tmpD4;
if(known1 && known2)
return i1 <= i2;
# 426
return({Cyc_Evexp_same_const_exp(e1,e2);});}}}struct _tuple19{struct Cyc_Absyn_Tqual f1;void*f2;};
# 335
int Cyc_Evexp_okay_szofarg(void*t){
# 434
void*_stmttmp19=({Cyc_Tcutil_compress(t);});void*_tmpD6=_stmttmp19;struct Cyc_Absyn_Typedefdecl*_tmpD7;struct Cyc_List_List*_tmpD8;struct Cyc_Absyn_Exp*_tmpDA;void*_tmpD9;struct Cyc_List_List*_tmpDB;struct Cyc_Absyn_Tvar*_tmpDC;struct Cyc_List_List*_tmpDE;void*_tmpDD;switch(*((int*)_tmpD6)){case 0U: _LL1: _tmpDD=(void*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpD6)->f1;_tmpDE=((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmpD6)->f2;_LL2: {void*c=_tmpDD;struct Cyc_List_List*ts=_tmpDE;
# 436
void*_tmpDF=c;struct Cyc_Absyn_Aggrdecl*_tmpE0;struct Cyc_Absyn_Datatypefield*_tmpE1;switch(*((int*)_tmpDF)){case 21U: if(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmpDF)->f1).KnownDatatypefield).tag == 2){_LL1C: _tmpE1=(((((struct Cyc_Absyn_DatatypeFieldCon_Absyn_TyCon_struct*)_tmpDF)->f1).KnownDatatypefield).val).f2;_LL1D: {struct Cyc_Absyn_Datatypefield*tuf=_tmpE1;
# 438
{struct Cyc_List_List*tqs=tuf->typs;for(0;tqs != 0;tqs=tqs->tl){
if(!({Cyc_Evexp_okay_szofarg((*((struct _tuple19*)tqs->hd)).f2);}))
return 0;}}
# 438
return 1;}}else{_LL1E: _LL1F:
# 442
 return 0;}case 5U: _LL20: _LL21:
 goto _LL23;case 1U: _LL22: _LL23:
 goto _LL25;case 2U: _LL24: _LL25:
 goto _LL27;case 19U: _LL26: _LL27:
 goto _LL29;case 17U: _LL28: _LL29:
 goto _LL2B;case 3U: _LL2A: _LL2B:
 goto _LL2D;case 4U: _LL2C: _LL2D:
 goto _LL2F;case 20U: _LL2E: _LL2F:
 goto _LL31;case 18U: _LL30: _LL31:
 return 1;case 0U: _LL32: _LL33:
# 453
 goto _LL35;case 6U: _LL34: _LL35:
 goto _LL37;case 8U: _LL36: _LL37:
 goto _LL39;case 7U: _LL38: _LL39:
 goto _LL3B;case 11U: _LL3A: _LL3B:
 goto _LL3D;case 9U: _LL3C: _LL3D:
 goto _LL3F;case 10U: _LL3E: _LL3F:
 goto _LL41;case 13U: _LL40: _LL41:
 goto _LL43;case 14U: _LL42: _LL43:
 goto _LL45;case 15U: _LL44: _LL45:
 goto _LL47;case 16U: _LL46: _LL47:
 goto _LL49;case 12U: _LL48: _LL49:
 return 0;default: if(((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmpDF)->f1).UnknownAggr).tag == 1){_LL4A: _LL4B:
# 466
({void*_tmpE2=0U;({int(*_tmp154)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpE4)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpE4;});struct _fat_ptr _tmp153=({const char*_tmpE3="szof on unchecked StructType";_tag_fat(_tmpE3,sizeof(char),29U);});_tmp154(_tmp153,_tag_fat(_tmpE2,sizeof(void*),0U));});});}else{_LL4C: _tmpE0=*((((struct Cyc_Absyn_AggrCon_Absyn_TyCon_struct*)_tmpDF)->f1).KnownAggr).val;_LL4D: {struct Cyc_Absyn_Aggrdecl*ad=_tmpE0;
# 468
if(ad->impl == 0)return 0;else{
# 470
struct _RegionHandle _tmpE5=_new_region("temp");struct _RegionHandle*temp=& _tmpE5;_push_region(temp);
{struct Cyc_List_List*inst=({Cyc_List_rzip(temp,temp,ad->tvs,ts);});
{struct Cyc_List_List*fs=((struct Cyc_Absyn_AggrdeclImpl*)_check_null(ad->impl))->fields;for(0;fs != 0;fs=fs->tl){
if(!({int(*_tmpE6)(void*t)=Cyc_Evexp_okay_szofarg;void*_tmpE7=({Cyc_Tcutil_rsubstitute(temp,inst,((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);});_tmpE6(_tmpE7);})){int _tmpE8=0;_npop_handler(0U);return _tmpE8;}}}{
# 472
int _tmpE9=1;_npop_handler(0U);return _tmpE9;}}
# 471
;_pop_region();}}}}_LL1B:;}case 2U: _LL3: _tmpDC=((struct Cyc_Absyn_VarType_Absyn_Type_struct*)_tmpD6)->f1;_LL4: {struct Cyc_Absyn_Tvar*tv=_tmpDC;
# 478
enum Cyc_Absyn_KindQual _stmttmp1A=({Cyc_Tcutil_tvar_kind(tv,& Cyc_Tcutil_bk);})->kind;enum Cyc_Absyn_KindQual _tmpEA=_stmttmp1A;if(_tmpEA == Cyc_Absyn_BoxKind){_LL4F: _LL50:
 return 1;}else{_LL51: _LL52:
 return 0;}_LL4E:;}case 1U: if(((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmpD6)->f1 != 0){if(((struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)((struct Cyc_Absyn_Evar_Absyn_Type_struct*)_tmpD6)->f1)->v)->kind == Cyc_Absyn_BoxKind){_LL5: _LL6:
# 482
 return 1;}else{goto _LL7;}}else{_LL7: _LL8:
 return 0;}case 6U: _LL9: _tmpDB=((struct Cyc_Absyn_TupleType_Absyn_Type_struct*)_tmpD6)->f1;_LLA: {struct Cyc_List_List*tqs=_tmpDB;
# 485
for(0;tqs != 0;tqs=tqs->tl){
if(!({Cyc_Evexp_okay_szofarg((*((struct _tuple19*)tqs->hd)).f2);}))
return 0;}
# 485
return 1;}case 3U: _LLB: _LLC:
# 491
 return 1;case 4U: _LLD: _tmpD9=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpD6)->f1).elt_type;_tmpDA=(((struct Cyc_Absyn_ArrayType_Absyn_Type_struct*)_tmpD6)->f1).num_elts;_LLE: {void*t2=_tmpD9;struct Cyc_Absyn_Exp*e=_tmpDA;
# 494
return e != 0;}case 5U: _LLF: _LL10:
 return 0;case 7U: _LL11: _tmpD8=((struct Cyc_Absyn_AnonAggrType_Absyn_Type_struct*)_tmpD6)->f2;_LL12: {struct Cyc_List_List*fs=_tmpD8;
# 497
for(0;fs != 0;fs=fs->tl){
if(!({Cyc_Evexp_okay_szofarg(((struct Cyc_Absyn_Aggrfield*)fs->hd)->type);}))
return 0;}
# 497
return 1;}case 10U: _LL13: _LL14:
# 501
 goto _LL16;case 9U: _LL15: _LL16:
 goto _LL18;case 11U: _LL17: _LL18:
 return 0;default: _LL19: _tmpD7=((struct Cyc_Absyn_TypedefType_Absyn_Type_struct*)_tmpD6)->f3;_LL1A: {struct Cyc_Absyn_Typedefdecl*td=_tmpD7;
# 506
if(td == 0 || td->kind == 0)
({void*_tmpEB=0U;({int(*_tmp158)(struct _fat_ptr,struct _fat_ptr)=({int(*_tmpEF)(struct _fat_ptr,struct _fat_ptr)=(int(*)(struct _fat_ptr,struct _fat_ptr))Cyc_Tcutil_impos;_tmpEF;});struct _fat_ptr _tmp157=(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmpEE=({struct Cyc_String_pa_PrintArg_struct _tmp11D;_tmp11D.tag=0U,({struct _fat_ptr _tmp155=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_typ2string(t);}));_tmp11D.f1=_tmp155;});_tmp11D;});void*_tmpEC[1U];_tmpEC[0]=& _tmpEE;({struct _fat_ptr _tmp156=({const char*_tmpED="szof typedeftype %s";_tag_fat(_tmpED,sizeof(char),20U);});Cyc_aprintf(_tmp156,_tag_fat(_tmpEC,sizeof(void*),1U));});});_tmp158(_tmp157,_tag_fat(_tmpEB,sizeof(void*),0U));});});
# 506
return(int)((struct Cyc_Absyn_Kind*)((struct Cyc_Core_Opt*)_check_null(td->kind))->v)->kind == (int)Cyc_Absyn_BoxKind;}}_LL0:;}
