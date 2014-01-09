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
 struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Core_Opt{void*v;};extern char Cyc_Core_Invalid_argument[17U];struct Cyc_Core_Invalid_argument_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Failure[8U];struct Cyc_Core_Failure_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Impossible[11U];struct Cyc_Core_Impossible_exn_struct{char*tag;struct _fat_ptr f1;};extern char Cyc_Core_Not_found[10U];struct Cyc_Core_Not_found_exn_struct{char*tag;};extern char Cyc_Core_Unreachable[12U];struct Cyc_Core_Unreachable_exn_struct{char*tag;struct _fat_ptr f1;};
# 168 "core.h"
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 54 "list.h"
struct Cyc_List_List*Cyc_List_list(struct _fat_ptr);
# 61
int Cyc_List_length(struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 133
void Cyc_List_iter(void(*f)(void*),struct Cyc_List_List*x);
# 172
struct Cyc_List_List*Cyc_List_rev(struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct _tuple0{struct Cyc_List_List*f1;struct Cyc_List_List*f2;};
# 294
struct _tuple0 Cyc_List_split(struct Cyc_List_List*x);struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};
# 49 "string.h"
int Cyc_strcmp(struct _fat_ptr s1,struct _fat_ptr s2);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple1{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 1058 "absyn.h"
struct Cyc_Absyn_Exp*Cyc_Absyn_new_exp(void*,unsigned);
# 1060
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*);
# 1074
struct Cyc_Absyn_Exp*Cyc_Absyn_var_exp(struct _tuple1*,unsigned);
# 1114
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,unsigned);
# 1129
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
# 1141
struct Cyc_Absyn_Stmt*Cyc_Absyn_declare_stmt(struct _tuple1*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned);struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 67 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
struct _fat_ptr Cyc_Absynpp_stmt2string(struct Cyc_Absyn_Stmt*);
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple1*);
# 40 "warn.h"
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple1*f1;};struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 278 "tcutil.h"
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);struct Cyc_Hashtable_Table;
# 38 "toc.h"
void*Cyc_Toc_typ_to_c(void*);
# 40
struct _tuple1*Cyc_Toc_temp_var();extern char Cyc_Toc_Dest[5U];struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};
# 35 "toseqc.cyc"
enum Cyc_Toseqc_SideEffect{Cyc_Toseqc_Const =0U,Cyc_Toseqc_NoEffect =1U,Cyc_Toseqc_ExnEffect =2U,Cyc_Toseqc_AnyEffect =3U};struct _tuple12{enum Cyc_Toseqc_SideEffect f1;enum Cyc_Toseqc_SideEffect f2;};
# 48 "toseqc.cyc"
static enum Cyc_Toseqc_SideEffect Cyc_Toseqc_join_side_effect(enum Cyc_Toseqc_SideEffect e1,enum Cyc_Toseqc_SideEffect e2){
struct _tuple12 _stmttmp0=({struct _tuple12 _tmpDE;_tmpDE.f1=e1,_tmpDE.f2=e2;_tmpDE;});struct _tuple12 _tmp0=_stmttmp0;if(_tmp0.f1 == Cyc_Toseqc_AnyEffect){_LL1: _LL2:
 goto _LL4;}else{if(_tmp0.f2 == Cyc_Toseqc_AnyEffect){_LL3: _LL4:
 return Cyc_Toseqc_AnyEffect;}else{if(_tmp0.f1 == Cyc_Toseqc_ExnEffect){if(_tmp0.f2 == Cyc_Toseqc_ExnEffect){_LL5: _LL6:
# 53
 return Cyc_Toseqc_AnyEffect;}else{_LL7: _LL8:
 goto _LLA;}}else{if(_tmp0.f2 == Cyc_Toseqc_ExnEffect){_LL9: _LLA:
 return Cyc_Toseqc_ExnEffect;}else{if(_tmp0.f1 == Cyc_Toseqc_NoEffect){_LLB: _LLC:
 goto _LLE;}else{if(_tmp0.f2 == Cyc_Toseqc_NoEffect){_LLD: _LLE:
 return Cyc_Toseqc_NoEffect;}else{if(_tmp0.f1 == Cyc_Toseqc_Const){if(_tmp0.f2 == Cyc_Toseqc_Const){_LLF: _LL10:
 return Cyc_Toseqc_Const;}else{goto _LL11;}}else{_LL11: _LL12:
({void*_tmp1=0U;({int(*_tmpE6)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp3)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp3;});struct _fat_ptr _tmpE5=({const char*_tmp2="join_side_effect";_tag_fat(_tmp2,sizeof(char),17U);});_tmpE6(_tmpE5,_tag_fat(_tmp1,sizeof(void*),0U));});});}}}}}}}_LL0:;}static char _tmp5[14U]="_get_fat_size";static char _tmp6[24U]="_get_zero_arr_size_char";static char _tmp7[25U]="_get_zero_arr_size_short";static char _tmp8[23U]="_get_zero_arr_size_int";static char _tmp9[25U]="_get_zero_arr_size_float";static char _tmpA[26U]="_get_zero_arr_size_double";static char _tmpB[30U]="_get_zero_arr_size_longdouble";static char _tmpC[28U]="_get_zero_arr_size_voidstar";
# 48
static struct _fat_ptr Cyc_Toseqc_pure_funs[8U]={{_tmp5,_tmp5,_tmp5 + 14U},{_tmp6,_tmp6,_tmp6 + 24U},{_tmp7,_tmp7,_tmp7 + 25U},{_tmp8,_tmp8,_tmp8 + 23U},{_tmp9,_tmp9,_tmp9 + 25U},{_tmpA,_tmpA,_tmpA + 26U},{_tmpB,_tmpB,_tmpB + 30U},{_tmpC,_tmpC,_tmpC + 28U}};static char _tmpD[12U]="_check_null";static char _tmpE[28U]="_check_known_subscript_null";static char _tmpF[31U]="_check_known_subscript_notnull";static char _tmp10[21U]="_check_fat_subscript";static char _tmp11[15U]="_untag_fat_ptr";static char _tmp12[15U]="_region_malloc";
# 75
static struct _fat_ptr Cyc_Toseqc_exn_effect_funs[6U]={{_tmpD,_tmpD,_tmpD + 12U},{_tmpE,_tmpE,_tmpE + 28U},{_tmpF,_tmpF,_tmpF + 31U},{_tmp10,_tmp10,_tmp10 + 21U},{_tmp11,_tmp11,_tmp11 + 15U},{_tmp12,_tmp12,_tmp12 + 15U}};
# 83
static enum Cyc_Toseqc_SideEffect Cyc_Toseqc_fun_effect(struct _fat_ptr fn){
{int i=0;for(0;(unsigned)i < 8U;++ i){
if(!({Cyc_strcmp((struct _fat_ptr)Cyc_Toseqc_pure_funs[i],(struct _fat_ptr)fn);}))
return Cyc_Toseqc_NoEffect;}}
# 84
{
# 87
int i=0;
# 84
for(0;(unsigned)i < 6U;++ i){
# 88
if(!({Cyc_strcmp((struct _fat_ptr)Cyc_Toseqc_exn_effect_funs[i],(struct _fat_ptr)fn);}))
return Cyc_Toseqc_ExnEffect;}}
# 84
return Cyc_Toseqc_AnyEffect;}
# 83
static enum Cyc_Toseqc_SideEffect Cyc_Toseqc_exp_effect(struct Cyc_Absyn_Exp*e);
# 94
static enum Cyc_Toseqc_SideEffect Cyc_Toseqc_stmt_effect(struct Cyc_Absyn_Stmt*s);
static enum Cyc_Toseqc_SideEffect Cyc_Toseqc_exps_effect(struct Cyc_List_List*es){
enum Cyc_Toseqc_SideEffect res=0U;
# 99
{struct Cyc_List_List*x=es;for(0;x != 0;x=x->tl){
enum Cyc_Toseqc_SideEffect eres=({Cyc_Toseqc_exp_effect((struct Cyc_Absyn_Exp*)x->hd);});
res=({Cyc_Toseqc_join_side_effect(eres,res);});
# 104
if((int)res == (int)3U)
# 106
return res;}}
# 110
return res;}
# 114
static enum Cyc_Toseqc_SideEffect Cyc_Toseqc_exp_effect(struct Cyc_Absyn_Exp*e){
struct _fat_ptr bad_form=({const char*_tmp54="";_tag_fat(_tmp54,sizeof(char),1U);});
{void*_stmttmp1=e->r;void*_tmp15=_stmttmp1;struct Cyc_Absyn_Stmt*_tmp16;struct Cyc_Absyn_Exp*_tmp17;struct Cyc_Absyn_Exp*_tmp18;struct Cyc_Absyn_Exp*_tmp19;struct Cyc_Absyn_Exp*_tmp1A;struct Cyc_Absyn_Exp*_tmp1B;struct Cyc_Absyn_Exp*_tmp1C;struct Cyc_Absyn_Exp*_tmp1D;struct Cyc_Absyn_Exp*_tmp1E;struct Cyc_Absyn_Exp*_tmp1F;struct Cyc_Absyn_Exp*_tmp21;struct Cyc_Absyn_Exp*_tmp20;struct Cyc_Absyn_Exp*_tmp23;struct Cyc_Absyn_Exp*_tmp22;struct Cyc_Absyn_Exp*_tmp25;struct Cyc_Absyn_Exp*_tmp24;struct Cyc_Absyn_Exp*_tmp27;struct Cyc_Absyn_Exp*_tmp26;struct Cyc_Absyn_Exp*_tmp2A;struct Cyc_Absyn_Exp*_tmp29;struct Cyc_Absyn_Exp*_tmp28;struct Cyc_List_List*_tmp2B;struct Cyc_List_List*_tmp2C;struct Cyc_Absyn_Exp*_tmp2D;switch(*((int*)_tmp15)){case 0U: _LL1: _LL2:
 goto _LL4;case 18U: _LL3: _LL4:
 goto _LL6;case 17U: _LL5: _LL6:
 goto _LL8;case 19U: _LL7: _LL8:
 goto _LLA;case 33U: _LL9: _LLA:
 goto _LLC;case 32U: _LLB: _LLC:
 return Cyc_Toseqc_Const;case 1U: _LLD: _LLE:
 return Cyc_Toseqc_NoEffect;case 4U: _LLF: _LL10:
# 125
 goto _LL12;case 41U: _LL11: _LL12:
 return Cyc_Toseqc_AnyEffect;case 10U: _LL13: _tmp2D=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL14: {struct Cyc_Absyn_Exp*e=_tmp2D;
# 129
void*_stmttmp2=e->r;void*_tmp2E=_stmttmp2;struct _tuple1*_tmp2F;struct Cyc_Absyn_Vardecl*_tmp30;struct Cyc_Absyn_Fndecl*_tmp31;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2E)->tag == 1U)switch(*((int*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2E)->f1)){case 2U: _LL58: _tmp31=((struct Cyc_Absyn_Funname_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2E)->f1)->f1;_LL59: {struct Cyc_Absyn_Fndecl*d=_tmp31;
return({enum Cyc_Toseqc_SideEffect(*_tmp32)(struct _fat_ptr fn)=Cyc_Toseqc_fun_effect;struct _fat_ptr _tmp33=({Cyc_Absynpp_qvar2string(d->name);});_tmp32(_tmp33);});}case 1U: _LL5A: _tmp30=((struct Cyc_Absyn_Global_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2E)->f1)->f1;_LL5B: {struct Cyc_Absyn_Vardecl*vd=_tmp30;
return({enum Cyc_Toseqc_SideEffect(*_tmp34)(struct _fat_ptr fn)=Cyc_Toseqc_fun_effect;struct _fat_ptr _tmp35=({Cyc_Absynpp_qvar2string(vd->name);});_tmp34(_tmp35);});}case 0U: _LL5C: _tmp2F=((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp2E)->f1)->f1;_LL5D: {struct _tuple1*v=_tmp2F;
return({enum Cyc_Toseqc_SideEffect(*_tmp36)(struct _fat_ptr fn)=Cyc_Toseqc_fun_effect;struct _fat_ptr _tmp37=({Cyc_Absynpp_qvar2string(v);});_tmp36(_tmp37);});}default: goto _LL5E;}else{_LL5E: _LL5F:
# 134
 return Cyc_Toseqc_AnyEffect;}_LL57:;}case 37U: _LL15: _tmp2C=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp15)->f2;_LL16: {struct Cyc_List_List*dles=_tmp2C;
# 138
struct _tuple0 _stmttmp3=({Cyc_List_split(dles);});struct Cyc_List_List*_tmp38;_LL61: _tmp38=_stmttmp3.f2;_LL62: {struct Cyc_List_List*es=_tmp38;
_tmp2B=es;goto _LL18;}}case 3U: _LL17: _tmp2B=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp15)->f2;_LL18: {struct Cyc_List_List*es=_tmp2B;
# 142
return({Cyc_Toseqc_exps_effect(es);});}case 6U: _LL19: _tmp28=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_tmp29=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp15)->f2;_tmp2A=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp15)->f3;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp28;struct Cyc_Absyn_Exp*e2=_tmp29;struct Cyc_Absyn_Exp*e3=_tmp2A;
# 145
return({enum Cyc_Toseqc_SideEffect(*_tmp39)(struct Cyc_List_List*es)=Cyc_Toseqc_exps_effect;struct Cyc_List_List*_tmp3A=({struct Cyc_Absyn_Exp*_tmp3B[3U];_tmp3B[0]=e1,_tmp3B[1]=e2,_tmp3B[2]=e3;Cyc_List_list(_tag_fat(_tmp3B,sizeof(struct Cyc_Absyn_Exp*),3U));});_tmp39(_tmp3A);});}case 23U: _LL1B: _tmp26=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_tmp27=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp15)->f2;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp26;struct Cyc_Absyn_Exp*e2=_tmp27;
# 147
_tmp24=e1;_tmp25=e2;goto _LL1E;}case 7U: _LL1D: _tmp24=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_tmp25=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp15)->f2;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp24;struct Cyc_Absyn_Exp*e2=_tmp25;
_tmp22=e1;_tmp23=e2;goto _LL20;}case 8U: _LL1F: _tmp22=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_tmp23=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp15)->f2;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp22;struct Cyc_Absyn_Exp*e2=_tmp23;
_tmp20=e1;_tmp21=e2;goto _LL22;}case 9U: _LL21: _tmp20=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_tmp21=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp15)->f2;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp20;struct Cyc_Absyn_Exp*e2=_tmp21;
# 151
return({enum Cyc_Toseqc_SideEffect(*_tmp3C)(struct Cyc_List_List*es)=Cyc_Toseqc_exps_effect;struct Cyc_List_List*_tmp3D=({struct Cyc_Absyn_Exp*_tmp3E[2U];_tmp3E[0]=e1,_tmp3E[1]=e2;Cyc_List_list(_tag_fat(_tmp3E,sizeof(struct Cyc_Absyn_Exp*),2U));});_tmp3C(_tmp3D);});}case 42U: _LL23: _tmp1F=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp1F;
# 153
_tmp1E=e1;goto _LL26;}case 12U: _LL25: _tmp1E=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp1E;
_tmp1D=e1;goto _LL28;}case 13U: _LL27: _tmp1D=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp1D;
_tmp1C=e1;goto _LL2A;}case 14U: _LL29: _tmp1C=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp15)->f2;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp1C;
_tmp1B=e1;goto _LL2C;}case 15U: _LL2B: _tmp1B=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL2C: {struct Cyc_Absyn_Exp*e1=_tmp1B;
_tmp1A=e1;goto _LL2E;}case 20U: _LL2D: _tmp1A=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp1A;
_tmp19=e1;goto _LL30;}case 21U: _LL2F: _tmp19=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp19;
_tmp18=e1;goto _LL32;}case 22U: _LL31: _tmp18=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp18;
_tmp17=e1;goto _LL34;}case 5U: _LL33: _tmp17=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp17;
# 162
return({Cyc_Toseqc_exp_effect(e1);});}case 38U: _LL35: _tmp16=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp15)->f1;_LL36: {struct Cyc_Absyn_Stmt*s=_tmp16;
# 164
return({Cyc_Toseqc_stmt_effect(s);});}case 34U: _LL37: _LL38:
 bad_form=({const char*_tmp3F="Spawn_e";_tag_fat(_tmp3F,sizeof(char),8U);});goto _LL0;case 2U: _LL39: _LL3A:
 bad_form=({const char*_tmp40="Pragma_e";_tag_fat(_tmp40,sizeof(char),9U);});goto _LL0;case 11U: _LL3B: _LL3C:
 bad_form=({const char*_tmp41="Throw_e";_tag_fat(_tmp41,sizeof(char),8U);});goto _LL0;case 16U: _LL3D: _LL3E:
 bad_form=({const char*_tmp42="New_e";_tag_fat(_tmp42,sizeof(char),6U);});goto _LL0;case 24U: _LL3F: _LL40:
 bad_form=({const char*_tmp43="Tuple_e";_tag_fat(_tmp43,sizeof(char),8U);});goto _LL0;case 25U: _LL41: _LL42:
 bad_form=({const char*_tmp44="CompoundLit_e";_tag_fat(_tmp44,sizeof(char),14U);});goto _LL0;case 26U: _LL43: _LL44:
 bad_form=({const char*_tmp45="Array_e";_tag_fat(_tmp45,sizeof(char),8U);});goto _LL0;case 27U: _LL45: _LL46:
 bad_form=({const char*_tmp46="Comprehension_e";_tag_fat(_tmp46,sizeof(char),16U);});goto _LL0;case 28U: _LL47: _LL48:
 bad_form=({const char*_tmp47="ComprehensionNoinit_e";_tag_fat(_tmp47,sizeof(char),22U);});goto _LL0;case 29U: _LL49: _LL4A:
 bad_form=({const char*_tmp48="Aggregate_e";_tag_fat(_tmp48,sizeof(char),12U);});goto _LL0;case 30U: _LL4B: _LL4C:
 bad_form=({const char*_tmp49="AnonStruct_e";_tag_fat(_tmp49,sizeof(char),13U);});goto _LL0;case 31U: _LL4D: _LL4E:
 bad_form=({const char*_tmp4A="Datatype_e";_tag_fat(_tmp4A,sizeof(char),11U);});goto _LL0;case 35U: _LL4F: _LL50:
 bad_form=({const char*_tmp4B="Malloc_e";_tag_fat(_tmp4B,sizeof(char),9U);});goto _LL0;case 36U: _LL51: _LL52:
 bad_form=({const char*_tmp4C="Swap_e";_tag_fat(_tmp4C,sizeof(char),7U);});goto _LL0;case 39U: _LL53: _LL54:
 bad_form=({const char*_tmp4D="Tagcheck_e";_tag_fat(_tmp4D,sizeof(char),11U);});goto _LL0;default: _LL55: _LL56:
 bad_form=({const char*_tmp4E="Valueof_e";_tag_fat(_tmp4E,sizeof(char),10U);});goto _LL0;}_LL0:;}
# 182
({struct Cyc_String_pa_PrintArg_struct _tmp52=({struct Cyc_String_pa_PrintArg_struct _tmpE0;_tmpE0.tag=0U,_tmpE0.f1=(struct _fat_ptr)((struct _fat_ptr)bad_form);_tmpE0;});struct Cyc_String_pa_PrintArg_struct _tmp53=({struct Cyc_String_pa_PrintArg_struct _tmpDF;_tmpDF.tag=0U,({
struct _fat_ptr _tmpE7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpDF.f1=_tmpE7;});_tmpDF;});void*_tmp4F[2U];_tmp4F[0]=& _tmp52,_tmp4F[1]=& _tmp53;({int(*_tmpE9)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 182
int(*_tmp51)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp51;});struct _fat_ptr _tmpE8=({const char*_tmp50="bad exp form %s (exp=|%s|) after xlation to C";_tag_fat(_tmp50,sizeof(char),46U);});_tmpE9(_tmpE8,_tag_fat(_tmp4F,sizeof(void*),2U));});});}
# 185
static enum Cyc_Toseqc_SideEffect Cyc_Toseqc_stmt_effect(struct Cyc_Absyn_Stmt*s){
enum Cyc_Toseqc_SideEffect res=0U;
while(1){
void*_stmttmp4=s->r;void*_tmp56=_stmttmp4;struct Cyc_Absyn_Stmt*_tmp58;struct Cyc_Absyn_Stmt*_tmp57;struct Cyc_Absyn_Stmt*_tmp59;struct Cyc_Absyn_Exp*_tmp5A;switch(*((int*)_tmp56)){case 0U: _LL1: _LL2:
 return res;case 1U: _LL3: _tmp5A=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmp56)->f1;_LL4: {struct Cyc_Absyn_Exp*e=_tmp5A;
return({enum Cyc_Toseqc_SideEffect(*_tmp5B)(enum Cyc_Toseqc_SideEffect e1,enum Cyc_Toseqc_SideEffect e2)=Cyc_Toseqc_join_side_effect;enum Cyc_Toseqc_SideEffect _tmp5C=res;enum Cyc_Toseqc_SideEffect _tmp5D=({Cyc_Toseqc_exp_effect(e);});_tmp5B(_tmp5C,_tmp5D);});}case 13U: _LL5: _tmp59=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmp56)->f2;_LL6: {struct Cyc_Absyn_Stmt*s2=_tmp59;
# 192
s=s2;continue;}case 2U: _LL7: _tmp57=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp56)->f1;_tmp58=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmp56)->f2;_LL8: {struct Cyc_Absyn_Stmt*s1=_tmp57;struct Cyc_Absyn_Stmt*s2=_tmp58;
# 194
res=({enum Cyc_Toseqc_SideEffect(*_tmp5E)(enum Cyc_Toseqc_SideEffect e1,enum Cyc_Toseqc_SideEffect e2)=Cyc_Toseqc_join_side_effect;enum Cyc_Toseqc_SideEffect _tmp5F=res;enum Cyc_Toseqc_SideEffect _tmp60=({Cyc_Toseqc_stmt_effect(s1);});_tmp5E(_tmp5F,_tmp60);});
s=s2;
continue;}case 6U: _LL9: _LLA:
# 198
 goto _LLC;case 7U: _LLB: _LLC:
 goto _LLE;case 8U: _LLD: _LLE:
 goto _LL10;case 3U: _LLF: _LL10:
 goto _LL12;case 4U: _LL11: _LL12:
 goto _LL14;case 5U: _LL13: _LL14:
 goto _LL16;case 9U: _LL15: _LL16:
 goto _LL18;case 14U: _LL17: _LL18:
 goto _LL1A;case 12U: _LL19: _LL1A:
 goto _LL1C;case 10U: _LL1B: _LL1C:
 return Cyc_Toseqc_AnyEffect;default: _LL1D: _LL1E:
({void*_tmp61=0U;({int(*_tmpEB)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp63)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp63;});struct _fat_ptr _tmpEA=({const char*_tmp62="bad stmt after xlation to C";_tag_fat(_tmp62,sizeof(char),28U);});_tmpEB(_tmpEA,_tag_fat(_tmp61,sizeof(void*),0U));});});}_LL0:;}}
# 215
static int Cyc_Toseqc_is_toc_var(struct Cyc_Absyn_Exp*e){
void*_stmttmp5=e->r;void*_tmp65=_stmttmp5;struct _tuple1*_tmp66;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp65)->tag == 1U){if(((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp65)->f1)->tag == 0U){_LL1: _tmp66=((struct Cyc_Absyn_Unresolved_b_Absyn_Binding_struct*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp65)->f1)->f1;_LL2: {struct _tuple1*v=_tmp66;
return 1;}}else{goto _LL3;}}else{_LL3: _LL4:
 return 0;}_LL0:;}
# 215
static void Cyc_Toseqc_stmt_to_seqc(struct Cyc_Absyn_Stmt*s);
# 223
static void Cyc_Toseqc_exp_to_seqc(struct Cyc_Absyn_Exp*s);
# 241 "toseqc.cyc"
struct Cyc_Absyn_Stmt*Cyc_Toseqc_exps_to_seqc(struct Cyc_Absyn_Exp*e,struct Cyc_List_List*es){
# 243
({({void(*_tmpEC)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=({void(*_tmp68)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x)=(void(*)(void(*f)(struct Cyc_Absyn_Exp*),struct Cyc_List_List*x))Cyc_List_iter;_tmp68;});_tmpEC(Cyc_Toseqc_exp_to_seqc,es);});});
# 246
if(({Cyc_List_length(es);})<= 1 ||(int)({Cyc_Toseqc_exps_effect(es);})!= (int)Cyc_Toseqc_AnyEffect)return 0;{struct Cyc_Absyn_Stmt*stmt=({struct Cyc_Absyn_Stmt*(*_tmp6A)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp6B=({Cyc_Absyn_copy_exp(e);});unsigned _tmp6C=e->loc;_tmp6A(_tmp6B,_tmp6C);});
# 251
struct Cyc_Absyn_Stmt*laststmt=stmt;
int did_skip=0;
int did_convert=0;
{struct Cyc_List_List*x=({Cyc_List_rev(es);});for(0;x != 0;x=x->tl){
struct Cyc_Absyn_Exp*e1=(struct Cyc_Absyn_Exp*)x->hd;
# 258
if(({Cyc_Tcutil_is_const_exp(e1);})||({Cyc_Toseqc_is_toc_var(e1);}))continue;if(!did_skip){
# 264
did_skip=1;
continue;}
# 258
did_convert=1;{
# 270
struct _tuple1*v=({Cyc_Toc_temp_var();});
struct Cyc_Absyn_Exp*ve=({Cyc_Absyn_var_exp(v,e1->loc);});
# 274
struct Cyc_Absyn_Exp*e2=({Cyc_Absyn_new_exp(e1->r,e1->loc);});
e2->annot=e1->annot;
e2->topt=e1->topt;{
# 278
void*t=e2->topt == 0?(void*)({struct Cyc_Absyn_TypeofType_Absyn_Type_struct*_tmp69=_cycalloc(sizeof(*_tmp69));_tmp69->tag=11U,_tmp69->f1=e2;_tmp69;}):({Cyc_Toc_typ_to_c((void*)_check_null(e2->topt));});
# 282
stmt=({Cyc_Absyn_declare_stmt(v,t,e2,stmt,e->loc);});
# 285
e1->r=ve->r;}}}}
# 289
if(did_convert){
({void*_tmpED=({Cyc_Absyn_stmt_exp(stmt,e->loc);})->r;e->r=_tmpED;});
return laststmt;}else{
# 293
return 0;}}}
# 299
static void Cyc_Toseqc_exp_to_seqc(struct Cyc_Absyn_Exp*e){
struct _fat_ptr bad_form=({const char*_tmpAC="";_tag_fat(_tmpAC,sizeof(char),1U);});
{void*_stmttmp6=e->r;void*_tmp6E=_stmttmp6;struct Cyc_Absyn_Stmt*_tmp6F;struct Cyc_List_List*_tmp70;struct Cyc_Absyn_Exp*_tmp71;struct Cyc_Absyn_Exp*_tmp72;struct Cyc_Absyn_Exp*_tmp73;struct Cyc_Absyn_Exp*_tmp74;struct Cyc_Absyn_Exp*_tmp75;struct Cyc_Absyn_Exp*_tmp76;struct Cyc_Absyn_Exp*_tmp77;struct Cyc_Absyn_Exp*_tmp78;struct Cyc_Absyn_Exp*_tmp79;struct Cyc_Absyn_Exp*_tmp7A;struct Cyc_Absyn_Exp*_tmp7C;struct Cyc_Absyn_Exp*_tmp7B;struct Cyc_Absyn_Exp*_tmp7E;struct Cyc_Absyn_Exp*_tmp7D;struct Cyc_Absyn_Exp*_tmp80;struct Cyc_Absyn_Exp*_tmp7F;struct Cyc_Absyn_Exp*_tmp83;struct Cyc_Absyn_Exp*_tmp82;struct Cyc_Absyn_Exp*_tmp81;struct Cyc_Absyn_Exp*_tmp86;struct Cyc_Core_Opt*_tmp85;struct Cyc_Absyn_Exp*_tmp84;struct Cyc_Absyn_Exp*_tmp88;struct Cyc_Absyn_Exp*_tmp87;struct Cyc_List_List*_tmp89;struct Cyc_List_List*_tmp8B;struct Cyc_Absyn_Exp*_tmp8A;switch(*((int*)_tmp6E)){case 0U: _LL1: _LL2:
 goto _LL4;case 1U: _LL3: _LL4:
 return;case 10U: _LL5: _tmp8A=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_tmp8B=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_LL6: {struct Cyc_Absyn_Exp*e1=_tmp8A;struct Cyc_List_List*es=_tmp8B;
# 305
_tmp89=({struct Cyc_List_List*_tmp8C=_cycalloc(sizeof(*_tmp8C));_tmp8C->hd=e1,_tmp8C->tl=es;_tmp8C;});goto _LL8;}case 3U: _LL7: _tmp89=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_LL8: {struct Cyc_List_List*es=_tmp89;
({Cyc_Toseqc_exps_to_seqc(e,es);});return;}case 23U: _LL9: _tmp87=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_tmp88=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_LLA: {struct Cyc_Absyn_Exp*e1=_tmp87;struct Cyc_Absyn_Exp*e2=_tmp88;
({struct Cyc_Absyn_Stmt*(*_tmp8D)(struct Cyc_Absyn_Exp*e,struct Cyc_List_List*es)=Cyc_Toseqc_exps_to_seqc;struct Cyc_Absyn_Exp*_tmp8E=e;struct Cyc_List_List*_tmp8F=({struct Cyc_Absyn_Exp*_tmp90[2U];_tmp90[0]=e1,_tmp90[1]=e2;Cyc_List_list(_tag_fat(_tmp90,sizeof(struct Cyc_Absyn_Exp*),2U));});_tmp8D(_tmp8E,_tmp8F);});return;}case 4U: _LLB: _tmp84=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_tmp85=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_tmp86=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmp6E)->f3;_LLC: {struct Cyc_Absyn_Exp*e1=_tmp84;struct Cyc_Core_Opt*p=_tmp85;struct Cyc_Absyn_Exp*e2=_tmp86;
# 310
if(p == 0){
void*_stmttmp7=e1->r;void*_tmp91=_stmttmp7;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp91)->tag == 1U){_LL58: _LL59:
({Cyc_Toseqc_exp_to_seqc(e2);});return;}else{_LL5A: _LL5B:
 goto _LL57;}_LL57:;}
# 310
({struct Cyc_Absyn_Stmt*(*_tmp92)(struct Cyc_Absyn_Exp*e,struct Cyc_List_List*es)=Cyc_Toseqc_exps_to_seqc;struct Cyc_Absyn_Exp*_tmp93=e;struct Cyc_List_List*_tmp94=({struct Cyc_Absyn_Exp*_tmp95[2U];_tmp95[0]=e2,_tmp95[1]=e1;Cyc_List_list(_tag_fat(_tmp95,sizeof(struct Cyc_Absyn_Exp*),2U));});_tmp92(_tmp93,_tmp94);});
# 317
return;}case 6U: _LLD: _tmp81=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_tmp82=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_tmp83=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmp6E)->f3;_LLE: {struct Cyc_Absyn_Exp*e1=_tmp81;struct Cyc_Absyn_Exp*e2=_tmp82;struct Cyc_Absyn_Exp*e3=_tmp83;
# 319
({Cyc_Toseqc_exp_to_seqc(e3);});_tmp7F=e1;_tmp80=e2;goto _LL10;}case 7U: _LLF: _tmp7F=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_tmp80=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_LL10: {struct Cyc_Absyn_Exp*e1=_tmp7F;struct Cyc_Absyn_Exp*e2=_tmp80;
_tmp7D=e1;_tmp7E=e2;goto _LL12;}case 8U: _LL11: _tmp7D=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_tmp7E=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp7D;struct Cyc_Absyn_Exp*e2=_tmp7E;
_tmp7B=e1;_tmp7C=e2;goto _LL14;}case 9U: _LL13: _tmp7B=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_tmp7C=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_LL14: {struct Cyc_Absyn_Exp*e1=_tmp7B;struct Cyc_Absyn_Exp*e2=_tmp7C;
({Cyc_Toseqc_exp_to_seqc(e2);});_tmp7A=e1;goto _LL16;}case 42U: _LL15: _tmp7A=((struct Cyc_Absyn_Extension_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL16: {struct Cyc_Absyn_Exp*e1=_tmp7A;
# 324
_tmp79=e1;goto _LL18;}case 12U: _LL17: _tmp79=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL18: {struct Cyc_Absyn_Exp*e1=_tmp79;
_tmp78=e1;goto _LL1A;}case 13U: _LL19: _tmp78=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL1A: {struct Cyc_Absyn_Exp*e1=_tmp78;
_tmp77=e1;goto _LL1C;}case 14U: _LL1B: _tmp77=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp77;
_tmp76=e1;goto _LL1E;}case 15U: _LL1D: _tmp76=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp76;
_tmp75=e1;goto _LL20;}case 18U: _LL1F: _tmp75=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp75;
_tmp74=e1;goto _LL22;}case 20U: _LL21: _tmp74=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp74;
_tmp73=e1;goto _LL24;}case 21U: _LL23: _tmp73=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL24: {struct Cyc_Absyn_Exp*e1=_tmp73;
_tmp72=e1;goto _LL26;}case 22U: _LL25: _tmp72=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL26: {struct Cyc_Absyn_Exp*e1=_tmp72;
_tmp71=e1;goto _LL28;}case 5U: _LL27: _tmp71=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp71;
({Cyc_Toseqc_exp_to_seqc(e1);});return;}case 17U: _LL29: _LL2A:
# 335
 goto _LL2C;case 19U: _LL2B: _LL2C:
 goto _LL2E;case 33U: _LL2D: _LL2E:
 goto _LL30;case 32U: _LL2F: _LL30:
 return;case 37U: _LL31: _tmp70=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmp6E)->f2;_LL32: {struct Cyc_List_List*dles=_tmp70;
# 341
struct _tuple0 _stmttmp8=({Cyc_List_split(dles);});struct Cyc_List_List*_tmp96;_LL5D: _tmp96=_stmttmp8.f2;_LL5E: {struct Cyc_List_List*es=_tmp96;
({Cyc_Toseqc_exps_to_seqc(e,es);});
return;}}case 38U: _LL33: _tmp6F=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmp6E)->f1;_LL34: {struct Cyc_Absyn_Stmt*s=_tmp6F;
# 345
({Cyc_Toseqc_stmt_to_seqc(s);});return;}case 41U: _LL35: _LL36:
# 347
 return;case 34U: _LL37: _LL38:
# 349
 bad_form=({const char*_tmp97="Spawn_e";_tag_fat(_tmp97,sizeof(char),8U);});goto _LL0;case 2U: _LL39: _LL3A:
 bad_form=({const char*_tmp98="Pragma_e";_tag_fat(_tmp98,sizeof(char),9U);});goto _LL0;case 11U: _LL3B: _LL3C:
 bad_form=({const char*_tmp99="Throw_e";_tag_fat(_tmp99,sizeof(char),8U);});goto _LL0;case 16U: _LL3D: _LL3E:
 bad_form=({const char*_tmp9A="New_e";_tag_fat(_tmp9A,sizeof(char),6U);});goto _LL0;case 24U: _LL3F: _LL40:
 bad_form=({const char*_tmp9B="Tuple_e";_tag_fat(_tmp9B,sizeof(char),8U);});goto _LL0;case 25U: _LL41: _LL42:
 bad_form=({const char*_tmp9C="CompoundLit_e";_tag_fat(_tmp9C,sizeof(char),14U);});goto _LL0;case 26U: _LL43: _LL44:
 bad_form=({const char*_tmp9D="Array_e";_tag_fat(_tmp9D,sizeof(char),8U);});goto _LL0;case 27U: _LL45: _LL46:
 bad_form=({const char*_tmp9E="Comprehension_e";_tag_fat(_tmp9E,sizeof(char),16U);});goto _LL0;case 28U: _LL47: _LL48:
 bad_form=({const char*_tmp9F="ComprehensionNoinit_e";_tag_fat(_tmp9F,sizeof(char),22U);});goto _LL0;case 29U: _LL49: _LL4A:
 bad_form=({const char*_tmpA0="Aggregate_e";_tag_fat(_tmpA0,sizeof(char),12U);});goto _LL0;case 30U: _LL4B: _LL4C:
 bad_form=({const char*_tmpA1="AnonStruct_e";_tag_fat(_tmpA1,sizeof(char),13U);});goto _LL0;case 31U: _LL4D: _LL4E:
 bad_form=({const char*_tmpA2="Datatype_e";_tag_fat(_tmpA2,sizeof(char),11U);});goto _LL0;case 35U: _LL4F: _LL50:
 bad_form=({const char*_tmpA3="Malloc_e";_tag_fat(_tmpA3,sizeof(char),9U);});goto _LL0;case 36U: _LL51: _LL52:
 bad_form=({const char*_tmpA4="Swap_e";_tag_fat(_tmpA4,sizeof(char),7U);});goto _LL0;case 39U: _LL53: _LL54:
 bad_form=({const char*_tmpA5="Tagcheck_e";_tag_fat(_tmpA5,sizeof(char),11U);});goto _LL0;default: _LL55: _LL56:
 bad_form=({const char*_tmpA6="Valueof_e";_tag_fat(_tmpA6,sizeof(char),10U);});goto _LL0;}_LL0:;}
# 366
({struct Cyc_String_pa_PrintArg_struct _tmpAA=({struct Cyc_String_pa_PrintArg_struct _tmpE2;_tmpE2.tag=0U,_tmpE2.f1=(struct _fat_ptr)((struct _fat_ptr)bad_form);_tmpE2;});struct Cyc_String_pa_PrintArg_struct _tmpAB=({struct Cyc_String_pa_PrintArg_struct _tmpE1;_tmpE1.tag=0U,({
struct _fat_ptr _tmpEE=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpE1.f1=_tmpEE;});_tmpE1;});void*_tmpA7[2U];_tmpA7[0]=& _tmpAA,_tmpA7[1]=& _tmpAB;({int(*_tmpF0)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 366
int(*_tmpA9)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpA9;});struct _fat_ptr _tmpEF=({const char*_tmpA8="bad exp form %s (exp=|%s|) after xlation to C";_tag_fat(_tmpA8,sizeof(char),46U);});_tmpF0(_tmpEF,_tag_fat(_tmpA7,sizeof(void*),2U));});});}
# 370
static void Cyc_Toseqc_stmt_to_seqc(struct Cyc_Absyn_Stmt*s){
# 372
while(1){
void*_stmttmp9=s->r;void*_tmpAE=_stmttmp9;struct Cyc_Absyn_Stmt*_tmpB0;struct Cyc_Absyn_Decl*_tmpAF;struct Cyc_List_List*_tmpB2;struct Cyc_Absyn_Exp*_tmpB1;struct Cyc_Absyn_Stmt*_tmpB6;struct Cyc_Absyn_Exp*_tmpB5;struct Cyc_Absyn_Exp*_tmpB4;struct Cyc_Absyn_Exp*_tmpB3;struct Cyc_Absyn_Stmt*_tmpB8;struct Cyc_Absyn_Exp*_tmpB7;struct Cyc_Absyn_Exp*_tmpBA;struct Cyc_Absyn_Stmt*_tmpB9;struct Cyc_Absyn_Stmt*_tmpBC;struct Cyc_Absyn_Stmt*_tmpBB;struct Cyc_Absyn_Stmt*_tmpBF;struct Cyc_Absyn_Stmt*_tmpBE;struct Cyc_Absyn_Exp*_tmpBD;struct Cyc_Absyn_Exp*_tmpC0;struct Cyc_Absyn_Exp*_tmpC1;struct Cyc_Absyn_Stmt*_tmpC2;switch(*((int*)_tmpAE)){case 0U: _LL1: _LL2:
 goto _LL4;case 6U: _LL3: _LL4:
 goto _LL6;case 7U: _LL5: _LL6:
 goto _LL8;case 8U: _LL7: _LL8:
 return;case 13U: _LL9: _tmpC2=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmpAE)->f2;_LLA: {struct Cyc_Absyn_Stmt*s2=_tmpC2;
s=s2;continue;}case 3U: _LLB: _tmpC1=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmpAE)->f1;_LLC: {struct Cyc_Absyn_Exp*eopt=_tmpC1;
# 380
if(eopt == 0)
return;
# 380
_tmpC0=eopt;goto _LLE;}case 1U: _LLD: _tmpC0=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmpAE)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmpC0;
# 384
({Cyc_Toseqc_exp_to_seqc(e);});
# 392
return;}case 4U: _LLF: _tmpBD=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpAE)->f1;_tmpBE=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpAE)->f2;_tmpBF=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpAE)->f3;_LL10: {struct Cyc_Absyn_Exp*e=_tmpBD;struct Cyc_Absyn_Stmt*s1=_tmpBE;struct Cyc_Absyn_Stmt*s2=_tmpBF;
# 394
({Cyc_Toseqc_exp_to_seqc(e);});
_tmpBB=s1;_tmpBC=s2;goto _LL12;}case 2U: _LL11: _tmpBB=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpAE)->f1;_tmpBC=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpAE)->f2;_LL12: {struct Cyc_Absyn_Stmt*s1=_tmpBB;struct Cyc_Absyn_Stmt*s2=_tmpBC;
# 397
({Cyc_Toseqc_stmt_to_seqc(s1);});
s=s2;
continue;}case 14U: _LL13: _tmpB9=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpAE)->f1;_tmpBA=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpAE)->f2).f1;_LL14: {struct Cyc_Absyn_Stmt*s2=_tmpB9;struct Cyc_Absyn_Exp*e=_tmpBA;
_tmpB7=e;_tmpB8=s2;goto _LL16;}case 5U: _LL15: _tmpB7=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpAE)->f1).f1;_tmpB8=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpAE)->f2;_LL16: {struct Cyc_Absyn_Exp*e=_tmpB7;struct Cyc_Absyn_Stmt*s2=_tmpB8;
# 402
({Cyc_Toseqc_exp_to_seqc(e);});
s=s2;
continue;}case 9U: _LL17: _tmpB3=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpAE)->f1;_tmpB4=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpAE)->f2).f1;_tmpB5=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpAE)->f3).f1;_tmpB6=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpAE)->f4;_LL18: {struct Cyc_Absyn_Exp*e1=_tmpB3;struct Cyc_Absyn_Exp*e2=_tmpB4;struct Cyc_Absyn_Exp*e3=_tmpB5;struct Cyc_Absyn_Stmt*s2=_tmpB6;
# 406
({Cyc_Toseqc_exp_to_seqc(e1);});
({Cyc_Toseqc_exp_to_seqc(e2);});
({Cyc_Toseqc_exp_to_seqc(e3);});
s=s2;
continue;}case 10U: _LL19: _tmpB1=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpAE)->f1;_tmpB2=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpAE)->f2;_LL1A: {struct Cyc_Absyn_Exp*e=_tmpB1;struct Cyc_List_List*scs=_tmpB2;
# 414
({Cyc_Toseqc_exp_to_seqc(e);});
for(0;scs != 0;scs=scs->tl){
({Cyc_Toseqc_stmt_to_seqc(((struct Cyc_Absyn_Switch_clause*)scs->hd)->body);});}
return;}case 12U: _LL1B: _tmpAF=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpAE)->f1;_tmpB0=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpAE)->f2;_LL1C: {struct Cyc_Absyn_Decl*d=_tmpAF;struct Cyc_Absyn_Stmt*s2=_tmpB0;
# 419
{void*_stmttmpA=d->r;void*_tmpC3=_stmttmpA;struct Cyc_Absyn_Vardecl*_tmpC4;if(((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpC3)->tag == 0U){_LL20: _tmpC4=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpC3)->f1;_LL21: {struct Cyc_Absyn_Vardecl*vd=_tmpC4;
# 421
if(vd->initializer != 0){
# 424
void*_stmttmpB=((struct Cyc_Absyn_Exp*)_check_null(vd->initializer))->r;void*_tmpC5=_stmttmpB;struct Cyc_List_List*_tmpC6;if(((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpC5)->tag == 37U){_LL25: _tmpC6=((struct Cyc_Absyn_UnresolvedMem_e_Absyn_Raw_exp_struct*)_tmpC5)->f2;_LL26: {struct Cyc_List_List*dles=_tmpC6;
# 437 "toseqc.cyc"
struct _tuple0 _stmttmpC=({Cyc_List_split(dles);});struct Cyc_List_List*_tmpC7;_LL2A: _tmpC7=_stmttmpC.f2;_LL2B: {struct Cyc_List_List*es=_tmpC7;
struct Cyc_Absyn_Stmt*laststmt=({Cyc_Toseqc_exps_to_seqc((struct Cyc_Absyn_Exp*)_check_null(vd->initializer),es);});
# 440
if(laststmt == 0)goto _LL24;{void*_stmttmpD=laststmt->r;void*_tmpC8=_stmttmpD;struct Cyc_Absyn_Exp*_tmpC9;if(((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmpC8)->tag == 1U){_LL2D: _tmpC9=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmpC8)->f1;_LL2E: {struct Cyc_Absyn_Exp*initexp=_tmpC9;
# 448
{void*_stmttmpE=((struct Cyc_Absyn_Exp*)_check_null(vd->initializer))->r;void*_tmpCA=_stmttmpE;struct Cyc_Absyn_Stmt*_tmpCB;if(((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmpCA)->tag == 38U){_LL32: _tmpCB=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmpCA)->f1;_LL33: {struct Cyc_Absyn_Stmt*s3=_tmpCB;
# 450
vd->initializer=initexp;
laststmt->r=s->r;
s->r=s3->r;
goto _LL31;}}else{_LL34: _LL35:
# 455
({struct Cyc_String_pa_PrintArg_struct _tmpCF=({struct Cyc_String_pa_PrintArg_struct _tmpE3;_tmpE3.tag=0U,({
struct _fat_ptr _tmpF1=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string((struct Cyc_Absyn_Exp*)_check_null(vd->initializer));}));_tmpE3.f1=_tmpF1;});_tmpE3;});void*_tmpCC[1U];_tmpCC[0]=& _tmpCF;({int(*_tmpF3)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 455
int(*_tmpCE)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpCE;});struct _fat_ptr _tmpF2=({const char*_tmpCD="exps_to_seqc updated to non-stmt-exp |%s|";_tag_fat(_tmpCD,sizeof(char),42U);});_tmpF3(_tmpF2,_tag_fat(_tmpCC,sizeof(void*),1U));});});}_LL31:;}
# 458
goto _LL2C;}}else{_LL2F: _LL30:
# 460
({struct Cyc_String_pa_PrintArg_struct _tmpD3=({struct Cyc_String_pa_PrintArg_struct _tmpE4;_tmpE4.tag=0U,({
struct _fat_ptr _tmpF4=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_stmt2string(laststmt);}));_tmpE4.f1=_tmpF4;});_tmpE4;});void*_tmpD0[1U];_tmpD0[0]=& _tmpD3;({int(*_tmpF6)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 460
int(*_tmpD2)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpD2;});struct _fat_ptr _tmpF5=({const char*_tmpD1="exps_to_seqc returned non-exp-stmt |%s|";_tag_fat(_tmpD1,sizeof(char),40U);});_tmpF6(_tmpF5,_tag_fat(_tmpD0,sizeof(void*),1U));});});}_LL2C:;}
# 463
goto _LL24;}}}else{_LL27: _LL28:
# 466
({Cyc_Toseqc_exp_to_seqc((struct Cyc_Absyn_Exp*)_check_null(vd->initializer));});
goto _LL24;}_LL24:;}
# 421 "toseqc.cyc"
s=s2;
# 471 "toseqc.cyc"
continue;}}else{_LL22: _LL23:
 goto _LL1F;}_LL1F:;}
# 474
goto _LL1E;}default: _LL1D: _LL1E:
({void*_tmpD4=0U;({int(*_tmpF8)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpD6)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpD6;});struct _fat_ptr _tmpF7=({const char*_tmpD5="bad stmt after xlation to C";_tag_fat(_tmpD5,sizeof(char),28U);});_tmpF8(_tmpF7,_tag_fat(_tmpD4,sizeof(void*),0U));});});}_LL0:;}}
# 480
struct Cyc_List_List*Cyc_Toseqc_toseqc(struct Cyc_List_List*ds){
struct Cyc_List_List*old_ds=ds;
# 483
for(0;old_ds != 0;old_ds=old_ds->tl){
struct Cyc_Absyn_Decl*next_d=(struct Cyc_Absyn_Decl*)old_ds->hd;
void*_stmttmpF=next_d->r;void*_tmpD8=_stmttmpF;struct Cyc_Absyn_Fndecl*_tmpD9;switch(*((int*)_tmpD8)){case 1U: _LL1: _tmpD9=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpD8)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmpD9;
({Cyc_Toseqc_stmt_to_seqc(fd->body);});goto _LL0;}case 0U: _LL3: _LL4:
 goto _LL6;case 2U: _LL5: _LL6:
 goto _LL8;case 3U: _LL7: _LL8:
 goto _LLA;case 4U: _LL9: _LLA:
 goto _LLC;case 5U: _LLB: _LLC:
 goto _LLE;case 6U: _LLD: _LLE:
 goto _LL10;case 7U: _LLF: _LL10:
 goto _LL12;case 8U: _LL11: _LL12:
 goto _LL0;case 9U: _LL13: _LL14:
 goto _LL16;case 10U: _LL15: _LL16:
 goto _LL18;case 11U: _LL17: _LL18:
 goto _LL1A;case 12U: _LL19: _LL1A:
 goto _LL1C;case 13U: _LL1B: _LL1C:
 goto _LL1E;case 14U: _LL1D: _LL1E:
 goto _LL20;case 15U: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
({void*_tmpDA=0U;({int(*_tmpFA)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpDC)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpDC;});struct _fat_ptr _tmpF9=({const char*_tmpDB="unexpected toplevel decl in toseqc";_tag_fat(_tmpDB,sizeof(char),35U);});_tmpFA(_tmpF9,_tag_fat(_tmpDA,sizeof(void*),0U));});});}_LL0:;}
# 505
return ds;}
