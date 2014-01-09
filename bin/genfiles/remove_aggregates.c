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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 178 "list.h"
struct Cyc_List_List*Cyc_List_imp_rev(struct Cyc_List_List*x);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Position_Error;struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 984 "absyn.h"
extern void*Cyc_Absyn_char_type;extern void*Cyc_Absyn_uint_type;
# 1045
void*Cyc_Absyn_strctq(struct _tuple0*name);
void*Cyc_Absyn_unionq_type(struct _tuple0*name);
# 1060
struct Cyc_Absyn_Exp*Cyc_Absyn_copy_exp(struct Cyc_Absyn_Exp*);
# 1067
struct Cyc_Absyn_Exp*Cyc_Absyn_signed_int_exp(int,unsigned);
# 1069
struct Cyc_Absyn_Exp*Cyc_Absyn_char_exp(char,unsigned);
# 1074
struct Cyc_Absyn_Exp*Cyc_Absyn_var_exp(struct _tuple0*,unsigned);
# 1088
struct Cyc_Absyn_Exp*Cyc_Absyn_lt_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1092
struct Cyc_Absyn_Exp*Cyc_Absyn_assign_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_increment_exp(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned);
# 1097
struct Cyc_Absyn_Exp*Cyc_Absyn_seq_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1109
struct Cyc_Absyn_Exp*Cyc_Absyn_deref_exp(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrmember_exp(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_aggrarrow_exp(struct Cyc_Absyn_Exp*,struct _fat_ptr*,unsigned);
struct Cyc_Absyn_Exp*Cyc_Absyn_subscript_exp(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned);
# 1114
struct Cyc_Absyn_Exp*Cyc_Absyn_stmt_exp(struct Cyc_Absyn_Stmt*,unsigned);
# 1129
struct Cyc_Absyn_Stmt*Cyc_Absyn_exp_stmt(struct Cyc_Absyn_Exp*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_seq_stmt(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned);
# 1137
struct Cyc_Absyn_Stmt*Cyc_Absyn_for_stmt(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned);
# 1140
struct Cyc_Absyn_Stmt*Cyc_Absyn_decl_stmt(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned);
struct Cyc_Absyn_Stmt*Cyc_Absyn_declare_stmt(struct _tuple0*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned);
# 1234
struct _fat_ptr*Cyc_Absyn_designatorlist_to_fieldname(struct Cyc_List_List*);
# 40 "warn.h"
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 67 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);struct Cyc_Hashtable_Table;
# 40 "toc.h"
struct _tuple0*Cyc_Toc_temp_var();extern char Cyc_Toc_Dest[5U];struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct{char*tag;struct Cyc_Absyn_Exp*f1;};struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 50 "relations-ap.h"
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 48 "tcutil.h"
int Cyc_Tcutil_is_array_type(void*);
# 50 "remove_aggregates.cyc"
static void Cyc_RemoveAggrs_massage_toplevel_aggr(struct Cyc_Absyn_Exp*e){
if(e == 0)
return;}
# 58
static struct Cyc_Absyn_Exp*Cyc_RemoveAggrs_member_exp(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc){
void*_stmttmp0=e->r;void*_tmp1=_stmttmp0;struct Cyc_Absyn_Exp*_tmp2;if(((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp1)->tag == 20U){_LL1: _tmp2=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmp1)->f1;_LL2: {struct Cyc_Absyn_Exp*e1=_tmp2;
return({Cyc_Absyn_aggrarrow_exp(e1,f,loc);});}}else{_LL3: _LL4:
 return({Cyc_Absyn_aggrmember_exp(e,f,loc);});}_LL0:;}
# 58
enum Cyc_RemoveAggrs_ExpContext{Cyc_RemoveAggrs_Initializer =0U,Cyc_RemoveAggrs_NewDest =1U,Cyc_RemoveAggrs_AggrField =2U,Cyc_RemoveAggrs_Other =3U};struct Cyc_RemoveAggrs_Env{enum Cyc_RemoveAggrs_ExpContext ctxt;struct Cyc_Absyn_Exp*dest;};
# 78
static struct Cyc_RemoveAggrs_Env Cyc_RemoveAggrs_other_env={Cyc_RemoveAggrs_Other,0};
# 80
enum Cyc_RemoveAggrs_ExpResult{Cyc_RemoveAggrs_WasArray =0U,Cyc_RemoveAggrs_OtherRes =1U};struct Cyc_RemoveAggrs_Result{enum Cyc_RemoveAggrs_ExpResult res;};
# 89
static struct Cyc_RemoveAggrs_Result Cyc_RemoveAggrs_remove_aggrs_exp(struct Cyc_RemoveAggrs_Env,struct Cyc_Absyn_Exp*);
static void Cyc_RemoveAggrs_remove_aggrs_stmt(struct Cyc_Absyn_Stmt*s);
# 92
static void Cyc_RemoveAggrs_noarray_remove_aggrs_exp(struct Cyc_RemoveAggrs_Env env,struct Cyc_Absyn_Exp*e){
struct Cyc_RemoveAggrs_Result _stmttmp1=({Cyc_RemoveAggrs_remove_aggrs_exp(env,e);});enum Cyc_RemoveAggrs_ExpResult _tmp4;_LL1: _tmp4=_stmttmp1.res;_LL2: {enum Cyc_RemoveAggrs_ExpResult r=_tmp4;
if((int)r != (int)1U)
({struct Cyc_String_pa_PrintArg_struct _tmp8=({struct Cyc_String_pa_PrintArg_struct _tmpDE;_tmpDE.tag=0U,({
struct _fat_ptr _tmpE7=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpDE.f1=_tmpE7;});_tmpDE;});void*_tmp5[1U];_tmp5[0]=& _tmp8;({int(*_tmpE9)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 95
int(*_tmp7)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp7;});struct _fat_ptr _tmpE8=({const char*_tmp6="remove_aggrs_exp -- unexpected array or comprehension: %s";_tag_fat(_tmp6,sizeof(char),58U);});_tmpE9(_tmpE8,_tag_fat(_tmp5,sizeof(void*),1U));});});}}struct _tuple12{struct Cyc_List_List*f1;struct Cyc_Absyn_Exp*f2;};
# 99
static struct Cyc_RemoveAggrs_Result Cyc_RemoveAggrs_remove_aggrs_exp(struct Cyc_RemoveAggrs_Env env,struct Cyc_Absyn_Exp*e){
enum Cyc_RemoveAggrs_ExpResult res=1U;
int did_assign=0;
# 103
{void*_stmttmp2=e->r;void*_tmpA=_stmttmp2;struct Cyc_Absyn_Stmt*_tmpB;struct Cyc_Absyn_Exp*_tmpC;struct Cyc_Absyn_Exp*_tmpD;struct Cyc_Absyn_Exp*_tmpE;struct Cyc_Absyn_Exp*_tmpF;struct Cyc_Absyn_Exp*_tmp10;struct Cyc_Absyn_Exp*_tmp11;struct Cyc_Absyn_Exp*_tmp12;struct Cyc_Absyn_Exp*_tmp13;struct Cyc_Absyn_Exp*_tmp14;struct Cyc_Absyn_Exp*_tmp16;struct Cyc_Absyn_Exp*_tmp15;struct Cyc_Absyn_Exp*_tmp18;struct Cyc_Absyn_Exp*_tmp17;struct Cyc_Absyn_Exp*_tmp1A;struct Cyc_Absyn_Exp*_tmp19;struct Cyc_List_List*_tmp1B;struct Cyc_List_List*_tmp1D;struct Cyc_Absyn_Exp*_tmp1C;struct Cyc_Absyn_Exp*_tmp20;struct Cyc_Absyn_Exp*_tmp1F;struct Cyc_Absyn_Exp*_tmp1E;struct Cyc_Absyn_Exp*_tmp22;struct Cyc_Absyn_Exp*_tmp21;struct Cyc_Absyn_Exp*_tmp24;struct Cyc_Absyn_Exp*_tmp23;struct Cyc_Absyn_Exp*_tmp26;struct Cyc_Absyn_Exp*_tmp25;struct Cyc_Absyn_Exp*_tmp27;int _tmp2B;struct Cyc_Absyn_Exp*_tmp2A;struct Cyc_Absyn_Exp*_tmp29;struct Cyc_Absyn_Vardecl*_tmp28;struct Cyc_List_List*_tmp2C;struct Cyc_Absyn_Aggrdecl*_tmp2F;struct Cyc_List_List*_tmp2E;struct _tuple0*_tmp2D;struct Cyc_Absyn_Exp*_tmp30;struct _fat_ptr _tmp31;switch(*((int*)_tmpA)){case 0U: if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpA)->f1).String_c).tag == 8){_LL1: _tmp31=((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmpA)->f1).String_c).val;_LL2: {struct _fat_ptr s=_tmp31;
# 106
if((int)env.ctxt == (int)Cyc_RemoveAggrs_AggrField &&({Cyc_Tcutil_is_array_type((void*)_check_null(e->topt));})){
struct Cyc_List_List*dles=0;
unsigned n=_get_fat_size(s,sizeof(char));
{unsigned i=0U;for(0;i < n;++ i){
struct Cyc_Absyn_Exp*c=({Cyc_Absyn_char_exp(((const char*)s.curr)[(int)i],0U);});
c->topt=Cyc_Absyn_char_type;
dles=({struct Cyc_List_List*_tmp33=_cycalloc(sizeof(*_tmp33));({struct _tuple12*_tmpEA=({struct _tuple12*_tmp32=_cycalloc(sizeof(*_tmp32));_tmp32->f1=0,_tmp32->f2=c;_tmp32;});_tmp33->hd=_tmpEA;}),_tmp33->tl=dles;_tmp33;});}}
# 114
dles=({Cyc_List_imp_rev(dles);});
({void*_tmpEB=(void*)({struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*_tmp34=_cycalloc(sizeof(*_tmp34));_tmp34->tag=26U,_tmp34->f1=dles;_tmp34;});e->r=_tmpEB;});
return({Cyc_RemoveAggrs_remove_aggrs_exp(env,e);});}else{
goto _LL0;}}}else{_LL3: _LL4:
 goto _LL6;}case 17U: _LL5: _LL6:
 goto _LL8;case 19U: _LL7: _LL8:
 goto _LLA;case 33U: _LL9: _LLA:
 goto _LLC;case 32U: _LLB: _LLC:
 goto _LLE;case 41U: _LLD: _LLE:
 goto _LL10;case 1U: _LLF: _LL10:
 goto _LL0;case 16U: _LL11: _tmp30=((struct Cyc_Absyn_New_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL12: {struct Cyc_Absyn_Exp*e1=_tmp30;
# 127
{void*_stmttmp3=e->annot;void*_tmp35=_stmttmp3;struct Cyc_Absyn_Exp*_tmp36;if(((struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct*)_tmp35)->tag == Cyc_Toc_Dest){_LL44: _tmp36=((struct Cyc_Toc_Dest_Absyn_AbsynAnnot_struct*)_tmp35)->f1;_LL45: {struct Cyc_Absyn_Exp*dest=_tmp36;
# 129
({({struct Cyc_RemoveAggrs_Env _tmpEC=({struct Cyc_RemoveAggrs_Env _tmpDF;_tmpDF.ctxt=Cyc_RemoveAggrs_NewDest,_tmpDF.dest=dest;_tmpDF;});Cyc_RemoveAggrs_remove_aggrs_exp(_tmpEC,e1);});});
*e=*e1;
goto _LL43;}}else{_LL46: _LL47:
({void*_tmp37=0U;({int(*_tmpEE)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp39)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp39;});struct _fat_ptr _tmpED=({const char*_tmp38="removeAggrs: toc did not set a new-destination";_tag_fat(_tmp38,sizeof(char),47U);});_tmpEE(_tmpED,_tag_fat(_tmp37,sizeof(void*),0U));});});}_LL43:;}
# 134
goto _LL0;}case 29U: _LL13: _tmp2D=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp2E=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpA)->f3;_tmp2F=((struct Cyc_Absyn_Aggregate_e_Absyn_Raw_exp_struct*)_tmpA)->f4;_LL14: {struct _tuple0*tdn=_tmp2D;struct Cyc_List_List*dles=_tmp2E;struct Cyc_Absyn_Aggrdecl*sdopt=_tmp2F;
# 137
did_assign=1;{
int do_stmt_exp=1;
if((int)env.ctxt == (int)Cyc_RemoveAggrs_AggrField ||(int)env.ctxt == (int)Cyc_RemoveAggrs_NewDest)
do_stmt_exp=0;{
# 139
struct Cyc_Absyn_Exp*dest;
# 142
struct _tuple0**v;
if(do_stmt_exp){
v=({struct _tuple0**_tmp3A=_cycalloc(sizeof(*_tmp3A));({struct _tuple0*_tmpEF=({Cyc_Toc_temp_var();});*_tmp3A=_tmpEF;});_tmp3A;});
dest=({Cyc_Absyn_var_exp(*v,0U);});}else{
# 147
v=0;
dest=(struct Cyc_Absyn_Exp*)_check_null(env.dest);
if((int)env.ctxt == (int)Cyc_RemoveAggrs_NewDest)
dest=({Cyc_Absyn_deref_exp(dest,0U);});}
# 153
{struct Cyc_List_List*dles2=dles;for(0;dles2 != 0;dles2=dles2->tl){
struct _tuple12*_stmttmp4=(struct _tuple12*)dles2->hd;struct Cyc_Absyn_Exp*_tmp3C;struct Cyc_List_List*_tmp3B;_LL49: _tmp3B=_stmttmp4->f1;_tmp3C=_stmttmp4->f2;_LL4A: {struct Cyc_List_List*ds=_tmp3B;struct Cyc_Absyn_Exp*field_exp=_tmp3C;
struct _fat_ptr*f=({Cyc_Absyn_designatorlist_to_fieldname(ds);});
struct Cyc_Absyn_Exp*field_dest=({struct Cyc_Absyn_Exp*(*_tmp3D)(struct Cyc_Absyn_Exp*e,struct _fat_ptr*f,unsigned loc)=Cyc_RemoveAggrs_member_exp;struct Cyc_Absyn_Exp*_tmp3E=({Cyc_Absyn_copy_exp(dest);});struct _fat_ptr*_tmp3F=f;unsigned _tmp40=0U;_tmp3D(_tmp3E,_tmp3F,_tmp40);});
({({struct Cyc_RemoveAggrs_Env _tmpF0=({struct Cyc_RemoveAggrs_Env _tmpE0;_tmpE0.ctxt=Cyc_RemoveAggrs_AggrField,_tmpE0.dest=field_dest;_tmpE0;});Cyc_RemoveAggrs_remove_aggrs_exp(_tmpF0,field_exp);});});}}}
# 160
if(dles == 0)
({void*_tmp41=0U;({int(*_tmpF2)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp43)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp43;});struct _fat_ptr _tmpF1=({const char*_tmp42="zero-field aggregate";_tag_fat(_tmp42,sizeof(char),21U);});_tmpF2(_tmpF1,_tag_fat(_tmp41,sizeof(void*),0U));});});{
# 160
struct Cyc_Absyn_Exp*init_e=(*((struct _tuple12*)dles->hd)).f2;
# 163
for(dles=dles->tl;dles != 0;dles=dles->tl){
init_e=({Cyc_Absyn_seq_exp(init_e,(*((struct _tuple12*)dles->hd)).f2,0U);});}
if(do_stmt_exp){
void*(*f)(struct _tuple0*name)=(unsigned)sdopt &&(int)sdopt->kind == (int)Cyc_Absyn_UnionA?Cyc_Absyn_unionq_type: Cyc_Absyn_strctq;
({void*_tmpF3=({struct Cyc_Absyn_Exp*(*_tmp44)(struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_stmt_exp;struct Cyc_Absyn_Stmt*_tmp45=({struct Cyc_Absyn_Stmt*(*_tmp46)(struct _tuple0*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple0*_tmp47=*((struct _tuple0**)_check_null(v));void*_tmp48=({f(tdn);});struct Cyc_Absyn_Exp*_tmp49=0;struct Cyc_Absyn_Stmt*_tmp4A=({struct Cyc_Absyn_Stmt*(*_tmp4B)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp4C=({Cyc_Absyn_exp_stmt(init_e,0U);});struct Cyc_Absyn_Stmt*_tmp4D=({Cyc_Absyn_exp_stmt(dest,0U);});unsigned _tmp4E=0U;_tmp4B(_tmp4C,_tmp4D,_tmp4E);});unsigned _tmp4F=0U;_tmp46(_tmp47,_tmp48,_tmp49,_tmp4A,_tmp4F);});unsigned _tmp50=0U;_tmp44(_tmp45,_tmp50);})->r;e->r=_tmpF3;});}else{
# 171
e->r=init_e->r;
e->topt=0;}
# 174
goto _LL0;}}}}case 26U: _LL15: _tmp2C=((struct Cyc_Absyn_Array_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL16: {struct Cyc_List_List*dles=_tmp2C;
# 179
res=0U;
if(dles == 0){
({struct Cyc_Absyn_Exp _tmpF4=*({Cyc_Absyn_signed_int_exp(0,0U);});*e=_tmpF4;});
goto _LL0;}
# 180
{enum Cyc_RemoveAggrs_ExpContext _stmttmp5=env.ctxt;enum Cyc_RemoveAggrs_ExpContext _tmp51=_stmttmp5;if(_tmp51 == Cyc_RemoveAggrs_Other){_LL4C: _LL4D:
# 185
({void*_tmp52=0U;({int(*_tmpF6)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp54)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp54;});struct _fat_ptr _tmpF5=({const char*_tmp53="remove-aggrs: Array_e in bad position";_tag_fat(_tmp53,sizeof(char),38U);});_tmpF6(_tmpF5,_tag_fat(_tmp52,sizeof(void*),0U));});});}else{_LL4E: _LL4F:
 goto _LL4B;}_LL4B:;}
# 188
did_assign=1;{
struct Cyc_Absyn_Exp*dest=(struct Cyc_Absyn_Exp*)_check_null(env.dest);
# 191
int i=0;
{struct Cyc_List_List*dles2=dles;for(0;dles2 != 0;(dles2=dles2->tl,++ i)){
# 195
struct _tuple12*_stmttmp6=(struct _tuple12*)dles2->hd;struct Cyc_Absyn_Exp*_tmp55;_LL51: _tmp55=_stmttmp6->f2;_LL52: {struct Cyc_Absyn_Exp*field_exp=_tmp55;
struct Cyc_Absyn_Exp*fielddest=({struct Cyc_Absyn_Exp*(*_tmp56)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_subscript_exp;struct Cyc_Absyn_Exp*_tmp57=({Cyc_Absyn_copy_exp(dest);});struct Cyc_Absyn_Exp*_tmp58=({Cyc_Absyn_signed_int_exp(i,0U);});unsigned _tmp59=0U;_tmp56(_tmp57,_tmp58,_tmp59);});
({({struct Cyc_RemoveAggrs_Env _tmpF7=({struct Cyc_RemoveAggrs_Env _tmpE1;_tmpE1.ctxt=Cyc_RemoveAggrs_AggrField,_tmpE1.dest=fielddest;_tmpE1;});Cyc_RemoveAggrs_remove_aggrs_exp(_tmpF7,field_exp);});});}}}{
# 199
struct Cyc_Absyn_Exp*init_e=(*((struct _tuple12*)dles->hd)).f2;
for(dles=dles->tl;dles != 0;dles=dles->tl){
init_e=({Cyc_Absyn_seq_exp(init_e,(*((struct _tuple12*)dles->hd)).f2,0U);});}
e->r=init_e->r;
e->topt=0;
goto _LL0;}}}case 27U: _LL17: _tmp28=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp29=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_tmp2A=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpA)->f3;_tmp2B=((struct Cyc_Absyn_Comprehension_e_Absyn_Raw_exp_struct*)_tmpA)->f4;_LL18: {struct Cyc_Absyn_Vardecl*vd=_tmp28;struct Cyc_Absyn_Exp*bd=_tmp29;struct Cyc_Absyn_Exp*body=_tmp2A;int zero_term=_tmp2B;
# 208
did_assign=1;
res=0U;
{enum Cyc_RemoveAggrs_ExpContext _stmttmp7=env.ctxt;enum Cyc_RemoveAggrs_ExpContext _tmp5A=_stmttmp7;if(_tmp5A == Cyc_RemoveAggrs_Other){_LL54: _LL55:
({void*_tmp5B=0U;({int(*_tmpF9)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp5D)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp5D;});struct _fat_ptr _tmpF8=({const char*_tmp5C="remove-aggrs: comprehension in bad position";_tag_fat(_tmp5C,sizeof(char),44U);});_tmpF9(_tmpF8,_tag_fat(_tmp5B,sizeof(void*),0U));});});}else{_LL56: _LL57:
 goto _LL53;}_LL53:;}{
# 214
struct _tuple0*max=({Cyc_Toc_temp_var();});
struct _tuple0*i=vd->name;
struct Cyc_Absyn_Exp*ea=({struct Cyc_Absyn_Exp*(*_tmp8F)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmp90=({Cyc_Absyn_var_exp(i,0U);});struct Cyc_Absyn_Exp*_tmp91=({Cyc_Absyn_signed_int_exp(0,0U);});unsigned _tmp92=0U;_tmp8F(_tmp90,_tmp91,_tmp92);});
struct Cyc_Absyn_Exp*eb=({struct Cyc_Absyn_Exp*(*_tmp8B)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_lt_exp;struct Cyc_Absyn_Exp*_tmp8C=({Cyc_Absyn_var_exp(i,0U);});struct Cyc_Absyn_Exp*_tmp8D=({Cyc_Absyn_var_exp(max,0U);});unsigned _tmp8E=0U;_tmp8B(_tmp8C,_tmp8D,_tmp8E);});
struct Cyc_Absyn_Exp*ec=({struct Cyc_Absyn_Exp*(*_tmp87)(struct Cyc_Absyn_Exp*,enum Cyc_Absyn_Incrementor,unsigned)=Cyc_Absyn_increment_exp;struct Cyc_Absyn_Exp*_tmp88=({Cyc_Absyn_var_exp(i,0U);});enum Cyc_Absyn_Incrementor _tmp89=Cyc_Absyn_PreInc;unsigned _tmp8A=0U;_tmp87(_tmp88,_tmp89,_tmp8A);});
struct Cyc_Absyn_Exp*lval=({struct Cyc_Absyn_Exp*(*_tmp83)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_subscript_exp;struct Cyc_Absyn_Exp*_tmp84=(struct Cyc_Absyn_Exp*)_check_null(env.dest);struct Cyc_Absyn_Exp*_tmp85=({Cyc_Absyn_var_exp(i,0U);});unsigned _tmp86=0U;_tmp83(_tmp84,_tmp85,_tmp86);});
({struct Cyc_RemoveAggrs_Result(*_tmp5E)(struct Cyc_RemoveAggrs_Env env,struct Cyc_Absyn_Exp*e)=Cyc_RemoveAggrs_remove_aggrs_exp;struct Cyc_RemoveAggrs_Env _tmp5F=({struct Cyc_RemoveAggrs_Env _tmpE2;_tmpE2.ctxt=Cyc_RemoveAggrs_AggrField,({struct Cyc_Absyn_Exp*_tmpFA=({Cyc_Absyn_copy_exp(lval);});_tmpE2.dest=_tmpFA;});_tmpE2;});struct Cyc_Absyn_Exp*_tmp60=body;_tmp5E(_tmp5F,_tmp60);});{
struct Cyc_Absyn_Stmt*s=({struct Cyc_Absyn_Stmt*(*_tmp7D)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_for_stmt;struct Cyc_Absyn_Exp*_tmp7E=ea;struct Cyc_Absyn_Exp*_tmp7F=eb;struct Cyc_Absyn_Exp*_tmp80=ec;struct Cyc_Absyn_Stmt*_tmp81=({Cyc_Absyn_exp_stmt(body,0U);});unsigned _tmp82=0U;_tmp7D(_tmp7E,_tmp7F,_tmp80,_tmp81,_tmp82);});
if(zero_term){
# 229
struct Cyc_Absyn_Exp*ex=({struct Cyc_Absyn_Exp*(*_tmp65)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmp66=({struct Cyc_Absyn_Exp*(*_tmp67)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_subscript_exp;struct Cyc_Absyn_Exp*_tmp68=({Cyc_Absyn_copy_exp(env.dest);});struct Cyc_Absyn_Exp*_tmp69=({Cyc_Absyn_var_exp(max,0U);});unsigned _tmp6A=0U;_tmp67(_tmp68,_tmp69,_tmp6A);});struct Cyc_Absyn_Exp*_tmp6B=({Cyc_Absyn_signed_int_exp(0,0U);});unsigned _tmp6C=0U;_tmp65(_tmp66,_tmp6B,_tmp6C);});
# 232
s=({struct Cyc_Absyn_Stmt*(*_tmp61)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp62=s;struct Cyc_Absyn_Stmt*_tmp63=({Cyc_Absyn_exp_stmt(ex,0U);});unsigned _tmp64=0U;_tmp61(_tmp62,_tmp63,_tmp64);});}
# 222
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,bd);});
# 236
({void*_tmpFB=({struct Cyc_Absyn_Exp*(*_tmp6D)(struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_stmt_exp;struct Cyc_Absyn_Stmt*_tmp6E=({struct Cyc_Absyn_Stmt*(*_tmp6F)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmp70=({struct Cyc_Absyn_Stmt*(*_tmp71)(struct _tuple0*,void*,struct Cyc_Absyn_Exp*init,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_declare_stmt;struct _tuple0*_tmp72=max;void*_tmp73=Cyc_Absyn_uint_type;struct Cyc_Absyn_Exp*_tmp74=bd;struct Cyc_Absyn_Stmt*_tmp75=({Cyc_Absyn_declare_stmt(i,Cyc_Absyn_uint_type,0,s,0U);});unsigned _tmp76=0U;_tmp71(_tmp72,_tmp73,_tmp74,_tmp75,_tmp76);});struct Cyc_Absyn_Stmt*_tmp77=({struct Cyc_Absyn_Stmt*(*_tmp78)(struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_exp_stmt;struct Cyc_Absyn_Exp*_tmp79=({Cyc_Absyn_signed_int_exp(0,0U);});unsigned _tmp7A=0U;_tmp78(_tmp79,_tmp7A);});unsigned _tmp7B=0U;_tmp6F(_tmp70,_tmp77,_tmp7B);});unsigned _tmp7C=0U;_tmp6D(_tmp6E,_tmp7C);})->r;e->r=_tmpFB;});
# 239
e->topt=0;
goto _LL0;}}}case 28U: _LL19: _tmp27=((struct Cyc_Absyn_ComprehensionNoinit_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL1A: {struct Cyc_Absyn_Exp*bd=_tmp27;
# 243
did_assign=1;
res=0U;
{enum Cyc_RemoveAggrs_ExpContext _stmttmp8=env.ctxt;enum Cyc_RemoveAggrs_ExpContext _tmp93=_stmttmp8;if(_tmp93 == Cyc_RemoveAggrs_Other){_LL59: _LL5A:
({void*_tmp94=0U;({int(*_tmpFD)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp96)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp96;});struct _fat_ptr _tmpFC=({const char*_tmp95="remove-aggrs: no-init-comp in bad position";_tag_fat(_tmp95,sizeof(char),43U);});_tmpFD(_tmpFC,_tag_fat(_tmp94,sizeof(void*),0U));});});}else{_LL5B: _LL5C:
 goto _LL58;}_LL58:;}
# 250
*e=*bd;
goto _LL0;}case 4U: _LL1B: _tmp25=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp26=((struct Cyc_Absyn_AssignOp_e_Absyn_Raw_exp_struct*)_tmpA)->f3;_LL1C: {struct Cyc_Absyn_Exp*e1=_tmp25;struct Cyc_Absyn_Exp*e2=_tmp26;
# 254
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e1);});
({Cyc_RemoveAggrs_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e2);});
goto _LL0;}case 34U: _LL1D: _tmp23=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp24=((struct Cyc_Absyn_Spawn_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL1E: {struct Cyc_Absyn_Exp*e1=_tmp23;struct Cyc_Absyn_Exp*e2=_tmp24;
# 259
did_assign=1;
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e1);});
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(env,e2);});
goto _LL0;}case 9U: _LL1F: _tmp21=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp22=((struct Cyc_Absyn_SeqExp_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL20: {struct Cyc_Absyn_Exp*e1=_tmp21;struct Cyc_Absyn_Exp*e2=_tmp22;
# 265
did_assign=1;
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e1);});
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(env,e2);});
goto _LL0;}case 6U: _LL21: _tmp1E=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp1F=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_tmp20=((struct Cyc_Absyn_Conditional_e_Absyn_Raw_exp_struct*)_tmpA)->f3;_LL22: {struct Cyc_Absyn_Exp*e1=_tmp1E;struct Cyc_Absyn_Exp*e2=_tmp1F;struct Cyc_Absyn_Exp*e3=_tmp20;
# 270
did_assign=1;
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e1);});
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(env,e2);});{
struct Cyc_RemoveAggrs_Env env3=({struct Cyc_RemoveAggrs_Env _tmpE3;_tmpE3.ctxt=env.ctxt,env.dest == 0?_tmpE3.dest=0:({struct Cyc_Absyn_Exp*_tmpFE=({Cyc_Absyn_copy_exp(env.dest);});_tmpE3.dest=_tmpFE;});_tmpE3;});
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(env3,e3);});
goto _LL0;}}case 10U: if(((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpA)->f3 == 0){_LL23: _tmp1C=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp1D=((struct Cyc_Absyn_FnCall_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL24: {struct Cyc_Absyn_Exp*e=_tmp1C;struct Cyc_List_List*es=_tmp1D;
# 278
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e);});
_tmp1B=es;goto _LL26;}}else{goto _LL41;}case 3U: _LL25: _tmp1B=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL26: {struct Cyc_List_List*es=_tmp1B;
# 281
for(0;es != 0;es=es->tl){
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,(struct Cyc_Absyn_Exp*)es->hd);});}
goto _LL0;}case 23U: _LL27: _tmp19=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp1A=((struct Cyc_Absyn_Subscript_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL28: {struct Cyc_Absyn_Exp*e1=_tmp19;struct Cyc_Absyn_Exp*e2=_tmp1A;
# 285
_tmp17=e1;_tmp18=e2;goto _LL2A;}case 7U: _LL29: _tmp17=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp18=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL2A: {struct Cyc_Absyn_Exp*e1=_tmp17;struct Cyc_Absyn_Exp*e2=_tmp18;
_tmp15=e1;_tmp16=e2;goto _LL2C;}case 8U: _LL2B: _tmp15=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_tmp16=((struct Cyc_Absyn_Or_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL2C: {struct Cyc_Absyn_Exp*e1=_tmp15;struct Cyc_Absyn_Exp*e2=_tmp16;
# 288
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e1);});
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e2);});
goto _LL0;}case 13U: _LL2D: _tmp14=((struct Cyc_Absyn_Instantiate_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL2E: {struct Cyc_Absyn_Exp*e1=_tmp14;
# 293
_tmp13=e1;goto _LL30;}case 12U: _LL2F: _tmp13=((struct Cyc_Absyn_NoInstantiate_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL30: {struct Cyc_Absyn_Exp*e1=_tmp13;
_tmp12=e1;goto _LL32;}case 14U: _LL31: _tmp12=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmpA)->f2;_LL32: {struct Cyc_Absyn_Exp*e1=_tmp12;
_tmp11=e1;goto _LL34;}case 15U: _LL33: _tmp11=((struct Cyc_Absyn_Address_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL34: {struct Cyc_Absyn_Exp*e1=_tmp11;
_tmp10=e1;goto _LL36;}case 20U: _LL35: _tmp10=((struct Cyc_Absyn_Deref_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL36: {struct Cyc_Absyn_Exp*e1=_tmp10;
_tmpF=e1;goto _LL38;}case 21U: _LL37: _tmpF=((struct Cyc_Absyn_AggrMember_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL38: {struct Cyc_Absyn_Exp*e1=_tmpF;
_tmpE=e1;goto _LL3A;}case 22U: _LL39: _tmpE=((struct Cyc_Absyn_AggrArrow_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL3A: {struct Cyc_Absyn_Exp*e1=_tmpE;
_tmpD=e1;goto _LL3C;}case 18U: _LL3B: _tmpD=((struct Cyc_Absyn_Sizeofexp_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL3C: {struct Cyc_Absyn_Exp*e1=_tmpD;
_tmpC=e1;goto _LL3E;}case 5U: _LL3D: _tmpC=((struct Cyc_Absyn_Increment_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL3E: {struct Cyc_Absyn_Exp*e1=_tmpC;
# 302
({Cyc_RemoveAggrs_noarray_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e1);});
goto _LL0;}case 38U: _LL3F: _tmpB=((struct Cyc_Absyn_StmtExp_e_Absyn_Raw_exp_struct*)_tmpA)->f1;_LL40: {struct Cyc_Absyn_Stmt*s=_tmpB;
# 306
({Cyc_RemoveAggrs_remove_aggrs_stmt(s);});
goto _LL0;}default: _LL41: _LL42:
# 309
({struct Cyc_String_pa_PrintArg_struct _tmp9A=({struct Cyc_String_pa_PrintArg_struct _tmpE4;_tmpE4.tag=0U,({
struct _fat_ptr _tmpFF=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmpE4.f1=_tmpFF;});_tmpE4;});void*_tmp97[1U];_tmp97[0]=& _tmp9A;({int(*_tmp101)(struct _fat_ptr fmt,struct _fat_ptr ap)=({
# 309
int(*_tmp99)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp99;});struct _fat_ptr _tmp100=({const char*_tmp98="bad exp after translation to C: %s";_tag_fat(_tmp98,sizeof(char),35U);});_tmp101(_tmp100,_tag_fat(_tmp97,sizeof(void*),1U));});});}_LL0:;}
# 312
if((int)env.ctxt == (int)Cyc_RemoveAggrs_AggrField && !did_assign)
({void*_tmp102=({struct Cyc_Absyn_Exp*(*_tmp9B)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmp9C=(struct Cyc_Absyn_Exp*)_check_null(env.dest);struct Cyc_Absyn_Exp*_tmp9D=({Cyc_Absyn_copy_exp(e);});unsigned _tmp9E=0U;_tmp9B(_tmp9C,_tmp9D,_tmp9E);})->r;e->r=_tmp102;});
# 312
if(
# 314
(int)env.ctxt == (int)Cyc_RemoveAggrs_NewDest && !did_assign)
({void*_tmp103=({struct Cyc_Absyn_Exp*(*_tmp9F)(struct Cyc_Absyn_Exp*,struct Cyc_Absyn_Exp*,unsigned)=Cyc_Absyn_assign_exp;struct Cyc_Absyn_Exp*_tmpA0=({Cyc_Absyn_deref_exp((struct Cyc_Absyn_Exp*)_check_null(env.dest),0U);});struct Cyc_Absyn_Exp*_tmpA1=({Cyc_Absyn_copy_exp(e);});unsigned _tmpA2=0U;_tmp9F(_tmpA0,_tmpA1,_tmpA2);})->r;e->r=_tmp103;});
# 312
return({struct Cyc_RemoveAggrs_Result _tmpE5;_tmpE5.res=res;_tmpE5;});}
# 323
static void Cyc_RemoveAggrs_remove_aggrs_stmt(struct Cyc_Absyn_Stmt*s){
void*_stmttmp9=s->r;void*_tmpA4=_stmttmp9;struct Cyc_Absyn_Stmt*_tmpA6;struct Cyc_Absyn_Decl*_tmpA5;struct Cyc_List_List*_tmpA8;struct Cyc_Absyn_Exp*_tmpA7;struct Cyc_Absyn_Stmt*_tmpAC;struct Cyc_Absyn_Exp*_tmpAB;struct Cyc_Absyn_Exp*_tmpAA;struct Cyc_Absyn_Exp*_tmpA9;struct Cyc_Absyn_Stmt*_tmpAE;struct Cyc_Absyn_Exp*_tmpAD;struct Cyc_Absyn_Exp*_tmpB0;struct Cyc_Absyn_Stmt*_tmpAF;struct Cyc_Absyn_Stmt*_tmpB3;struct Cyc_Absyn_Stmt*_tmpB2;struct Cyc_Absyn_Exp*_tmpB1;struct Cyc_Absyn_Stmt*_tmpB5;struct Cyc_Absyn_Stmt*_tmpB4;struct Cyc_Absyn_Exp*_tmpB6;struct Cyc_Absyn_Exp*_tmpB7;struct Cyc_Absyn_Stmt*_tmpB8;switch(*((int*)_tmpA4)){case 0U: _LL1: _LL2:
 goto _LL4;case 6U: _LL3: _LL4:
 goto _LL6;case 7U: _LL5: _LL6:
 goto _LL8;case 8U: _LL7: _LL8:
 return;case 13U: _LL9: _tmpB8=((struct Cyc_Absyn_Label_s_Absyn_Raw_stmt_struct*)_tmpA4)->f2;_LLA: {struct Cyc_Absyn_Stmt*s2=_tmpB8;
({Cyc_RemoveAggrs_remove_aggrs_stmt(s2);});return;}case 3U: _LLB: _tmpB7=((struct Cyc_Absyn_Return_s_Absyn_Raw_stmt_struct*)_tmpA4)->f1;_LLC: {struct Cyc_Absyn_Exp*eopt=_tmpB7;
# 331
if(eopt == 0)
return;
# 331
_tmpB6=eopt;goto _LLE;}case 1U: _LLD: _tmpB6=((struct Cyc_Absyn_Exp_s_Absyn_Raw_stmt_struct*)_tmpA4)->f1;_LLE: {struct Cyc_Absyn_Exp*e=_tmpB6;
# 334
({Cyc_RemoveAggrs_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e);});return;}case 2U: _LLF: _tmpB4=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpA4)->f1;_tmpB5=((struct Cyc_Absyn_Seq_s_Absyn_Raw_stmt_struct*)_tmpA4)->f2;_LL10: {struct Cyc_Absyn_Stmt*s1=_tmpB4;struct Cyc_Absyn_Stmt*s2=_tmpB5;
# 336
({Cyc_RemoveAggrs_remove_aggrs_stmt(s1);});
({Cyc_RemoveAggrs_remove_aggrs_stmt(s2);});
return;}case 4U: _LL11: _tmpB1=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpA4)->f1;_tmpB2=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpA4)->f2;_tmpB3=((struct Cyc_Absyn_IfThenElse_s_Absyn_Raw_stmt_struct*)_tmpA4)->f3;_LL12: {struct Cyc_Absyn_Exp*e=_tmpB1;struct Cyc_Absyn_Stmt*s1=_tmpB2;struct Cyc_Absyn_Stmt*s2=_tmpB3;
# 340
({Cyc_RemoveAggrs_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e);});
({Cyc_RemoveAggrs_remove_aggrs_stmt(s1);});
({Cyc_RemoveAggrs_remove_aggrs_stmt(s2);});
return;}case 14U: _LL13: _tmpAF=((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpA4)->f1;_tmpB0=(((struct Cyc_Absyn_Do_s_Absyn_Raw_stmt_struct*)_tmpA4)->f2).f1;_LL14: {struct Cyc_Absyn_Stmt*s2=_tmpAF;struct Cyc_Absyn_Exp*e=_tmpB0;
_tmpAD=e;_tmpAE=s2;goto _LL16;}case 5U: _LL15: _tmpAD=(((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpA4)->f1).f1;_tmpAE=((struct Cyc_Absyn_While_s_Absyn_Raw_stmt_struct*)_tmpA4)->f2;_LL16: {struct Cyc_Absyn_Exp*e=_tmpAD;struct Cyc_Absyn_Stmt*s2=_tmpAE;
# 346
({Cyc_RemoveAggrs_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e);});
({Cyc_RemoveAggrs_remove_aggrs_stmt(s2);});
return;}case 9U: _LL17: _tmpA9=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpA4)->f1;_tmpAA=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpA4)->f2).f1;_tmpAB=(((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpA4)->f3).f1;_tmpAC=((struct Cyc_Absyn_For_s_Absyn_Raw_stmt_struct*)_tmpA4)->f4;_LL18: {struct Cyc_Absyn_Exp*e1=_tmpA9;struct Cyc_Absyn_Exp*e2=_tmpAA;struct Cyc_Absyn_Exp*e3=_tmpAB;struct Cyc_Absyn_Stmt*s2=_tmpAC;
# 350
({Cyc_RemoveAggrs_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e1);});
({Cyc_RemoveAggrs_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e2);});
({Cyc_RemoveAggrs_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e3);});
({Cyc_RemoveAggrs_remove_aggrs_stmt(s2);});
return;}case 10U: _LL19: _tmpA7=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpA4)->f1;_tmpA8=((struct Cyc_Absyn_Switch_s_Absyn_Raw_stmt_struct*)_tmpA4)->f2;_LL1A: {struct Cyc_Absyn_Exp*e=_tmpA7;struct Cyc_List_List*scs=_tmpA8;
# 358
({Cyc_RemoveAggrs_remove_aggrs_exp(Cyc_RemoveAggrs_other_env,e);});
for(0;scs != 0;scs=scs->tl){
({Cyc_RemoveAggrs_remove_aggrs_stmt(((struct Cyc_Absyn_Switch_clause*)scs->hd)->body);});}
return;}case 12U: _LL1B: _tmpA5=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpA4)->f1;_tmpA6=((struct Cyc_Absyn_Decl_s_Absyn_Raw_stmt_struct*)_tmpA4)->f2;_LL1C: {struct Cyc_Absyn_Decl*d=_tmpA5;struct Cyc_Absyn_Stmt*s2=_tmpA6;
# 363
({Cyc_RemoveAggrs_remove_aggrs_stmt(s2);});
{void*_stmttmpA=d->r;void*_tmpB9=_stmttmpA;struct Cyc_Absyn_Fndecl*_tmpBA;struct Cyc_Absyn_Vardecl*_tmpBB;switch(*((int*)_tmpB9)){case 0U: _LL20: _tmpBB=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpB9)->f1;_LL21: {struct Cyc_Absyn_Vardecl*vd=_tmpBB;
# 366
if((int)vd->sc == (int)Cyc_Absyn_Static){
({Cyc_RemoveAggrs_massage_toplevel_aggr(vd->initializer);});
goto _LL1F;}
# 366
if(vd->initializer != 0){
# 378
struct Cyc_RemoveAggrs_Result _stmttmpB=({struct Cyc_RemoveAggrs_Result(*_tmpC5)(struct Cyc_RemoveAggrs_Env env,struct Cyc_Absyn_Exp*e)=Cyc_RemoveAggrs_remove_aggrs_exp;struct Cyc_RemoveAggrs_Env _tmpC6=({struct Cyc_RemoveAggrs_Env _tmpE6;_tmpE6.ctxt=Cyc_RemoveAggrs_Initializer,({struct Cyc_Absyn_Exp*_tmp104=({Cyc_Absyn_var_exp(vd->name,0U);});_tmpE6.dest=_tmp104;});_tmpE6;});struct Cyc_Absyn_Exp*_tmpC7=(struct Cyc_Absyn_Exp*)_check_null(vd->initializer);_tmpC5(_tmpC6,_tmpC7);});enum Cyc_RemoveAggrs_ExpResult _tmpBC;_LL27: _tmpBC=_stmttmpB.res;_LL28: {enum Cyc_RemoveAggrs_ExpResult r=_tmpBC;
# 380
if((int)r == (int)0U){
({void*_tmp105=({struct Cyc_Absyn_Stmt*(*_tmpBD)(struct Cyc_Absyn_Decl*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_decl_stmt;struct Cyc_Absyn_Decl*_tmpBE=d;struct Cyc_Absyn_Stmt*_tmpBF=({struct Cyc_Absyn_Stmt*(*_tmpC0)(struct Cyc_Absyn_Stmt*,struct Cyc_Absyn_Stmt*,unsigned)=Cyc_Absyn_seq_stmt;struct Cyc_Absyn_Stmt*_tmpC1=({Cyc_Absyn_exp_stmt((struct Cyc_Absyn_Exp*)_check_null(vd->initializer),0U);});struct Cyc_Absyn_Stmt*_tmpC2=s2;unsigned _tmpC3=0U;_tmpC0(_tmpC1,_tmpC2,_tmpC3);});unsigned _tmpC4=0U;_tmpBD(_tmpBE,_tmpBF,_tmpC4);})->r;s->r=_tmp105;});
# 383
vd->initializer=0;}}}
# 366
goto _LL1F;}case 1U: _LL22: _tmpBA=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpB9)->f1;_LL23: {struct Cyc_Absyn_Fndecl*fd=_tmpBA;
# 387
({Cyc_RemoveAggrs_remove_aggrs_stmt(fd->body);});goto _LL1F;}default: _LL24: _LL25:
({void*_tmpC8=0U;({int(*_tmp107)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpCA)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpCA;});struct _fat_ptr _tmp106=({const char*_tmpC9="bad local declaration after xlation to C";_tag_fat(_tmpC9,sizeof(char),41U);});_tmp107(_tmp106,_tag_fat(_tmpC8,sizeof(void*),0U));});});}_LL1F:;}
# 390
return;}default: _LL1D: _LL1E:
({void*_tmpCB=0U;({int(*_tmp109)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpCD)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpCD;});struct _fat_ptr _tmp108=({const char*_tmpCC="bad stmt after xlation to C";_tag_fat(_tmpCC,sizeof(char),28U);});_tmp109(_tmp108,_tag_fat(_tmpCB,sizeof(void*),0U));});});}_LL0:;}
# 395
struct Cyc_List_List*Cyc_RemoveAggrs_remove_aggrs(struct Cyc_List_List*ds){
{struct Cyc_List_List*ds2=ds;for(0;ds2 != 0;ds2=ds2->tl){
void*_stmttmpC=((struct Cyc_Absyn_Decl*)ds2->hd)->r;void*_tmpCF=_stmttmpC;struct Cyc_List_List*_tmpD0;struct Cyc_List_List*_tmpD1;struct Cyc_List_List*_tmpD2;struct Cyc_List_List*_tmpD3;struct Cyc_Absyn_Aggrdecl*_tmpD4;struct Cyc_Absyn_Vardecl*_tmpD5;struct Cyc_Absyn_Fndecl*_tmpD6;switch(*((int*)_tmpCF)){case 1U: _LL1: _tmpD6=((struct Cyc_Absyn_Fn_d_Absyn_Raw_decl_struct*)_tmpCF)->f1;_LL2: {struct Cyc_Absyn_Fndecl*fd=_tmpD6;
({Cyc_RemoveAggrs_remove_aggrs_stmt(fd->body);});goto _LL0;}case 0U: _LL3: _tmpD5=((struct Cyc_Absyn_Var_d_Absyn_Raw_decl_struct*)_tmpCF)->f1;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmpD5;
({Cyc_RemoveAggrs_massage_toplevel_aggr(vd->initializer);});goto _LL0;}case 5U: _LL5: _tmpD4=((struct Cyc_Absyn_Aggr_d_Absyn_Raw_decl_struct*)_tmpCF)->f1;_LL6: {struct Cyc_Absyn_Aggrdecl*ad=_tmpD4;
goto _LL8;}case 7U: _LL7: _LL8:
 goto _LLA;case 8U: _LL9: _LLA:
 goto _LL0;case 4U: _LLB: _LLC:
 goto _LLE;case 6U: _LLD: _LLE:
 goto _LL10;case 2U: _LLF: _LL10:
 goto _LL12;case 3U: _LL11: _LL12:
({void*_tmpD7=0U;({int(*_tmp10B)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpD9)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpD9;});struct _fat_ptr _tmp10A=({const char*_tmpD8="Cyclone decl after xlation to C";_tag_fat(_tmpD8,sizeof(char),32U);});_tmp10B(_tmp10A,_tag_fat(_tmpD7,sizeof(void*),0U));});});case 9U: _LL13: _tmpD3=((struct Cyc_Absyn_Namespace_d_Absyn_Raw_decl_struct*)_tmpCF)->f2;_LL14: {struct Cyc_List_List*ds2=_tmpD3;
_tmpD2=ds2;goto _LL16;}case 10U: _LL15: _tmpD2=((struct Cyc_Absyn_Using_d_Absyn_Raw_decl_struct*)_tmpCF)->f2;_LL16: {struct Cyc_List_List*ds2=_tmpD2;
_tmpD1=ds2;goto _LL18;}case 11U: _LL17: _tmpD1=((struct Cyc_Absyn_ExternC_d_Absyn_Raw_decl_struct*)_tmpCF)->f1;_LL18: {struct Cyc_List_List*ds2=_tmpD1;
_tmpD0=ds2;goto _LL1A;}case 12U: _LL19: _tmpD0=((struct Cyc_Absyn_ExternCinclude_d_Absyn_Raw_decl_struct*)_tmpCF)->f1;_LL1A: {struct Cyc_List_List*ds2=_tmpD0;
goto _LL1C;}case 13U: _LL1B: _LL1C:
 goto _LL1E;case 14U: _LL1D: _LL1E:
 goto _LL20;case 15U: _LL1F: _LL20:
 goto _LL22;default: _LL21: _LL22:
# 415
({void*_tmpDA=0U;({int(*_tmp10D)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpDC)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpDC;});struct _fat_ptr _tmp10C=({const char*_tmpDB="nested translation unit after xlation to C";_tag_fat(_tmpDB,sizeof(char),43U);});_tmp10D(_tmp10C,_tag_fat(_tmpDA,sizeof(void*),0U));});});}_LL0:;}}
# 417
return ds;}
