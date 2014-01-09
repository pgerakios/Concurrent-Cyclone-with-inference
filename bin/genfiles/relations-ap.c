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
extern struct _RegionHandle*Cyc_Core_unique_region;struct Cyc_Core_DynamicRegion;struct Cyc_Core_NewDynamicRegion{struct Cyc_Core_DynamicRegion*key;};struct Cyc_Core_ThinRes{void*arr;unsigned nelts;};struct Cyc_List_List{void*hd;struct Cyc_List_List*tl;};
# 86 "list.h"
struct Cyc_List_List*Cyc_List_rmap_c(struct _RegionHandle*,void*(*f)(void*,void*),void*env,struct Cyc_List_List*x);extern char Cyc_List_List_mismatch[14U];struct Cyc_List_List_mismatch_exn_struct{char*tag;};
# 190
struct Cyc_List_List*Cyc_List_rappend(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y);extern char Cyc_List_Nth[4U];struct Cyc_List_Nth_exn_struct{char*tag;};struct Cyc_Position_Error;struct Cyc___cycFILE;struct Cyc_String_pa_PrintArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Int_pa_PrintArg_struct{int tag;unsigned long f1;};struct Cyc_Double_pa_PrintArg_struct{int tag;double f1;};struct Cyc_LongDouble_pa_PrintArg_struct{int tag;long double f1;};struct Cyc_ShortPtr_pa_PrintArg_struct{int tag;short*f1;};struct Cyc_IntPtr_pa_PrintArg_struct{int tag;unsigned long*f1;};
# 73 "cycboot.h"
struct _fat_ptr Cyc_aprintf(struct _fat_ptr,struct _fat_ptr);
# 100
int Cyc_fprintf(struct Cyc___cycFILE*,struct _fat_ptr,struct _fat_ptr);struct Cyc_ShortPtr_sa_ScanfArg_struct{int tag;short*f1;};struct Cyc_UShortPtr_sa_ScanfArg_struct{int tag;unsigned short*f1;};struct Cyc_IntPtr_sa_ScanfArg_struct{int tag;int*f1;};struct Cyc_UIntPtr_sa_ScanfArg_struct{int tag;unsigned*f1;};struct Cyc_StringPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};struct Cyc_DoublePtr_sa_ScanfArg_struct{int tag;double*f1;};struct Cyc_FloatPtr_sa_ScanfArg_struct{int tag;float*f1;};struct Cyc_CharPtr_sa_ScanfArg_struct{int tag;struct _fat_ptr f1;};extern char Cyc_FileCloseError[15U];struct Cyc_FileCloseError_exn_struct{char*tag;};extern char Cyc_FileOpenError[14U];struct Cyc_FileOpenError_exn_struct{char*tag;struct _fat_ptr f1;};struct Cyc_Iter_Iter{void*env;int(*next)(void*env,void*dest);};struct Cyc_Dict_T;struct Cyc_Dict_Dict{int(*rel)(void*,void*);struct _RegionHandle*r;const struct Cyc_Dict_T*t;};extern char Cyc_Dict_Present[8U];struct Cyc_Dict_Present_exn_struct{char*tag;};extern char Cyc_Dict_Absent[7U];struct Cyc_Dict_Absent_exn_struct{char*tag;};struct Cyc_Relations_Reln;struct _union_Nmspace_Rel_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Abs_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_C_n{int tag;struct Cyc_List_List*val;};struct _union_Nmspace_Loc_n{int tag;int val;};union Cyc_Absyn_Nmspace{struct _union_Nmspace_Rel_n Rel_n;struct _union_Nmspace_Abs_n Abs_n;struct _union_Nmspace_C_n C_n;struct _union_Nmspace_Loc_n Loc_n;};struct _tuple0{union Cyc_Absyn_Nmspace f1;struct _fat_ptr*f2;};
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
# 997 "absyn.h"
void*Cyc_Absyn_var_type(struct Cyc_Absyn_Tvar*);
# 1022
extern void*Cyc_Absyn_fat_bound_type;
# 1026
void*Cyc_Absyn_bounds_one();
# 29 "warn.h"
void Cyc_Warn_warn(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 35
void Cyc_Warn_err(unsigned loc,struct _fat_ptr fmt,struct _fat_ptr ap);
# 40
void*Cyc_Warn_impos(struct _fat_ptr fmt,struct _fat_ptr ap);struct Cyc_Warn_String_Warn_Warg_struct{int tag;struct _fat_ptr f1;};struct Cyc_Warn_Exp_Warn_Warg_struct{int tag;struct Cyc_Absyn_Exp*f1;};struct Cyc_Warn_Typ_Warn_Warg_struct{int tag;void*f1;};struct Cyc_Warn_Qvar_Warn_Warg_struct{int tag;struct _tuple0*f1;};struct _union_RelnOp_RConst{int tag;unsigned val;};struct _union_RelnOp_RVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_RelnOp_RType{int tag;void*val;};struct _union_RelnOp_RParam{int tag;unsigned val;};struct _union_RelnOp_RParamNumelts{int tag;unsigned val;};struct _union_RelnOp_RReturn{int tag;unsigned val;};union Cyc_Relations_RelnOp{struct _union_RelnOp_RConst RConst;struct _union_RelnOp_RVar RVar;struct _union_RelnOp_RNumelts RNumelts;struct _union_RelnOp_RType RType;struct _union_RelnOp_RParam RParam;struct _union_RelnOp_RParamNumelts RParamNumelts;struct _union_RelnOp_RReturn RReturn;};
# 41 "relations-ap.h"
union Cyc_Relations_RelnOp Cyc_Relations_RConst(unsigned);union Cyc_Relations_RelnOp Cyc_Relations_RVar(struct Cyc_Absyn_Vardecl*);union Cyc_Relations_RelnOp Cyc_Relations_RNumelts(struct Cyc_Absyn_Vardecl*);union Cyc_Relations_RelnOp Cyc_Relations_RType(void*);
# 50
enum Cyc_Relations_Relation{Cyc_Relations_Req =0U,Cyc_Relations_Rneq =1U,Cyc_Relations_Rlte =2U,Cyc_Relations_Rlt =3U};struct Cyc_Relations_Reln{union Cyc_Relations_RelnOp rop1;enum Cyc_Relations_Relation relation;union Cyc_Relations_RelnOp rop2;};struct _tuple12{struct Cyc_Absyn_Exp*f1;enum Cyc_Relations_Relation f2;struct Cyc_Absyn_Exp*f3;};
# 64
struct _tuple12 Cyc_Relations_primop2relation(struct Cyc_Absyn_Exp*e1,enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e2);
# 68
enum Cyc_Relations_Relation Cyc_Relations_flip_relation(enum Cyc_Relations_Relation r);
# 70
struct Cyc_Relations_Reln*Cyc_Relations_negate(struct _RegionHandle*,struct Cyc_Relations_Reln*);
# 76
int Cyc_Relations_exp2relnop(struct Cyc_Absyn_Exp*e,union Cyc_Relations_RelnOp*p);
# 84
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*r,struct Cyc_Absyn_Exp*e);
# 87
struct Cyc_List_List*Cyc_Relations_add_relation(struct _RegionHandle*rgn,union Cyc_Relations_RelnOp rop1,enum Cyc_Relations_Relation r,union Cyc_Relations_RelnOp rop2,struct Cyc_List_List*relns);
# 100
struct Cyc_List_List*Cyc_Relations_reln_assign_var(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Vardecl*,struct Cyc_Absyn_Exp*);
# 106
struct Cyc_List_List*Cyc_Relations_reln_kill_var(struct _RegionHandle*,struct Cyc_List_List*,struct Cyc_Absyn_Vardecl*);
# 120
struct _fat_ptr Cyc_Relations_reln2string(struct Cyc_Relations_Reln*r);
struct _fat_ptr Cyc_Relations_rop2string(union Cyc_Relations_RelnOp r);
struct _fat_ptr Cyc_Relations_relation2string(enum Cyc_Relations_Relation r);
# 129
int Cyc_Relations_consistent_relations(struct Cyc_List_List*rlns);
# 29 "unify.h"
int Cyc_Unify_unify(void*,void*);struct Cyc_RgnOrder_RgnPO;extern char Cyc_Tcenv_Env_error[10U];struct Cyc_Tcenv_Env_error_exn_struct{char*tag;};struct Cyc_Tcenv_Genv{struct Cyc_Dict_Dict aggrdecls;struct Cyc_Dict_Dict datatypedecls;struct Cyc_Dict_Dict enumdecls;struct Cyc_Dict_Dict typedefs;struct Cyc_Dict_Dict ordinaries;};struct Cyc_Tcenv_Fenv;struct Cyc_Tcenv_Tenv{struct Cyc_List_List*ns;struct Cyc_Tcenv_Genv*ae;struct Cyc_Tcenv_Fenv*le;int allow_valueof: 1;int in_extern_c_include: 1;int in_tempest: 1;int tempest_generalize: 1;};
# 90 "tcenv.h"
enum Cyc_Tcenv_NewStatus{Cyc_Tcenv_NoneNew =0U,Cyc_Tcenv_InNew =1U,Cyc_Tcenv_InNewAggr =2U};
# 43 "tcutil.h"
int Cyc_Tcutil_is_integral_type(void*);
# 75
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 87
struct Cyc_Absyn_Exp*Cyc_Tcutil_get_bounds_exp(void*def,void*b);
# 110
void*Cyc_Tcutil_compress(void*);
# 144
extern struct Cyc_Absyn_Kind Cyc_Tcutil_ik;
# 178
void*Cyc_Tcutil_kind_to_bound(struct Cyc_Absyn_Kind*k);
# 261
struct Cyc_Absyn_Tvar*Cyc_Tcutil_new_tvar(void*);
# 278
int Cyc_Tcutil_is_const_exp(struct Cyc_Absyn_Exp*);
# 296
struct Cyc_Absyn_Vardecl*Cyc_Tcutil_nonesc_vardecl(void*);
# 8 "ap.h"
extern struct Cyc_AP_T*Cyc_AP_one;
# 10
struct Cyc_AP_T*Cyc_AP_fromint(long x);
# 14
struct Cyc_AP_T*Cyc_AP_neg(struct Cyc_AP_T*x);
# 16
struct Cyc_AP_T*Cyc_AP_add(struct Cyc_AP_T*x,struct Cyc_AP_T*y);
struct Cyc_AP_T*Cyc_AP_sub(struct Cyc_AP_T*x,struct Cyc_AP_T*y);struct _union_Node_NZero{int tag;int val;};struct _union_Node_NVar{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_Node_NNumelts{int tag;struct Cyc_Absyn_Vardecl*val;};struct _union_Node_NType{int tag;void*val;};struct _union_Node_NParam{int tag;unsigned val;};struct _union_Node_NParamNumelts{int tag;unsigned val;};struct _union_Node_NReturn{int tag;int val;};union Cyc_Pratt_Node{struct _union_Node_NZero NZero;struct _union_Node_NVar NVar;struct _union_Node_NNumelts NNumelts;struct _union_Node_NType NType;struct _union_Node_NParam NParam;struct _union_Node_NParamNumelts NParamNumelts;struct _union_Node_NReturn NReturn;};
# 61 "pratt-ap.h"
extern union Cyc_Pratt_Node Cyc_Pratt_zero_node;
# 63
union Cyc_Pratt_Node Cyc_Pratt_NVar(struct Cyc_Absyn_Vardecl*);
union Cyc_Pratt_Node Cyc_Pratt_NType(void*);
union Cyc_Pratt_Node Cyc_Pratt_NNumelts(struct Cyc_Absyn_Vardecl*);
union Cyc_Pratt_Node Cyc_Pratt_NParam(unsigned);
union Cyc_Pratt_Node Cyc_Pratt_NParamNumelts(unsigned);
union Cyc_Pratt_Node Cyc_Pratt_NReturn();struct Cyc_Pratt_Graph;
# 76
struct Cyc_Pratt_Graph*Cyc_Pratt_empty_graph();
struct Cyc_Pratt_Graph*Cyc_Pratt_copy_graph(struct Cyc_Pratt_Graph*);
# 81
struct Cyc_Pratt_Graph*Cyc_Pratt_add_edge(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a);struct Cyc_PP_Ppstate;struct Cyc_PP_Out;struct Cyc_PP_Doc;struct Cyc_Absynpp_Params{int expand_typedefs;int qvar_to_Cids;int add_cyc_prefix;int to_VC;int decls_first;int rewrite_temp_tvars;int print_all_tvars;int print_all_kinds;int print_all_effects;int print_using_stmts;int print_externC_stmts;int print_full_evars;int print_zeroterm;int generate_line_directives;int use_curr_namespace;struct Cyc_List_List*curr_namespace;};
# 62 "absynpp.h"
struct _fat_ptr Cyc_Absynpp_typ2string(void*);
# 67
struct _fat_ptr Cyc_Absynpp_exp2string(struct Cyc_Absyn_Exp*);
# 69
struct _fat_ptr Cyc_Absynpp_qvar2string(struct _tuple0*);struct _tuple13{unsigned f1;int f2;};
# 28 "evexp.h"
struct _tuple13 Cyc_Evexp_eval_const_uint_exp(struct Cyc_Absyn_Exp*e);
# 64 "string.h"
struct _fat_ptr Cyc_strconcat_l(struct Cyc_List_List*);
# 34 "relations-ap.cyc"
union Cyc_Relations_RelnOp Cyc_Relations_RConst(unsigned i){return({union Cyc_Relations_RelnOp _tmp128;(_tmp128.RConst).tag=1U,(_tmp128.RConst).val=i;_tmp128;});}union Cyc_Relations_RelnOp Cyc_Relations_RVar(struct Cyc_Absyn_Vardecl*v){
return({union Cyc_Relations_RelnOp _tmp129;(_tmp129.RVar).tag=2U,(_tmp129.RVar).val=v;_tmp129;});}
# 34
union Cyc_Relations_RelnOp Cyc_Relations_RNumelts(struct Cyc_Absyn_Vardecl*v){
# 36
return({union Cyc_Relations_RelnOp _tmp12A;(_tmp12A.RNumelts).tag=3U,(_tmp12A.RNumelts).val=v;_tmp12A;});}
# 34
union Cyc_Relations_RelnOp Cyc_Relations_RType(void*t){
# 37
return({union Cyc_Relations_RelnOp _tmp12B;(_tmp12B.RType).tag=4U,({void*_tmp140=({Cyc_Tcutil_compress(t);});(_tmp12B.RType).val=_tmp140;});_tmp12B;});}
# 34
union Cyc_Relations_RelnOp Cyc_Relations_RParam(unsigned i){
# 38
return({union Cyc_Relations_RelnOp _tmp12C;(_tmp12C.RParam).tag=5U,(_tmp12C.RParam).val=i;_tmp12C;});}
# 34
union Cyc_Relations_RelnOp Cyc_Relations_RParamNumelts(unsigned i){
# 39
return({union Cyc_Relations_RelnOp _tmp12D;(_tmp12D.RParamNumelts).tag=6U,(_tmp12D.RParamNumelts).val=i;_tmp12D;});}
# 34
union Cyc_Relations_RelnOp Cyc_Relations_RReturn(){
# 40
return({union Cyc_Relations_RelnOp _tmp12E;(_tmp12E.RReturn).tag=7U,(_tmp12E.RReturn).val=0U;_tmp12E;});}struct _tuple14{union Cyc_Relations_RelnOp f1;union Cyc_Relations_RelnOp f2;};
# 42
static int Cyc_Relations_same_relop(union Cyc_Relations_RelnOp r1,union Cyc_Relations_RelnOp r2){
struct _tuple14 _stmttmp0=({struct _tuple14 _tmp12F;_tmp12F.f1=r1,_tmp12F.f2=r2;_tmp12F;});struct _tuple14 _tmp7=_stmttmp0;unsigned _tmp9;unsigned _tmp8;unsigned _tmpB;unsigned _tmpA;void*_tmpD;void*_tmpC;struct Cyc_Absyn_Vardecl*_tmpF;struct Cyc_Absyn_Vardecl*_tmpE;struct Cyc_Absyn_Vardecl*_tmp11;struct Cyc_Absyn_Vardecl*_tmp10;unsigned _tmp13;unsigned _tmp12;switch(((_tmp7.f1).RParamNumelts).tag){case 1U: if(((_tmp7.f2).RConst).tag == 1){_LL1: _tmp12=((_tmp7.f1).RConst).val;_tmp13=((_tmp7.f2).RConst).val;_LL2: {unsigned c1=_tmp12;unsigned c2=_tmp13;
return c1 == c2;}}else{goto _LLF;}case 2U: if(((_tmp7.f2).RVar).tag == 2){_LL3: _tmp10=((_tmp7.f1).RVar).val;_tmp11=((_tmp7.f2).RVar).val;_LL4: {struct Cyc_Absyn_Vardecl*x1=_tmp10;struct Cyc_Absyn_Vardecl*x2=_tmp11;
return x1 == x2;}}else{goto _LLF;}case 3U: if(((_tmp7.f2).RNumelts).tag == 3){_LL5: _tmpE=((_tmp7.f1).RNumelts).val;_tmpF=((_tmp7.f2).RNumelts).val;_LL6: {struct Cyc_Absyn_Vardecl*x1=_tmpE;struct Cyc_Absyn_Vardecl*x2=_tmpF;
return x1 == x2;}}else{goto _LLF;}case 4U: if(((_tmp7.f2).RType).tag == 4){_LL7: _tmpC=((_tmp7.f1).RType).val;_tmpD=((_tmp7.f2).RType).val;_LL8: {void*t1=_tmpC;void*t2=_tmpD;
return({Cyc_Unify_unify(t1,t2);});}}else{goto _LLF;}case 5U: if(((_tmp7.f2).RParam).tag == 5){_LL9: _tmpA=((_tmp7.f1).RParam).val;_tmpB=((_tmp7.f2).RParam).val;_LLA: {unsigned i=_tmpA;unsigned j=_tmpB;
return i == j;}}else{goto _LLF;}case 6U: if(((_tmp7.f2).RParamNumelts).tag == 6){_LLB: _tmp8=((_tmp7.f1).RParamNumelts).val;_tmp9=((_tmp7.f2).RParamNumelts).val;_LLC: {unsigned i=_tmp8;unsigned j=_tmp9;
return i == j;}}else{goto _LLF;}default: if(((_tmp7.f2).RReturn).tag == 7){_LLD: _LLE:
 return 1;}else{_LLF: _LL10:
 return 0;}}_LL0:;}
# 55
struct Cyc_List_List*Cyc_Relations_add_relation(struct _RegionHandle*rgn,union Cyc_Relations_RelnOp rop1,enum Cyc_Relations_Relation relation,union Cyc_Relations_RelnOp rop2,struct Cyc_List_List*relns){
# 59
{struct Cyc_List_List*rs=relns;for(0;rs != 0;rs=rs->tl){
struct Cyc_Relations_Reln*r=(struct Cyc_Relations_Reln*)rs->hd;
if((({Cyc_Relations_same_relop(r->rop1,rop1);})&&(int)r->relation == (int)relation)&&({Cyc_Relations_same_relop(r->rop2,rop2);}))
return relns;}}
# 64
return({struct Cyc_List_List*_tmp16=_region_malloc(rgn,sizeof(*_tmp16));({struct Cyc_Relations_Reln*_tmp141=({struct Cyc_Relations_Reln*_tmp15=_region_malloc(rgn,sizeof(*_tmp15));_tmp15->rop1=rop1,_tmp15->relation=relation,_tmp15->rop2=rop2;_tmp15;});_tmp16->hd=_tmp141;}),_tmp16->tl=relns;_tmp16;});}
# 67
struct Cyc_List_List*Cyc_Relations_duplicate_relation(struct _RegionHandle*rgn,struct Cyc_Absyn_Exp*e_old,struct Cyc_Absyn_Exp*e_new,struct Cyc_List_List*relns){
# 70
union Cyc_Relations_RelnOp rop_old=({Cyc_Relations_RConst(0U);});
union Cyc_Relations_RelnOp rop_new=({Cyc_Relations_RConst(0U);});
if(!({Cyc_Relations_exp2relnop(e_old,& rop_old);}))return relns;if(!({Cyc_Relations_exp2relnop(e_new,& rop_new);})){
# 74
({void*_tmp18=0U;({unsigned _tmp143=e_new->loc;struct _fat_ptr _tmp142=({const char*_tmp19="Could not construct relation";_tag_fat(_tmp19,sizeof(char),29U);});Cyc_Warn_warn(_tmp143,_tmp142,_tag_fat(_tmp18,sizeof(void*),0U));});});
return relns;}
# 72
{
# 77
struct Cyc_List_List*rs=relns;
# 72
for(0;rs != 0;rs=rs->tl){
# 78
struct Cyc_Relations_Reln*r=(struct Cyc_Relations_Reln*)rs->hd;
union Cyc_Relations_RelnOp rop1=r->rop1;
union Cyc_Relations_RelnOp rop2=r->rop2;
int addIt=0;
if(({Cyc_Relations_same_relop(rop1,rop_old);})){
addIt=1;
rop1=rop_new;}
# 82
if(({Cyc_Relations_same_relop(rop2,rop_old);})){
# 87
addIt=1;
rop2=rop_new;}
# 82
if(addIt)
# 91
relns=({struct Cyc_List_List*_tmp1B=_region_malloc(rgn,sizeof(*_tmp1B));({struct Cyc_Relations_Reln*_tmp144=({struct Cyc_Relations_Reln*_tmp1A=_region_malloc(rgn,sizeof(*_tmp1A));_tmp1A->rop1=rop1,_tmp1A->relation=r->relation,_tmp1A->rop2=rop2;_tmp1A;});_tmp1B->hd=_tmp144;}),_tmp1B->tl=relns;_tmp1B;});}}
# 93
return relns;}
# 67
int Cyc_Relations_relns_approx(struct Cyc_List_List*r2s,struct Cyc_List_List*r1s){
# 97
if(r1s == r2s)return 1;for(0;r1s != 0;r1s=r1s->tl){
# 101
struct Cyc_Relations_Reln*r1=(struct Cyc_Relations_Reln*)r1s->hd;
int found=0;
{struct Cyc_List_List*_tmp1D=r2s;struct Cyc_List_List*r2s=_tmp1D;for(0;r2s != 0;r2s=r2s->tl){
struct Cyc_Relations_Reln*r2=(struct Cyc_Relations_Reln*)r2s->hd;
if(r1 == r2 ||(({Cyc_Relations_same_relop(r1->rop1,r2->rop1);})&&(int)r1->relation == (int)r2->relation)&&({Cyc_Relations_same_relop(r1->rop2,r2->rop2);})){
# 108
found=1;
break;}}}
# 112
if(!found)
return 0;}
# 115
return 1;}
# 118
struct Cyc_List_List*Cyc_Relations_join_relns(struct _RegionHandle*r,struct Cyc_List_List*r1s,struct Cyc_List_List*r2s){
if(r1s == r2s)return r1s;{struct Cyc_List_List*res=0;
# 121
int diff=0;
{struct Cyc_List_List*_tmp1F=r1s;struct Cyc_List_List*r1s=_tmp1F;for(0;r1s != 0;r1s=r1s->tl){
struct Cyc_Relations_Reln*r1=(struct Cyc_Relations_Reln*)r1s->hd;
int found=0;
{struct Cyc_List_List*_tmp20=r2s;struct Cyc_List_List*r2s=_tmp20;for(0;r2s != 0;r2s=r2s->tl){
struct Cyc_Relations_Reln*r2=(struct Cyc_Relations_Reln*)r2s->hd;
if(r1 == r2 ||(({Cyc_Relations_same_relop(r1->rop1,r2->rop1);})&&(int)r1->relation == (int)r2->relation)&&({Cyc_Relations_same_relop(r1->rop2,r2->rop2);})){
# 130
res=({struct Cyc_List_List*_tmp21=_region_malloc(r,sizeof(*_tmp21));_tmp21->hd=r1,_tmp21->tl=res;_tmp21;});
found=1;
break;}}}
# 135
if(!found)diff=1;}}
# 137
if(!diff)res=r1s;return res;}}
# 141
static int Cyc_Relations_rop_contains_var(union Cyc_Relations_RelnOp r,struct Cyc_Absyn_Vardecl*v){
union Cyc_Relations_RelnOp _tmp23=r;struct Cyc_Absyn_Vardecl*_tmp24;struct Cyc_Absyn_Vardecl*_tmp25;switch((_tmp23.RReturn).tag){case 2U: _LL1: _tmp25=(_tmp23.RVar).val;_LL2: {struct Cyc_Absyn_Vardecl*v2=_tmp25;
return v == v2;}case 3U: _LL3: _tmp24=(_tmp23.RNumelts).val;_LL4: {struct Cyc_Absyn_Vardecl*v2=_tmp24;
return v == v2;}case 4U: _LL5: _LL6:
 goto _LL8;case 5U: _LL7: _LL8:
 goto _LLA;case 6U: _LL9: _LLA:
 goto _LLC;case 7U: _LLB: _LLC:
 goto _LLE;default: _LLD: _LLE:
 return 0;}_LL0:;}
# 141
union Cyc_Relations_RelnOp Cyc_Relations_subst_rop_var(union Cyc_Relations_RelnOp r,struct Cyc_Absyn_Vardecl*v,union Cyc_Relations_RelnOp new_rop){
# 154
union Cyc_Relations_RelnOp _tmp27=r;struct Cyc_Absyn_Vardecl*_tmp28;struct Cyc_Absyn_Vardecl*_tmp29;switch((_tmp27.RReturn).tag){case 2U: _LL1: _tmp29=(_tmp27.RVar).val;_LL2: {struct Cyc_Absyn_Vardecl*v2=_tmp29;
return v == v2?new_rop: r;}case 3U: _LL3: _tmp28=(_tmp27.RNumelts).val;_LL4: {struct Cyc_Absyn_Vardecl*v2=_tmp28;
return v == v2?new_rop: r;}case 4U: _LL5: _LL6:
 goto _LL8;case 5U: _LL7: _LL8:
 goto _LLA;case 6U: _LL9: _LLA:
 goto _LLC;case 7U: _LLB: _LLC:
 goto _LLE;default: _LLD: _LLE:
 return r;}_LL0:;}
# 165
struct Cyc_Relations_Reln*Cyc_Relations_substitute_rop_var(struct _RegionHandle*rgn,struct Cyc_Relations_Reln*reln,struct Cyc_Absyn_Vardecl*v,union Cyc_Relations_RelnOp new_rop){
# 167
return({struct Cyc_Relations_Reln*_tmp2B=_region_malloc(rgn,sizeof(*_tmp2B));({union Cyc_Relations_RelnOp _tmp146=({Cyc_Relations_subst_rop_var(reln->rop1,v,new_rop);});_tmp2B->rop1=_tmp146;}),_tmp2B->relation=reln->relation,({
union Cyc_Relations_RelnOp _tmp145=({Cyc_Relations_subst_rop_var(reln->rop2,v,new_rop);});_tmp2B->rop2=_tmp145;});_tmp2B;});}
# 170
struct Cyc_List_List*Cyc_Relations_reln_kill_var(struct _RegionHandle*rgn,struct Cyc_List_List*rs,struct Cyc_Absyn_Vardecl*v){
struct Cyc_List_List*p;
int found=0;
for(p=rs;!found && p != 0;p=p->tl){
struct Cyc_Relations_Reln*r=(struct Cyc_Relations_Reln*)p->hd;
if(({Cyc_Relations_rop_contains_var(r->rop1,v);})||({Cyc_Relations_rop_contains_var(r->rop2,v);})){
found=1;
break;}}
# 180
if(!found)return rs;{struct Cyc_Absyn_Tvar*tv=({struct Cyc_Absyn_Tvar*(*_tmp31)(void*)=Cyc_Tcutil_new_tvar;void*_tmp32=({Cyc_Tcutil_kind_to_bound(& Cyc_Tcutil_ik);});_tmp31(_tmp32);});
# 183
union Cyc_Relations_RelnOp new_rop=({union Cyc_Relations_RelnOp(*_tmp2F)(void*t)=Cyc_Relations_RType;void*_tmp30=({Cyc_Absyn_var_type(tv);});_tmp2F(_tmp30);});
struct Cyc_List_List*new_rs=0;
for(p=rs;p != 0;p=p->tl){
struct Cyc_Relations_Reln*r=(struct Cyc_Relations_Reln*)p->hd;
if(({Cyc_Relations_rop_contains_var(r->rop1,v);})||({Cyc_Relations_rop_contains_var(r->rop2,v);}))
new_rs=({struct Cyc_List_List*_tmp2D=_region_malloc(rgn,sizeof(*_tmp2D));({struct Cyc_Relations_Reln*_tmp147=({Cyc_Relations_substitute_rop_var(rgn,r,v,new_rop);});_tmp2D->hd=_tmp147;}),_tmp2D->tl=new_rs;_tmp2D;});else{
# 190
new_rs=({struct Cyc_List_List*_tmp2E=_region_malloc(rgn,sizeof(*_tmp2E));_tmp2E->hd=r,_tmp2E->tl=new_rs;_tmp2E;});}}
# 193
return new_rs;}}
# 170
int Cyc_Relations_exp2relnop(struct Cyc_Absyn_Exp*e,union Cyc_Relations_RelnOp*p){
# 202
RETRY:
 if(e->topt != 0){
void*_stmttmp1=({Cyc_Tcutil_compress((void*)_check_null(e->topt));});void*_tmp34=_stmttmp1;struct Cyc_Absyn_Exp*_tmp35;void*_tmp36;switch(*((int*)_tmp34)){case 0U: if(((struct Cyc_Absyn_TagCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp34)->f1)->tag == 5U){if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp34)->f2 != 0){_LL1: _tmp36=(void*)(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp34)->f2)->hd;_LL2: {void*t=_tmp36;
# 206
({union Cyc_Relations_RelnOp _tmp148=({Cyc_Relations_RType(t);});*p=_tmp148;});return 1;
goto _LL0;}}else{goto _LL5;}}else{goto _LL5;}case 9U: _LL3: _tmp35=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp34)->f1;_LL4: {struct Cyc_Absyn_Exp*type_exp=_tmp35;
# 209
e=type_exp;
goto _LL0;}default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 202
{void*_stmttmp2=e->r;void*_tmp37=_stmttmp2;void*_tmp38;void*_tmp39;int _tmp3A;void*_tmp3B;struct Cyc_Absyn_Exp*_tmp3C;switch(*((int*)_tmp37)){case 14U: _LL8: _tmp3C=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp37)->f2;_LL9: {struct Cyc_Absyn_Exp*e=_tmp3C;
# 215
return({Cyc_Relations_exp2relnop(e,p);});}case 1U: _LLA: _tmp3B=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp37)->f1;_LLB: {void*b=_tmp3B;
# 217
struct Cyc_Absyn_Vardecl*v=({Cyc_Tcutil_nonesc_vardecl(b);});
if(v != 0){
({union Cyc_Relations_RelnOp _tmp149=({Cyc_Relations_RVar(v);});*p=_tmp149;});
return 1;}
# 218
goto _LL7;}case 0U: if(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp37)->f1).Int_c).tag == 5){_LLC: _tmp3A=(((((struct Cyc_Absyn_Const_e_Absyn_Raw_exp_struct*)_tmp37)->f1).Int_c).val).f2;_LLD: {int i=_tmp3A;
# 224
({union Cyc_Relations_RelnOp _tmp14A=({Cyc_Relations_RConst((unsigned)i);});*p=_tmp14A;});
return 1;}}else{goto _LL12;}case 3U: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37)->f1 == Cyc_Absyn_Numelts){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37)->f2 != 0){if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37)->f2)->hd)->r)->tag == 1U){_LLE: _tmp39=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp37)->f2)->hd)->r)->f1;_LLF: {void*b=_tmp39;
# 227
struct Cyc_Absyn_Vardecl*v=({Cyc_Tcutil_nonesc_vardecl(b);});
if(v != 0){
({union Cyc_Relations_RelnOp _tmp14B=({Cyc_Relations_RNumelts(v);});*p=_tmp14B;});
return 1;}
# 228
goto _LL7;}}else{goto _LL12;}}else{goto _LL12;}}else{goto _LL12;}case 40U: _LL10: _tmp38=(void*)((struct Cyc_Absyn_Valueof_e_Absyn_Raw_exp_struct*)_tmp37)->f1;_LL11: {void*t=_tmp38;
# 234
{void*_stmttmp3=({Cyc_Tcutil_compress(t);});void*_tmp3D=_stmttmp3;struct Cyc_Absyn_Exp*_tmp3E;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp3D)->tag == 9U){_LL15: _tmp3E=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp3D)->f1;_LL16: {struct Cyc_Absyn_Exp*type_exp=_tmp3E;
# 236
e=type_exp;
goto RETRY;}}else{_LL17: _LL18:
# 239
({union Cyc_Relations_RelnOp _tmp14C=({Cyc_Relations_RType(t);});*p=_tmp14C;});
return 1;}_LL14:;}
# 242
goto _LL7;}default: _LL12: _LL13:
 goto _LL7;}_LL7:;}
# 246
if(({Cyc_Tcutil_is_const_exp(e);})){
struct _tuple13 _stmttmp4=({Cyc_Evexp_eval_const_uint_exp(e);});int _tmp40;unsigned _tmp3F;_LL1A: _tmp3F=_stmttmp4.f1;_tmp40=_stmttmp4.f2;_LL1B: {unsigned t1=_tmp3F;int known=_tmp40;
if(known){
({union Cyc_Relations_RelnOp _tmp14D=({Cyc_Relations_RConst(t1);});*p=_tmp14D;});
return 1;}}}
# 246
return 0;}
# 256
struct Cyc_List_List*Cyc_Relations_reln_kill_exp(struct _RegionHandle*rgn,struct Cyc_List_List*r,struct Cyc_Absyn_Exp*e){
void*_stmttmp5=e->r;void*_tmp42=_stmttmp5;void*_tmp43;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp42)->tag == 1U){_LL1: _tmp43=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp42)->f1;_LL2: {void*b=_tmp43;
# 259
struct Cyc_Absyn_Vardecl*v=({Cyc_Tcutil_nonesc_vardecl(b);});
if(v != 0)
return({Cyc_Relations_reln_kill_var(rgn,r,v);});else{
return r;}}}else{_LL3: _LL4:
 return r;}_LL0:;}
# 267
struct Cyc_Relations_Reln*Cyc_Relations_copy_reln(struct _RegionHandle*r2,struct Cyc_Relations_Reln*r){
return({struct Cyc_Relations_Reln*_tmp45=_region_malloc(r2,sizeof(*_tmp45));*_tmp45=*r;_tmp45;});}
# 271
struct Cyc_List_List*Cyc_Relations_copy_relns(struct _RegionHandle*r2,struct Cyc_List_List*relns){
return({({struct Cyc_List_List*(*_tmp150)(struct _RegionHandle*,struct Cyc_Relations_Reln*(*f)(struct _RegionHandle*,struct Cyc_Relations_Reln*),struct _RegionHandle*env,struct Cyc_List_List*x)=({struct Cyc_List_List*(*_tmp47)(struct _RegionHandle*,struct Cyc_Relations_Reln*(*f)(struct _RegionHandle*,struct Cyc_Relations_Reln*),struct _RegionHandle*env,struct Cyc_List_List*x)=(struct Cyc_List_List*(*)(struct _RegionHandle*,struct Cyc_Relations_Reln*(*f)(struct _RegionHandle*,struct Cyc_Relations_Reln*),struct _RegionHandle*env,struct Cyc_List_List*x))Cyc_List_rmap_c;_tmp47;});struct _RegionHandle*_tmp14F=r2;struct _RegionHandle*_tmp14E=r2;_tmp150(_tmp14F,Cyc_Relations_copy_reln,_tmp14E,relns);});});}
# 271
int Cyc_Relations_same_relns(struct Cyc_List_List*r1,struct Cyc_List_List*r2){
# 276
for(0;r1 != 0 && r2 != 0;(r1=r1->tl,r2=r2->tl)){
struct Cyc_Relations_Reln*x1=(struct Cyc_Relations_Reln*)r1->hd;
struct Cyc_Relations_Reln*x2=(struct Cyc_Relations_Reln*)r2->hd;
if((!({Cyc_Relations_same_relop(x1->rop1,x2->rop1);})||(int)x1->relation != (int)x2->relation)|| !({Cyc_Relations_same_relop(x1->rop2,x2->rop2);}))
# 281
return 0;}
# 283
if(r1 != 0 || r2 != 0)return 0;else{
return 1;}}
# 287
struct Cyc_List_List*Cyc_Relations_reln_assign_var(struct _RegionHandle*rgn,struct Cyc_List_List*r,struct Cyc_Absyn_Vardecl*v,struct Cyc_Absyn_Exp*e){
# 291
if(v->escapes)return r;r=({Cyc_Relations_reln_kill_var(rgn,r,v);});
# 299
{void*_stmttmp6=e->r;void*_tmp4A=_stmttmp6;struct Cyc_Absyn_Exp*_tmp4B;if(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp4A)->tag == 35U){if((((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp4A)->f1).fat_result == 1){_LL1: _tmp4B=(((struct Cyc_Absyn_Malloc_e_Absyn_Raw_exp_struct*)_tmp4A)->f1).num_elts;_LL2: {struct Cyc_Absyn_Exp*e2=_tmp4B;
# 302
malloc_loop: {
void*_stmttmp7=e2->r;void*_tmp4C=_stmttmp7;void*_tmp4D;struct Cyc_Absyn_Exp*_tmp4E;switch(*((int*)_tmp4C)){case 14U: _LL6: _tmp4E=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp4C)->f2;_LL7: {struct Cyc_Absyn_Exp*e3=_tmp4E;
e2=e3;goto malloc_loop;}case 1U: _LL8: _tmp4D=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp4C)->f1;_LL9: {void*b=_tmp4D;
# 306
struct Cyc_Absyn_Vardecl*vd=({Cyc_Tcutil_nonesc_vardecl(b);});
if(vd != 0)
return({struct Cyc_List_List*(*_tmp4F)(struct _RegionHandle*rgn,union Cyc_Relations_RelnOp rop1,enum Cyc_Relations_Relation relation,union Cyc_Relations_RelnOp rop2,struct Cyc_List_List*relns)=Cyc_Relations_add_relation;struct _RegionHandle*_tmp50=rgn;union Cyc_Relations_RelnOp _tmp51=({Cyc_Relations_RVar(vd);});enum Cyc_Relations_Relation _tmp52=Cyc_Relations_Req;union Cyc_Relations_RelnOp _tmp53=({Cyc_Relations_RNumelts(v);});struct Cyc_List_List*_tmp54=r;_tmp4F(_tmp50,_tmp51,_tmp52,_tmp53,_tmp54);});else{
return r;}}default: _LLA: _LLB:
 return r;}_LL5:;}}}else{goto _LL3;}}else{_LL3: _LL4:
# 313
 goto _LL0;}_LL0:;}
# 316
if(!({Cyc_Tcutil_is_integral_type(v->type);}))return r;{union Cyc_Relations_RelnOp eop=({Cyc_Relations_RConst(0U);});
# 319
if(({Cyc_Relations_exp2relnop(e,& eop);}))
return({struct Cyc_List_List*(*_tmp55)(struct _RegionHandle*rgn,union Cyc_Relations_RelnOp rop1,enum Cyc_Relations_Relation relation,union Cyc_Relations_RelnOp rop2,struct Cyc_List_List*relns)=Cyc_Relations_add_relation;struct _RegionHandle*_tmp56=rgn;union Cyc_Relations_RelnOp _tmp57=({Cyc_Relations_RVar(v);});enum Cyc_Relations_Relation _tmp58=Cyc_Relations_Req;union Cyc_Relations_RelnOp _tmp59=eop;struct Cyc_List_List*_tmp5A=r;_tmp55(_tmp56,_tmp57,_tmp58,_tmp59,_tmp5A);});
# 319
return r;}}
# 325
struct Cyc_List_List*Cyc_Relations_reln_assign_exp(struct _RegionHandle*rgn,struct Cyc_List_List*r,struct Cyc_Absyn_Exp*e1,struct Cyc_Absyn_Exp*e2){
void*_stmttmp8=e1->r;void*_tmp5C=_stmttmp8;void*_tmp5D;if(((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp5C)->tag == 1U){_LL1: _tmp5D=(void*)((struct Cyc_Absyn_Var_e_Absyn_Raw_exp_struct*)_tmp5C)->f1;_LL2: {void*b=_tmp5D;
# 328
struct Cyc_Absyn_Vardecl*v=({Cyc_Tcutil_nonesc_vardecl(b);});
if(v != 0)
return({Cyc_Relations_reln_assign_var(rgn,r,v,e2);});else{
return r;}}}else{_LL3: _LL4:
 return r;}_LL0:;}
# 337
static union Cyc_Pratt_Node Cyc_Relations_rop2node(union Cyc_Relations_RelnOp r){
union Cyc_Relations_RelnOp _tmp5F=r;unsigned _tmp60;unsigned _tmp61;void*_tmp62;struct Cyc_Absyn_Vardecl*_tmp63;struct Cyc_Absyn_Vardecl*_tmp64;switch((_tmp5F.RReturn).tag){case 2U: _LL1: _tmp64=(_tmp5F.RVar).val;_LL2: {struct Cyc_Absyn_Vardecl*x=_tmp64;
return({Cyc_Pratt_NVar(x);});}case 3U: _LL3: _tmp63=(_tmp5F.RNumelts).val;_LL4: {struct Cyc_Absyn_Vardecl*x=_tmp63;
return({Cyc_Pratt_NNumelts(x);});}case 4U: _LL5: _tmp62=(_tmp5F.RType).val;_LL6: {void*x=_tmp62;
return({Cyc_Pratt_NType(x);});}case 5U: _LL7: _tmp61=(_tmp5F.RParam).val;_LL8: {unsigned i=_tmp61;
return({Cyc_Pratt_NParam(i);});}case 6U: _LL9: _tmp60=(_tmp5F.RParamNumelts).val;_LLA: {unsigned i=_tmp60;
return({Cyc_Pratt_NParamNumelts(i);});}case 7U: _LLB: _LLC:
 return({Cyc_Pratt_NReturn();});default: _LLD: _LLE:
({void*_tmp65=0U;({int(*_tmp152)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmp67)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmp67;});struct _fat_ptr _tmp151=({const char*_tmp66="rop2node";_tag_fat(_tmp66,sizeof(char),9U);});_tmp152(_tmp151,_tag_fat(_tmp65,sizeof(void*),0U));});});}_LL0:;}
# 337
struct _fat_ptr Cyc_Relations_relation2string(enum Cyc_Relations_Relation r){
# 350
enum Cyc_Relations_Relation _tmp69=r;switch(_tmp69){case Cyc_Relations_Req: _LL1: _LL2:
 return({const char*_tmp6A="==";_tag_fat(_tmp6A,sizeof(char),3U);});case Cyc_Relations_Rneq: _LL3: _LL4:
 return({const char*_tmp6B="!=";_tag_fat(_tmp6B,sizeof(char),3U);});case Cyc_Relations_Rlt: _LL5: _LL6:
 return({const char*_tmp6C="<";_tag_fat(_tmp6C,sizeof(char),2U);});case Cyc_Relations_Rlte: _LL7: _LL8:
 goto _LLA;default: _LL9: _LLA: return({const char*_tmp6D="<=";_tag_fat(_tmp6D,sizeof(char),3U);});}_LL0:;}
# 337
struct _fat_ptr Cyc_Relations_rop2string(union Cyc_Relations_RelnOp r){
# 359
union Cyc_Relations_RelnOp _tmp6F=r;unsigned _tmp70;unsigned _tmp71;struct Cyc_Absyn_Vardecl*_tmp72;void*_tmp73;struct Cyc_Absyn_Vardecl*_tmp74;unsigned _tmp75;switch((_tmp6F.RParamNumelts).tag){case 1U: _LL1: _tmp75=(_tmp6F.RConst).val;_LL2: {unsigned c=_tmp75;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp78=({struct Cyc_Int_pa_PrintArg_struct _tmp130;_tmp130.tag=1U,_tmp130.f1=(unsigned long)((int)c);_tmp130;});void*_tmp76[1U];_tmp76[0]=& _tmp78;({struct _fat_ptr _tmp153=({const char*_tmp77="%d";_tag_fat(_tmp77,sizeof(char),3U);});Cyc_aprintf(_tmp153,_tag_fat(_tmp76,sizeof(void*),1U));});});}case 2U: _LL3: _tmp74=(_tmp6F.RVar).val;_LL4: {struct Cyc_Absyn_Vardecl*vd=_tmp74;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp7B=({struct Cyc_String_pa_PrintArg_struct _tmp131;_tmp131.tag=0U,({struct _fat_ptr _tmp154=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp131.f1=_tmp154;});_tmp131;});void*_tmp79[1U];_tmp79[0]=& _tmp7B;({struct _fat_ptr _tmp155=({const char*_tmp7A="%s";_tag_fat(_tmp7A,sizeof(char),3U);});Cyc_aprintf(_tmp155,_tag_fat(_tmp79,sizeof(void*),1U));});});}case 4U: _LL5: _tmp73=(_tmp6F.RType).val;_LL6: {void*t=_tmp73;
return({Cyc_Absynpp_typ2string(t);});}case 3U: _LL7: _tmp72=(_tmp6F.RNumelts).val;_LL8: {struct Cyc_Absyn_Vardecl*vd=_tmp72;
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp7E=({struct Cyc_String_pa_PrintArg_struct _tmp132;_tmp132.tag=0U,({struct _fat_ptr _tmp156=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_qvar2string(vd->name);}));_tmp132.f1=_tmp156;});_tmp132;});void*_tmp7C[1U];_tmp7C[0]=& _tmp7E;({struct _fat_ptr _tmp157=({const char*_tmp7D="numelts(%s)";_tag_fat(_tmp7D,sizeof(char),12U);});Cyc_aprintf(_tmp157,_tag_fat(_tmp7C,sizeof(void*),1U));});});}case 5U: _LL9: _tmp71=(_tmp6F.RParam).val;_LLA: {unsigned i=_tmp71;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp81=({struct Cyc_Int_pa_PrintArg_struct _tmp133;_tmp133.tag=1U,_tmp133.f1=(unsigned long)((int)i);_tmp133;});void*_tmp7F[1U];_tmp7F[0]=& _tmp81;({struct _fat_ptr _tmp158=({const char*_tmp80="param(%d)";_tag_fat(_tmp80,sizeof(char),10U);});Cyc_aprintf(_tmp158,_tag_fat(_tmp7F,sizeof(void*),1U));});});}case 6U: _LLB: _tmp70=(_tmp6F.RParamNumelts).val;_LLC: {unsigned i=_tmp70;
return(struct _fat_ptr)({struct Cyc_Int_pa_PrintArg_struct _tmp84=({struct Cyc_Int_pa_PrintArg_struct _tmp134;_tmp134.tag=1U,_tmp134.f1=(unsigned long)((int)i);_tmp134;});void*_tmp82[1U];_tmp82[0]=& _tmp84;({struct _fat_ptr _tmp159=({const char*_tmp83="numelts(param(%d))";_tag_fat(_tmp83,sizeof(char),19U);});Cyc_aprintf(_tmp159,_tag_fat(_tmp82,sizeof(void*),1U));});});}default: _LLD: _LLE:
 return(struct _fat_ptr)({void*_tmp85=0U;({struct _fat_ptr _tmp15A=({const char*_tmp86="return_value";_tag_fat(_tmp86,sizeof(char),13U);});Cyc_aprintf(_tmp15A,_tag_fat(_tmp85,sizeof(void*),0U));});});}_LL0:;}
# 337
struct _fat_ptr Cyc_Relations_reln2string(struct Cyc_Relations_Reln*r){
# 371
return(struct _fat_ptr)({struct Cyc_String_pa_PrintArg_struct _tmp8A=({struct Cyc_String_pa_PrintArg_struct _tmp137;_tmp137.tag=0U,({struct _fat_ptr _tmp15B=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Relations_rop2string(r->rop1);}));_tmp137.f1=_tmp15B;});_tmp137;});struct Cyc_String_pa_PrintArg_struct _tmp8B=({struct Cyc_String_pa_PrintArg_struct _tmp136;_tmp136.tag=0U,({struct _fat_ptr _tmp15C=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Relations_relation2string(r->relation);}));_tmp136.f1=_tmp15C;});_tmp136;});struct Cyc_String_pa_PrintArg_struct _tmp8C=({struct Cyc_String_pa_PrintArg_struct _tmp135;_tmp135.tag=0U,({
struct _fat_ptr _tmp15D=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Relations_rop2string(r->rop2);}));_tmp135.f1=_tmp15D;});_tmp135;});void*_tmp88[3U];_tmp88[0]=& _tmp8A,_tmp88[1]=& _tmp8B,_tmp88[2]=& _tmp8C;({struct _fat_ptr _tmp15E=({const char*_tmp89="%s %s %s";_tag_fat(_tmp89,sizeof(char),9U);});Cyc_aprintf(_tmp15E,_tag_fat(_tmp88,sizeof(void*),3U));});});}
# 337
struct _fat_ptr Cyc_Relations_relns2string(struct Cyc_List_List*rs){
# 376
if(rs == 0)return({const char*_tmp8E="true";_tag_fat(_tmp8E,sizeof(char),5U);});{struct Cyc_List_List*ss=0;
# 378
for(0;rs != 0;rs=rs->tl){
ss=({struct Cyc_List_List*_tmp90=_cycalloc(sizeof(*_tmp90));({struct _fat_ptr*_tmp160=({struct _fat_ptr*_tmp8F=_cycalloc(sizeof(*_tmp8F));({struct _fat_ptr _tmp15F=({Cyc_Relations_reln2string((struct Cyc_Relations_Reln*)rs->hd);});*_tmp8F=_tmp15F;});_tmp8F;});_tmp90->hd=_tmp160;}),_tmp90->tl=ss;_tmp90;});
if(rs->tl != 0)ss=({struct Cyc_List_List*_tmp93=_cycalloc(sizeof(*_tmp93));({struct _fat_ptr*_tmp162=({struct _fat_ptr*_tmp92=_cycalloc(sizeof(*_tmp92));({struct _fat_ptr _tmp161=({const char*_tmp91=" && ";_tag_fat(_tmp91,sizeof(char),5U);});*_tmp92=_tmp161;});_tmp92;});_tmp93->hd=_tmp162;}),_tmp93->tl=ss;_tmp93;});}
# 382
return(struct _fat_ptr)({Cyc_strconcat_l(ss);});}}
# 385
void Cyc_Relations_print_relns(struct Cyc___cycFILE*f,struct Cyc_List_List*r){
for(0;r != 0;r=r->tl){
({struct Cyc_String_pa_PrintArg_struct _tmp97=({struct Cyc_String_pa_PrintArg_struct _tmp138;_tmp138.tag=0U,({struct _fat_ptr _tmp163=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Relations_reln2string((struct Cyc_Relations_Reln*)r->hd);}));_tmp138.f1=_tmp163;});_tmp138;});void*_tmp95[1U];_tmp95[0]=& _tmp97;({struct Cyc___cycFILE*_tmp165=f;struct _fat_ptr _tmp164=({const char*_tmp96="%s";_tag_fat(_tmp96,sizeof(char),3U);});Cyc_fprintf(_tmp165,_tmp164,_tag_fat(_tmp95,sizeof(void*),1U));});});
if(r->tl != 0)({void*_tmp98=0U;({struct Cyc___cycFILE*_tmp167=f;struct _fat_ptr _tmp166=({const char*_tmp99=",";_tag_fat(_tmp99,sizeof(char),2U);});Cyc_fprintf(_tmp167,_tmp166,_tag_fat(_tmp98,sizeof(void*),0U));});});}}
# 392
static union Cyc_Relations_RelnOp Cyc_Relations_reduce_relnop(union Cyc_Relations_RelnOp rop){
{union Cyc_Relations_RelnOp _tmp9B=rop;struct Cyc_Absyn_Vardecl*_tmp9C;void*_tmp9D;switch((_tmp9B.RNumelts).tag){case 4U: _LL1: _tmp9D=(_tmp9B.RType).val;_LL2: {void*t=_tmp9D;
# 395
{void*_stmttmp9=({Cyc_Tcutil_compress(t);});void*_tmp9E=_stmttmp9;struct Cyc_Absyn_Exp*_tmp9F;if(((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp9E)->tag == 9U){_LL8: _tmp9F=((struct Cyc_Absyn_ValueofType_Absyn_Type_struct*)_tmp9E)->f1;_LL9: {struct Cyc_Absyn_Exp*e=_tmp9F;
# 397
union Cyc_Relations_RelnOp rop2=rop;
if(({Cyc_Relations_exp2relnop(e,& rop2);}))return rop2;goto _LL7;}}else{_LLA: _LLB:
# 400
 goto _LL7;}_LL7:;}
# 402
goto _LL0;}case 3U: _LL3: _tmp9C=(_tmp9B.RNumelts).val;_LL4: {struct Cyc_Absyn_Vardecl*v=_tmp9C;
# 404
{void*_stmttmpA=({Cyc_Tcutil_compress(v->type);});void*_tmpA0=_stmttmpA;void*_tmpA1;if(((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA0)->tag == 3U){_LLD: _tmpA1=((((struct Cyc_Absyn_PointerType_Absyn_Type_struct*)_tmpA0)->f1).ptr_atts).bounds;_LLE: {void*b=_tmpA1;
# 408
if(({void*_tmp168=b;_tmp168 == ({Cyc_Absyn_bounds_one();});}))goto _LLC;{struct Cyc_Absyn_Exp*eopt=({Cyc_Tcutil_get_bounds_exp(Cyc_Absyn_fat_bound_type,b);});
# 410
if(eopt != 0){
struct Cyc_Absyn_Exp*e=eopt;
union Cyc_Relations_RelnOp rop2=rop;
if(({Cyc_Relations_exp2relnop(e,& rop2);}))return rop2;}
# 410
goto _LLC;}}}else{_LLF: _LL10:
# 416
 goto _LLC;}_LLC:;}
# 418
goto _LL0;}default: _LL5: _LL6:
 goto _LL0;}_LL0:;}
# 421
return rop;}
# 426
static int Cyc_Relations_consist_relations(struct Cyc_Pratt_Graph*G,struct Cyc_List_List*rlns){
for(0;(unsigned)G && rlns != 0;rlns=rlns->tl){
struct Cyc_Relations_Reln*reln=(struct Cyc_Relations_Reln*)rlns->hd;
({union Cyc_Relations_RelnOp _tmp169=({Cyc_Relations_reduce_relnop(reln->rop1);});reln->rop1=_tmp169;});
({union Cyc_Relations_RelnOp _tmp16A=({Cyc_Relations_reduce_relnop(reln->rop2);});reln->rop2=_tmp16A;});{
# 436
struct Cyc_Relations_Reln*_tmpA3=reln;union Cyc_Relations_RelnOp _tmpA6;enum Cyc_Relations_Relation _tmpA5;union Cyc_Relations_RelnOp _tmpA4;unsigned _tmpA9;enum Cyc_Relations_Relation _tmpA8;union Cyc_Relations_RelnOp _tmpA7;union Cyc_Relations_RelnOp _tmpAB;union Cyc_Relations_RelnOp _tmpAA;unsigned _tmpAD;union Cyc_Relations_RelnOp _tmpAC;union Cyc_Relations_RelnOp _tmpB0;enum Cyc_Relations_Relation _tmpAF;unsigned _tmpAE;union Cyc_Relations_RelnOp _tmpB2;unsigned _tmpB1;unsigned _tmpB5;enum Cyc_Relations_Relation _tmpB4;unsigned _tmpB3;if(((((struct Cyc_Relations_Reln*)_tmpA3)->rop1).RConst).tag == 1){if(((((struct Cyc_Relations_Reln*)_tmpA3)->rop2).RConst).tag == 1){_LL1: _tmpB3=((_tmpA3->rop1).RConst).val;_tmpB4=_tmpA3->relation;_tmpB5=((_tmpA3->rop2).RConst).val;_LL2: {unsigned i=_tmpB3;enum Cyc_Relations_Relation reln=_tmpB4;unsigned j=_tmpB5;
# 438
{enum Cyc_Relations_Relation _tmpB6=reln;switch(_tmpB6){case Cyc_Relations_Req: _LL10: _LL11:
 if(i != j)return 0;goto _LLF;case Cyc_Relations_Rneq: _LL12: _LL13:
 if(i == j)return 0;goto _LLF;case Cyc_Relations_Rlt: _LL14: _LL15:
 if(i >= j)return 0;goto _LLF;case Cyc_Relations_Rlte: _LL16: _LL17:
 goto _LL19;default: _LL18: _LL19: if(i > j)return 0;goto _LLF;}_LLF:;}
# 444
goto _LL0;}}else{if(((struct Cyc_Relations_Reln*)_tmpA3)->relation == Cyc_Relations_Rneq){_LL5: _tmpB1=((_tmpA3->rop1).RConst).val;_tmpB2=_tmpA3->rop2;_LL6: {unsigned i=_tmpB1;union Cyc_Relations_RelnOp rop=_tmpB2;
# 451
union Cyc_Pratt_Node n=({Cyc_Relations_rop2node(rop);});
struct Cyc_Pratt_Graph*G2=({Cyc_Pratt_copy_graph(G);});
G2=({struct Cyc_Pratt_Graph*(*_tmpB7)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpB8=G2;union Cyc_Pratt_Node _tmpB9=n;union Cyc_Pratt_Node _tmpBA=Cyc_Pratt_zero_node;struct Cyc_AP_T*_tmpBB=({struct Cyc_AP_T*(*_tmpBC)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_sub;struct Cyc_AP_T*_tmpBD=({Cyc_AP_fromint((long)i);});struct Cyc_AP_T*_tmpBE=Cyc_AP_one;_tmpBC(_tmpBD,_tmpBE);});_tmpB7(_tmpB8,_tmpB9,_tmpBA,_tmpBB);});
G=({struct Cyc_Pratt_Graph*(*_tmpBF)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpC0=G;union Cyc_Pratt_Node _tmpC1=Cyc_Pratt_zero_node;union Cyc_Pratt_Node _tmpC2=n;struct Cyc_AP_T*_tmpC3=({struct Cyc_AP_T*(*_tmpC4)(struct Cyc_AP_T*x)=Cyc_AP_neg;struct Cyc_AP_T*_tmpC5=({struct Cyc_AP_T*(*_tmpC6)(struct Cyc_AP_T*x,struct Cyc_AP_T*y)=Cyc_AP_add;struct Cyc_AP_T*_tmpC7=({Cyc_AP_fromint((long)i);});struct Cyc_AP_T*_tmpC8=Cyc_AP_one;_tmpC6(_tmpC7,_tmpC8);});_tmpC4(_tmpC5);});_tmpBF(_tmpC0,_tmpC1,_tmpC2,_tmpC3);});
# 461
if(G != 0 && G2 != 0)
return({Cyc_Relations_consist_relations(G,rlns->tl);})||({Cyc_Relations_consist_relations(G2,rlns->tl);});else{
# 464
if(G == 0)
G=G2;}
# 461
goto _LL0;}}else{_LLB: _tmpAE=((_tmpA3->rop1).RConst).val;_tmpAF=_tmpA3->relation;_tmpB0=_tmpA3->rop2;_LLC: {unsigned i=_tmpAE;enum Cyc_Relations_Relation reln=_tmpAF;union Cyc_Relations_RelnOp rop=_tmpB0;
# 495
union Cyc_Pratt_Node n=({Cyc_Relations_rop2node(rop);});
if((int)reln == (int)3U)i=i + (unsigned)1;G=({struct Cyc_Pratt_Graph*(*_tmpDF)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpE0=G;union Cyc_Pratt_Node _tmpE1=Cyc_Pratt_zero_node;union Cyc_Pratt_Node _tmpE2=n;struct Cyc_AP_T*_tmpE3=({struct Cyc_AP_T*(*_tmpE4)(struct Cyc_AP_T*x)=Cyc_AP_neg;struct Cyc_AP_T*_tmpE5=({Cyc_AP_fromint((long)i);});_tmpE4(_tmpE5);});_tmpDF(_tmpE0,_tmpE1,_tmpE2,_tmpE3);});
# 500
if((unsigned)G &&(int)reln == (int)0U)
# 505
G=({struct Cyc_Pratt_Graph*(*_tmpE6)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpE7=G;union Cyc_Pratt_Node _tmpE8=n;union Cyc_Pratt_Node _tmpE9=Cyc_Pratt_zero_node;struct Cyc_AP_T*_tmpEA=({Cyc_AP_fromint((long)i);});_tmpE6(_tmpE7,_tmpE8,_tmpE9,_tmpEA);});
# 500
goto _LL0;}}}}else{if(((struct Cyc_Relations_Reln*)_tmpA3)->relation == Cyc_Relations_Rneq){if(((((struct Cyc_Relations_Reln*)_tmpA3)->rop2).RConst).tag == 1){_LL3: _tmpAC=_tmpA3->rop1;_tmpAD=((_tmpA3->rop2).RConst).val;_LL4: {union Cyc_Relations_RelnOp rop=_tmpAC;unsigned i=_tmpAD;
# 449
_tmpB1=i;_tmpB2=rop;goto _LL6;}}else{_LL7: _tmpAA=_tmpA3->rop1;_tmpAB=_tmpA3->rop2;_LL8: {union Cyc_Relations_RelnOp rop1=_tmpAA;union Cyc_Relations_RelnOp rop2=_tmpAB;
# 468
union Cyc_Pratt_Node n1=({Cyc_Relations_rop2node(rop1);});
union Cyc_Pratt_Node n2=({Cyc_Relations_rop2node(rop2);});
struct Cyc_Pratt_Graph*G2=({Cyc_Pratt_copy_graph(G);});
G2=({struct Cyc_Pratt_Graph*(*_tmpC9)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpCA=G2;union Cyc_Pratt_Node _tmpCB=n1;union Cyc_Pratt_Node _tmpCC=n2;struct Cyc_AP_T*_tmpCD=({Cyc_AP_neg(Cyc_AP_one);});_tmpC9(_tmpCA,_tmpCB,_tmpCC,_tmpCD);});
G=({struct Cyc_Pratt_Graph*(*_tmpCE)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpCF=G;union Cyc_Pratt_Node _tmpD0=n2;union Cyc_Pratt_Node _tmpD1=n1;struct Cyc_AP_T*_tmpD2=({Cyc_AP_neg(Cyc_AP_one);});_tmpCE(_tmpCF,_tmpD0,_tmpD1,_tmpD2);});
if(G != 0 && G2 != 0)
return({Cyc_Relations_consist_relations(G,rlns->tl);})||({Cyc_Relations_consist_relations(G2,rlns->tl);});else{
# 476
if(G == 0)
G=G2;}
# 473
goto _LL0;}}}else{if(((((struct Cyc_Relations_Reln*)_tmpA3)->rop2).RConst).tag == 1){_LL9: _tmpA7=_tmpA3->rop1;_tmpA8=_tmpA3->relation;_tmpA9=((_tmpA3->rop2).RConst).val;_LLA: {union Cyc_Relations_RelnOp rop=_tmpA7;enum Cyc_Relations_Relation reln=_tmpA8;unsigned i=_tmpA9;
# 482
union Cyc_Pratt_Node n=({Cyc_Relations_rop2node(rop);});
if((int)reln == (int)3U)i=i - (unsigned)1;G=({struct Cyc_Pratt_Graph*(*_tmpD3)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpD4=G;union Cyc_Pratt_Node _tmpD5=n;union Cyc_Pratt_Node _tmpD6=Cyc_Pratt_zero_node;struct Cyc_AP_T*_tmpD7=({Cyc_AP_fromint((long)i);});_tmpD3(_tmpD4,_tmpD5,_tmpD6,_tmpD7);});
# 486
if((unsigned)G &&(int)reln == (int)0U)
G=({struct Cyc_Pratt_Graph*(*_tmpD8)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpD9=G;union Cyc_Pratt_Node _tmpDA=Cyc_Pratt_zero_node;union Cyc_Pratt_Node _tmpDB=n;struct Cyc_AP_T*_tmpDC=({struct Cyc_AP_T*(*_tmpDD)(struct Cyc_AP_T*x)=Cyc_AP_neg;struct Cyc_AP_T*_tmpDE=({Cyc_AP_fromint((long)i);});_tmpDD(_tmpDE);});_tmpD8(_tmpD9,_tmpDA,_tmpDB,_tmpDC);});
# 486
goto _LL0;}}else{_LLD: _tmpA4=_tmpA3->rop1;_tmpA5=_tmpA3->relation;_tmpA6=_tmpA3->rop2;_LLE: {union Cyc_Relations_RelnOp rop1=_tmpA4;enum Cyc_Relations_Relation reln=_tmpA5;union Cyc_Relations_RelnOp rop2=_tmpA6;
# 515
union Cyc_Pratt_Node n1=({Cyc_Relations_rop2node(rop1);});
union Cyc_Pratt_Node n2=({Cyc_Relations_rop2node(rop2);});
int i=(int)reln == (int)3U?- 1: 0;
# 520
G=({struct Cyc_Pratt_Graph*(*_tmpEB)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpEC=G;union Cyc_Pratt_Node _tmpED=n1;union Cyc_Pratt_Node _tmpEE=n2;struct Cyc_AP_T*_tmpEF=({Cyc_AP_fromint(i);});_tmpEB(_tmpEC,_tmpED,_tmpEE,_tmpEF);});
# 522
if((unsigned)G &&(int)reln == (int)0U)
G=({struct Cyc_Pratt_Graph*(*_tmpF0)(struct Cyc_Pratt_Graph*G,union Cyc_Pratt_Node i,union Cyc_Pratt_Node j,struct Cyc_AP_T*a)=Cyc_Pratt_add_edge;struct Cyc_Pratt_Graph*_tmpF1=G;union Cyc_Pratt_Node _tmpF2=n2;union Cyc_Pratt_Node _tmpF3=n1;struct Cyc_AP_T*_tmpF4=({Cyc_AP_fromint(i);});_tmpF0(_tmpF1,_tmpF2,_tmpF3,_tmpF4);});
# 522
goto _LL0;}}}}_LL0:;}}
# 527
if(G != 0)return 1;else{return 0;}}
# 426
int Cyc_Relations_consistent_relations(struct Cyc_List_List*rlns){
# 541
struct Cyc_Pratt_Graph*G=({Cyc_Pratt_empty_graph();});
return({Cyc_Relations_consist_relations(G,rlns);});}
# 426
int Cyc_Relations_check_logical_implication(struct Cyc_List_List*r1,struct Cyc_List_List*r2){
# 548
for(0;r2 != 0;r2=r2->tl){
struct Cyc_Relations_Reln*r=({Cyc_Relations_negate(Cyc_Core_heap_region,(struct Cyc_Relations_Reln*)r2->hd);});
struct Cyc_List_List*relns=({struct Cyc_List_List*_tmpF7=_cycalloc(sizeof(*_tmpF7));_tmpF7->hd=r,_tmpF7->tl=r1;_tmpF7;});
if(({Cyc_Relations_consistent_relations(relns);}))return 0;}
# 553
return 1;}
# 559
struct _tuple12 Cyc_Relations_primop2relation(struct Cyc_Absyn_Exp*e1,enum Cyc_Absyn_Primop p,struct Cyc_Absyn_Exp*e2){
# 561
enum Cyc_Absyn_Primop _tmpF9=p;switch(_tmpF9){case Cyc_Absyn_Eq: _LL1: _LL2:
 return({struct _tuple12 _tmp139;_tmp139.f1=e1,_tmp139.f2=Cyc_Relations_Req,_tmp139.f3=e2;_tmp139;});case Cyc_Absyn_Neq: _LL3: _LL4:
 return({struct _tuple12 _tmp13A;_tmp13A.f1=e1,_tmp13A.f2=Cyc_Relations_Rneq,_tmp13A.f3=e2;_tmp13A;});case Cyc_Absyn_Lt: _LL5: _LL6:
 return({struct _tuple12 _tmp13B;_tmp13B.f1=e1,_tmp13B.f2=Cyc_Relations_Rlt,_tmp13B.f3=e2;_tmp13B;});case Cyc_Absyn_Lte: _LL7: _LL8:
 return({struct _tuple12 _tmp13C;_tmp13C.f1=e1,_tmp13C.f2=Cyc_Relations_Rlte,_tmp13C.f3=e2;_tmp13C;});case Cyc_Absyn_Gt: _LL9: _LLA:
 return({struct _tuple12 _tmp13D;_tmp13D.f1=e2,_tmp13D.f2=Cyc_Relations_Rlt,_tmp13D.f3=e1;_tmp13D;});case Cyc_Absyn_Gte: _LLB: _LLC:
 return({struct _tuple12 _tmp13E;_tmp13E.f1=e2,_tmp13E.f2=Cyc_Relations_Rlte,_tmp13E.f3=e1;_tmp13E;});default: _LLD: _LLE:
({void*_tmpFA=0U;({int(*_tmp16C)(struct _fat_ptr fmt,struct _fat_ptr ap)=({int(*_tmpFC)(struct _fat_ptr fmt,struct _fat_ptr ap)=(int(*)(struct _fat_ptr fmt,struct _fat_ptr ap))Cyc_Warn_impos;_tmpFC;});struct _fat_ptr _tmp16B=({const char*_tmpFB="primop2relation";_tag_fat(_tmpFB,sizeof(char),16U);});_tmp16C(_tmp16B,_tag_fat(_tmpFA,sizeof(void*),0U));});});}_LL0:;}
# 559
enum Cyc_Relations_Relation Cyc_Relations_flip_relation(enum Cyc_Relations_Relation r){
# 574
enum Cyc_Relations_Relation _tmpFE=r;switch(_tmpFE){case Cyc_Relations_Req: _LL1: _LL2:
 return Cyc_Relations_Rneq;case Cyc_Relations_Rneq: _LL3: _LL4:
 return Cyc_Relations_Req;case Cyc_Relations_Rlt: _LL5: _LL6:
 return Cyc_Relations_Rlte;case Cyc_Relations_Rlte: _LL7: _LL8:
 goto _LLA;default: _LL9: _LLA: return Cyc_Relations_Rlt;}_LL0:;}
# 582
struct Cyc_Relations_Reln*Cyc_Relations_negate(struct _RegionHandle*r,struct Cyc_Relations_Reln*rln){
return({struct Cyc_Relations_Reln*_tmp100=_region_malloc(r,sizeof(*_tmp100));_tmp100->rop1=rln->rop2,({enum Cyc_Relations_Relation _tmp16D=({Cyc_Relations_flip_relation(rln->relation);});_tmp100->relation=_tmp16D;}),_tmp100->rop2=rln->rop1;_tmp100;});}
# 593
struct Cyc_List_List*Cyc_Relations_exp2relns(struct _RegionHandle*r,struct Cyc_Absyn_Exp*e){
# 595
{void*_stmttmpB=e->r;void*_tmp102=_stmttmpB;struct Cyc_Absyn_Exp*_tmp105;struct Cyc_Absyn_Exp*_tmp104;enum Cyc_Absyn_Primop _tmp103;struct Cyc_Absyn_Exp*_tmp108;struct Cyc_Absyn_Exp*_tmp107;enum Cyc_Absyn_Primop _tmp106;struct Cyc_Absyn_Exp*_tmp10A;struct Cyc_Absyn_Exp*_tmp109;struct Cyc_Absyn_Exp*_tmp10C;void*_tmp10B;switch(*((int*)_tmp102)){case 14U: _LL1: _tmp10B=(void*)((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp102)->f1;_tmp10C=((struct Cyc_Absyn_Cast_e_Absyn_Raw_exp_struct*)_tmp102)->f2;_LL2: {void*t=_tmp10B;struct Cyc_Absyn_Exp*e=_tmp10C;
# 598
{void*_stmttmpC=({Cyc_Tcutil_compress(t);});void*_tmp10D=_stmttmpC;if(((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10D)->tag == 0U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10D)->f1)->tag == 1U){if(((struct Cyc_Absyn_IntCon_Absyn_TyCon_struct*)((struct Cyc_Absyn_AppType_Absyn_Type_struct*)_tmp10D)->f1)->f2 == Cyc_Absyn_Int_sz){_LLC: _LLD:
 return({Cyc_Relations_exp2relns(r,e);});}else{goto _LLE;}}else{goto _LLE;}}else{_LLE: _LLF:
 goto _LLB;}_LLB:;}
# 602
goto _LL0;}case 7U: _LL3: _tmp109=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp102)->f1;_tmp10A=((struct Cyc_Absyn_And_e_Absyn_Raw_exp_struct*)_tmp102)->f2;_LL4: {struct Cyc_Absyn_Exp*e1=_tmp109;struct Cyc_Absyn_Exp*e2=_tmp10A;
return({struct Cyc_List_List*(*_tmp10E)(struct _RegionHandle*,struct Cyc_List_List*x,struct Cyc_List_List*y)=Cyc_List_rappend;struct _RegionHandle*_tmp10F=r;struct Cyc_List_List*_tmp110=({Cyc_Relations_exp2relns(r,e1);});struct Cyc_List_List*_tmp111=({Cyc_Relations_exp2relns(r,e2);});_tmp10E(_tmp10F,_tmp110,_tmp111);});}case 3U: if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f1 == Cyc_Absyn_Not){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2 != 0){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->hd)->r)->tag == 3U){if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->hd)->r)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->hd)->r)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->hd)->r)->f2)->tl)->tl == 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl == 0){_LL5: _tmp106=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->hd)->r)->f1;_tmp107=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->hd)->r)->f2)->hd;_tmp108=(struct Cyc_Absyn_Exp*)((((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)((struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->hd)->r)->f2)->tl)->hd;_LL6: {enum Cyc_Absyn_Primop p=_tmp106;struct Cyc_Absyn_Exp*e1=_tmp107;struct Cyc_Absyn_Exp*e2=_tmp108;
# 605
{enum Cyc_Absyn_Primop _tmp112=p;switch(_tmp112){case Cyc_Absyn_Eq: _LL11: _LL12:
 goto _LL14;case Cyc_Absyn_Neq: _LL13: _LL14: goto _LL16;case Cyc_Absyn_Lt: _LL15: _LL16: goto _LL18;case Cyc_Absyn_Lte: _LL17: _LL18: goto _LL1A;case Cyc_Absyn_Gt: _LL19: _LL1A: goto _LL1C;case Cyc_Absyn_Gte: _LL1B: _LL1C: {
struct _tuple12 _stmttmpD=({Cyc_Relations_primop2relation(e1,p,e2);});struct Cyc_Absyn_Exp*_tmp115;enum Cyc_Relations_Relation _tmp114;struct Cyc_Absyn_Exp*_tmp113;_LL20: _tmp113=_stmttmpD.f1;_tmp114=_stmttmpD.f2;_tmp115=_stmttmpD.f3;_LL21: {struct Cyc_Absyn_Exp*e1=_tmp113;enum Cyc_Relations_Relation rln=_tmp114;struct Cyc_Absyn_Exp*e2=_tmp115;
union Cyc_Relations_RelnOp rop1=({Cyc_Relations_RConst(0U);});
union Cyc_Relations_RelnOp rop2=({Cyc_Relations_RConst(0U);});
if(({Cyc_Relations_exp2relnop(e1,& rop1);})&&({Cyc_Relations_exp2relnop(e2,& rop2);}))
return({struct Cyc_List_List*_tmp117=_region_malloc(r,sizeof(*_tmp117));({struct Cyc_Relations_Reln*_tmp16F=({struct Cyc_Relations_Reln*_tmp116=_region_malloc(r,sizeof(*_tmp116));_tmp116->rop1=rop2,({enum Cyc_Relations_Relation _tmp16E=({Cyc_Relations_flip_relation(rln);});_tmp116->relation=_tmp16E;}),_tmp116->rop2=rop1;_tmp116;});_tmp117->hd=_tmp16F;}),_tmp117->tl=0;_tmp117;});
# 610
goto _LL10;}}default: _LL1D: _LL1E:
# 613
 goto _LL10;}_LL10:;}
# 615
goto _LL0;}}else{if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl)->tl == 0)goto _LL7;else{goto _LL9;}}}else{if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl)->tl == 0)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl)->tl == 0)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl)->tl == 0)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl)->tl == 0)goto _LL7;else{goto _LL9;}}else{goto _LL9;}}}else{goto _LL9;}}else{if(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2 != 0){if(((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl != 0){if(((struct Cyc_List_List*)((struct Cyc_List_List*)((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl)->tl == 0){_LL7: _tmp103=((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f1;_tmp104=(struct Cyc_Absyn_Exp*)(((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->hd;_tmp105=(struct Cyc_Absyn_Exp*)((((struct Cyc_Absyn_Primop_e_Absyn_Raw_exp_struct*)_tmp102)->f2)->tl)->hd;_LL8: {enum Cyc_Absyn_Primop p=_tmp103;struct Cyc_Absyn_Exp*e1=_tmp104;struct Cyc_Absyn_Exp*e2=_tmp105;
# 617
{enum Cyc_Absyn_Primop _tmp118=p;switch(_tmp118){case Cyc_Absyn_Eq: _LL23: _LL24:
 goto _LL26;case Cyc_Absyn_Neq: _LL25: _LL26: goto _LL28;case Cyc_Absyn_Lt: _LL27: _LL28: goto _LL2A;case Cyc_Absyn_Lte: _LL29: _LL2A: goto _LL2C;case Cyc_Absyn_Gt: _LL2B: _LL2C: goto _LL2E;case Cyc_Absyn_Gte: _LL2D: _LL2E: {
struct _tuple12 _stmttmpE=({Cyc_Relations_primop2relation(e1,p,e2);});struct Cyc_Absyn_Exp*_tmp11B;enum Cyc_Relations_Relation _tmp11A;struct Cyc_Absyn_Exp*_tmp119;_LL32: _tmp119=_stmttmpE.f1;_tmp11A=_stmttmpE.f2;_tmp11B=_stmttmpE.f3;_LL33: {struct Cyc_Absyn_Exp*e1=_tmp119;enum Cyc_Relations_Relation rln=_tmp11A;struct Cyc_Absyn_Exp*e2=_tmp11B;
union Cyc_Relations_RelnOp rop1=({Cyc_Relations_RConst(0U);});
union Cyc_Relations_RelnOp rop2=({Cyc_Relations_RConst(0U);});
if(({Cyc_Relations_exp2relnop(e1,& rop1);})&&({Cyc_Relations_exp2relnop(e2,& rop2);}))
return({struct Cyc_List_List*_tmp11D=_region_malloc(r,sizeof(*_tmp11D));({struct Cyc_Relations_Reln*_tmp170=({struct Cyc_Relations_Reln*_tmp11C=_region_malloc(r,sizeof(*_tmp11C));_tmp11C->rop1=rop1,_tmp11C->relation=rln,_tmp11C->rop2=rop2;_tmp11C;});_tmp11D->hd=_tmp170;}),_tmp11D->tl=0;_tmp11D;});
# 622
goto _LL22;}}default: _LL2F: _LL30:
# 625
 goto _LL22;}_LL22:;}
# 627
goto _LL0;}}else{goto _LL9;}}else{goto _LL9;}}else{goto _LL9;}}default: _LL9: _LLA:
 goto _LL0;}_LL0:;}
# 631
if(({Cyc_Tcutil_is_const_exp(e);})){
struct _tuple13 _stmttmpF=({Cyc_Evexp_eval_const_uint_exp(e);});int _tmp11F;unsigned _tmp11E;_LL35: _tmp11E=_stmttmpF.f1;_tmp11F=_stmttmpF.f2;_LL36: {unsigned i=_tmp11E;int known=_tmp11F;
if(known){
if((int)i)return 0;else{
# 636
return({struct Cyc_List_List*_tmp121=_region_malloc(r,sizeof(*_tmp121));({struct Cyc_Relations_Reln*_tmp173=({struct Cyc_Relations_Reln*_tmp120=_region_malloc(r,sizeof(*_tmp120));({union Cyc_Relations_RelnOp _tmp172=({Cyc_Relations_RConst(0U);});_tmp120->rop1=_tmp172;}),_tmp120->relation=Cyc_Relations_Rlt,({union Cyc_Relations_RelnOp _tmp171=({Cyc_Relations_RConst(0U);});_tmp120->rop2=_tmp171;});_tmp120;});_tmp121->hd=_tmp173;}),_tmp121->tl=0;_tmp121;});}}
# 633
({struct Cyc_String_pa_PrintArg_struct _tmp124=({struct Cyc_String_pa_PrintArg_struct _tmp13F;_tmp13F.tag=0U,({
# 639
struct _fat_ptr _tmp174=(struct _fat_ptr)((struct _fat_ptr)({Cyc_Absynpp_exp2string(e);}));_tmp13F.f1=_tmp174;});_tmp13F;});void*_tmp122[1U];_tmp122[0]=& _tmp124;({unsigned _tmp176=e->loc;struct _fat_ptr _tmp175=({const char*_tmp123="unable to convert `%s' to static relation";_tag_fat(_tmp123,sizeof(char),42U);});Cyc_Warn_err(_tmp176,_tmp175,_tag_fat(_tmp122,sizeof(void*),1U));});});}}
# 631
return({struct Cyc_List_List*_tmp126=_region_malloc(r,sizeof(*_tmp126));
# 642
({struct Cyc_Relations_Reln*_tmp179=({struct Cyc_Relations_Reln*_tmp125=_region_malloc(r,sizeof(*_tmp125));({union Cyc_Relations_RelnOp _tmp178=({Cyc_Relations_RConst(0U);});_tmp125->rop1=_tmp178;}),_tmp125->relation=Cyc_Relations_Rlt,({union Cyc_Relations_RelnOp _tmp177=({Cyc_Relations_RConst(0U);});_tmp125->rop2=_tmp177;});_tmp125;});_tmp126->hd=_tmp179;}),_tmp126->tl=0;_tmp126;});}
